# DO NOT EDIT! GENERATED AUTOMATICALLY!
# Copyright (C) 2002-2008 Free Software Foundation, Inc.
#
# This file is free software, distributed under the terms of the GNU
# General Public License.  As a special exception to the GNU General
# Public License, this file may be distributed as part of a program
# that contains a configuration script generated by Autoconf, under
# the same distribution terms as the rest of that program.
#
# Generated by gnulib-tool.
#
# This file represents the compiled summary of the specification in
# gnulib-cache.m4. It lists the computed macro invocations that need
# to be invoked from configure.ac.
# In projects using CVS, this file can be treated like other built files.


# This macro should be invoked from ./configure.in, in the section
# "Checks for programs", right after AC_PROG_CC, and certainly before
# any checks for libraries, header files, types and library functions.
AC_DEFUN([gl_EARLY],
[
  m4_pattern_forbid([^gl_[A-Z]])dnl the gnulib macro namespace
  m4_pattern_allow([^gl_ES$])dnl a valid locale name
  m4_pattern_allow([^gl_LIBOBJS$])dnl a variable
  m4_pattern_allow([^gl_LTLIBOBJS$])dnl a variable
  AC_REQUIRE([AC_PROG_RANLIB])
  AC_REQUIRE([AM_PROG_CC_C_O])
  AC_REQUIRE([AC_GNU_SOURCE])
  AC_REQUIRE([gl_USE_SYSTEM_EXTENSIONS])
  AC_REQUIRE([AC_FUNC_FSEEKO])
  AC_REQUIRE([AC_FUNC_FSEEKO])
])

