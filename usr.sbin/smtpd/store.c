/*	$OpenBSD: store.c,v 1.21 2009/08/06 13:40:45 gilles Exp $	*/

/*
 * Copyright (c) 2008 Gilles Chehade <gilles@openbsd.org>
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

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/queue.h>
#include <sys/tree.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <event.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "smtpd.h"

int file_copy(FILE *, FILE *, struct path *, enum action_type);

int
file_copy_session(struct smtpd *env, FILE *dest, FILE *src)
{
	char *buf, *lbuf;
	size_t len;

	lbuf = NULL;
	while ((buf = fgetln(src, &len))) {
		if (buf[len - 1] == '\n') {
			buf[len - 1] = '\0';
			len--;
		}
		else {
			/* EOF without EOL, copy and add the NUL */
			if ((lbuf = malloc(len + 1)) == NULL)
				err(1, NULL);
			memcpy(lbuf, buf, len);
			lbuf[len] = '\0';
			buf = lbuf;
		}

		if (fprintf(dest, "%s\r\n", buf) != (int)len + 2)
			return 0;
	}
	free(lbuf);

	return 1;
}

int
file_copy(FILE *dest, FILE *src, struct path *path, enum action_type type)
{
	char *buf, *lbuf;
	size_t len;
	char *escape;
	int inheaders = 1;

	lbuf = NULL;
	while ((buf = fgetln(src, &len))) {
		if (buf[len - 1] == '\n') {
			buf[len - 1] = '\0';
			len--;
		}
		else {
			/* EOF without EOL, copy and add the NUL */
			if ((lbuf = malloc(len + 1)) == NULL)
				err(1, NULL);
			memcpy(lbuf, buf, len);
			lbuf[len] = '\0';
			buf = lbuf;
		}

		/* If we are NOT dealing with a mailer daemon copy, we have
		 * path set to the original recipient. In that case, we can
		 * add the X-OpenSMTPD-Loop header to help loop detection.
		 */
		if (path != NULL && inheaders &&
		    strchr(buf, ':') == NULL && !isspace(*buf)) {
			if (fprintf(dest, "X-OpenSMTPD-Loop: %s@%s\n",
				path->user, path->domain) == -1)
				return 0;
			inheaders = 0;
		}

		if (type == A_MBOX) {
			escape = buf;
			while (*escape == '>')
				++escape;
			if (strncmp("From ", escape, 5) == 0) {
				if (fprintf(dest, ">") != 1)
					return 0;
			}
		}

		if (fprintf(dest, "%s\n", buf) != (int)len + 1)
			return 0;
	}
	free(lbuf);

	if (type == A_MBOX) {
		if (fprintf(dest, "\n") != 1)
			return 0;
	}

	return 1;
}

int
store_write_message(struct batch *batchp, struct message *messagep)
{
	FILE *mboxfp;
	FILE *messagefp;

	mboxfp = fdopen(batchp->sessionp->mboxfd, "a");
	if (mboxfp == NULL)
		return 0;

	messagefp = fdopen(batchp->sessionp->messagefd, "r");
	if (messagefp == NULL)
		goto bad;

	if (! file_copy(mboxfp, messagefp, &messagep->session_rcpt,
		messagep->recipient.rule.r_action))
		goto bad;

	fflush(mboxfp);
	fsync(fileno(mboxfp));
	fclose(mboxfp);
	fclose(messagefp);
	return 1;

bad:
	if (mboxfp != NULL)
		fclose(mboxfp);

	if (messagefp != NULL)
		fclose(messagefp);

	return 0;
}

int
store_message(struct batch *batchp, struct message *messagep)
{
	struct stat sb;

	if (fstat(batchp->sessionp->mboxfd, &sb) == -1)
		return 0;

	if (! store_write_message(batchp, messagep)) {
		if (S_ISREG(sb.st_mode)) {
			ftruncate(batchp->sessionp->mboxfd, sb.st_size);
			return 0;
		}
		return 0;
	}
	return 1;
}
