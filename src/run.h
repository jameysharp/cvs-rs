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

#ifndef RUN_H
#define RUN_H

#include <stdbool.h>
#include <stdio.h>

/* flags for run_exec(), the fast system() for CVS */
#define	RUN_NORMAL            0x0        /* no special behaviour */
#define	RUN_COMBINED          (0x1 << 0) /* stdout is duped to stderr */
#define	RUN_REALLY            (0x1 << 1) /* do the exec, even if noexec is on */
#define	RUN_STDOUT_APPEND     (0x1 << 2) /* append to stdout, don't truncate */
#define	RUN_STDERR_APPEND     (0x1 << 3) /* append to stderr, don't truncate */
#define	RUN_SIGIGNORE         (0x1 << 4) /* ignore interrupts for command */
#define	RUN_TTY               NULL

void run_add_arg_p (int *, size_t *, char ***, const char *s);
void run_arg_free_p (int, char **);
void run_add_arg (const char *s);
void run_print (FILE * fp);
void run_setup (const char *prog);
int run_exec (const char *stin, const char *stout, const char *sterr,
              int flags);
int run_piped (int *, int *);
FILE *run_popen (const char *, const char *);
int piped_child (char *const *, int *, int *, bool);
void close_on_exec (int);


#endif /* RUN_H */
