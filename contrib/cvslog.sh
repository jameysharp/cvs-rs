#!/bin/sh
#
# $Id: cvslog.sh,v 1.1 2006/04/18 16:07:40 mdb Exp $
#
# This script is a rewrite of 'log.pl' as provided with the CVS sources
#
# Copyright (c) 2006, Jan Schaumann <jschauma@netmeister.org>
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. Neither the name of the author nor the names of any contributors
#    may be used to endorse or promote products derived from this software
#    without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
# OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.


# Usage:  cvslog.sh [-u user] [[-m mailto] ...] [-s] [-V]
#                   [-a rev] [-b rev] -f logfile dirname file ...
#
#	-u user		- $USER passed from loginfo
#	-m mailto	- for each user to receive cvs log reports
#			(multiple -m's permitted)
#	-s		- to prevent "cvs status -v" messages
#	-V		- without '-s', don't pass '-v' to cvs status
#	-f logfile	- for the logfile to append to (mandatory,
#			but only one logfile can be specified).

# Tested with NetBSD's bourne shell, /bin/sh.

# here is what the output looks like:
#
#    From: woods@kuma.domain.top
#    Subject: CVS update: testmodule
#
#    Module name:  misc
#    Commited by:  jschauma
#    Date:         Tue Apr 11 21:56:58 EDT 2006
#
#    Modified Files:
#    	test3 
#    Added Files:
#    	test6 
#    Removed Files:
#    	test4 
#
#    Log Message:
#    - wow, what a test
#
# (and for each file the "cvs status -v" output is appended unless -s is used)
#
#    ==================================================================
#    File: test3           	Status: Up-to-date
#    
#       Working revision:	1.41	Wed Nov 23 14:15:59 1994
#       Repository revision:	1.41	/local/src-CVS/cvs/testmodule/test3,v
#       Sticky Options:	-ko
#    
#       Existing Tags:
#    	local-v2                 	(revision: 1.7)
#    	local-v1                 	(revision: 1.1.1.2)
#    	CVS-1_4A2                	(revision: 1.1.1.2)
#    	local-v0                 	(revision: 1.2)
#    	CVS-1_4A1                	(revision: 1.1.1.1)
#    	CVS                      	(branch: 1.1.1)

errx()
{
	echo "$0: error: $@"
	exit 1
}

log_header()
{
	DATE=`date`
	echo ""
	echo "Module name:  ${MODULE}"
	echo "Commited by:  ${LOGIN}"
	echo "Date:         ${DATE}"
	echo ""
}

usage()
{
	optstring="[-h|-?] [-u user] [[-m mailto] ...] [-s] [-V] -f logfile"
	echo "Usage: $0 $optstring 'dirname file ...'"
	echo " -?,-h        print a usage message and exit successfully"
	echo " -u user      \$USER passed from loginfo"
	echo " -m mailto    for each user to receive cvs log reports"
	echo "              (multiple -m's permitted)"
	echo " -s           to prevent "cvs status -v" messages"
	echo " -V           without '-s', don't pass '-v' to cvs status"
	echo " -f logfile   for the logfile to append to"
	echo "              (mandatory, but only one logfile can be specified)"
}

write()
{
	echo "$@" >> ${LOGFILE}
	if [ ! -z "${USERS}" ]; then
		echo "$@" >> ${TMPFILE}
	fi
}

DOSTATUS=1
VERBOSESTATUS=1
LOGFILE=""
USERS=""
LOGIN=`whoami`
TMPFILE=/dev/null

while getopts ?hf:m:u:sV opts
do
	case "$opts" in
		h|\?)
			usage
			exit 0
			;;
		f)
			if [ x"${LOGFILE}" = x"" ]; then
				LOGFILE="${OPTARG}"
			else
				errx "Too many '-f' args."
			fi
			;;
		m)
			USERS="${OPTARG} ${USERS}"
			;;
		u)
			LOGIN="${OPTARG}"
			;;
		s)
			DOSTATUS=0
			;;
		V)
			VERBOSESTATUS=0
			;;
		-)
			shift; break
	esac
done
shift `expr $OPTIND - 1`

if [ x"${LOGFILE}" = x"" ]; then
	errx "No logfile provided."
fi

MODULE=${1%% *}
shift

if [ ! -z "${USERS}" ]; then
	TMPFILE=`mktemp /tmp/cvslog.XXXXXX` || errx "unable to create temporary file"
	log_header >> ${TMPFILE}
fi

log_header >> ${LOGFILE}
if [ $? -gt 0 ]; then
	errx "Unable to write to ${LOGFILE}"
fi

IFS=""
while read line; do
	wline=`echo $line | sed -e 's/Update of .*//' -e 's/In directory .*//'`
	if [ "${wline%% *}" = "Log" ]; then
		write ""
	fi
	if [ ! -z "${wline}" ]; then
		write "$wline"
	fi
done

write ""

if [ "${DOSTATUS}" != 0 ]; then
	for f in "$@"; do
		if [ "$f" = "-" ]; then
			write "input file was '-']"
		fi
		CVSCMD="cvs -nQq status"
		if [ "${VERBOSESTATUS}" = 1 ]; then
			CVSCMD="${CVSCMD} -v"
		fi
		$CVSCMD ${f} | tee -a ${TMPFILE} >> ${LOGFILE}
	done
fi


if [ ! -z "${USERS}" ]; then
	mail -s "CVS update: $MODULE" ${USERS} <${TMPFILE}
	rm -f ${TMPFILE}
fi
