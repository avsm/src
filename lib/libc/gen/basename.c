/*	$OpenBSD: basename.c,v 1.9 2003/06/03 01:52:39 millert Exp $	*/

/*
 * Copyright (c) 1997 Todd C. Miller <Todd.Miller@courtesan.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND TODD C. MILLER DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL TODD C. MILLER BE LIABLE
 * FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef lint
static char rcsid[] = "$OpenBSD: basename.c,v 1.9 2003/06/03 01:52:39 millert Exp $";
#endif /* not lint */

#include <errno.h>
#include <libgen.h>
#include <string.h>
#include <sys/param.h>

char *
basename(path)
	const char *path;
{
	static char bname[MAXPATHLEN];
	register const char *endp, *startp;

	/* Empty or NULL string gets treated as "." */
	if (path == NULL || *path == '\0') {
		(void)strlcpy(bname, ".", sizeof bname);
		return(bname);
	}

	/* Strip trailing slashes */
	endp = path + strlen(path) - 1;
	while (endp > path && *endp == '/')
		endp--;

	/* All slashes become "/" */
	if (endp == path && *endp == '/') {
		(void)strlcpy(bname, "/", sizeof bname);
		return(bname);
	}

	/* Find the start of the base */
	startp = endp;
	while (startp > path && *(startp - 1) != '/')
		startp--;

	if (endp - startp + 2 > sizeof(bname)) {
		errno = ENAMETOOLONG;
		return(NULL);
	}
	strlcpy(bname, startp, endp - startp + 2);
	return(bname);
}
