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
 * You may distribute under the terms of the GNU General Public License as
 * specified in the README file that comes with the CVS source distribution.
 * 
 * Entries file to Files file
 * 
 * Creates the file Files containing the names that comprise the project, from
 * the Entries file.
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

/* Validate API */
#include "entries.h"

/* GNULIB */
#include "quote.h"

/* CVS */
#include "base.h"

#include "cvs.h"



static FILE *entfile;
static char *entfilename;		/* for error messages */



/*
 * structure used for list-private storage by Entries_Open() and
 * Version_TS() and Find_Directories().
 */
struct stickydirtag
{
    /* These fields pass sticky tag information from Entries_Open() to
       Version_TS().  */
    int aflag;
    char *tag;
    char *date;
    int nonbranch;

    /* This field is set by Entries_Open() if there was subdirectory
       information; Find_Directories() uses it to see whether it needs
       to scan the directory itself.  */
    int subdirs;
};



/*
 * Construct an Entnode
 */
static Entnode *
Entnode_Create (enum ent_type type, const char *user, const char *vn,
                const char *ts, const char *options, const char *tag,
                const char *date, const char *ts_conflict)
{
    Entnode *ent;
 
    TRACE (TRACE_FLOW,
	   "Entnode_Create (%d, %s, %s, %s, %s, %s, %s, %s)",
	   type, user, vn, ts, options, TRACE_NULL (tag), TRACE_NULL (date),
	   TRACE_NULL (ts_conflict));

    /* Note that timestamp and options must be non-NULL */
    ent = xmalloc (sizeof (Entnode));
    ent->type      = type;
    ent->user      = xstrdup (user);
    ent->version   = xstrdup (vn);
    ent->timestamp = xstrdup (ts ? ts : "");
    ent->options   = xstrdup (options ? options : "");
    ent->tag       = xstrdup (tag);
    ent->date      = xstrdup (date);
    ent->conflict  = xstrdup (ts_conflict);

    return ent;
}



/*
 * Destruct an Entnode
 */
static void
Entnode_Destroy (Entnode *ent)
{
    TRACE (TRACE_FLOW, "Entnode_Destroy ()");

    free (ent->user);
    free (ent->version);
    free (ent->timestamp);
    free (ent->options);
    if (ent->tag)
	free (ent->tag);
    if (ent->date)
	free (ent->date);
    if (ent->conflict)
	free (ent->conflict);
    free (ent);
}



static int
fputentent (FILE *fp, Entnode *p)
{
    switch (p->type)
    {
    case ENT_FILE:
        break;
    case ENT_SUBDIR:
        if (fprintf (fp, "D") < 0)
	    return 1;
	break;
    }

    if (fprintf (fp, "/%s/%s/%s", p->user, p->version, p->timestamp) < 0)
	return 1;
    if (p->conflict)
    {
	if (fprintf (fp, "+%s", p->conflict) < 0)
	    return 1;
    }
    if (fprintf (fp, "/%s/", p->options) < 0)
	return 1;

    if (p->tag)
    {
	if (fprintf (fp, "T%s\n", p->tag) < 0)
	    return 1;
    }
    else if (p->date)
    {
	if (fprintf (fp, "D%s\n", p->date) < 0)
	    return 1;
    }
    else 
    {
	if (fprintf (fp, "\n") < 0)
	    return 1;
    }

    return 0;
}



/*
 * Write out the line associated with a node of an entries file
 */
static int
write_ent_proc (Node *node, void *closure)
{
    Entnode *entnode = node->data;

    if (closure != NULL && entnode->type != ENT_FILE)
	*(int *) closure = 1;

    if (fputentent (entfile, entnode))
	error (1, errno, "cannot write %s", entfilename);

    return 0;
}



/*
 * write out the current entries file given a list,  making a backup copy
 * first of course
 */
