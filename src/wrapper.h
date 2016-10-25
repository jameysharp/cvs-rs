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

#ifndef WRAPPER_H
#define WRAPPER_H

typedef enum { WRAP_MERGE, WRAP_COPY } WrapMergeMethod;
typedef enum {
    /* -t and -f wrapper options.  Treating directories as single files.  */
    WRAP_TOCVS,
    WRAP_FROMCVS,
    /* -k wrapper option.  Default keyword expansion options.  */
    WRAP_RCSOPTION
} WrapMergeHas;

void  wrap_setup (void);
int   wrap_name_has (const char *name, WrapMergeHas has);
char *wrap_rcsoption (const char *fileName, int asFlag);
char *wrap_tocvs_process_file (const char *fileName);
int   wrap_merge_is_copy (const char *fileName);
void wrap_fromcvs_process_file (const char *fileName);
void wrap_add_file (const char *file, int temp);
void wrap_add (char *line, int temp);
void wrap_send (void);
#if defined (SERVER_SUPPORT) || defined (CLIENT_SUPPORT)
void wrap_unparse_rcs_options (char **, int);
#endif /* SERVER_SUPPORT || CLIENT_SUPPORT */

#endif /* WRAPPER_H */
