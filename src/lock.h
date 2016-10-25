/*
 * Copyright (C) 2005-2007 The Free Software Foundation, Inc.
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

#ifndef LOCK_H
#define LOCK_H

int Reader_Lock (const char *xrepository);
void Simple_Lock_Cleanup (void);
void Lock_Cleanup (void);

/* Recursively aquire a promotable read lock for the subtree specified by ARGC,
 * ARGV, LOCAL, and AFLAG.
 */
int lock_tree_promotably (int argc, char **argv, int local, int which,
			  int aflag);

/* See lock.c for description.  */
void lock_dir_for_write (const char *);

/* Get a write lock for the history file.  */
int history_lock (const char *);
void clear_history_lock (void);

/* Get a write lock for the val-tags file.  */
int val_tags_lock (const char *);
void clear_val_tags_lock (void);

#endif /* !defined LOCK_H */