static void
write_entries (List *list, const char *update_dir, const char *dir)
{
    int sawdir;
    char *update_file;
    char *bakfilename;
    char *entfilename;

    assert (update_dir);
    assert (dir);

    TRACE (TRACE_FUNCTION, "write_entries (%s, %s)", update_dir, dir);

    sawdir = 0;

    /* open the new one and walk the list writing entries */
    bakfilename = dir_append (dir, CVSADM_ENTBAK);
    update_file = dir_append_dirs (update_dir, dir, CVSADM_ENTBAK, NULL);
    entfile = CVS_FOPEN (bakfilename, "w+");
    if (!entfile)
    {
	/* Make this a warning, not an error.  For example, one user might
	   have checked out a working directory which, for whatever reason,
	   contains an Entries.Log file.  A second user, without write access
	   to that working directory, might want to do a "cvs log".  The
	   problem rewriting Entries shouldn't affect the ability of "cvs log"
	   to work, although the warning is probably a good idea so that
	   whether Entries gets rewritten is not an inexplicable process.  */
	error (0, errno, "cannot rewrite %s", quote (update_file));

	/* Now just return.  We leave the Entries.Log file around.  As far
	   as I know, there is never any data lying around in 'list' that
	   is not in Entries.Log at this time (if there is an error writing
	   Entries.Log that is a separate problem).  */
	goto done;
    }

    walklist (list, write_ent_proc, &sawdir);
    if (!sawdir)
    {
	/* We didn't write out any directories.  Check the list
           private data to see whether subdirectory information is
           known.  If it is, we need to write out an empty D line.  */
	if (entriesHasAllSubdirs (list))
	    if (fprintf (entfile, "D\n") < 0)
		error (1, errno, "cannot write %s", quote (update_file));
    }
    if (fclose (entfile) == EOF)
	error (1, errno, "error closing %s", quote (update_file));

    /* now, atomically (on systems that support it) rename it */
    entfilename = dir_append (dir, CVSADM_ENT);
    rename_file (bakfilename, entfilename);
    free (entfilename);

    entfilename = dir_append (dir, CVSADM_ENTLOG);
    /* now, remove the log file */
    if (unlink_file (entfilename) < 0 && !existence_error (errno))
    {
	char *newupdate_file = dir_append_dirs (update_dir, dir, CVSADM_ENTLOG,
						NULL);
	error (0, errno, "cannot remove %s", quote (newupdate_file));
	free (newupdate_file);
    }
    free (entfilename);

done:
    free (bakfilename);
    free (update_file);
}



/*
 * Removes the argument file from the Entries file if necessary.
 * Deletes the base file, if it existed.
 */
void
Scratch_Entry (List *list, const char *fname)
{
    Node *node;

    TRACE (TRACE_FUNCTION, "Scratch_Entry(%s)", fname);

    /* hashlookup to see if it is there */
    if ((node = findnode_fn (list, fname)) != NULL)
    {
	if (!noexec)
	{
	    Entnode *e = node->data;
	    base_remove (fname, e->version);

	    entfilename = CVSADM_ENTLOG;
	    entfile = xfopen (entfilename, "a");

	    if (fprintf (entfile, "R ") < 0)
		error (1, errno, "cannot write %s", entfilename);

	    write_ent_proc (node, NULL);

	    if (fclose (entfile) == EOF)
		error (1, errno, "error closing %s", entfilename);
	}

	delnode (node);			/* delete the node */

#ifdef SERVER_SUPPORT
	if (server_active)
	    server_scratch (fname);
#endif
    }
}



/*
 * Free up the memory associated with the data section of an ENTRIES type
 * node
 */
static void
Entries_delproc (Node *node)
{
    Entnode *p = node->data;

    Entnode_Destroy (p);
}



/*
 * Get an Entries file list node, initialize it, and add it to the specified
 * list
 */
static Node *
AddEntryNode (List *list, Entnode *entdata)
{
    Node *p;

    TRACE (TRACE_FLOW, "AddEntryNode (%s, %s)",
	   entdata->user, entdata->timestamp);

    /* was it already there? */
    if ((p  = findnode_fn (list, entdata->user)) != NULL)
    {
	/* take it out */
	delnode (p);
    }

    /* get a node and fill in the regular stuff */
    p = getnode ();
    p->type = ENTRIES;
    p->delproc = Entries_delproc;

    /* this one gets a key of the name for hashing */
    /* FIXME This results in duplicated data --- the hash package shouldn't
       assume that the key is dynamically allocated.  The user's free proc
       should be responsible for freeing the key. */
    p->key = xstrdup (entdata->user);
    p->data = entdata;

    /* put the node into the list */
    addnode (list, p);
    return p;
}



