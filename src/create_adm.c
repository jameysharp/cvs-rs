/*
 * Copyright (C) 1986-2007 The Free Software Foundation, Inc.
 *
 * Portions Copyright (C) 1998-2007 Derek Price,
 *                                  Ximbiot LLC <http://ximbiot.com>,
 *                                  and others.
 *
 * Portions Copyright (C) 1992, Brian Berliner and Jeff Polk
 * Portions Copyright (C) 1989-1992, Brian Berliner
 * 
 * You may distribute under the terms of the GNU General Public License as
 * specified in the README file that comes with the CVS source distribution.
 * 
 * Create Administration.
 * 
 * Creates a CVS administration directory based on the argument repository; the
 * "Entries" file is prefilled from the "initrecord" argument.
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

/* GNULIB */
#include "quote.h"

/* CVS */
#include "repos.h"

#include "cvs.h"



/* update_dir includes dir as its last component.

   Return value is 0 for success, or 1 if we printed a warning message.
   Note that many errors are still fatal; particularly for unlikely errors
   a fatal error is probably better than a warning which might be missed
   or after which CVS might do something non-useful.  If WARN is zero, then
   don't print warnings; all errors are fatal then.  */

int
Create_Admin (const char *dir, const char *update_dir, const char *repository,
              const char *tag, const char *date, int nonbranch, int warn,
              int dotemplate)
{
    FILE *fout;
    char *reposcopy;
    const char *short_repos;
    char *tmp, *ud;

    TRACE (TRACE_FUNCTION, "Create_Admin (%s, %s, %s, %s, %s, %d, %d, %d)",
	   dir, update_dir, repository, TRACE_NULL (tag),
	   TRACE_NULL (date), nonbranch, warn, dotemplate);

    if (noexec)
	return 0;

    /* A leading "./" looks bad in error messages.  */
    tmp = dir_append (dir, CVSADM);
    if (isfile (tmp))
	error (1, 0, "there is a version in %s already",
	       quote (NULL2DOT (update_dir)));

    ud = dir_append (update_dir, CVSADM);
    if (!cvs_mkdir (tmp, ud, warn ? 0 : MD_FATAL))
    {
	free (tmp);
	free (ud);
	return 1;
    }
    /* else */

    free (tmp);
    free (ud);

    /* record the current cvs root for later use */

    Create_Root (dir, original_parsed_root->original);

    tmp = dir_append (dir, CVSADM_REP);
    fout = CVS_FOPEN (tmp, "w+");
    if (!fout)
	error (1, errno, "cannot open %s",
	       quote (dir_append (update_dir, CVSADM_REP)));

    reposcopy = xstrdup (repository);
    Sanitize_Repository_Name (reposcopy);

    /* The Repository file is to hold a relative path, so strip off any
     * leading CVSroot argument.
     */
    short_repos = Short_Repository (reposcopy);

    if (fprintf (fout, "%s\n", NULL2DOT (short_repos)) < 0)
	error (1, errno, "write to %s failed",
	       quote (dir_append (update_dir, CVSADM_REP)));
    if (fclose (fout) == EOF)
	error (1, errno, "cannot close %s",
	       quote (dir_append (update_dir, CVSADM_REP)));

    /* now, do the Entries file */
    free (tmp);
    tmp = dir_append (dir, CVSADM_ENT);
    fout = CVS_FOPEN (tmp, "w+");
    if (!fout)
	error (1, errno, "cannot open %s",
	       quote (dir_append (update_dir, CVSADM_ENT)));
    if (fclose (fout) == EOF)
	    error (1, errno, "cannot close %s",
		   quote (dir_append (update_dir, CVSADM_ENT)));

    /* Create a new CVS/Tag file */
    WriteTag (dir, tag, date, nonbranch, update_dir, repository);

    TRACE (TRACE_FUNCTION, "Create_Admin");

    free (reposcopy);
    free (tmp);
    return 0;
}
