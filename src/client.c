/* 
 * Copyright (C) 2007 The Free Software Foundation, Inc.
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

/*
 * CVS client-related stuff.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif /* HAVE_CONFIG_H */

/* Verify interface.  */
#include "client.h"

/* C99 Headers.  */
#include <inttypes.h>

/* GNULIB headers.  */
#include "quote.h"
#include "save-cwd.h"

/* CVS headers.  */
#include "base.h"
#include "buffer.h"
#include "command_line_opt.h"
#include "diff.h"
#include "difflib.h"
#include "edit.h"
#include "find-names.h"
#include "gpg.h"
#include "ignore.h"
#include "recurse.h"
#include "repos.h"
#include "wrapper.h"

#include "cvs.h"



#ifdef CLIENT_SUPPORT

# include "log-buffer.h"
# include "md5.h"
# include "sign.h"

#include "socket-client.h"
#include "rsh-client.h"

# ifdef HAVE_GSSAPI
#   include "gssapi-client.h"
# endif

# ifdef HAVE_KERBEROS
#   include "kerberos4-client.h"
# endif



/* Whether the last Base-merge command from the server resulted in a conflict
 * or not.
 */
static bool last_merge;
static bool last_merge_conflict;
static bool last_merge_made_base;
static char *base_merge_rev1;
static char *base_merge_rev2;
static char *temp_checkout1;
static char *temp_checkout2;

/* Similarly, to ignore bad entries on error.  */
static bool base_copy_error;



/* Keep track of any paths we are sending for Max-dotdot so that we can verify
 * that uplevel paths coming back form the server are valid.
 *
 * FIXME: The correct way to do this is probably provide some sort of virtual
 * path map on the client side.  This would be generic enough to be applied to
 * absolute paths supplied by the user too.
 */
static List *uppaths;



static void add_prune_candidate (const char *);

/* All the commands.  */
int add (int argc, char **argv);
int admin (int argc, char **argv);
int checkout (int argc, char **argv);
int commit (int argc, char **argv);
int diff (int argc, char **argv);
int history (int argc, char **argv);
int import (int argc, char **argv);
int cvslog (int argc, char **argv);
int patch (int argc, char **argv);
int release (int argc, char **argv);
int cvsremove (int argc, char **argv);
int rtag (int argc, char **argv);
int status (int argc, char **argv);
int tag (int argc, char **argv);
int update (int argc, char **argv);

static size_t try_read_from_server (char *, size_t);

static void auth_server (cvsroot_t *, struct buffer *, struct buffer *,
			 int, int, struct hostent *);



/* This is the referrer who referred us to a primary, or write server, using
 * the "Redirect" request.
 */
static cvsroot_t *client_referrer;

/* We need to keep track of the list of directories we've sent to the
   server.  This list, along with the current CVSROOT, will help us
   decide which command-line arguments to send.  */
List *dirs_sent_to_server;

/* walklist() callback for determining if a D is a dir name already sent to the
 * server or the parent of one.
 */
static int
is_arg_a_parent_or_listed_dir (Node *n, void *d)
{
    return isParentPath (d, n->key);
}



/* Return true if this argument should not be sent to the server. */
static bool
arg_should_not_be_sent_to_server (char *arg)
{
    const char *root_string;
    char *dir;

    /* Decide if we should send this directory name to the server.  We
     * should always send argv[i] if:
     *
     * 1) the list of directories sent to the server is empty (as it
     *    will be for checkout, etc.).
     *
     * 2) the argument is "."
     *
     * 3) the argument is, or is a parent of, one of the paths in
     *    DIRS_SENT_TO_SERVER.
     *
     * 4) the argument is a file in the CWD and the CWD is checked out
     *    from the current root
     */

    if (list_isempty (dirs_sent_to_server))
	return false;		/* always send it */

    if (STREQ (arg, "."))
	return false;		/* always send it */

    /* We should send arg if it is one of the directories sent to the
     * server or the parent of one; this tells the server to descend
     * the hierarchy starting at this level.
     */
    if (isdir (arg))
    {
	if (walklist (dirs_sent_to_server, is_arg_a_parent_or_listed_dir, arg))
	    return false;

	/* If arg wasn't a parent, we don't know anything about it (we
	   would have seen something related to it during the
	   send_files phase).  Don't send it.  */
	return true;
    }

    /* Now we either have a file or ARG does not exist locally.  Try to decide
     * whether we should send arg to the server by checking the contents of
     * ARGS's parent directory's CVSADM dir, if it exists.
     */

    dir = dir_name (arg);

    /* First, check to see if we already sent this directory to the server,
     * because it takes less time than actually opening the stuff in the
     * CVSADM directory.
     */
    if (findnode_fn (dirs_sent_to_server, dir))
    {
	free (dir);
	return false;
    }

    if (CVSroot_cmdline)
	root_string = CVSroot_cmdline;
    else
    {
	const cvsroot_t *this_root;
	char *update_dir = update_dir_name (arg);

	this_root = Name_Root (dir, update_dir);
	root_string = this_root->original;
	free (update_dir);
    }
    free (dir);

    /* Now check the value for root. */
    if (root_string && current_parsed_root
	&& !STREQ (root_string, original_parsed_root->original))
	/* Don't send this, since the CVSROOTs don't match. */
	return true;
    
    /* OK, let's send it.  */
    return false;
}
#endif /* CLIENT_SUPPORT */



#if defined(CLIENT_SUPPORT) || defined(SERVER_SUPPORT)

/* Shared with server.  */

/*
 * Return a malloc'd, '\0'-terminated string
 * corresponding to the mode in SB.
 */
char *
mode_to_string (mode_t mode)
{
    char u[4], g[4], o[4];
    int i;

    i = 0;
    if (mode & S_IRUSR) u[i++] = 'r';
    if (mode & S_IWUSR) u[i++] = 'w';
    if (mode & S_IXUSR) u[i++] = 'x';
    u[i] = '\0';

    i = 0;
    if (mode & S_IRGRP) g[i++] = 'r';
    if (mode & S_IWGRP) g[i++] = 'w';
    if (mode & S_IXGRP) g[i++] = 'x';
    g[i] = '\0';

    i = 0;
    if (mode & S_IROTH) o[i++] = 'r';
    if (mode & S_IWOTH) o[i++] = 'w';
    if (mode & S_IXOTH) o[i++] = 'x';
    o[i] = '\0';

    return Xasprintf ("u=%s,g=%s,o=%s", u, g, o);
}



/*
 * Change mode of FILENAME to MODE_STRING.
 * Returns 0 for success or errno code.
 * If RESPECT_UMASK is set, then honor the umask.
 */
int
change_mode (const char *filename, const char *mode_string, int respect_umask)
{
#ifdef CHMOD_BROKEN
    char *p;
    int writeable = 0;

    /* We can only distinguish between
         1) readable
         2) writeable
         3) Picasso's "Blue Period"
       We handle the first two. */
    p = mode_string;
    while (*p != '\0')
    {
	if ((p[0] == 'u' || p[0] == 'g' || p[0] == 'o') && p[1] == '=')
	{
	    char *q = p + 2;
	    while (*q != ',' && *q != '\0')
	    {
		if (*q == 'w')
		    writeable = 1;
		++q;
	    }
	}
	/* Skip to the next field.  */
	while (*p != ',' && *p != '\0')
	    ++p;
	if (*p == ',')
	    ++p;
    }

    /* xchmod honors the umask for us.  In the !respect_umask case, we
       don't try to cope with it (probably to handle that well, the server
       needs to deal with modes in data structures, rather than via the
       modes in temporary files).  */
    xchmod (filename, writeable);
	return 0;

#else /* ! CHMOD_BROKEN */

    const char *p;
    mode_t mode = 0;
    mode_t oumask;

    p = mode_string;
    while (*p != '\0')
    {
	if ((p[0] == 'u' || p[0] == 'g' || p[0] == 'o') && p[1] == '=')
	{
	    int can_read = 0, can_write = 0, can_execute = 0;
	    const char *q = p + 2;
	    while (*q != ',' && *q != '\0')
	    {
		if (*q == 'r')
		    can_read = 1;
		else if (*q == 'w')
		    can_write = 1;
		else if (*q == 'x')
		    can_execute = 1;
		++q;
	    }
	    if (p[0] == 'u')
	    {
		if (can_read)
		    mode |= S_IRUSR;
		if (can_write)
		    mode |= S_IWUSR;
		if (can_execute)
		    mode |= S_IXUSR;
	    }
	    else if (p[0] == 'g')
	    {
		if (can_read)
		    mode |= S_IRGRP;
		if (can_write)
		    mode |= S_IWGRP;
		if (can_execute)
		    mode |= S_IXGRP;
	    }
	    else if (p[0] == 'o')
	    {
		if (can_read)
		    mode |= S_IROTH;
		if (can_write)
		    mode |= S_IWOTH;
		if (can_execute)
		    mode |= S_IXOTH;
	    }
	}
	/* Skip to the next field.  */
	while (*p != ',' && *p != '\0')
	    ++p;
	if (*p == ',')
	    ++p;
    }

    if (respect_umask)
    {
	oumask = umask (0);
	(void) umask (oumask);
	mode &= ~oumask;
    }

    if (chmod (filename, mode) < 0)
	return errno;
    return 0;
#endif /* ! CHMOD_BROKEN */
}
#endif /* CLIENT_SUPPORT or SERVER_SUPPORT */



#ifdef CLIENT_SUPPORT
int client_prune_dirs;

static List *ignlist = NULL;

/* Buffer to write to the server.  */
static struct buffer *global_to_server;

/* Buffer used to read from the server.  */
static struct buffer *global_from_server;



/*
 * Read a line from the server.  Result does not include the terminating \n.
 *
 * Space for the result is malloc'd and should be freed by the caller.
 *
 * Returns number of bytes read.
 */
static size_t
read_line_via (struct buffer *via_from_buffer, struct buffer *via_to_buffer,
               char **resultp)
{
    int status;
    char *result;
    size_t len;

    status = buf_flush (via_to_buffer, 1);
    if (status != 0)
	error (1, status, "writing to server");

    status = buf_read_line (via_from_buffer, &result, &len);
    if (status != 0)
    {
	if (status == -1)
	    error (1, 0,
                   "end of file from server (consult above messages if any)");
	else if (status == -2)
	    error (1, 0, "out of memory");
	else
	    error (1, status, "reading from server");
    }

    if (resultp)
	*resultp = result;
    else
	free (result);

    return len;
}



static size_t
read_line (char **resultp)
{
  return read_line_via (global_from_server, global_to_server, resultp);
}
#endif /* CLIENT_SUPPORT */



#if defined(CLIENT_SUPPORT) || defined(SERVER_SUPPORT)
/*
 * Level of compression to use when running gzip on a single file.
 */
int file_gzip_level;

#endif /* CLIENT_SUPPORT or SERVER_SUPPORT */

#ifdef CLIENT_SUPPORT

/* Whether the server asked us to force compression.  */
static bool force_gzip;

/*
 * The Repository for the top level of this command (not necessarily
 * the CVSROOT, just the current directory at the time we do it).
 */
static char *toplevel_repos;

/* Working directory when we first started.  Note: we could speed things
   up on some systems by using savecwd.h here instead of just always
   storing a name.  */
char *toplevel_wd;



static void
handle_ok (char *args, size_t len)
{
    return;
}



static void
handle_error (char *args, size_t len)
{
    int something_printed;
    
    /*
     * First there is a symbolic error code followed by a space, which
     * we ignore.
     */
    char *p = strchr (args, ' ');
    if (!p)
    {
	error (0, 0, "invalid data from cvs server");
	return;
    }
    ++p;

    /* Next we print the text of the message from the server.  We
       probably should be prefixing it with "server error" or some
       such, because if it is something like "Out of memory", the
       current behavior doesn't say which machine is out of
       memory.  */

    len -= p - args;
    something_printed = 0;
    for (; len > 0; --len)
    {
	something_printed = 1;
	putc (*p++, stderr);
    }
    if (something_printed)
	putc ('\n', stderr);
}



static void
handle_valid_requests (char *args, size_t len)
{
    char *p = args;
    char *q;
    struct request *rq;
    do
    {
	q = strchr (p, ' ');
	if (q)
	    *q++ = '\0';
	for (rq = requests; rq->name; ++rq)
	{
	    if (STREQ (rq->name, p))
		break;
	}
	if (!rq->name)
	    /*
	     * It is a request we have never heard of (and thus never
	     * will want to use).  So don't worry about it.
	     */
	    ;
	else
	{
	    if (rq->flags & RQ_ENABLEME)
	    {
		/*
		 * Server wants to know if we have this, to enable the
		 * feature.
		 */
		send_to_server (rq->name, 0);
                send_to_server ("\012", 0);
	    }
	    else
		rq->flags |= RQ_SUPPORTED;
	}
	p = q;
    } while (q);
    for (rq = requests; rq->name; ++rq)
    {
	if ((rq->flags & RQ_SUPPORTED)
	    || (rq->flags & RQ_ENABLEME))
	    continue;
	if (rq->flags & RQ_ESSENTIAL)
	    error (1, 0, "request `%s' not supported by server", rq->name);
    }
}

static void
handle_force_gzip (char *args, size_t len)
{
    force_gzip = true;
}



/* Has the server told us its name since the last redirect?
 */
static bool referred_since_last_redirect = false;
static bool free_client_referrer = false;



static void
handle_referrer (char *args, size_t len)
{
    TRACE (TRACE_FUNCTION, "handle_referrer (%s)", args);
    client_referrer = parse_cvsroot (args);
    referred_since_last_redirect = true;
    free_client_referrer = true;
}



/* Redirect our connection to a different server and start over.
 *
 * GLOBALS
 *   current_parsed_root	The CVSROOT being accessed.
 *   client_referrer		Used to track the server which referred us to a
 *				new server.  Can be supplied by the referring
 *				server.
 *   free_client_referrer	Used to track whether the client_referrer needs
 *				to be freed before changing it.
 *   referred_since_last_redirect	
 *				Tracks whether the currect server told us how
 *				to refer to it.
 *
 * OUTPUTS
 *   current_parsed_root	Updated to point to the new CVSROOT.
 *   referred_since_last_redirect
 *				Always cleared.
 *   client_referrer		Set automatically to current_parsed_root if
 *				the current server did not give us a name to
 *				refer to it by.
 *   free_client_referrer	Reset when necessary.
 */
static void
handle_redirect (char *args, size_t len)
{
    static List *redirects = NULL;

    TRACE (TRACE_FUNCTION, "handle_redirect (%s)", args);

    if (redirects && findnode (redirects, args))
	error (1, 0, "`Redirect' loop detected.  Server misconfiguration?");
    else
    {
	if (!redirects) redirects = getlist();
	push_string (redirects, xstrdup (args));
    }

    if (referred_since_last_redirect)
	referred_since_last_redirect = false;
    else
    {
	if (free_client_referrer) free (client_referrer);
	client_referrer = current_parsed_root;
	free_client_referrer = false;
    }

    current_parsed_root = parse_cvsroot (args);

    /* We deliberately do not set ORIGINAL_PARSED_ROOT here.
     * ORIGINAL_PARSED_ROOT is used by the client to determine the current root
     * being processed for the purpose of looking it up in lists and such, even
     * after a redirect.
     *
     * FIXME
     *   CURRENT_PARSED_ROOT should not be reset by this function.  Redirects
     *   should be "added" to it.  The REDIRECTS list should also be replaced
     *   by this new CURRENT_PARSED_ROOT element.  This way, if, for instance,
     *   a multi-root workspace had two secondaries pointing to the same
     *   primary, then the client would not report a looping error.
     *
     *   There is also a potential memory leak above and storing new roots as
     *   part of the original could help avoid it fairly elegantly.
     */
    if (!current_parsed_root)
	error (1, 0, "Server requested redirect to invalid root: `%s'",
	       args);
}



/*
 * This is a proc for walklist().  It inverts the error return premise of
 * walklist.
 *
 * RETURNS
 *   True       If this path is prefixed by one of the paths in walklist and
 *              does not step above the prefix path.
 *   False      Otherwise.
 */
static
int path_list_prefixed (Node *p, void *closure)
{
    const char *questionable = closure;
    const char *prefix = p->key;
    if (!STRNEQ (prefix, questionable, strlen (prefix))) return 0;
    questionable += strlen (prefix);
    while (ISSLASH (*questionable)) questionable++;
    if (*questionable == '\0') return 1;
    return pathname_levels (questionable);
}



/*
 * Need to validate the client pathname.  Disallowed paths include:
 *
 *   1. Absolute paths.
 *   2. Pathnames that do not reference a specifically requested update
 *      directory.
 *
 * In case 2, we actually only check that the directory is under the uppermost
 * directories mentioned on the command line.
 *
 * RETURNS
 *   True       If the path is valid.
 *   False      Otherwise.
 */
static
int is_valid_client_path (const char *pathname)
{
    /* 1. Absolute paths. */
    if (ISABSOLUTE (pathname)) return 0;
    /* 2. No up-references in path.  */
    if (pathname_levels (pathname) == 0) return 1;
    /* 2. No Max-dotdot paths registered.  */
    if (!uppaths) return 0;

    return walklist (uppaths, path_list_prefixed, (void *)pathname);
}



/*
 * Do all the processing for PATHNAME, where pathname consists of the
 * repository and the filename.  When this function is called, it is expected
 * that the REPOSITORY line is still available to be read from the server
 * (an error will be reported if the server failed to send it).
 *
 * CALLBACK FUNCTION
 *
 *   The parameters we pass to FUNC are:
 *
 *     data	the DATA parameter which was passed to call_in_directory().
 *     finfo	a complete struct file_info specifying the entries list for
 *		this directory, as well as the rest of the information
 *		required to specify the file or directory to be operated on
 *		locally and locate it in the repository.
 *
 *   When FUNC is called, the current directory will be the directory
 *   containing the file or directory specified by FINFO.
 */
    /*
     * Do the whole descent in parallel for the repositories, so we
     * know what to put in CVS/Repository files.  I'm not sure the
     * full hair is necessary since the server does a similar
     * computation; I suspect that we only end up creating one
     * directory at a time anyway.
     *
     * Also note that we must *only* worry about this stuff when we
     * are creating directories; `cvs co foo/bar; cd foo/bar; cvs co
     * CVSROOT; cvs update' is legitimate, but in this case
     * foo/bar/CVSROOT/CVS/Repository is not a subdirectory of
     * foo/bar/CVS/Repository.
     */