/*
 * Enters the given file name/version/time-stamp into the Entries file,
 * removing the old entry first, if necessary.
 */
void
Register (const struct file_info *finfo, const char *vn, const char *ts,
	  const char *options, const char *tag, const char *date,
	  const char *ts_conflict)
{
    Entnode *entnode;
    Node *node;

    TRACE (TRACE_FUNCTION, "Register(%s, %s, %s%s%s, %s, %s %s)",
	   finfo->fullname, vn, ts ? ts : "",
	   ts_conflict ? "+" : "", ts_conflict ? ts_conflict : "",
	   options, tag ? tag : "", date ? date : "");

#ifdef SERVER_SUPPORT
    if (server_active)
	server_register (finfo->file, vn, ts, options, tag, date, ts_conflict);
#endif

    entnode = Entnode_Create (ENT_FILE, finfo->file, vn, ts, options, tag,
			      date, ts_conflict);
    node = AddEntryNode (finfo->entries, entnode);

    if (!noexec)
    {
	entfilename = CVSADM_ENTLOG;
	entfile = CVS_FOPEN (entfilename, "a");

	if (!entfile)
	{
	    /* Warning, not error, as in write_entries.  */
	    /* FIXME-update-dir: should be including update_dir in message.  */
	    error (0, errno, "cannot open %s", entfilename);
	    return;
	}

	if (fprintf (entfile, "A ") < 0)
	    error (1, errno, "cannot write %s", entfilename);

	write_ent_proc (node, NULL);

        if (fclose (entfile) == EOF)
	    error (1, errno, "error closing %s", entfilename);
    }
}



/*
 * Node delete procedure for list-private sticky dir tag/date info
 */
static void
freesdt (Node *p)
{
    struct stickydirtag *sdtp = p->data;

    if (sdtp->tag)
	free (sdtp->tag);
    if (sdtp->date)
	free (sdtp->date);
    free ((char *) sdtp);
}



/* Return true iff ENTRIES && SDTP && (SDTP->tag || SDTP->date).
 */
bool
entriesHasSticky (List *entries)
{
    struct stickydirtag *sdtp;

    if (!entries) return false;

    assert (entries->list);

    sdtp = entries->list->data;
    return sdtp && (sdtp->tag || sdtp->date);
}



/* When SDTP is NULL, or SDTP->subdirs is nonzero, then all subdirectory
 * information is recorded in ENTRIES.
 */
bool
entriesHasAllSubdirs (List *entries)
{
    struct stickydirtag *sdtp;

    assert (entries);
    assert (entries->list);

    sdtp = entries->list->data;
    return !sdtp || sdtp->subdirs;
}



/* Returns false if !ENTRIES || !SDTP.  Otherwise, returns SDTP->aflag.
 */
bool
entriesGetAflag (List *entries)
{
    struct stickydirtag *sdtp;

    if (!entries) return false;

    assert (entries->list);

    sdtp = entries->list->data;
    if (!sdtp) return false;

    return sdtp->aflag;
}



const char *
entriesGetTag (List *entries)
{
    struct stickydirtag *sdtp;

    assert (entries);
    assert (entries->list);
    assert (entries->list->data);

    sdtp = entries->list->data;
    return sdtp->tag;
}



int
entriesGetNonbranch (List *entries)
{
    struct stickydirtag *sdtp;

    assert (entries);
    assert (entries->list);
    assert (entries->list->data);

    sdtp = entries->list->data;
    return sdtp->nonbranch;
}



const char *
entriesGetDate (List *entries)
{
    struct stickydirtag *sdtp;

    assert (entries);
    assert (entries->list);
    assert (entries->list->data);

    sdtp = entries->list->data;
    return sdtp->date;
}



/* Return the next real Entries line.  On end of file, returns NULL.
   On error, prints an error message and returns NULL.  */
