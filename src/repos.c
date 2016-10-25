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
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

/* Verify interface.  */
#include "repos.h"

/* GNULIB */
#include "quote.h"

/* CVS */
#include "cvs.h"



/* Determine the name of the RCS repository for directory DIR in the
 * current working directory, or for the current working directory
 * itself if DIR is NULL.  Returns the name in a newly-malloc'd
 * string.  On error, gives a fatal error and does not return.
 * UPDATE_DIR is the path from where cvs was invoked (for use in error
 * messages), and should contain DIR as its last component.
 * UPDATE_DIR can be NULL to signify the directory in which cvs was
 * invoked.
 */
char *
Name_Repository (const char *dir, const char *update_dir)
{
    FILE *fpin;
    char *repos = NULL;
    size_t repos_allocated = 0;
    char *tmp;
    char *cp;

    TRACE (TRACE_FUNCTION, "Name_Repository (%s, %s)",
	   TRACE_NULL (dir), update_dir);

    tmp = dir_append (dir, CVSADM_REP);  /* dir may be NULL */

    /* The assumption here is that the repository is always contained in the
     * first line of the "Repository" file.
     */
    fpin = CVS_FOPEN (tmp, "r");
    if (!fpin)
    {
	int save_errno = errno;
	char *cvsadm;
	char *file = dir_append (update_dir, CVSADM);

	cvsadm = dir_append (dir, CVSADM);
	if (!isdir (cvsadm))
	{
	    error (1, 0, "admin directory %s is missing; do %s first",
		   quote_n (0, file),
		   quote_n (1, Xasprintf ("%s checkout", program_name)));
	    /* NOT REACHED */
	}

	if (existence_error (save_errno))
	{
	    /* This occurs at least in the case where the user manually
	     * creates a directory named CVS.
	     */
	    error (0, 0,
		   "admin directory %s found without administrative files.",
		   quote (file));
	    error (0, 0, "Use CVS to create the CVS directory, or rename the");
	    error (0, 0, "directory if it is intended to store something");
	    error (0, 0, "besides CVS administrative files.");
	    error (1, 0, "*PANIC* administration files missing!");
	    /* NOT REACHED */
	}

	error (1, save_errno, "cannot open %s", quote (file));
	/* NOT REACHED */
    }

    if (getline (&repos, &repos_allocated, fpin) < 0)
    {
	char *file = dir_append (update_dir, CVSADM_REP);
	if (feof (fpin))
	    error (1, 0, "unexpected EOF reading %s", quote (file));
	else
	    error (1, errno, "cannot read %s", quote (file));
	/* NOT REACHED */
    }
    if (fclose (fpin) < 0)
	error (0, errno, "cannot close %s", quote (tmp));
    free (tmp);

    if ((cp = strrchr (repos, '\n')) != NULL)
	*cp = '\0';			/* strip the newline */

    /* If this is a relative repository pathname, turn it into an absolute
     * one by tacking on the current root.  There is no need to grab it from
     * the CVS/Root file via the Name_Root() function because by the time
     * this function is called, the contents of CVS/Root have already been
     * compared to original_root and found to match.
     */
    if (!ISABSOLUTE (repos))
    {
	char *newrepos;
	assert (current_parsed_root);
	if (pathname_levels (repos) > 0)
	    error (1, 0,
"unsupported %s-relative repository specification found in %s",
		   quote_n (0, ".."),
		   quote_n (1, dir_append (update_dir, CVSADM_REP)));

	newrepos = dir_append (original_parsed_root->directory, repos);
	free (repos);
	repos = newrepos;
    }

    Sanitize_Repository_Name (repos);

    TRACE (TRACE_DATA, "Name_Repository returning %s", repos);
    return repos;
}



/* Return a pointer to the repository name relative to CVSROOT from a
 * possibly fully qualified repository
 *
 * Given NULL, return NULL.
 */
const char *
Short_Repository (const char *repository)
{
    const char *rep;

    if (!repository || !ISABSOLUTE (repository))
	return repository;

    /* Strip off a leading CVSroot from the beginning of repository.  */
    assert (STRNEQ (original_parsed_root->directory, repository,
		    strlen (original_parsed_root->directory)));

    rep = repository + strlen (original_parsed_root->directory);
    assert (!*rep || ISSLASH (*rep));

    /* Skip leading slashes in rep, in case CVSroot ended with a slash.  */
    while (ISSLASH (*rep)) rep++;

    return rep;
}



/* Sanitize the repository name (in place) by removing trailing
 * slashes and a trailing "." if present.  It should be safe for
 * callers to use strcat and friends to create repository names.
 * Without this check, names like "/path/to/repos/./foo" and
 * "/path/to/repos//foo" would be created.  For example, one
 * significant case is the CVSROOT-detection code in commit.c.  It
 * decides whether or not it needs to rebuild the administrative file
 * database by doing a string compare.  If we've done a `cvs co .' to
 * get the CVSROOT files, "/path/to/repos/./CVSROOT" and
 * "/path/to/repos/CVSROOT" are the arguments that are compared!
 *
 * This function ends up being called from the same places as
 * strip_path, though what it does is much more conservative.  Many
 * comments about this operation (which was scattered around in
 * several places in the source code) ran thus:
 *
 *    ``repository ends with "/."; omit it.  This sort of thing used
 *    to be taken care of by strip_path.  Now we try to be more
 *    selective.  I suspect that it would be even better to push it
 *    back further someday, so that the trailing "/." doesn't get into
 *    repository in the first place, but we haven't taken things that
 *    far yet.''        --Jim Kingdon (recurse.c, 07-Sep-97)
 */

void
Sanitize_Repository_Name (char *repository)
{
    size_t len;

    assert (repository);

    strip_trailing_slashes (repository);
    len = strlen (repository);
    while (len >= 2
	   && repository[len - 1] == '.'
	   && ISSLASH (repository[len - 2]))
    {
	/* Beware the case where the string is exactly "/." or "//.".
	 * Paths with a leading "//" are special on some early UNIXes.
	 */
	if (strlen (repository) == 2
	    || (strlen (repository) == 3 && ISSLASH (*repository)))
	    repository[strlen (repository) - 1] = '\0';
	else
	    repository[strlen (repository) - 2] = '\0';
	strip_trailing_slashes (repository);
	len = strlen (repository);
    }
}