static void
call_in_directory (const char *pathname,
                   void (*func) (void *data, const struct file_info *finfo),
                   void *data)
{
    /* This variable holds the result of Entries_Open. */
    List *last_entries = NULL;
    char *dir, *bdir, *pdir;

    /* The name of the file or directory to operate on.  */
    char *filename;
    char *fullname;		/* Becomes FINFO->fullname  */
    const char *update_dir;
    /* The repository line as sent by the server.  */
    char *server_repos;
    const char *repository;	/* The repository line as stored locally.  */
    bool newdir;
    struct file_info finfo;

    assert (pathname);
    assert (toplevel_repos);

    server_repos = NULL;
    read_line (&server_repos);
    if (!server_repos)
	error (1, 0, "server failed to specify repository in %s.",
	       quote (NULL2DOT (pathname)));

    TRACE (TRACE_FLOW, "call_in_directory (%s, %s, %s)",
	   pathname, server_repos, toplevel_repos);

    /* Why do we do this?  I know old servers used to send absolute repository
     * lines, but I thought newer ones always sent a line relative to
     * CURRENT_PARSED_ROOT->directory, not TOPLEVEL_REPOS.
     */
    if (STRNEQ (server_repos, toplevel_repos, strlen (toplevel_repos))
	&& ISSLASH (server_repos[strlen (toplevel_repos)]))
	repository = server_repos + strlen (toplevel_repos) + 1;
    else
	repository = server_repos;

    /* Now that we have SHORT_REPOS, we can calculate the path to the file or
     * directory are being requested to operate on.  We can't just use
     * base_name() here, because it does the wrong thing with directories (sent
     * by the server as PATH/) - it returns base_name (PATH) instead of the
     * empty string.
     */
    if (*repository && ISSLASH (repository[strlen(repository) - 1]))
	filename = xstrdup ("");
    else
	filename = base_name (repository);
    fullname = dir_append (pathname, filename);

    /* Now that we know the path to the file we were requested to operate on,
     * we can verify that it is valid.
     *
     * For security reasons, if SHORT_PATHNAME is absolute or attempts to
     * ascend outside of the current sanbbox, we abort.  The server should not
     * send us anything but relative paths which remain inside the sandbox
     * here.  Anything less means a trojan CVS server could create and edit
     * arbitrary files on the client.
     */
    if (!is_valid_client_path (fullname))
	error (1, 0,
               "Server attempted to update a file via invalid pathname %s.",
	       quote (fullname));

    if (!*pathname
	/* or PATHNAME == "./" */
	|| (pathname[0] == '.' && ISSLASH (pathname[1]) && !pathname[2]))
    {
	/* Old servers always send a trailing '/' on PATHNAME and normalize an
	 * UPDATE_DIR of "" or "." to "./".  We have to assume something in
	 * that case, so assume an empty UPDATE_DIR since it is the more common
	 * case.
	 *
	 * See the comment above output_dir() in server.c for more info on how
	 * UPDATE_DIR is now preserved.
	 */
	dir = xstrdup (".");
	update_dir = "";
    }
    else
    {
	dir = xstrdup (pathname);
	update_dir = dir;
    }

    if (client_prune_dirs)
	add_prune_candidate (dir);

    if (!toplevel_wd)
    {
	toplevel_wd = xgetcwd();
	if (!toplevel_wd)
	    error (1, errno, "could not get working directory");
    }
    else if (CVS_CHDIR (toplevel_wd) < 0)
	error (1, errno, "could not chdir to %s", toplevel_wd);

    pdir = dir_name (dir);
    if (!STREQ (pdir, ".") /* Make an exception for the top level directory.  */
	&& !(STREQ (cvs_cmd_name, "export") ? isdir (pdir) : hasAdmin (pdir)))
	error (1, 0, "cannot create directory %s outside working directory",
	       quote (dir));

    bdir = base_name (dir);
    if (!fncmp (bdir, CVSADM))
    {
	error (0, 0, "cannot create a directory named %s", quote (dir));
	error (0, 0, "because CVS uses %s for its own purposes",
	       quote (CVSADM));
	error (1, 0, "rename the directory and try again");
    }

    /* Make sure this directory exists.  */
    newdir = cvs_xmkdir (dir, NULL, MD_EXIST_OK);

    /* Don't create CVSADM directories if this is export.  */
    if (!STREQ (cvs_cmd_name, "export") && !hasAdmin (dir))
    {
	Create_Admin (dir, update_dir, server_repos, NULL, NULL, 0,
		      !newdir, /* Only warn about failures unless we just
				* created this directory.
				*/
		      1);
	Subdir_Register (NULL, pdir, bdir);
    }

    if (CVS_CHDIR (dir) < 0)
	error (1, errno, "could not chdir to %s", dir);

    if (!STREQ (cvs_cmd_name, "export"))
    {
	last_entries = Entries_Open (0, update_dir);

	/* If this is a newly created directory, we will record
	 * all subdirectory information, so call Subdirs_Known in
	 * case there are no subdirectories.  If this is not a
	 * newly created directory, it may be an old working
	 * directory from before we recorded subdirectory
	 * information in the Entries file.  We force a search for
	 * all subdirectories now, to make sure our subdirectory
	 * information is up to date.  If the Entries file does
	 * record subdirectory information, then this call only
	 * does list manipulation.
	 */

	if (newdir)
	    Subdirs_Known (last_entries);
	else if (!entriesHasAllSubdirs (last_entries))
	{
	    List *dirlist;
	    dirlist = Find_Directories (NULL, update_dir, W_LOCAL,
					last_entries);
	    dellist (&dirlist);
	}
    }

    finfo.update_dir = pathname;
    finfo.file = filename;
    finfo.fullname = fullname;
    finfo.repository = repository;
    finfo.entries = last_entries;

    (*func) (data, &finfo);
    if (last_entries)
	Entries_Close (last_entries, update_dir);
    free (dir);
    free (bdir);
    free (pdir);
    free (filename);
    free (fullname);
    free (server_repos);
}



static void
copy_a_file (void *data, const struct file_info *finfo)
{
    char *newname;

    read_line (&newname);

#ifdef USE_VMS_FILENAMES
    {
	/* Mogrify the filename so VMS is happy with it. */
	char *p;
	for(p = newname; *p; p++)
	   if(*p == '.' || *p == '#') *p = '_';
    }
#endif
    /* cvsclient.texi has said for a long time that newname must be in the
       same directory.  Wouldn't want a malicious or buggy server overwriting
       ~/.profile, /etc/passwd, or anything like that.  */
    if (last_component (newname) != newname)
	error (1, 0,
	       "protocol error: Copy-file tried to specify directory (%s)",
	       quote (newname));

    if (unlink_file (newname) && !existence_error (errno))
	error (0, errno, "unable to remove %s", quote (newname));
    copy_file (finfo->file, newname);
    free (newname);
}



static void
handle_copy_file (char *args, size_t len)
{
    call_in_directory (args, copy_a_file, NULL);
}



/* Attempt to read a file size from a string.  Accepts base 8 (0N), base 16
 * (0xN), or base 10.  Exits on error.
 *
 * RETURNS
 *   The file size, in a size_t.
 *
 * FATAL ERRORS
 *   1.  As strtoumax().
 *   2.  If the number read exceeds SIZE_MAX.
 */
static size_t
strto_file_size (const char *s)
{
    uintmax_t tmp;
    char *endptr;

    /* Read it.  */
    errno = 0;
    tmp = strtoumax (s, &endptr, 0);

    /* Check for errors.  */
    if (errno || endptr == s)
	error (1, errno, "Unable to parse file size sent by server, `%s'", s);
    if (*endptr != '\0')
	error (1, 0,
	       "Server sent trailing characters in file size, `%s'",
	       endptr);
    if (tmp > SIZE_MAX)
	error (1, 0, "Server sent file size exceeding client maximum.");

    /* Return it.  */
    return (size_t)tmp;
}



/* Read from the server the count for the length of a file, then read
 * the contents of that file and write them to FILENAME.  FULLNAME is
 * the name of the file for use in error messages.
 */
static void
read_counted_file (char *filename, char *fullname)
{
    char *size_string;
    size_t size;
    char *buf;

    /* Pointers in buf to the place to put data which will be read,
       and the data which needs to be written, respectively.  */
    char *pread;
    char *pwrite;
    /* Number of bytes left to read and number of bytes in buf waiting to
       be written, respectively.  */
    size_t nread;
    size_t nwrite;

    FILE *fp;

    read_line (&size_string);
    if (size_string[0] == 'z')
	error (1, 0, "\
protocol error: compressed files not supported for that operation");
    size = strto_file_size (size_string);
    free (size_string);

    /* A more sophisticated implementation would use only a limited amount
       of buffer space (8K perhaps), and read that much at a time.  We allocate
       a buffer for the whole file only to make it easy to keep track what
       needs to be read and written.  */
    buf = xmalloc (size);

    /* FIXME-someday: caller should pass in a flag saying whether it
       is binary or not.  I haven't carefully looked into whether
       CVS/Template files should use local text file conventions or
       not.  */
    fp = CVS_FOPEN (filename, "wb");
    if (!fp)
	error (1, errno, "cannot write %s", fullname);
    nread = size;
    nwrite = 0;
    pread = buf;
    pwrite = buf;
    while (nread > 0 || nwrite > 0)
    {
	size_t n;

	if (nread > 0)
	{
	    n = try_read_from_server (pread, nread);
	    nread -= n;
	    pread += n;
	    nwrite += n;
	}

	if (nwrite > 0)
	{
	    n = fwrite (pwrite, sizeof *pwrite, nwrite, fp);
	    if (ferror (fp))
		error (1, errno, "cannot write %s", fullname);
	    nwrite -= n;
	    pwrite += n;
	}
    }
    free (buf);
    if (fclose (fp) < 0)
	error (1, errno, "cannot close %s", fullname);
}



/* OK, we want to swallow the "U foo.c" response and then output it only
   if we can update the file.  In the future we probably want some more
   systematic approach to parsing tagged text, but for now we keep it
   ad hoc.  "Why," I hear you cry, "do we not just look at the
   Update-existing and Created responses?"  That is an excellent question,
   and the answer is roughly conservatism/laziness--I haven't read through
   update.c enough to figure out the exact correspondence or lack thereof
   between those responses and a "U foo.c" line (note that Merged, from
   join_file, can be either "C foo" or "U foo" depending on the context).  */
/* Nonzero if we have seen +updated and not -updated.  */
static int updated_seen;
/* Filename from an "fname" tagged response within +updated/-updated.  */
static char *updated_fname;

/* This struct is used to hold data when reading the +importmergecmd
   and -importmergecmd tags.  We put the variables in a struct only
   for namespace issues.  FIXME: As noted above, we need to develop a
   more systematic approach.  */
static struct
{
    /* Nonzero if we have seen +importmergecmd and not -importmergecmd.  */
    int seen;
    /* Number of conflicts, from a "conflicts" tagged response.  */
    int conflicts;
    /* First merge tag, from a "mergetag1" tagged response.  */
    char *mergetag1;
    /* Second merge tag, from a "mergetag2" tagged response.  */
    char *mergetag2;
    /* Repository, from a "repository" tagged response.  */
    char *repository;
} importmergecmd;

/* Nonzero if we should arrange to return with a failure exit status.  */
static bool failure_exit;


/*
 * The time stamp of the last file we registered.
 */
static time_t last_register_time;



/*
 * The Checksum response gives the checksum for the file transferred
 * over by the next Updated, Merged or Patch response.  We just store
 * it here, and then check it in update_entries.
 */
static int stored_checksum_valid;
static checksum_t stored_ck;	/* sixteen bytes for MD5 checksum */
static void
handle_checksum (char *args, size_t len)
{
    char *s;
    char buf[3];
    int i;

    if (stored_checksum_valid)
        error (1, 0, "Checksum received before last one was used");

    s = args;
    buf[2] = '\0';
    for (i = 0; i < 16; i++)
    {
        char *bufend;

	buf[0] = *s++;
	buf[1] = *s++;
	stored_ck.char_checksum[i] = (unsigned char) strtol (buf, &bufend, 16);
	if (bufend != buf + 2)
	    break;
    }

    if (i < 16 || *s != '\0')
        error (1, 0, "Invalid Checksum response: `%s'", args);

    stored_checksum_valid = 1;
}



/* Mode that we got in a "Mode" response (malloc'd), or NULL if none.  */
static char *stored_mode;
static void
handle_mode (char *args, size_t len)
{
    if (stored_mode)
	error (1, 0, "protocol error: duplicate Mode");
    stored_mode = xstrdup (args);
}



/* Nonzero if time was specified in Mod-time.  */
static int stored_modtime_valid;
/* Time specified in Mod-time.  */
static time_t stored_modtime;
static void
handle_mod_time (char *args, size_t len)
{
    struct timespec newtime;
    if (stored_modtime_valid)
	error (0, 0, "protocol error: duplicate Mod-time");
    if (get_date (&newtime, args, NULL))
    {
	/* Truncate nanoseconds.  */
	stored_modtime = newtime.tv_sec;
	stored_modtime_valid = 1;
    }
    else
	error (0, 0, "protocol error: cannot parse date %s", args);
}



/*
 * If we receive a patch, but the patch program fails to apply it, we
 * want to request the original file.  We keep a list of files whose
 * patches have failed.
 */

char **failed_patches;
int failed_patches_count;



struct update_entries_data
{
    enum {
      /*
       * We are just getting an Entries line; the local file is
       * correct.
       */
      UPDATE_ENTRIES_CHECKIN,

      /* The file content may be in a temp file, waiting to be renamed.  */
      UPDATE_ENTRIES_BASE,

      /* We are getting the file contents as well.  */
      UPDATE_ENTRIES_UPDATE,
      /*
       * We are getting a patch against the existing local file, not
       * an entire new file.
       */
      UPDATE_ENTRIES_PATCH,
      /*
       * We are getting an RCS change text (diff -n output) against
       * the existing local file, not an entire new file.
       */
      UPDATE_ENTRIES_RCS_DIFF
    } contents;

    enum update_existing existp;

    /*
     * String to put in the timestamp field or NULL to use the timestamp
     * of the file.
     */
    char *timestamp;
};



static void
discard_file (void)
{
    char *mode_string;
    char *size_string;
    size_t size, nread;

    read_line (&mode_string);
    free (mode_string);

    read_line (&size_string);
    if (size_string[0] == 'z')
	size = atoi (size_string + 1);
    else
	size = atoi (size_string);
    free (size_string);

    /* Now read and discard the file contents.  */
    nread = 0;
    while (nread < size)
    {
	char buf[8192];
	size_t toread;

	toread = size - nread;
	if (toread > sizeof buf)
	    toread = sizeof buf;

	nread += try_read_from_server (buf, toread);
	if (nread == size)
	    break;
    }

    return;
}



static char *
newfilename (const char *filename)
{
#ifdef USE_VMS_FILENAMES
    /* A VMS rename of "blah.dat" to "foo" to implies a
     * destination of "foo.dat" which is unfortinate for CVS.
     */
    return Xasprintf ("%s_new_", filename);
#else
#ifdef _POSIX_NO_TRUNC
    return Xasprintf (".new.%.9s", filename);
#else /* _POSIX_NO_TRUNC */
    return Xasprintf (".new.%s", filename);
#endif /* _POSIX_NO_TRUNC */
#endif /* USE_VMS_FILENAMES */
}



static char *
read_file_from_server (const char *fullname, char **mode_string, size_t *size)
{
    char *size_string;
    bool use_gzip;
    char *buf;
    char *s;

    read_line (mode_string);
    
    read_line (&size_string);
    if (size_string[0] == 'z')
    {
	use_gzip = true;
	s = size_string + 1;
    }
    else
    {
	use_gzip = false;
	s = size_string;
    }
    *size = strto_file_size (s);
    free (size_string);

    buf = xmalloc (*size);
    read_from_server (buf, *size);

    if (use_gzip)
    {
	char *outbuf;

	if (gunzip_in_mem (fullname, (unsigned char *) buf, size, &outbuf))
	    error (1, 0, "aborting due to compression error");

	free (buf);
	buf = outbuf;
    }

    return buf;
}



/* Cache for OpenPGP signatures so they may be written to a file only on a
 * successful commit.
 */
static List *sig_cache;



