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
 * specified in the README file that comes with the CVS kit.
 */

/*
 * basic information used in all source files
 *
 */


/* Some GNULIB headers require that we include system headers first.  */
#include "system.h"

/* begin GNULIB headers */
#include "dirname.h"
#include "getdate.h"
#include "minmax.h"
#include "regex.h"
#include "stat-macros.h"
#include "timespec.h"
#include "unlocked-io.h"
#include "xalloc.h"
#include "xgetcwd.h"
#include "xsize.h"
/* end GNULIB headers */

#if ! STDC_HEADERS
char *getenv();
#endif /* ! STDC_HEADERS */

/* Under OS/2, <stdio.h> doesn't define popen()/pclose(). */
#ifdef USE_OWN_POPEN
#include "popen.h"
#endif

#ifdef SERVER_SUPPORT
/* If the system doesn't provide strerror, it won't be declared in
   string.h.  */
char *strerror (int);
#endif

#include "hash.h"
#include "stack.h"

#include "root.h"

#if defined(SERVER_SUPPORT) || defined(CLIENT_SUPPORT)
# include "client.h"
#endif

#ifdef MY_NDBM
#include "myndbm.h"
#else
#include <ndbm.h>
#endif /* MY_NDBM */

#include "rcs.h"



/* Note that the _ONLY_ reason for PATH_MAX is if various system calls (getwd,
 * getcwd, readlink) require/want us to use it.  All other parts of CVS
 * allocate pathname buffers dynamically, and we want to keep it that way.
 */
#include "pathmax.h"



/* Definitions for the CVS Administrative directory and the files it contains.
   Here as #define's to make changing the names a simple task.  */

#ifdef USE_VMS_FILENAMES
#define CVSADM          "CVS"
#define CVSADM_ENT      "CVS/Entries."
#define CVSADM_ENTBAK   "CVS/Entries.Backup"
#define CVSADM_ENTLOG   "CVS/Entries.Log"
#define CVSADM_ENTSTAT  "CVS/Entries.Static"
#define CVSADM_REP      "CVS/Repository."
#define CVSADM_ROOT     "CVS/Root."
#define CVSADM_TAG      "CVS/Tag."
#define CVSADM_NOTIFY   "CVS/Notify."
#define CVSADM_NOTIFYTMP "CVS/Notify.tmp"
#define CVSADM_BASE      "CVS/Base"
#define CVSADM_BASEREV   "CVS/Baserev."
#define CVSADM_BASEREVTMP "CVS/Baserev.tmp"
#define CVSADM_TEMPLATE "CVS/Template."
#else /* !USE_VMS_FILENAMES */
#define	CVSADM		"CVS"
#define	CVSADM_ENT	"CVS/Entries"
#define	CVSADM_ENTBAK	"CVS/Entries.Backup"
#define CVSADM_ENTLOG	"CVS/Entries.Log"
#define	CVSADM_ENTSTAT	"CVS/Entries.Static"
#define	CVSADM_REP	"CVS/Repository"
#define	CVSADM_ROOT	"CVS/Root"
#define	CVSADM_TAG	"CVS/Tag"
#define CVSADM_NOTIFY	"CVS/Notify"
#define CVSADM_NOTIFYTMP "CVS/Notify.tmp"
/* A directory in which we store base versions of files we currently are
   editing with "cvs edit".  */
#define CVSADM_BASE     "CVS/Base"
#define CVSADM_BASEREV  "CVS/Baserev"
#define CVSADM_BASEREVTMP "CVS/Baserev.tmp"
/* File which contains the template for use in log messages.  */
#define CVSADM_TEMPLATE "CVS/Template"
#endif /* USE_VMS_FILENAMES */

/* This is the special directory which we use to store various extra
   per-directory information in the repository.  It must be the same as
   CVSADM to avoid creating a new reserved directory name which users cannot
   use, but is a separate #define because if anyone changes it (which I don't
   recommend), one needs to deal with old, unconverted, repositories.
   
   See fileattr.h for details about file attributes, the only thing stored
   in CVSREP currently.  */
