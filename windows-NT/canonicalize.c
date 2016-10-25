/* Return the canonical absolute name of a given file.
   Copyright (C) 1996-2006 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.
   If not, write to the Free Software Foundation,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "canonicalize.h"

#include <errno.h>
#include<windows.h>
#include <shlwapi.h>
#include <xalloc.h>
#include <xgetcwd.h>

#ifndef __set_errno
# define __set_errno(Val) errno = (Val)
#endif


#if !HAVE_CANONICALIZE_FILE_NAME
/* Return the canonical absolute name of file NAME.  A canonical name
   does not contain any `.', `..' components nor any repeated file name
   separators ('\') or symlinks.  All components must exist.
   The result is malloc'd.  */

char *
canonicalize_file_name (const char *name)
{
    return canonicalize_filename_mode (name, CAN_EXISTING);
}
#endif /* !HAVE_CANONICALIZE_FILE_NAME */

/* Return the canonical absolute name of file NAME.  A canonical name
   does not contain any `.', `..' components nor any repeated file name
   separators ('\') or symlinks.  Whether components must exist
   or not depends on canonicalize mode.  The result is malloc'd.  */

char *
canonicalize_filename_mode (const char *name, canonicalize_mode_t can_mode)
{
    char work[MAX_PATH+1];

    char *rname;
    char *last_sep;

    if (name == NULL)
    {
	__set_errno (EINVAL);
	return NULL;
    }

    if (name[0] == '\0')
    {
	__set_errno (ENOENT);
	return NULL;
    }

    /* Does it start with a drive spec? */
    if ( strlen(name) > 1 )
    {
	if (name[0] == '\\' && name[1] == '\\' )
	{
	    /* UNC name - bail out */
	    __set_errno (EINVAL);
	    return NULL;
	}
    }

    /* Windows remembers the current directory for each drive, so if
       the current directory on drive d: is d:\some\path, and
       d:\some\path contains a file named test.txt, then
       'd:test.txt' is equivalent to 'd:\some\path\test.txt'.
       Unfortunately, the shell function PathIsRelative returns false
       if you pass "d:test.txt". So, we have to parse the path ourselves
       to handle the 'd:file' case. */

    /* Did the user specify a drive letter? */
    if ( strlen(name) > 1 && name[1] == ':' )
    {
	/* OK, there's a drive letter - is there a root specifier? */
	if ( name[2] != '\\' )
	{
	    /* No drive specifier - don't even bother trying to figure this 
	       one out. */
	    __set_errno (EINVAL);
	    return NULL;
	}
	/* Easy - the user's provided a full, absolute path. */
	strcpy(work, name);
    } else {
	char * cwd = xgetcwd();
	char * next_char = work;
	if ( name[0] == '\\' )
	{
	    *next_char++=*cwd;
	    *next_char++=':';
	    strcpy(next_char, name);
	} else {
	    strcpy(work, cwd);
	    strcat(work, "\\");
	    strcat(work, name);
	}
	free( cwd );
    }

    rname=xmalloc(MAX_PATH+1);

    if ( !PathCanonicalize(rname, work ) )
	goto error;
    
    last_sep = strrchr(rname, '\\');
    switch( can_mode )
    {
    case CAN_MISSING:
	break;
    case CAN_ALL_BUT_LAST:
	if ( last_sep != NULL )
	{
	    *last_sep = '\0';
	}
	/* Fall through */
    case CAN_EXISTING:
	if ( !PathFileExists(rname) )
	{
	    goto error;
	}
	if (last_sep != NULL)
	    *last_sep = '\\';
	break;
    default:
	goto error;
    }
    return rname;

error:
  free (rname);
  return NULL;
}

#ifdef TEST_CANONICALIZE

/* Built-in low-level unit test.
   To run the test, make sure Visual Studio is in your path,
   issue the command
    cl -D TEST_CANONICALIZE -I..\lib canonicalize.c shlwapi.lib ..\lib\WinDebug\libcvs.lib

   then run canonicalize.exe. Sorry, this isn't automated, you'll have to
   inspect the output to make sure the last path mentioned is correct.
   */

int error;

void do_canon(const char * input, canonicalize_mode_t mode)
{
    char * canonical = canonicalize_filename_mode( input, mode );
    printf("%s, %s\n", input, canonical);
    free(canonical);
}

int main()
{
    char * winroot=getenv("SystemRoot");
    char explorerPath[MAX_PATH+1];
    strcpy(explorerPath, winroot);
    strcat(explorerPath, "\\explorer.exe");

    printf("The output of this program will be\n"
	   "path_before_canonicalization, path_after_canonicalization\n");
    printf("\nThe following should output complete paths\n");
    do_canon("canonicalize.c", CAN_EXISTING);
    do_canon(explorerPath, CAN_EXISTING);
    do_canon(&explorerPath[2], CAN_EXISTING);
    do_canon("c:\\no\\such\\directory", CAN_MISSING);
    do_canon("non_existing_file", CAN_ALL_BUT_LAST);

    printf("\nThe following should output '(null)'\n");
    do_canon("canonicalize.cxx", CAN_EXISTING);
    do_canon("c:\\canonicalize.c", CAN_EXISTING);
    do_canon("c:canonicalize.c", CAN_EXISTING);
    do_canon("\\\\some_server\\some\\path", CAN_MISSING);
    do_canon("non_existing_file", CAN_EXISTING);
    do_canon("nodir\\non_existing_file", CAN_ALL_BUT_LAST);
}
#endif
