## Process this file with automake to produce Makefile.in
# Makefile for GNU CVS program.
#
# Copyright (C) 1986-2005 The Free Software Foundation, Inc.
#
# Portions Copyright (C) 1998-2005 Derek Price, Ximbiot <http://ximbiot.com>,
#                                  and others.

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

SHELL = /bin/sh

AM_CPPFLAGS = -I$(top_srcdir)/lib -I$(top_builddir)/lib \
              -I$(top_srcdir)/diff $(ZLIB_CPPFLAGS)

bin_PROGRAMS = cvs
bin_SCRIPTS = cvsbug

# The cvs executable
cvs_SOURCES = \
	add.c \
	admin.c \
	annotate.c \
	base.c base.h \
	buffer.c \
	checkin.c \
	checkout.c \
	classify.c classify.h \
	client.c \
	command_line_opt.h \
	commit.c \
	create_adm.c \
	cvsrc.c \
	diff.c diff.h \
	difflib.c difflib.h \
	edit.c \
	entries.c entries.h \
	error.c \
	exithandle.c \
	expand_path.c \
	fileattr.c \
	filesubr.c filesubr.h \
	find_names.c \
	gpg.c gpg.h \
	hardlink.c \
	hash.c \
	history.c \
	ignore.c ignore.h \
	import.c \
	lock.c \
	log.c \
	log-buffer.c log-buffer.h \
	login.c \
	logmsg.c logmsg.h \
	ls.c \
	main.c \
	mkmodules.c \
	modules.c \
	ms-buffer.c ms-buffer.h \
	myndbm.c \
	no_diff.c no_diff.h \
	parseinfo.c parseinfo.h \
	patch.c \
	rcs.c \
	rcscmds.c \
	recurse.c recurse.h \
	release.c \
	remove.c \
	repos.c repos.h \
	root.c \
	rsh-client.c rsh-client.h \
	run.c run.h \
	scramble.c \
	server.c \
	sign.c sign.h \
	stack.c stack.h \
	status.c \
	subr.c subr.h \
	tag.c \
	update.c \
	verify.c verify.h \
	version.c \
	vers_ts.c vers_ts.h \
	watch.c \
	wrapper.c wrapper.h \
	zlib.c \
	buffer.h \
	client.h \
	cvs.h \
	edit.h \
	fileattr.h \
	hardlink.h \
	hash.h \
	history.h \
	myndbm.h \
	rcs.h \
	root.h \
	server.h \
	update.h \
	watch.h

EXTRA_cvs_SOURCES = gssapi-client.c gssapi-client.h \
                    kerberos4-client.c kerberos4-client.h \
		    socket-client.c socket-client.h

cvs_DEPENDENCIES = $(cvs_client_objects) \
	../diff/libdiff.a \
	../lib/libcvs.a \
	$(ZLIB_LIBS)
cvs_LDADD = $(cvs_client_objects) \
	../diff/libdiff.a \
	../lib/libcvs.a \
	$(ZLIB_LIBS) \
	$(LIB_CLOCK_GETTIME) \
	$(LIB_NANOSLEEP) \
	$(LIBINTL)

# General
EXTRA_DIST = \
	.cvsignore \
	ChangeLog-9194 \
	ChangeLog-9395 \
	ChangeLog-96 \
	ChangeLog-97 \
	build_src.com \
	sanity.sh

check-local: localcheck remotecheck nobaseremotecheck proxycheck

.PHONY: maintainercheck-local
maintainercheck-local: check-local nobaselocalcheck nobaseproxycheck

.PHONY: localcheck
localcheck: all sanity.config.sh
	$(SHELL) $(srcdir)/sanity.sh `pwd`/cvs$(EXEEXT)

.PHONY: nobaselocalcheck
nobaselocalcheck: all sanity.config.sh
	$(SHELL) $(srcdir)/sanity.sh -B `pwd`/cvs$(EXEEXT)

.PHONY: remotecheck
remotecheck: all sanity.config.sh
	$(SHELL) $(srcdir)/sanity.sh -r `pwd`/cvs$(EXEEXT)

.PHONY: nobaseremotecheck
nobaseremotecheck: all sanity.config.sh
	$(SHELL) $(srcdir)/sanity.sh -Br `pwd`/cvs$(EXEEXT)

.PHONY: proxycheck
proxycheck: all sanity.config.sh
	$(SHELL) $(srcdir)/sanity.sh -p `pwd`/cvs$(EXEEXT)

.PHONY: nobaseproxycheck
nobaseproxycheck: all sanity.config.sh
	$(SHELL) $(srcdir)/sanity.sh -Bp `pwd`/cvs$(EXEEXT)

# Our distclean targets
distclean-local:
	rm -f check.log check.plog check.plog~

## MAINTAINER Targets

# for backwards compatibility with the old makefiles
.PHONY: realclean
realclean: maintainer-clean