#define CVSREP "CVS"

/*
 * Definitions for the CVSROOT Administrative directory and the files it
 * contains.  This directory is created as a sub-directory of the $CVSROOT
 * environment variable, and holds global administration information for the
 * entire source repository beginning at $CVSROOT.
 */
#define	CVSROOTADM		"CVSROOT"
#define	CVSROOTADM_CHECKOUTLIST "checkoutlist"
#define CVSROOTADM_COMMITINFO	"commitinfo"
#define CVSROOTADM_CONFIG	"config"
#define	CVSROOTADM_HISTORY	"history"
#define	CVSROOTADM_IGNORE	"cvsignore"
#define	CVSROOTADM_LOGINFO	"loginfo"
#define	CVSROOTADM_MODULES	"modules"
#define CVSROOTADM_NOTIFY	"notify"
#define CVSROOTADM_PASSWD	"passwd"
#define CVSROOTADM_POSTADMIN	"postadmin"
#define CVSROOTADM_POSTPROXY	"postproxy"
#define CVSROOTADM_POSTTAG	"posttag"
#define CVSROOTADM_POSTWATCH	"postwatch"
#define CVSROOTADM_PREPROXY	"preproxy"
#define	CVSROOTADM_RCSINFO	"rcsinfo"
#define CVSROOTADM_READERS	"readers"
#define CVSROOTADM_TAGINFO      "taginfo"
#define CVSROOTADM_USERS	"users"
#define CVSROOTADM_VALTAGS	"val-tags"
#define CVSROOTADM_VERIFYMSG    "verifymsg"
#define CVSROOTADM_WRAPPER	"cvswrappers"
#define CVSROOTADM_WRITERS	"writers"

#define CVSNULLREPOS		"Emptydir"	/* an empty directory */

/* Other CVS file names */

/* Files go in the attic if the head main branch revision is dead,
   otherwise they go in the regular repository directories.  The whole
   concept of having an attic is sort of a relic from before death
   support but on the other hand, it probably does help the speed of
   some operations (such as main branch checkouts and updates).  */
#define	CVSATTIC	"Attic"

#define	CVSLCK		"#cvs.lock"
#define	CVSHISTORYLCK	"#cvs.history.lock"
#define	CVSVALTAGSLCK	"#cvs.val-tags.lock"
#define	CVSRFL		"#cvs.rfl"
#define	CVSPFL		"#cvs.pfl"
#define	CVSWFL		"#cvs.wfl"
#define CVSPFLPAT	"#cvs.pfl.*"	/* wildcard expr to match plocks */
#define CVSRFLPAT	"#cvs.rfl.*"	/* wildcard expr to match read locks */
#define	CVSEXT_LOG	",t"
#define	CVSPREFIX	",,"
#define CVSDOTIGNORE	".cvsignore"
#define CVSDOTWRAPPER   ".cvswrappers"

/* Command attributes -- see function lookup_command_attribute(). */
#define CVS_CMD_IGNORE_ADMROOT        1

/* Set if CVS needs to create a CVS/Root file upon completion of this
   command.  The name may be slightly confusing, because the flag
   isn't really as general purpose as it seems (it is not set for cvs
   release).  */

#define CVS_CMD_USES_WORK_DIR         2

#define CVS_CMD_MODIFIES_REPOSITORY   4

/* miscellaneous CVS defines */

/* This is the string which is at the start of the non-log-message lines
   that we put up for the user when they edit the log message.  */
#define	CVSEDITPREFIX	"CVS: "
/* Number of characters in CVSEDITPREFIX to compare when deciding to strip
   off those lines.  We don't check for the space, to accomodate users who
   have editors which strip trailing spaces.  */
#define CVSEDITPREFIXLEN 4

#define	CVSLCKAGE	(60*60)		/* 1-hour old lock files cleaned up */
#define	CVSLCKSLEEP	30		/* wait 30 seconds before retrying */
#define	CVSBRANCH	"1.1.1"		/* RCS branch used for vendor srcs */

