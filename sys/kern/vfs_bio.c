/*	$OpenBSD: vfs_bio.c,v 1.14 1997/04/14 04:23:23 tholo Exp $	*/
/*	$NetBSD: vfs_bio.c,v 1.44 1996/06/11 11:15:36 pk Exp $	*/

/*-
 * Copyright (c) 1994 Christopher G. Demetriou
 * Copyright (c) 1982, 1986, 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)vfs_bio.c	8.6 (Berkeley) 1/11/94
 */

/*
 * Some references:
 *	Bach: The Design of the UNIX Operating System (Prentice Hall, 1986)
 *	Leffler, et al.: The Design and Implementation of the 4.3BSD
 *		UNIX Operating System (Addison Welley, 1989)
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/buf.h>
#include <sys/vnode.h>
#include <sys/mount.h>
#include <sys/trace.h>
#include <sys/malloc.h>
#include <sys/resourcevar.h>
#include <sys/conf.h>
#include <sys/kernel.h>

#include <vm/vm.h>

/* Macros to clear/set/test flags. */
#define	SET(t, f)	(t) |= (f)
#define	CLR(t, f)	(t) &= ~(f)
#define	ISSET(t, f)	((t) & (f))

/*
 * Definitions for the buffer hash lists.
 */
#define	BUFHASH(dvp, lbn)	\
	(&bufhashtbl[((long)(dvp) / sizeof(*(dvp)) + (int)(lbn)) & bufhash])
LIST_HEAD(bufhashhdr, buf) *bufhashtbl, invalhash;
u_long	bufhash;

/*
 * Insq/Remq for the buffer hash lists.
 */
#define	binshash(bp, dp)	LIST_INSERT_HEAD(dp, bp, b_hash)
#define	bremhash(bp)		LIST_REMOVE(bp, b_hash)

/*
 * Definitions for the buffer free lists.
 */
#define	BQUEUES		4		/* number of free buffer queues */

#define	BQ_LOCKED	0		/* super-blocks &c */
#define	BQ_LRU		1		/* lru, useful buffers */
#define	BQ_AGE		2		/* rubbish */
#define	BQ_EMPTY	3		/* buffer headers with no memory */

TAILQ_HEAD(bqueues, buf) bufqueues[BQUEUES];
int needbuffer;

/*
 * Insq/Remq for the buffer free lists.
 */
#define	binsheadfree(bp, dp)	TAILQ_INSERT_HEAD(dp, bp, b_freelist)
#define	binstailfree(bp, dp)	TAILQ_INSERT_TAIL(dp, bp, b_freelist)

static __inline struct buf *bio_doread __P((struct vnode *, daddr_t, int,
					    struct ucred *, int));
int count_lock_queue __P((void));

void
bremfree(bp)
	struct buf *bp;
{
	struct bqueues *dp = NULL;

	/*
	 * We only calculate the head of the freelist when removing
	 * the last element of the list as that is the only time that
	 * it is needed (e.g. to reset the tail pointer).
	 *
	 * NB: This makes an assumption about how tailq's are implemented.
	 */
	if (bp->b_freelist.tqe_next == NULL) {
		for (dp = bufqueues; dp < &bufqueues[BQUEUES]; dp++)
			if (dp->tqh_last == &bp->b_freelist.tqe_next)
				break;
		if (dp == &bufqueues[BQUEUES])
			panic("bremfree: lost tail");
	}
	TAILQ_REMOVE(dp, bp, b_freelist);
}

/*
 * Initialize buffers and hash links for buffers.
 */