static Entnode *
fgetentent (FILE *fpin, char *cmd, int *sawdir)
{
    Entnode *ent;
    char *line;
    size_t line_chars_allocated;
    register char *cp;
    enum ent_type type;
    char *l, *user, *vn, *ts, *options;
    char *tag_or_date, *tag, *date, *ts_conflict;
    int line_length;

    line = NULL;
    line_chars_allocated = 0;

    ent = NULL;
    while ((line_length = getline (&line, &line_chars_allocated, fpin)) > 0)
    {
	l = line;

	/* If CMD is not NULL, we are reading an Entries.Log file.
	   Each line in the Entries.Log file starts with a single
	   character command followed by a space.  For backward
	   compatibility, the absence of a space indicates an add
	   command.  */
	if (cmd != NULL)
	{
	    if (l[1] != ' ')
		*cmd = 'A';
	    else
	    {
		*cmd = l[0];
		l += 2;
	    }
	}

	type = ENT_FILE;

	if (l[0] == 'D')
	{
	    type = ENT_SUBDIR;
	    *sawdir = 1;
	    ++l;
	    /* An empty D line is permitted; it is a signal that this
	       Entries file lists all known subdirectories.  */
	}

	if (l[0] != '/')
	    continue;

	user = l + 1;
	if ((cp = strchr (user, '/')) == NULL)
	    continue;
	*cp++ = '\0';
	vn = cp;
	if ((cp = strchr (vn, '/')) == NULL)
	    continue;
	*cp++ = '\0';
	ts = cp;
	if ((cp = strchr (ts, '/')) == NULL)
	    continue;
	*cp++ = '\0';
	options = cp;
	if ((cp = strchr (options, '/')) == NULL)
	    continue;
	*cp++ = '\0';
	tag_or_date = cp;
	if ((cp = strchr (tag_or_date, '\n')) == NULL)
	    continue;
	*cp = '\0';
	tag = NULL;
	date = NULL;
	if (*tag_or_date == 'T')
	    tag = tag_or_date + 1;
	else if (*tag_or_date == 'D')
	    date = tag_or_date + 1;

	if ((ts_conflict = strchr (ts, '+')))
	    *ts_conflict++ = '\0';
	    
	/*
	 * XXX - Convert timestamp from old format to new format.
	 *
	 * If the timestamp doesn't match the file's current
	 * mtime, we'd have to generate a string that doesn't
	 * match anyways, so cheat and base it on the existing
	 * string; it doesn't have to match the same mod time.
	 *
	 * For an unmodified file, write the correct timestamp.
	 */
	{
	    struct stat sb;
	    if (strlen (ts) > 30 && stat (user, &sb) == 0)
	    {
		char *c = ctime (&sb.st_mtime);
		/* Fix non-standard format.  */
		if (c[8] == '0') c[8] = ' ';

		if (STRNEQ (ts + 25, c, 24))
		    ts = time_stamp (user);
		else
		{
		    ts += 24;
		    ts[0] = '*';
		}
	    }
	}

	ent = Entnode_Create (type, user, vn, ts, options, tag, date,
			      ts_conflict);
	break;
    }

    if (line_length < 0 && !feof (fpin))
	error (0, errno, "cannot read entries file");

    free (line);
    return ent;
}



/* Read the entries file into a list, hashing on the file name.

   UPDATE_DIR is the name of the current directory, for use in error
   messages, or NULL if not known (that is, noone has gotten around
   to updating the caller to pass in the information).  */