/* Update the Entries line for this file.  */
static void
update_entries (void *data_arg, const struct file_info *finfo)
{
    char *entries_line;
    struct update_entries_data *data = data_arg;

    char *cp;
    char *user;
    char *vn;
    /* Timestamp field.  Always empty according to the protocol.  */
    char *ts;
    char *options = NULL;
    char *tag = NULL;
    char *date = NULL;
    char *tag_or_date;
    char *scratch_entries = NULL;
    bool bin;
    char *temp_filename;

#ifdef UTIME_EXPECTS_WRITABLE
    int change_it_back = 0;
#endif

    TRACE (TRACE_FUNCTION, "update_entries (%s)", finfo->fullname);

    read_line (&entries_line);

    /*
     * Parse the entries line.
     */
    scratch_entries = xstrdup (entries_line);

    if (scratch_entries[0] != '/')
        error (1, 0, "bad entries line %s from server", quote (entries_line));
    user = scratch_entries + 1;
    if (!(cp = strchr (user, '/')))
        error (1, 0, "bad entries line %s from server", quote (entries_line));
    *cp++ = '\0';
    vn = cp;
    if (!(cp = strchr (vn, '/')))
        error (1, 0, "bad entries line %s from server", quote (entries_line));
    *cp++ = '\0';
    
    ts = cp;
    if (!(cp = strchr (ts, '/')))
        error (1, 0, "bad entries line %s from server", quote (entries_line));
    *cp++ = '\0';
    options = cp;
    if (!(cp = strchr (options, '/')))
        error (1, 0, "bad entries line %s from server", quote (entries_line));
    *cp++ = '\0';
    tag_or_date = cp;
    
    /* If a slash ends the tag_or_date, ignore everything after it.  */
    cp = strchr (tag_or_date, '/');
    if (cp)
        *cp = '\0';
    if (*tag_or_date == 'T')
        tag = tag_or_date + 1;
    else if (*tag_or_date == 'D')
        date = tag_or_date + 1;

    /* Done parsing the entries line. */

    temp_filename = newfilename (finfo->file);

    if (data->contents == UPDATE_ENTRIES_UPDATE
	|| data->contents == UPDATE_ENTRIES_PATCH
	|| data->contents == UPDATE_ENTRIES_RCS_DIFF)
    {
	char *mode_string;
	size_t size;
	char *buf;
	bool patch_failed;

	if (get_verify_checkouts (true) && !STREQ (cvs_cmd_name, "export"))
	    error (get_verify_checkouts_fatal (), 0,
		   "No signature for %s.", quote (finfo->fullname));

	if (!validate_change (data->existp, finfo))
	{
	    /* The Mode, Mod-time, and Checksum responses should not carry
	     * over to a subsequent Created (or whatever) response, even
	     * in the error case.
	     */
	    if (updated_fname)
	    {
		free (updated_fname);
		updated_fname = NULL;
	    }
	    if (stored_mode)
	    {
		free (stored_mode);
		stored_mode = NULL;
	    }
	    stored_modtime_valid = 0;
	    stored_checksum_valid = 0;

	    failure_exit = true;

	discard_file_and_return:
	    discard_file ();
	    free (scratch_entries);
	    free (entries_line);
	    return;
	}

	buf = read_file_from_server (finfo->fullname, &mode_string, &size);

        /* Some systems, like OS/2 and Windows NT, end lines with CRLF
           instead of just LF.  Format translation is done in the C
           library I/O funtions.  Here we tell them whether or not to
           convert -- if this file is marked "binary" with the RCS -kb
           flag, then we don't want to convert, else we do (because
           CVS assumes text files by default). */
	if (options)
	    bin = STREQ (options, "-kb");
	else
	    bin = false;

	if (data->contents != UPDATE_ENTRIES_RCS_DIFF)
	{
	    int fd;

	    fd = CVS_OPEN (temp_filename,
			   (O_WRONLY | O_CREAT | O_TRUNC
			    | (bin ? OPEN_BINARY : 0)),
			   0777);

	    if (fd < 0)
	    {
		/* I can see a case for making this a fatal error; for
		   a condition like disk full or network unreachable
		   (for a file server), carrying on and giving an
		   error on each file seems unnecessary.  But if it is
		   a permission problem, or some such, then it is
		   entirely possible that future files will not have
		   the same problem.  */
		error (0, errno, "cannot write %s", quote (finfo->fullname));
		free (temp_filename);
		free (buf);
		goto discard_file_and_return;
	    }

	    if (write (fd, buf, size) != size)
		error (1, errno, "writing %s", quote (finfo->fullname));

	    if (close (fd) < 0)
		error (1, errno, "writing %s", quote (finfo->fullname));
	}

	patch_failed = false;

	if (data->contents == UPDATE_ENTRIES_UPDATE)
	{
	    rename_file (temp_filename, finfo->file);
	}
	else if (data->contents == UPDATE_ENTRIES_PATCH)
	{
	    /* You might think we could just leave Patched out of
	       Valid-responses and not get this response.  However, if
	       memory serves, the CVS 1.9 server bases this on -u
	       (update-patches), and there is no way for us to send -u
	       or not based on whether the server supports "Rcs-diff".  

	       Fall back to transmitting entire files.  */
	    error (0, 0,
		   "unsupported patch format received for %s; will refetch",
		   quote (finfo->fullname));
	    patch_failed = true;
	}
	else
	{
	    char *filebuf;
	    size_t filebufsize;
	    size_t nread;
	    char *patchedbuf;
	    size_t patchedlen;

	    /* Handle UPDATE_ENTRIES_RCS_DIFF.  */

	    if (!isfile (finfo->file))
	        error (1, 0, "patch original file %s does not exist",
		       quote (finfo->fullname));
	    filebuf = NULL;
	    filebufsize = 0;
	    nread = 0;

	    get_file (finfo->file, finfo->fullname,
		      bin ? FOPEN_BINARY_READ : "r",
		      &filebuf, &filebufsize, &nread);
	    /* At this point the contents of the existing file are in
               FILEBUF, and the length of the contents is in NREAD.
               The contents of the patch from the network are in BUF,
               and the length of the patch is in SIZE.  */

	    if (!rcs_change_text (finfo->fullname, filebuf, nread, buf, size,
				   &patchedbuf, &patchedlen))
	    {
		error (0, 0, "patch failed for %s; will refetch",
		       quote (finfo->fullname));
		patch_failed = true;
	    }
	    else
	    {
		if (stored_checksum_valid)
		{
		    checksum_t ck;

		    /* We have a checksum.  Check it before writing
		       the file out, so that we don't have to read it
		       back in again.  */
		    md5_buffer (patchedbuf, patchedlen, ck.char_checksum);
		    if (memcmp (ck.char_checksum, stored_ck.char_checksum, 16) != 0)
		    {
			error (0, 0,
"checksum failure after patch to %s; will refetch",
			       quote (finfo->fullname));

			patch_failed = true;
		    }

		    stored_checksum_valid = 0;
		}

		if (!patch_failed)
		{
		    FILE *e;

		    e = xfopen (temp_filename,
				bin ? FOPEN_BINARY_WRITE : "w");
		    if (fwrite (patchedbuf, sizeof *patchedbuf, patchedlen, e)
			!= patchedlen)
			error (1, errno, "cannot write %s", temp_filename);
		    if (fclose (e) == EOF)
			error (1, errno, "cannot close %s", temp_filename);
		    rename_file (temp_filename, finfo->file);
		}

		free (patchedbuf);
	    }

	    free (filebuf);
	}

	free (buf);
	free (temp_filename);

	if (stored_checksum_valid && !patch_failed)
	{
	    FILE *e;
	    struct md5_ctx context;
	    unsigned char buf[8192];
	    unsigned len;
	    checksum_t ck;

	    /*
	     * Compute the MD5 checksum.  This will normally only be
	     * used when receiving a patch, so we always compute it
	     * here on the final file, rather than on the received
	     * data.
	     *
	     * Note that if the file is a text file, we should read it
	     * here using text mode, so its lines will be terminated the same
	     * way they were transmitted.
	     */
	    e = CVS_FOPEN (finfo->file, "r");
	    if (!e)
	        error (1, errno, "could not open %s", quote (finfo->fullname));

	    md5_init_ctx (&context);
	    while ((len = fread (buf, 1, sizeof buf, e)) != 0)
		md5_process_bytes (buf, len, &context);
	    if (ferror (e))
		error (1, errno, "could not read %s", quote (finfo->fullname));
	    md5_finish_ctx (&context, ck.char_checksum);

	    fclose (e);

	    stored_checksum_valid = 0;

	    if (memcmp (ck.char_checksum, stored_ck.char_checksum, 16) != 0)
	    {
	        if (data->contents != UPDATE_ENTRIES_PATCH)
		    error (1, 0, "checksum failure on %s",
			   quote (finfo->fullname));

		error (0, 0,
		       "checksum failure after patch to %s; will refetch",
		       quote (finfo->fullname));

		patch_failed = true;
	    }
	}

	if (patch_failed)
	{
	    /* Save this file to retrieve later.  */
	    failed_patches = xnrealloc (failed_patches,
					failed_patches_count + 1,
					sizeof (char *));
	    failed_patches[failed_patches_count] = xstrdup (finfo->fullname);
	    ++failed_patches_count;

	    stored_checksum_valid = 0;

	    free (mode_string);
	    free (scratch_entries);
	    free (entries_line);

	    if (updated_fname)
	    {
		free (updated_fname);
		updated_fname = NULL;
	    }

	    return;
	}
	else if (updated_fname)
	{
	    cvs_output ("U ", 0);
	    cvs_output (updated_fname, 0);
	    cvs_output ("\n", 1);
	    free (updated_fname);
	    updated_fname = NULL;
	}

        {
	    int status = change_mode (finfo->file, mode_string, 1);
	    if (status != 0)
		error (0, status, "cannot change mode of %s",
		       quote (finfo->fullname));
	}

	free (mode_string);
    }
    else if (data->contents == UPDATE_ENTRIES_BASE)
    {
	Node *n;
	if (!noexec && (n = findnode_fn (finfo->entries, finfo->file)))
	{
	    Entnode *e = n->data;
	    /* After a join, control can get here without having changed the
	     * version number.  In this case, do not remove the base file.
	     */
	    if (!STREQ (vn, e->version))
		base_remove (finfo->file, e->version);
	}

	if (last_merge)
	{
	    /* Won't need these now that the merge is complete.  */
	    if (!STREQ (vn, base_merge_rev1))
		base_remove (finfo->file, base_merge_rev1);
	    free (base_merge_rev1);
	    if (!STREQ (vn, base_merge_rev2))
		base_remove (finfo->file, base_merge_rev2);
	    free (base_merge_rev2);
	}

	if (base_copy_error)
	{
	    /* The previous base_copy command returned an error, such as in the
	     * "move away `FILE'; it is in the way" case.  Do not allow the
	     * entry to be updated.
	     */
	    if (updated_fname)
	    {
		/* validate_change() has already printed "C filename" via the
		 * call from client_base_copy().
		 */
		free (updated_fname);
		updated_fname = false;
	    }
	    base_copy_error = false;
	    return;
	}
	if (!noexec)
	    rename_file (temp_filename, finfo->file);
	if (updated_fname)
	{
	    cvs_output ("U ", 0);
	    cvs_output (updated_fname, 0);
	    cvs_output ("\n", 1);
	    free (updated_fname);
	    updated_fname = NULL;
	}
    }
    else if (data->contents == UPDATE_ENTRIES_CHECKIN
	     && !noexec
	     /* This isn't add or remove.  */
	     && !STREQ (vn, "0") && *vn != '-')
    {
	/* On checkin, create the base file.  */
	Node *n;
	bool makebase = true;

	if ((n = findnode_fn (finfo->entries, finfo->file)))
	{
	    /* This could be a readd of a locally removed file or, for
	     * instance, an update that changed keyword options without
	     * changing the revision number or the base file.
	     */
	    Entnode *e = n->data;
	    if (!STREQ (vn, e->version))
		/* The version number has changed.  */
		base_remove (finfo->file, e->version);
	    else
		/* The version number has not changed.  */
		makebase = false;
	}

	if (makebase)
	{
	    /* A real checkin.  */
	    char *basefile = make_base_file_name (finfo->file, vn);

	    cvs_xmkdir (CVSADM_BASE, NULL, MD_EXIST_OK);
	    copy_file (finfo->file, basefile);

	    if ((n = findnode_fn (sig_cache, finfo->fullname)))
	    {
		char *sigfile = Xasprintf ("%s%s", basefile, ".sig");
		write_file (sigfile, n->data, n->len);
		delnode (n);
		free (sigfile);
	    }
	    else if (get_sign_commits (supported_request ("Signature")))
		error (0, 0,
"Internal error: OpenPGP signature for %s not found in cache.",
		       quote (finfo->fullname));
	    free (basefile);
	}
    }
    else if (data->contents != UPDATE_ENTRIES_CHECKIN)
	/* This error is important.  It makes sure that all three cases which
	 * write files are caught by the openpgp2 set of tests when the user
	 * has requested that failed checkout verification is fatal and the
	 * server attempts to bypass signatures by sending old-style responses
	 * which do not support signatures.  (The `Checkin' response does not
	 * count since it does not accept any file data from the server and is
	 * used in both modes.)
	 */
	error (1, 0, "internal error: unhandled update_entries cases.");

    if (stored_mode)
    {
	change_mode (finfo->file, stored_mode, 1);
	free (stored_mode);
	stored_mode = NULL;
    }
   
    if (stored_modtime_valid)
    {
	struct utimbuf t;

	memset (&t, 0, sizeof (t));
	t.modtime = stored_modtime;
	(void) time (&t.actime);

#ifdef UTIME_EXPECTS_WRITABLE
	if (!iswritable (finfo->file))
	{
	    xchmod (finfo->file, 1);
	    change_it_back = 1;
	}
#endif  /* UTIME_EXPECTS_WRITABLE  */

	if (utime (finfo->file, &t) < 0)
	    error (0, errno, "cannot set time on %s", quote (finfo->fullname));

#ifdef UTIME_EXPECTS_WRITABLE
	if (change_it_back)
	{
	    xchmod (finfo->file, 0);
	    change_it_back = 0;
	}
#endif  /*  UTIME_EXPECTS_WRITABLE  */

	stored_modtime_valid = 0;
    }

    /*
     * Process the entries line.  Do this after we've written the file,
     * since we need the timestamp.
     */
    if (!STREQ (cvs_cmd_name, "export"))
    {
	char *local_timestamp;
	char *file_timestamp;
	bool ignore_merge;

	time (&last_register_time);

	local_timestamp = data->timestamp;
	if (!local_timestamp || ts[0] == '+' || last_merge_conflict)
	    file_timestamp = time_stamp (finfo->file);
	else
	    file_timestamp = NULL;

	/*
	 * These special version numbers signify that it is not up to
	 * date.  Create a dummy timestamp which will never compare
	 * equal to the timestamp of the file.
	 */
	if (vn[0] == '\0' || STREQ (vn, "0") || vn[0] == '-')
	    local_timestamp = "dummy timestamp";
	else if (!local_timestamp)
	{
	    local_timestamp = file_timestamp;

	    /* Checking for cvs_cmd_name of "commit" doesn't seem like
	       the cleanest way to handle this, but it seem to roughly
	       parallel what the :local: code which calls
	       mark_up_to_date ends up amounting to.  Some day, should
	       think more about what the Checked-in response means
	       vis-a-vis both Entries and Base and clarify
	       cvsclient.texi accordingly.  */

	    if (STREQ (cvs_cmd_name, "commit"))
		mark_up_to_date (finfo->update_dir, finfo->file);
	}

	if (last_merge)
	{
	    if (last_merge_made_base)
	    {
		Node *n;
		Entnode *e;

		n = findnode_fn (finfo->entries, finfo->file);
		assert (n);

		e = n->data;
		if (!STREQ (vn, e->version))
		    /* Merge.  */
		    ignore_merge = false;
		else
		    /* Join. */
		    ignore_merge = true;
	    }
	    else
		ignore_merge = false;
	}
	else
	    ignore_merge = true;
	
	Register (finfo, vn,
		  ignore_merge ? local_timestamp : "Result of merge",
		  options, tag, date,
		  ts[0] == '+' || last_merge_conflict ? file_timestamp : NULL);
	if (last_merge_conflict)
	{
	    assert (!ignore_merge);
	    if (!really_quiet)
	    {
		cvs_output ("C ", 2);
		cvs_output (finfo->fullname, 0);
		cvs_output ("\n", 1);
	    }
	}
	else if (!ignore_merge)
	{
	    if (!really_quiet)
	    {
		cvs_output ("M ", 2);
		cvs_output (finfo->fullname, 0);
		cvs_output ("\n", 1);
	    }
	}
	last_merge = false;
	last_merge_conflict = false;
	last_merge_made_base = false;

	if (file_timestamp)
	    free (file_timestamp);

    }
    free (scratch_entries);
    free (entries_line);
}



static void
handle_checked_in (char *args, size_t len)
{
    struct update_entries_data dat;
    dat.contents = UPDATE_ENTRIES_CHECKIN;
    dat.existp = UPDATE_ENTRIES_EXISTING_OR_NEW;
    dat.timestamp = NULL;
    call_in_directory (args, update_entries, &dat);
}



static void
handle_new_entry (char *args, size_t len)
{
    struct update_entries_data dat;
    dat.contents = UPDATE_ENTRIES_CHECKIN;
    dat.existp = UPDATE_ENTRIES_EXISTING_OR_NEW;
    dat.timestamp = "dummy timestamp from new-entry";
    call_in_directory (args, update_entries, &dat);
}



static void
handle_updated (char *args, size_t len)
{
    struct update_entries_data dat;
    dat.contents = UPDATE_ENTRIES_UPDATE;
    dat.existp = UPDATE_ENTRIES_EXISTING_OR_NEW;
    dat.timestamp = NULL;
    call_in_directory (args, update_entries, &dat);
}



static void
handle_created (char *args, size_t len)
{
    struct update_entries_data dat;
    dat.contents = UPDATE_ENTRIES_UPDATE;
    dat.existp = UPDATE_ENTRIES_NEW;
    dat.timestamp = NULL;
    call_in_directory (args, update_entries, &dat);
}



static void
handle_update_existing (char *args, size_t len)
{
    struct update_entries_data dat;
    dat.contents = UPDATE_ENTRIES_UPDATE;
    dat.existp = UPDATE_ENTRIES_EXISTING;
    dat.timestamp = NULL;
    call_in_directory (args, update_entries, &dat);
}



static void
handle_merged (char *args, size_t len)
{
    struct update_entries_data dat;
    dat.contents = UPDATE_ENTRIES_UPDATE;
    /* Think this could be UPDATE_ENTRIES_EXISTING, but just in case...  */
    dat.existp = UPDATE_ENTRIES_EXISTING_OR_NEW;
    dat.timestamp = "Result of merge";
    call_in_directory (args, update_entries, &dat);
}



static void
handle_patched (char *args, size_t len)
{
    struct update_entries_data dat;
    dat.contents = UPDATE_ENTRIES_PATCH;
    /* Think this could be UPDATE_ENTRIES_EXISTING, but just in case...  */
    dat.existp = UPDATE_ENTRIES_EXISTING_OR_NEW;
    dat.timestamp = NULL;
    call_in_directory (args, update_entries, &dat);
}



static void
handle_rcs_diff (char *args, size_t len)
{
    struct update_entries_data dat;
    dat.contents = UPDATE_ENTRIES_RCS_DIFF;
    /* Think this could be UPDATE_ENTRIES_EXISTING, but just in case...  */
    dat.existp = UPDATE_ENTRIES_EXISTING_OR_NEW;
    dat.timestamp = NULL;
    call_in_directory (args, update_entries, &dat);
}



/*
 * The OpenPGP-signature response gives the signature for the file to be
 * transmitted in the next Base-checkout or Temp-checkout response.
 */
static struct buffer *stored_signatures;
static void
handle_openpgp_signature (char *args, size_t len)
{
    int status;

    if (!stored_signatures)
	stored_signatures = buf_nonio_initialize (NULL);

    status = next_signature (global_from_server, stored_signatures);
    if (status == -2)
	xalloc_die ();
    else if (status)
	error (1, 0, "Malformed signature received from server.");
}



/* Write the signatures in the global STORED_SIGNATURES to SIGFILE.  Use
 * WRITABLE to set permissions.  If SIGCOPY is not NULL, assume that SIGLEN
 * isn't either and save a copy of the signature in newly allocated memory
 * stored at *SIGCOPY and set *SIGLEN to its length.
 */
static void
client_write_sigfile (const char *sigfile, bool writable, char **sigcopy,
		      size_t *siglen)
{
    FILE *e;
    size_t want;

    assert (!sigcopy || siglen);

    if (!stored_signatures)
	return;

    if (!writable && isfile (sigfile))
	xchmod (sigfile, true);
    e = xfopen (sigfile, FOPEN_BINARY_WRITE);

    want = buf_length (stored_signatures);
    if (sigcopy)
    {
	*sigcopy = NULL;
	*siglen = 0;
    }
    while (want > 0)
    {
	char *data;
	size_t got;

	buf_read_data (stored_signatures, want, &data, &got);

	if (fwrite (data, sizeof *data, got, e) != got)
	    error (1, errno, "cannot write signature file `%s'", sigfile);

	if (sigcopy)
	{
	    *sigcopy = xrealloc (*sigcopy, *siglen + got);
	    memcpy (*sigcopy + *siglen, data, got);
	    *siglen += got;
	}

	want -= got;
    }
	
    if (fclose (e) == EOF)
	error (0, errno, "cannot close signature file `%s'", sigfile);

    if (!writable)
	xchmod (sigfile, false);

    buf_free (stored_signatures);
    stored_signatures = NULL;
}



static void
client_base_checkout (void *data_arg, const struct file_info *finfo)
{
    /* Options for this file, a Previous REVision if this is a diff, and the
     * REVision of the new file.
     */
    char *options, *prev, *rev;

    /* The base file to be created.  */
    char *basefile;
    char *fullbase;

    /* File buf from net.  May be an RCS diff from PREV to REV.  */
    char *buf;
    char *mode_string;
    size_t size;

    bool bin;
    bool patch_failed;
    bool *istemp = data_arg;

    TRACE (TRACE_FUNCTION, "client_base_checkout (%s)", finfo->fullname);

    /* Read OPTIONS, PREV, and REV from the server.  */
    read_line (&options);
    read_line (&prev);
    read_line (&rev);

    /* Use these values to get our base file name.  */
    if (*istemp)
    {
	if (temp_checkout2)
	    error (1, 0,
		   "Server sent more than two temp files without using them.");
	basefile = cvs_temp_name ();
	if (temp_checkout1)
	    temp_checkout2 = basefile;
	else
	    temp_checkout1 = basefile;
    }
    else
	basefile = make_base_file_name (finfo->file, rev);

    /* FIXME?  It might be nice to verify that base files aren't being
     * overwritten except when the keyword mode has changed.
     */
    if (!*istemp && isfile (basefile))
	force_xchmod (basefile, true);

    if (*istemp) fullbase = xstrdup (basefile);
    else fullbase = dir_append (finfo->update_dir, basefile);

    /* Read the file or patch from the server.  */
    /* FIXME: Read/write to file is repeated and could be optimized to
     * write directly to disk without using so much mem.
     */
    buf = read_file_from_server (fullbase, &mode_string, &size);

    if (options) bin = STREQ (options, "-kb");
    else bin = false;

    if (*prev && !STREQ (prev, rev))
    {
	char *filebuf;
	size_t filebufsize;
	size_t nread;
	char *patchedbuf;
	size_t patchedlen;
	char *pbasefile;
	char *pfullbase;

	/* Handle UPDATE_ENTRIES_RCS_DIFF.  */

	pbasefile = make_base_file_name (finfo->file, prev);
	pfullbase = dir_append (finfo->update_dir, pbasefile);

	if (!isfile (pbasefile))
	    error (1, 0, "patch original file %s does not exist",
		   quote (pfullbase));

	filebuf = NULL;
	filebufsize = 0;
	nread = 0;

	get_file (pbasefile, pfullbase, bin ? FOPEN_BINARY_READ : "r",
		  &filebuf, &filebufsize, &nread);
	/* At this point the contents of the existing file are in
	 * FILEBUF, and the length of the contents is in NREAD.
	 * The contents of the patch from the network are in BUF,
	 * and the length of the patch is in SIZE.
	 */

	patch_failed = !rcs_change_text (fullbase, filebuf, nread, buf,
					 size, &patchedbuf, &patchedlen);

	if (!patch_failed)
	{
	    free (buf);
	    buf = patchedbuf;
	    size = patchedlen;
	}
	else
	    free (patchedbuf);

	free (filebuf);
	free (pbasefile);
	free (pfullbase);
    }
    else
	patch_failed = false;

    if (!patch_failed)
    {
	FILE *e;
	int status;
	bool verify = get_verify_checkouts (true);

	if (!*istemp)
	    cvs_xmkdir (CVSADM_BASE, NULL, MD_EXIST_OK);
	e = xfopen (basefile, bin ? FOPEN_BINARY_WRITE : "w");
	if (fwrite (buf, sizeof *buf, size, e) != size)
	    error (1, errno, "cannot write `%s'", fullbase);
	if (fclose (e) == EOF)
	    error (0, errno, "cannot close `%s'", fullbase);

	status = change_mode (basefile, mode_string, 1);
	if (status != 0)
	    error (0, status, "cannot change mode of `%s'", fullbase);

	if (stored_signatures)
	{
	    char *sigfile = Xasprintf ("%s.sig", basefile);
	    char *sigcopy;
	    size_t siglen;

	    /* A lot of trouble is gone through here to copy the signatures
	     * into a buffer in addition to writing them to disk.  Writing to
	     * disk requires a call to fsync () before the call to
	     * verify_signature otherwise, and fsync () is quite slow.
	     */
	    client_write_sigfile (sigfile, *istemp,
				  verify ? &sigcopy : NULL, &siglen);

	    /* Verify the signature here, when configured to do so.  */
	    if (verify /* cannot be `cvs export'. */)
	    {
		if (!verify_signature (finfo->repository, sigcopy, siglen,
				       basefile, bin,
				       get_verify_checkouts_fatal ()))
		{
		    /* verify_signature exits when VERIFY_FATAL.  */
		    assert (!get_verify_checkouts_fatal ());
		    error (0, 0, "Bad signature for `%s'.", fullbase);
		}
		free (sigcopy);
	    }

	    if (*istemp && CVS_UNLINK (sigfile) < 0)
		error (0, errno, "Failed to remove temp sig file %s",
		       quote (sigfile));

	    free (sigfile);
	}
	else if (verify /* cannot be `cvs export'. */)
	    error (get_verify_checkouts_fatal (), 0,
		   "No signature for %s.", quote (fullbase));
    }

    free (buf);
    free (rev);
    free (prev);
    if (!*istemp)
	free (basefile);
    free (fullbase);
}



static void
handle_base_checkout (char *args, size_t len)
{
    bool istemp = false;
    if (suppress_bases)
	error (1, 0, "Server sent Base-* response when asked not to.");
    call_in_directory (args, client_base_checkout, &istemp);
}



static void
handle_temp_checkout (char *args, size_t len)
{
    bool istemp = true;
    if (suppress_bases)
	error (1, 0, "Server sent Base-* response when asked not to.");
    call_in_directory (args, client_base_checkout, &istemp);
}



static void
client_base_signatures (void *data_arg, const struct file_info *finfo)
{
    char *rev;
    char *basefile;
    char *sigfile;
    bool *clear = data_arg;

    TRACE (TRACE_FUNCTION, "client_base_signatures (%s)", finfo->fullname);

    if (!stored_signatures && !*clear)
	error (1, 0,
	       "Server sent `Base-signatures' response without signature.");

    if (stored_signatures && *clear)
	error (1, 0, "Server sent unused signature data.");

    /* Read REV from the server.  */
    read_line (&rev);

    basefile = make_base_file_name (finfo->file, rev);
    sigfile = Xasprintf ("%s.sig", basefile);

    if (*clear)
    {
	if (unlink_file (sigfile) < 0 && !existence_error (errno))
	    error (0, 0, "Failed to delete signature file `%s'",
		   sigfile);
    }
    else
	client_write_sigfile (sigfile, false, NULL, NULL);

    free (rev);
    free (basefile);
    free (sigfile);
}



static void
handle_base_signatures (char *args, size_t len)
{
    bool clear = false;
    if (suppress_bases)
	error (1, 0, "Server sent Base-* response when asked not to.");
    call_in_directory (args, client_base_signatures, &clear);
}