void
bufinit()
{
	register struct buf *bp;
	struct bqueues *dp;
	register int i;
	int base, residual;

	TAILQ_INIT(&bdirties);
	for (dp = bufqueues; dp < &bufqueues[BQUEUES]; dp++)
		TAILQ_INIT(dp);
	bufhashtbl = hashinit(nbuf, M_CACHE, &bufhash);
	base = bufpages / nbuf;
	residual = bufpages % nbuf;
	for (i = 0; i < nbuf; i++) {
		bp = &buf[i];
		bzero((char *)bp, sizeof *bp);
		bp->b_dev = NODEV;
		bp->b_rcred = NOCRED;
		bp->b_wcred = NOCRED;
		bp->b_vnbufs.le_next = NOLIST;
		bp->b_data = buffers + i * MAXBSIZE;
		if (i < residual)
			bp->b_bufsize = (base + 1) * CLBYTES;
		else
			bp->b_bufsize = base * CLBYTES;
		bp->b_flags = B_INVAL;
		dp = bp->b_bufsize ? &bufqueues[BQ_AGE] : &bufqueues[BQ_EMPTY];
		binsheadfree(bp, dp);
		binshash(bp, &invalhash);
	}
}

static __inline struct buf *
bio_doread(vp, blkno, size, cred, async)
	struct vnode *vp;
	daddr_t blkno;
	int size;
	struct ucred *cred;
	int async;
{
	register struct buf *bp;

	bp = getblk(vp, blkno, size, 0, 0);

	/*
	 * If buffer does not have data valid, start a read.
	 * Note that if buffer is B_INVAL, getblk() won't return it.
	 * Therefore, it's valid if it's I/O has completed or been delayed.
	 */
	if (!ISSET(bp->b_flags, (B_DONE | B_DELWRI))) {
		/* Start I/O for the buffer (keeping credentials). */
		SET(bp->b_flags, B_READ | async);
		if (cred != NOCRED && bp->b_rcred == NOCRED) {
			crhold(cred);
			bp->b_rcred = cred;
		}
		VOP_STRATEGY(bp);

		/* Pay for the read. */
		curproc->p_stats->p_ru.ru_inblock++;		/* XXX */
	} else if (async) {
		brelse(bp);
	}

	return (bp);
}

/*
 * Read a disk block.
 * This algorithm described in Bach (p.54).
 */
int
bread(vp, blkno, size, cred, bpp)
	struct vnode *vp;
	daddr_t blkno;
	int size;
	struct ucred *cred;
	struct buf **bpp;
{
	register struct buf *bp;

	/* Get buffer for block. */
	bp = *bpp = bio_doread(vp, blkno, size, cred, 0);

	/* Wait for the read to complete, and return result. */
	return (biowait(bp));
}

/*
 * Read-ahead multiple disk blocks. The first is sync, the rest async.
 * Trivial modification to the breada algorithm presented in Bach (p.55).
 */
int
breadn(vp, blkno, size, rablks, rasizes, nrablks, cred, bpp)
	struct vnode *vp;
	daddr_t blkno; int size;
	daddr_t rablks[]; int rasizes[];
	int nrablks;
	struct ucred *cred;
	struct buf **bpp;
{
	register struct buf *bp;
	int i;

	bp = *bpp = bio_doread(vp, blkno, size, cred, 0);

	/*
	 * For each of the read-ahead blocks, start a read, if necessary.
	 */
	for (i = 0; i < nrablks; i++) {
		/* If it's in the cache, just go on to next one. */
		if (incore(vp, rablks[i]))
			continue;

		/* Get a buffer for the read-ahead block */
		(void) bio_doread(vp, rablks[i], rasizes[i], cred, B_ASYNC);
	}

	/* Otherwise, we had to start a read for it; wait until it's valid. */
	return (biowait(bp));
}

/*
 * Read with single-block read-ahead.  Defined in Bach (p.55), but
 * implemented as a call to breadn().
 * XXX for compatibility with old file systems.
 */
int
breada(vp, blkno, size, rablkno, rabsize, cred, bpp)
	struct vnode *vp;
	daddr_t blkno; int size;
	daddr_t rablkno; int rabsize;
	struct ucred *cred;
	struct buf **bpp;
{

	return (breadn(vp, blkno, size, &rablkno, &rabsize, 1, cred, bpp));	
}

/*
 * Block write.  Described in Bach (p.56)
 */
int
bwrite(bp)
	struct buf *bp;
{
	int rv, sync, wasdelayed, s;

