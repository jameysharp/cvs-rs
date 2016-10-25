/*
 * Copyright (C) 1986-2007 The Free Software Foundation, Inc.
 *
 * Portions Copyright (C) 1998-2007 Derek Price,
 *                                  Ximbiot LLC <http://ximbiot.com>,
 *                                  and others.
 *
 * Portions Copyright (C) 1992, Brian Berliner and Jeff Polk
 * Portions Copyright (C) 1989-1992, Brian Berliner
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
 */

#ifndef SUBR_H
# define SUBR_H

/* ANSI C */
#include <string.h>

/* CVS */
/* FIXME - This shouldn't be needed here.  Any routine that needs to understand
 * the underlying structure of an RCSNode should be in rcs*.c.
 */
#include "rcs.h"



#ifdef USE_VMS_FILENAMES
# define BAKPREFIX	"_$"
#else /* USE_VMS_FILENAMES */
# define BAKPREFIX	".#"		/* when rcsmerge'ing */
#endif /* USE_VMS_FILENAMES */



void expand_string (char **, size_t *, size_t);
char *Xreadlink (const char *link, size_t size);
void xrealloc_and_strcat (char **, size_t *, const char *);
int strip_trailing_newlines (char *str);
int pathname_levels (const char *path);
void free_names (int *pargc, char *argv[]);
void line2argv (int *pargc, char ***argv, char *line, char *sepchars);
int numdots (const char *s);
int compare_revnums (const char *, const char *);
char *increment_revnum (const char *);
char *getcaller (void);
char *previous_rev (RCSNode *rcs, const char *rev);
char *gca (const char *rev1, const char *rev2);
void check_numeric (const char *, int, char **);
char *make_message_rcsvalid (const char *message);
int file_has_markers (const struct file_info *);
bool file_contains_keyword (const struct file_info *finfo);
void get_stream (FILE *, const char *, char **, size_t *, size_t *);
void get_file (const char *, const char *, const char *,
               char **, size_t *, size_t *);
void force_write_file (const char *file, const char *data, size_t len);
void write_file (const char *file, const char *data, size_t len);
void resolve_symlink (char **filename);
char *backup_file (const char *file, const char *suffix);
char *shell_escape (char *buf, const char *str);
void sleep_past (time_t desttime);

/* for format_cmdline function - when a list variable is bound to a user string,
 * we need to pass some data through walklist into the callback function.
 * We use this struct.
 */
struct format_cmdline_walklist_closure
{
    const char *file;	/* The trigger file being processed.  */
    int line;		/* The line number in the trigger being processed.  */
    const char *format;	/* the format string the user passed us */
    char **buf;		/* *dest = our NUL terminated and possibly too short
			 * destination string
			 */
    size_t *length;	/* *dlen = how many bytes have already been allocated to
			 * *dest.
			 */
    char **d;		/* our pointer into buf where the next char should go */
    char quotes;	/* quotes we are currently between, if any */
#ifdef SUPPORT_OLD_INFO_FMT_STRINGS
    int onearg;
    int firstpass;
    const char *srepos;
#endif /* SUPPORT_OLD_INFO_FMT_STRINGS */
    void *closure;	/* our user defined closure */
};
char *cmdlinequote (char quotes, char *s);
char *cmdlineescape (char quotes, char *s);
char *format_cmdline (
#ifdef SUPPORT_OLD_INFO_FMT_STRINGS
		      bool oldway, const char *srepos,
#endif /* SUPPORT_OLD_INFO_FMT_STRINGS */
		      const char *file, int line, const char *format, ...);

/* Many, many CVS calls to xstrdup depend on it to return NULL when its
 * argument is NULL.
 */
#define xstrdup Xstrdup
char *Xstrdup (const char *str)
	__attribute__ ((__malloc__));

char *Xasprintf (const char *format, ...)
	__attribute__ ((__malloc__, __format__ (__printf__, 1, 2)));
char *Xasnprintf (char *resultbuf, size_t *lengthp, const char *format, ...)
        __attribute__ ((__malloc__, __format__ (__printf__, 3, 4)));
bool readBool (const char *infopath, const char *option,
	       const char *p, bool *val);

FILE *xfopen (const char *, const char *);
char *xcanonicalize_file_name (const char *path);
bool isThisHost (const char *otherhost);
bool isSamePath (const char *path1_in, const char *path2_in);
bool isParentPath (const char *maybe_parent, const char *maybe_child);

# ifdef HAVE_CVS_ADMIN_GROUP
bool is_admin (void);
# else
/* If the CVS_ADMIN_GROUP is not being used, then anyone may run admin
 * commands.
 */
#   define is_admin()	true
# endif

#define MD_QUIET	(1 << 0)	/* Don't spout on nonfatal errors.  */
#define MD_FATAL	(1 << 1)	/* Die on error.  */
#define MD_REPO		(1 << 2)	/* Honor CVSUMASK.  */
#define MD_FORCE	(1 << 3)	/* Ignore NOEXEC.  */
#define MD_EXIST_OK	(1 << 4)	/* Ok if directory already exists.  */
bool cvs_mkdir (const char *name, const char *update_dir, unsigned int flags);
bool cvs_xmkdir (const char *name, const char *update_dir, unsigned int flags);
bool cvs_mkdirs (const char *name, mode_t mode, const char *update_dir,
		 unsigned int flags);
bool cvs_xmkdirs (const char *name, mode_t mode, const char *update_dir,
		  unsigned int flags);

char *dir_append (const char *dir, const char *append);
char *dir_append_dirs (const char *dir, ...);
bool hasSlash (const char *path);
bool hasAdmin (const char *dir);
char *update_dir_name (const char *path);



/* The STREQ macro speeds string comparison up a bit by skipping the function
 * call to strcmp() when the first characters are different.  The inlined
 * streq() call is necessary to avoid evaluating the macro arguments more than
 * once, but we don't want to do that with a compiler that doesn't deal
 * effectively with the inline keyword.
 */
#ifdef HAVE_INLINE
static inline bool
streq (const char *a, const char *b)
{
    return *a == *b && strcmp (a, b) == 0;
}
static inline bool
strneq (const char *a, const char *b, size_t n)
{
    return !n || (*a == *b && strncmp (a, b, n) == 0);
}
# define STREQ(a, b) streq ((a), (b))
# define STRNEQ(a, b, n) strneq ((a), (b), (n))
#else /* !HAVE_INLINE */
# define STREQ(a, b) (strcmp ((a), (b)) == 0)
# define STRNEQ(a, b, n) (strncmp ((a), (b), (n)) == 0)
#endif /* HAVE_INLINE */

/* Convenience macro for printing the commonly used but sometimes empty
 * UPDATE_DIR string.
 */
#define NULL2DOT(u) ((u) && *(u) ? (u) : ".")

/* Convenience macro for equating a NULL pointer and the empty string.  */
#define NULL2MT(s) ((s) ? (s) : "")

/* Replace pointer D with S, freeing D afterwards.  This is useful when S
 * is an expression containing D.
 */
#define REPLACE(d, s)	{void *tmp = (d); d = (s); free (tmp);}

#endif /* !SUBR_H */
