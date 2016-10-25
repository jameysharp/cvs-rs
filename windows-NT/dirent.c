/* dirent.c - portable directory routines
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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

/* Validate API.  */
#include <dirent.h>

/* System (WOE32) Includes.  */
#include <dos.h>
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

/* GNULIB Includes.  */
#include "xalloc.h"



DIR *
opendir (const char *name)
{
  struct _finddata_t find_buf;
  DIR *dirp;
  struct _dircontents *dp;
  char name_buf[_MAX_PATH + 1];
  char *slash = "";
  long hFile;

  if (!name)
    name = "";
  else if (*name)
    {
      const char *s;
      int l = strlen (name);

      s = name + l - 1;
      if ( !(l == 2 && *s == ':') && *s != '\\' && *s != '/')
	slash = "/";	/* save to insert slash between path and "*.*" */
    }

  sprintf (name_buf, "%s%s%s", name, slash, "*.*");

  dirp = xmalloc (sizeof (DIR));
  dirp->dd_loc = 0;
  dirp->dd_contents = dirp->dd_cp = NULL;

  if ((hFile = _findfirst (name_buf, &find_buf)) < 0)
    {
      free (dirp);
      return NULL;
    }

  do
    {
      dp = xmalloc (sizeof (struct _dircontents));
      dp->_d_length = strlen (find_buf.name);
      dp->_d_entry = xmalloc (dp->_d_length + 1);
      memcpy (dp->_d_entry, find_buf.name, dp->_d_length + 1);

      if (dirp->dd_contents)
	dirp->dd_cp = dirp->dd_cp->_d_next = dp;
      else
	dirp->dd_contents = dirp->dd_cp = dp;

      dp->_d_next = NULL;

    } while (!_findnext (hFile, &find_buf));

  dirp->dd_cp = dirp->dd_contents;

  _findclose(hFile);

  return dirp;
}



/* Garbage collection */
static void
free_dircontents (struct _dircontents *dp)
{
  struct _dircontents *odp;

  while (dp)
    {
      if (dp->_d_entry)
	free (dp->_d_entry);
      dp = (odp = dp)->_d_next;
      free (odp);
    }
}



int
closedir (DIR *dirp)
{
  free_dircontents (dirp->dd_contents);
  free (dirp);
  return 0;
}



int
readdir_r (DIR *dirp, struct dirent * restrict dp,
	   struct dirent ** restrict result)
{
  if (!dirp->dd_cp)
    *result = NULL;
  else
    {
      dp->d_namlen = dirp->dd_cp->_d_length;
      memcpy (dp->d_name, dirp->dd_cp->_d_entry, dp->d_namlen + 1);

      dirp->dd_cp = dirp->dd_cp->_d_next;
      dirp->dd_loc++;
      *result = dp;
    }

  return 0;
}



struct dirent *
readdir (DIR *dirp)
{
  static struct dirent dp;
  static struct dirent *retval;
  int err = readdir_r (dirp, &dp, &retval);
  if (err)
    {
      errno = err;
      retval = NULL;
    }
  return retval;
}



void
seekdir (DIR *dirp, long off)
{
  long i = off;
  struct _dircontents *dp;

  if (off < 0)
    return;
  for (dp = dirp->dd_contents; --i >= 0 && dp; dp = dp->_d_next)
    ;
  dirp->dd_loc = off - (i + 1);
  dirp->dd_cp = dp;
}



long
telldir (DIR *dirp)
{
  return dirp->dd_loc;
}



#ifdef TEST
void
main (int argc, char *argv[])
{
  static DIR *directory;
  struct dirent *entry = NULL;

  char *name = "";

  if (argc > 1)
    name = argv[1];

  directory = opendir (name);

  if (!directory)
    {
      fprintf (stderr, "can't open directory `%s'.\n", name);
      exit (2);
    }

  while (entry = readdir (directory))
    printf ("> %s\n", entry->d_name);

  printf ("done.\n");
}
#endif /* TEST */
