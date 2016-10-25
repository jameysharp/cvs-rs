/* gpg.h - OpenPGP functions header.
 * Copyright (C) 2005-2006 Free Software Foundation, Inc.
 *
 * This file is part of of CVS.
 *
 * GnuPG is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * GnuPG is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
 * USA.
 */

#ifndef GPG_H
#define GPG_H

/* ANSI C Headers.  */
#include <stdint.h>

/* CVS Headers.  */
#include "buffer.h"
#include "subr.h"



struct openpgp_signature
{
  time_t ctime;
  uint64_t keyid;
  uint8_t *raw;
  size_t rawlen;
};



int next_signature (struct buffer *bpin, struct buffer *bpout);
int parse_signature (struct buffer *bpin, struct openpgp_signature *spout);

void set_openpgp_textmode (const char *textmode);
const char *get_openpgp_textmode (void);

# define gpg_keyid2string(k) \
	 Xasprintf ("0x%lx", (unsigned long)((k) & 0xFFFFFFFF))

# ifdef HAVE_LONG_LONG
#   define gpg_keyid2longstring(k) \
	   Xasprintf ("0x%llx", (unsigned long long)(k))
# else
char *gpg_keyid2longstring (uint64_t keyid);
# endif
#endif /* GPG_H */