	/*
	 * Remember buffer type, to switch on it later.  If the write was
	 * synchronous, but the file system was mounted with MNT_ASYNC,
	 * convert it to a delayed write.  
	 * XXX note that this relies on delayed tape writes being converted
	 * to async, not sync writes (which is safe, but ugly).
	 */
	sync = !ISSET(bp->b_flags, B_ASYNC);
	if (sync && bp->b_vp && bp->b_vp->v_mount &&
	    ISSET(bp->b_vp->v_mount->mnt_flag, MNT_ASYNC)) {
		bdwrite(bp);
		return (0);
	}
	wasdelayed = ISSET(bp->b_flags, B_DELWRI);
	CLR(bp->b_flags, (B_READ | B_DONE | B_ERROR | B_DELWRI));
	/*
	 * If this was a delayed write, remove it from the
	 * list of dirty blocks now
	 */
	if (wasdelayed)
		TAILQ_REMOVE(&bdirties, bp, b_synclist);

	s = splbio();
	if (!sync) {
		/*
		 * If not synchronous, pay for the I/O operation and make
		 * sure the buf is on the correct vnode queue.  We have
		 * to do this now, because if we don't, the vnode may not
		 * be properly notified that its I/O has completed.
		 */
		if (wasdelayed)
			reassignbuf(bp, bp->b_vp);
		else
			curproc->p_stats->p_ru.ru_oublock++;
	}

	/* Initiate disk write.  Make sure the appropriate party is charged. */
	bp->b_vp->v_numoutput++;
	splx(s);
	SET(bp->b_flags, B_WRITEINPROG);
	VOP_STRATEGY(bp);

	if (sync) {
		/*
		 * If I/O was synchronous, wait for it to complete.
		 */
		rv = biowait(bp);

		/*
		 * Pay for the I/O operation, if it's not been paid for, and
		 * make sure it's on the correct vnode queue. (async operatings
		 * were payed for above.)
		 */
		s = splbio();
		if (wasdelayed)
			reassignbuf(bp, bp->b_vp);
		else
			curproc->p_stats->p_ru.ru_oublock++;
		splx(s);

		/* Release the buffer. */
		brelse(bp);

		return (rv);
	} else {
		return (0);
	}
}

int
vn_bwrite(v)
	void *v;
{
	struct vop_bwrite_args *ap = v;

	return (bwrite(ap->a_bp));
}

/*
 * Delayed write.
 *
 * The buffer is marked dirty, but is not queued for I/O.
 * This routine should be used when the buffer is expected
 * to be modified again soon, typically a small write that
 * partially fills a buffer.
 *
 * NB: magnetic tapes cannot be delayed; they must be
 * written in the order that the writes are requested.
 *
 * Described in Leffler, et al. (pp. 208-213).
 */
void
bdwrite(bp)
	struct buf *bp;
{
	/*
	 * If the block hasn't been seen before:
	 *	(1) Mark it as having been seen,
	 *	(2) Charge for the write.
	 *	(3) Make sure it's on its vnode's correct block list,
	 *	(4) If a buffer is rewritten, move it to end of dirty list
	 */
	bp->b_synctime = time.tv_sec + 30;
	if (!ISSET(bp->b_flags, B_DELWRI)) {
		/*
		 * Add the buffer to the list of dirty blocks.
		 * If it is the first entry on the list, schedule
		 * a timeout to flush it to disk
		 */
		TAILQ_INSERT_TAIL(&bdirties, bp, b_synclist);
		if (bdirties.tqh_first == bp) {
			untimeout((void (*)__P((void *)))wakeup,
				  &bdirties);		/* XXX */
			timeout((void (*)__P((void *)))wakeup,
				&bdirties, 30 * hz);
		}
		SET(bp->b_flags, B_DELWRI);
		curproc->p_stats->p_ru.ru_oublock++;	/* XXX */
		reassignbuf(bp, bp->b_vp);
	}

	/* If this is a tape block, write the block now. */
	if (bdevsw[major(bp->b_dev)].d_type == D_TAPE) {
		bawrite(bp);
		return;
	}

