/*	$OpenBSD: pcibios.c,v 1.18 2001/01/24 23:16:14 mickey Exp $	*/
/*	$NetBSD: pcibios.c,v 1.5 2000/08/01 05:23:59 uch Exp $	*/

/*
 * Copyright (c) 2000 Michael Shalayeff
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Michael Shalayeff.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR HIS RELATIVES BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF MIND, USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */
/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Copyright (c) 1999, by UCHIYAMA Yasushi
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the developer may NOT be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND 
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE 
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL 
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS 
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) 
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT 
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY 
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF 
 * SUCH DAMAGE. 
 */

/*
 * Interface to the PCI BIOS and PCI Interrupt Routing table.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <dev/isa/isareg.h>
#include <i386/isa/isa_machdep.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

#include <i386/pci/pcibiosvar.h>
#include <i386/pci/pci_intr_fixup.h>
#include <i386/pci/pci_addr_fixup.h>

#include <machine/biosvar.h>

#ifdef PCIBIOSVERBOSE
int	pcibiosverbose = 1;
#endif

int pcibios_present;

struct pcibios_pir_header pcibios_pir_header;
struct pcibios_intr_routing *pcibios_pir_table;
int pcibios_pir_table_nentries;
int pcibios_max_bus;

struct bios32_entry pcibios_entry;
struct bios32_entry_info pcibios_entry_info;

struct pcibios_softc {
	struct  device sc_dev;
};

struct pcibios_intr_routing *pcibios_pir_init __P((struct pcibios_softc *));

int	pcibios_get_status __P((struct pcibios_softc *,
	    u_int32_t *, u_int32_t *, u_int32_t *,
	    u_int32_t *, u_int32_t *, u_int32_t *, u_int32_t *));
int	pcibios_get_intr_routing __P((struct pcibios_softc *,
	    struct pcibios_intr_routing *, int *, u_int16_t *));

int	pcibios_return_code __P((struct pcibios_softc *, u_int16_t, const char *));

void	pcibios_print_exclirq __P((struct pcibios_softc *));
#ifdef PCIINTR_DEBUG
void	pcibios_print_pir_table __P((void));
#endif

#define	PCI_IRQ_TABLE_START	0xf0000
#define	PCI_IRQ_TABLE_END	0xfffff

struct cfdriver pcibios_cd = {
	NULL, "pcibios", DV_DULL
};

int pcibiosprobe __P((struct device *, void *, void *));
void pcibiosattach __P((struct device *, struct device *, void *));

struct cfattach pcibios_ca = {
	sizeof(struct pcibios_softc), pcibiosprobe, pcibiosattach
};

int
pcibiosprobe(parent, match, aux)
	struct device *parent;
	void *match, *aux;
{
	struct bios_attach_args *ba = aux;
	u_int32_t rev_maj, rev_min, mech1, mech2, scmech1, scmech2, maxbus;
	int rv;

	if (strcmp(ba->bios_dev, "pcibios"))
		return 0;

	rv = bios32_service(PCIBIOS_SIGNATURE, &pcibios_entry,
		&pcibios_entry_info);

#ifdef PCIBIOSVERBOSE
	printf("pcibiosprobe: 0x%lx:0x%lx at 0x%lx[0x%lx]\n",
	    pcibios_entry.segment,
	    pcibios_entry.offset,
	    pcibios_entry_info.bei_base,
	    pcibios_entry_info.bei_size);
#endif

	return rv &&
	    pcibios_get_status(NULL, &rev_maj, &rev_min, &mech1, &mech2,
	        &scmech1, &scmech2, &maxbus) == PCIBIOS_SUCCESS;
}

int pcibios_flags = -1;

void
pcibiosattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct pcibios_softc *sc = (struct pcibios_softc *)self;
	u_int32_t rev_maj, rev_min, mech1, mech2, scmech1, scmech2;

	pcibios_flags = sc->sc_dev.dv_cfdata->cf_flags;

	pcibios_get_status((struct pcibios_softc *)self, &rev_maj,
	    &rev_min, &mech1, &mech2,
	    &scmech1, &scmech2, &pcibios_max_bus);

	printf(": rev. %d.%d found at 0x%lx[0x%lx]\n",
	    rev_maj, rev_min >> 4, pcibios_entry_info.bei_base,
	    pcibios_entry_info.bei_size);
#ifdef PCIBIOSVERBOSE
	printf("%s: config mechanism %s%s, special cycles %s%s, last bus %d\n",
	    sc->sc_dev.dv_xname,
	    mech1 ? "[1]" : "[x]",
	    mech2 ? "[2]" : "[x]",
	    scmech1 ? "[1]" : "[x]",
	    scmech2 ? "[2]" : "[x]",
	    pcibios_max_bus);
#endif

	/*
	 * The PCI BIOS tells us the config mechanism; fill it in now
	 * so that pci_mode_detect() doesn't have to look for it.
	 */
	pci_mode = mech1 ? 1 : 2;

	pcibios_present = 1;

	/*
	 * Find the PCI IRQ Routing table.
	 */

	if (!(pcibios_flags & PCIBIOS_INTR_FIXUP) &&
	    pcibios_pir_init((struct pcibios_softc *)self) != NULL) {
		int rv;
		u_int16_t pciirq;

		/*
		 * Fixup interrupt routing.
		 */
		rv = pci_intr_fixup(NULL, I386_BUS_SPACE_IO, &pciirq);
		switch (rv) {
		case -1:
			/* Non-fatal error. */
			printf("%s: Warning, unable to fix up PCI interrupt "
			    "routing\n", sc->sc_dev.dv_xname);
			break;

		case 1:
			/* Fatal error. */
			printf("%s: interrupt fixup failed\n", sc->sc_dev.dv_xname);
			return;
		}

		/*
		 * XXX Clear `pciirq' from the ISA interrupt allocation
		 * XXX mask.
		 */
	}

	if (!(pcibios_flags & PCIBIOS_BUS_FIXUP)) {
		pcibios_max_bus = pci_bus_fixup(NULL, 0);
		printf("%s: PCI bus #%d is the last bus\n",
		    sc->sc_dev.dv_xname, pcibios_max_bus);
	}

	if (!(pcibios_flags & PCIBIOS_ADDR_FIXUP))
		pci_addr_fixup(NULL, pcibios_max_bus);
}

