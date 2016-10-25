/* This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.  */

/* This file simply performs the include magic necessary for using select */

/* select also requires <sys/time.h>, <sys/types.h>, and <unistd.h> */

/* FIXME: GNULIB doesn't do this.  Is it needed?  */
#ifdef HAVE_SYS_BSDTYPES_H
# include <sys/bsdtypes.h>
#endif

#include <sys/select.h>