/*
 * Special tags. -rHEAD	refers to the head of an RCS file, regardless of any
 * sticky tags. -rBASE	refers to the current revision the user has checked
 * out This mimics the behaviour of RCS.
 */
#define	TAG_HEAD	"HEAD"
#define	TAG_BASE	"BASE"

/* Environment variable used by CVS */
#define	CVSREAD_ENV	"CVSREAD"	/* make files read-only */
#define	CVSREAD_DFLT	0		/* writable files by default */

					/* repository is read-only */
#define	CVSREADONLYFS_ENV "CVSREADONLYFS"

					/* verify checkouts */
#define	CVS_VERIFY_CHECKOUTS_ENV \
			"CVS_VERIFY_CHECKOUTS"
					/* verify template */
#define	CVS_VERIFY_TEMPLATE_ENV \
			"CVS_VERIFY_TEMPLATE"

					/* sign commits */
#define	CVS_SIGN_COMMITS_ENV \
			"CVS_SIGN_COMMITS"

#define	TMPDIR_ENV	"TMPDIR"	/* Temporary directory */
#define	CVS_PID_ENV	"CVS_PID"	/* pid of running cvs */

#define	EDITOR1_ENV	"CVSEDITOR"	/* which editor to use */
#define	EDITOR2_ENV	"VISUAL"	/* which editor to use */
#define	EDITOR3_ENV	"EDITOR"	/* which editor to use */

#define	CVSROOT_ENV	"CVSROOT"	/* source directory root */
/* Define CVSROOT_DFLT to a fallback value for CVSROOT.
 *
#undef	CVSROOT_DFL
 */

#define	IGNORE_ENV	"CVSIGNORE"	/* More files to ignore */
#define WRAPPER_ENV     "CVSWRAPPERS"   /* name of the wrapper file */

#define	CVSUMASK_ENV	"CVSUMASK"	/* Effective umask for repository */

#define	CVSNOBASES_ENV	"CVSNOBASES"	/* Suppress use of base files when
					 * set.
					 */

/*
 * If the beginning of the Repository matches the following string, strip it
 * so that the output to the logfile does not contain a full pathname.
 *
 * If the CVSROOT environment variable is set, it overrides this define.
 */
#define	REPOS_STRIP	"/master/"

/* Large enough to hold DATEFORM.  Not an arbitrary limit as long as
   it is used for that purpose, and not to hold a string from the
   command line, the client, etc.  */
#define MAXDATELEN	50

#include "entries.h"

/* The type of request that is being done in do_module() */
enum mtype
{
    CHECKOUT, TAG, PATCH, EXPORT, MISC
};



extern const char *program_name, *program_path, *cvs_cmd_name;
extern char *Editor;
extern int cvsadmin_root;
extern char *CurDir;
extern int really_quiet, quiet;
extern int use_editor;
extern int cvswrite;
extern mode_t cvsumask;

/* Temp dir abstraction.  */
/* From main.c.  */
const char *get_cvs_tmp_dir (void);
/* From filesubr.c.  */
const char *get_system_temp_dir (void);
void push_env_temp_dir (void);


/* This global variable holds the global -d option.  It is NULL if -d
   was not used, which means that we must get the CVSroot information
   from the CVSROOT environment variable or from a CVS/Root file.  */
extern char *CVSroot_cmdline;

/* This variable keeps track of all of the CVSROOT directories that
 * have been seen by the client.
 */
extern List *root_directories;

char *emptydir_name (void);
int safe_location (char *);

extern int noexec;		/* Don't modify disk anywhere */
extern int readonlyfs;		/* fail on all write locks; succeed all read locks */
extern int logoff;		/* Don't write history entry */
extern bool suppress_bases;


/* This structure holds the global configuration data.  */
extern struct config *config;

#ifdef CLIENT_SUPPORT
extern List *dirs_sent_to_server; /* used to decide which "Argument
				     xxx" commands to send to each
				     server in multiroot mode. */
#endif

extern char *hostname;

/* Externs that are included directly in the CVS sources */

/* Flags used by RCS_* functions.  See the description of the individual
   functions for which flags mean what for each function.  */
