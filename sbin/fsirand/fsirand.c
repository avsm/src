/*	$OpenBSD: fsirand.c,v 1.18 2003/06/17 21:56:24 millert Exp $	*/

/*
 * Copyright (c) 1997 Todd C. Miller <Todd.Miller@courtesan.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef lint
static char rcsid[] = "$OpenBSD: fsirand.c,v 1.18 2003/06/17 21:56:24 millert Exp $";
#endif /* not lint */

#include <sys/types.h>
#include <sys/disklabel.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <sys/resource.h>
#include <sys/time.h>

#include <ufs/ffs/fs.h>
#include <ufs/ufs/dinode.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <util.h>

void usage(int);
int fsirand(char *);

extern char *__progname;

int printonly = 0, force = 0, ignorelabel = 0;

int
main(int argc, char *argv[])
{
	int n, ex = 0;
	struct rlimit rl;

	while ((n = getopt(argc, argv, "bfp")) != -1) {
		switch (n) {
		case 'b':
			ignorelabel++;
			break;
		case 'p':
			printonly++;
			break;
		case 'f':
			force++;
			break;
		default:
			usage(1);
		}
	}
	if (argc - optind < 1)
		usage(1);

	/* Increase our data size to the max */
	if (getrlimit(RLIMIT_DATA, &rl) == 0) {
		rl.rlim_cur = rl.rlim_max;
		if (setrlimit(RLIMIT_DATA, &rl) < 0)
			warn("Can't get resource limit to max data size");
	} else
		warn("Can't get resource limit for data size");

	for (n = optind; n < argc; n++) {
		if (argc - optind != 1)
			(void)puts(argv[n]);
		ex += fsirand(argv[n]);
		if (n < argc - 1)
			putchar('\n');
	}

	exit(ex);
}