static void
handle_base_clear_signatures (char *args, size_t len)
{
    bool clear = true;
    if (suppress_bases)
	error (1, 0, "Server sent Base-* response when asked not to.");
    call_in_directory (args, client_base_signatures, &clear);
}



/* Create am up-to-date temporary workfile from a base file.
 *
 * NOTES
 *   Base-copy needs to be able to copy temp files in the case of a join which
 *   adds a file.  Though keeping a base file in this case could potentially
 *   be interesting, this is not what certain portions of the code currently
 *   expect.
 */
static void
client_base_copy (void *data_arg, const struct file_info *finfo)
{
    char *rev, *flags;
    char *basefile;
    char *temp_filename;

    TRACE (TRACE_FUNCTION, "client_base_copy (%s)", finfo->fullname);

    read_line (&rev);

    read_line (&flags);
    if (!validate_change (translate_exists (flags), finfo))
    {
	/* The Mode, Mod-time, and Checksum responses should not carry
	 * over to a subsequent Created (or whatever) response, even
	 * in the error case.
	 */
	if (updated_fname)
	{
	    free (updated_fname);
	    updated_fname = NULL;
	}
	if (stored_mode)
	{
	    free (stored_mode);
	    stored_mode = NULL;
	}
	stored_modtime_valid = 0;
	stored_checksum_valid = 0;

	failure_exit = true;
	base_copy_error = true;
	free (rev);
	free (flags);
	return;
    }

    if (temp_checkout1)
    {
	if (temp_checkout2)
	    error (1, 0, "Server sent two temp files before a Base-copy.");
	basefile = temp_checkout1;
    }
    else
	basefile = make_base_file_name (finfo->file, rev);

    temp_filename = newfilename (finfo->file);
    copy_file (basefile, temp_filename);

    if (flags[0] && flags[1] == 'n')
	xchmod (temp_filename, false);
    else
	xchmod (temp_filename, true);

    /* I think it is ok to assume that if the server is sending base_copy,
     * then it sent the commands necessary to create the required base file.
     * If not, then it may be necessary to provide a way to request the base
     * file be sent.
     */

    if (temp_checkout1)
    {
	temp_checkout1 = NULL;
	if (CVS_UNLINK (basefile) < 0)
	    error (0, errno, "Failed to remove temp file %s", quote (basefile));
    }

    free (flags);
    free (temp_filename);
    free (basefile);
    free (rev);
}



static void
handle_base_copy (char *args, size_t len)
{
    if (suppress_bases)
	error (1, 0, "Server sent Base-* response when asked not to.");
    call_in_directory (args, client_base_copy, NULL);
}



static void
client_base_merge (void *data_arg, const struct file_info *finfo)
{
    char *f1, *f2;
    char *temp_filename;
    int status;

    TRACE (TRACE_FUNCTION, "client_base_merge (%s)", finfo->fullname);

    read_line (&base_merge_rev1);
    read_line (&base_merge_rev2);

    if (!really_quiet)
    {
	cvs_output ("Merging differences between ", 0);
	cvs_output (base_merge_rev1, 0);
	cvs_output (" and ", 5);
	cvs_output (base_merge_rev2, 0);
	cvs_output (" into ", 6);
	cvs_output (quote (finfo->fullname), 0);
	cvs_output ("\n", 1);
    }

    if (temp_checkout1)
    {
	f1 = temp_checkout1;
	if (temp_checkout2)
	    f2 = temp_checkout2;
	else
	{
	    f2 = make_base_file_name (finfo->file, base_merge_rev2);
	    if (!isfile (f2))
		error (1, 0, "Server sent only one temp file before a merge.");
	}
    }
    else
    {
	f1 = make_base_file_name (finfo->file, base_merge_rev1);
	f2 = make_base_file_name (finfo->file, base_merge_rev2);
    }

    temp_filename = newfilename (finfo->file);

    force_copy_file (finfo->file, temp_filename);
    force_xchmod (temp_filename, true);

    status = merge (temp_filename, finfo->file, f1, base_merge_rev1, f2,
		    base_merge_rev2);

    if (status != 0 && status != 1)
	error (status == -1, status == -1 ? errno : 0,
	       "could not merge differences between %s & %s of %s",
	       base_merge_rev1, base_merge_rev2, quote (finfo->fullname));

    if (last_merge && !noexec)
	error (1, 0,
"protocol error: received two `Base-merge' responses without a `Base-entry'");
    last_merge = true;

    if (status == 1)
    {
	/* The server won't send a response telling the client to update the
	 * entry in noexec mode.  Normally the client delays printing the
	 * "C filename" line until then.
	 */
	last_merge_conflict = true;
	if (noexec && !really_quiet)
	{
	    cvs_output ("C ", 2);
	    cvs_output (finfo->fullname, 0);
	    cvs_output ("\n", 1);
	}
    }
    else
    {
	Node *n;

	if (!xcmp (temp_filename, finfo->file))
	{
	    if (!quiet)
	    {
		cvs_output ("`", 1);
		cvs_output (finfo->fullname, 0);
		cvs_output ("' already contains the differences between ", 0);
		cvs_output (base_merge_rev1, 0);
		cvs_output (" and ", 5);
		cvs_output (base_merge_rev2, 0);
		cvs_output ("\n", 1);
	    }
	}

	/* This next is a separate case because a join could restore the file
	 * to its state at checkout time.
	 */
	if ((n = findnode_fn (finfo->entries, finfo->file)))
	{
	    Entnode *e = n->data;
	    char *basefile = make_base_file_name (finfo->file, e->version);
	    if (isfile (basefile) && !xcmp (basefile, temp_filename))
		/* The user's file is identical to the base file.
		 * Pretend this merge never happened.
		 */
		last_merge_made_base = true;
	    free (basefile);
	}
    }

    /* In the noexec case, just remove our results.  */
    if (noexec && CVS_UNLINK (temp_filename) < 0)
	error (0, errno, "Failed to remove `%s'", temp_filename);

    /* Let update_entries remove our "temporary" base files, since it should
     * know which one should be kept.
     */

    if (temp_checkout1)
    {
	temp_checkout1 = NULL;
	if (CVS_UNLINK (f1) < 0)
	    error (0, errno, "Failed to remove temp file %s", quote (f1));
    }
    if (temp_checkout2)
    {
	temp_checkout2 = NULL;
	if (CVS_UNLINK (f2) < 0)
	    error (0, errno, "Failed to remove temp file %s", quote (f2));
    }
    free (f1);
    free (f2);
}



static void
handle_base_merge (char *args, size_t len)
{
    if (suppress_bases)
	error (1, 0, "Server sent Base-* response when asked not to.");
    call_in_directory (args, client_base_merge, NULL);
}



static void
handle_base_entry (char *args, size_t len)
{
    struct update_entries_data dat;
    if (suppress_bases)
	error (1, 0, "Server sent Base-* response when asked not to.");
    dat.contents = UPDATE_ENTRIES_BASE;
    dat.existp = UPDATE_ENTRIES_EXISTING_OR_NEW;
    dat.timestamp = NULL;
    call_in_directory (args, update_entries, &dat);
}



static void
handle_base_merged (char *args, size_t len)
{
    struct update_entries_data dat;
    if (suppress_bases)
	error (1, 0, "Server sent Base-* response when asked not to.");
    dat.contents = UPDATE_ENTRIES_BASE;
    dat.existp = UPDATE_ENTRIES_EXISTING_OR_NEW;
    dat.timestamp = "Result of merge";
    call_in_directory (args, update_entries, &dat);
}



static void
client_base_diff (void *data_arg, const struct file_info *finfo)
{
    struct diff_info *di = data_arg;
    char *ft1, *ft2, *rev1, *rev2, *label1, *label2;
    const char *f1 = NULL, *f2 = NULL;
    bool used_t1 = false, used_t2 = false;

    read_line (&ft1);

    read_line (&rev1);
    if (!*rev1)
    {
	free (rev1);
	rev1 = NULL;
    }

    read_line (&label1);
    if (!*label1)
    {
	free (label1);
	label1 = NULL;
    }

    read_line (&ft2);

    read_line (&rev2);
    if (!*rev2)
    {
	free (rev2);
	rev2 = NULL;
    }

    read_line (&label2);
    if (!*label2)
    {
	free (label2);
	label2 = NULL;
    }

    if (STREQ (ft1, "TEMP"))
    {
	if (!temp_checkout1)
	    error (1, 0,
		   "Server failed to send enough files before Base-diff.");
	f1 = temp_checkout1;

	used_t1 = true;
    }
    else if (STREQ (ft1, "DEVNULL"))
	f1 = DEVNULL;
    else
	error (1, 0, "Server sent unrecognized diff file type (%s)",
	       quote (ft1));

    if (STREQ (ft2, "TEMP"))
    {
	if ((used_t1 && !temp_checkout2) || (!used_t1 && !temp_checkout1))
	    error (1, 0,
		   "Server failed to send enough files before Base-diff.");

	if (used_t1)
	{
	    f2 = temp_checkout2;
	    used_t2 = true;
	}
	else
	{
	    f2 = temp_checkout1;
	    used_t1 = true;
	}
    }
    else if (STREQ (ft2, "DEVNULL"))
	f2 = DEVNULL;
    else if (STREQ (ft2, "WORKFILE"))
	f2 = finfo->file;
    else
	error (1, 0, "Server sent unrecognized diff file type (%s)",
	       quote (ft2));

    if ((!used_t1 && temp_checkout1)
	|| (!used_t2 && temp_checkout2))
	error (1, 0, "Unused temp files sent from server before Base-diff.");

    diff_mark_errors (base_diff (finfo, di->diff_argc, di->diff_argv,
				 f1, rev1, label1, f2,
				 rev2, label2, di->empty_files));

    if (ft1) free (ft1);
    if (ft2) free (ft2);
    if (rev1) free (rev1);
    if (rev2) free (rev2);
    if (label1) free (label1);
    if (label2) free (label2);
    if (temp_checkout1)
    {
	char *tmp = temp_checkout1;
	temp_checkout1 = NULL;
	if (CVS_UNLINK (tmp) < 0)
	    error (0, errno, "Failed to remove temp file %s", quote (tmp));
	free (tmp);
    }
    if (temp_checkout2)
    {
	char *tmp = temp_checkout2;
	temp_checkout2 = NULL;
	if (CVS_UNLINK (tmp) < 0)
	    error (0, errno, "Failed to remove temp file %s", quote (tmp));
	free (tmp);
    }

    return;
}



static void
handle_base_diff (char *args, size_t len)
{
    if (suppress_bases)
	error (1, 0, "Server sent Base-* response when asked not to.");
    call_in_directory (args, client_base_diff, get_diff_info ());
}



static void
remove_entry (void *data, const struct file_info *finfo)
{
    Scratch_Entry (finfo->entries, finfo->file);
}



static void
handle_remove_entry (char *args, size_t len)
{
    call_in_directory (args, remove_entry, NULL);
}



static void
remove_entry_and_file (void *data, const struct file_info *finfo)
{
    Scratch_Entry (finfo->entries, finfo->file);
    /* Note that we don't ignore existence_error's here.  The server
       should be sending Remove-entry rather than Removed in cases
       where the file does not exist.  And if the user removes the
       file halfway through a cvs command, we should be printing an
       error.  */
    if (unlink_file (finfo->file) < 0)
	error (0, errno, "unable to remove %s", quote (finfo->fullname));
}



static void
handle_removed (char *args, size_t len)
{
    call_in_directory (args, remove_entry_and_file, NULL);
}



/* Does relative path specification PATHNAME specify the top level repository
 * directory (directory containing the configuration project, CVSROOT) for the
 * current repository?
 */
static bool
is_cvsroot_level (char *pathname)
{
    /* If the current TOPLEVEL_REPOS isn't for the current root, return
     * false.  The server can sometimes set TOPLEVEL_REPOS to a subpath of
     * CURRENT_PARSED_ROOT->directory.
     */
    if (!STREQ (toplevel_repos, current_parsed_root->directory))
	return false;

    /* A "./" should no longer be possible.  */
    assert (pathname[0] != '.' || !ISSLASH (pathname[1]) || pathname[2]);

    /* An empty (relative) repository specifies the top level.  */
    return !*pathname;
}



static void
set_static (void *data, const struct file_info *finfo)
{
    FILE *fp;
    fp = xfopen (CVSADM_ENTSTAT, "w+");
    if (fclose (fp) == EOF)
        error (1, errno, "cannot close %s", CVSADM_ENTSTAT);
}



static void
handle_set_static_directory (char *args, size_t len)
{
    if (STREQ (cvs_cmd_name, "export"))
    {
	/* Swallow the repository.  */
	read_line (NULL);
	return;
    }
    call_in_directory (args, set_static, NULL);
}



static void
clear_static (void *data, const struct file_info *finfo)
{
    if (unlink_file (CVSADM_ENTSTAT) < 0 && ! existence_error (errno))
        error (1, errno, "cannot remove file %s", CVSADM_ENTSTAT);
}



static void
handle_clear_static_directory (char *pathname, size_t len)
{
    if (STREQ (cvs_cmd_name, "export"))
    {
	/* Swallow the repository.  */
	read_line (NULL);
	return;
    }

    call_in_directory (pathname, clear_static, NULL);
}



static void
set_sticky (void *data, const struct file_info *finfo)
{
    char *tagspec;
    FILE *f;

    read_line (&tagspec);

    /* FIXME-update-dir: error messages should include the directory.  */
    f = CVS_FOPEN (CVSADM_TAG, "w+");
    if (!f)
    {
	/* Making this non-fatal is a bit of a kludge (see dirs2
	   in testsuite).  A better solution would be to avoid having
	   the server tell us about a directory we shouldn't be doing
	   anything with anyway (e.g. by handling directory
	   addition/removal better).  */
	error (0, errno, "cannot open %s", CVSADM_TAG);
	free (tagspec);
	return;
    }
    if (fprintf (f, "%s\n", tagspec) < 0)
	error (1, errno, "writing %s", CVSADM_TAG);
    if (fclose (f) == EOF)
	error (1, errno, "closing %s", CVSADM_TAG);
    free (tagspec);
}



static void
handle_set_sticky (char *pathname, size_t len)
{
    if (STREQ (cvs_cmd_name, "export"))
    {
	/* Swallow the repository.  */
	read_line (NULL);
        /* Swallow the tag line.  */
	read_line (NULL);
	return;
    }
    if (is_cvsroot_level (pathname))
    {
        /*
	 * Top level (directory containing CVSROOT).  This seems to normally
	 * lack a CVS directory, so don't try to create files in it.
	 */

	/* Swallow the repository.  */
	read_line (NULL);
        /* Swallow the tag line.  */
	read_line (NULL);
	return;
    }

    call_in_directory (pathname, set_sticky, NULL);
}



static void
clear_sticky (void *data, const struct file_info *finfo)
{
    if (unlink_file (CVSADM_TAG) < 0 && ! existence_error (errno))
	error (1, errno, "cannot remove %s", CVSADM_TAG);
}



static void
handle_clear_sticky (char *pathname, size_t len)
{
    if (STREQ (cvs_cmd_name, "export"))
    {
	/* Swallow the repository.  */
	read_line (NULL);
	return;
    }

    if (is_cvsroot_level (pathname))
    {
        /*
	 * Top level (directory containing CVSROOT).  This seems to normally
	 * lack a CVS directory, so don't try to create files in it.
	 */
	return;
    }

    call_in_directory (pathname, clear_sticky, NULL);
}



/* Handle the client-side support for a successful edit.
 */
static void
handle_edit_file (char *pathname, size_t len)
{
    call_in_directory (pathname, edit_file, NULL);
}



static void
template (void *data, const struct file_info *finfo)
{
    char *buf = Xasprintf ("%s/%s", finfo->update_dir, CVSADM_TEMPLATE);
    read_counted_file (CVSADM_TEMPLATE, buf);
    free (buf);
}



static void
handle_template (char *pathname, size_t len)
{
    call_in_directory (pathname, template, NULL);
}



static void
clear_template (void *data, const struct file_info *finfo)
{
    if (unlink_file (CVSADM_TEMPLATE) < 0 && ! existence_error (errno))
	error (1, errno, "cannot remove %s", CVSADM_TEMPLATE);
}



static void
handle_clear_template (char *pathname, size_t len)
{
    call_in_directory (pathname, clear_template, NULL);
}



struct save_dir {
    char *dir;
    struct save_dir *next;
};

struct save_dir *prune_candidates;

static void
add_prune_candidate (const char *dir)
{
    struct save_dir *p;
    char *q;

    if ((dir[0] == '.' && dir[1] == '\0')
	|| (prune_candidates && STREQ (dir, prune_candidates->dir)))
	return;
    p = xmalloc (sizeof (struct save_dir));
    p->dir = xstrdup (dir);
    for (q = p->dir + strlen(p->dir); q > p->dir && ISSLASH(q[-1]); q--) ;
    *q = '\0';
    p->next = prune_candidates;
    prune_candidates = p;
}



static void
process_prune_candidates (void)
{
    struct save_dir *p;
    struct save_dir *q;

    if (toplevel_wd)
    {
	if (CVS_CHDIR (toplevel_wd) < 0)
	    error (1, errno, "could not chdir to %s", toplevel_wd);
    }
    for (p = prune_candidates; p; )
    {
	if (isemptydir ("", p->dir, 1))
	{
	    char *b;

	    if (unlink_file_dir (p->dir) < 0)
		error (0, errno, "cannot remove %s", p->dir);
	    b = strrchr (p->dir, '/');
	    if (!b)
		Subdir_Deregister (NULL, NULL, p->dir);
	    else
	    {
		*b = '\0';
		Subdir_Deregister (NULL, p->dir, b + 1);
	    }
	}
	free (p->dir);
	q = p->next;
	free (p);
	p = q;
    }
    prune_candidates = NULL;
}



/* Send a Repository line.  */
static char *last_repos;
static char *last_update_dir;
static void
send_repository (const char *dir, const char *repos, const char *update_dir)
{
    TRACE (TRACE_FUNCTION, "send_repository (%s, %s, %s)",
	   dir, repos, update_dir);

    /* FIXME: this is probably not the best place to check; I wish I
     * knew where in here's callers to really trap this bug.  To
     * reproduce the bug, just do this:
     * 
     *       mkdir junk
     *       cd junk
     *       cvs -d some_repos update foo
     *
     * Poof, CVS seg faults and dies!  It's because it's trying to
     * send a NULL string to the server but dies in send_to_server.
     * That string was supposed to be the repository, but it doesn't
     * get set because there's no CVSADM dir, and somehow it's not
     * getting set from the -d argument either... ?
     */
    if (!repos)
        /* Lame error.  I want a real fix but can't stay up to track
           this down right now. */
        error (1, 0, "no repository");

    if (last_repos && STREQ (repos, last_repos)
	&& last_update_dir && STREQ (NULL2DOT (update_dir), last_update_dir))
	/* We've already sent it.  */
	return;

    if (client_prune_dirs)
	add_prune_candidate (update_dir);

    /* Add a directory name to the list of those sent to the
       server. */
    if (!STREQ (NULL2DOT (update_dir), ".")
	&& !findnode (dirs_sent_to_server, NULL2DOT (update_dir)))
    {
	Node *n;
	n = getnode();
	n->type = NT_UNKNOWN;
	n->key = xstrdup (NULL2DOT (update_dir));
	n->data = NULL;

	if (addnode (dirs_sent_to_server, n))
	    error (1, 0, "cannot add directory %s to list", n->key);
    }

    send_to_server ("Directory ", 0);
    {
	/* Send the directory name.  I know that this
	 * sort of duplicates code elsewhere, but each
	 * case seems slightly different...
	 */
	const char *p;

	/* Old servers don't support preserving update_dir in the Directory
	 * request.  See the comment aboce output_dir() for more.
	 */
	if (supported_request ("Signature"))
	    p = NULL2MT (update_dir);
	else
	    p = NULL2DOT (update_dir);

	while (*p)
	{
	    assert (*p != '\012');
	    send_to_server (ISSLASH (*p) ? "/" : p, 1);
	    ++p;
	}
    }
    send_to_server ("\012", 1);
    if (supported_request ("Relative-directory"))
    {
	const char *short_repos = Short_Repository (repos);
	send_to_server (short_repos, 0);
    }
    else
	send_to_server (repos, 0);
    send_to_server ("\012", 1);

    if (!STREQ (cvs_cmd_name, "import")
	&& supported_request ("Static-directory"))
    {
	char *adm_name = dir_append (dir, CVSADM_ENTSTAT);
	if (isreadable (adm_name))
	    send_to_server ("Static-directory\012", 0);
	free (adm_name);
    }
    if (!STREQ (cvs_cmd_name, "import")
	&& supported_request ("Sticky"))
    {
	char *adm_name = dir_append (dir, CVSADM_TAG);
	FILE *f = CVS_FOPEN (adm_name, "r");
	if (!f)
	{
	    if (!existence_error (errno))
		error (1, errno, "reading %s",
		       quote (dir_append (update_dir, CVSADM_TAG)));
	}
	else
	{
	    char line[80];
	    const char *nl = NULL;
	    send_to_server ("Sticky ", 0);
	    while (fgets (line, sizeof (line), f))
	    {
		send_to_server (line, 0);
		nl = strchr (line, '\n');
		if (nl)
		    break;
	    }
	    if (!nl)
                send_to_server ("\012", 1);
	    if (fclose (f) == EOF)
	    {
		char *fn = dir_append (update_dir, CVSADM_TAG);
		error (0, errno, "closing %s", quote (fn));
		free (fn);
	    }
	}
	free (adm_name);
    }
    if (last_repos) free (last_repos);
    if (last_update_dir) free (last_update_dir);
    last_repos = xstrdup (repos);
    last_update_dir = xstrdup (NULL2DOT (update_dir));
}



