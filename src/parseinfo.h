/*
 *  Copyright (C) 2004, 2006 The Free Software Foundation, Inc.
 *  Copyright (C) 2004  Derek Price, Ximbiot <http://ximbiot.com>
 *
 *  You may distribute under the terms of the GNU General Public License
 *  as specified in the README file that comes with the CVS source
 *  distribution.
 *
 * This is the header file for definitions and functions shared by parseinfo.c
 * with other portions of CVS.
 */
#ifndef PARSEINFO_H
# define PARSEINFO_H

/* ANSI C headers.  */
#include <stdbool.h>
#include <stddef.h>	/* Get size_t.  */

/* CVS headers.  */
#include "root.h"
#include "verify.h"



struct config
{
    void *keywords;
    bool top_level_admin;
    char *lock_dir;

    char *logHistory;
    bool usingDefaultLogHistory;

    char *HistoryLogPath;
    char *HistorySearchPath;
    char *TmpDir;

    /* Should the logmsg be re-read during the do_verify phase?
     * RereadLogAfterVerify=no|stat|yes
     * LOGMSG_REREAD_NEVER  - never re-read the logmsg
     * LOGMSG_REREAD_STAT   - re-read the logmsg only if it has changed
     * LOGMSG_REREAD_ALWAYS - always re-read the logmsg
     */
    int RereadLogAfterVerify;

    /* Should the verifymsg failures cause CVS to exit instantly or should
     * the script be run in each directory before the application quits a la
     * commitinfo.
     */
    bool FirstVerifyLogErrorFatal;

    char *UserAdminOptions;

    /* Control default behavior of 'cvs import' (-X option on or off) in
     * CVSROOT/config.  Defaults to off, for backward compatibility.
     */
    bool ImportNewFilesToVendorBranchOnly;

    size_t MaxCommentLeaderLength;
    bool UseArchiveCommentLeader;

#ifdef AUTH_SERVER_SUPPORT
    /* Should we check for system usernames/passwords?  */
    bool system_auth;
#endif /* AUTH_SERVER_SUPPORT */

#ifdef SUPPORT_OLD_INFO_FMT_STRINGS
    bool UseNewInfoFmtStrings;
#endif /* SUPPORT_OLD_INFO_FMT_STRINGS */
    cvsroot_t *PrimaryServer;
#ifdef PROXY_SUPPORT
    size_t MaxProxyBufferSize;
#endif /* PROXY_SUPPORT */
#ifdef SERVER_SUPPORT
    size_t MinCompressionLevel;
    size_t MaxCompressionLevel;
#endif /* SERVER_SUPPORT */

    verify_state VerifyCommits;
    char *VerifyTemplate;
    char *OpenPGPTextmode;
    List *VerifyArgs;

#ifdef PRESERVE_PERMISSIONS_SUPPORT
    bool preserve_perms;
#endif /* PRESERVE_PERMISSIONS_SUPPORT */
};



/***
 ***
 ***   CVSROOT/config options
 ***
 ***/
extern struct config *config;



/* Option flags for Parse_Info().  */
#define PIOPT_ALL 1	/* accept "all" keyword */



/* Callback proc for Parse_Info().  */
typedef	int (*CALLPROC)	(const char *repository, const char *value,
                         const char *filename, int linenum, void *closure);



/* Function prototypes.  */
int Parse_Info (const char *infofile, const char *repository,
                CALLPROC callproc, int opt, void *closure);
bool parse_error (const char *, unsigned int);
struct config *parse_config (const char *, const char *);
void free_config (struct config *data);
#endif /* !PARSEINFO_H */
