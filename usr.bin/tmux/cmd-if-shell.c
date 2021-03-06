/* $OpenBSD: cmd-if-shell.c,v 1.13 2011/10/18 08:57:01 nicm Exp $ */

/*
 * Copyright (c) 2009 Tiago Cunha <me@tiagocunha.org>
 * Copyright (c) 2009 Nicholas Marriott <nicm@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF MIND, USE, DATA OR PROFITS, WHETHER
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/types.h>
#include <sys/wait.h>

#include <string.h>

#include "tmux.h"

/*
 * Executes a tmux command if a shell command returns true.
 */

int	cmd_if_shell_check(struct args *);
int	cmd_if_shell_exec(struct cmd *, struct cmd_ctx *);

void	cmd_if_shell_callback(struct job *);
void	cmd_if_shell_free(void *);

const struct cmd_entry cmd_if_shell_entry = {
	"if-shell", "if",
	"", 2, 4,
	"shell-command command [else command]",
	0,
	NULL,
	cmd_if_shell_check,
	cmd_if_shell_exec
};

struct cmd_if_shell_data {
	char		*cmd_if;
	char		*cmd_else;
	struct cmd_ctx	 ctx;
};

int
cmd_if_shell_check(struct args *args)
{
	if (args->argc == 3)
		return (-1);
	if (args->argc == 4 && strcmp(args->argv[2], "else") != 0)
		return (-1);
	return (0);
}

int
cmd_if_shell_exec(struct cmd *self, struct cmd_ctx *ctx)
{
	struct args			*args = self->args;
	struct cmd_if_shell_data	*cdata;
	const char			*shellcmd = args->argv[0];

	cdata = xmalloc(sizeof *cdata);
	cdata->cmd_if = xstrdup(args->argv[1]);
	if (args->argc == 4)
		cdata->cmd_else = xstrdup(args->argv[3]);
	else
		cdata->cmd_else = NULL;
	memcpy(&cdata->ctx, ctx, sizeof cdata->ctx);

	if (ctx->cmdclient != NULL)
		ctx->cmdclient->references++;
	if (ctx->curclient != NULL)
		ctx->curclient->references++;

	job_run(shellcmd, cmd_if_shell_callback, cmd_if_shell_free, cdata);

	return (1);	/* don't let client exit */
}

void
cmd_if_shell_callback(struct job *job)
{
	struct cmd_if_shell_data	*cdata = job->data;
	struct cmd_ctx			*ctx = &cdata->ctx;
	struct cmd_list			*cmdlist;
	char				*cmd;
	char				*cause;

	if (!WIFEXITED(job->status) || WEXITSTATUS(job->status) != 0) {
		cmd = cdata->cmd_else;
		if (cmd == NULL)
			return;
	} else
		cmd = cdata->cmd_if;
	if (cmd_string_parse(cmd, &cmdlist, &cause) != 0) {
		if (cause != NULL) {
			ctx->error(ctx, "%s", cause);
			xfree(cause);
		}
		return;
	}

	cmd_list_exec(cmdlist, ctx);
	cmd_list_free(cmdlist);
}

void
cmd_if_shell_free(void *data)
{
	struct cmd_if_shell_data	*cdata = data;
	struct cmd_ctx			*ctx = &cdata->ctx;
	struct msg_exit_data		 exitdata;

	if (ctx->cmdclient != NULL) {
		ctx->cmdclient->references--;
		exitdata.retcode = ctx->cmdclient->retcode;
		ctx->cmdclient->flags |= CLIENT_EXIT;
	}
	if (ctx->curclient != NULL)
		ctx->curclient->references--;

	if (cdata->cmd_else != NULL)
		xfree(cdata->cmd_else);
	xfree(cdata->cmd_if);
	xfree(cdata);
}