	/* Otherwise, the "write" is done, so mark and release the buffer. */
	CLR(bp->b_flags, B_NEEDCOMMIT);
	SET(bp->b_flags, B_DONE);
	brelse(bp);
}

/*
 * Asynchronous block write; just an asynchronous bwrite().
 */
void
bawrite(bp)
	struct buf *bp;
{

	SET(bp->b_flags, B_ASYNC);
	VOP_BWRITE(bp);
}

/*
 * Write out dirty buffers if they have been on the dirty
 * list for more than 30 seconds; scan for such buffers
 * once a second.
 */
void
vn_update()
{
	struct mount *mp, *nmp;
	struct timespec ts;
	struct vnode *vp;
	struct buf *bp;
	int async, s;

	/*
	 * In case any buffers got scheduled for write before the
	 * process got started (should never happen)
	 */
	untimeout((void (*)__P((void *)))wakeup,
		  &bdirties);
	for (;;) {
		s = splbio();
		/*
		 * Schedule a wakeup when the next buffer is to
		 * be flushed to disk.  If no buffers are enqueued,
		 * a wakeup will be scheduled at the time a new
		 * buffer is enqueued
		 */
		if ((bp = bdirties.tqh_first) != NULL) {
                        untimeout((void (*)__P((void *)))wakeup,
				  &bdirties);		/* XXX */
                        timeout((void (*)__P((void *)))wakeup,
				&bdirties, (bp->b_synctime - time.tv_sec) * hz);
		}
		tsleep(&bdirties, PZERO - 1, "dirty", 0);
		/*
		 * Walk the dirty block list, starting an asyncroneous
		 * write of any block that has timed out
		 */
		while ((bp = bdirties.tqh_first) != NULL &&
		       bp->b_synctime <= time.tv_sec) {
			/*
			 * If the block is currently busy (perhaps being
			 * written), move it to the end of the dirty list
			 * and go to the next block
			 */
			if (ISSET(bp->b_flags, B_BUSY)) {
				TAILQ_REMOVE(&bdirties, bp, b_synclist);
				TAILQ_INSERT_TAIL(&bdirties, bp, b_synclist);
				bp->b_synctime = time.tv_sec + 30;
				continue;
			}
			/*
			 * Remove the block from the per-vnode dirty
			 * list and mark it as busy
			 */
			bremfree(bp);
			SET(bp->b_flags, B_BUSY);
			splx(s);
			/*
			 * Start an asyncroneous write of the buffer.
			 * Note that this will also remove the buffer
			 * from the dirty list
			 */
			bawrite(bp);
			s = splbio();
		}
		splx(s);
		/*
		 * We also need to flush out modified vnodes
		 */
		for (mp = mountlist.cqh_last;
		     mp != (void *)&mountlist;
		     mp = nmp) {
			/*
			 * Get the next pointer in case we hang of vfs_busy()
			 * while being unmounted
			 */
			nmp = mp->mnt_list.cqe_prev;
			/*
			 * The lock check below is to avoid races with mount
			 * and unmount
			 */
			if ((mp->mnt_flag & (MNT_MLOCK | MNT_RDONLY | MNT_MPBUSY)) == 0 &&
			    !vfs_busy(mp)) {
				/*
				 * Turn off the file system async flag until
				 * we are done writing out vnodes
				 */
				async = mp->mnt_flag & MNT_ASYNC;
				mp->mnt_flag &= ~MNT_ASYNC;
				/*
				 * Walk the vnode list for the file system,
				 * writing each modified vnode out
				 */
loop:
				for (vp = mp->mnt_vnodelist.lh_first;
				     vp != NULL;
				     vp = vp->v_mntvnodes.le_next) {
					/*
					 * If the vnode is no longer associated
					 * with the file system in question, skip
					 * it
					 */
					if (vp->v_mount != mp)
						goto loop;
					/*
					 * If the vnode is currently locked,
					 * ignore it
					 */
					if (VOP_ISLOCKED(vp))
						continue;
					/*
					 * Lock the vnode, start a write and
					 * release the vnode
					 */
					if (vget(vp, 1))
						goto loop;
					TIMEVAL_TO_TIMESPEC(&time, &ts);
					VOP_UPDATE(vp, &ts, &ts, 0);
					vput(vp);
				}
				/*
				 * Restore the file system async flag if it
				 * were previously set for this file system
				 */
				mp->mnt_flag |= async;
				/*
				 * Get the next pointer again as the next
				 * file system might have been unmounted
				 * while we were flushing vnodes
				 */
				nmp = mp->mnt_list.cqe_prev;
				vfs_unbusy(mp);
			}
		}
	}
}

