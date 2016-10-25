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

#ifndef VERIFY_H
#define VERIFY_H

#include <stdbool.h>
#include <stddef.h>

/* Get List.  */
#include "hash.h"



typedef enum
{
  VERIFY_DEFAULT = 0,
  VERIFY_OFF,
  VERIFY_WARN,
  VERIFY_FATAL
} verify_state;



/* Set values to override current_parsed_root.  */
void set_verify_checkouts (verify_state verify);
void set_verify_template (const char *template);
void set_verify_textmode (const char *textmode);
void add_verify_arg (const char *arg);

/* Get values.  */
bool get_verify_checkouts (bool server_support);
bool get_verify_checkouts_fatal (void);
bool get_verify_commits (void);
bool get_verify_commits_fatal (void);
bool verify_signature (const char *srepos, const char *sig, size_t siglen,
		       const char *filename, bool bin, bool fatal);

/* User command.  */
int verify (int argc, char **argv);
#endif /* VERIFY_H */
