/* Provide a working getlogin_r for systems which lack it.

   Copyright (C) 2005, 2006, 2007 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */

/* written by Paul Eggert and Derek Price */

#include <config.h>

/* Specification.  */
#include <unistd.h>

#include <errno.h>
#include <string.h>

#if !HAVE_DECL_GETLOGIN
char *getlogin (void);
#endif

/* See unistd.in.h for documentation.  */
int
getlogin_r (char *name, size_t size)
{
  char *n;
  size_t nlen;

  errno = 0;
  n = getlogin ();

  /* A system function like getlogin_r is never supposed to set errno
     to zero, so make sure errno is nonzero here.  ENOENT is a
     reasonable errno value if getlogin returns NULL.  */
  if (!errno)
    errno = ENOENT;

  if (!n)
    return errno;
  nlen = strlen (n);
  if (size <= nlen)
    return ERANGE;
  memcpy (name, n, nlen + 1);
  return 0;
}
