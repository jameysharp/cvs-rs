/*
 * Copyright (C) 2005-2007 The Free Software Foundation, Inc.
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
#include "base.h"

/* Standards  */
#include <assert.h>

/* GNULIB */
#include "quote.h"

/* CVS headers.  */
#include "difflib.h"
#include "server.h"
#include "subr.h"

#include "cvs.h"	/* For CVSADM_BASE. */



char *
make_base_file_name (const char *filename, const char *rev)
{
    return Xasprintf ("%s/.#%s.%s", CVSADM_BASE, filename, rev);
}



/* OK, the following base_* code tracks the revisions of the files in
   CVS/Base.  We do this in a file CVS/Baserev.  Separate from
   CVS/Entries because it needs to go in separate data structures
   anyway (the name in Entries must be unique), so this seemed
   cleaner.  The business of rewriting the whole file in
   base_deregister and base_register is the kind of thing we used to
   do for Entries and which turned out to be slow, which is why there
   is now the Entries.Log machinery.  So maybe from that point of
   view it is a mistake to do this separately from Entries, I dunno.  */

enum base_walk
{
    /* Set the revision for FILE to *REV.  */
    BASE_REGISTER,
    /* Get the revision for FILE and put it in a newly malloc'd string
       in *REV, or put NULL if not mentioned.  */
    BASE_GET,
    /* Remove FILE.  */
    BASE_DEREGISTER
};



/* Read through the lines in CVS/Baserev, taking the actions as documented
   for CODE.  */
static void
base_walk (enum base_walk code, const char *update_dir, const char *file,
	   char **rev)
{
    FILE *fp;
    char *line;
    size_t line_allocated;
    FILE *newf;
    char *baserev_fullname;
    char *baserevtmp_fullname;

    line = NULL;
    line_allocated = 0;
    newf = NULL;

    /* First compute the fullnames for the error messages.  This
       computation probably should be broken out into a separate function,
       as recurse.c does it too and places like Entries_Open should be
       doing it.  */
    if (update_dir[0] != '\0')
    {
	baserev_fullname = Xasprintf ("%s/%s", update_dir,
				      CVSADM_BASEREV);
	baserevtmp_fullname = Xasprintf ("%s/%s", update_dir,
					 CVSADM_BASEREVTMP);
    }
    else
    {
	baserev_fullname = xstrdup (CVSADM_BASEREV);
	baserevtmp_fullname = xstrdup (CVSADM_BASEREVTMP);
    }

    fp = CVS_FOPEN (CVSADM_BASEREV, "r");
    if (!fp)
    {
	if (!existence_error (errno))
	{
	    error (0, errno, "cannot open %s for reading", baserev_fullname);
	    goto out;
	}
    }

    switch (code)
    {
	case BASE_REGISTER:
	case BASE_DEREGISTER:
	    newf = CVS_FOPEN (CVSADM_BASEREVTMP, "w");
	    if (!newf)
	    {
		error (0, errno, "cannot open %s for writing",
		       baserevtmp_fullname);
		goto out;
	    }
	    break;
	case BASE_GET:
	    *rev = NULL;
	    break;
    }

    if (fp)
    {
	while (getline (&line, &line_allocated, fp) >= 0)
	{
	    char *linefile;
	    char *p;
	    char *linerev;

	    if (line[0] != 'B')
		/* Ignore, for future expansion.  */
		continue;

	    linefile = line + 1;
	    p = strchr (linefile, '/');
	    if (!p)
		/* Syntax error, ignore.  */
		continue;
	    linerev = p + 1;
	    p = strchr (linerev, '/');
	    if (!p) continue;

	    linerev[-1] = '\0';
	    if (fncmp (linefile, file) == 0)
	    {
		switch (code)
		{
		case BASE_REGISTER:
		case BASE_DEREGISTER:
		    /* Don't copy over the old entry, we don't want it.  */
		    break;
		case BASE_GET:
		    *p = '\0';
		    *rev = xstrdup (linerev);
		    *p = '/';
		    goto got_it;
		}
	    }
	    else
	    {
		linerev[-1] = '/';
		switch (code)
		{
		case BASE_REGISTER:
		case BASE_DEREGISTER:
		    if (fprintf (newf, "%s\n", line) < 0)
			error (0, errno, "error writing %s",
			       baserevtmp_fullname);
		    break;
		case BASE_GET:
		    break;
		}
	    }
	}
	if (ferror (fp))
	    error (0, errno, "cannot read %s", baserev_fullname);
    }
 got_it:

    if (code == BASE_REGISTER)
    {
	if (fprintf (newf, "B%s/%s/\n", file, *rev) < 0)
	    error (0, errno, "error writing %s",
		   baserevtmp_fullname);
    }

 out:

    if (line) free (line);

    if (fp)
    {
	if (fclose (fp) < 0)
	    error (0, errno, "cannot close %s", baserev_fullname);
    }
    if (newf)
    {
	if (fclose (newf) < 0)
	    error (0, errno, "cannot close %s", baserevtmp_fullname);
	rename_file (CVSADM_BASEREVTMP, CVSADM_BASEREV);
    }

    free (baserev_fullname);
    free (baserevtmp_fullname);
}



