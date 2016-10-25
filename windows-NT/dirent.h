/* dirent.h - portable directory routines
   Copyright (C) 2006 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */

/* Everything non trivial in this code came originally from: @(#)msd_dir.c 1.4
   87/11/06, a public domain implementation of BSD directory routines for
   MS-DOS, written by Michael Rendell ({uunet,utai}michael@garfield),
   August 1987.

   Converted to CVS's "windows-NT/ndir.c" in 1990 by Thorsten Ohl
   <td12@ddagsi3.bitnet>.

   Minor adaptations made in 2006 by Derek R. Price <derek@ximbiot.com>, with
   Windows API oversight by Jim Hyslop <jhyslop@dreampossible.ca>, to meet the
   POSIX.1 <dirent.h> API with some GNU extensions (as opposed to its
   intermediate incarnation as CVS's "windows-NT/ndir.c").  */

#ifndef WINDOWSNT_DIRENT_H_INCLUDED
#define WINDOWSNT_DIRENT_H_INCLUDED

#include <stddef.h>

#define	rewinddir(dirp)	seekdir (dirp, 0L)

/* 255 is said to be big enough for Windows NT.  The more elegant
 * solution would be declaring d_name as one byte long and allocating
 * it to the actual size needed.
 */
#ifndef NAME_MAX
# define NAME_MAX 255
#endif
struct dirent
{
  size_t d_namlen;		/* GNU extension.  */
  char d_name[NAME_MAX + 1];	/* guarantee null termination */
};

struct _dircontents
{
  size_t _d_length;
  char *_d_entry;
  struct _dircontents *_d_next;
};

typedef struct _dirdesc
{
  int dd_id;			/* uniquely identify each open directory */
  long dd_loc;			/* where we are in directory entry is this */
  struct _dircontents *dd_contents;	/* pointer to contents of dir */
  struct _dircontents *dd_cp;	/* pointer to current position */
} DIR;

int closedir (DIR *);
DIR *opendir (const char *);
struct dirent *readdir (DIR *);
int readdir_r (DIR *, struct dirent *restrict, struct dirent **restrict);
void seekdir (DIR *, long);
long telldir (DIR *);

/* GNU extension.  */
#define _D_EXACT_NAMLEN(dp) (dp)->d_namlen

#endif /* WINDOWSNT_DIRENT_H_INCLUDED */