/*
 * Release a buffer on to the free lists.
 * Described in Bach (p. 46).
 */
void
brelse(bp)
	struct buf *bp;
{
	struct bqueues *bufq;
	int s;

	/* Wake up any processes waiting for any buffer to become free. */
	if (needbuffer) {
		needbuffer = 0;
		wakeup(&needbuffer);
	}

	/* Wake up any proceeses waiting for _this_ buffer to become free. */
	if (ISSET(bp->b_flags, B_WANTED)) {
		CLR(bp->b_flags, B_WANTED);
		wakeup(bp);
	}

	/* Block disk interrupts. */
	s = splbio();

	/*
	 * Determine which queue the buffer should be on, then put it there.
	 */

	/* If it's locked, don't report an error; try again later. */
	if (ISSET(bp->b_flags, (B_LOCKED|B_ERROR)) == (B_LOCKED|B_ERROR))
		CLR(bp->b_flags, B_ERROR);

	/* If it's not cacheable, or an error, mark it invalid. */
	if (ISSET(bp->b_flags, (B_NOCACHE|B_ERROR)))
		SET(bp->b_flags, B_INVAL);

	if (ISSET(bp->b_flags, B_VFLUSH)) {
		/*
		 * This is a delayed write buffer that was just flushed to
		 * disk.  It is still on the LRU queue.  If it's become
		 * invalid, then we need to move it to a different queue;
		 * otherwise leave it in its current position.
		 */
		CLR(bp->b_flags, B_VFLUSH);
		if (!ISSET(bp->b_flags, B_ERROR|B_INVAL|B_LOCKED|B_AGE))
			goto already_queued;
		else
			bremfree(bp);
	}

	if ((bp->b_bufsize <= 0) || ISSET(bp->b_flags, B_INVAL)) {
		/*
		 * If it's invalid or empty, dissociate it from its vnode
		 * and put on the head of the appropriate queue.
		 */
		if (bp->b_vp)
			brelvp(bp);
		if (ISSET(bp->b_flags, B_DELWRI))
			TAILQ_REMOVE(&bdirties, bp, b_synclist);
		CLR(bp->b_flags, B_DELWRI);
		if (bp->b_bufsize <= 0)
			/* no data */
			bufq = &bufqueues[BQ_EMPTY];
		else
			/* invalid data */
			bufq = &bufqueues[BQ_AGE];
		binsheadfree(bp, bufq);
	} else {
		/*
		 * It has valid data.  Put it on the end of the appropriate
		 * queue, so that it'll stick around for as long as possible.
		 */
		if (ISSET(bp->b_flags, B_LOCKED))
			/* locked in core */
			bufq = &bufqueues[BQ_LOCKED];
		else if (ISSET(bp->b_flags, B_AGE))
			/* stale but valid data */
			bufq = &bufqueues[BQ_AGE];
		else
			/* valid data */
			bufq = &bufqueues[BQ_LRU];
		binstailfree(bp, bufq);
	}

already_queued:
	/* Unlock the buffer. */
	CLR(bp->b_flags, (B_AGE | B_ASYNC | B_BUSY | B_NOCACHE));

	/* Allow disk interrupts. */
	splx(s);
}

/*
 * Determine if a block is in the cache.
 * Just look on what would be its hash chain.  If it's there, return
 * a pointer to it, unless it's marked invalid.  If it's marked invalid,
 * we normally don't return the buffer, unless the caller explicitly
 * wants us to.
 */
