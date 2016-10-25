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

#ifndef BASE_H
#define BASE_H

#include "rcs.h"

enum update_existing {
    /* We are replacing an existing file.  */
    UPDATE_ENTRIES_EXISTING,
    /* We are creating a new file.  */
    UPDATE_ENTRIES_NEW,
    /* We don't know whether it is existing or new.  */
    UPDATE_ENTRIES_EXISTING_OR_NEW
};



char *make_base_file_name (const char *filename, const char *rev);

char *base_get (const char *update_dir, const char *file);
void base_register (const char *update_dir, const char *file, char *rev);
void base_deregister (const char *update_dir, const char *file);

int base_checkout (RCSNode *rcs, struct file_info *finfo,
		   const char *prev, const char *rev, const char *ptag,
		   const char *tag, const char *poptions, const char *options);
char *temp_checkout (RCSNode *rcs, struct file_info *finfo,
		     const char *prev, const char *rev, const char *ptag,
		     const char *tag, const char *poptions,
		     const char *options);
enum update_existing translate_exists (const char *exists);
bool validate_change (enum update_existing existp,
		      const struct file_info *finfo);
void base_copy (struct file_info *finfo, const char *rev, const char *flags);
void temp_copy (struct file_info *finfo, const char *flags,
		const char *tempfile);
void base_remove (const char *file, const char *rev);
int base_merge (RCSNode *rcs, struct file_info *finfo, const char *ptag,
		const char *poptions, const char *options,
	        const char *urev, const char *rev1, const char *rev2,
		bool join);
int base_diff (const struct file_info *finfo,
	       int diff_argc, char *const *diff_argv,
	       const char *f1, const char *use_rev1, const char *label1,
	       const char *f2, const char *use_rev2, const char *label2,
	       bool empty_files);
#endif /* BASE_H */
