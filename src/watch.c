/*
 * Copyright (C) 2007 The Free Software Foundation, Inc.
 *
 * Implementation for "cvs watch add", "cvs watchers", and related commands
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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

/* Verify interface.  */
#include "watch.h"

/* CVS headers.  */
#include "edit.h"
#include "fileattr.h"
#include "ignore.h"
#include "lock.h"
#include "recurse.h"

#include "cvs.h"



const char *const watch_usage[] =
{
    "Usage: %s %s {on|off|add|remove} [-lR] [-a action]... [-u user]\n",
    "                 [<path>]...\n",
    "on/off\t\tTurn on/off read-only checkouts of files.\n",
    "add/remove\tAdd or remove notification on actions.\n",
    "\t-l\tLocal directory only, not recursive (on/off/add/remove).\n",
    "\t-R\tProcess directories recursively (default, on/off/add/remove).\n",
    "\t-a action\n",
    "\t\tSpecify what actions, one of: `edit', `unedit', `commit',\n",
    "\t\t`all', or `none' (defaults to `all', add/remove).\n",
    "\t-e\tWith remove, remove temporary watches (aquired via `cvs edit')\n",
    "\t-u user\tApply selection to user's watches (defaults to current\n",
    "\t\tuser, only cvs administrators may affect other users, add/remove)\n",
    "(Specify the --help global option for a list of other help options.)\n",
    NULL
};

static struct addremove_args the_args;