int
fsirand(char *device)
{
	static struct dinode *inodebuf;
	static size_t oldibufsize;
	size_t ibufsize;
	struct fs *sblock, *tmpsblock;
	ino_t inumber, maxino;
	daddr_t dblk;
	char sbuf[SBSIZE], sbuftmp[SBSIZE];
	int devfd, n, cg;
	char *devpath;
	u_int32_t bsize = DEV_BSIZE;
	struct disklabel label;

	if ((devfd = opendev(device, printonly ? O_RDONLY : O_RDWR,
	    OPENDEV_PART, &devpath)) < 0) {
		warn("Can't open %s", devpath);
		return (1);
	}

	/* Get block size (usually 512) from disklabel if possible */
	if (!ignorelabel) {
		if (ioctl(devfd, DIOCGDINFO, &label) < 0)
			warn("Can't read disklabel, using sector size of %d",
			    bsize);
		else
			bsize = label.d_secsize;
	}

	/* Read in master superblock */
	(void)memset(&sbuf, 0, sizeof(sbuf));
	sblock = (struct fs *)&sbuf;
	if (lseek(devfd, (off_t)SBOFF, SEEK_SET) == -1) {
		warn("Can't seek to superblock (%qd) on %s", SBOFF, devpath);
		return (1);
	}
	if ((n = read(devfd, (void *)sblock, SBSIZE)) != SBSIZE) {
		warnx("Can't read superblock on %s: %s", devpath,
		    (n < SBSIZE) ? "short read" : strerror(errno));
		return (1);
	}
	maxino = sblock->fs_ncg * sblock->fs_ipg;

	/* Simple sanity checks on the superblock */
	if (sblock->fs_magic != FS_MAGIC) {
		warnx("Bad magic number in superblock");
		return (1);
	}
	if (sblock->fs_sbsize > SBSIZE) {
		warnx("Superblock size is preposterous");
		return (1);
	}
	if (sblock->fs_postblformat == FS_42POSTBLFMT) {
		warnx("Filesystem format is too old, sorry");
		return (1);
	}
	if (!force && !printonly && sblock->fs_clean != FS_ISCLEAN) {
		warnx("Filesystem is not clean, fsck %s first.", devpath);
		return (1);
	}

	/* Make sure backup superblocks are sane. */
	tmpsblock = (struct fs *)&sbuftmp;
	for (cg = 0; cg < sblock->fs_ncg; cg++) {
		dblk = fsbtodb(sblock, cgsblock(sblock, cg));
		if (lseek(devfd, (off_t)dblk * (off_t)bsize, SEEK_SET) < 0) {
			warn("Can't seek to %qd", (off_t)dblk * bsize);
			return (1);
		} else if ((n = read(devfd, (void *)tmpsblock, SBSIZE)) != SBSIZE) {
			warn("Can't read backup superblock %d on %s: %s",
			    cg + 1, devpath, (n < SBSIZE) ? "short read"
			    : strerror(errno));
			return (1);
		}
		if (tmpsblock->fs_magic != FS_MAGIC) {
			warnx("Bad magic number in backup superblock %d on %s",
			    cg + 1, devpath);
			return (1);
		}
		if (tmpsblock->fs_sbsize > SBSIZE) {
			warnx("Size of backup superblock %d on %s is preposterous",
			    cg + 1, devpath);
			return (1);
		}
	}

	/* XXX - should really cap buffer at 512kb or so */
	ibufsize = sizeof(struct dinode) * sblock->fs_ipg;
	if (oldibufsize < ibufsize) {
		if ((inodebuf = realloc(inodebuf, ibufsize)) == NULL)
			errx(1, "Can't allocate memory for inode buffer");
		oldibufsize = ibufsize;
	}

	if (printonly && (sblock->fs_id[0] || sblock->fs_id[1])) {
		if (sblock->fs_inodefmt >= FS_44INODEFMT && sblock->fs_id[0])
			(void)printf("%s was randomized on %s", devpath,
			    ctime((const time_t *)&(sblock->fs_id[0])));
		(void)printf("fsid: %x %x\n", sblock->fs_id[0],
		    sblock->fs_id[1]);
	}

	/* Randomize fs_id unless old 4.2BSD filesystem */
	if ((sblock->fs_inodefmt >= FS_44INODEFMT) && !printonly) {
		/* Randomize fs_id and write out new sblock and backups */
		sblock->fs_id[0] = (u_int32_t)time(NULL);
		sblock->fs_id[1] = arc4random();

		if (lseek(devfd, (off_t)SBOFF, SEEK_SET) == -1) {
			warn("Can't seek to superblock (%qd) on %s", SBOFF,
			    devpath);
			return (1);
		}
		if ((n = write(devfd, (void *)sblock, SBSIZE)) != SBSIZE) {
			warn("Can't write superblock on %s: %s", devpath,
			    (n < SBSIZE) ? "short write" : strerror(errno));
			return (1);
		}
	}

	/* For each cylinder group, randomize inodes and update backup sblock */
	for (cg = 0, inumber = 0; cg < sblock->fs_ncg; cg++) {
		/* Update superblock if appropriate */
		if ((sblock->fs_inodefmt >= FS_44INODEFMT) && !printonly) {
			dblk = fsbtodb(sblock, cgsblock(sblock, cg));
			if (lseek(devfd, (off_t)dblk * (off_t)bsize,
			    SEEK_SET) < 0) {
				warn("Can't seek to %qd", (off_t)dblk * bsize);
				return (1);
			} else if ((n = write(devfd, (void *)sblock, SBSIZE)) !=
			    SBSIZE) {
				warn("Can't read backup superblock %d on %s: %s",
				    cg + 1, devpath, (n < SBSIZE) ? "short write"
				    : strerror(errno));
				return (1);
			}
		}

		/* Read in inodes, then print or randomize generation nums */
		dblk = fsbtodb(sblock, ino_to_fsba(sblock, inumber));
		if (lseek(devfd, (off_t)dblk * (off_t)bsize, SEEK_SET) < 0) {
			warn("Can't seek to %qd", (off_t)dblk * bsize);
			return (1);
		} else if ((n = read(devfd, inodebuf, ibufsize)) != ibufsize) {
			warnx("Can't read inodes: %s",
			    (n < ibufsize) ? "short read" : strerror(errno));
			return (1);
		}

		for (n = 0; n < sblock->fs_ipg; n++, inumber++) {
			if (inumber >= ROOTINO) {
				if (printonly)
					(void)printf("ino %d gen %x\n", inumber,
					    inodebuf[n].di_gen);
				else
					inodebuf[n].di_gen = arc4random();
			}
		}

		/* Write out modified inodes */
		if (!printonly) {
			if (lseek(devfd, (off_t)dblk * (off_t)bsize, SEEK_SET) < 0) {
				warn("Can't seek to %qd",
				    (off_t)dblk * bsize);
				return (1);
			} else if ((n = write(devfd, inodebuf, ibufsize)) !=
				 ibufsize) {
				warnx("Can't write inodes: %s",
				    (n != ibufsize) ? "short write" :
				    strerror(errno));
				return (1);
			}
		}
	}
	(void)close(devfd);

	return(0);
}

void
usage(int ex)
{
	(void)fprintf(stderr, "Usage: %s [ -b ] [ -f ] [ -p ] special [special ...]\n",
	    __progname);
	exit(ex);
}
