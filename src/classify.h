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

#ifndef CLASSIFY_H
#define CLASSIFY_H

/* CVS Headers.  */
#include "rcs.h"
#include "vers_ts.h"



/*
 * defines for Classify_File() to determine the current state of a file.
 * These are also used as types in the data field for the list we make for
 * Update_Logfile in commit, import, and add.
 */
enum classify_type
{
    T_UNKNOWN = 1,			/* no old-style analog existed	 */
    T_CONFLICT,				/* C (conflict) list		 */
    T_NEEDS_MERGE,			/* G (needs merging) list	 */
    T_MODIFIED,				/* M (needs checked in) list 	 */
    T_CHECKOUT,				/* O (needs checkout) list	 */
    T_ADDED,				/* A (added file) list		 */
    T_REMOVED,				/* R (removed file) list	 */
    T_REMOVE_ENTRY,			/* W (removed entry) list	 */
    T_UPTODATE,				/* File is up-to-date		 */
    T_PATCH,				/* P Like C, but can patch	 */
    T_TITLE				/* title for node type 		 */
};
typedef enum classify_type Ctype;

Ctype Classify_File (struct file_info *finfo, char *tag, char *date,
		     char *options, int force_tag_match, int aflag,
		     Vers_TS **versp, int pipeout);

#endif /* CLASSIFY_H */