void
watch_modify_watchers (const char *file, struct addremove_args *what)
{
    char *curattr = fileattr_get0 (file, "_watchers");
    char *p;
    char *pend;
    char *nextp;
    const char *who;
    int who_len;
    char *mycurattr;
    char *mynewattr;
    size_t mynewattr_size;

    int add_edit_pending;
    int add_unedit_pending;
    int add_commit_pending;
    int remove_edit_pending;
    int remove_unedit_pending;
    int remove_commit_pending;
    int add_tedit_pending;
    int add_tunedit_pending;
    int add_tcommit_pending;

    TRACE (TRACE_FUNCTION, "watch_modify_watchers (%s)", file);

    if (the_args.user && !STREQ (getcaller (), the_args.user) && !is_admin ())
    {
	error (1, 0,
	       "Editing other user's watches is restricted to the %s group.",
	       CVS_ADMIN_GROUP);
    }

    who = the_args.user ? the_args.user : getcaller ();
    who_len = strlen (who);

    /* Look for current watcher types for this user.  */
    mycurattr = NULL;
    if (curattr != NULL)
    {
	p = curattr;
	while (1) {
	    if (STRNEQ (who, p, who_len) && p[who_len] == '>')
	    {
		/* Found this user.  */
		mycurattr = p + who_len + 1;
	    }
	    p = strchr (p, ',');
	    if (p == NULL)
		break;
	    ++p;
	}
    }
    if (mycurattr != NULL)
    {
	mycurattr = xstrdup (mycurattr);
	p = strchr (mycurattr, ',');
	if (p != NULL)
	    *p = '\0';
    }

    /* Now copy mycurattr to mynewattr, making the requisite modifications.
       Note that we add a dummy '+' to the start of mynewattr, to reduce
       special cases (but then we strip it off when we are done).  */

    mynewattr_size = sizeof "+edit+unedit+commit+tedit+tunedit+tcommit";
    if (mycurattr != NULL)
	mynewattr_size += strlen (mycurattr);
    mynewattr = xmalloc (mynewattr_size);
    mynewattr[0] = '\0';

    add_edit_pending = what->adding && what->edit;
    add_unedit_pending = what->adding && what->unedit;
    add_commit_pending = what->adding && what->commit;
    remove_edit_pending = !what->adding && what->edit;
    remove_unedit_pending = !what->adding && what->unedit;
    remove_commit_pending = !what->adding && what->commit;
    add_tedit_pending = what->add_tedit;
    add_tunedit_pending = what->add_tunedit;
    add_tcommit_pending = what->add_tcommit;

    /* Copy over existing watch types, except those to be removed.  */
    p = mycurattr;
    while (p != NULL)
    {
	pend = strchr (p, '+');
	if (pend == NULL)
	{
	    pend = p + strlen (p);
	    nextp = NULL;
	}
	else
	    nextp = pend + 1;

	/* Process this item.  */
	if (pend - p == 4 && STRNEQ ("edit", p, 4))
	{
	    if (!remove_edit_pending)
		strcat (mynewattr, "+edit");
	    add_edit_pending = 0;
	}
	else if (pend - p == 6 && STRNEQ ("unedit", p, 6))
	{
	    if (!remove_unedit_pending)
		strcat (mynewattr, "+unedit");
	    add_unedit_pending = 0;
	}
	else if (pend - p == 6 && STRNEQ ("commit", p, 6))
	{
	    if (!remove_commit_pending)
		strcat (mynewattr, "+commit");
	    add_commit_pending = 0;
	}
	else if (pend - p == 5 && STRNEQ ("tedit", p, 5))
	{
	    if (!what->remove_temp)
		strcat (mynewattr, "+tedit");
	    add_tedit_pending = 0;
	}
	else if (pend - p == 7 && STRNEQ ("tunedit", p, 7))
	{
	    if (!what->remove_temp)
		strcat (mynewattr, "+tunedit");
	    add_tunedit_pending = 0;
	}
	else if (pend - p == 7 && STRNEQ ("tcommit", p, 7))
	{
	    if (!what->remove_temp)
		strcat (mynewattr, "+tcommit");
	    add_tcommit_pending = 0;
	}
	else
	{
	    char *mp;

	    /* Copy over any unrecognized watch types, for future
	       expansion.  */
	    mp = mynewattr + strlen (mynewattr);
	    *mp++ = '+';
	    strncpy (mp, p, pend - p);
	    *(mp + (pend - p)) = '\0';
	}

	/* Set up for next item.  */
	p = nextp;
    }

    /* Add in new watch types.  */
    if (add_edit_pending)
	strcat (mynewattr, "+edit");
    if (add_unedit_pending)
	strcat (mynewattr, "+unedit");
    if (add_commit_pending)
	strcat (mynewattr, "+commit");
    if (add_tedit_pending)
	strcat (mynewattr, "+tedit");
    if (add_tunedit_pending)
	strcat (mynewattr, "+tunedit");
    if (add_tcommit_pending)
	strcat (mynewattr, "+tcommit");

    {
	char *curattr_new;

	curattr_new =
	  fileattr_modify (curattr,
			   who,
			   mynewattr[0] == '\0' ? NULL : mynewattr + 1,
			   '>',
			   ',');
	/* If the attribute is unchanged, don't rewrite the attribute file.  */
	if (!((curattr_new == NULL && curattr == NULL)
	      || (curattr_new != NULL
		  && curattr != NULL
		  && STREQ (curattr_new, curattr))))
	    fileattr_set (file,
			  "_watchers",
			  curattr_new);
	if (curattr_new != NULL)
	    free (curattr_new);
    }

    if (curattr != NULL)
	free (curattr);
    if (mycurattr != NULL)
	free (mycurattr);
    if (mynewattr != NULL)
	free (mynewattr);
}



static int
addremove_fileproc (void *callerdat, struct file_info *finfo)
{
    TRACE (TRACE_FUNCTION, "addremove_fileproc (%s)", finfo->fullname);
    watch_modify_watchers (finfo->file, &the_args);
    return 0;
}



static int
addremove_filesdoneproc (void *callerdat, int err, const char *repository,
			 const char *update_dir, List *entries)
{
    int set_default = the_args.setting_default;
    int dir_check = 0;

    TRACE (TRACE_FLOW, "addremove_filesdoneproc (%d, %s, %s)",
	   err, repository, update_dir);

    while (!set_default && dir_check < the_args.num_dirs)
    {
	/* If we are recursing, then just see if the first part of update_dir 
	 * matches any of the specified directories.  Otherwise, it must be an
	 * exact match.
	 */
	if (the_args.local)
	    set_default = STREQ (NULL2DOT (update_dir),
				 the_args.dirs[dir_check]);
	else 
	    set_default = STRNEQ (NULL2DOT (update_dir),
				  the_args.dirs[dir_check],
		    		  strlen (the_args.dirs[dir_check]));
	dir_check++;
    }

    if (set_default)
	watch_modify_watchers (NULL, &the_args);
    return err;
}