List *
Entries_Open_Dir (int aflag, const char *update_dir_i, const char *dir)
{
    List *entries;
    struct stickydirtag *sdtp = NULL;
    Entnode *ent;
    char *dirtag, *dirdate;
    int dirnonbranch;
    int do_rewrite = 0;
    FILE *fpin;
    int sawdir;
    char *entfile;
    char *update_dir;

    assert (update_dir_i);
    assert (dir);

    TRACE (TRACE_FLOW, "Entries_Open_Dir (%s, %s)", update_dir_i, dir);

    /* get a fresh list... */
    entries = getlist ();

    /*
     * Parse the CVS/Tag file, to get any default tag/date settings. Use
     * list-private storage to tuck them away for Version_TS().
     */
    ParseTag (&dirtag, &dirdate, &dirnonbranch);
    if (aflag || dirtag || dirdate)
    {
	sdtp = xzalloc (sizeof (*sdtp));
	sdtp->aflag = aflag;
	sdtp->tag = xstrdup (dirtag);
	sdtp->date = xstrdup (dirdate);
	sdtp->nonbranch = dirnonbranch;

	/* feed it into the list-private area */
	entries->list->data = sdtp;
	entries->list->delproc = freesdt;
    }

    sawdir = 0;

    update_dir = dir_append (update_dir_i, dir);
    entfile = dir_append (dir, CVSADM_ENT);
    fpin = CVS_FOPEN (entfile, "r");
    if (fpin)
    {
	while ((ent = fgetentent (fpin, NULL, &sawdir)) != NULL)
	    AddEntryNode (entries, ent);

	if (fclose (fpin) < 0)
	{
	    /* FIXME-update-dir: should include update_dir in message.  */
	    char *update_file = dir_append (update_dir, CVSADM_ENT);
	    error (0, errno, "cannot close %s", quote (update_file));
	    free (update_file);
	}
    }
    else
    {
	char *update_file = dir_append (update_dir, CVSADM_ENT);
	error (0, errno, "cannot open %s for reading", quote (update_file));
	free (update_file);
    }
    free (entfile);

    entfile = dir_append (dir, CVSADM_ENTLOG);
    fpin = CVS_FOPEN (entfile, "r");
    if (fpin) 
    {
	char cmd;
	Node *node;

	while ((ent = fgetentent (fpin, &cmd, &sawdir)) != NULL)
	{
	    switch (cmd)
	    {
	    case 'A':
		(void) AddEntryNode (entries, ent);
		break;
	    case 'R':
		node = findnode_fn (entries, ent->user);
		if (node) delnode (node);
		Entnode_Destroy (ent);
		break;
	    default:
		/* Ignore unrecognized commands.  */
		Entnode_Destroy (ent);
	        break;
	    }
	}
	do_rewrite = 1;
	if (fclose (fpin) < 0)
	{
	    char *update_file = dir_append (update_dir, CVSADM_ENTLOG);
	    error (0, errno, "cannot close %s", quote (update_file));
	    free (update_file);
	}
    }
    free (entfile);

    /* Update the list private data to indicate whether subdirectory
       information is known.  Nonexistent list private data is taken
       to mean that it is known.  */
    if (sdtp)
	sdtp->subdirs = sawdir;
    else if (!sawdir)
    {
	sdtp = xzalloc (sizeof (*sdtp));
	sdtp->subdirs = 0;
	entries->list->data = sdtp;
	entries->list->delproc = freesdt;
    }

    if (do_rewrite && !noexec)
	write_entries (entries, update_dir_i, dir);

    /* clean up and return */
    free (update_dir);
    if (dirtag)
	free (dirtag);
    if (dirdate)
	free (dirdate);
    return entries;
}



List *
Entries_Open (int aflag, const char *update_dir)
{
    return Entries_Open_Dir (aflag, update_dir, ".");
}



void
Entries_Close_Dir (List *list, const char *update_dir, const char *dir)
{
    assert (dir);
    assert (update_dir);

    if (list)
    {
	if (!noexec)
	{
	    char *entfile = dir_append (dir, CVSADM_ENTLOG);
	    if (isfile (entfile))
		write_entries (list, update_dir, dir);
	    free (entfile);
	}
	dellist (&list);
    }
}



void
Entries_Close (List *list, const char *update_dir)
{
    Entries_Close_Dir (list, update_dir, ".");
}



/*
 * Write out the CVS/Template file.
 */
void
WriteTemplate (const char *update_dir, int xdotemplate, const char *repository)
{
#ifdef SERVER_SUPPORT
    TRACE (TRACE_FUNCTION, "Write_Template (%s, %s)", update_dir, repository);

    if (noexec)
	return;

    if (server_active && xdotemplate)
    {
	/* Clear the CVS/Template if supported to allow for the case
	 * where the rcsinfo file no longer has an entry for this
	 * directory.
	 */
	server_clear_template (update_dir, repository);
	server_template (update_dir, repository);
    }
#endif

    return;
}



