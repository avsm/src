/*	$OpenBSD: auth.c,v 1.17 2009/06/06 07:31:26 eric Exp $ */

/*
 * Copyright (c) 2004, 2005 Esben Norby <norby@openbsd.org>
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
#include <sys/socket.h>
#include <limits.h>
#include <md5.h>
#include <stdlib.h>
#include <string.h>

#include "ospfd.h"
#include "ospf.h"
#include "log.h"
#include "ospfe.h"

struct auth_md *md_list_find(struct auth_md_head *, u_int8_t);

int
auth_validate(void *buf, u_int16_t len, struct iface *iface, struct nbr *nbr)
{
	MD5_CTX		 hash;
	u_int8_t	 digest[MD5_DIGEST_LENGTH];
	u_int8_t	 recv_digest[MD5_DIGEST_LENGTH];
	struct ospf_hdr	*ospf_hdr = buf;
	struct auth_md	*md;
	char		*auth_data;

	if (ntohs(ospf_hdr->auth_type) != (u_int16_t)iface->auth_type) {
		log_debug("auth_validate: wrong auth type, interface %s",
		    iface->name);
		return (-1);
	}

	switch (iface->auth_type) {
	case AUTH_SIMPLE:
		if (memcmp(ospf_hdr->auth_key.simple, iface->auth_key,
		    sizeof(ospf_hdr->auth_key.simple))) {
			log_debug("auth_validate: wrong password, interface %s",
			    iface->name);
			return (-1);
		}
		/* FALLTHROUGH */
	case AUTH_NONE:
		/* clear the key before chksum */
		bzero(ospf_hdr->auth_key.simple,
		     sizeof(ospf_hdr->auth_key.simple));

		if (in_cksum(ospf_hdr, ntohs(ospf_hdr->len))) {
			log_debug("auth_validate: invalid checksum, "
			    "interface %s", iface->name);
			return (-1);
		}
		break;
	case AUTH_CRYPT:
		/*
		 * We must allow keys that are configured on the interface
		 * but not necessarily set as the transmit key
		 * (iface->auth_keyid). This allows for key rotation to new
		 * keys without taking down the network.
		 */
		if ((md = md_list_find(&iface->auth_md_list,
		    ospf_hdr->auth_key.crypt.keyid)) == NULL) {
			log_debug("auth_validate: keyid %d not configured, "
			    "interface %s", ospf_hdr->auth_key.crypt.keyid,
			    iface->name);
			return (-1);
		}

		if (nbr != NULL && ntohl(ospf_hdr->auth_key.crypt.seq_num) <
		    nbr->crypt_seq_num) {
			log_debug("auth_validate: decreasing seq num, "
			    "interface %s", iface->name);
			return (-1);
		}

		if (ospf_hdr->auth_key.crypt.len != MD5_DIGEST_LENGTH) {
			log_debug("auth_validate: invalid key length, "
			    "interface %s", iface->name);
			return (-1);
		}

		if (len - ntohs(ospf_hdr->len) < MD5_DIGEST_LENGTH) {
			log_debug("auth_validate: invalid key length, "
			    "interface %s", iface->name);
			return (-1);
		}

		auth_data = buf;
		auth_data += ntohs(ospf_hdr->len);

		/* save the received digest and clear it in the packet */
		memcpy(recv_digest, auth_data, sizeof(recv_digest));
		bzero(auth_data, MD5_DIGEST_LENGTH);

		/* insert plaintext key */
		bzero(digest, MD5_DIGEST_LENGTH);
		strncpy(digest, md->key, MD5_DIGEST_LENGTH);

		/* calculate MD5 digest */
		MD5Init(&hash);
		MD5Update(&hash, buf, ntohs(ospf_hdr->len));
		MD5Update(&hash, digest, MD5_DIGEST_LENGTH);
		MD5Final(digest, &hash);

		if (memcmp(recv_digest, digest, sizeof(digest))) {
			log_debug("auth_validate: invalid MD5 digest, "
			    "interface %s", iface->name);
			return (-1);
		}

		if (nbr != NULL)
			nbr->crypt_seq_num =
			    ntohl(ospf_hdr->auth_key.crypt.seq_num);
		break;
	default:
		log_debug("auth_validate: unknown auth type, interface %s",
		    iface->name);
		return (-1);
	}

	return (0);
}

