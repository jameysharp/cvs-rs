/*
 * Copyright (C) 2005-2006 The Free Software Foundation, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

/* Verify interface.  */
#include "verify.h"

/* ANSI C headers.  */
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

/* POSIX.2 headers.  */
#include <getopt.h>

/* GNULIB headers.  */
#include "error.h"
#include "wait.h"
#include "xalloc.h"

/* CVS headers.  */
#include "base.h"
#include "entries.h"
#include "filesubr.h"
#include "gpg.h"
#include "parseinfo.h"
#include "recurse.h"
#include "repos.h"
#include "root.h"		/* Get current_parsed_root.  */
#include "run.h"
#include "server.h"
#include "stack.h"
#include "subr.h"
#include "system.h"



extern int noexec;
extern int really_quiet, quiet;
void usage (const char *const *cpp);



/*
 * Globals set via the command line parser in main.c.
 */

/* If a program capable of generating OpenPGP signatures couldn't be found at
 * configure time, default the sign state to off, otherwise, depend on the
 * server support.
 */
#ifdef HAVE_OPENPGP
static verify_state verify_checkouts = VERIFY_DEFAULT;
static verify_state verify_commits = VERIFY_DEFAULT;
#else
static verify_state verify_checkouts = VERIFY_OFF;
static verify_state verify_commits = VERIFY_OFF;
#endif

static char *verify_template;
static List *verify_args;



void
set_verify_checkouts (verify_state verify)
{
    TRACE (TRACE_FUNCTION, "set_verify_checkouts (%d)", verify);
    verify_checkouts = verify;
}



void
set_verify_template (const char *template)
{
    assert (template);
    if (verify_template) free (verify_template);
    verify_template = xstrdup (template);
}



void
add_verify_arg (const char *arg)
{
    if (!verify_args) verify_args = getlist ();
    push_string (verify_args, xstrdup (arg));
}



/* Return the current verify_state based on the command line options, current
 * cvsroot, and compiled default.
 *
 * INPUTS
 *   server_support	Whether the server supports signed files.
 *
 * GLOBALS
 *   server_active	Whether the server is active.
 *
 * ERRORS
 *   This function exits with a fatal error when the server does not support
 *   OpenPGP signatures and VERIFY_FATAL would otherwise be returned.
 *
 * RETURNS
 *   VERIFY_OFF, VERIFY_WARN, or VERIFY_FATAL.
 */
static verify_state
iget_verify_checkouts (bool server_support)
{
    verify_state tmp;

    /* Only verify checkouts from the client (and in local mode).  */
    if (server_active) return VERIFY_OFF;

    tmp = verify_checkouts;

    if (tmp == VERIFY_DEFAULT)
	tmp = current_parsed_root->verify;

    if (tmp == VERIFY_DEFAULT)
	tmp = VERIFY_FATAL;

    if (tmp == VERIFY_FATAL && !server_support)
	error (1, 0, "Server does not support OpenPGP signatures.");

    return tmp;
}



/* Return true if the client should attempt to verify files sent by the server.
 *
 * GLOBALS
 *   server_active	Whether the server is active.
 *
 * INPUTS
 *   server_support	Whether the server supports signed files.
 *
 * ERRORS
 *   This function exits with a fatal error if iget_verify_checkouts does.
 */
bool
get_verify_checkouts (bool server_support)
{
    verify_state tmp = iget_verify_checkouts (server_support);
    return tmp == VERIFY_WARN || tmp == VERIFY_FATAL;
}



/* Return true if a client failure to verify a checkout should be fatal.
 *
 * GLOBALS
 *   server_active	Whether the server is active (via
 *   			iget_verify_checkouts).
 */
bool
get_verify_checkouts_fatal (void)
{
    verify_state tmp = iget_verify_checkouts (true);
    return tmp == VERIFY_FATAL;
}



static const char *
verify_state_to_string (verify_state state)
{
    switch (state)
    {
	case VERIFY_FATAL:
	    return "VERIFY_FATAL";
	case VERIFY_WARN:
	    return "VERIFY_WARN";
	case VERIFY_OFF:
	    return "VERIFY_OFF";
	case VERIFY_DEFAULT:
	    return "VERIFY_DEFAULT";
	default:
	    error (1, 0, "Unknown verify_state %d", state);
	    return "Can't reach";
    }
}