/* Return, in a newly malloc'd string, the revision for FILE in CVS/Baserev,
 * or NULL if not listed.
 */
char *
base_get (const char *update_dir, const char *file)
{
    char *rev;
    base_walk (BASE_GET, update_dir, file, &rev);
    return rev;
}



/* Set the revision for FILE to REV.  */
void
base_register (const char *update_dir, const char *file, char *rev)
{
    base_walk (BASE_REGISTER, update_dir, file, &rev);
}



/* Remove FILE.  */
void
base_deregister (const char *update_dir, const char *file)
{
    base_walk (BASE_DEREGISTER, update_dir, file, NULL);
}



int
base_checkout (RCSNode *rcs, struct file_info *finfo,
	       const char *prev, const char *rev, const char *ptag,
	       const char *tag, const char *poptions, const char *options)
{
    int status;
    char *basefile;

    TRACE (TRACE_FUNCTION, "base_checkout (%s, %s, %s, %s, %s, %s, %s)",
	   finfo->fullname, prev, rev, ptag, tag, poptions, options);

    if (noexec)
	return 0;

    cvs_xmkdir (CVSADM_BASE, NULL, MD_EXIST_OK);

    assert (!current_parsed_root->isremote);

    basefile = make_base_file_name (finfo->file, rev);
    status = RCS_checkout (rcs, basefile, rev, tag, options,
			   NULL, NULL, NULL);

    /* Always mark base files as read-only, to make disturbing them
     * accidentally at least slightly challenging.
     */
    xchmod (basefile, false);
    free (basefile);

    /* FIXME: Verify the signature in local mode.  */

#ifdef SERVER_SUPPORT
    if (server_active && !STREQ (cvs_cmd_name, "export"))
	server_base_checkout (rcs, finfo, prev, rev, ptag, tag,
			      poptions, options);
#endif

    return status;
}



char *
temp_checkout (RCSNode *rcs, struct file_info *finfo,
	       const char *prev, const char *rev, const char *ptag,
	       const char *tag, const char *poptions, const char *options)
{
    char *tempfile;
    bool save_noexec;

    TRACE (TRACE_FUNCTION, "temp_checkout (%s, %s, %s, %s, %s, %s, %s)",
	   finfo->fullname, prev, rev, ptag, tag, poptions, options);

    assert (!current_parsed_root->isremote);

    tempfile = cvs_temp_name ();
    save_noexec = noexec;
    noexec = false;
    if (RCS_checkout (rcs, tempfile, rev, tag, options, NULL, NULL, NULL))
    {
	error (0, 0, "Failed to check out revision %s of `%s'",
	       rev, finfo->fullname);
	free (tempfile);
	noexec = save_noexec;
	return NULL;
    }
    noexec = save_noexec;

    assert (!STREQ (cvs_cmd_name, "export"));

#ifdef SERVER_SUPPORT
    if (server_active)
	server_temp_checkout (rcs, finfo, prev, rev, ptag, tag,
			      poptions, options, tempfile);
#endif

    return tempfile;
}



enum update_existing
translate_exists (const char *exists)
{
    if (*exists == 'n') return UPDATE_ENTRIES_NEW;
    if (*exists == 'y') return UPDATE_ENTRIES_EXISTING;
    if (*exists == 'm') return UPDATE_ENTRIES_EXISTING_OR_NEW;
    /* The error below should only happen due to a programming error or when
     * the server sends a bad response.
     */
    error (1, 0, "unknown existence code received from server: `%s'",
	   exists);

    /* Placate GCC.  */
    assert (!"Internal error");
    return UPDATE_ENTRIES_EXISTING_OR_NEW;
}



/* Validate the existance of FILENAME against whether we think it should exist
 * or not.  If it should exist and doesn't, issue a warning and return success.
 * If it shouldn't exist and does, issue a warning and return false to avoid
 * accidentally overwriting a user's changes.
 */