struct buf *
incore(vp, blkno)
	struct vnode *vp;
	daddr_t blkno;
{
	struct buf *bp;

	bp = BUFHASH(vp, blkno)->lh_first;

	/* Search hash chain */
	for (; bp != NULL; bp = bp->b_hash.le_next) {
		if (bp->b_lblkno == blkno && bp->b_vp == vp &&
		    !ISSET(bp->b_flags, B_INVAL))
		return (bp);
	}

	return (0);
}

/*
 * Get a block of requested size that is associated with
 * a given vnode and block offset. If it is found in the
 * block cache, mark it as having been found, make it busy
 * and return it. Otherwise, return an empty block of the
 * correct size. It is up to the caller to insure that the
 * cached blocks be of the correct size.
 */
struct buf *
getblk(vp, blkno, size, slpflag, slptimeo)
	register struct vnode *vp;
	daddr_t blkno;
	int size, slpflag, slptimeo;
{
	struct bufhashhdr *bh;
	struct buf *bp;
	int s, err;

	/*
	 * XXX
	 * The following is an inlined version of 'incore()', but with
	 * the 'invalid' test moved to after the 'busy' test.  It's
	 * necessary because there are some cases in which the NFS
	 * code sets B_INVAL prior to writing data to the server, but 
	 * in which the buffers actually contain valid data.  In this
	 * case, we can't allow the system to allocate a new buffer for
	 * the block until the write is finished.
	 */
	bh = BUFHASH(vp, blkno);
start:
        bp = bh->lh_first;
        for (; bp != NULL; bp = bp->b_hash.le_next) {
                if (bp->b_lblkno != blkno || bp->b_vp != vp)
			continue;

		s = splbio();
		if (ISSET(bp->b_flags, B_BUSY)) {
			SET(bp->b_flags, B_WANTED);
			err = tsleep(bp, slpflag | (PRIBIO + 1), "getblk",
			    slptimeo);
			splx(s);
			if (err)
				return (NULL);
			goto start;
		}

		if (!ISSET(bp->b_flags, B_INVAL)) {
			SET(bp->b_flags, (B_BUSY | B_CACHE));
			bremfree(bp);
			splx(s);
			break;
		}
		splx(s);
        }

	if (bp == NULL) {
		if ((bp = getnewbuf(slpflag, slptimeo)) == NULL)
			goto start;
		binshash(bp, bh);
		bp->b_blkno = bp->b_lblkno = blkno;
		s = splbio();
		bgetvp(vp, bp);
		splx(s);
	}
	allocbuf(bp, size);
	return (bp);
}

/*
 * Get an empty, disassociated buffer of given size.
 */
struct buf *
geteblk(size)
	int size;
{
	struct buf *bp; 

	while ((bp = getnewbuf(0, 0)) == 0)
		;
	SET(bp->b_flags, B_INVAL);
	binshash(bp, &invalhash);
	allocbuf(bp, size);

	return (bp);
}

/*
 * Expand or contract the actual memory allocated to a buffer.
 *
 * If the buffer shrinks, data is lost, so it's up to the
 * caller to have written it out *first*; this routine will not
 * start a write.  If the buffer grows, it's the callers
 * responsibility to fill out the buffer's additional contents.
 */