/* Return the current verify_state based on the command line options, current
 * config, and compiled default.
 *
 * RETURNS
 *   VERIFY_OFF, VERIFY_WARN, or VERIFY_FATAL.
 */
static verify_state
iget_verify_commits (void)
{
    verify_state tmp;

    /* Only verify checkouts from the server (and in local mode).  */
    if (current_parsed_root->isremote) return VERIFY_OFF;

    tmp = verify_commits;

    if (config && tmp == VERIFY_DEFAULT)
	tmp = config->VerifyCommits;

    if (tmp == VERIFY_DEFAULT)
	tmp = VERIFY_OFF;

    TRACE (TRACE_DATA, "iget_verify_commits () returning %s",
	   trace >= TRACE_DATA ? verify_state_to_string (tmp) : "");

    return tmp;
}



/* Return true if the server should attempt to verify files sent by the client.
 */
bool
get_verify_commits (void)
{
    verify_state tmp = iget_verify_commits ();
    return tmp == VERIFY_WARN || tmp == VERIFY_FATAL;
}



bool
get_verify_commits_fatal (void)
{
    verify_state tmp = iget_verify_commits ();
    return tmp == VERIFY_FATAL;
}



/* Return VERIFY_TEMPLATE from the command line if it exists, else return the
 * VERIFY_TEMPLATE from CURRENT_PARSED_ROOT.
 */
static inline const char *
get_verify_template (void)
{
    if (verify_template) return verify_template;
    if (config && config->VerifyTemplate)
	return config->VerifyTemplate;
    if (current_parsed_root->verify_template)
	return current_parsed_root->verify_template;
    return DEFAULT_VERIFY_TEMPLATE;
}



/* Return VERIFY_ARGS from the command line if it exists, else return the
 * VERIFY_ARGS from CURRENT_PARSED_ROOT.
 */
static inline List *
get_verify_args (void)
{
    if (verify_args && !list_isempty (verify_args)) return verify_args;
    if (config && config->VerifyArgs && !list_isempty (config->VerifyArgs))
	return config->VerifyArgs;
    return current_parsed_root->verify_args;
}



/* This function is intended to be passed into walklist() with a list of args
 * to be substituted into the sign template.
 *
 * closure will be a struct format_cmdline_walklist_closure
 * where closure is undefined.
 */
static int
verify_args_list_to_args_proc (Node *p, void *closure)
{
    struct format_cmdline_walklist_closure *c = closure;
    char *arg = NULL;
    const char *f;
    char *d;
    size_t doff;

    if (p->data == NULL) return 1;

    f = c->format;
    d = *c->d;
    /* foreach requested attribute */
    while (*f)
    {
	switch (*f++)
	{
	    case 'a':
		arg = p->key;
		break;
	    default:
		error (1, 0,
		       "Unknown format character or not a list attribute: %c",
		       f[-1]);
		/* NOTREACHED */
		break;
	}
	/* copy the attribute into an argument */
	if (c->quotes)
	{
	    arg = cmdlineescape (c->quotes, arg);
	}
	else
	{
	    arg = cmdlinequote ('"', arg);
	}

	doff = d - *c->buf;
	expand_string (c->buf, c->length, doff + strlen (arg));
	d = *c->buf + doff;
	strncpy (d, arg, strlen (arg));
	d += strlen (arg);
	free (arg);

	/* Always put the extra space on.  we'll have to back up a char
	 * when we're done, but that seems most efficient.
	 */
	doff = d - *c->buf;
	expand_string (c->buf, c->length, doff + 1);
	d = *c->buf + doff;
	*d++ = ' ';
    }
    /* correct our original pointer into the buff */
    *c->d = d;
    return 0;
}



/* Verify a signature for the data in WORKFILE, returning true or false.  If
 * SIG is set, it must contain signature data of length of length SIGLEN.
 * Otherwise, assume WORKFILE.sig contains the signature data.
 *
 * INPUTS
 *   finfo	File information on the file being signed.
 *
 * ERRORS
 *   Exits with a fatal error when FATAL and a signature cannot be verified.
 */
