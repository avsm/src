/* $OpenBSD: pci_1000.c,v 1.1 2002/06/30 16:24:40 miod Exp $ */
/* $NetBSD: pci_1000.c,v 1.12 2001/07/27 00:25:20 thorpej Exp $ */

/*
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is based on pci_kn20aa.c, written by Chris G. Demetriou at
 * Carnegie-Mellon University. Platform support for Mikasa and Mikasa/Pinnacle
 * (Pinkasa) by Ross Harvey with copyright assignment by permission of Avalon
 * Computer Systems, Inc.
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
 * Copyright (c) 1995, 1996 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Author: Chris G. Demetriou
 * 
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS" 
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND 
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/malloc.h>
#include <sys/device.h>

#include <uvm/uvm_extern.h>

#include <machine/autoconf.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>

#include <alpha/pci/pci_1000.h>

#include "sio.h"
#if NSIO > 0 || NPCEB > 0
#include <alpha/pci/siovar.h>
#endif

static bus_space_tag_t another_mystery_icu_iot;
static bus_space_handle_t another_mystery_icu_ioh;

int	dec_1000_intr_map(void *, pcitag_t, int, int, pci_intr_handle_t *);
const char *dec_1000_intr_string(void *, pci_intr_handle_t);
int	dec_1000_intr_line(void *, pci_intr_handle_t);
#if 0
const struct evcnt *dec_1000_intr_evcnt(void *, pci_intr_handle_t);
#endif
void	*dec_1000_intr_establish(void *, pci_intr_handle_t,
	    int, int (*func)(void *), void *, char *);
void	dec_1000_intr_disestablish(void *, void *);

#define	PCI_NIRQ	16
#define	PCI_STRAY_MAX	5

struct alpha_shared_intr *dec_1000_pci_intr;

void dec_1000_iointr(void *framep, unsigned long vec);
void dec_1000_enable_intr(int irq);
void dec_1000_disable_intr(int irq);
void pci_1000_imi(void);
static pci_chipset_tag_t pc_tag;

void
pci_1000_pickintr(core, iot, memt, pc)
	void *core;
	bus_space_tag_t iot, memt;
	pci_chipset_tag_t pc;
{
#if 0
	char *cp;
#endif
	int i;

	another_mystery_icu_iot = iot;

	pc_tag = pc;
	if (bus_space_map(iot, 0x536, 2, 0, &another_mystery_icu_ioh))
		panic("pci_1000_pickintr");
        pc->pc_intr_v = core;
        pc->pc_intr_map = dec_1000_intr_map;
        pc->pc_intr_string = dec_1000_intr_string;
	pc->pc_intr_line = dec_1000_intr_line;
#if 0
	pc->pc_intr_evcnt = dec_1000_intr_evcnt;
#endif
        pc->pc_intr_establish = dec_1000_intr_establish;
        pc->pc_intr_disestablish = dec_1000_intr_disestablish;

	pc->pc_pciide_compat_intr_establish = NULL;
	pc->pc_pciide_compat_intr_disestablish = NULL;

	dec_1000_pci_intr =
	    alpha_shared_intr_alloc(PCI_NIRQ);
	for (i = 0; i < PCI_NIRQ; i++) {
		alpha_shared_intr_set_maxstrays(dec_1000_pci_intr, i,
		    PCI_STRAY_MAX);
		
#if 0
		cp = alpha_shared_intr_string(dec_1000_pci_intr, i);
		sprintf(cp, "irq %d", i);
		evcnt_attach_dynamic(alpha_shared_intr_evcnt(
		    dec_1000_pci_intr, i), EVCNT_TYPE_INTR, NULL,
		    "dec_1000", cp);
#endif
	}

	pci_1000_imi();
#if NSIO > 0 || NPCEB > 0
	sio_intr_setup(pc, iot);
#endif
	set_iointr(dec_1000_iointr);
}

int     
dec_1000_intr_map(ccv, bustag, buspin, line, ihp)
	void *ccv;
	pcitag_t bustag;
	int buspin, line;
        pci_intr_handle_t *ihp;
{
	int	device;

	if (buspin == 0)	/* No IRQ used. */
		return 1;
	if (!(1 <= buspin && buspin <= 4))
		goto bad;

	alpha_pci_decompose_tag(pc_tag, bustag, NULL, &device, NULL);

	switch(device) {
	case 6:
		if(buspin != 1)
			break;
		*ihp = 0xc;		/* integrated ncr scsi */
		return 0;
	case 11:
	case 12:
	case 13:
		*ihp = (device - 11) * 4 + buspin - 1;
		return 0;
	}

