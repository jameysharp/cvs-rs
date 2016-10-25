/*
 * Copyright (C) 1986-2008 The Free Software Foundation, Inc.
 *
 * Portions Copyright (C) 1998-2008 Derek Price,
 *                                  Ximbiot LLC <http://ximbiot.com>,
 *                                  and others.
 *
 * Portions Copyright (C) 1992, Brian Berliner and Jeff Polk
 * Portions Copyright (C) 1989-1992, Brian Berliner
 * 
 * You may distribute under the terms of the GNU General Public License as
 * specified in the README file that comes with the CVS source distribution.
 * 
 * Find Names
 * 
 * Finds all the pertinent file names, both from the administration and from the
 * repository
 * 
 * Find Dirs
 * 
 * Finds all pertinent sub-directories of the checked out instantiation and the
 * repository (and optionally the attic)
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

/* Verify interface.  */
#include "find-names.h"

/* ANSI C */
#include <assert.h>
#include <glob.h>

/* GNULIB */
#include "quote.h"

/* CVS */
#include "recurse.h"

#include "cvs.h"



/*
 * add the key from entry on entries list to the files list
 */
static int
add_entries_proc (Node *node, void *closure)
{
    Node *fnode;
    List *filelist = closure;
    Entnode *entnode = node->data;

    if (entnode->type != ENT_FILE)
	return 0;

    fnode = getnode ();
    fnode->type = FILES;
    fnode->key = xstrdup (node->key);
    if (addnode (filelist, fnode))
	freenode (fnode);
    return 0;
}



/* walklist() proc which strips a trailing RCSEXT from node keys.
 */
static int
strip_rcsext (Node *p, void *closure)
{
    char *s = p->key + strlen (p->key) - strlen (RCSEXT);
    assert (STREQ (s, RCSEXT));
    *s = '\0'; /* strip the ,v */
    return 0;
}



/*
 * Finds all the ,v files in the directory DIR, and adds them to the LIST.
 * Returns 0 for success and non-zero if DIR cannot be opened, in which case
 * ERRNO is set to indicate the error.  In the error case, LIST is left in some
 * reasonable state (unchanged, or containing the files which were found before
 * the error occurred).
 *
 * INPUTS
 *   dir	The directory to open for read.
 *
 * OUTPUTS
 *   list	Where to store matching file entries.
 *
 * GLOBALS
 *   errno	Set on error.
 *
 * RETURNS
 *   0, for success.
 *   <> 0, on error.
 */
static int
find_rcs (dir, list)
    const char *dir;
    List *list;
{
    List *newlist;
    if (!(newlist = find_files (dir, RCSPAT)))
	return 1;
    walklist (newlist, strip_rcsext, NULL);
    mergelists (list, &newlist);
    return 0;
}



/* Find files in the repository and/or working directory.  On error,
 * may either print a nonfatal error and return NULL, or just give
 * a fatal error.  On success, return non-NULL (even if it is an empty
 * list).
 */
List *
Find_Names (const char *repository, const char *update_dir,
	    int which, int aflag, List *entries)
{
    List *files;

    TRACE (TRACE_FUNCTION, "Find_Names (%s, %s, %d, %d)",
	   repository, update_dir, which, aflag);

    /* make a list for the files */
    files = getlist ();

    /* look at entries (if necessary) */
    if (which & W_LOCAL)
	/* walk the entries file adding elements to the files list */
	walklist (entries, add_entries_proc, files);

    if (which & W_REPOS && repository && !isreadable (CVSADM_ENTSTAT))
    {
	/* search the repository */
	if (find_rcs (repository, files))
	{
	    error (0, errno, "cannot open directory %s",
		   primary_root_inverse_translate (repository));
	    goto error_exit;
	}

	/* search the attic too */
	if (which & W_ATTIC)
	{
	    char *dir = Xasprintf ("%s/%s", repository, CVSATTIC);
	    if (find_rcs (dir, files) != 0
		&& !existence_error (errno))
		/* For now keep this a fatal error, seems less useful
		   for access control than the case above.  */
		error (1, errno, "cannot open directory %s",
		       primary_root_inverse_translate (dir));
	    free (dir);
	}
    }

    /* sort the list into alphabetical order and return it */
    sortlist (files, fsortcmp);
    return files;

 error_exit:
    dellist (&files);
    return NULL;
}



/*
 * Add an entry from the subdirs list to the directories list.  This
 * is called via walklist.
 */
static int
add_subdir_proc (Node *p, void *closure)
{
    List *dirlist = closure;
    Entnode *entnode = p->data;
    Node *dnode;

    if (entnode->type != ENT_SUBDIR)
	return 0;

    dnode = getnode ();
    dnode->type = DIRS;
    dnode->key = xstrdup (entnode->user);
    if (addnode (dirlist, dnode))
	freenode (dnode);
    return 0;
}



/*
 * Register a subdirectory.  This is called via walklist.
 */