bool
validate_change (enum update_existing existp, const struct file_info *finfo)
{
    /* Note that checking this separately from writing the file is
       a race condition: if the existence or lack thereof of the
       file changes between now and the actual calls which
       operate on it, we lose.  However (a) there are so many
       cases, I'm reluctant to try to fix them all, (b) in some
       cases the system might not even have a system call which
       does the right thing, and (c) it isn't clear this needs to
       work.  */
    if (existp == UPDATE_ENTRIES_EXISTING
	&& !isfile (finfo->file))
	/* Emit a warning and update the file anyway.  */
	error (0, 0, "warning: %s unexpectedly disappeared",
	       quote (finfo->fullname));
    else if (existp == UPDATE_ENTRIES_NEW
	&& isfile (finfo->file))
    {
	/* This error might be confusing; it isn't really clear to
	   the user what to do about it.  Keep in mind that it has
	   several causes: (1) something/someone creates the file
	   during the time that CVS is running, (2) the repository
	   has two files whose names clash for the client because
	   of case-insensitivity or similar causes, See 3 for
	   additional notes.  (3) a special case of this is that a
	   file gets renamed for example from a.c to A.C.  A
	   "cvs update" on a case-insensitive client will get this
	   error.  In this case and in case 2, the filename
	   (short_pathname) printed in the error message will likely _not_
	   have the same case as seen by the user in a directory listing.
	   (4) the client has a file which the server doesn't know
	   about (e.g. "? foo" file), and that name clashes with a file
	   the server does know about, (5) classify.c will print the same
	   message for other reasons.

	   I hope the above paragraph makes it clear that making this
	   clearer is not a one-line fix.  */
	error (0, 0, "move away %s; it is in the way",
	       quote (finfo->fullname));

	/* The Mode, Mod-time, and Checksum responses should not carry
	 * over to a subsequent Created (or whatever) response, even
	 * in the error case.
	 */
	if (!really_quiet)
	{
	    cvs_output ("C ", 0);
	    cvs_output (finfo->fullname, 0);
	    cvs_output ("\n", 1);
	}
	return false;
    }

    return true;
}



static void
ibase_copy (struct file_info *finfo, const char *rev, const char *flags,
	    const char *tempfile)
{
    const char *basefile;

    TRACE (TRACE_FUNCTION, "ibase_copy (%s, %s, %s, %s)",
	   finfo->fullname, rev, flags,
	   tempfile ? tempfile : "(null)");

    assert (flags && flags[0] && flags[1]);

    if (!server_active /* The server still doesn't stay perfectly in sync with
			* the client workspace.
			*/
	&& !validate_change (translate_exists (flags), finfo))
	exit (EXIT_FAILURE);

    if (noexec)
	return;

    if (tempfile)
	basefile = tempfile;
    else
	basefile = make_base_file_name (finfo->file, rev);

    if (isfile (finfo->file))
	xchmod (finfo->file, true);

    copy_file (basefile, finfo->file);
    if (flags[1] == 'y')
	xchmod (finfo->file, true);

#ifdef SERVER_SUPPORT
    if (server_active && !STREQ (cvs_cmd_name, "export"))
	server_base_copy (finfo, rev ? rev : "", flags);
#endif

    if (suppress_bases || tempfile)
    {
	char *sigfile = Xasprintf ("%s.sig", basefile);
	if (CVS_UNLINK (basefile) < 0)
	    error (0, errno, "Failed to remove temp file `%s'", basefile);
	if (CVS_UNLINK (sigfile) < 0 && !existence_error (errno))
	    error (0, errno, "Failed to remove temp file `%s'", sigfile);
	free (sigfile);
    }
    if (!tempfile)
	free ((char *)basefile);
}



void
temp_copy (struct file_info *finfo, const char *flags, const char *tempfile)
{
    ibase_copy (finfo, NULL, flags, tempfile);
}



void
base_copy (struct file_info *finfo, const char *rev, const char *flags)
{
    ibase_copy (finfo, rev, flags, NULL);
}



/* Remove the base file for FILE & REV, and any sigfile present for the same.
 */
void
base_remove (const char *file, const char *rev)
{
    char *basefile;
    char *sigfile;

    TRACE (TRACE_FUNCTION, "base_remove (%s, %s)", file, rev);

    if (*rev == '-') rev++;
    basefile = make_base_file_name (file, rev);
    if (unlink_file (basefile) < 0 && !existence_error (errno))
	error (0, errno, "Failed to remove `%s'", basefile);
    sigfile = Xasprintf ("%s.sig", basefile);
    if (unlink_file (sigfile) < 0 && !existence_error (errno))
	error (0, errno, "Failed to remove `%s'", sigfile);
    free (sigfile);
    free (basefile);
}



