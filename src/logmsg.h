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

#ifndef LOGMSG_H
#define LOGMSG_H

/* CVS Headers.  */
#include "classify.h"



/*
 * structure used for list nodes passed to Update_Logfile() and
 * do_editor().
 */
struct logfile_info
{
  Ctype type;
  char *tag;
  char *rev_old;		/* rev number before a commit/modify,
				   NULL for add or import */
  char *rev_new;		/* rev number after a commit/modify,
				   add, or import, NULL for remove */
};



#define LOGMSG_REREAD_NEVER 0	/* do_verify - never  reread message */
#define LOGMSG_REREAD_ALWAYS 1	/* do_verify - always reread message */
#define LOGMSG_REREAD_STAT 2	/* do_verify - reread message if changed */
int do_verify (char **messagep, const char *repository, List *changes);

#endif /* LOGMSG_H */
