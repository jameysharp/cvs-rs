/*
 * Copyright (C) 1986-2005 The Free Software Foundation, Inc.
 *
 * Portions Copyright (C) 1998-2005 Derek Price, Ximbiot <http://ximbiot.com>,
 *                                  and others.
 *
 * Portions Copyright (C) 1992, Brian Berliner and Jeff Polk
 * Portions Copyright (C) 1989-1992, Brian Berliner
 * 
 * You may distribute under the terms of the GNU General Public License as
 * specified in the README file that comes with the CVS source distribution.
 * 
 * The functions in this file provide an interface for performing 
 * operations directly on RCS files. 
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

/* Standard headers.  */
#include <stdio.h>

/* GNULIB */
#include "quotearg.h"

/* CVS */
#include "difflib.h"
#include "cvs.h"

/* This file, rcs.h, and rcs.c, together sometimes known as the "RCS
   library", are intended to define our interface to RCS files.

   Whether there will also be a version of RCS which uses this
   library, or whether the library will be packaged for uses beyond
   CVS or RCS (many people would like such a thing) is an open
   question.  Some considerations:

   1.  An RCS library for CVS must have the capabilities of the
   existing CVS code which accesses RCS files.  In particular, simple
   approaches will often be slow.

   2.  An RCS library should not use code from the current RCS
   (5.7 and its ancestors).  The code has many problems.  Too few
   comments, too many layers of abstraction, too many global variables
   (the correct number for a library is zero), too much intricately
   interwoven functionality, and too many clever hacks.  Paul Eggert,
   the current RCS maintainer, agrees.

   3.  More work needs to be done in terms of separating out the RCS
   library from the rest of CVS (for example, cvs_output should be
   replaced by a callback, and the declarations should be centralized
   into rcs.h, and probably other such cleanups).

   4.  To be useful for RCS and perhaps for other uses, the library
   may need features beyond those needed by CVS.

   5.  Any changes to the RCS file format *must* be compatible.  Many,
   many tools (not just CVS and RCS) can at least import this format.
   RCS and CVS must preserve the current ability to import/export it
   (preferably improved--magic branches are currently a roadblock).
   See doc/RCSFILES in the CVS distribution for documentation of this
   file format.

   On a related note, see the comments at diff_exec, later in this file,
   for more on the diff library.  */



/* Stuff to deal with passing arguments the way libdiff.a wants to deal
   with them.  This is a crufty interface; there is no good reason for it
   to resemble a command line rather than something closer to "struct
   log_data" in log.c.  */


/* Show differences between two files.  This is the start of a diff library.

   Some issues:

   * Should option parsing be part of the library or the caller?  The
   former allows the library to add options without changing the callers,
   but it causes various problems.  One is that something like --brief really
   wants special handling in CVS, and probably the caller should retain
   some flexibility in this area.  Another is online help (the library could
   have some feature for providing help, but how does that interact with
   the help provided by the caller directly?).  Another is that as things
   stand currently, there is no separate namespace for diff options versus
   "cvs diff" options like -l (that is, if the library adds an option which
   conflicts with a CVS option, it is trouble).

   * This isn't required for a first-cut diff library, but if there
   would be a way for the caller to specify the timestamps that appear
   in the diffs (rather than the library getting them from the files),
   that would clean up the kludgy utime() calls in patch.c.

   Show differences between FILE1 and FILE2.  Either one can be
   DEVNULL to indicate a nonexistent file (same as an empty file
   currently, I suspect, but that may be an issue in and of itself).
   OPTIONS is a list of diff options, or "" if none.  At a minimum,
   CVS expects that -c (update.c, patch.c) and -n (update.c) will be
   supported.  Other options, like -u, --speed-large-files, &c, will
   be specified if the user specified them.

   OUT is a filename to send the diffs to, or RUN_TTY to send them to
   stdout.  Error messages go to stderr.  Return value is 0 for
   success, -1 for a failure which set errno, 1 for success (and some
   differences were found), or >1 for a failure which printed a
   message on stderr.  */

int
diff_exec (const char *file1, const char *file2, const char *label1,
           const char *label2, int dargc, char * const *dargv,
	   const char *out)
{
    TRACE (TRACE_FUNCTION, "diff_exec (%s, %s, %s, %s, %s)",
	   file1, file2, label1, label2, out);

#ifdef PRESERVE_PERMISSIONS_SUPPORT
    /* If either file1 or file2 are special files, pretend they are
       /dev/null.  Reason: suppose a file that represents a block
       special device in one revision becomes a regular file.  CVS
       must find the `difference' between these files, but a special
       file contains no data useful for calculating this metric.  The
       safe thing to do is to treat the special file as an empty file,
       thus recording the regular file's full contents.  Doing so will
       create extremely large deltas at the point of transition
       between device files and regular files, but this is probably
       very rare anyway.

       There may be ways around this, but I think they are fraught
       with danger. -twp */

    if (preserve_perms && !STREQ (file1, DEVNULL) && !STREQ (file2, DEVNULL))
    {
	struct stat sb1, sb2;

	if (lstat (file1, &sb1) < 0)
	    error (1, errno, "cannot get file information for %s", file1);
	if (lstat (file2, &sb2) < 0)
	    error (1, errno, "cannot get file information for %s", file2);

	if (!S_ISREG (sb1.st_mode) && !S_ISDIR (sb1.st_mode))
	    file1 = DEVNULL;
	if (!S_ISREG (sb2.st_mode) && !S_ISDIR (sb2.st_mode))
	    file2 = DEVNULL;
    }
#endif

    /* The first arg to call_diff_setup is used only for error reporting. */
    call_diff_setup ("diff", dargc, dargv);
    if (label1)
	call_diff_add_arg (label1);
    if (label2)
	call_diff_add_arg (label2);
    call_diff_add_arg ("--");
    call_diff_add_arg (file1);
    call_diff_add_arg (file2);

    return call_diff (out);
}

/* Print the options passed to DIFF, in the format used by rcsdiff.
   The rcsdiff code that produces this output is extremely hairy, and
   it is not clear how rcsdiff decides which options to print and
   which not to print.  The code below reproduces every rcsdiff run
   that I have seen. */

void
RCS_output_diff_options (int diff_argc, char * const *diff_argv,
			 bool devnull, const char *rev1, const char *rev2,
                         const char *workfile)
{
    int i;
    
    cvs_output ("diff", 0);
    for (i = 0; i < diff_argc; i++)
    {
        cvs_output (" ", 1);
	cvs_output (quotearg_style (shell_quoting_style, diff_argv[i]), 0);
    }

    if (devnull)
	cvs_output (" -N", 3);

    if (rev1)
    {
	cvs_output (" -r", 3);
	cvs_output (rev1, 0);
    }

    if (rev2)
    {
	cvs_output (" -r", 3);
	cvs_output (rev2, 0);
    }

    assert (workfile);
    cvs_output (" ", 1);
    cvs_output (workfile, 0);

    cvs_output ("\n", 1);
}