bad:	printf("dec_1000_intr_map: can't map dev %d pin %d\n", device, buspin);
	return 1;
}

const char *
dec_1000_intr_string(ccv, ih)
	void *ccv;
	pci_intr_handle_t ih;
{
	static const char irqmsg_fmt[] = "dec_1000 irq %ld";
        static char irqstr[sizeof irqmsg_fmt];

        if (ih >= PCI_NIRQ)
                panic("dec_1000_intr_string: bogus dec_1000 IRQ 0x%lx", ih);

        snprintf(irqstr, sizeof irqstr, irqmsg_fmt, ih);
        return (irqstr);
}

int
dec_1000_intr_line(ccv, ih)
	void *ccv;
	pci_intr_handle_t ih;
{
#if NSIO > 0
	return sio_intr_line(NULL /*XXX*/, ih);
#else
	return (ih);
#endif
}

#if 0
const struct evcnt *
dec_1000_intr_evcnt(ccv, ih)
	void *ccv;
	pci_intr_handle_t ih;
{

	if (ih >= PCI_NIRQ)
		panic("dec_1000_intr_evcnt: bogus dec_1000 IRQ 0x%lx", ih);

	return (alpha_shared_intr_evcnt(dec_1000_pci_intr, ih));
}
#endif

void *
dec_1000_intr_establish(ccv, ih, level, func, arg, name)
        void *ccv;
        pci_intr_handle_t ih;
        int level;
        int (*func)(void *);
	void *arg;
	char *name;
{           
	void *cookie;

        if (ih >= PCI_NIRQ)
                panic("dec_1000_intr_establish: IRQ too high, 0x%lx", ih);

	cookie = alpha_shared_intr_establish(dec_1000_pci_intr, ih, IST_LEVEL,
	    level, func, arg, name);

	if (cookie != NULL &&
	    alpha_shared_intr_isactive(dec_1000_pci_intr, ih)) {
		dec_1000_enable_intr(ih);
	}
	return (cookie);
}

void    
dec_1000_intr_disestablish(ccv, cookie)
        void *ccv, *cookie;
{
	struct alpha_shared_intrhand *ih = cookie;
	unsigned int irq = ih->ih_num;
	int s;

	s = splhigh();

	alpha_shared_intr_disestablish(dec_1000_pci_intr, cookie,
	    "dec_1000 irq");
	if (alpha_shared_intr_isactive(dec_1000_pci_intr, irq) == 0) {
		dec_1000_disable_intr(irq);
		alpha_shared_intr_set_dfltsharetype(dec_1000_pci_intr, irq,
		    IST_NONE);
	}

	splx(s);
}

void
dec_1000_iointr(framep, vec)
	void *framep;
	unsigned long vec;
{
	int irq;

	if (vec >= 0x900) {
		if (vec >= 0x900 + (PCI_NIRQ << 4))
			panic("dec_1000_iointr: vec 0x%lx out of range", vec);
		irq = (vec - 0x900) >> 4;

		if (!alpha_shared_intr_dispatch(dec_1000_pci_intr, irq)) {
			alpha_shared_intr_stray(dec_1000_pci_intr, irq,
			    "dec_1000 irq");
			if (ALPHA_SHARED_INTR_DISABLE(dec_1000_pci_intr, irq))
				dec_1000_disable_intr(irq);
		}
		return;
	}
#if NSIO > 0 || NPCEB > 0
	if (vec >= 0x800) {
		sio_iointr(framep, vec);
		return;
	}
#endif
	panic("dec_1000_intr: weird vec 0x%lx", vec);
}

/*
 * Read and write the mystery ICU IMR registers
 */

#define	IR() bus_space_read_2(another_mystery_icu_iot,		\
				another_mystery_icu_ioh, 0)

#define	IW(v) bus_space_write_2(another_mystery_icu_iot,	\
				another_mystery_icu_ioh, 0, (v))

/*
 * Enable and disable interrupts at the ICU level
 */

void
dec_1000_enable_intr(irq)
	int irq;
{
	IW(IR() | 1 << irq);
}

void
dec_1000_disable_intr(irq)
	int irq;
{
	IW(IR() & ~(1 << irq));
}
/*
 * Initialize mystery ICU
 */
void
pci_1000_imi()
{
	IW(0);					/* XXX ?? */
}
