/*	$OpenBSD: pci_machdep.c,v 1.9.10.4 2001/11/13 21:00:49 niklas Exp $	*/
/*	$NetBSD: pci_machdep.c,v 1.7 1996/11/19 04:57:32 cgd Exp $	*/

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

/*
 * Machine-specific functions for PCI autoconfiguration.
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/device.h>
#include <uvm/uvm_extern.h>
#include <machine/cpu.h>
#include <sys/sysctl.h>

#include <dev/isa/isavar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

#include "vga.h"
#if NVGA_PCI
#include <dev/pci/vga_pcivar.h>
#endif

#include "tga.h"
#if NTGA
#include <dev/pci/tgavar.h>
#endif

struct alpha_pci_chipset *alpha_pci_chipset;

void
pci_display_console(iot, memt, pc, bus, device, function)
	bus_space_tag_t iot, memt;
	pci_chipset_tag_t pc;
	int bus, device, function;
{
	pcitag_t tag;
	pcireg_t id, class;
	int match;
#if NVGA_PCI || NTGA
	int nmatch;
#endif
	int (*fn) __P((bus_space_tag_t, bus_space_tag_t, pci_chipset_tag_t,
	    int, int, int));

	tag = pci_make_tag(pc, bus, device, function);
	id = pci_conf_read(pc, tag, PCI_ID_REG);
	if (id == 0 || id == 0xffffffff)
		panic("pci_display_console: no device at %d/%d/%d",
		    bus, device, function);
	class = pci_conf_read(pc, tag, PCI_CLASS_REG);

	match = 0;
	fn = NULL;

#if NVGA_PCI
	nmatch = DEVICE_IS_VGA_PCI(class, id);
	if (nmatch > match) {
		match = nmatch;
		fn = vga_pci_cnattach;
	}
#endif
#if NTGA
	nmatch = DEVICE_IS_TGA(class, id);
	if (nmatch > match) {
		match = nmatch;
		fn = tga_cnattach;
	}
#endif

	if (fn != NULL)
		(*fn)(iot, memt, pc, bus, device, function);
	else
		panic("pci_display_console: unconfigured device at %d/%d/%d",
		    bus, device, function);
}

int
alpha_sysctl_chipset(int *name, u_int namelen, char *where, size_t *sizep)
{
	if (namelen != 1)
		return (ENOTDIR);

	if (alpha_pci_chipset == NULL)
		return (EOPNOTSUPP);

	switch (name[0]) {
	case CPU_CHIPSET_TYPE:
		return (sysctl_rdstring(where, sizep, NULL,
		    alpha_pci_chipset->pc_name));
	case CPU_CHIPSET_BWX:
		return (sysctl_rdint(where, sizep, NULL,
		    alpha_pci_chipset->pc_bwx));
	case CPU_CHIPSET_MEM:
		return (sysctl_rdquad(where, sizep, NULL,
		    alpha_pci_chipset->pc_mem));
	case CPU_CHIPSET_DENSE:
		return (sysctl_rdquad(where, sizep, NULL,
		    alpha_pci_chipset->pc_dense));
	case CPU_CHIPSET_PORTS:
		return (sysctl_rdquad(where, sizep, NULL,
		    alpha_pci_chipset->pc_ports));
	case CPU_CHIPSET_HAE_MASK:
		return (sysctl_rdquad(where, sizep, NULL,
		    alpha_pci_chipset->pc_hae_mask));
	default:
		return (EOPNOTSUPP);
	}
	/* NOTREACHED */
}