void
allocbuf(bp, size)
	struct buf *bp;
	int size;
{
	struct buf      *nbp;
	vm_size_t       desired_size;
	int	     s;

	desired_size = roundup(size, CLBYTES);
	if (desired_size > MAXBSIZE)
		panic("allocbuf: buffer larger than MAXBSIZE requested");

	if (bp->b_bufsize == desired_size)
		goto out;

	/*
	 * If the buffer is smaller than the desired size, we need to snarf
	 * it from other buffers.  Get buffers (via getnewbuf()), and
	 * steal their pages.
	 */
	while (bp->b_bufsize < desired_size) {
		int amt;

		/* find a buffer */
		while ((nbp = getnewbuf(0, 0)) == NULL)
			;
		SET(nbp->b_flags, B_INVAL);
		binshash(nbp, &invalhash);

		/* and steal its pages, up to the amount we need */
		amt = min(nbp->b_bufsize, (desired_size - bp->b_bufsize));
		pagemove((nbp->b_data + nbp->b_bufsize - amt),
			 bp->b_data + bp->b_bufsize, amt);
		bp->b_bufsize += amt;
		nbp->b_bufsize -= amt;

		/* reduce transfer count if we stole some data */
		if (nbp->b_bcount > nbp->b_bufsize)
			nbp->b_bcount = nbp->b_bufsize;

#ifdef DIAGNOSTIC
		if (nbp->b_bufsize < 0)
			panic("allocbuf: negative bufsize");
#endif

		brelse(nbp);
	}

	/*
	 * If we want a buffer smaller than the current size,
	 * shrink this buffer.  Grab a buf head from the EMPTY queue,
	 * move a page onto it, and put it on front of the AGE queue.
	 * If there are no free buffer headers, leave the buffer alone.
	 */
	if (bp->b_bufsize > desired_size) {
		s = splbio();
		if ((nbp = bufqueues[BQ_EMPTY].tqh_first) == NULL) {
			/* No free buffer head */
			splx(s);
			goto out;
		}
		bremfree(nbp);
		SET(nbp->b_flags, B_BUSY);
		splx(s);

		/* move the page to it and note this change */
		pagemove(bp->b_data + desired_size,
		    nbp->b_data, bp->b_bufsize - desired_size);
		nbp->b_bufsize = bp->b_bufsize - desired_size;
		bp->b_bufsize = desired_size;
		nbp->b_bcount = 0;
		SET(nbp->b_flags, B_INVAL);

		/* release the newly-filled buffer and leave */
		brelse(nbp);
	}

out:
	bp->b_bcount = size;
}

/*
 * Find a buffer which is available for use.
 * Select something from a free list.
 * Preference is to AGE list, then LRU list.    
 */
struct buf *
getnewbuf(slpflag, slptimeo)
	int slpflag, slptimeo;
{
	register struct buf *bp;
	int s;

start:
	s = splbio();
	if ((bp = bufqueues[BQ_AGE].tqh_first) != NULL ||
	    (bp = bufqueues[BQ_LRU].tqh_first) != NULL) {
		bremfree(bp);
	} else {
		/* wait for a free buffer of any kind */
		needbuffer = 1;
		tsleep(&needbuffer, slpflag|(PRIBIO+1), "getnewbuf", slptimeo);
		splx(s);
		return (0);
	}

	if (ISSET(bp->b_flags, B_VFLUSH)) {
		/*
		 * This is a delayed write buffer being flushed to disk.  Make
		 * sure it gets aged out of the queue when it's finished, and
		 * leave it off the LRU queue.
		 */
		CLR(bp->b_flags, B_VFLUSH);
		SET(bp->b_flags, B_AGE);
		splx(s);
		goto start;
	}

	/* Buffer is no longer on free lists. */
	SET(bp->b_flags, B_BUSY);

	/* If buffer was a delayed write, start it, and go back to the top. */
	if (ISSET(bp->b_flags, B_DELWRI)) {
		splx(s);
		/*
		 * This buffer has gone through the LRU, so make sure it gets
		 * reused ASAP.
		 */
		SET(bp->b_flags, B_AGE);
		bawrite(bp);
		goto start;
	}

	/* disassociate us from our vnode, if we had one... */
	if (bp->b_vp)
		brelvp(bp);
	splx(s);

	/* clear out various other fields */
	bp->b_flags = B_BUSY;
	bp->b_dev = NODEV;
	bp->b_blkno = bp->b_lblkno = 0;
	bp->b_iodone = 0;
	bp->b_error = 0;
	bp->b_resid = 0;
	bp->b_bcount = 0;
	bp->b_dirtyoff = bp->b_dirtyend = 0;
	bp->b_validoff = bp->b_validend = 0;

	/* nuke any credentials we were holding */
	if (bp->b_rcred != NOCRED) {
		crfree(bp->b_rcred);
		bp->b_rcred = NOCRED; 
	}
	if (bp->b_wcred != NOCRED) {
		crfree(bp->b_wcred);
		bp->b_wcred = NOCRED;
	}
	
	bremhash(bp);
	return (bp); 
}