/* Send a Repository line and set toplevel_repos.  */
void
send_a_repository (const char *dir, const char *repository,
                   const char *update_dir_in)
{
    char *update_dir;

    assert (update_dir_in);

    TRACE (TRACE_FLOW, "send_a_repository (%s, %s, %s)",
	   dir, repository, update_dir_in);

    update_dir = xstrdup (update_dir_in);

    if (!toplevel_repos && repository)
    {
	if (update_dir[0] == '\0'
	    || (update_dir[0] == '.' && update_dir[1] == '\0'))
	    toplevel_repos = xstrdup (repository);
	else
	{
	    /*
	     * Get the repository from a CVS/Repository file if update_dir
	     * is absolute.  This is not correct in general, because
	     * the CVS/Repository file might not be the top-level one.
	     * This is for cases like "cvs update /foo/bar" (I'm not
	     * sure it matters what toplevel_repos we get, but it does
	     * matter that we don't hit the "internal error" code below).
	     */
	    if (update_dir[0] == '/')
		toplevel_repos = Name_Repository (update_dir, update_dir);
	    else
	    {
		/*
		 * Guess the repository of that directory by looking at a
		 * subdirectory and removing as many pathname components
		 * as are in update_dir.  I think that will always (or at
		 * least almost always) be 1.
		 *
		 * So this deals with directories which have been
		 * renamed, though it doesn't necessarily deal with
		 * directories which have been put inside other
		 * directories (and cvs invoked on the containing
		 * directory).  I'm not sure the latter case needs to
		 * work.
		 *
		 * 21 Aug 1998: Well, Mr. Above-Comment-Writer, it
		 * does need to work after all.  When we are using the
		 * client in a multi-cvsroot environment, it will be
		 * fairly common that we have the above case (e.g.,
		 * cwd checked out from one repository but
		 * subdirectory checked out from another).  We can't
		 * assume that by walking up a directory in our wd we
		 * necessarily walk up a directory in the repository.
		 */
		/*
		 * This gets toplevel_repos wrong for "cvs update ../foo"
		 * but I'm not sure toplevel_repos matters in that case.
		 */

		int repository_len, update_dir_len;

		strip_trailing_slashes (update_dir);

		repository_len = strlen (repository);
		update_dir_len = strlen (update_dir);

		/* Try to remove the path components in UPDATE_DIR
                   from REPOSITORY.  If the path elements don't exist
                   in REPOSITORY, or the removal of those path
                   elements mean that we "step above"
                   current_parsed_root->directory, set toplevel_repos to
                   current_parsed_root->directory. */
		if (repository_len > update_dir_len
		    && STREQ (repository + repository_len - update_dir_len,
			      update_dir)
		    /* TOPLEVEL_REPOS shouldn't be above
		     * CURRENT_PARSED_ROOT->directory.
		     */
		    && ((size_t)(repository_len - update_dir_len)
			> strlen (current_parsed_root->directory)))
		{
		    /* The repository name contains UPDATE_DIR.  Set
		     * toplevel_repos to the repository name without
		     * UPDATE_DIR.
		     */

		    toplevel_repos = xmalloc (repository_len - update_dir_len);
		    /* Note that we don't copy the trailing '/'.  */
		    strncpy (toplevel_repos, repository,
			     repository_len - update_dir_len - 1);
		    toplevel_repos[repository_len - update_dir_len - 1] = '\0';
		}
		else
		    toplevel_repos = xstrdup (current_parsed_root->directory);
	    }
	}
    }

    send_repository (dir, repository, update_dir);
    free (update_dir);
}



static void
notified_a_file (void *data, const struct file_info *finfo)
{
    FILE *fp;
    FILE *newf;
    size_t line_len = 8192;
    char *line = xmalloc (line_len);
    char *cp;
    int nread;
    int nwritten;
    char *p;

    fp = xfopen (CVSADM_NOTIFY, "r");
    if (getline (&line, &line_len, fp) < 0)
    {
	if (feof (fp))
	    error (0, 0, "cannot read %s: end of file", CVSADM_NOTIFY);
	else
	    error (0, errno, "cannot read %s", CVSADM_NOTIFY);
	goto error_exit;
    }
    cp = strchr (line, '\t');
    if (!cp)
    {
	error (0, 0, "malformed %s file", CVSADM_NOTIFY);
	goto error_exit;
    }
    *cp = '\0';
    if (!STREQ (finfo->file, line + 1))
	error (0, 0, "protocol error: notified %s, expected %s", finfo->file,
	       line + 1);

    if (getline (&line, &line_len, fp) < 0)
    {
	if (feof (fp))
	{
	    free (line);
	    if (fclose (fp) < 0)
		error (0, errno, "cannot close %s", CVSADM_NOTIFY);
	    if (CVS_UNLINK (CVSADM_NOTIFY) < 0)
		error (0, errno, "cannot remove %s", CVSADM_NOTIFY);
	    return;
	}
	else
	{
	    error (0, errno, "cannot read %s", CVSADM_NOTIFY);
	    goto error_exit;
	}
    }
    newf = xfopen (CVSADM_NOTIFYTMP, "w");
    if (fputs (line, newf) < 0)
    {
	error (0, errno, "cannot write %s", CVSADM_NOTIFYTMP);
	goto error2;
    }
    while ((nread = fread (line, 1, line_len, fp)) > 0)
    {
	p = line;
	while ((nwritten = fwrite (p, sizeof *p, nread, newf)) > 0)
	{
	    nread -= nwritten;
	    p += nwritten;
	}
	if (ferror (newf))
	{
	    error (0, errno, "cannot write %s", CVSADM_NOTIFYTMP);
	    goto error2;
	}
    }
    if (ferror (fp))
    {
	error (0, errno, "cannot read %s", CVSADM_NOTIFY);
	goto error2;
    }
    if (fclose (newf) < 0)
    {
	error (0, errno, "cannot close %s", CVSADM_NOTIFYTMP);
	goto error_exit;
    }
    free (line);
    if (fclose (fp) < 0)
    {
	error (0, errno, "cannot close %s", CVSADM_NOTIFY);
	return;
    }

    {
        /* In this case, we want rename_file() to ignore noexec. */
        int saved_noexec = noexec;
        noexec = 0;
        rename_file (CVSADM_NOTIFYTMP, CVSADM_NOTIFY);
        noexec = saved_noexec;
    }

    return;
  error2:
    fclose (newf);
  error_exit:
    free (line);
    fclose (fp);
}



static void
handle_notified (char *args, size_t len)
{
    call_in_directory (args, notified_a_file, NULL);
}



/* The "expanded" modules.  */
static int modules_count;
static int modules_allocated;
static char **modules_vector;

static void
handle_module_expansion (char *args, size_t len)
{
    if (!modules_vector)
    {
	modules_allocated = 1; /* Small for testing */
	modules_vector = xnmalloc (modules_allocated,
				   sizeof (modules_vector[0]));
    }
    else if (modules_count >= modules_allocated)
    {
	modules_allocated *= 2;
	modules_vector = xnrealloc (modules_vector,
				    modules_allocated,
				    sizeof (modules_vector[0]));
    }
    modules_vector[modules_count] = xstrdup (args);
    ++modules_count;
}



/* Original, not "expanded" modules.  */
static int module_argc;
static char **module_argv;

void
client_expand_modules (int argc, char **argv, int local)
{
    int errs;
    int i;

    module_argc = argc;
    module_argv = xnmalloc (argc + 1, sizeof (module_argv[0]));
    for (i = 0; i < argc; ++i)
	module_argv[i] = xstrdup (argv[i]);
    module_argv[argc] = NULL;

    for (i = 0; i < argc; ++i)
	send_arg (argv[i]);
    send_a_repository ("", current_parsed_root->directory, "");

    send_to_server ("expand-modules\012", 0);

    errs = get_server_responses ();

    if (last_repos) free (last_repos);
    last_repos = NULL;

    if (last_update_dir) free (last_update_dir);
    last_update_dir = NULL;

    if (errs)
	error (errs, 0, "cannot expand modules");
}



void
client_send_expansions (int local, char *where, int build_dirs)
{
    int i;
    char *argv[1];

    TRACE (TRACE_FUNCTION, "client_send_expansions (%d, %s, %d)",
	   local, where, build_dirs);

    /* Send the original module names.  The "expanded" module name might
       not be suitable as an argument to a co request (e.g. it might be
       the result of a -d argument in the modules file).  It might be
       cleaner if we genuinely expanded module names, all the way to a
       local directory and repository, but that isn't the way it works
       now.  */
    send_file_names (module_argc, module_argv, 0);

    for (i = 0; i < modules_count; ++i)
    {
	argv[0] = where ? where : modules_vector[i];
	if (isfile (argv[0]))
	    send_files (1, argv, local, 0, build_dirs ? SEND_BUILD_DIRS : 0);
    }
    send_a_repository ("", current_parsed_root->directory, "");
}



void
client_nonexpanded_setup (void)
{
    send_a_repository ("", current_parsed_root->directory, "");
}



/* Receive a cvswrappers line from the server; it must be a line
   containing an RCS option (e.g., "*.exe   -k 'b'").

   Note that this doesn't try to handle -t/-f options (which are a
   whole separate issue which noone has thought much about, as far
   as I know).

   We need to know the keyword expansion mode so we know whether to
   read the file in text or binary mode.  */
static void
handle_wrapper_rcs_option (char *args, size_t len)
{
    char *p;

    /* Enforce the notes in cvsclient.texi about how the response is not
       as free-form as it looks.  */
    p = strchr (args, ' ');
    if (!p)
	goto handle_error;
    if (*++p != '-'
	|| *++p != 'k'
	|| *++p != ' '
	|| *++p != '\'')
	goto handle_error;
    if (!strchr (p, '\''))
	goto handle_error;

    /* Add server-side cvswrappers line to our wrapper list. */
    wrap_add (args, 0);
    return;
 handle_error:
    error (0, errno, "protocol error: ignoring invalid wrappers %s", args);
}




static void
handle_m (char *args, size_t len)
{
    /* In the case where stdout and stderr point to the same place,
       fflushing stderr will make output happen in the correct order.
       Often stderr will be line-buffered and this won't be needed,
       but not always (is that true?  I think the comment is probably
       based on being confused between default buffering between
       stdout and stderr.  But I'm not sure).  */
    fflush (stderr);
    fwrite (args, sizeof *args, len, stdout);
    putc ('\n', stdout);
}



static void
handle_mbinary (char *args, size_t len)
{
    char *size_string;
    size_t size;
    size_t totalread;
    size_t nread;
    size_t toread;
    char buf[8192];

    /* See comment at handle_m about (non)flush of stderr.  */

    /* Get the size.  */
    read_line (&size_string);
    size = strto_file_size (size_string);
    free (size_string);

    /* OK, now get all the data.  The algorithm here is that we read
       as much as the network wants to give us in
       try_read_from_server, and then we output it all, and then
       repeat, until we get all the data.  */
    totalread = 0;
    while (totalread < size)
    {
	toread = size - totalread;
	if (toread > sizeof buf)
	    toread = sizeof buf;

	nread = try_read_from_server (buf, toread);
	cvs_output_binary (buf, nread);
	totalread += nread;
    }
}



static void
handle_e (char *args, size_t len)
{
    /* In the case where stdout and stderr point to the same place,
       fflushing stdout will make output happen in the correct order.  */
    fflush (stdout);
    fwrite (args, sizeof *args, len, stderr);
    putc ('\n', stderr);
}



/*ARGSUSED*/
static void
handle_f  (char *args, size_t len)
{
    fflush (stderr);
}



static void
handle_mt (char *args, size_t len)
{
    char *p;
    char *tag = args;
    char *text;

    /* See comment at handle_m for more details.  */
    fflush (stderr);

    p = strchr (args, ' ');
    if (!p)
	text = NULL;
    else
    {
	*p++ = '\0';
	text = p;
    }

    switch (tag[0])
    {
	case '+':
	    if (STREQ (tag, "+updated"))
		updated_seen = 1;
	    else if (STREQ (tag, "+importmergecmd"))
		importmergecmd.seen = 1;
	    break;
	case '-':
	    if (STREQ (tag, "-updated"))
		updated_seen = 0;
	    else if (STREQ (tag, "-importmergecmd"))
	    {
		char buf[80];

		/* Now that we have gathered the information, we can
                   output the suggested merge command.  */

		if (importmergecmd.conflicts == 0
		    || !importmergecmd.mergetag1
		    || !importmergecmd.mergetag2
		    || !importmergecmd.repository)
		{
		    error (0, 0,
			   "invalid server: incomplete importmergecmd tags");
		    break;
		}

		if (importmergecmd.conflicts == -1)
 		    sprintf (buf, "\nNo conflicts created by this import.\n");
		else
		    sprintf (buf, "\n%d conflicts created by this import.\n",
			     importmergecmd.conflicts);
		cvs_output (buf, 0);
		cvs_output ("Use the following command to help the merge:\n\n",
			    0);
		cvs_output ("\t", 1);
		cvs_output (program_name, 0);
		if (CVSroot_cmdline)
		{
		    cvs_output (" -d ", 0);
		    cvs_output (CVSroot_cmdline, 0);
		}
		cvs_output (" checkout -j", 0);
		cvs_output (importmergecmd.mergetag1, 0);
		cvs_output (" -j", 0);
		cvs_output (importmergecmd.mergetag2, 0);
		cvs_output (" ", 1);
		cvs_output (importmergecmd.repository, 0);
		cvs_output ("\n\n", 0);

		/* Clear the static variables so that everything is
                   ready for any subsequent importmergecmd tag.  */
		importmergecmd.conflicts = 0;
		free (importmergecmd.mergetag1);
		importmergecmd.mergetag1 = NULL;
		free (importmergecmd.mergetag2);
		importmergecmd.mergetag2 = NULL;
		free (importmergecmd.repository);
		importmergecmd.repository = NULL;

		importmergecmd.seen = 0;
	    }
	    break;
	default:
	    if (updated_seen)
	    {
		if (STREQ (tag, "fname"))
		{
		    if (updated_fname)
		    {
			/* Output the previous message now.  This can happen
			   if there was no Update-existing or other such
			   response, due to the -n global option.  */
			cvs_output ("U ", 0);
			cvs_output (updated_fname, 0);
			cvs_output ("\n", 1);
			free (updated_fname);
		    }
		    updated_fname = xstrdup (text);
		}
		/* Swallow all other tags.  Either they are extraneous
		   or they reflect future extensions that we can
		   safely ignore.  */
	    }
	    else if (importmergecmd.seen)
	    {
		if (STREQ (tag, "conflicts"))
		{
		    if (text == NULL || STREQ (text, "No"))
			importmergecmd.conflicts = -1;
		    else
			importmergecmd.conflicts = atoi (text);
		}
		else if (STREQ (tag, "mergetag1"))
		    importmergecmd.mergetag1 = xstrdup (text);
		else if (STREQ (tag, "mergetag2"))
		    importmergecmd.mergetag2 = xstrdup (text);
		else if (STREQ (tag, "repository"))
		    importmergecmd.repository = xstrdup (text);
		/* Swallow all other tags.  Either they are text for
                   which we are going to print our own version when we
                   see -importmergecmd, or they are future extensions
                   we can safely ignore.  */
	    }
	    else if (STREQ (tag, "newline"))
		printf ("\n");
	    else if (STREQ (tag, "date"))
	    {
		if (text)
		{
		    char *date = format_date_alloc (text);
		    printf ("%s", date);
		    free (date);
		}
	    }
	    else if (text)
		printf ("%s", text);
    }
}



#endif /* CLIENT_SUPPORT */
#if defined(CLIENT_SUPPORT) || defined(SERVER_SUPPORT)

/* This table must be writeable if the server code is included.  */
struct response responses[] =
{
#ifdef CLIENT_SUPPORT
#define RSP_LINE(n, f, t, s) {n, f, t, s}
#else /* ! CLIENT_SUPPORT */
#define RSP_LINE(n, f, t, s) {n, s}
#endif /* CLIENT_SUPPORT */

    RSP_LINE("ok", handle_ok, response_type_ok, rs_essential),
    RSP_LINE("error", handle_error, response_type_error, rs_essential),
    RSP_LINE("Valid-requests", handle_valid_requests, response_type_normal,
       rs_essential),
    RSP_LINE("Force-gzip", handle_force_gzip, response_type_normal,
       rs_optional),
    RSP_LINE("Referrer", handle_referrer, response_type_normal, rs_optional),
    RSP_LINE("Redirect", handle_redirect, response_type_redirect, rs_optional),
    RSP_LINE("Checked-in", handle_checked_in, response_type_normal,
       rs_essential),
    RSP_LINE("New-entry", handle_new_entry, response_type_normal, rs_optional),
    RSP_LINE("Checksum", handle_checksum, response_type_normal, rs_optional),
    RSP_LINE("Copy-file", handle_copy_file, response_type_normal, rs_optional),
    RSP_LINE("Updated", handle_updated, response_type_normal, rs_essential),
    RSP_LINE("Created", handle_created, response_type_normal, rs_optional),
    RSP_LINE("Update-existing", handle_update_existing, response_type_normal,
       rs_optional),
    RSP_LINE("Merged", handle_merged, response_type_normal, rs_essential),
    RSP_LINE("Patched", handle_patched, response_type_normal, rs_optional),
    RSP_LINE("Rcs-diff", handle_rcs_diff, response_type_normal, rs_optional),
    RSP_LINE("Mode", handle_mode, response_type_normal, rs_optional),
    RSP_LINE("Mod-time", handle_mod_time, response_type_normal, rs_optional),
    RSP_LINE("Removed", handle_removed, response_type_normal, rs_essential),
    RSP_LINE("Remove-entry", handle_remove_entry, response_type_normal,
       rs_optional),
    RSP_LINE("Set-static-directory", handle_set_static_directory,
       response_type_normal,
       rs_optional),
    RSP_LINE("Clear-static-directory", handle_clear_static_directory,
       response_type_normal,
       rs_optional),
    RSP_LINE("Set-sticky", handle_set_sticky, response_type_normal,
       rs_optional),
    RSP_LINE("Clear-sticky", handle_clear_sticky, response_type_normal,
       rs_optional),
    RSP_LINE("Edit-file", handle_edit_file, response_type_normal,
       rs_optional),
    RSP_LINE("Template", handle_template, response_type_normal,
       rs_optional),
    RSP_LINE("Clear-template", handle_clear_template, response_type_normal,
       rs_optional),
    RSP_LINE("Notified", handle_notified, response_type_normal, rs_optional),
    RSP_LINE("Module-expansion", handle_module_expansion, response_type_normal,
       rs_optional),
    RSP_LINE("Wrapper-rcsOption", handle_wrapper_rcs_option,
       response_type_normal,
       rs_optional),
    RSP_LINE("M", handle_m, response_type_normal, rs_essential),
    RSP_LINE("Mbinary", handle_mbinary, response_type_normal, rs_optional),
    RSP_LINE("E", handle_e, response_type_normal, rs_essential),
    RSP_LINE("F", handle_f, response_type_normal, rs_optional),
    RSP_LINE("MT", handle_mt, response_type_normal, rs_optional),

    /* The server makes the assumption that if the client handles one of the
     * Base-* responses, then it will handle all of them.
     */
    RSP_LINE("Base-checkout", handle_base_checkout, response_type_normal,
	     rs_optional),
    RSP_LINE("Temp-checkout", handle_temp_checkout, response_type_normal,
	     rs_optional),
    RSP_LINE("Base-copy", handle_base_copy, response_type_normal, rs_optional),
    RSP_LINE("Base-merge", handle_base_merge, response_type_normal,
	     rs_optional),
    RSP_LINE("Base-entry", handle_base_entry, response_type_normal,
	     rs_optional),
    RSP_LINE("Base-merged", handle_base_merged, response_type_normal,
	     rs_optional),
    RSP_LINE("Base-diff", handle_base_diff, response_type_normal, rs_optional),

    RSP_LINE("Base-signatures", handle_base_signatures, response_type_normal,
	     rs_optional),
    RSP_LINE("Base-clear-signatures", handle_base_clear_signatures,
	     response_type_normal, rs_optional),
    RSP_LINE("OpenPGP-signature", handle_openpgp_signature,
	     response_type_normal, rs_optional),

    /* Possibly should be response_type_error.  */
    RSP_LINE(NULL, NULL, response_type_normal, rs_essential)

#undef RSP_LINE
};

#endif /* CLIENT_SUPPORT or SERVER_SUPPORT */
#ifdef CLIENT_SUPPORT



