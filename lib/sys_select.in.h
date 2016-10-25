/* Substitute for <sys/select.h>.
   Copyright (C) 2007-2008 Free Software Foundation, Inc.

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

#ifndef _GL_SYS_SELECT_H

#if @HAVE_SYS_SELECT_H@

@PRAGMA_SYSTEM_HEADER@

/* On many platforms, <sys/select.h> assumes prior inclusion of
   <sys/types.h>.  */
# include <sys/types.h>

/* The include_next requires a split double-inclusion guard.  */
# @INCLUDE_NEXT@ @NEXT_SYS_SELECT_H@

#endif

#ifndef _GL_SYS_SELECT_H
#define _GL_SYS_SELECT_H

#if !@HAVE_SYS_SELECT_H@

/* A platform that lacks <sys/select.h>.  */

# include <sys/socket.h>

#endif

#endif /* _GL_SYS_SELECT_H */
#endif /* _GL_SYS_SELECT_H */
