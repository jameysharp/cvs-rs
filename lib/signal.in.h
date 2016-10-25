/* A GNU-like <signal.h>.

   Copyright (C) 2006-2008 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

@PRAGMA_SYSTEM_HEADER@

#if defined __need_sig_atomic_t || defined __need_sigset_t
/* Special invocation convention inside glibc header files.  */

# @INCLUDE_NEXT@ @NEXT_SIGNAL_H@

#else
/* Normal invocation convention.  */

#ifndef _GL_SIGNAL_H

/* The include_next requires a split double-inclusion guard.  */
#@INCLUDE_NEXT@ @NEXT_SIGNAL_H@

#ifndef _GL_SIGNAL_H
#define _GL_SIGNAL_H

/* The definition of GL_LINK_WARNING is copied here.  */

/* Define pid_t, uid_t.
   Also, mingw defines sigset_t not in <signal.h>, but in <sys/types.h>.  */
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif


#if !@HAVE_POSIX_SIGNALBLOCKING@

/* Maximum signal number + 1.  */
# ifndef NSIG
#  define NSIG 32
# endif

/* This code supports only 32 signals.  */
typedef int verify_NSIG_constraint[2 * (NSIG <= 32) - 1];

/* A set or mask of signals.  */
# if !@HAVE_SIGSET_T@
typedef unsigned int sigset_t;
# endif

/* Test whether a given signal is contained in a signal set.  */
extern int sigismember (const sigset_t *set, int sig);

/* Initialize a signal set to the empty set.  */
extern int sigemptyset (sigset_t *set);

/* Add a signal to a signal set.  */
extern int sigaddset (sigset_t *set, int sig);

/* Remove a signal from a signal set.  */
extern int sigdelset (sigset_t *set, int sig);

/* Fill a signal set with all possible signals.  */
extern int sigfillset (sigset_t *set);

/* Return the set of those blocked signals that are pending.  */
extern int sigpending (sigset_t *set);

/* If OLD_SET is not NULL, put the current set of blocked signals in *OLD_SET.
   Then, if SET is not NULL, affect the current set of blocked signals by
   combining it with *SET as indicated in OPERATION.
   In this implementation, you are not allowed to change a signal handler
   while the signal is blocked.  */
# define SIG_BLOCK   0  /* blocked_set = blocked_set | *set; */
# define SIG_SETMASK 1  /* blocked_set = *set; */
# define SIG_UNBLOCK 2  /* blocked_set = blocked_set & ~*set; */
extern int sigprocmask (int operation, const sigset_t *set, sigset_t *old_set);

# define signal rpl_signal
/* Install the handler FUNC for signal SIG, and return the previous
   handler.  */
extern void (*signal (int sig, void (*func) (int))) (int);

#endif /* !@HAVE_POSIX_SIGNALBLOCKING@ */

#if !@HAVE_SIGACTION@

# if !@HAVE_SIGINFO_T@
/* Present to allow compilation, but unsupported by gnulib.  */
union sigval
{
  int sival_int;
  void *sival_ptr;
};

/* Present to allow compilation, but unsupported by gnulib.  */
struct siginfo_t
{
  int si_signo;
  int si_code;
  int si_errno;
  pid_t si_pid;
  uid_t si_uid;
  void *si_addr;
  int si_status;
  long si_band;
  union sigval si_value;
};
typedef struct siginfo_t siginfo_t;
# endif /* !@HAVE_SIGINFO_T@ */

/* We assume that platforms which lack the sigaction() function also lack
   the 'struct sigaction' type, and vice versa.  */

struct sigaction
{
  union
  {
    void (*_sa_handler) (int);
    /* Present to allow compilation, but unsupported by gnulib.  POSIX
       says that implementations may, but not must, make sa_sigaction
       overlap with sa_handler, but we know of no implementation where
       they do not overlap.  */
    void (*_sa_sigaction) (int, siginfo_t *, void *);
  } _sa_func;
  sigset_t sa_mask;
  /* Not all POSIX flags are supported.  */
  int sa_flags;
};
# define sa_handler _sa_func._sa_handler
# define sa_sigaction _sa_func._sa_sigaction
/* Unsupported flags are not present.  */
# define SA_RESETHAND 1
# define SA_NODEFER 2
# define SA_RESTART 4

extern int sigaction (int, const struct sigaction *restrict,
                      struct sigaction *restrict);

#elif !@HAVE_STRUCT_SIGACTION_SA_SIGACTION@

# define sa_sigaction sa_handler

#endif /* !@HAVE_SIGACTION@, !@HAVE_STRUCT_SIGACTION_SA_SIGACTION@ */


#ifdef __cplusplus
}
#endif

#endif /* _GL_SIGNAL_H */
#endif /* _GL_SIGNAL_H */
#endif