/* 
 * If LEN is 0, then send_to_server_via() computes string's length itself.
 *
 * Therefore, pass the real length when transmitting data that might
 * contain 0's.
 */
void
send_to_server_via (struct buffer *via_buffer, const char *str, size_t len)
{
    static int nbytes;

    if (len == 0)
	len = strlen (str);

    buf_output (via_buffer, str, len);
      
    /* There is no reason not to send data to the server, so do it
       whenever we've accumulated enough information in the buffer to
       make it worth sending.  */
    nbytes += len;
    if (nbytes >= 2 * BUFFER_DATA_SIZE)
    {
	int status;

        status = buf_send_output (via_buffer);
	if (status != 0)
	    error (1, status, "error writing to server");
	nbytes = 0;
    }
}



void
send_to_server (const char *str, size_t len)
{
  send_to_server_via (global_to_server, str, len);
}



/* Read up to LEN bytes from the server.  Returns actual number of
   bytes read, which will always be at least one; blocks if there is
   no data available at all.  Gives a fatal error on EOF or error.  */
static size_t
try_read_from_server( char *buf, size_t len )
{
    int status;
    size_t nread;
    char *data;

    status = buf_read_data (global_from_server, len, &data, &nread);
    if (status != 0)
    {
	if (status == -1)
	    error (1, 0,
		   "end of file from server (consult above messages if any)");
	else if (status == -2)
	    error (1, 0, "out of memory");
	else
	    error (1, status, "reading from server");
    }

    memcpy (buf, data, nread);

    return nread;
}



/*
 * Read LEN bytes from the server or die trying.
 */
void
read_from_server (char *buf, size_t len)
{
    size_t red = 0;
    while (red < len)
    {
	red += try_read_from_server (buf + red, len - red);
	if (red == len)
	    break;
    }
}



/* Get some server responses and process them.
 *
 * RETURNS
 *   0		Success
 *   1		Error
 *   2		Redirect
 */
int
get_server_responses (void)
{
    struct response *rs;
    do
    {
	char *cmd;
	size_t len;
	
	len = read_line (&cmd);
	for (rs = responses; rs->name; ++rs)
	    if (STRNEQ (cmd, rs->name, strlen (rs->name)))
	    {
		size_t cmdlen = strlen (rs->name);
		if (cmd[cmdlen] == '\0')
		    ;
		else if (cmd[cmdlen] == ' ')
		    ++cmdlen;
		else
		    /*
		     * The first len characters match, but it's a different
		     * response.  e.g. the response is "oklahoma" but we
		     * matched "ok".
		     */
		    continue;
		(*rs->func) (cmd + cmdlen, len - cmdlen);
		break;
	    }
	if (!rs->name)
	    /* It's OK to print just to the first '\0'.  */
	    /* We might want to handle control characters and the like
	       in some other way other than just sending them to stdout.
	       One common reason for this error is if people use :ext:
	       with a version of rsh which is doing CRLF translation or
	       something, and so the client gets "ok^M" instead of "ok".
	       Right now that will tend to print part of this error
	       message over the other part of it.  It seems like we could
	       do better (either in general, by quoting or omitting all
	       control characters, and/or specifically, by detecting the CRLF
	       case and printing a specific error message).  */
	    error (0, 0,
		   "warning: unrecognized response `%s' from cvs server",
		   cmd);
	free (cmd);
    } while (rs->type == response_type_normal);

    if (updated_fname)
    {
	/* Output the previous message now.  This can happen
	   if there was no Update-existing or other such
	   response, due to the -n global option.  */
	cvs_output ("U ", 0);
	cvs_output (updated_fname, 0);
	cvs_output ("\n", 1);
	free (updated_fname);
	updated_fname = NULL;
    }

    if (rs->type == response_type_redirect) return 2;
    if (rs->type == response_type_error) return 1;
    if (failure_exit) return 1;
    return 0;
}



static inline void
close_connection_to_server (struct buffer **to, struct buffer **from)
{
    int status;

    /* First we shut down GLOBAL_TO_SERVER.  That tells the server that its
     * input is finished.  It then shuts down the buffer it is sending to us,
     * at which point our shut down of GLOBAL_FROM_SERVER will complete.
     */

    TRACE (TRACE_FUNCTION, "close_connection_to_server ()");

    status = buf_shutdown (*to);
    if (status != 0)
	error (0, status, "shutting down buffer to server");
    buf_free (*to);
    *to = NULL;

    status = buf_shutdown (*from);
    if (status != 0)
	error (0, status, "shutting down buffer from server");
    buf_free (*from);
    *from = NULL;
}



/* Get the responses and then close the connection.  */

/*
 * Flag var; we'll set it in start_server() and not one of its
 * callees, such as start_rsh_server().  This means that there might
 * be a small window between the starting of the server and the
 * setting of this var, but all the code in that window shouldn't care
 * because it's busy checking return values to see if the server got
 * started successfully anyway.
 */
int server_started = 0;

int
get_responses_and_close (void)
{
    int errs = get_server_responses ();

    /* The following is necessary when working with multiple cvsroots, at least
     * with commit.  It used to be buried nicely in do_deferred_progs() before
     * that function was removed.  I suspect it wouldn't be necessary if
     * call_in_directory() saved its working directory via save_cwd() before
     * changing its directory and restored the saved working directory via
     * restore_cwd() before exiting.  Of course, calling CVS_CHDIR only once,
     * here, may be more efficient.
     */
    if (toplevel_wd)
    {
	if (CVS_CHDIR (toplevel_wd) < 0)
	    error (1, errno, "could not chdir to %s", toplevel_wd);
    }

    if (client_prune_dirs)
	process_prune_candidates ();

    close_connection_to_server (&global_to_server, &global_from_server);
    server_started = 0;

    /* see if we need to sleep before returning to avoid time-stamp races */
    if (last_register_time)
	sleep_past (last_register_time);

    return errs;
}



bool
supported_request (const char *name)
{
    struct request *rq;

    for (rq = requests; rq->name; rq++)
	if (STREQ (rq->name, name))
	    return (rq->flags & RQ_SUPPORTED) != 0;
    error (1, 0, "internal error: testing support for unknown request?");
    /* NOTREACHED */
    return 0;
}



#if defined (AUTH_CLIENT_SUPPORT) || defined (SERVER_SUPPORT) || defined (HAVE_KERBEROS) || defined (HAVE_GSSAPI)


/* Generic function to do port number lookup tasks.
 *
 * In order of precedence, will return:
 * 	getenv (envname), if defined
 * 	getservbyname (portname), if defined
 * 	defaultport
 */
static int
get_port_number (const char *envname, const char *portname, int defaultport)
{
    struct servent *s;
    char *port_s;

    if (envname && (port_s = getenv (envname)))
    {
	int port = atoi (port_s);
	if (port <= 0)
	{
	    error (0, 0, "%s must be a positive integer!  If you", envname);
	    error (0, 0, "are trying to force a connection via rsh, please");
	    error (0, 0, "put \":server:\" at the beginning of your CVSROOT");
	    error (1, 0, "variable.");
	}
	return port;
    }
    else if (portname && (s = getservbyname (portname, "tcp")))
	return ntohs (s->s_port);
    else
	return defaultport;
}



/* get the port number for a client to connect to based on the port
 * and method of a cvsroot_t.
 *
 * we do this here instead of in parse_cvsroot so that we can keep network
 * code confined to a localized area and also to delay the lookup until the
 * last possible moment so it remains possible to run cvs client commands that
 * skip opening connections to the server (i.e. skip network operations
 * entirely)
 *
 * and yes, I know none of the commands do that now, but here's to planning
 * for the future, eh?  cheers.
 */
int
get_cvs_port_number (const cvsroot_t *root)
{

    if (root->port) return root->port;

    switch (root->method)
    {
# ifdef HAVE_GSSAPI
	case gserver_method:
# endif /* HAVE_GSSAPI */
# ifdef AUTH_CLIENT_SUPPORT
	case pserver_method:
# endif /* AUTH_CLIENT_SUPPORT */
# if defined (AUTH_CLIENT_SUPPORT) || defined (HAVE_GSSAPI)
	    return get_port_number ("CVS_CLIENT_PORT", "cvspserver",
                                    CVS_AUTH_PORT);
# endif /* defined (AUTH_CLIENT_SUPPORT) || defined (HAVE_GSSAPI) */
# ifdef HAVE_KERBEROS
	case kserver_method:
	    return get_port_number ("CVS_CLIENT_PORT", "cvs", CVS_PORT);
# endif /* HAVE_KERBEROS */
	default:
	    error(1, EINVAL,
"internal error: get_cvs_port_number called for invalid connection method (%s)",
		  method_names[root->method]);
	    break;
    }
    /* NOTREACHED */
    return -1;
}



/* get the port number for a client to connect to based on the proxy port
 * of a cvsroot_t.
 */
static int
get_proxy_port_number (const cvsroot_t *root)
{

    if (root->proxy_port) return root->proxy_port;

    return get_port_number ("CVS_PROXY_PORT", NULL, CVS_PROXY_PORT);
}



void
make_bufs_from_fds(int tofd, int fromfd, int child_pid, cvsroot_t *root,
                   struct buffer **to_server_p,
                   struct buffer **from_server_p, int is_sock)
{
# ifdef NO_SOCKET_TO_FD
    if (is_sock)
    {
	assert (tofd == fromfd);
	*to_server_p = socket_buffer_initialize (tofd, 0, NULL);
	*from_server_p = socket_buffer_initialize (tofd, 1, NULL);
    }
    else
# endif /* NO_SOCKET_TO_FD */
    {
	/* todo: some OS's don't need these calls... */
	close_on_exec (tofd);
	close_on_exec (fromfd);

	/* SCO 3 and AIX have a nasty bug in the I/O libraries which precludes
	   fdopening the same file descriptor twice, so dup it if it is the
	   same.  */
	if (tofd == fromfd)
	{
	    fromfd = dup (tofd);
	    if (fromfd < 0)
		error (1, errno, "cannot dup net connection");
	}

	/* These will use binary mode on systems which have it.  */
	/*
	 * Also, we know that from_server is shut down second, so we pass
	 * child_pid in there.  In theory, it should be stored in both
	 * buffers with a ref count...
	 */
	*to_server_p = fd_buffer_initialize (tofd, 0, root, false,
					     connection_timeout, NULL);
	*from_server_p = fd_buffer_initialize (fromfd, child_pid, root,
                                               true, connection_timeout, NULL);
    }
}
#endif /* defined (AUTH_CLIENT_SUPPORT) || defined (SERVER_SUPPORT) || defined (HAVE_KERBEROS) || defined(HAVE_GSSAPI) */



#if defined (AUTH_CLIENT_SUPPORT) || defined(HAVE_GSSAPI)
/* Connect to the authenticating server.

   If VERIFY_ONLY is non-zero, then just verify that the password is
   correct and then shutdown the connection.

   If VERIFY_ONLY is 0, then really connect to the server.

   If DO_GSSAPI is non-zero, then we use GSSAPI authentication rather
   than the pserver password authentication.

   If we fail to connect or if access is denied, then die with fatal
   error.  */
void
connect_to_pserver (cvsroot_t *root, struct buffer **to_server_p,
                    struct buffer **from_server_p, int verify_only,
                    int do_gssapi)
{
    int sock;
    int port_number,
	proxy_port_number = 0; /* Initialize to silence -Wall.  Dumb.  */
    union sai {
	struct sockaddr_in addr_in;
	struct sockaddr addr;
    } client_sai;
    struct hostent *hostinfo;
    struct buffer *to_server, *from_server;

    sock = socket (AF_INET, SOCK_STREAM, 0);
    if (sock == -1)
	error (1, 0, "cannot create socket: %s", SOCK_STRERROR (SOCK_ERRNO));
    port_number = get_cvs_port_number (root);

    /* if we have a proxy connect to that instead */
    if (root->proxy_hostname)
    {
        proxy_port_number = get_proxy_port_number (root);
	hostinfo = init_sockaddr (&client_sai.addr_in, root->proxy_hostname,
                                  proxy_port_number);
        TRACE (TRACE_FUNCTION, "Connecting to %s:%d via proxy %s(%s):%d.",
               root->hostname, port_number, root->proxy_hostname,
               inet_ntoa (client_sai.addr_in.sin_addr), proxy_port_number);
    }
    else
    {
	hostinfo = init_sockaddr (&client_sai.addr_in, root->hostname,
				  port_number);
        TRACE (TRACE_FUNCTION, "Connecting to %s(%s):%d.",
               root->hostname,
               inet_ntoa (client_sai.addr_in.sin_addr), port_number);
    }

    if (connect (sock, &client_sai.addr, sizeof (client_sai))
	< 0)
	error (1, 0, "connect to %s(%s):%d failed: %s",
	       root->proxy_hostname ? root->proxy_hostname : root->hostname,
	       inet_ntoa (client_sai.addr_in.sin_addr),
	       root->proxy_hostname ? proxy_port_number : port_number,
               SOCK_STRERROR (SOCK_ERRNO));

    make_bufs_from_fds (sock, sock, 0, root, &to_server, &from_server, 1);

    /* if we have proxy then connect to the proxy first */
    if (root->proxy_hostname)
    {
#define CONNECT_STRING "CONNECT %s:%d HTTP/1.0\r\n\r\n"
	/* Send a "CONNECT" command to proxy: */
	char* read_buf;
	int codenum;
	size_t count;
	/* 4 characters for port covered by the length of %s & %d */
	char* write_buf = Xasnprintf (NULL, &count, CONNECT_STRING,
                                      root->hostname, port_number);
	send_to_server_via (to_server, write_buf, count);

	/* Wait for HTTP status code, bail out if you don't get back a 2xx
         * code.
         */
	read_line_via (from_server, to_server, &read_buf);
	sscanf (read_buf, "%s %d", write_buf, &codenum);

	if ((codenum / 100) != 2)
	    error (1, 0, "proxy server %s:%d does not support http tunnelling",
		   root->proxy_hostname, proxy_port_number);
	free (read_buf);
	free (write_buf);

	/* Skip through remaining part of MIME header, recv_line
           consumes the trailing \n */
	while (read_line_via (from_server, to_server, &read_buf) > 0)
	{
	    if (read_buf[0] == '\r' || read_buf[0] == 0)
	    {
		free (read_buf);
		break;
	    }
	    free (read_buf);
	}
    }

    auth_server (root, to_server, from_server, verify_only, do_gssapi,
                 hostinfo);

    if (verify_only)
    {
	int status;

	status = buf_shutdown (to_server);
	if (status != 0)
	    error (0, status, "shutting down buffer to server");
	buf_free (to_server);
	to_server = NULL;

	status = buf_shutdown (from_server);
	if (status != 0)
	    error (0, status, "shutting down buffer from server");
	buf_free (from_server);
	from_server = NULL;

	/* Don't need to set server_started = 0 since we don't set it to 1
	 * until returning from this call.
	 */
    }
    else
    {
	*to_server_p = to_server;
	*from_server_p = from_server;
    }

    return;
}



static void
auth_server (cvsroot_t *root, struct buffer *to_server,
             struct buffer *from_server, int verify_only, int do_gssapi,
             struct hostent *hostinfo)
{
    char *username = NULL;		/* the username we use to connect */
    char no_passwd = 0;			/* gets set if no password found */

    /* Run the authorization mini-protocol before anything else. */
    if (do_gssapi)
    {
# ifdef HAVE_GSSAPI
	int fd = buf_get_fd (to_server);
	struct stat s;

	if ((fd < 0) || (fstat (fd, &s) < 0) || !S_ISSOCK(s.st_mode))
	{
	    error (1, 0,
                   "gserver currently only enabled for socket connections");
	}

	if (! connect_to_gserver (root, fd, hostinfo))
	{
	    error (1, 0,
		    "authorization failed: server %s rejected access to %s",
		    root->hostname, root->directory);
	}
# else /* ! HAVE_GSSAPI */
	error (1, 0,
"INTERNAL ERROR: This client does not support GSSAPI authentication");
# endif /* HAVE_GSSAPI */
    }
    else /* ! do_gssapi */
    {
# ifdef AUTH_CLIENT_SUPPORT
	char *begin      = NULL;
	char *password   = NULL;
	char *end        = NULL;
	
	if (verify_only)
	{
	    begin = "BEGIN VERIFICATION REQUEST";
	    end   = "END VERIFICATION REQUEST";
	}
	else
	{
	    begin = "BEGIN AUTH REQUEST";
	    end   = "END AUTH REQUEST";
	}

	/* Get the password, probably from ~/.cvspass. */
	password = get_cvs_password ();
	username = root->username ? root->username : getcaller();

	/* Send the empty string by default.  This is so anonymous CVS
	   access doesn't require client to have done "cvs login". */
	if (!password) 
	{
	    no_passwd = 1;
	    password = scramble ("");
	}

	/* Announce that we're starting the authorization protocol. */
	send_to_server_via(to_server, begin, 0);
	send_to_server_via(to_server, "\012", 1);

	/* Send the data the server needs. */
	send_to_server_via(to_server, root->directory, 0);
	send_to_server_via(to_server, "\012", 1);
	send_to_server_via(to_server, username, 0);
	send_to_server_via(to_server, "\012", 1);
	send_to_server_via(to_server, password, 0);
	send_to_server_via(to_server, "\012", 1);

	/* Announce that we're ending the authorization protocol. */
	send_to_server_via(to_server, end, 0);
	send_to_server_via(to_server, "\012", 1);

	free_cvs_password (password);
	password = NULL;
# else /* ! AUTH_CLIENT_SUPPORT */
	error (1, 0, "INTERNAL ERROR: This client does not support pserver authentication");
# endif /* AUTH_CLIENT_SUPPORT */
    } /* if (do_gssapi) */

    {
	char *read_buf;

	/* Loop, getting responses from the server.  */
	while (1)
	{
	    read_line_via (from_server, to_server, &read_buf);

	    if (STREQ (read_buf, "I HATE YOU"))
	    {
		/* Authorization not granted.
		 *
		 * This is a little confusing since we can reach this while
		 * loop in GSSAPI mode, but if GSSAPI authentication failed,
		 * we already jumped to the rejected label (there is no case
		 * where the connect_to_gserver function can return 1 and we
		 * will not receive "I LOVE YOU" from the server, barring
		 * broken connections and garbled messages, of course).  The
		 * GSSAPI case is also the case where username can be NULL
		 * since username is initialized in the !gssapi section.
		 *
		 * i.e. This is a pserver specific error message and should be
		 * since GSSAPI doesn't use username.
		 */
		error (0, 0,
"authorization failed: server %s rejected access to %s for user %s",
		       root->hostname, root->directory,
		       username ? username : "(null)");

		/* Output a special error message if authentication was attempted
		with no password -- the user should be made aware that they may
		have missed a step. */
		if (no_passwd)
		{
		    error (0, 0,
"used empty password; try \"cvs login\" with a real password");
		}
		exit (EXIT_FAILURE);
	    }
	    else if (STRNEQ (read_buf, "E ", 2))
	    {
		fprintf (stderr, "%s\n", read_buf + 2);

		/* Continue with the authentication protocol.  */
	    }
	    else if (STRNEQ (read_buf, "error ", 6))
	    {
		char *p;

		/* First skip the code.  */
		p = read_buf + 6;
		while (*p != ' ' && *p != '\0')
		    ++p;

		/* Skip the space that follows the code.  */
		if (*p == ' ')
		    ++p;

		/* Now output the text.  */
		fprintf (stderr, "%s\n", p);
		exit (EXIT_FAILURE);
	    }
	    else if (STREQ (read_buf, "I LOVE YOU"))
	    {
		free (read_buf);
		break;
	    }
	    else
	    {
		error (1, 0, 
		       "unrecognized auth response from %s: %s", 
		       root->hostname, read_buf);
	    }
	    free (read_buf);
	}
    }
}
#endif /* defined (AUTH_CLIENT_SUPPORT) || defined(HAVE_GSSAPI) */



#if defined (CLIENT_SUPPORT) || defined (SERVER_SUPPORT)
/* 
 * Connect to a forked server process.
 */
static void
connect_to_forked_server (cvsroot_t *root, struct buffer **to_server_p,
                          struct buffer **from_server_p)
{
    int tofd, fromfd;
    int child_pid;

    /* This is pretty simple.  All we need to do is choose the correct
       cvs binary and call piped_child. */

     char *command[3];

    command[0] = (root->cvs_server
		  ? root->cvs_server : getenv ("CVS_SERVER"));
    if (!command[0])
# ifdef SERVER_SUPPORT
        /* FIXME:
         * I'm casting out the const below because I know that piped_child, the
         * only function we pass COMMAND to, accepts COMMAND as a
         * (char *const *) and won't alter it, and we don't alter it in this
         * function.  This is yucky, there should be a way to declare COMMAND
         * such that this casting isn't needed, but I don't know how.  If I
         * declare it as (const char *command[]), the compiler complains about
         * an incompatible arg 1 being passed to piped_child and if I declare
         * it as (char *const command[3]), then the compiler complains when I
         * assign values to command[i].
         */
	command[0] = (char *)program_path;
# else /* SERVER_SUPPORT */
    {
	error( 0, 0, "You must set the CVS_SERVER environment variable when" );
	error( 0, 0, "using the :fork: access method." );
	error( 1, 0, "This CVS was not compiled with server support." );
    }
# endif /* SERVER_SUPPORT */
    
