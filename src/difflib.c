/*
 * Copyright (C) 2005 The Free Software Foundation, Inc.
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
#include "difflib.h"

/* Standard headers.  */
#include <stddef.h>

/* GNULIB */
#include "error.h"

/* diffutils */
#include "diffrun.h"

/* CVS */
#include "filesubr.h"
#include "run.h"
#include "server.h"
#include "system.h"

extern int noexec;		/* Don't modify disk anywhere */
extern int really_quiet, quiet;



/* First call call_diff_setup to setup any initial arguments.  The
   argument will be parsed into whitespace separated words and added
   to the global call_diff_argv list.

   Then, optionally, call call_diff_add_arg for each additional argument
   that you'd like to pass to the diff library.

   Finally, call call_diff or call_diff3 to produce the diffs.  */

static char **call_diff_argv;
static int call_diff_argc;
static size_t call_diff_arg_allocated;



/* VARARGS */
void
call_diff_add_arg (const char *s)
{
    TRACE (TRACE_DATA, "call_diff_add_arg (%s)", s);
    run_add_arg_p (&call_diff_argc, &call_diff_arg_allocated, &call_diff_argv,
		   s);
}



void 
call_diff_setup (const char *prog, int argc, char * const *argv)
{
    int i;

    /* clean out any malloc'ed values from call_diff_argv */
    run_arg_free_p (call_diff_argc, call_diff_argv);
    call_diff_argc = 0;

    /* put each word into call_diff_argv, allocating it as we go */
    call_diff_add_arg (prog);
    for (i = 0; i < argc; i++)
	call_diff_add_arg (argv[i]);
}



/* Callback function for the diff library to write data to the output
   file.  This is used when we are producing output to stdout.  */

static void
call_diff_write_output (const char *text, size_t len)
{
    if (len > 0)
	cvs_output (text, len);
}



/* Call back function for the diff library to flush the output file.
   This is used when we are producing output to stdout.  */
static void
call_diff_flush_output (void)
{
    cvs_flushout ();
}



/* Call back function for the diff library to write to stdout.  */
static void
call_diff_write_stdout (const char *text)
{
    cvs_output (text, 0);
}



/* Call back function for the diff library to write to stderr.  */
static void
call_diff_error (const char *format, const char *a1, const char *a2)
{
    /* FIXME: Should we somehow indicate that this error is coming from
       the diff library?  */
    error (0, 0, format, a1, a2);
}



/* This set of callback functions is used if we are sending the diff
   to stdout.  */
static struct diff_callbacks call_diff_stdout_callbacks =
{
    call_diff_write_output,
    call_diff_flush_output,
    call_diff_write_stdout,
    call_diff_error
};



/* This set of callback functions is used if we are sending the diff
   to a file.  */
static struct diff_callbacks call_diff_file_callbacks =
{
    NULL,
    NULL,
    call_diff_write_stdout,
    call_diff_error
};



int
call_diff (const char *out)
{
    call_diff_add_arg (NULL);

    if (out == RUN_TTY)
	return diff_run( call_diff_argc, call_diff_argv, NULL,
			 &call_diff_stdout_callbacks );
    else
	return diff_run( call_diff_argc, call_diff_argv, out,
			 &call_diff_file_callbacks );
}



int
call_diff3 (char *out)
{
    if (out == RUN_TTY)
	return diff3_run (call_diff_argc, call_diff_argv, NULL,
			  &call_diff_stdout_callbacks);
    else
	return diff3_run (call_diff_argc, call_diff_argv, out,
			  &call_diff_file_callbacks);
}



/* Merge the changes between files J1 & J2 into file DEST.  Mark portions from
 * particular files using strings REV1 & REV2.
 */
int
merge (const char *dest, const char *dlabel, const char *j1,
       const char *j1label, const char *j2, const char *j2label)
{
    char *diffout;
    int retval;

    /* Remember that the first word in the `call_diff_setup' string is used
       now only for diagnostic messages -- CVS no longer forks to run
       diff3. */
    diffout = cvs_temp_name();
    call_diff_setup ("diff3", 0, NULL);
    call_diff_add_arg ("-E");
    call_diff_add_arg ("-am");

    call_diff_add_arg ("-L");
    call_diff_add_arg (dlabel);
    call_diff_add_arg ("-L");
    call_diff_add_arg (j1label);
    call_diff_add_arg ("-L");
    call_diff_add_arg (j2label);

    call_diff_add_arg ("--");
    call_diff_add_arg (dest);
    call_diff_add_arg (j1);
    call_diff_add_arg (j2);

    retval = call_diff3 (diffout);

    if (retval == 1 && !really_quiet)
	error (0, 0, "conflicts during merge");
    else if (retval == 2)
	error (1, 0, "diff3 failed.");

    copy_file (diffout, dest);

    /* Clean up. */
    if (CVS_UNLINK (diffout) < 0 && !existence_error (errno))
	error (0, errno, "cannot remove temp file `%s'", diffout);
    free (diffout);

    return retval;
}