/*ARGSUSED*/
static int
register_subdir_proc (Node *p, void *closure)
{
    List *entries = (List *) closure;

    Subdir_Register (entries, NULL, p->key);
    return 0;
}



/*
 * Finds all the subdirectories of the argument dir and adds them to
 * the specified list.  Sub-directories without a CVS administration
 * directory are optionally ignored.  If ENTRIES is not NULL, all
 * files on the list are ignored.  Returns 0 for success or 1 on
 * error, in which case errno is set to indicate the error.
 */
int
find_dirs (const char *dir, List *list, int checkadm, List *entries)
{
    Node *p;
    char *tmp = NULL;
    size_t tmp_size = 0;
    struct dirent *dp;
    DIR *dirp;
    int skip_emptydir = 0;

    TRACE (TRACE_FUNCTION, "find_dirs (%s, %d)", dir, checkadm);

    /* First figure out whether we need to skip directories named
       Emptydir.  Except in the CVSNULLREPOS case, Emptydir is just
       a normal directory name.  */
    if (ISABSOLUTE (dir)
	&& STRNEQ (dir, current_parsed_root->directory,
		   strlen (current_parsed_root->directory))
	&& ISSLASH (dir[strlen (current_parsed_root->directory)])
	&& STREQ (dir + strlen (current_parsed_root->directory) + 1,
		  CVSROOTADM))
	skip_emptydir = 1;

    /* set up to read the dir */
    if (!(dirp = CVS_OPENDIR (dir)))
	return 1;

    /* read the dir, grabbing sub-dirs */
    errno = 0;
    while ((dp = CVS_READDIR (dirp)) != NULL)
    {
	if (STREQ (dp->d_name, ".")
	    || STREQ (dp->d_name, "..")
	    || STREQ (dp->d_name, CVSATTIC)
	    || STREQ (dp->d_name, CVSLCK)
	    || STREQ (dp->d_name, CVSREP))
	    goto do_it_again;

	/* findnode() is going to be significantly faster than stat()
	   because it involves no system calls.  That is why we bother
	   with the entries argument, and why we check this first.  */
	if (entries && findnode (entries, dp->d_name))
	    goto do_it_again;

	if (skip_emptydir
	    && STREQ (dp->d_name, CVSNULLREPOS))
	    goto do_it_again;

	if (!DIRENT_MIGHT_BE_DIR(dp))
	    goto do_it_again;

	/* don't bother stating ,v files */
	if (!CVS_FNMATCH (RCSPAT, dp->d_name, 0))
	    goto do_it_again;

	if (!DIRENT_MUST_BE(dp, DT_DIR))
	{
	    expand_string (&tmp,
			   &tmp_size,
			   strlen (dir) + strlen (dp->d_name) + 10);
	    sprintf (tmp, "%s/%s", dir, dp->d_name);
	    if (!isdir (tmp))
		goto do_it_again;
	}

	/* check for administration directories (if needed) */
	if (checkadm)
	{
	    /* blow off symbolic links to dirs in local dir */
	    if (DIRENT_MUST_BE(dp, DT_LNK)
		|| (DIRENT_MIGHT_BE_SYMLINK(dp) && islink(tmp)))
		goto do_it_again;

	    /* check for new style */
	    expand_string (&tmp,
			   &tmp_size,
			   (strlen (dir) + strlen (dp->d_name)
			    + sizeof (CVSADM) + 10));
	    sprintf (tmp, "%s/%s/%s", dir, dp->d_name, CVSADM);
	    if (!isdir (tmp))
		goto do_it_again;
	}

	/* put it in the list */
	p = getnode ();
	p->type = DIRS;
	p->key = xstrdup (dp->d_name);
	if (addnode (list, p) != 0)
	    freenode (p);

    do_it_again:
	errno = 0;
    }
    if (errno)
    {
	int save_errno = errno;
	CVS_CLOSEDIR (dirp);
	errno = save_errno;
	return 1;
    }
    CVS_CLOSEDIR (dirp);
    if (tmp) free (tmp);
    return 0;
}



/*
 * create a list of directories to traverse from the current directory
 */