#define RCS_FLAGS_FORCE 1
#define RCS_FLAGS_DEAD 2
#define RCS_FLAGS_QUIET 4
#define RCS_FLAGS_MODTIME 8
#define RCS_FLAGS_KEEPFILE 16
#define RCS_FLAGS_USETIME 32

int RCS_exec_rcsdiff (RCSNode *rcsfile, int diff_argc,
                      char * const *diff_argv, const char *options,
                      const char *rev1, const char *rev1_cache,
                      const char *rev2,
                      const char *label1, const char *label2,
                      const char *workfile);
int diff_exec (const char *file1, const char *file2,
               const char *label1, const char *label2,
               int iargc, char * const *iargv, const char *out);


#include "error.h"

/* If non-zero, error will use the CVS protocol to report error
 * messages.  This will only be set in the CVS server parent process;
 * most other code is run via do_cvs_command, which forks off a child
 * process and packages up its stderr in the protocol.
 *
 * This needs to be here rather than in error.h in order to use an unforked
 * error.h from GNULIB.
 */
extern int error_use_protocol;


DBM *open_module (void);
void Subdirs_Known (List *entries);
void Subdir_Register (List *, const char *, const char *);
void Subdir_Deregister (List *, const char *, const char *);

void parse_tagdate (char **tag, char **date, const char *input);
char *Make_Date (const char *rawdate);
char *date_from_time_t (time_t);
void date_to_internet (char *, const char *);
void date_to_tm (struct tm *, const char *);
void tm_to_internet (char *, const struct tm *);
char *gmformat_time_t (time_t unixtime);
char *format_date_alloc (char *text);

char *entries_time (time_t unixtime);
time_t unix_time_stamp (const char *file);

typedef	RETSIGTYPE (*SIGCLEANUPPROC)	(int);
int SIG_register (int sig, SIGCLEANUPPROC sigcleanup);

#include "filesubr.h"
#include "subr.h"

int ls (int argc, char *argv[]);

int update (int argc, char *argv[]);
/* The only place this is currently used outside of update.c is add.c.
 * Restricting its use to update.c seems to be in the best interest of
 * modularity, but I can't think of a good way to get an update of a
 * resurrected file done and print the fact otherwise.
 */
void write_letter (struct file_info *finfo, int letter);
int xcmp (const char *file1, const char *file2);

int Create_Admin (const char *dir, const char *update_dir,
                  const char *repository, const char *tag, const char *date,
                  int nonbranch, int warn, int dotemplate);
int expand_at_signs (const char *, size_t, FILE *);

void Scratch_Entry (List * list, const char *fname);
void ParseTag (char **tagp, char **datep, int *nonbranchp);
void WriteTag (const char *dir, const char *tag, const char *date,
               int nonbranch, const char *update_dir, const char *repository);
void WriteTemplate (const char *update_dir, int dotemplate,
                    const char *repository);
void cat_module (int status);
void check_entries (char *dir);
void close_module (DBM * db);
void fperrmsg (FILE * fp, int status, int errnum, char *message,...);

#include "update.h"

void make_directories (const char *name);
void make_directory (const char *name);
int mkdir_if_needed (const char *name);
void rename_file (const char *from, const char *to);
/* Expand wildcards in each element of (ARGC,ARGV).  This is according to the
   files which exist in the current directory, and accordingly to OS-specific
   conventions regarding wildcard syntax.  It might be desirable to change the
   former in the future (e.g. "cvs status *.h" including files which don't exist
   in the working directory).  The result is placed in *PARGC and *PARGV;
   the *PARGV array itself and all the strings it contains are newly
   malloc'd.  It is OK to call it with PARGC == &ARGC or PARGV == &ARGV.  */
void expand_wild (int argc, char **argv, 
                  int *pargc, char ***pargv);

/* exithandle.c */
void signals_register (RETSIGTYPE (*handler)(int));
void cleanup_register (void (*handler) (void));

void update_delproc (Node * p);
void usage (const char *const *cpp);
void Update_Logfile (const char *repository, const char *xmessage,
                     FILE *xlogfp, List *xchanges);