# This macro should be invoked from ./configure.in, in the section
# "Check for header files, types and library functions".
AC_DEFUN([gl_INIT],
[
  AM_CONDITIONAL([GL_COND_LIBTOOL], [false])
  gl_cond_libtool=false
  gl_libdeps=
  gl_ltlibdeps=
  m4_pushdef([AC_LIBOBJ], m4_defn([gl_LIBOBJ]))
  m4_pushdef([AC_REPLACE_FUNCS], m4_defn([gl_REPLACE_FUNCS]))
  m4_pushdef([AC_LIBSOURCES], m4_defn([gl_LIBSOURCES]))
  m4_pushdef([gl_LIBSOURCES_LIST], [])
  m4_pushdef([gl_LIBSOURCES_DIR], [])
  gl_COMMON
  gl_source_base='lib'
  gl_FUNC_ALLOCA
  gl_HEADER_ARPA_INET
  AC_PROG_MKDIR_P
  gl_FUNC_ATEXIT
  gl_FUNC_BASE64
  gl_CANON_HOST
  AC_FUNC_CANONICALIZE_FILE_NAME
  gl_MODULE_INDICATOR([canonicalize])
  gl_CANONICALIZE_LGPL
  gl_MODULE_INDICATOR([canonicalize-lgpl])
  gl_FUNC_CHDIR_LONG
  gl_FUNC_CHOWN
  gl_UNISTD_MODULE_INDICATOR([chown])
  gl_CLOCK_TIME
  gl_CLOSE_STREAM
  gl_MODULE_INDICATOR([close-stream])
  gl_CLOSEOUT
  gl_MD5
  gl_CHECK_TYPE_STRUCT_DIRENT_D_INO
  gl_CHECK_TYPE_STRUCT_DIRENT_D_TYPE
  gl_FUNC_DIRFD
  gl_DIRNAME
  gl_DOUBLE_SLASH_ROOT
  gl_FUNC_DUP2
  gl_UNISTD_MODULE_INDICATOR([dup2])
  gl_ENVIRON
  gl_UNISTD_MODULE_INDICATOR([environ])
  gl_HEADER_ERRNO_H
  gl_ERROR
  m4_ifdef([AM_XGETTEXT_OPTION],
    [AM_XGETTEXT_OPTION([--flag=error:3:c-format])
     AM_XGETTEXT_OPTION([--flag=error_at_line:5:c-format])])
  gl_EXITFAIL
  gl_FUNC_FCHDIR
  gl_UNISTD_MODULE_INDICATOR([fchdir])
  gl_FCNTL_H
  gl_FCNTL_SAFER
  gl_MODULE_INDICATOR([fcntl-safer])
  gl_FILE_NAME_CONCAT
  gl_FLOAT_H
  # No macro. You should also use one of fnmatch-posix or fnmatch-gnu.
  gl_FUNC_FNMATCH_POSIX
  gl_FUNC_FPENDING
  AC_REQUIRE([AC_C_INLINE])
  gl_FUNC_FSEEKO
  gl_STDIO_MODULE_INDICATOR([fseeko])
  gl_FUNC_FTELLO
  gl_STDIO_MODULE_INDICATOR([ftello])
  gl_FUNC_FTRUNCATE
  gl_UNISTD_MODULE_INDICATOR([ftruncate])
  gl_GETADDRINFO
  gl_FUNC_GETCWD
  gl_UNISTD_MODULE_INDICATOR([getcwd])
  gl_GETDATE
  gl_FUNC_GETDELIM
  gl_STDIO_MODULE_INDICATOR([getdelim])
  gl_FUNC_GETHOSTNAME
  gl_FUNC_GETLINE
  gl_STDIO_MODULE_INDICATOR([getline])
  gl_GETLOGIN_R
  gl_UNISTD_MODULE_INDICATOR([getlogin_r])
  gl_GETNDELIM2
  gl_GETNLINE
  gl_GETOPT
  gl_FUNC_GETPAGESIZE
  gl_UNISTD_MODULE_INDICATOR([getpagesize])
  gl_FUNC_GETPASS_GNU
  dnl you must add AM_GNU_GETTEXT([external]) or similar to configure.ac.
  AM_GNU_GETTEXT_VERSION([0.17])
  AC_SUBST([LIBINTL])
  AC_SUBST([LTLIBINTL])
  gl_GETTIME
  gl_FUNC_GETTIMEOFDAY
  gl_GLOB
  gl_HASH
  gl_INET_NTOP
  gl_ARPA_INET_MODULE_INDICATOR([inet_ntop])
  gl_INLINE
  gl_INTTYPES_H
  gl_FUNC_LCHMOD
  gl_FUNC_LCHOWN
  gl_UNISTD_MODULE_INDICATOR([lchown])
  gl_LOCALCHARSET
  LOCALCHARSET_TESTS_ENVIRONMENT="CHARSETALIASDIR=\"\$(top_builddir)/$gl_source_base\""
  AC_SUBST([LOCALCHARSET_TESTS_ENVIRONMENT])
  AC_REQUIRE([AC_TYPE_LONG_LONG_INT])
  AC_REQUIRE([AC_TYPE_UNSIGNED_LONG_LONG_INT])
  gl_FUNC_LSEEK
  gl_UNISTD_MODULE_INDICATOR([lseek])
  gl_FUNC_LSTAT
  AC_FUNC_MALLOC
  AC_DEFINE([GNULIB_MALLOC_GNU], 1, [Define to indicate the 'malloc' module.])
  gl_FUNC_MALLOC_POSIX
  gl_STDLIB_MODULE_INDICATOR([malloc-posix])
  gl_MALLOCA
  gl_MBCHAR
  gl_FUNC_MBSLEN
  gl_STRING_MODULE_INDICATOR([mbslen])
  gl_FUNC_MBSSTR
  gl_STRING_MODULE_INDICATOR([mbsstr])
  gl_MBITER
  gl_FUNC_MEMCHR
  gl_FUNC_MEMMOVE
  gl_FUNC_MEMPCPY
  gl_STRING_MODULE_INDICATOR([mempcpy])
  gl_FUNC_MEMRCHR
  gl_STRING_MODULE_INDICATOR([memrchr])
  gl_MINMAX
  gl_MKANCESDIRS
  gl_FUNC_MKDIR_TRAILING_SLASH
  gl_MKDIR_PARENTS
  gl_FUNC_MKSTEMP
  gl_STDLIB_MODULE_INDICATOR([mkstemp])
  gl_FUNC_MKTIME
  gl_FUNC_NANOSLEEP
  gl_HEADER_NETINET_IN
  AC_PROG_MKDIR_P
  gl_FUNC_OPEN
  gl_MODULE_INDICATOR([open])
  gl_FCNTL_MODULE_INDICATOR([open])
  gl_FUNC_OPENAT
  gl_PATHMAX
  gl_QUOTE
  gl_QUOTEARG
  AC_REPLACE_FUNCS(raise)
  gl_FUNC_READLINK
  gl_UNISTD_MODULE_INDICATOR([readlink])
  AC_FUNC_REALLOC
  AC_DEFINE([GNULIB_REALLOC_GNU], 1, [Define to indicate the 'realloc' module.])
  gl_FUNC_REALLOC_POSIX
  gl_STDLIB_MODULE_INDICATOR([realloc-posix])
  gl_REGEX
  gl_FUNC_RENAME
  gl_FUNC_RPMATCH
  gl_STDLIB_MODULE_INDICATOR([rpmatch])
  gl_SAME
  gl_SAVE_CWD
  gl_SAVEWD
  gl_FUNC_SETENV
  gl_STDLIB_MODULE_INDICATOR([setenv])
  gl_SIGACTION
  gl_SIGNAL_MODULE_INDICATOR([sigaction])
  gl_SIGNAL_H
  gl_SIGNALBLOCKING
  gl_SIGNAL_MODULE_INDICATOR([sigprocmask])
  gl_SIZE_MAX
  gl_FUNC_SNPRINTF
  gl_STDIO_MODULE_INDICATOR([snprintf])
  gl_TYPE_SOCKLEN_T
  gt_TYPE_SSIZE_T
  AM_STDBOOL_H
  gl_STDINT_H
  gl_STDIO_H
  gl_STDLIB_H
  gl_STRCASE
  gl_FUNC_STRDUP
  gl_STRING_MODULE_INDICATOR([strdup])
  gl_FUNC_STRERROR
  gl_STRING_MODULE_INDICATOR([strerror])
  gl_FUNC_GNU_STRFTIME
  gl_HEADER_STRING_H
  gl_HEADER_STRINGS_H
  gl_FUNC_STRNDUP
  gl_STRING_MODULE_INDICATOR([strndup])
  gl_FUNC_STRNLEN
  gl_STRING_MODULE_INDICATOR([strnlen])
  gl_FUNC_STRTOIMAX
  gl_INTTYPES_MODULE_INDICATOR([strtoimax])
  gl_FUNC_STRTOL
  gl_FUNC_STRTOLL
  gl_FUNC_STRTOUL
  gl_FUNC_STRTOULL
  gl_FUNC_STRTOUMAX
  gl_INTTYPES_MODULE_INDICATOR([strtoumax])
  gl_HEADER_SYS_SELECT
  AC_PROG_MKDIR_P
  gl_HEADER_SYS_SOCKET
  AC_PROG_MKDIR_P
  gl_HEADER_SYS_STAT_H
  AC_PROG_MKDIR_P
  gl_HEADER_SYS_TIME_H
  AC_PROG_MKDIR_P
  gl_FUNC_GEN_TEMPNAME
  gl_HEADER_TIME_H
  gl_TIME_R
  gl_TIMESPEC
  gl_FUNC_TZSET_CLOBBER
  gl_UNISTD_H
  gl_UNISTD_SAFER
  gl_FUNC_GLIBC_UNLOCKED_IO
  gl_FUNC_UNSETENV
  gl_STDLIB_MODULE_INDICATOR([unsetenv])
  gl_FUNC_VASNPRINTF
  gl_FUNC_VASPRINTF
  gl_STDIO_MODULE_INDICATOR([vasprintf])
  m4_ifdef([AM_XGETTEXT_OPTION],
    [AM_XGETTEXT_OPTION([--flag=asprintf:2:c-format])
     AM_XGETTEXT_OPTION([--flag=vasprintf:2:c-format])])
  gl_WCHAR_H
  gl_WCTYPE_H
  gl_FUNC_WCWIDTH
  gl_WCHAR_MODULE_INDICATOR([wcwidth])
  gl_XALLOC
  gl_XGETCWD
  gl_XSIZE
  gl_XSTRNDUP
  gl_YESNO
  m4_ifval(gl_LIBSOURCES_LIST, [
    m4_syscmd([test ! -d ]m4_defn([gl_LIBSOURCES_DIR])[ ||
      for gl_file in ]gl_LIBSOURCES_LIST[ ; do
        if test ! -r ]m4_defn([gl_LIBSOURCES_DIR])[/$gl_file ; then
          echo "missing file ]m4_defn([gl_LIBSOURCES_DIR])[/$gl_file" >&2
          exit 1
        fi
      done])dnl
      m4_if(m4_sysval, [0], [],
        [AC_FATAL([expected source file, required through AC_LIBSOURCES, not found])])
  ])
  m4_popdef([gl_LIBSOURCES_DIR])
  m4_popdef([gl_LIBSOURCES_LIST])
  m4_popdef([AC_LIBSOURCES])
  m4_popdef([AC_REPLACE_FUNCS])
  m4_popdef([AC_LIBOBJ])
  AC_CONFIG_COMMANDS_PRE([
    gl_libobjs=
    gl_ltlibobjs=
    if test -n "$gl_LIBOBJS"; then
      # Remove the extension.
      sed_drop_objext='s/\.o$//;s/\.obj$//'
      for i in `for i in $gl_LIBOBJS; do echo "$i"; done | sed "$sed_drop_objext" | sort | uniq`; do
        gl_libobjs="$gl_libobjs $i.$ac_objext"
        gl_ltlibobjs="$gl_ltlibobjs $i.lo"
      done
    fi
    AC_SUBST([gl_LIBOBJS], [$gl_libobjs])
    AC_SUBST([gl_LTLIBOBJS], [$gl_ltlibobjs])
  ])
  gltests_libdeps=
  gltests_ltlibdeps=
  m4_pushdef([AC_LIBOBJ], m4_defn([gltests_LIBOBJ]))
  m4_pushdef([AC_REPLACE_FUNCS], m4_defn([gltests_REPLACE_FUNCS]))
  m4_pushdef([AC_LIBSOURCES], m4_defn([gltests_LIBSOURCES]))
  m4_pushdef([gltests_LIBSOURCES_LIST], [])
  m4_pushdef([gltests_LIBSOURCES_DIR], [])
  gl_COMMON
  gl_source_base='tests'
  m4_ifval(gltests_LIBSOURCES_LIST, [
    m4_syscmd([test ! -d ]m4_defn([gltests_LIBSOURCES_DIR])[ ||
      for gl_file in ]gltests_LIBSOURCES_LIST[ ; do
        if test ! -r ]m4_defn([gltests_LIBSOURCES_DIR])[/$gl_file ; then
          echo "missing file ]m4_defn([gltests_LIBSOURCES_DIR])[/$gl_file" >&2
          exit 1
        fi
      done])dnl
      m4_if(m4_sysval, [0], [],
        [AC_FATAL([expected source file, required through AC_LIBSOURCES, not found])])
  ])
  m4_popdef([gltests_LIBSOURCES_DIR])
  m4_popdef([gltests_LIBSOURCES_LIST])
  m4_popdef([AC_LIBSOURCES])
  m4_popdef([AC_REPLACE_FUNCS])
  m4_popdef([AC_LIBOBJ])
  AC_CONFIG_COMMANDS_PRE([
    gltests_libobjs=
    gltests_ltlibobjs=
    if test -n "$gltests_LIBOBJS"; then
      # Remove the extension.
      sed_drop_objext='s/\.o$//;s/\.obj$//'
      for i in `for i in $gltests_LIBOBJS; do echo "$i"; done | sed "$sed_drop_objext" | sort | uniq`; do
        gltests_libobjs="$gltests_libobjs $i.$ac_objext"
        gltests_ltlibobjs="$gltests_ltlibobjs $i.lo"
      done
    fi
    AC_SUBST([gltests_LIBOBJS], [$gltests_libobjs])
    AC_SUBST([gltests_LTLIBOBJS], [$gltests_ltlibobjs])
  ])
  LIBCVS_LIBDEPS="$gl_libdeps"
  AC_SUBST([LIBCVS_LIBDEPS])
  LIBCVS_LTLIBDEPS="$gl_ltlibdeps"
  AC_SUBST([LIBCVS_LTLIBDEPS])
])

