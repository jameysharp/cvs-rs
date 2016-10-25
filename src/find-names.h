/*
 * Copyright (C) 2007 The Free Software Foundation, Inc.
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

#ifndef FIND_NAMES_H
#define FIND_NAMES_H

/* CVS */
#include "hash.h"

List *Find_Directories (const char *repository, const char *update_dir,
			int which, List *entries);
List *Find_Names (const char *repository, const char *update_dir,
		  int which, int aflag, List *entries);
int find_dirs (const char *dir, List *list, int checkadm, List *entries);

#endif /* !defined FIND_NAMES_H */