bool
verify_signature (const char *srepos, const char *sig, size_t siglen,
		  const char *filename, bool bin, bool fatal)
{
    char *cmdline;
    char *sigfile;
    FILE *pipefp;
    bool save_noexec = noexec;
    int pipestatus;
    bool retval;

    if (sig)
	sigfile = "-";
    else
	sigfile = Xasprintf ("%s%s", filename, ".sig");

    if (!sig && !isfile (sigfile))
    {
	error (fatal, 0, "No signature file found (`%s')", sigfile);
	free (sigfile);
	return false;
    }

    /*
     * %p = shortrepos
     * %r = repository
     * %{@} = user defined sign args
     * %M = textmode flag
     * %s = signature file name
     * %d = signed (data) file name
     */
    /*
     * Cast any NULL arguments as appropriate pointers as this is an
     * stdarg function and we need to be certain the caller gets what
     * is expected.
     */
    cmdline = format_cmdline (
#ifdef SUPPORT_OLD_INFO_FMT_STRINGS
	                      false, srepos,
#endif /* SUPPORT_OLD_INFO_FMT_STRINGS */
	                      "verify template", 1, get_verify_template (),
	                      "@", ",", get_verify_args (),
			      verify_args_list_to_args_proc, (void *) NULL,
	                      "r", "s", current_parsed_root->directory,
	                      "p", "s", srepos,
	                      "M", "s", bin ? NULL : get_openpgp_textmode (),
	                      "S", "s", sigfile,
	                      "s", "s", filename,
	                      (char *) NULL);

    if (!cmdline || !strlen (cmdline))
    {
	error (fatal, 0, "verify template resolved to the empty string!");
	if (cmdline) free (cmdline);
	free (sigfile);
	return false;
    }

    noexec = false;
    if (!(pipefp = run_popen (cmdline, "w" POPEN_BINARY_FLAG)))
    {
	error (fatal, errno, "failed to execute signature verifier");
	retval = false;
	goto done;
    }
    noexec = save_noexec;

    if (sig)
    {
	size_t len;
	len = fwrite (sig, sizeof *sig, siglen, pipefp);
	if (len < siglen)
	    error (0, ferror (pipefp), "Error writing to verify program.");
    }

    pipestatus = pclose (pipefp);
    if (pipestatus == -1)
    {
	error (fatal, errno,
	       "failed to obtain exit status from verify program");
	retval = false;
    }
    else if (pipestatus)
    {
	if (WIFEXITED (pipestatus))
	    error (fatal, 0,
		   "failed to verify `%s': exited with error code %d",
		   filename, WEXITSTATUS (pipestatus));
	else
	    error (fatal, 0, "failed to verify `%s': exited via signal %d",
		   filename, WTERMSIG (pipestatus));
	retval = false;
    }
    else
	retval = true;

done:
    if (!sig)
	free (sigfile);
    free (cmdline);

    return retval;
}



static const char *const verify_usage[] =
{
    "Usage: %s %s [-lpR] [path...]\n",
    "\t-l\tLocal directory only, no recursion.\n",
    "\t-R\tProcess directories recursively.\n",
    "\t-p\tOutput signature to STDOUT without verifying.\n",
    "(Specify the --help global option for a list of other help options)\n",
    NULL
};



struct verify_closure
{
    bool pipeout;
};



/*
 * GLOBALS
 *   CURRENT_PARSED_ROOT->ISREMOTE
 *
 * NOTES
 *   Need to deal with all 4 combinations of USERARGS->PIPEOUT &
 *   CURRENT_PARSED_ROOT->ISREMOTE.
 */