List *
Find_Directories (const char *repository, const char *update_dir,
		  int which, List *entries)
{
    List *dirlist;

    TRACE (TRACE_FUNCTION, "Find_Directories (%s, %s, %d)",
	   TRACE_NULL(repository), update_dir, which);

    /* make a list for the directories */
    dirlist = getlist();

    /* find the local ones */
    if (which & W_LOCAL)
    {
	/* If we do have an entries list, then if sdtp is NULL, or if
           sdtp->subdirs is nonzero, all subdirectory information is
           recorded in the entries list.  */
	if (entries && entriesHasAllSubdirs (entries))
	    walklist (entries, add_subdir_proc, dirlist);
	else
	{
	    /* This is an old working directory, in which subdirectory
               information is not recorded in the Entries file.  Find
               the subdirectories the hard way, and, if possible, add
               it to the Entries file for next time.  */

	    /* find_dirs() is appropriate here.  It was originally designed for
	     * use in the repository, so it skips CVSATTIC and CVSLCK
	     * directories, but this is still the right thing to do in a
	     * sandbox since doing otherwise would cause name conflicts when
	     * the directories are imported.
	     *
	     * FIXME: The user should really see a warning about the skipped
	     * directories, however.
	     */
	    if (find_dirs (".", dirlist, 1, entries))
		error (1, errno, "cannot open %s", quote (update_dir));
	    if (entries)
	    {
		if (!list_isempty (dirlist))
		    walklist (dirlist, register_subdir_proc, entries);
		else
		    Subdirs_Known (entries);
	    }
	}
    }

    /* look for sub-dirs in the repository */
    if (which & W_REPOS && repository)
    {
	/* search the repository */
	if (find_dirs (repository, dirlist, 0, entries))
	    error (1, errno, "cannot open directory %s", repository);

	/* We don't need to look in the attic because directories
	   never go in the attic.  In the future, there hopefully will
	   be a better mechanism for detecting whether a directory in
	   the repository is alive or dead; it may or may not involve
	   moving directories to the attic.  */
    }

    /* sort the list into alphabetical order and return it */
    sortlist (dirlist, fsortcmp);

    if (trace >= TRACE_MINUTIA)
    {
	TRACE (TRACE_MINUTIA, "Find_Directories returning dirlist:");
	printlist (dirlist);
    }
    return dirlist;
}



/* Finds all the files matching PAT.  If DIR is NULL, PAT will be interpreted
 * as either absolute or relative to the PWD and read errors, e.g. failure to
 * open a directory, will be ignored.  If DIR is not NULL, PAT is
 * always interpreted as relative to DIR.  Adds all matching files and
 * directories to a new List.  Returns the new List for success and NULL in
 * case of error, in which case ERRNO will also be set.
 *
 * NOTES
 *   When DIR is NULL, this is really just a thinly veiled wrapper for glob().
 *
 *   Much of the cruft in this function could be avoided if DIR was eliminated.
 *
 * INPUTS
 *   dir	The directory to match relative to.
 *   pat	The pattern to match against, via glob().
 *
 * GLOBALS
 *   errno		Set on error.
 *   really_quiet	Used to decide whether to print warnings.
 *
 * RETURNS
 *   A pointer to a List of matching file and directory names, on success.
 *   NULL, on error.
 *
 * ERRORS
 *   Error returns can be caused if glob() returns an error.  ERRNO will be
 *   set.  When !REALLY_QUIET and the failure was not a read error, a warning
 *   message will be printed via error (0, errno, ...).
 */
List *
find_files (const char *dir, const char *pat)
{
    List *retval;
    glob_t glist;
    int err, i;
    char *catpat = NULL;
    bool dirslash = false;

    if (dir && *dir)
    {
	size_t catpatlen = 0;
	const char *p;
	if (glob_pattern_p (dir, false))
	{
	    /* Escape special characters in DIR.  */
	    size_t len = 0;
	    p = dir;
	    while (*p)
	    {
		switch (*p)
		{
		    case '\\':
		    case '*':
		    case '[':
		    case ']':
		    case '?':
			expand_string (&catpat, &catpatlen, len + 1);
			catpat[len++] = '\\';
		    default:
			expand_string (&catpat, &catpatlen, len + 1);
			catpat[len++] = *p++;
			break;
		}
	    }
	    catpat[len] = '\0';
	}
	else
	{
	    xrealloc_and_strcat (&catpat, &catpatlen, dir);
	    p = dir + strlen (dir);
	}

	dirslash = *p - 1 == '/';
	if (!dirslash)
	    xrealloc_and_strcat (&catpat, &catpatlen, "/");

	xrealloc_and_strcat (&catpat, &catpatlen, pat);
	pat = catpat;
    }

    err = glob (pat, GLOB_PERIOD | (dir ? GLOB_ERR : 0), NULL, &glist);
    if (err && err != GLOB_NOMATCH)
    {
	if (err == GLOB_ABORTED)
	{
	    /* Let our caller handle the problem.  */
	    if (catpat) free (catpat);
	    return NULL;
	}
	if (err == GLOB_NOSPACE) errno = ENOMEM;
	if (!really_quiet)
	    error (0, errno, "glob failed");
	if (catpat) free (catpat);
	return NULL;
    }

    /* Copy what glob() returned into a List for our caller.  */
    retval = getlist ();
    for (i = 0; i < glist.gl_pathc; i++)
    {
	Node *p;
	const char *tmp;

	/* Ignore `.' && `..'.  */
	tmp = last_component (glist.gl_pathv[i]);
	if (STREQ (tmp, ".") || STREQ (tmp, ".."))
	    continue;

	p = getnode ();
	p->type = FILES;
	p->key = xstrdup (glist.gl_pathv[i]
			  + (dir ? strlen (dir) + !dirslash : 0));
	if (addnode (retval, p)) freenode (p);
    }

    if (catpat) free (catpat);
    globfree (&glist);
    return retval;
}