static int
watch_addremove (int argc, char **argv)
{
    int c;
    int err;
    int a_omitted;
    int arg_index;
    int max_dirs;

    TRACE (TRACE_FUNCTION, "watch_addremove (%d)", argc);

    a_omitted = 1;
    the_args.commit = 0;
    the_args.edit = 0;
    the_args.unedit = 0;
    the_args.num_dirs = 0;
    the_args.dirs = NULL;
    the_args.local = 0;
    the_args.user = NULL;

    optind = 0;
    while ((c = getopt (argc, argv, "+lRa:eu:")) != -1)
    {
	switch (c)
	{
	    case 'l':
		the_args.local = 1;
		break;
	    case 'R':
		the_args.local = 0;
		break;
	    case 'a':
		a_omitted = 0;
		if (STREQ (optarg, "edit"))
		    the_args.edit = 1;
		else if (STREQ (optarg, "unedit"))
		    the_args.unedit = 1;
		else if (STREQ (optarg, "commit"))
		    the_args.commit = 1;
		else if (STREQ (optarg, "all"))
		{
		    the_args.edit = 1;
		    the_args.unedit = 1;
		    the_args.commit = 1;
		}
		else if (STREQ (optarg, "none"))
		{
		    the_args.edit = 0;
		    the_args.unedit = 0;
		    the_args.commit = 0;
		}
		else
		    usage (watch_usage);
		break;
	    case 'e':
		if (the_args.adding)
		{
		    error (0, 0, "-e is only valid with remove.");
		    usage (watch_usage);
		}
		the_args.remove_temp = 1;
		break;
	    case 'u':
		if (the_args.user) free ((char *)the_args.user);
		the_args.user = xstrdup (optarg);
		break;
	    case '?':
	    default:
		usage (watch_usage);
		break;
	}
    }
    argc -= optind;
    argv += optind;

    the_args.num_dirs = 0;
    max_dirs = 4; /* Arbitrary choice. */
    the_args.dirs = xmalloc (sizeof (const char *) * max_dirs);

    TRACE (TRACE_FUNCTION, "watch_addremove (%d)", argc);
    for (arg_index=0; arg_index<argc; ++arg_index)
    {
	TRACE (TRACE_FUNCTION, "\t%s", argv[arg_index]);
	if (isdir (argv[arg_index]))
	{
	    if (the_args.num_dirs >= max_dirs)
	    {
		max_dirs *= 2;
		the_args.dirs = xrealloc ((void *)the_args.dirs, max_dirs);
	    }
	    the_args.dirs[the_args.num_dirs++] = argv[arg_index];
	}
    }

    if (a_omitted)
    {
	the_args.edit = 1;
	the_args.unedit = 1;
	the_args.commit = 1;
    }

#ifdef CLIENT_SUPPORT
    if (current_parsed_root->isremote)
    {
	start_server();
	ign_setup();

	if (the_args.local)
	    send_arg ("-l");
	/* FIXME: copes poorly with "all" if server is extended to have
	   new watch types and client is still running an old version.  */
	if (the_args.edit)
	    option_with_arg ("-a", "edit");
	if (the_args.unedit)
	    option_with_arg ("-a", "unedit");
	if (the_args.commit)
	    option_with_arg ("-a", "commit");
	if (!the_args.edit && !the_args.unedit && !the_args.commit)
	    option_with_arg ("-a", "none");
	if (the_args.remove_temp)
	    send_arg ("-e");
	if (the_args.user)
	    option_with_arg ("-u", the_args.user);
	send_arg ("--");
	send_files (argc, argv, the_args.local, 0, SEND_NO_CONTENTS);
	send_file_names (argc, argv, SEND_EXPAND_WILD);
	send_to_server (the_args.adding ?
                        "watch-add\012" : "watch-remove\012",
                        0);
	return get_responses_and_close ();
    }
#endif /* CLIENT_SUPPORT */

    the_args.setting_default = (argc <= 0);

    lock_tree_promotably (argc, argv, the_args.local, W_LOCAL, 0);

    err = start_recursion
	(addremove_fileproc, addremove_filesdoneproc, NULL, NULL, NULL,
	 argc, argv, the_args.local, W_LOCAL, 0, CVS_LOCK_WRITE,
	 NULL, 1, NULL);

    Lock_Cleanup();
    free ((void *)the_args.dirs);
    the_args.dirs = NULL;

    return err;
}