# Like AC_LIBOBJ, except that the module name goes
# into gl_LIBOBJS instead of into LIBOBJS.
AC_DEFUN([gl_LIBOBJ], [
  AS_LITERAL_IF([$1], [gl_LIBSOURCES([$1.c])])dnl
  gl_LIBOBJS="$gl_LIBOBJS $1.$ac_objext"
])

# Like AC_REPLACE_FUNCS, except that the module name goes
# into gl_LIBOBJS instead of into LIBOBJS.
AC_DEFUN([gl_REPLACE_FUNCS], [
  m4_foreach_w([gl_NAME], [$1], [AC_LIBSOURCES(gl_NAME[.c])])dnl
  AC_CHECK_FUNCS([$1], , [gl_LIBOBJ($ac_func)])
])

# Like AC_LIBSOURCES, except the directory where the source file is
# expected is derived from the gnulib-tool parameterization,
# and alloca is special cased (for the alloca-opt module).
# We could also entirely rely on EXTRA_lib..._SOURCES.
AC_DEFUN([gl_LIBSOURCES], [
  m4_foreach([_gl_NAME], [$1], [
    m4_if(_gl_NAME, [alloca.c], [], [
      m4_define([gl_LIBSOURCES_DIR], [lib])
      m4_append([gl_LIBSOURCES_LIST], _gl_NAME, [ ])
    ])
  ])
])