int
auth_gen(struct buf *buf, struct iface *iface)
{
	MD5_CTX		 hash;
	u_int8_t	 digest[MD5_DIGEST_LENGTH];
	struct ospf_hdr	*ospf_hdr;
	struct auth_md	*md;

	if ((ospf_hdr = buf_seek(buf, 0, sizeof(ospf_hdr))) == NULL)
		fatalx("auth_gen: buf_seek failed");

	/* update length */
	if (buf_size(buf) > USHRT_MAX)
		fatalx("auth_gen: resulting ospf packet too big");
	ospf_hdr->len = htons(buf_size(buf));
	/* clear auth_key field */
	bzero(ospf_hdr->auth_key.simple, sizeof(ospf_hdr->auth_key.simple));

	switch (iface->auth_type) {
	case AUTH_NONE:
		ospf_hdr->chksum = in_cksum(buf->buf, buf_size(buf));
		break;
	case AUTH_SIMPLE:
		ospf_hdr->chksum = in_cksum(buf->buf, buf_size(buf));

		strncpy(ospf_hdr->auth_key.simple, iface->auth_key,
		    sizeof(ospf_hdr->auth_key.simple));
		break;
	case AUTH_CRYPT:
		ospf_hdr->chksum = 0;
		ospf_hdr->auth_key.crypt.keyid = iface->auth_keyid;
		ospf_hdr->auth_key.crypt.seq_num = htonl(iface->crypt_seq_num);
		ospf_hdr->auth_key.crypt.len = MD5_DIGEST_LENGTH;
		iface->crypt_seq_num++;

		/* insert plaintext key */
		if ((md = md_list_find(&iface->auth_md_list,
		    iface->auth_keyid)) == NULL) {
			log_debug("auth_gen: keyid %d not configured, "
			    "interface %s", iface->auth_keyid, iface->name);
			return (-1);
		}

		bzero(digest, MD5_DIGEST_LENGTH);
		strncpy(digest, md->key, MD5_DIGEST_LENGTH);

		/* calculate MD5 digest */
		MD5Init(&hash);
		MD5Update(&hash, buf->buf, buf_size(buf));
		MD5Update(&hash, digest, MD5_DIGEST_LENGTH);
		MD5Final(digest, &hash);

		return (buf_add(buf, digest, MD5_DIGEST_LENGTH));
	default:
		log_debug("auth_gen: unknown auth type, interface %s",
		    iface->name);
		return (-1);
	}

	return (0);
}

/* md list */
void
md_list_add(struct auth_md_head *head, u_int8_t keyid, char *key)
{
	struct auth_md	*md;

	if ((md = md_list_find(head, keyid)) != NULL) {
		/* update key */
		strncpy(md->key, key, sizeof(md->key));
		return;
	}

	if ((md = calloc(1, sizeof(struct auth_md))) == NULL)
		fatalx("md_list_add");

	md->keyid = keyid;
	strncpy(md->key, key, sizeof(md->key));
	TAILQ_INSERT_TAIL(head, md, entry);
}

void
md_list_copy(struct auth_md_head *to, struct auth_md_head *from)
{
	struct auth_md	*m, *md;

	TAILQ_INIT(to);

	TAILQ_FOREACH(m, from, entry) {
		if ((md = calloc(1, sizeof(struct auth_md))) == NULL)
			fatalx("md_list_copy");

		md->keyid = m->keyid;
		strncpy(md->key, m->key, sizeof(md->key));
		TAILQ_INSERT_TAIL(to, md, entry);
	}
}

void
md_list_clr(struct auth_md_head *head)
{
	struct auth_md	*m;

	while ((m = TAILQ_FIRST(head)) != NULL) {
		TAILQ_REMOVE(head, m, entry);
		free(m);
	}
}

struct auth_md *
md_list_find(struct auth_md_head *head, u_int8_t keyid)
{
	struct auth_md	*m;

	TAILQ_FOREACH(m, head, entry)
		if (m->keyid == keyid)
			return (m);

	return (NULL);
}

int
md_list_send(struct auth_md_head *head, struct imsgev *to)
{
	struct auth_md	*m;

	TAILQ_FOREACH(m, head, entry)
		if (imsg_compose_event(to, IMSG_RECONF_AUTHMD, 0, 0, -1, m, 
		    sizeof(*m)) == -1)
			return (-1);

	return (0);
}
