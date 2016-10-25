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
 * Check In
 * 
 * Does a very careful checkin of the file "user", and tries not to spoil its
 * modification time (to avoid needless recompilations). When RCS ID keywords
 * get expanded on checkout, however, the modification time is updated and
 * there is no good way to get around this.
 * 
 * Returns non-zero on error.
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

/* CVS */
#include "base.h"
#include "edit.h"
#include "fileattr.h"
#include "wrapper.h"

#include "cvs.h"



int
Checkin (int type, struct file_info *finfo, char *rev, char *tag,
	 char *options, char *message)
{
    Vers_TS *vers, *pvers;
    int set_time;
    char *tocvsPath = NULL;

    tocvsPath = wrap_tocvs_process_file (finfo->file);
    if (!noexec)
    {
        if (tocvsPath)
	{
	    if (unlink_file_dir (finfo->file) < 0)
		if (! existence_error (errno))
		    error (1, errno, "cannot remove %s", finfo->fullname);
	    rename_file (tocvsPath, finfo->file);
	}
    }

    /* There use to be a check for finfo->rcs == NULL here and then a
     * call to RCS_parse when necessary, but Checkin() isn't called
     * if the RCS file hasn't already been parsed in one of the
     * check functions.
     */
    assert (finfo->rcs != NULL);

    pvers = Version_TS (finfo, NULL, tag, NULL, 1, 0);
    switch (RCS_checkin (finfo->rcs, finfo->update_dir, finfo->file, message,
			 rev, 0, RCS_FLAGS_KEEPFILE))
    {
	case 0:			/* everything normal */

	    /* The checkin succeeded.  If checking the file out again
               would not cause any changes, we are done.  Otherwise,
               we need to check out the file, which will change the
               modification time of the file.

	       The only way checking out the file could cause any
	       changes is if the file contains RCS keywords.  So we if
	       we are not expanding RCS keywords, we are done.  */

	    if (options && STREQ (options, "-V4")) /* upgrade to V5 now */
		options[0] = '\0';

	    /* FIXME: If PreservePermissions is on, RCS_cmp_file is
               going to call RCS_checkout into a temporary file
               anyhow.  In that case, it would be more efficient to
               call RCS_checkout here, compare the resulting files
               using xcmp, and rename if necessary.  I think this
               should be fixed in RCS_cmp_file.  */
	    if ((1
#ifdef PRESERVE_PERMISSIONS_SUPPORT
		 !config->preserve_perms
#endif /* PRESERVE_PERMISSIONS_SUPPORT */
		 && options
		 && (STREQ (options, "-ko") || STREQ (options, "-kb")))
		|| !RCS_cmp_file (finfo->rcs, pvers->tag, rev, NULL, NULL,
	                          options, finfo->file))
		/* The existing file is correct.  We don't have to do
                   anything.  */
		set_time = 0;
	    else
		set_time = 1;

	    vers = Version_TS (finfo, NULL, tag, NULL, 1, set_time);

	    if (set_time)
	    {
		/* The existing file is incorrect.  We need to check
                   out the correct file contents.  */
		if (base_checkout (finfo->rcs, finfo, pvers->vn_user,
				   vers->vn_rcs, pvers->entdata->tag,
				   vers->tag, pvers->entdata->options,
				   options))
		    error (1, 0, "failed when checking out new copy of %s",
			   finfo->fullname);
		base_copy (finfo, vers->vn_rcs,
			   cvswrite && !fileattr_get (finfo->file, "_watched")
			   ? "yy" : "yn");
		set_time = 1;
	    }
	    else if (!suppress_bases)
	    {
		/* Still need to update the base file.  */
		char *basefile;
		cvs_xmkdir (CVSADM_BASE, NULL, MD_EXIST_OK);
		basefile = make_base_file_name (finfo->file, vers->vn_rcs);
		copy_file (finfo->file, basefile);
		free (basefile);
	    }
	    /* Remove the previous base file, in local mode.  */
	    base_remove (finfo->file, pvers->vn_user);
	    freevers_ts (&pvers);

	    wrap_fromcvs_process_file (finfo->file);

	    /*
	     * If we want read-only files, muck the permissions here, before
	     * getting the file time-stamp.
	     */
	    if (!cvswrite || fileattr_get (finfo->file, "_watched"))
		xchmod (finfo->file, 0);

	    /* Re-register with the new data.  */
	    if (STREQ (vers->options, "-V4"))
		vers->options[0] = '\0';
	    Register (finfo, vers->vn_rcs, vers->ts_user,
		      vers->options, vers->tag, vers->date, NULL);
	    history_write (type, NULL, vers->vn_rcs,
			   finfo->file, finfo->repository);

	    if (tocvsPath)
		if (unlink_file_dir (tocvsPath) < 0)
		    error (0, errno, "cannot remove %s", tocvsPath);

	    break;

	case -1:			/* fork failed */
	    if (tocvsPath)
		if (unlink_file_dir (tocvsPath) < 0)
		    error (0, errno, "cannot remove %s", tocvsPath);

	    if (!noexec)
		error (1, errno, "could not check in %s -- fork failed",
		       finfo->fullname);
	    return (1);

	default:			/* ci failed */

	    /* The checkin failed, for some unknown reason, so we
	       print an error, and return an error.  We assume that
	       the original file has not been touched.  */
	    if (tocvsPath)
		if (unlink_file_dir (tocvsPath) < 0)
		    error (0, errno, "cannot remove %s", tocvsPath);

	    if (!noexec)
		error (0, 0, "could not check in %s", finfo->fullname);
	    return (1);
    }

    /*
     * When checking in a specific revision, we may have locked the wrong
     * branch, so to be sure, we do an extra unlock here before
     * returning.
     */
    if (rev)
    {
	(void) RCS_unlock (finfo->rcs, NULL, 1);
	RCS_rewrite (finfo->rcs, NULL, NULL);
    }

#ifdef SERVER_SUPPORT
    if (server_active)
    {
	if (set_time)
	    /* Need to update the checked out file on the client side.  */
	    server_updated (finfo, vers, SERVER_UPDATED,
			    (mode_t) -1, NULL, NULL);
	else
	    server_checked_in (finfo->file, finfo->update_dir,
			       finfo->repository);
    }
    else
#endif
	mark_up_to_date (finfo->update_dir, finfo->file);

    freevers_ts (&vers);
    return 0;
}