struct pcibios_intr_routing *
pcibios_pir_init(sc)
	struct pcibios_softc *sc;
{
	paddr_t pa;

	pcibios_pir_table = NULL;
	for (pa = PCI_IRQ_TABLE_START; pa < PCI_IRQ_TABLE_END; pa += 16) {
		char devinfo[256];
		u_int8_t *p, cksum;
		struct pcibios_pir_header *pirh;
		int i;

		pirh = (struct pcibios_pir_header *)p = ISA_HOLE_VADDR(pa);
		if (pirh->signature != BIOS32_MAKESIG('$', 'P', 'I', 'R'))
			continue;
		
		cksum = 0;
		for (i = 0; i < pirh->tablesize; i++)
			cksum += p[i];

		printf("%s: PCI IRQ Routing Table rev. %d.%d found at 0x%lx, "
		    "size %d bytes (%d entries)\n", sc->sc_dev.dv_xname,
		    pirh->version >> 8, pirh->version & 0xff, pa,
		    pirh->tablesize, (pirh->tablesize - 32) / 16);

		if (cksum != 0) {
			printf("%s: bad IRQ table checksum\n",
			    sc->sc_dev.dv_xname);
			continue;
		}

		if (pirh->tablesize < 32 || (pirh->tablesize % 16) != 0) {
			printf("%s: bad IRQ table size\n", sc->sc_dev.dv_xname);
			continue;
		}

		if (pirh->version != 0x0100) {
			printf("%s: unsupported IRQ table version\n",
			    sc->sc_dev.dv_xname);
			continue;
		}

		/*
		 * We can handle this table!  Make a copy of it.
		 */
		pcibios_pir_header = *pirh;
		pcibios_pir_table =
		    malloc(pirh->tablesize - 32, M_DEVBUF, M_NOWAIT);
		if (pcibios_pir_table == NULL) {
			printf("%s: no memory for $PIR\n", sc->sc_dev.dv_xname);
			return NULL;
		}
		bcopy(p + 32, pcibios_pir_table, pirh->tablesize - 32);
		pcibios_pir_table_nentries = (pirh->tablesize - 32) / 16;

		printf("%s: PCI Interrupt Router at %03d:%02d:%01d",
		    sc->sc_dev.dv_xname, pirh->router_bus,
		    PIR_DEVFUNC_DEVICE(pirh->router_devfunc),
		    PIR_DEVFUNC_FUNCTION(pirh->router_devfunc));
		if (pirh->compat_router != 0) {
			pci_devinfo(pirh->compat_router, 0, 0, devinfo);
			printf(" (%s)", devinfo);
		}
		printf("\n");
	}