/* Merge revisions REV1 and REV2. */
int
base_merge (RCSNode *rcs, struct file_info *finfo, const char *ptag,
	    const char *poptions, const char *options, const char *urev,
	    const char *rev1, const char *rev2, bool join)
{
    char *f1, *f2;
    int retval;

    assert (!options || !options[0]
	    || (options[0] == '-' && options[1] == 'k'));

    /* Check out chosen revisions.  The error message when RCS_checkout
       fails is not very informative -- it is taken verbatim from RCS 5.7,
       and relies on RCS_checkout saying something intelligent upon failure. */

    if (!(f1 = temp_checkout (rcs, finfo, urev, rev1, ptag, rev1, poptions,
			      options)))
	error (1, 0, "checkout of revision %s of `%s' failed.\n",
	       rev1, finfo->fullname);
    if (join || noexec || suppress_bases)
    {
	if (!(f2 = temp_checkout (rcs, finfo, urev, rev2, ptag, rev2, poptions,
				  options)))
	    error (1, 0, "checkout of revision %s of `%s' failed.\n",
		   rev2, finfo->fullname);
    }
    else
    {
	if (base_checkout (rcs, finfo, urev, rev2, ptag, rev2, poptions,
			   options))
	    error (1, 0, "checkout of revision %s of `%s' failed.\n",
		   rev2, finfo->fullname);
	f2 = make_base_file_name (finfo->file, rev2);
    }


    if (!server_active || !server_use_bases())
    {
	/* Merge changes. */
	/* It may violate the current abstraction to fail to generate the same
	 * files on the server as will be generated on the client, but I do not
	 * believe that they are being used currently and it saves server CPU.
	 */
	if (!really_quiet)
	{
	    cvs_output ("Merging differences between ", 0);
	    cvs_output (rev1, 0);
	    cvs_output (" and ", 5);
	    cvs_output (rev2, 0);
	    cvs_output (" into `", 7);
	    if (!finfo->update_dir || STREQ (finfo->update_dir, "."))
		cvs_output (finfo->file, 0);
	    else
		cvs_output (finfo->fullname, 0);
	    cvs_output ("'\n", 2);
	}

	retval = merge (finfo->file, finfo->file, f1, rev1, f2, rev2);
    }
    else
	retval = 0;

#ifdef SERVER_SUPPORT
    if (server_active)
	server_base_merge (finfo, rev1, rev2);
#endif

    if (CVS_UNLINK (f1) < 0)
	error (0, errno, "unable to remove %s", quote (f1));
    /* Using a base file instead of a temp file here and not deleting it is an
     * optimization since, for instance, on merge from 1.1 to 1.4 with local
     * changes, the client is going to want to leave base 1.4 and delete base
     * 1.1 rather than the other way around.
     */
    if (join || noexec || suppress_bases)
    {
	char *sigfile = Xasprintf ("%s.sig", f2);
	if (CVS_UNLINK (f2) < 0)
	    error (0, errno, "unable to remove %s", quote (f2));
	if (CVS_UNLINK (sigfile) < 0 && !existence_error (errno))
	    error (0, errno, "unable to remove %s", quote (sigfile));
	free (sigfile);
    }
    free (f1);
    free (f2);

    return retval;
}



/* Merge revisions REV1 and REV2. */
int
base_diff (const struct file_info *finfo,
	   int diff_argc, char *const *diff_argv,
	   const char *f1, const char *use_rev1, const char *label1,
	   const char *f2, const char *use_rev2, const char *label2,
	   bool empty_files)
{
    int status, err;

#ifdef SERVER_SUPPORT
    if (server_use_bases ())
    {
	server_base_diff (finfo, f1, use_rev1, label1, f2, use_rev2, label2);

	err = 0;
    }
    else
#endif
    if (xcmp (f1, f2))
    {
	RCS_output_diff_options (diff_argc, diff_argv, empty_files,
				 use_rev1, use_rev2, finfo->fullname);

	status = diff_exec (f1, f2, label1, label2, diff_argc, diff_argv,
			    RUN_TTY);

	switch (status)
	{
	    case -1:			/* fork failed */
		error (2, errno, "fork failed while diffing %s",
		       finfo->fullname);
	    case 0:				/* everything ok */
		err = 0;
		break;
	    default:			/* other error */
		err = status;
		break;
	}
    }
    else
	err = 0;

    return err;
}