    command[1] = "server";
    command[2] = NULL;

    TRACE (TRACE_FUNCTION, "Forking server: %s %s",
	   command[0] ? command[0] : "(null)", command[1]);

    child_pid = piped_child (command, &tofd, &fromfd, false);
    if (child_pid < 0)
	error (1, 0, "could not fork server process");

    make_bufs_from_fds (tofd, fromfd, child_pid, root, to_server_p,
                        from_server_p, 0);
}
#endif /* CLIENT_SUPPORT || SERVER_SUPPORT */



static int
send_variable_proc (Node *node, void *closure)
{
    send_to_server ("Set ", 0);
    send_to_server (node->key, 0);
    send_to_server ("=", 1);
    send_to_server (node->data, 0);
    send_to_server ("\012", 1);
    return 0;
}



/* Open up the connection to the server and perform any necessary
 * authentication.
 */
void
open_connection_to_server (cvsroot_t *root, struct buffer **to_server_p,
                           struct buffer **from_server_p)
{
    /* Note that generally speaking we do *not* fall back to a different
       way of connecting if the first one does not work.  This is slow
       (*really* slow on a 14.4kbps link); the clean way to have a CVS
       which supports several ways of connecting is with access methods.  */

    TRACE (TRACE_FUNCTION, "open_connection_to_server (%s)", root->original);

    switch (root->method)
    {
	case pserver_method:
#ifdef AUTH_CLIENT_SUPPORT
	    /* Toss the return value.  It will die with an error message if
	     * anything goes wrong anyway.
	     */
	    connect_to_pserver (root, to_server_p, from_server_p, 0, 0);
#else /* AUTH_CLIENT_SUPPORT */
	    error (0, 0, "CVSROOT is set for a pserver access method but your");
	    error (1, 0, "CVS executable doesn't support it.");
#endif /* AUTH_CLIENT_SUPPORT */
	    break;

	case kserver_method:
#if HAVE_KERBEROS
	    start_kerberos4_server (root, to_server_p, 
                                    from_server_p);
#else /* !HAVE_KERBEROS */
	    error (0, 0,
	           "CVSROOT is set for a kerberos access method but your");
	    error (1, 0, "CVS executable doesn't support it.");
#endif /* HAVE_KERBEROS */
	    break;

	case gserver_method:
#ifdef HAVE_GSSAPI
	    /* GSSAPI authentication is handled by the pserver.  */
	    connect_to_pserver (root, to_server_p, from_server_p, 0, 1);
#else /* !HAVE_GSSAPI */
	    error (0, 0, "CVSROOT is set for a GSSAPI access method but your");
	    error (1, 0, "CVS executable doesn't support it.");
#endif /* HAVE_GSSAPI */
	    break;

	case ext_method:
	case extssh_method:
#ifdef NO_EXT_METHOD
	    error (0, 0, ":ext: method not supported by this port of CVS");
	    error (1, 0, "try :server: instead");
#else /* ! NO_EXT_METHOD */
	    start_rsh_server (root, to_server_p,
                              from_server_p);
#endif /* NO_EXT_METHOD */
	    break;

	case server_method:
#ifdef START_SERVER
	    {
	    int tofd, fromfd;
	    START_SERVER (&tofd, &fromfd, getcaller (),
			  root->username,
                          root->hostname,
			  root->directory);
# ifdef START_SERVER_RETURNS_SOCKET
	    make_bufs_from_fds (tofd, fromfd, 0, root, to_server_p,
                                from_server_p, 1);
# else /* ! START_SERVER_RETURNS_SOCKET */
	    make_bufs_from_fds (tofd, fromfd, 0, root, to_server_p,
                                from_server_p, 0);
# endif /* START_SERVER_RETURNS_SOCKET */
	    }
#else /* ! START_SERVER */
	    /* FIXME: It should be possible to implement this portably,
	       like pserver, which would get rid of the duplicated code
	       in {vms,windows-NT,...}/startserver.c.  */
	    error (1, 0,
"the :server: access method is not supported by this port of CVS");
#endif /* START_SERVER */
	    break;

        case fork_method:
	    connect_to_forked_server (root, to_server_p, from_server_p);
	    break;

	default:
	    error (1, 0,
                   "(start_server internal error): unknown access method");
	    break;
    }

    /* "Hi, I'm Darlene and I'll be your server tonight..." */
    server_started = 1;
}



/* Contact the server.  */
void
start_server (void)
{
    bool rootless;
    int status;
    bool have_global;

    do
    {
	/* Clear our static variables for this invocation. */
	if (toplevel_repos)
	    free (toplevel_repos);
	toplevel_repos = NULL;

	open_connection_to_server (current_parsed_root, &global_to_server,
				   &global_from_server);
	setup_logfiles ("CVS_CLIENT_LOG", &global_to_server,
			&global_from_server);

	/* Clear static variables.  */
	if (toplevel_repos)
	{
	    free (toplevel_repos);
	    toplevel_repos = NULL;
	}
	if (last_repos)
	{
	    free (last_repos);
	    last_repos = NULL;
	}
	if (last_update_dir)
	{
	    free (last_update_dir);
	    last_update_dir = NULL;
	}
	stored_checksum_valid = 0;
	if (stored_mode)
	{
	    free (stored_mode);
	    stored_mode = NULL;
	}

	rootless = STREQ (cvs_cmd_name, "init");
	if (!rootless)
	{
	    send_to_server ("Root ", 0);
	    send_to_server (current_parsed_root->directory, 0);
	    send_to_server ("\012", 1);
	}

	{
	    struct response *rs;
	    bool suppress_redirect = !current_parsed_root->redirect;

	    send_to_server ("Valid-responses", 0);

	    for (rs = responses; rs->name; ++rs)
	    {
		if (suppress_redirect && STREQ (rs->name, "Redirect"))
		    continue;
		if (suppress_bases && STRNEQ (rs->name, "Base-", 5))
		    continue;
		if (suppress_bases && STREQ (rs->name, "OpenPGP-signature"))
		    continue;
		if (suppress_bases && STREQ (rs->name, "Temp-checkout"))
		    continue;

		send_to_server (" ", 0);
		send_to_server (rs->name, 0);
	    }
	    send_to_server ("\012", 1);
	}
	send_to_server ("valid-requests\012", 0);

	if (get_server_responses ())
	    exit (EXIT_FAILURE);

	have_global = supported_request ("Global_option");

	/* Encryption needs to come before compression.  Good encryption can
	 * render compression useless in the other direction.
	 */
	if (cvsencrypt && !rootless)
	{
#ifdef ENCRYPTION
	    /* Turn on encryption before turning on compression.  We do
	     * not want to try to compress the encrypted stream.  Instead,
	     * we want to encrypt the compressed stream.  If we can't turn
	     * on encryption, bomb out; don't let the user think the data
	     * is being encrypted when it is not.
	     */
#  ifdef HAVE_KERBEROS
	    if (current_parsed_root->method == kserver_method)
	    {
		if (!supported_request ("Kerberos-encrypt"))
		    error (1, 0, "This server does not support encryption");
		send_to_server ("Kerberos-encrypt\012", 0);
	       initialize_kerberos4_encryption_buffers (&global_to_server,
							&global_from_server);
	    }
	    else
#  endif /* HAVE_KERBEROS */
#  ifdef HAVE_GSSAPI
	    if (current_parsed_root->method == gserver_method)
	    {
		if (!supported_request ("Gssapi-encrypt"))
		    error (1, 0, "This server does not support encryption");
		send_to_server ("Gssapi-encrypt\012", 0);
		initialize_gssapi_buffers (&global_to_server,
					   &global_from_server);
		cvs_gssapi_encrypt = 1;
	    }
	    else
#  endif /* HAVE_GSSAPI */
		error (1, 0,
"Encryption is only supported when using GSSAPI or Kerberos");
#else /* ! ENCRYPTION */
	    error (1, 0, "This client does not support encryption");
#endif /* ! ENCRYPTION */
	}

	/* Send this before compression to enable supression of the
	 * "Forcing compression level Z" messages.
	 */
	if (quiet)
	{
	    if (have_global)
	    {
		send_to_server ("Global_option -q\012", 0);
	    }
	    else
		error (1, 0,
		       "This server does not support the global -q option.");
	}
	if (really_quiet)
	{
	    if (have_global)
	    {
		send_to_server ("Global_option -Q\012", 0);
	    }
	    else
		error (1, 0,
		       "This server does not support the global -Q option.");
	}

	/* Compression needs to come before any of the rooted requests to
	 * work with compression limits.
	 */
	if (!rootless && (gzip_level || force_gzip))
	{
	    if (supported_request ("Gzip-stream"))
	    {
		char *gzip_level_buf = Xasprintf ("%d", gzip_level);
		send_to_server ("Gzip-stream ", 0);
		send_to_server (gzip_level_buf, 0);
		free (gzip_level_buf);
		send_to_server ("\012", 1);

		/* All further communication with the server will be
		   compressed.  */

		global_to_server =
		    compress_buffer_initialize (global_to_server, 0,
					        gzip_level, NULL);
		global_from_server =
		    compress_buffer_initialize (global_from_server, 1,
						gzip_level, NULL);
	    }
#ifndef NO_CLIENT_GZIP_PROCESS
	    else if (supported_request ("gzip-file-contents"))
	    {
		char *gzip_level_buf = Xasprintf ("%d", gzip_level);
		send_to_server ("gzip-file-contents ", 0);
		send_to_server (gzip_level_buf, 0);
		free (gzip_level_buf);
		send_to_server ("\012", 1);

		file_gzip_level = gzip_level;
	    }
#endif
	    else
	    {
		fprintf (stderr, "server doesn't support gzip-file-contents\n");
		/* Setting gzip_level to 0 prevents us from giving the
		   error twice if update has to contact the server again
		   to fetch unpatchable files.  */
		gzip_level = 0;
	    }
	}

	if (client_referrer && supported_request ("Referrer"))
	{
	    send_to_server ("Referrer ", 0);
	    send_to_server (client_referrer->original, 0);
	    send_to_server ("\012", 0);
	}

	/* FIXME: I think we should still be sending this for init.  */
	if (!rootless && supported_request ("Command-prep"))
	{
	    send_to_server ("Command-prep ", 0);
	    send_to_server (cvs_cmd_name, 0);
	    send_to_server ("\012", 0);
	    status = get_server_responses ();
	    if (status == 1) exit (EXIT_FAILURE);
	    if (status == 2) close_connection_to_server (&global_to_server,
							 &global_from_server);
	}
	else status = 0;
    } while (status == 2);


    /*
     * Now handle global options.
     *
     * -H, -f, -d, -e should be handled OK locally.
     *
     * -b we ignore (treating it as a server installation issue).
     * FIXME: should be an error message.
     *
     * -v we print local version info; FIXME: Add a protocol request to get
     * the version from the server so we can print that too.
     *
     * -l -t -r -w -q -n and -Q need to go to the server.
     */
    if (noexec)
    {
	if (have_global)
	{
	    send_to_server ("Global_option -n\012", 0);
	}
	else
	    error (1, 0,
		   "This server does not support the global -n option.");
    }
    if (!cvswrite)
    {
	if (have_global)
	{
	    send_to_server ("Global_option -r\012", 0);
	}
	else
	    error (1, 0,
		   "This server does not support the global -r option.");
    }
    if (trace)
    {
	if (have_global)
	{
	    int count = trace;
	    while (count--) send_to_server ("Global_option -t\012", 0);
	}
	else
	    error (1, 0,
		   "This server does not support the global -t option.");
    }

    /* Find out about server-side cvswrappers.  An extra network
       turnaround for cvs import seems to be unavoidable, unless we
       want to add some kind of client-side place to configure which
       filenames imply binary.  For cvs add, we could avoid the
       problem by keeping a copy of the wrappers in CVSADM (the main
       reason to bother would be so we could make add work without
       contacting the server, I suspect).  */

    if (STREQ (cvs_cmd_name, "import") || STREQ (cvs_cmd_name, "add"))
    {
	if (supported_request ("wrapper-sendme-rcsOptions"))
	{
	    int err;
	    send_to_server ("wrapper-sendme-rcsOptions\012", 0);
	    err = get_server_responses ();
	    if (err != 0)
		error (err, 0, "error reading from server");
	}
    }

    if (cvsauthenticate && ! cvsencrypt && !rootless)
    {
	/* Turn on authentication after turning on compression, so
	   that we can compress the authentication information.  We
	   assume that encrypted data is always authenticated--the
	   ability to decrypt the data stream is itself a form of
	   authentication.  */
#ifdef HAVE_GSSAPI
	if (current_parsed_root->method == gserver_method)
	{
	    if (! supported_request ("Gssapi-authenticate"))
		error (1, 0,
		       "This server does not support stream authentication");
	    send_to_server ("Gssapi-authenticate\012", 0);
	    initialize_gssapi_buffers(&global_to_server, &global_from_server);

	}
	else
	    error (1, 0, "Stream authentication is only supported when using GSSAPI");
#else /* ! HAVE_GSSAPI */
	error (1, 0, "This client does not support stream authentication");
#endif /* ! HAVE_GSSAPI */
    }

    /* If "Set" is not supported, just silently fail to send the variables.
       Users with an old server should get a useful error message when it
       fails to recognize the ${=foo} syntax.  This way if someone uses
       several servers, some of which are new and some old, they can still
       set user variables in their .cvsrc without trouble.  */
    if (supported_request ("Set"))
	walklist (variable_list, send_variable_proc, NULL);
}



/* Send an argument STRING.  */
void
send_arg (const char *string)
{
    const char *p = string;

    send_to_server ("Argument ", 0);

    while (*p)
    {
	if (*p == '\n')
	    send_to_server ("\012Argumentx ", 0);
	else
	    send_to_server (p, 1);
	++p;
    }
    send_to_server ("\012", 1);
}



/* VERS->OPTIONS specifies whether the file is binary or not.  NOTE: BEFORE
   using any other fields of the struct vers, we would need to fix
   client_process_import_file to set them up.  */
static void
send_modified (const char *file, const char *short_pathname, Vers_TS *vers)
{
    /* File was modified, send it.  */
    struct stat sb;
    int fd;
    unsigned char *buf;
    char *mode_string;
    size_t bufsize;
    int bin;

    TRACE (TRACE_FUNCTION, "Sending file `%s' to server", file);

    /* Don't think we can assume fstat exists.  */
    if (stat (file, &sb) < 0)
	error (1, errno, "reading %s", short_pathname);

    mode_string = mode_to_string (sb.st_mode);

    /* Beware: on systems using CRLF line termination conventions,
       the read and write functions will convert CRLF to LF, so the
       number of characters read is not the same as sb.st_size.  Text
       files should always be transmitted using the LF convention, so
       we don't want to disable this conversion.  */
    bufsize = sb.st_size;
    buf = xmalloc (bufsize);

    /* Is the file marked as containing binary data by the "-kb" flag?
       If so, make sure to open it in binary mode: */

    if (vers && vers->options)
      bin = STREQ (vers->options, "-kb");
    else
      bin = 0;

#ifdef BROKEN_READWRITE_CONVERSION
    if (!bin)
    {
	/* If only stdio, not open/write/etc., do text/binary
	   conversion, use convert_file which can compensate
	   (FIXME: we could just use stdio instead which would
	   avoid the whole problem).  */
	char *tfile = Xasprintf ("%s.CVSBFCTMP", file);
	convert_file (file, O_RDONLY,
		      tfile, O_WRONLY | O_CREAT | O_TRUNC | OPEN_BINARY);
	fd = CVS_OPEN (tfile, O_RDONLY | OPEN_BINARY);
	if (fd < 0)
	    error (1, errno, "reading %s", short_pathname);
	free (tfile);
    }
    else
	fd = CVS_OPEN (file, O_RDONLY | OPEN_BINARY);
#else
    fd = CVS_OPEN (file, O_RDONLY | (bin ? OPEN_BINARY : 0));
#endif

    if (fd < 0)
	error (1, errno, "reading %s", short_pathname);

    if (file_gzip_level && sb.st_size > 100)
    {
	size_t newsize = 0;

	if (read_and_gzip (fd, short_pathname, &buf,
			   &bufsize, &newsize,
			   file_gzip_level))
	    error (1, 0, "aborting due to compression error");

	if (close (fd) < 0)
	    error (0, errno, "warning: can't close %s", short_pathname);

        {
          char tmp[80];

	  send_to_server ("Modified ", 0);
	  send_to_server (file, 0);
	  send_to_server ("\012", 1);
	  send_to_server (mode_string, 0);
	  send_to_server ("\012z", 2);
	  sprintf (tmp, "%lu\n", (unsigned long) newsize);
	  send_to_server (tmp, 0);

          send_to_server ((char *) buf, newsize);
        }
    }
    else
    {
    	int newsize;

        {
	    unsigned char *bufp = buf;
	    int len;

	    /* FIXME: This is gross.  It assumes that we might read
	       less than st_size bytes (true on NT), but not more.
	       Instead of this we should just be reading a block of
	       data (e.g. 8192 bytes), writing it to the network, and
	       so on until EOF.  */
	    while ((len = read (fd, bufp, (buf + sb.st_size) - bufp)) > 0)
	        bufp += len;

	    if (len < 0)
	        error (1, errno, "reading %s", short_pathname);

	    newsize = bufp - buf;
	}
	if (close (fd) < 0)
	    error (0, errno, "warning: can't close %s", short_pathname);

        {
          char tmp[80];

	  send_to_server ("Modified ", 0);
	  send_to_server (file, 0);
	  send_to_server ("\012", 1);
	  send_to_server (mode_string, 0);
	  send_to_server ("\012", 1);
          sprintf (tmp, "%lu\012", (unsigned long) newsize);
          send_to_server (tmp, 0);
        }
#ifdef BROKEN_READWRITE_CONVERSION
	if (!bin)
	{
	    char *tfile = Xasprintf ("%s.CVSBFCTMP", file);
	    if (CVS_UNLINK (tfile) < 0)
		error (0, errno, "warning: can't remove temp file %s", tfile);
	    free (tfile);
	}
#endif

	/*
	 * Note that this only ends with a newline if the file ended with
	 * one.
	 */
	if (newsize > 0)
	    send_to_server ((char *) buf, newsize);
    }
    free (buf);
    free (mode_string);
}



/* Generate and send an OpenPGP signature to the server.
 */
static void
send_signature (const char *srepos, const char *filename, const char *fullname,
		bool bin)
{
    char *sigbuf;
    size_t len;
    Node *n;

    TRACE (TRACE_FUNCTION, "send_signature (%s, %s, %s, %s)",
	   srepos, filename, fullname, TRACE_BOOL (bin));

    sigbuf = gen_signature (srepos, filename, bin, &len);

    send_to_server ("Signature\012", 0);
    send_to_server (sigbuf, len);

    /* Cache the signature for use with the base file which will be
     * automatically generated later since the server will not send a new base
     * file and signature unless keywords cause the file to change after it is
     * committed.
     */
    if (!sig_cache) sig_cache = getlist ();
    n = getnode ();
    n->key = xstrdup (fullname);
    n->data = sigbuf;
    n->len = len;
    addnode (sig_cache, n);
}



/* The address of an instance of this structure is passed to
 * send_fileproc, send_filesdoneproc, and send_dirent_proc, as the
 * callerdat parameter.
 */
struct send_data
{
    /* Each of the following flags are zero for clear or nonzero for set.  */
    bool build_dirs;
    bool force;
    bool no_contents;
    bool backup_modified;
    bool force_signatures;
};

