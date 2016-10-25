#! @PERL@ -T
# -*-Perl-*-

# Copyright (C) 1994-2006 The Free Software Foundation, Inc.

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# Perl filter to handle pre-commit checking of files.
#
# The CVS workflow is:
# - process all commitinfo hooks
# - actually commit
# - process all loginfo hooks
#
# This program records the last directory where commits will be taking
# place for use by the log_accum.pl script.
#
# IMPORTANT: commit_prep and log_accumy have to agree on the tmpfile
# name to use.  See $LAST_FILE below.
#
# Sample CVSROOT/commitinfo:
# ALL /usr/local/bin/commit_prep -T ccvs_1 %p
# ^prog1\(/\|$\) /usr/local/bin/commit_prep -T ccvs_2 %p
#
# Contributed by David Hampton <hampton@cisco.com>
# Stripped to minimum by Roy Fielding
# Changes by Sylvain Beucler <beuc@beuc.net> (2006-05-08):
# - option -T added again to support multiple log_accum hooks
# - deprecated misleading option -u
# - used 'use strict' and added compatibility for 'perl -T' switch
# - documented some more
# - removed $cvs_user in the temporary filename - its value is not
#   compatible with log_accum's and it's safer to use -T
#
############################################################

use strict;

# CONSTANTS
my $TMPDIR          = '/tmp';


# Options
my $temp_name       = "cvs";
my $full_directory_path = '';

while (@ARGV)
{
    my $arg = shift @ARGV;

    # -T is a string to be included in the $last_file filename. It is
    # necessary to pass different -T options to commit_prep if you
    # need to call it for different scripts in the same commit (eg:
    # call log_accum with different parameters in module/ and in ALL)
    if ($arg eq '-T' || $arg eq '-u')
    {
	warn "Using deprecated -u option. Use -T instead." if $arg eq '-u';

	my $param = shift @ARGV;
	if ($param =~ /^([a-zA-Z0-9_.-]+)$/)
	{
	    $temp_name = $1;
	}
	else
	{
	    die "Invalid identifier passed to option $arg: $param";
	}
    }
    else
    {
	# The non-option argument is a relative path to the current
	# commit directory.  It is written in a file and read by
	# log_accum after being character-escaped.  No security issues
	# here. We still check for '..' and ensure this is not a full
	# path.
	if ($arg !~ m#(^|/)\.\.(/|\$)# and $arg =~ m#^([^/].*)$#)
	{
	    $full_directory_path = $1;
	}
	else
	{
	    die "Commit path should be relative to CVSROOT: $arg";
	}
    }
}

die "Usage: $0 [-T hook_identifier] current_commit_path"
    if $full_directory_path eq '';



# This needs to match the corresponding var in log_accum.pl, including
# the appending of the pgrp and hook identifier suffixes (see uses of
# this var farther down).
my $id = getpgrp();
my $last_file = "$TMPDIR/#$temp_name.$id.lastdir";

# Record this directory as the last one checked.  This will be used
# by the log_accumulate script to determine when it is processing
# the final directory of a multi-directory commit.
open FILE, "> $last_file" or die "Cannot open $last_file: $!\n";
print FILE $full_directory_path, "\n";
close FILE;

exit 0;
