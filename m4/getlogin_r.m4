#serial 4

# Copyright (C) 2005, 2006, 2007 Free Software Foundation, Inc.
#
# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

dnl From Derek Price
dnl
dnl Provide getlogin_r when the system lacks it.
dnl

AC_DEFUN([gl_GETLOGIN_R],
[
  AC_REQUIRE([gl_UNISTD_H_DEFAULTS])
  AC_CHECK_FUNCS_ONCE([getlogin_r])
  if test $ac_cv_func_getlogin_r = no; then
    AC_LIBOBJ([getlogin_r])
    gl_PREREQ_GETLOGIN_R
    if test $ac_cv_have_decl_getlogin_r = yes; then
      HAVE_DECL_GETLOGIN_R=1
    else
      HAVE_DECL_GETLOGIN_R=0
    fi
  fi
])

AC_DEFUN([gl_PREREQ_GETLOGIN_R],
[
  AC_CHECK_DECLS_ONCE([getlogin])
  AC_CHECK_DECLS_ONCE([getlogin_r])
])