	/*
	 * If there was no PIR table found, try using the PCI BIOS
	 * Get Interrupt Routing call.
	 *
	 * XXX The interface to this call sucks; just allocate enough
	 * XXX room for 32 entries.
	 */
	if (pcibios_pir_table == NULL) {

		pcibios_pir_table_nentries = 32;
		pcibios_pir_table = malloc(pcibios_pir_table_nentries *
		    sizeof(*pcibios_pir_table), M_DEVBUF, M_NOWAIT);
		if (pcibios_pir_table == NULL) {
			printf("%s: no memory for $PIR\n", sc->sc_dev.dv_xname);
			return NULL;
		}
		if (pcibios_get_intr_routing(sc, pcibios_pir_table,
		    &pcibios_pir_table_nentries,
		    &pcibios_pir_header.exclusive_irq) != PCIBIOS_SUCCESS) {
			printf("%s: PCI IRQ Routing information unavailable.\n",
			    sc->sc_dev.dv_xname);
			free(pcibios_pir_table, M_DEVBUF);
			pcibios_pir_table = NULL;
			pcibios_pir_table_nentries = 0;
			return NULL;
		}
		printf("%s: PCI BIOS has %d Interrupt Routing table entries\n",
		    sc->sc_dev.dv_xname, pcibios_pir_table_nentries);
	}

	pcibios_print_exclirq(sc);
#ifdef PCIINTR_DEBUG
	pcibios_print_pir_table();
#endif
	return pcibios_pir_table;
}

int
pcibios_get_status(sc, rev_maj, rev_min, mech1, mech2, scmech1, scmech2, maxbus)
	struct pcibios_softc *sc;
	u_int32_t *rev_maj, *rev_min, *mech1, *mech2, *scmech1, *scmech2,
	    *maxbus;
{
	u_int32_t ax, bx, cx, edx;
	int rv;

	__asm __volatile("pushl	%%es\n\t"
			 "pushl	%%ds\n\t"
			 "movw	4(%%edi), %%ecx\n\t"
			 "movl	%%ecx, %%ds\n\t"
			 "lcall	%%cs:(%%edi)\n\t"
			 "pop	%%ds\n\t"
			 "pop	%%es\n\t"
			 "jc	1f\n\t"
			 "xor	%%ah, %%ah\n"
		    "1:"
		: "=a" (ax), "=b" (bx), "=c" (cx), "=d" (edx)
		: "0" (0xb101), "D" (&pcibios_entry)
		: "cc", "memory");

	rv = pcibios_return_code(sc, ax, "pcibios_get_status");
	if (rv != PCIBIOS_SUCCESS)
		return (rv);

	if (edx != BIOS32_MAKESIG('P', 'C', 'I', ' '))
		return (PCIBIOS_SERVICE_NOT_PRESENT);	/* XXX */

	/*
	 * Fill in the various pieces if info we're looking for.
	 */
	*mech1 = ax & 1;
	*mech2 = ax & (1 << 1);
	*scmech1 = ax & (1 << 4);
	*scmech2 = ax & (1 << 5);
	*rev_maj = (bx >> 8) & 0xff;
	*rev_min = bx & 0xff;
	*maxbus = cx & 0xff;

	return (PCIBIOS_SUCCESS);
}

int
pcibios_get_intr_routing(sc, table, nentries, exclirq)
	struct pcibios_softc *sc;
	struct pcibios_intr_routing *table;
	int *nentries;
	u_int16_t *exclirq;
{
	u_int32_t ax, bx;
	int rv;
	struct {
		u_int16_t size;
		u_int32_t offset;
		u_int16_t segment;
	} __attribute__((__packed__)) args;

	args.size = *nentries * sizeof(*table);
	args.offset = (u_int32_t)table;
	args.segment = GSEL(GDATA_SEL, SEL_KPL);

	memset(table, 0, args.size);

	__asm __volatile("pushl	%%es\n\t"
			 "pushl	%%ds\n\t"
			 "movw	4(%%esi), %%ecx\n\t"
			 "movl	%%ecx, %%ds\n\t"
			 "lcall	%%cs:(%%esi)\n\t"
			 "popl	%%ds\n\t"
			 "popl	%%es\n\t"
			 "jc	1f\n\t"
			 "xor	%%ah, %%ah\n"
		    "1:\n"
		: "=a" (ax), "=b" (bx)
		: "0" (0xb10e), "1" (0), "D" (&args), "S" (&pcibios_entry)
		: "%ecx", "%edx", "cc", "memory");

	rv = pcibios_return_code(sc, ax, "pcibios_get_intr_routing");
	if (rv != PCIBIOS_SUCCESS)
		return (rv);

	*nentries = args.size / sizeof(*table);
	*exclirq = bx;

	return (PCIBIOS_SUCCESS);
}

