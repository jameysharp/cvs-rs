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

#ifndef FILESUBR_H
#define FILESUBR_H

#include <stdbool.h>
#include <stdio.h>      /* Get FILE. */
#include <sys/types.h>  /* Get ssize_t.  */



bool isdir (const char *file);
bool isfile (const char *file);
ssize_t islink (const char *file);
bool isdevice (const char *file);
bool isreadable (const char *file);
bool iswritable (const char *file);
bool isaccessible (const char *file, const int mode);
char *get_homedir (void);
char *strcat_filename_onto_homedir (const char *, const char *);
char *cvs_temp_name (void);
FILE *cvs_temp_file (char **filename);
int unlink_file (const char *f);
int unlink_file_dir (const char *f);
void copy_file (const char *from, const char *to);
void force_copy_file (const char *from, const char *to);
void xchmod (const char *fname, bool writable);
void force_xchmod (const char *fname, bool writable);
int xcmp (const char *file1, const char *file2);

#endif /* FILESUBR_H */
