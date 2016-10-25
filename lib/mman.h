/* Copyright (C) 2008 The Free Software Foundation, Inc.
 *
 * Portions Copyright (C) 2008 Derek Price
 *                             and Ximbiot LLC <http://ximbiot.com>.
 *
 * Abstraction for the sys/mman.h header.  Don't include this unless you've
 * checked HAVE_MMAP.
 *
 * Probably, this should be moved into GNULIB.  The exact same cruft below is
 * used in GNULIB's pagealign_alloc.c.
 */
#ifndef MMAN_H
#define MMAN_H

#include <sys/mman.h>

/* Define MAP_FILE when it isn't otherwise.  */
#ifndef MAP_FILE
# define MAP_FILE 0
#endif
/* Define MAP_FAILED for old systems which neglect to.  */
#ifndef MAP_FAILED
# define MAP_FAILED ((void *)-1)
#endif

#endif /* MMAN_H double-inclusion guard.  */
