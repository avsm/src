/*
 * Copyright (c) 1994 Mats O Jansson <moj@stacken.kth.se>
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
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef LINT
static char rcsid[] = "$Id: yppasswdd_proc.c,v 1.3 1995/10/24 21:22:05 moj Exp $";
#endif

#include <sys/types.h>
#include <rpc/rpc.h>
#include <stdio.h>
#include <string.h>

#include "yppasswd.h"

int make_passwd __P((yppasswd *));

int *
yppasswdproc_update_1_svc(argp, rqstp, transp)
	yppasswd *argp;
        struct svc_req *rqstp;
	SVCXPRT *transp;
{
	static int res;

	bzero((char *)&res, sizeof(res));
	res = make_passwd(argp);

	if (!svc_sendreply(transp, xdr_int, (char *)&res))
		svcerr_systemerr(transp);

	if (!svc_freeargs(transp, xdr_yppasswd, (caddr_t) argp)) {
		(void)fprintf(stderr, "unable to free arguments\n");
		exit(1);
	}
	return ((void *)&res);
}