/*
 * Wait for operations on the buffer to complete.
 * When they do, extract and return the I/O's error value.
 */
int
biowait(bp)
	struct buf *bp;
{
	int s;

	s = splbio();
	while (!ISSET(bp->b_flags, B_DONE))
		tsleep(bp, PRIBIO + 1, "biowait", 0);
	splx(s);

	/* check for interruption of I/O (e.g. via NFS), then errors. */
	if (ISSET(bp->b_flags, B_EINTR)) {
		CLR(bp->b_flags, B_EINTR);
		return (EINTR);
	} else if (ISSET(bp->b_flags, B_ERROR))
		return (bp->b_error ? bp->b_error : EIO);
	else
		return (0);
}

/*
 * Mark I/O complete on a buffer.
 *
 * If a callback has been requested, e.g. the pageout
 * daemon, do so. Otherwise, awaken waiting processes.
 *
 * [ Leffler, et al., says on p.247:
 *	"This routine wakes up the blocked process, frees the buffer
 *	for an asynchronous write, or, for a request by the pagedaemon
 *	process, invokes a procedure specified in the buffer structure" ]
 *
 * In real life, the pagedaemon (or other system processes) wants
 * to do async stuff to, and doesn't want the buffer brelse()'d.
 * (for swap pager, that puts swap buffers on the free lists (!!!),
 * for the vn device, that puts malloc'd buffers on the free lists!)
 */
void
biodone(bp)
	struct buf *bp;
{
	if (ISSET(bp->b_flags, B_DONE))
		panic("biodone already");
	SET(bp->b_flags, B_DONE);		/* note that it's done */

	if (!ISSET(bp->b_flags, B_READ))	/* wake up reader */
		vwakeup(bp);

	if (ISSET(bp->b_flags, B_CALL)) {	/* if necessary, call out */
		CLR(bp->b_flags, B_CALL);	/* but note callout done */
		(*bp->b_iodone)(bp);
	} else if (ISSET(bp->b_flags, B_ASYNC))	/* if async, release it */
		brelse(bp);
	else {					/* or just wakeup the buffer */
		CLR(bp->b_flags, B_WANTED);
		wakeup(bp);
	}
}

/*
 * Return a count of buffers on the "locked" queue.
 */
int
count_lock_queue()
{
	register struct buf *bp;
	register int n = 0;

	for (bp = bufqueues[BQ_LOCKED].tqh_first; bp;
	    bp = bp->b_freelist.tqe_next)
		n++;
	return (n);
}

#ifdef DEBUG
/*
 * Print out statistics on the current allocation of the buffer pool.
 * Can be enabled to print out on every ``sync'' by setting "syncprt"
 * in vfs_syscalls.c using sysctl.
 */
void
vfs_bufstats()
{
	int s, i, j, count;
	register struct buf *bp;
	register struct bqueues *dp;
	int counts[MAXBSIZE/CLBYTES+1];
	static char *bname[BQUEUES] = { "LOCKED", "LRU", "AGE", "EMPTY" };

	for (dp = bufqueues, i = 0; dp < &bufqueues[BQUEUES]; dp++, i++) {
		count = 0;
		for (j = 0; j <= MAXBSIZE/CLBYTES; j++)
			counts[j] = 0;
		s = splbio();
		for (bp = dp->tqh_first; bp; bp = bp->b_freelist.tqe_next) {
			counts[bp->b_bufsize/CLBYTES]++;
			count++;
		}
		splx(s);
		printf("%s: total-%d", bname[i], count);
		for (j = 0; j <= MAXBSIZE/CLBYTES; j++)
			if (counts[j] != 0)
				printf(", %d-%d", j * CLBYTES, counts[j]);
		printf("\n");
	}
}
#endif /* DEBUG */