/* Deal with one file.  */
static int
send_fileproc (void *callerdat, struct file_info *finfo)
{
    struct send_data *args = callerdat;
    Vers_TS *vers;
    struct file_info xfinfo;
    /* File name to actually use.  Might differ in case from
       finfo->file.  */
    const char *filename;
    bool may_be_modified;

    TRACE (TRACE_FLOW, "send_fileproc (%s)", finfo->fullname);

    send_a_repository ("", finfo->repository, finfo->update_dir);

    xfinfo = *finfo;
    xfinfo.repository = NULL;
    xfinfo.rcs = NULL;
    vers = Version_TS (&xfinfo, NULL, NULL, NULL, 0, 0);

    if (vers->entdata)
	filename = vers->entdata->user;
    else
	filename = finfo->file;

    if (vers->vn_user)
    {
	/* The Entries request.  */
	send_to_server ("Entry /", 0);
	send_to_server (filename, 0);
	send_to_server ("/", 0);
	send_to_server (vers->vn_user, 0);
	send_to_server ("/", 0);
	if (vers->ts_conflict)
	{
	    if (vers->ts_user && STREQ (vers->ts_conflict, vers->ts_user))
		send_to_server ("+=", 0);
	    else
		send_to_server ("+modified", 0);
	}
	send_to_server ("/", 0);
	send_to_server (vers->entdata ? vers->entdata->options : vers->options,
			0);
	send_to_server ("/", 0);
	if (vers->entdata && vers->entdata->tag)
	{
	    send_to_server ("T", 0);
	    send_to_server (vers->entdata->tag, 0);
	}
	else if (vers->entdata && vers->entdata->date)
          {
	    send_to_server ("D", 0);
	    send_to_server (vers->entdata->date, 0);
          }
	send_to_server ("\012", 1);
    }
    else
    {
	/* It seems a little silly to re-read this on each file, but
	   send_dirent_proc doesn't get called if filenames are specified
	   explicitly on the command line.  */
	wrap_add_file (CVSDOTWRAPPER, 1);

	if (wrap_name_has (filename, WRAP_RCSOPTION))
	{
	    /* No "Entry", but the wrappers did give us a kopt so we better
	       send it with "Kopt".  As far as I know this only happens
	       for "cvs add".  Question: is there any reason why checking
	       for options from wrappers isn't done in Version_TS?

	       Note: it might have been better to just remember all the
	       kopts on the client side, rather than send them to the server,
	       and have it send us back the same kopts.  But that seemed like
	       a bigger change than I had in mind making now.  */

	    if (supported_request ("Kopt"))
	    {
		char *opt;

		send_to_server ("Kopt ", 0);
		opt = wrap_rcsoption (filename, 1);
		send_to_server (opt, 0);
		send_to_server ("\012", 1);
		free (opt);
	    }
	    else
		error (0, 0, "\
warning: ignoring -k options due to server limitations");
	}
    }

    if (!vers->ts_user)
    {
	/*
	 * Do we want to print "file was lost" like normal CVS?
	 * Would it always be appropriate?
	 */
	/* File no longer exists.  Don't do anything, missing files
	   just happen.  */
    }
    else if (!vers->ts_rcs || args->force)
	may_be_modified = true;
    else if (!STREQ (vers->ts_conflict && supported_request ("Empty-conflicts")
		     ? vers->ts_conflict : vers->ts_rcs, vers->ts_user)
	     || (vers->ts_conflict && STREQ (cvs_cmd_name, "diff")))
    {
	char *basefn = make_base_file_name (filename, vers->ts_user);
	if (!isfile (basefn) || xcmp (filename, basefn))
	    may_be_modified = true;
	else
	    may_be_modified = false;
	free (basefn);
    }
    else
    {
	may_be_modified = false;
    }

    if (vers->ts_user)
    {
	if (may_be_modified)
	{
	    if (args->force_signatures
		|| (STREQ (cvs_cmd_name, "commit")
		    && get_sign_commits (supported_request ("Signature"))))
	    {
		if (!supported_request ("Signature"))
		    error (1, 0, "Server doesn't support commit signatures.");

		send_signature (Short_Repository (finfo->repository),
				finfo->file, finfo->fullname,
				vers && STREQ (vers->options, "-kb"));
	    }

	    if (args->no_contents
		&& supported_request ("Is-modified"))
	    {
		send_to_server ("Is-modified ", 0);
		send_to_server (filename, 0);
		send_to_server ("\012", 1);
	    }
	    else
		send_modified (filename, finfo->fullname, vers);

	    if (args->backup_modified)
	    {
		char *bakname;
		bakname = backup_file (filename, vers->vn_user);
		/* This behavior is sufficiently unexpected to
		   justify overinformativeness, I think. */
		if (! really_quiet)
		    printf ("(Locally modified %s moved to %s)\n",
			    filename, bakname);
		free (bakname);
	    }
	}
	else
	{
	    if (args->force_signatures)
	    {
		if (!supported_request ("Signature"))
		    error (1, 0, "Server doesn't support commit signatures.");

		send_signature (Short_Repository (finfo->repository),
				finfo->file, finfo->fullname,
				vers && STREQ (vers->options, "-kb"));
	    }

	    send_to_server ("Unchanged ", 0);
	    send_to_server (filename, 0);
	    send_to_server ("\012", 1);
	}
    }

    /* if this directory has an ignore list, add this file to it */
    if (ignlist)
    {
	Node *p;

	p = getnode ();
	p->type = FILES;
	p->key = xstrdup (finfo->file);
	(void) addnode (ignlist, p);
    }

    freevers_ts (&vers);
    return 0;
}



static void
send_ignproc (const char *file, const char *dir)
{
    if (ign_inhibit_server || !supported_request ("Questionable"))
    {
	if (dir[0] != '\0')
	    (void) printf ("? %s/%s\n", dir, file);
	else
	    (void) printf ("? %s\n", file);
    }
    else
    {
	send_to_server ("Questionable ", 0);
	send_to_server (file, 0);
	send_to_server ("\012", 1);
    }
}



static int
send_filesdoneproc (void *callerdat, int err, const char *repository,
                    const char *update_dir, List *entries)
{
    /* if this directory has an ignore list, process it then free it */
    if (ignlist)
    {
	ignore_files (ignlist, entries, update_dir, send_ignproc);
	dellist (&ignlist);
    }

    return err;
}



/*
 * send_dirent_proc () is called back by the recursion processor before a
 * sub-directory is processed for update.
 * A return code of 0 indicates the directory should be
 * processed by the recursion code.  A return of non-zero indicates the
 * recursion code should skip this directory.
 *
 */
static Dtype
send_dirent_proc (void *callerdat, const char *dir, const char *repository,
                  const char *update_dir, List *entries)
{
    struct send_data *args = callerdat;
    bool dir_exists;

    TRACE (TRACE_FLOW, "send_dirent_proc (%s, %s, %s)",
	   dir, repository, update_dir);

    if (ignore_directory (NULL2DOT (update_dir)))
    {
	/* print the warm fuzzy message */
	if (!quiet)
	    error (0, 0, "Ignoring %s", NULL2DOT (update_dir));
        return R_SKIP_ALL;
    }

    /*
     * If the directory does not exist yet (e.g. "cvs update -d foo"),
     * no need to send any files from it.  If the directory does not
     * have a CVS directory, then we pretend that it does not exist.
     * Otherwise, we will fail when trying to open the Entries file.
     * This case will happen when checking out a module defined as
     * ``-a .''.
     */
    dir_exists = hasAdmin (dir);

    /*
     * If there is an empty directory (e.g. we are doing `cvs add' on a
     * newly-created directory), the server still needs to know about it.
     */

    if (dir_exists)
    {
	/*
	 * Get the repository from a CVS/Repository file whenever possible.
	 * The repository variable is wrong if the names in the local
	 * directory don't match the names in the repository.
	 */
	char *repos = Name_Repository (dir, update_dir);
	send_a_repository (dir, repos, update_dir);
	free (repos);

	/* initialize the ignore list for this directory */
	ignlist = getlist ();
    }
    else
    {
	/* It doesn't make sense to send a non-existent directory,
	   because there is no way to get the correct value for
	   the repository (I suppose maybe via the expand-modules
	   request).  In the case where the "obvious" choice for
	   repository is correct, the server can figure out whether
	   to recreate the directory; in the case where it is wrong
	   (that is, does not match what modules give us), we might as
	   well just fail to recreate it.

	   Checking for noexec is a kludge for "cvs -n add dir".  */
	/* Don't send a non-existent directory unless we are building
           new directories (build_dirs is true).  Otherwise, CVS may
           see a D line in an Entries file, and recreate a directory
           which the user removed by hand.  */
	if (args->build_dirs && noexec)
	    send_a_repository (dir, repository, update_dir);
    }

    return dir_exists ? R_PROCESS : R_SKIP_ALL;
}



/*
 * send_dirleave_proc () is called back by the recursion code upon leaving
 * a directory.  All it does is delete the ignore list if it hasn't already
 * been done (by send_filesdone_proc).
 */
/* ARGSUSED */
static int
send_dirleave_proc (void *callerdat, const char *dir, int err,
                    const char *update_dir, List *entries )
{

    /* Delete the ignore list if it hasn't already been done.  */
    if (ignlist)
	dellist (&ignlist);
    return err;
}



/*
 * Send each option in an array to the server, one by one.
 * argv might be "--foo=bar",  "-C", "5", "-y".
 */

void
send_options (int argc, char * const *argv)
{
    int i;
    for (i = 0; i < argc; i++)
	send_arg (argv[i]);
}



/* Send the names of all the argument files to the server.  */
void
send_file_names (int argc, char **argv, unsigned int flags)
{
    int i;

    TRACE (TRACE_FUNCTION, "send_file_names (%d, %s, %u)",
	   argc, TRACE_PTR (argv, 0), flags);
   
    /* The fact that we do this here as well as start_recursion is a bit 
       of a performance hit.  Perhaps worth cleaning up someday.  */
    if (flags & SEND_EXPAND_WILD)
	expand_wild (argc, argv, &argc, &argv);

    for (i = 0; i < argc; ++i)
    {
	char buf[1];
	char *p;
#ifdef FILENAMES_CASE_INSENSITIVE
	char *line = NULL;
#endif /* FILENAMES_CASE_INSENSITIVE */

	if (arg_should_not_be_sent_to_server (argv[i]))
	    continue;

#ifdef FILENAMES_CASE_INSENSITIVE
	/* We want to send the path as it appears in the
	   CVS/Entries files.  We put this inside an ifdef
	   to avoid doing all these system calls in
	   cases where fncmp is just strcmp anyway.  */
	/* The isdir (CVSADM) check could more gracefully be replaced
	   with a way of having Entries_Open report back the
	   error to us and letting us ignore existence_error.
	   Or some such.  */
	{
	    List *stack;
	    size_t line_len = 0;
	    char *q, *r;
	    struct saved_cwd sdir;
	    char *update_dir;

	    /* Split the argument onto the stack.  */
	    stack = getlist();
	    r = xstrdup (argv[i]);
	    while ((q = last_component (r)) != r && *q)
	    {
		push (stack, xstrdup (q));
		*--q = '\0';
	    }
	    push (stack, r);

	    /* Normalize the path into outstr. */
	    save_cwd (&sdir);
	    update_dir = xstrdup ("");
	    while (q = pop (stack))
	    {
		Node *node = NULL;
	        if (isdir (CVSADM))
		{
		    List *entries;

		    /* Note that if we are adding a directory,
		       the following will read the entry
		       that we just wrote there, that is, we
		       will get the case specified on the
		       command line, not the case of the
		       directory in the filesystem.  This
		       is correct behavior.  */
		    entries = Entries_Open (0, update_dir);
		    node = findnode_fn (entries, q);
		    if (node)
		    {
			/* Add the slash unless this is our first element. */
			if (line_len)
			    xrealloc_and_strcat (&line, &line_len, "/");
			xrealloc_and_strcat (&line, &line_len, node->key);
			delnode (node);
		    }
		    Entries_Close (entries, update_dir);
		}

		/* If node is still NULL then we either didn't find CVSADM or
		 * we didn't find an entry there.
		 */
		if (!node)
		{
		    /* Add the slash unless this is our first element. */
		    if (line_len)
			xrealloc_and_strcat (&line, &line_len, "/");
		    xrealloc_and_strcat (&line, &line_len, q);
		    break;
		}

		/* And descend the tree. */
		if (isdir (q))
		{
		    char *tmp_update_dir = dir_append (update_dir, q);
		    free (update_dir);
		    update_dir = tmp_update_dir;
		    CVS_CHDIR (q);
		}
		free (q);
	    }
	    restore_cwd (&sdir);
	    free (update_dir);
	    free_cwd (&sdir);

	    /* Now put everything we didn't find entries for back on. */
	    while (q = pop (stack))
	    {
		if (line_len)
		    xrealloc_and_strcat (&line, &line_len, "/");
		xrealloc_and_strcat (&line, &line_len, q);
		free (q);
	    }

	    p = line;

	    dellist (&stack);
	}
#else /* !FILENAMES_CASE_INSENSITIVE */
	p = argv[i];
#endif /* FILENAMES_CASE_INSENSITIVE */

	send_to_server ("Argument ", 0);

	while (*p)
	{
	    if (*p == '\n')
	    {
		send_to_server ("\012Argumentx ", 0);
	    }
	    else if (ISSLASH (*p))
	    {
		buf[0] = '/';
		send_to_server (buf, 1);
	    }
	    else
	    {
		buf[0] = *p;
		send_to_server (buf, 1);
	    }
	    ++p;
	}
	send_to_server ("\012", 1);
#ifdef FILENAMES_CASE_INSENSITIVE
	free (line);
#endif /* FILENAMES_CASE_INSENSITIVE */
    }

    if (flags & SEND_EXPAND_WILD)
    {
	int i;
	for (i = 0; i < argc; ++i)
	    free (argv[i]);
	free (argv);
    }
}



/* Calculate and send max-dotdot to the server */
static void
send_max_dotdot (argc, argv)
    int argc;
    char **argv;
{
    int i;
    int level = 0;
    int max_level = 0;

    /* Send Max-dotdot if needed.  */
    for (i = 0; i < argc; ++i)
    {
        level = pathname_levels (argv[i]);
	if (level > 0)
	{
            if (!uppaths) uppaths = getlist();
	    push_string (uppaths, xstrdup (argv[i]));
	}
        if (level > max_level)
            max_level = level;
    }

    if (max_level > 0)
    {
        if (supported_request ("Max-dotdot"))
        {
            char buf[10];
            sprintf (buf, "%d", max_level);

            send_to_server ("Max-dotdot ", 0);
            send_to_server (buf, 0);
            send_to_server ("\012", 1);
        }
        else
        {
            error (1, 0,
"backreference in path (`..') not supported by old (pre-Max-dotdot) servers");
        }
    }
}



/*
 * Send Repository, Modified and Entry.  Also sends Argument lines for argc
 * and argv, so should be called after options are sent.  
 *
 * ARGUMENTS
 *   argc	# of files to operate on (0 for everything).
 *   argv	Paths to file to operate on.
 *   local	nonzero if we should not recurse (-l option).
 *   flags	FLAGS & SEND_BUILD_DIRS if nonexistent directories should be
 *		sent.
 *		FLAGS & SEND_FORCE if we should send unmodified files to the
 *		server as though they were modified.
 *		FLAGS & SEND_NO_CONTENTS means that this command only needs to
 *		know _whether_ a file is modified, not the contents.
 *		FLAGS & FORCE_SIGNATURES means that OpenPGP signatures should
 *		be sent with files regardless of other settings, including
 *		server support.
 *
 * RETURNS
 *   Nothing.
 */
void
send_files (int argc, char **argv, int local, int aflag, unsigned int flags)
{
    struct send_data args;
    int err;

    TRACE (TRACE_FUNCTION, "send_files (%d, %s, %d, %d, %u)",
	   argc, TRACE_PTR (argv, 0), local, aflag, flags);

    send_max_dotdot (argc, argv);

    /*
     * aflag controls whether the tag/date is copied into the vers_ts.
     * But we don't actually use it, so I don't think it matters what we pass
     * for aflag here.
     */
    args.build_dirs = flags & SEND_BUILD_DIRS;
    args.force = flags & SEND_FORCE;
    args.no_contents = flags & SEND_NO_CONTENTS;
    args.backup_modified = flags & BACKUP_MODIFIED_FILES;
    args.force_signatures = flags & FORCE_SIGNATURES;
    err = start_recursion
	(send_fileproc, send_filesdoneproc, send_dirent_proc,
         send_dirleave_proc, &args, argc, argv, local, W_LOCAL, aflag,
         CVS_LOCK_NONE, NULL, 0, NULL);
    if (err)
	exit (EXIT_FAILURE);
    if (!toplevel_repos)
	/*
	 * This happens if we are not processing any files,
	 * or for checkouts in directories without any existing stuff
	 * checked out.  The following assignment is correct for the
	 * latter case; I don't think toplevel_repos matters for the
	 * former.
	 */
	toplevel_repos = xstrdup (current_parsed_root->directory);
    send_repository ("", toplevel_repos, ".");
}



void
client_import_setup (char *repository)
{
    if (!toplevel_repos)		/* should always be true */
        send_a_repository ("", repository, "");
}



/*
 * Process the argument import file.
 */
int
client_process_import_file (char *message, char *vfile, char *vtag, int targc,
                            char *targv[], char *repository,
                            int all_files_binary,
                            int modtime /* Nonzero for "import -d".  */ )
{
    char *update_dir;
    char *fullname;
    Vers_TS vers;

    assert (toplevel_repos);

    if (!STRNEQ (repository, toplevel_repos, strlen (toplevel_repos)))
	error (1, 0,
	       "internal error: pathname `%s' doesn't specify file in `%s'",
	       repository, toplevel_repos);

    if (STREQ (repository, toplevel_repos))
    {
	update_dir = "";
	fullname = xstrdup (vfile);
    }
    else
    {
	update_dir = repository + strlen (toplevel_repos) + 1;

	fullname = Xasprintf ("%s/%s", update_dir, vfile);
    }

    send_a_repository ("", repository, update_dir);
    if (all_files_binary)
	vers.options = xstrdup ("-kb");
    else
	vers.options = wrap_rcsoption (vfile, 1);

    if (vers.options)
    {
	if (supported_request ("Kopt"))
	{
	    send_to_server ("Kopt ", 0);
	    send_to_server (vers.options, 0);
	    send_to_server ("\012", 1);
	}
	else
	    error (0, 0,
		   "warning: ignoring -k options due to server limitations");
    }
    if (modtime)
    {
	if (supported_request ("Checkin-time"))
	{
	    struct stat sb;
	    char *rcsdate;
	    char netdate[MAXDATELEN];

	    if (stat (vfile, &sb) < 0)
		error (1, errno, "cannot stat %s", fullname);
	    rcsdate = date_from_time_t (sb.st_mtime);
	    date_to_internet (netdate, rcsdate);
	    free (rcsdate);

	    send_to_server ("Checkin-time ", 0);
	    send_to_server (netdate, 0);
	    send_to_server ("\012", 1);
	}
	else
	    error (0, 0,
		   "warning: ignoring -d option due to server limitations");
    }

    /* Send signature.  */
    if (get_sign_commits (supported_request ("Signature")))
    {
	if (!supported_request ("Signature"))
	    error (1, 0, "Server doesn't support commit signatures.");

	send_signature (Short_Repository (repository), vfile, fullname,
			vers.options && STREQ (vers.options, "-kb"));
    }

    send_modified (vfile, fullname, &vers);
    if (vers.options)
	free (vers.options);
    free (fullname);
    return 0;
}



void
client_import_done (void)
{
    if (!toplevel_repos)
	/*
	 * This happens if we are not processing any files,
	 * or for checkouts in directories without any existing stuff
	 * checked out.  The following assignment is correct for the
	 * latter case; I don't think toplevel_repos matters for the
	 * former.
	 */
        /* FIXME: "can't happen" now that we call client_import_setup
	   at the beginning.  */
	toplevel_repos = xstrdup (current_parsed_root->directory);
    send_repository ("", toplevel_repos, ".");
}



void
client_notify (const char *repository, const char *update_dir,
               const char *filename, int notif_type, const char *val)
{
    char buf[2];

    send_a_repository ("", repository, update_dir);
    send_to_server ("Notify ", 0);
    send_to_server (filename, 0);
    send_to_server ("\012", 1);
    buf[0] = notif_type;
    buf[1] = '\0';
    send_to_server (buf, 1);
    send_to_server ("\t", 1);
    send_to_server (val, 0);
}



/*
 * Send an option with an argument, dealing correctly with newlines in
 * the argument.  If ARG is NULL, forget the whole thing.
 */
void
option_with_arg (const char *option, const char *arg)
{
    if (!arg)
	return;

    send_to_server ("Argument ", 0);
    send_to_server (option, 0);
    send_to_server ("\012", 1);

    send_arg (arg);
}



/* Send a date to the server.  The input DATE is in RCS format.
   The time will be GMT.

   We then convert that to the format required in the protocol
   (including the "-D" option) and send it.  According to
   cvsclient.texi, RFC 822/1123 format is preferred.  */
void
client_senddate (const char *date)
{
    char buf[MAXDATELEN];

    date_to_internet (buf, date);
    option_with_arg ("-D", buf);
}



void
send_init_command (void)
{
    /* This is here because we need the current_parsed_root->directory variable.  */
    send_to_server ("init ", 0);
    send_to_server (current_parsed_root->directory, 0);
    send_to_server ("\012", 0);
}



#if defined AUTH_CLIENT_SUPPORT || defined HAVE_KERBEROS || defined HAVE_GSSAPI

struct hostent *
init_sockaddr (struct sockaddr_in *name, char *hostname, unsigned int port)
{
    struct hostent *hostinfo;
    unsigned short shortport = port;

    memset (name, 0, sizeof (*name));
    name->sin_family = AF_INET;
    name->sin_port = htons (shortport);
    hostinfo = gethostbyname (hostname);
    if (!hostinfo)
    {
	fprintf (stderr, "Unknown host %s.\n", hostname);
	exit (EXIT_FAILURE);
    }
    name->sin_addr = *(struct in_addr *) hostinfo->h_addr;
    return hostinfo;
}

#endif /* defined AUTH_CLIENT_SUPPORT || defined HAVE_KERBEROS
	* || defined HAVE_GSSAPI
	*/

#endif /* CLIENT_SUPPORT */
