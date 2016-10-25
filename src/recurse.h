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

#ifndef RECURSE_H
#define RECURSE_H

#include "rcs.h"	/* Get struct file_info.  */

/* Flags for find_{names,dirs} routines */
#define W_LOCAL		(1 << 0)	/* look for files locally */
#define W_REPOS		(1 << 1)	/* look for files in the repository */
#define W_ATTIC		(1 << 2)	/* look for files in the attic */

/* Flags for return values of direnter procs for the recursion processor */
enum direnter_type
{
    R_PROCESS = 1,			/* process files and maybe dirs */
    R_SKIP_FILES,			/* don't process files in this dir */
    R_SKIP_DIRS,			/* don't process sub-dirs */
    R_SKIP_ALL				/* don't process files or dirs */
};
#ifdef ENUMS_CAN_BE_TROUBLE
typedef int Dtype;
#else
typedef enum direnter_type Dtype;
#endif

/* Recursion processor lock types */
enum cvs_lock_type
{
    CVS_LOCK_NONE,
    CVS_LOCK_READ,
    CVS_LOCK_WRITE
};

/* Callback functions.  */
typedef	int (*FILEPROC) (void *callerdat, struct file_info *finfo);
typedef	int (*FILESDONEPROC) (void *callerdat, int err,
                              const char *repository, const char *update_dir,
                              List *entries);
typedef	Dtype (*DIRENTPROC) (void *callerdat, const char *dir,
                             const char *repos, const char *update_dir,
                             List *entries);
typedef	int (*DIRLEAVEPROC) (void *callerdat, const char *dir, int err,
                             const char *update_dir, List *entries);

int start_recursion (FILEPROC fileproc, FILESDONEPROC filesdoneproc,
		     DIRENTPROC direntproc, DIRLEAVEPROC dirleaveproc,
		     void *callerdat, int argc, char *argv[], int local,
		     int which, int aflag, enum cvs_lock_type locktype,
		     const char *update_preload, int dosrcs,
		     const char *repository);

#endif /* RECURSE_H */
