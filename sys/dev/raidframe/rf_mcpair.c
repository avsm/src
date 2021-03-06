/*	$OpenBSD: rf_mcpair.c,v 1.3 2002/12/16 07:01:04 tdeval Exp $	*/
/*	$NetBSD: rf_mcpair.c,v 1.3 1999/02/05 00:06:13 oster Exp $	*/

/*
 * Copyright (c) 1995 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Author: Jim Zelenka
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
 * rf_mcpair.c
 * An mcpair is a structure containing a mutex and a condition variable.
 * It's used to block the current thread until some event occurs.
 */

#include "rf_types.h"
#include "rf_threadstuff.h"
#include "rf_mcpair.h"
#include "rf_debugMem.h"
#include "rf_freelist.h"
#include "rf_shutdown.h"

#include <sys/proc.h>

static RF_FreeList_t *rf_mcpair_freelist;

#define	RF_MAX_FREE_MCPAIR	128
#define	RF_MCPAIR_INC		 16
#define	RF_MCPAIR_INITIAL	 24

int  rf_init_mcpair(RF_MCPair_t *);
void rf_clean_mcpair(RF_MCPair_t *);
void rf_ShutdownMCPair(void *);


int
rf_init_mcpair(RF_MCPair_t *t)
{
	int rc;

	rc = rf_mutex_init(&t->mutex);
	if (rc) {
		RF_ERRORMSG3("Unable to init mutex file %s line %d rc=%d\n",
		    __FILE__, __LINE__, rc);
		return (rc);
	}
	rc = rf_cond_init(&t->cond);
	if (rc) {
		RF_ERRORMSG3("Unable to init cond file %s line %d rc=%d\n",
		    __FILE__, __LINE__, rc);
		rf_mutex_destroy(&t->mutex);
		return (rc);
	}
	return (0);
}

void
rf_clean_mcpair(RF_MCPair_t *t)
{
	rf_mutex_destroy(&t->mutex);
	rf_cond_destroy(&t->cond);
}

void
rf_ShutdownMCPair(void *ignored)
{
	RF_FREELIST_DESTROY_CLEAN(rf_mcpair_freelist, next, (RF_MCPair_t *),
	    rf_clean_mcpair);
}

int
rf_ConfigureMCPair(RF_ShutdownList_t **listp)
{
	int rc;

	RF_FREELIST_CREATE(rf_mcpair_freelist, RF_MAX_FREE_MCPAIR,
	    RF_MCPAIR_INC, sizeof(RF_MCPair_t));
	rc = rf_ShutdownCreate(listp, rf_ShutdownMCPair, NULL);
	if (rc) {
		RF_ERRORMSG3("Unable to add to shutdown list file %s line %d"
		    " rc=%d\n", __FILE__, __LINE__, rc);
		rf_ShutdownMCPair(NULL);
		return (rc);
	}
	RF_FREELIST_PRIME_INIT(rf_mcpair_freelist, RF_MCPAIR_INITIAL, next,
	    (RF_MCPair_t *), rf_init_mcpair);
	return (0);
}

RF_MCPair_t *
rf_AllocMCPair(void)
{
	RF_MCPair_t *t;

	RF_FREELIST_GET_INIT(rf_mcpair_freelist, t, next, (RF_MCPair_t *),
	    rf_init_mcpair);
	if (t) {
		t->flag = 0;
		t->next = NULL;
	}
	return (t);
}

void
rf_FreeMCPair(RF_MCPair_t *t)
{
	RF_FREELIST_FREE_CLEAN(rf_mcpair_freelist, t, next, rf_clean_mcpair);
}

/*
 * The callback function used to wake you up when you use an mcpair to wait
 * for something.
 */
void
rf_MCPairWakeupFunc(RF_MCPair_t *mcpair)
{
	RF_LOCK_MUTEX(mcpair->mutex);
	mcpair->flag = 1;
#if 0
	printf("MCPairWakeupFunc called!\n");
#endif
	wakeup(&(mcpair->flag)); /* XXX Does this do anything useful !!!  GO */
	/*
	 * XXX
	 * Looks like the following is needed to truly get the
	 * functionality they were looking for here...  This could be a
	 * side-effect of my using a tsleep in the Net- and OpenBSD port
	 * though...
	 * XXX
	 */
	wakeup(&(mcpair->cond));	/* XXX XXX XXX GO */
	RF_UNLOCK_MUTEX(mcpair->mutex);
}
