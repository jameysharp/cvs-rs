/*
 * Copyright (C) 2005 The Free Software Foundation, Inc.
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
#ifndef DIFFLIB_H
#define DIFFLIB_H

int call_diff (const char *out);
int call_diff3 (char *out);
void call_diff_add_arg (const char *s);
void call_diff_setup (const char *prog, int argc, char * const *argv);
int merge (const char *dest, const char *dlabel, const char *j1,
	   const char *j1label, const char *j2, const char *j2label);

#endif /* DIFFLIB_H */