/*
 * Write out/Clear the CVS/Tag file.
 */
void
WriteTag (const char *dir, const char *tag, const char *date, int nonbranch,
          const char *update_dir, const char *repository)
{
    FILE *fout;
    char *tmp;

    if (noexec)
	return;

    if (dir != NULL)
	tmp = Xasprintf ("%s/%s", dir, CVSADM_TAG);
    else
	tmp = xstrdup (CVSADM_TAG);


    if (unlink_file (tmp) < 0 && ! existence_error (errno))
	error (1, errno, "cannot remove %s", tmp);

    if (tag || date)
    {
	fout = xfopen (tmp, "w");
	if (tag)
	{
	    if (fprintf (fout, "%c%s\n", nonbranch ? 'N' : 'T', tag) < 0)
		error (1, errno, "write to %s failed", tmp);
	}
	else
	{
	    if (fprintf (fout, "D%s\n", date) < 0)
		error (1, errno, "write to %s failed", tmp);
	}
	if (fclose (fout) == EOF)
	    error (1, errno, "cannot close %s", tmp);
    }
    free (tmp);
#ifdef SERVER_SUPPORT
    if (server_active)
	server_set_sticky (update_dir, repository, tag, date, nonbranch);
#endif
}



/* Parse the CVS/Tag file for the current directory.

   If it contains a date, sets *DATEP to the date in a newly malloc'd
   string, *TAGP to NULL, and *NONBRANCHP to an unspecified value.

   If it contains a branch tag, sets *TAGP to the tag in a newly
   malloc'd string, *NONBRANCHP to 0, and *DATEP to NULL.

   If it contains a nonbranch tag, sets *TAGP to the tag in a newly
   malloc'd string, *NONBRANCHP to 1, and *DATEP to NULL.

   If it does not exist, or contains something unrecognized by this
   version of CVS, set *DATEP and *TAGP to NULL and *NONBRANCHP to
   an unspecified value.

   If there is an error, print an error message, set *DATEP and *TAGP
   to NULL, and return.  */
void
ParseTag (char **tagp, char **datep, int *nonbranchp)
{
    FILE *fp;

    if (tagp)
	*tagp = NULL;
    if (datep)
	*datep = NULL;
    /* Always store a value here, even in the 'D' case where the value
       is unspecified.  Shuts up tools which check for references to
       uninitialized memory.  */
    if (nonbranchp != NULL)
	*nonbranchp = 0;
    fp = CVS_FOPEN (CVSADM_TAG, "r");
    if (fp)
    {
	char *line;
	int line_length;
	size_t line_chars_allocated;

	line = NULL;
	line_chars_allocated = 0;

	if ((line_length = getline (&line, &line_chars_allocated, fp)) > 0)
	{
	    /* Remove any trailing newline.  */
	    if (line[line_length - 1] == '\n')
	        line[--line_length] = '\0';
	    switch (*line)
	    {
		case 'T':
		    if (tagp != NULL)
			*tagp = xstrdup (line + 1);
		    break;
		case 'D':
		    if (datep != NULL)
			*datep = xstrdup (line + 1);
		    break;
		case 'N':
		    if (tagp != NULL)
			*tagp = xstrdup (line + 1);
		    if (nonbranchp != NULL)
			*nonbranchp = 1;
		    break;
		default:
		    /* Silently ignore it; it may have been
		       written by a future version of CVS which extends the
		       syntax.  */
		    break;
	    }
	}

	if (line_length < 0)
	{
	    /* FIXME-update-dir: should include update_dir in messages.  */
	    if (feof (fp))
		error (0, 0, "cannot read %s: end of file", CVSADM_TAG);
	    else
		error (0, errno, "cannot read %s", CVSADM_TAG);
	}

	if (fclose (fp) < 0)
	    /* FIXME-update-dir: should include update_dir in message.  */
	    error (0, errno, "cannot close %s", CVSADM_TAG);

	free (line);
    }
    else if (!existence_error (errno))
	/* FIXME-update-dir: should include update_dir in message.  */
	error (0, errno, "cannot open %s", CVSADM_TAG);
}



