/*
 * Copyright (C) 2006 The Free Software Foundation, Inc.
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

#ifndef IGNORE_H
#define IGNORE_H

#include "hash.h"

int ign_name (char *name);
void ign_add (char *ign, int hold);
void ign_add_file (char *file, int hold);
void ign_setup (void);
void ign_dir_add (char *name);
int ignore_directory (const char *name);
typedef void (*Ignore_proc) (const char *, const char *);
void ignore_files (List *, List *, const char *, Ignore_proc);
extern int ign_inhibit_server;

#endif /* IGNORE_H */