int
watch_add (int argc, char **argv)
{
    the_args.adding = 1;
    return watch_addremove (argc, argv);
}

int
watch_remove (int argc, char **argv)
{
    the_args.adding = 0;
    return watch_addremove (argc, argv);
}

int
watch (int argc, char **argv)
{
    if (argc <= 1)
	usage (watch_usage);
    if (STREQ (argv[1], "on"))
    {
	--argc;
	++argv;
	return watch_on (argc, argv);
    }
    else if (STREQ (argv[1], "off"))
    {
	--argc;
	++argv;
	return watch_off (argc, argv);
    }
    else if (STREQ (argv[1], "add"))
    {
	--argc;
	++argv;
	return watch_add (argc, argv);
    }
    else if (STREQ (argv[1], "remove"))
    {
	--argc;
	++argv;
	return watch_remove (argc, argv);
    }
    else
	usage (watch_usage);
    return 0;
}

static const char *const watchers_usage[] =
{
    "Usage: %s %s [-lR] [<file>]...\n",
    "-l\tProcess this directory only (not recursive).\n",
    "-R\tProcess directories recursively (default).\n",
    "(Specify the --help global option for a list of other help options.)\n",
    NULL
};

static int watchers_fileproc (void *callerdat,
				     struct file_info *finfo);

static int
watchers_fileproc (void *callerdat, struct file_info *finfo)
{
    char *them;
    char *p;

    them = fileattr_get0 (finfo->file, "_watchers");
    if (them == NULL)
	return 0;

    cvs_output (finfo->fullname, 0);

    p = them;
    while (1)
    {
	cvs_output ("\t", 1);
	while (*p != '>' && *p != '\0')
	    cvs_output (p++, 1);
	if (*p == '\0')
	{
	    /* Only happens if attribute is misformed.  */
	    cvs_output ("\n", 1);
	    break;
	}
	++p;
	cvs_output ("\t", 1);
	while (1)
	{
	    while (*p != '+' && *p != ',' && *p != '\0')
		cvs_output (p++, 1);
	    if (*p == '\0')
	    {
		cvs_output ("\n", 1);
		goto out;
	    }
	    if (*p == ',')
	    {
		++p;
		break;
	    }
	    ++p;
	    cvs_output ("\t", 1);
	}
	cvs_output ("\n", 1);
    }
  out:;
    free (them);
    return 0;
}

int
watchers (int argc, char **argv)
{
    int local = 0;
    int c;

    if (argc == -1)
	usage (watchers_usage);

    optind = 0;
    while ((c = getopt (argc, argv, "+lR")) != -1)
    {
	switch (c)
	{
	    case 'l':
		local = 1;
		break;
	    case 'R':
		local = 0;
		break;
	    case '?':
	    default:
		usage (watchers_usage);
		break;
	}
    }
    argc -= optind;
    argv += optind;

#ifdef CLIENT_SUPPORT
    if (current_parsed_root->isremote)
    {
	start_server ();
	ign_setup ();

	if (local)
	    send_arg ("-l");
	send_arg ("--");
	send_files (argc, argv, local, 0, SEND_NO_CONTENTS);
	send_file_names (argc, argv, SEND_EXPAND_WILD);
	send_to_server ("watchers\012", 0);
	return get_responses_and_close ();
    }
#endif /* CLIENT_SUPPORT */

    return start_recursion (watchers_fileproc, NULL, NULL,
			    NULL, NULL, argc, argv, local, W_LOCAL, 0,
			    CVS_LOCK_READ, NULL, 1, NULL);
}