/*
 * This is called if all subdirectory information is known, but there
 * aren't any subdirectories.  It records that fact in the list
 * private data.
 */
void
Subdirs_Known (List *entries)
{
    struct stickydirtag *sdtp = entries->list->data;

    /* If there is no list private data, that means that the
       subdirectory information is known.  */
    if (sdtp != NULL && ! sdtp->subdirs)
    {
	FILE *fp;

	sdtp->subdirs = 1;
	if (!noexec)
	{
	    /* Create Entries.Log so that Entries_Close will do something.  */
	    entfilename = CVSADM_ENTLOG;
	    fp = CVS_FOPEN (entfilename, "a");
	    if (fp == NULL)
	    {
		int save_errno = errno;

		/* As in subdir_record, just silently skip the whole thing
		   if there is no CVSADM directory.  */
		if (! isdir (CVSADM))
		    return;
		error (1, save_errno, "cannot open %s", entfilename);
	    }
	    else
	    {
		if (fclose (fp) == EOF)
		    error (1, errno, "cannot close %s", entfilename);
	    }
	}
    }
}



/* Record subdirectory information.  */
static Entnode *
subdir_record (int cmd, const char *parent, const char *dir)
{
    Entnode *entnode;

    /* None of the information associated with a directory is
       currently meaningful.  */
    entnode = Entnode_Create (ENT_SUBDIR, dir, "", "", "",
			      NULL, NULL, NULL);

    if (!noexec)
    {
	if (parent == NULL)
	    entfilename = CVSADM_ENTLOG;
	else
	    entfilename = Xasprintf ("%s/%s", parent, CVSADM_ENTLOG);

	entfile = CVS_FOPEN (entfilename, "a");
	if (entfile == NULL)
	{
	    int save_errno = errno;

	    /* It is not an error if there is no CVS administration
               directory.  Permitting this case simplifies some
               calling code.  */

	    if (parent == NULL)
	    {
		if (! isdir (CVSADM))
		    return entnode;
	    }
	    else
	    {
		free (entfilename);
		entfilename = Xasprintf ("%s/%s", parent, CVSADM);
		if (! isdir (entfilename))
		{
		    free (entfilename);
		    entfilename = NULL;
		    return entnode;
		}
	    }

	    error (1, save_errno, "cannot open %s", entfilename);
	}

	if (fprintf (entfile, "%c ", cmd) < 0)
	    error (1, errno, "cannot write %s", entfilename);

	if (fputentent (entfile, entnode) != 0)
	    error (1, errno, "cannot write %s", entfilename);

	if (fclose (entfile) == EOF)
	    error (1, errno, "error closing %s", entfilename);

	if (parent != NULL)
	{
	    free (entfilename);
	    entfilename = NULL;
	}
    }

    return entnode;
}



/*
 * Record the addition of a new subdirectory DIR in PARENT.  PARENT
 * may be NULL, which means the current directory.  ENTRIES is the
 * current entries list; it may be NULL, which means that it need not
 * be updated.
 */
void
Subdir_Register (List *entries, const char *parent, const char *dir)
{
    Entnode *entnode;

    TRACE (TRACE_FUNCTION, "Subdir_Register (%s, %s)", parent, dir);

    /* Ignore attempts to register ".".  These can happen in the
       server code.  */
    if (dir[0] == '.' && dir[1] == '\0')
	return;

    entnode = subdir_record ('A', parent, dir);

    if (entries && (!parent || STREQ (parent, ".")))
	(void) AddEntryNode (entries, entnode);
    else
	Entnode_Destroy (entnode);
}



/*
 * Record the removal of a subdirectory.  The arguments are the same
 * as for Subdir_Register.
 */

void
Subdir_Deregister (List *entries, const char *parent, const char *dir)
{
    Entnode *entnode;

    entnode = subdir_record ('R', parent, dir);
    Entnode_Destroy (entnode);

    if (entries && (!parent || STREQ (parent, ".")))
    {
	Node *p;

	p = findnode_fn (entries, dir);
	if (p != NULL)
	    delnode (p);
    }
}