int
pcibios_return_code(sc, ax, func)
	struct pcibios_softc *sc;
	u_int16_t ax;
	const char *func;
{
	const char *errstr;
	int rv = ax >> 8;
	char *nam;

	if (sc)
		nam = sc->sc_dev.dv_xname;
	else
		nam = "pcibios0";

	switch (rv) {
	case PCIBIOS_SUCCESS:
		return (PCIBIOS_SUCCESS);

	case PCIBIOS_SERVICE_NOT_PRESENT:
		errstr = "service not present";
		break;

	case PCIBIOS_FUNCTION_NOT_SUPPORTED:
		errstr = "function not supported";
		break;

	case PCIBIOS_BAD_VENDOR_ID:
		errstr = "bad vendor ID";
		break;

	case PCIBIOS_DEVICE_NOT_FOUND:
		errstr = "device not found";
		break;

	case PCIBIOS_BAD_REGISTER_NUMBER:
		errstr = "bad register number";
		break;

	case PCIBIOS_SET_FAILED:
		errstr = "set failed";
		break;

	case PCIBIOS_BUFFER_TOO_SMALL:
		errstr = "buffer too small";
		break;

	default:
		printf("%s: %s - unknown return code 0x%x\n",
		    nam, func, rv);
		return (rv);
	}

	printf("%s: %s - %s\n", nam, func, errstr);
	return (rv);
}

void
pcibios_print_exclirq(sc)
	struct pcibios_softc *sc;
{
	int i;

	if (pcibios_pir_header.exclusive_irq) {
		printf("%s: PCI Exclusive IRQs:", sc->sc_dev.dv_xname);
		for (i = 0; i < 16; i++) {
			if (pcibios_pir_header.exclusive_irq & (1 << i))
				printf(" %d", i);
		}
		printf("\n");
	}
}

#ifdef PCIINTR_DEBUG
void
pcibios_print_pir_table()
{
	int i, j;

	for (i = 0; i < pcibios_pir_table_nentries; i++) {
		printf("PIR Entry %d:\n", i);
		printf("\tBus: %d  Device: %d\n",
		    pcibios_pir_table[i].bus,
		    PIR_DEVFUNC_DEVICE(pcibios_pir_table[i].device));
		for (j = 0; j < 4; j++) {
			printf("\t\tINT%c: link 0x%02x bitmap 0x%04x\n",
			    'A' + j,
			    pcibios_pir_table[i].linkmap[j].link,
			    pcibios_pir_table[i].linkmap[j].bitmap);
		}
	}
}
#endif

void
pci_device_foreach(pc, maxbus, func)
	pci_chipset_tag_t pc;
	int maxbus;
	void (*func) __P((pci_chipset_tag_t, pcitag_t));
{
	const struct pci_quirkdata *qd;
	int bus, device, function, maxdevs, nfuncs;
	pcireg_t id, bhlcr;
	pcitag_t tag;

	for (bus = 0; bus <= maxbus; bus++) {
		maxdevs = pci_bus_maxdevs(pc, bus);
		for (device = 0; device < maxdevs; device++) {
			tag = pci_make_tag(pc, bus, device, 0);
			id = pci_conf_read(pc, tag, PCI_ID_REG);

			/* Invalid vendor ID value? */
			if (PCI_VENDOR(id) == PCI_VENDOR_INVALID)
				continue;
			/* XXX Not invalid, but we've done this ~forever. */
			if (PCI_VENDOR(id) == 0)
				continue;

			qd = pci_lookup_quirkdata(PCI_VENDOR(id),
			    PCI_PRODUCT(id));

			bhlcr = pci_conf_read(pc, tag, PCI_BHLC_REG);
			if (PCI_HDRTYPE_MULTIFN(bhlcr) ||
			    (qd != NULL &&
			     (qd->quirks & PCI_QUIRK_MULTIFUNCTION) != 0))
				nfuncs = 8;
			else
				nfuncs = 1;

			for (function = 0; function < nfuncs; function++) {
				tag = pci_make_tag(pc, bus, device, function);
				id = pci_conf_read(pc, tag, PCI_ID_REG);

				/* Invalid vendor ID value? */
				if (PCI_VENDOR(id) == PCI_VENDOR_INVALID)
					continue;
				/*
				 * XXX Not invalid, but we've done this
				 * ~forever.
				 */
				if (PCI_VENDOR(id) == 0)
					continue;
				(*func)(pc, tag);
			}
		}
	}
}