# Like AC_LIBOBJ, except that the module name goes
# into gltests_LIBOBJS instead of into LIBOBJS.
AC_DEFUN([gltests_LIBOBJ], [
  AS_LITERAL_IF([$1], [gltests_LIBSOURCES([$1.c])])dnl
  gltests_LIBOBJS="$gltests_LIBOBJS $1.$ac_objext"
])

# Like AC_REPLACE_FUNCS, except that the module name goes
# into gltests_LIBOBJS instead of into LIBOBJS.
AC_DEFUN([gltests_REPLACE_FUNCS], [
  m4_foreach_w([gl_NAME], [$1], [AC_LIBSOURCES(gl_NAME[.c])])dnl
  AC_CHECK_FUNCS([$1], , [gltests_LIBOBJ($ac_func)])
])

# Like AC_LIBSOURCES, except the directory where the source file is
# expected is derived from the gnulib-tool parameterization,
# and alloca is special cased (for the alloca-opt module).
# We could also entirely rely on EXTRA_lib..._SOURCES.
AC_DEFUN([gltests_LIBSOURCES], [
  m4_foreach([_gl_NAME], [$1], [
    m4_if(_gl_NAME, [alloca.c], [], [
      m4_define([gltests_LIBSOURCES_DIR], [tests])
      m4_append([gltests_LIBSOURCES_LIST], _gl_NAME, [ ])
    ])
  ])
])

