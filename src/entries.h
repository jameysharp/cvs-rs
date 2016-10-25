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

#ifndef ENTRIES_H
#define ENTRIES_H

/* Standards */
#include <stdbool.h>

/* CVS */
#include "rcs.h"



/* The type of an entnode.  */
enum ent_type
{
    ENT_FILE, ENT_SUBDIR
};

/* structure of a entry record */
struct entnode
{
    enum ent_type type;
    char *user;
    char *version;

    /* Timestamp, or "" if none (never NULL).  */
    char *timestamp;

    /* Keyword expansion options, or "" if none (never NULL).  */
    char *options;

    char *tag;
    char *date;
    char *conflict;
};
typedef struct entnode Entnode;

void Entries_Close (List *entries, const char *update_dir);
List *Entries_Open_Dir (int aflag, const char *update_dir, const char *dir);
List *Entries_Open (int aflag, const char *update_dir);

void Register (const struct file_info *finfo, const char *vn,
	       const char *ts, const char *options, const char *tag,
	       const char *date, const char *ts_conflict);

bool entriesHasSticky (List *entries);
bool entriesHasAllSubdirs (List *entries);
bool entriesGetAflag (List *entries);
const char *entriesGetTag (List *entries);
int entriesGetNonbranch (List *entries);
const char *entriesGetDate (List *entries);

#endif /* !defined ENTRIES_H */
