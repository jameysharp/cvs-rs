/*
 * Copyright (C) 2006 The Free Software Foundation, Inc.
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

#ifndef VERS_TS_H
#define VERS_TS_H

#include "entries.h"
#include "rcs.h"

/*
 * a struct vers_ts contains all the information about a file including the
 * user and rcs file names, and the version checked out and the head.
 *
 * this is usually obtained from a call to Version_TS which takes a
 * tag argument for the RCS file if desired
 */
struct vers_ts
{
    /* rcs version user file derives from, from CVS/Entries.
       It can have the following special values:

       NULL = file is not mentioned in Entries (this is also used for a
	      directory).
       "" = INVALID!  The comment used to say that it meant "no user file"
	    but as far as I know CVS didn't actually use it that way.
	    Note that according to cvs.texinfo, "" is not valid in the
	    Entries file.
       0 = user file is new
       -vers = user file to be removed.  */
    char *vn_user;

    /* Numeric revision number corresponding to ->vn_tag (->vn_tag
       will often be symbolic).  */
    char *vn_rcs;
    /* If ->tag is a simple tag in the RCS file--a tag which really
       exists which is not a magic revision--and if ->date is NULL,
       then this is a copy of ->tag.  Otherwise, it is a copy of
       ->vn_rcs.  */
    char *vn_tag;

    /* This is the timestamp from stating the file in the working directory.
       It is NULL if there is no file in the working directory.  It is
       "Is-modified" if we know the file is modified but don't have its
       contents.  */
    char *ts_user;
    /* Timestamp from CVS/Entries.  For the server, ts_user and ts_rcs
       are computed in a slightly different way, but the fact remains that
       if they are equal the file in the working directory is unmodified
       and if they differ it is modified.  */
    char *ts_rcs;

    /* Options from CVS/Entries (keyword expansion), malloc'd.  If none,
       then it is an empty string (never NULL).  */
    char *options;

    /* If non-NULL, there was a conflict (or merely a merge?  See merge_file)
       and the time stamp in this field is the time stamp of the working
       directory file which was created with the conflict markers in it.
       This is from CVS/Entries.  */
    char *ts_conflict;

    /* Tag specified on the command line, or if none, tag stored in
       CVS/Entries.  */
    char *tag;
    /* Date specified on the command line, or if none, date stored in
       CVS/Entries.  */
    char *date;
    /* If this is 1, then tag is not a branch tag.  If this is 0, then
       tag may or may not be a branch tag.  */
    int nonbranch;

    /* Pointer to entries file node  */
    Entnode *entdata;

    /* Pointer to parsed src file info */
    RCSNode *srcfile;
};
typedef struct vers_ts Vers_TS;

Vers_TS *Version_TS (struct file_info *finfo, const char *options,
		     const char *tag, const char *date, int force_tag_match,
		     int set_time);
void freevers_ts (Vers_TS **versp);
char *time_stamp (const char *file);

#endif /* VERS_TS_H */