void do_editor (const char *dir, char **messagep,
                const char *repository, List *changes);

typedef	int (*CALLBACKPROC)	(int argc, char *argv[], char *where,
	char *mwhere, char *mfile, int shorten, int local_specified,
	char *omodule, char *msg);

int mkmodules (char *dir);
int init (int argc, char **argv);

int do_module (DBM * db, char *mname, enum mtype m_type, char *msg,
		CALLBACKPROC callback_proc, char *where, int shorten,
		int local_specified, int run_module_prog, int build_dirs,
		char *extra_arg);
void history_write (int type, const char *update_dir, const char *revs,
                    const char *name, const char *repository);
void SIG_beginCrSect (void);
void SIG_endCrSect (void);
int SIG_inCrSect (void);
void read_cvsrc (int *argc, char ***argv, const char *cmdname);

#include "run.h"

pid_t waitpid (pid_t, int *, int);

#include "vers_ts.h"

/* Miscellaneous CVS infrastructure which layers on top of the recursion
   processor (for example, needs struct file_info).  */

int Checkin (int type, struct file_info *finfo, char *rev,
	     char *tag, char *options, char *message);
/* TODO: can the finfo argument to special_file_mismatch be changed? -twp */
int special_file_mismatch (struct file_info *finfo,
				  char *rev1, char *rev2);

/* Pathname expansion */
char *expand_path (const char *name, const char *cvsroot, bool formatsafe,
		   const char *file, int line);

/* User variables.  */
extern List *variable_list;

void variable_set (char *nameval);

int watch (int argc, char **argv);
int edit (int argc, char **argv);
int unedit (int argc, char **argv);
int editors (int argc, char **argv);
int watchers (int argc, char **argv);
int annotate (int argc, char **argv);
int add (int argc, char **argv);
int admin (int argc, char **argv);
int checkout (int argc, char **argv);
int commit (int argc, char **argv);
int diff (int argc, char **argv);
int history (int argc, char **argv);
int import (int argc, char **argv);
int cvslog (int argc, char **argv);
#ifdef AUTH_CLIENT_SUPPORT
/* Some systems (namely Mac OS X) have conflicting definitions for these
 * functions.  Avoid them.
 */
#ifdef HAVE_LOGIN
# define login		cvs_login
#endif /* HAVE_LOGIN */
#ifdef HAVE_LOGOUT
# define logout		cvs_logout
#endif /* HAVE_LOGOUT */
int login (int argc, char **argv);
int logout (int argc, char **argv);
#endif /* AUTH_CLIENT_SUPPORT */
int patch (int argc, char **argv);
int release (int argc, char **argv);
int cvsremove (int argc, char **argv);
int rtag (int argc, char **argv);
int cvsstatus (int argc, char **argv);
int cvstag (int argc, char **argv);
int version (int argc, char **argv);

unsigned long int lookup_command_attribute (const char *);

#if defined(AUTH_CLIENT_SUPPORT) || defined(AUTH_SERVER_SUPPORT)
char *scramble (char *str);
char *descramble (char *str);
#endif /* AUTH_CLIENT_SUPPORT || AUTH_SERVER_SUPPORT */

#ifdef AUTH_CLIENT_SUPPORT
char *get_cvs_password (void);
void free_cvs_password (char *str);
/* get_cvs_port_number() is not pure since the /etc/services file could change
 * between calls.  */
int get_cvs_port_number (const cvsroot_t *root);
/* normalize_cvsroot() is not pure since it calls get_cvs_port_number.  */
char *normalize_cvsroot (const cvsroot_t *root)
	__attribute__ ((__malloc__));
#endif /* AUTH_CLIENT_SUPPORT */

void tag_check_valid (const char *, int, char **, int, int, char *, bool);

#include "server.h"

extern const char *global_session_id;

/* From find_names.c.  */
List *find_files (const char *dir, const char *pat);

typedef union {
    uint32_t int_checksum[4];
    unsigned char char_checksum[16];
} checksum_t;