# This macro records the list of files which have been installed by
# gnulib-tool and may be removed by future gnulib-tool invocations.
AC_DEFUN([gl_FILE_LIST], [
  build-aux/config.rpath
  build-aux/link-warning.h
  doc/getdate.texi
  lib/alloca.c
  lib/alloca.in.h
  lib/areadlink-with-size.c
  lib/areadlink.h
  lib/arpa_inet.in.h
  lib/asnprintf.c
  lib/asprintf.c
  lib/at-func.c
  lib/atexit.c
  lib/base64.c
  lib/base64.h
  lib/basename.c
  lib/c-ctype.c
  lib/c-ctype.h
  lib/canon-host.c
  lib/canon-host.h
  lib/canonicalize-lgpl.c
  lib/canonicalize.c
  lib/canonicalize.h
  lib/chdir-long.c
  lib/chdir-long.h
  lib/chown.c
  lib/close-stream.c
  lib/close-stream.h
  lib/closeout.c
  lib/closeout.h
  lib/config.charset
  lib/creat-safer.c
  lib/dirchownmod.c
  lib/dirchownmod.h
  lib/dirent.in.h
  lib/dirfd.c
  lib/dirfd.h
  lib/dirname.c
  lib/dirname.h
  lib/dup-safer.c
  lib/dup2.c
  lib/errno.in.h
  lib/error.c
  lib/error.h
  lib/exitfail.c
  lib/exitfail.h
  lib/fchdir.c
  lib/fchmodat.c
  lib/fchown-stub.c
  lib/fchownat.c
  lib/fcntl--.h
  lib/fcntl-safer.h
  lib/fcntl.in.h
  lib/fd-safer.c
  lib/file-set.c
  lib/file-set.h
  lib/filenamecat.c
  lib/filenamecat.h
  lib/float+.h
  lib/float.in.h
  lib/fnmatch.c
  lib/fnmatch.in.h
  lib/fnmatch_loop.c
  lib/fpending.c
  lib/fpending.h
  lib/freadahead.c
  lib/freadahead.h
  lib/freadptr.c
  lib/freadptr.h
  lib/freadseek.c
  lib/freadseek.h
  lib/fseeko.c
  lib/fstatat.c
  lib/ftello.c
  lib/ftruncate.c
  lib/gai_strerror.c
  lib/getaddrinfo.c
  lib/getaddrinfo.h
  lib/getcwd.c
  lib/getdate.h
  lib/getdate.y
  lib/getdelim.c
  lib/gethostname.c
  lib/getline.c
  lib/getlogin_r.c
  lib/getndelim2.c
  lib/getndelim2.h
  lib/getnline.c
  lib/getnline.h
  lib/getopt.c
  lib/getopt.in.h
  lib/getopt1.c
  lib/getopt_int.h
  lib/getpagesize.c
  lib/getpass.c
  lib/getpass.h
  lib/gettext.h
  lib/gettime.c
  lib/gettimeofday.c
  lib/glob-libc.h
  lib/glob.c
  lib/glob.in.h
  lib/hash-pjw.c
  lib/hash-pjw.h
  lib/hash-triple.c
  lib/hash-triple.h
  lib/hash.c
  lib/hash.h
  lib/inet_ntop.c
  lib/intprops.h
  lib/inttypes.in.h
  lib/lchmod.h
  lib/lchown.c
  lib/localcharset.c
  lib/localcharset.h
  lib/lseek.c
  lib/lstat.c
  lib/lstat.h
  lib/malloc.c
  lib/malloca.c
  lib/malloca.h
  lib/malloca.valgrind
  lib/mbchar.c
  lib/mbchar.h
  lib/mbslen.c
  lib/mbsstr.c
  lib/mbuiter.h
  lib/md5.c
  lib/md5.h
  lib/memchr.c
  lib/memchr2.c
  lib/memchr2.h
  lib/memmove.c
  lib/mempcpy.c
  lib/memrchr.c
  lib/minmax.h
  lib/mkancesdirs.c
  lib/mkancesdirs.h
  lib/mkdir-p.c
  lib/mkdir-p.h
  lib/mkdir.c
  lib/mkdirat.c
  lib/mkstemp.c
  lib/mktime.c
  lib/nanosleep.c
  lib/netinet_in.in.h
  lib/open-safer.c
  lib/open.c
  lib/openat-die.c
  lib/openat-priv.h
  lib/openat-proc.c
  lib/openat.c
  lib/openat.h
  lib/pathmax.h
  lib/pipe-safer.c
  lib/printf-args.c
  lib/printf-args.h
  lib/printf-parse.c
  lib/printf-parse.h
  lib/quote.c
  lib/quote.h
  lib/quotearg.c
  lib/quotearg.h
  lib/raise.c
  lib/readlink.c
  lib/realloc.c
  lib/ref-add.sin
  lib/ref-del.sin
  lib/regcomp.c
  lib/regex.c
  lib/regex.h
  lib/regex_internal.c
  lib/regex_internal.h
  lib/regexec.c
  lib/rename.c
  lib/rpmatch.c
  lib/same-inode.h
  lib/same.c
  lib/same.h
  lib/save-cwd.c
  lib/save-cwd.h
  lib/savewd.c
  lib/savewd.h
  lib/setenv.c
  lib/sig-handler.h
  lib/sigaction.c
  lib/signal.in.h
  lib/sigprocmask.c
  lib/size_max.h
  lib/snprintf.c
  lib/stat-macros.h
  lib/stdbool.in.h
  lib/stdint.in.h
  lib/stdio-impl.h
  lib/stdio.in.h
  lib/stdlib.in.h
  lib/str-kmp.h
  lib/strcasecmp.c
  lib/strdup.c
  lib/streq.h
  lib/strerror.c
  lib/strftime.c
  lib/strftime.h
  lib/string.in.h
  lib/strings.in.h
  lib/stripslash.c
  lib/strncasecmp.c
  lib/strndup.c
  lib/strnlen.c
  lib/strnlen1.c
  lib/strnlen1.h
  lib/strtoimax.c
  lib/strtol.c
  lib/strtoll.c
  lib/strtoul.c
  lib/strtoull.c
  lib/strtoumax.c
  lib/sys_select.in.h
  lib/sys_socket.in.h
  lib/sys_stat.in.h
  lib/sys_time.in.h
  lib/tempname.c
  lib/tempname.h
  lib/time.in.h
  lib/time_r.c
  lib/timespec.h
  lib/unistd--.h
  lib/unistd-safer.h
  lib/unistd.in.h
  lib/unitypes.h
  lib/uniwidth.h
  lib/uniwidth/cjk.h
  lib/uniwidth/width.c
  lib/unlocked-io.h
  lib/unsetenv.c
  lib/vasnprintf.c
  lib/vasnprintf.h
  lib/vasprintf.c
  lib/verify.h
  lib/wchar.in.h
  lib/wctype.in.h
  lib/wcwidth.c
  lib/xalloc-die.c
  lib/xalloc.h
  lib/xgetcwd.c
  lib/xgetcwd.h
  lib/xgethostname.c
  lib/xgethostname.h
  lib/xmalloc.c
  lib/xsize.h
  lib/xstrndup.c
  lib/xstrndup.h
  lib/yesno.c
  lib/yesno.h
  m4/alloca.m4
  m4/arpa_inet_h.m4
  m4/atexit.m4
  m4/base64.m4
  m4/bison.m4
  m4/canon-host.m4
  m4/canonicalize-lgpl.m4
  m4/canonicalize.m4
  m4/chdir-long.m4
  m4/chown.m4
  m4/clock_time.m4
  m4/close-stream.m4
  m4/closeout.m4
  m4/codeset.m4
  m4/d-ino.m4
  m4/d-type.m4
  m4/dirfd.m4
  m4/dirname.m4
  m4/dos.m4
  m4/double-slash-root.m4
  m4/dup2.m4
  m4/eealloc.m4
  m4/environ.m4
  m4/errno_h.m4
  m4/error.m4
  m4/exitfail.m4
  m4/extensions.m4
  m4/fchdir.m4
  m4/fcntl-safer.m4
  m4/fcntl_h.m4
  m4/filenamecat.m4
  m4/float_h.m4
  m4/fnmatch.m4
  m4/fpending.m4
  m4/fseeko.m4
  m4/ftello.m4
  m4/ftruncate.m4
  m4/getaddrinfo.m4
  m4/getcwd-abort-bug.m4
  m4/getcwd-path-max.m4
  m4/getcwd.m4
  m4/getdate.m4
  m4/getdelim.m4
  m4/gethostname.m4
  m4/getline.m4
  m4/getlogin_r.m4
  m4/getndelim2.m4
  m4/getnline.m4
  m4/getopt.m4
  m4/getpagesize.m4
  m4/getpass.m4
  m4/gettext.m4
  m4/gettime.m4
  m4/gettimeofday.m4
  m4/glibc2.m4
  m4/glibc21.m4
  m4/glob.m4
  m4/gnulib-common.m4
  m4/hash.m4
  m4/iconv.m4
  m4/include_next.m4
  m4/inet_ntop.m4
  m4/inline.m4
  m4/intdiv0.m4
  m4/intl.m4
  m4/intldir.m4
  m4/intlmacosx.m4
  m4/intmax.m4
  m4/intmax_t.m4
  m4/inttypes-pri.m4
  m4/inttypes.m4
  m4/inttypes_h.m4
  m4/lchmod.m4
  m4/lchown.m4
  m4/lcmessage.m4
  m4/lib-ld.m4
  m4/lib-link.m4
  m4/lib-prefix.m4
  m4/localcharset.m4
  m4/lock.m4
  m4/longlong.m4
  m4/lseek.m4
  m4/lstat.m4
  m4/malloc.m4
  m4/malloca.m4
  m4/mbchar.m4
  m4/mbiter.m4
  m4/mbrtowc.m4
  m4/mbslen.m4
  m4/mbsstr.m4
  m4/mbstate_t.m4
  m4/md5.m4
  m4/memchr.m4
  m4/memmove.m4
  m4/mempcpy.m4
  m4/memrchr.m4
  m4/minmax.m4
  m4/mkancesdirs.m4
  m4/mkdir-p.m4
  m4/mkdir-slash.m4
  m4/mkstemp.m4
  m4/mktime.m4
  m4/nanosleep.m4
  m4/netinet_in_h.m4
  m4/nls.m4
  m4/open.m4
  m4/openat.m4
  m4/pathmax.m4
  m4/po.m4
  m4/printf-posix.m4
  m4/printf.m4
  m4/progtest.m4
  m4/quote.m4
  m4/quotearg.m4
  m4/readlink.m4
  m4/realloc.m4
  m4/regex.m4
  m4/rename.m4
  m4/rpmatch.m4
  m4/same.m4
  m4/save-cwd.m4
  m4/savewd.m4
  m4/setenv.m4
  m4/sigaction.m4
  m4/signal_h.m4
  m4/signalblocking.m4
  m4/size_max.m4
  m4/snprintf.m4
  m4/socklen.m4
  m4/sockpfaf.m4
  m4/ssize_t.m4
  m4/stdbool.m4
  m4/stdint.m4
  m4/stdint_h.m4
  m4/stdio_h.m4
  m4/stdlib_h.m4
  m4/strcase.m4
  m4/strdup.m4
  m4/strerror.m4
  m4/strftime.m4
  m4/string_h.m4
  m4/strings_h.m4
  m4/strndup.m4
  m4/strnlen.m4
  m4/strtoimax.m4
  m4/strtol.m4
  m4/strtoll.m4
  m4/strtoul.m4
  m4/strtoull.m4
  m4/strtoumax.m4
  m4/sys_select_h.m4
  m4/sys_socket_h.m4
  m4/sys_stat_h.m4
  m4/sys_time_h.m4
  m4/tempname.m4
  m4/threadlib.m4
  m4/time_h.m4
  m4/time_r.m4
  m4/timespec.m4
  m4/tm_gmtoff.m4
  m4/tzset.m4
  m4/uintmax_t.m4
  m4/unistd-safer.m4
  m4/unistd_h.m4
  m4/unlocked-io.m4
  m4/vasnprintf.m4
  m4/vasprintf.m4
  m4/visibility.m4
  m4/wchar.m4
  m4/wchar_t.m4
  m4/wctype.m4
  m4/wcwidth.m4
  m4/wint_t.m4
  m4/xalloc.m4
  m4/xgetcwd.m4
  m4/xsize.m4
  m4/xstrndup.m4
  m4/yesno.m4
])