static int
verify_fileproc (void *callerdat, struct file_info *finfo)
{
    struct verify_closure *userargs = callerdat;
    Node *n;
    bool bin;
    Entnode *e;
    bool errors = false;
    char *basefn, *basesigfn;
    char *tmpfn = NULL, *tmpsigfn = NULL;
    const char *signedfn = NULL;
    char *sigdata = NULL;
    size_t buflen;
    size_t siglen;

    n = findnode (finfo->entries, finfo->file);
    assert (n);

    e = n->data;
    bin = STREQ (e->options, "-kb");

    basefn = make_base_file_name (finfo->file, e->version);
    basesigfn = Xasprintf ("%s%s", basefn, ".sig");

    if (current_parsed_root->isremote)
    {
	char *updateprefix = finfo->update_dir && *finfo->update_dir
			     ? Xasprintf ("%s/", finfo->update_dir)
			     : xstrdup ("");
	char *fullbasefn = Xasprintf ("%s%s", updateprefix, basefn);
	char *fullsigfn = Xasprintf ("%s%s", updateprefix, basesigfn);

	/* FIXME: These errors should attempt a refetch instead.  */
	if (!isfile (basefn))
	    error (1, 0, "Base file missing `%s'", fullbasefn);

	if (!isfile (basesigfn))
	{
	    error (0, 0, "No signature available for `%s'", finfo->fullname);
	    errors = true;
	}

	/* FIXME: Once a "soft" connect to the server is possible, then when
	 * the server is available, the signatures should be updated here.
	 */

	if (!errors)
	{
	    if (userargs->pipeout)
		get_file (basesigfn, fullsigfn, "rb",
			  &sigdata, &buflen, &siglen);
	    else
	    {
		signedfn = basefn;
	    }
	}

	free (updateprefix);
	free (fullbasefn);
	free (fullsigfn);
    }
    else
    {
	/* In local mode, the signature data is still in the archive.  */
	assert (finfo->rcs);
	if (!RCS_get_openpgp_signatures (finfo, e->version, &sigdata,
					 &siglen))
	{
	    error (0, 0, "Failed to decode base64 signature for `%s'",
		   finfo->fullname);
	    errors = true;
	}
	else if (!sigdata)
	{
	    error (0, 0, "No signature available for `%s'",
		   finfo->fullname);
	    errors = true;
	}
    }

    /* In the remote/non-pipeout case, the signed data and the signature are
     * still in the base file and its signature counterpart, respectively.  In
     * the other three cases, the signature is in SIGDATA or ERRORS is true
     * (on error).
     */

    if (!errors && sigdata)
    {
	if (!siglen)
	{
	    error (0, 0, "No signature data found for `%s'",
		   finfo->fullname);
	    errors = true;
	}
	else if (userargs->pipeout)
	    /* First deal with both the piped cases - it's easy.  */
	    cvs_output (sigdata, siglen);
	else
	{
	    /* This must be the local case, where the signature had to be
	     * loaded from the archive.  Write the clean file and the signature
	     * to temp files.
	     */
	    assert (!current_parsed_root->isremote);

	    signedfn = tmpfn = cvs_temp_name ();

	    if (RCS_checkout (finfo->rcs, NULL, e->version, e->tag, e->options,
			      tmpfn, NULL, NULL))
		errors = true;
	    else
	    {
		tmpsigfn = Xasprintf ("%s.sig", tmpfn);
		force_write_file (tmpsigfn, sigdata, siglen);
	    }
	}
    }

    if (!errors && !userargs->pipeout)
	errors = !verify_signature (Short_Repository (finfo->repository),
				    NULL, 0, signedfn, bin, false);

    if (tmpfn)
    {
	if (CVS_UNLINK (tmpfn))
	    error (0, 0, "Failed to remove temp file `%s'", tmpfn);
	free (tmpfn);
    }
    if (tmpsigfn)
    {
	if (CVS_UNLINK (tmpsigfn))
	    error (0, 0, "Failed to remove temp file `%s'", tmpsigfn);
	free (tmpsigfn);
    }
    if (sigdata) free (sigdata);
    free (basefn);
    free (basesigfn);
    return errors;
}



int
verify (int argc, char **argv)
{
    bool local = false;
    int c;
    int err;
    struct verify_closure userargs;

    if (argc == -1)
	usage (verify_usage);

    /* parse the args */
    userargs.pipeout = false;
    optind = 0;
    while ((c = getopt (argc, argv, "+lRp")) != -1)
    {
	switch (c)
	{
	    case 'l':
		local = true;
		break;
	    case 'R':
		local = false;
		break;
	    case 'p':
		userargs.pipeout = true;
		break;
	    case '?':
	    default:
		usage (verify_usage);
		break;
	}
    }
    argc -= optind;
    argv += optind;

    /* call the recursion processor */
    err = start_recursion (verify_fileproc, NULL, NULL, NULL, &userargs,
			   argc, argv, local, W_LOCAL, false, CVS_LOCK_NONE,
			   NULL, 1, NULL);

    return err;
}
