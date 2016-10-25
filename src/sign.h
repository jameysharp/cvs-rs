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

#ifndef SIGN_H
#define SIGN_H

#include <stdbool.h>
#include <stddef.h>

/* Get List.  */
#include "hash.h"



typedef enum { SIGN_DEFAULT, SIGN_ALWAYS, SIGN_NEVER } sign_state;



/* Set values to override current_parsed_root.  */
void set_sign_commits (sign_state sign);
void set_sign_template (const char *template);
void add_sign_arg (const char *arg);

/* Get values.  */
bool get_sign_commits (bool server_support);
char *gen_signature (const char *srepos, const char *filename, bool bin,
		     size_t *len);
char *get_signature (const char *srepos, const char *filename, bool bin,
		     size_t *len);

/* Other utilities.  */
bool have_sigfile (const char *fn);
char *get_sigfile_name (const char *fn);

/* Sign command.  */
int sign (int argc, char **argv);

#endif /* SIGN_H */
