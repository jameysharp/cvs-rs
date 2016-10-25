/*
 * Copyright (C) 1986-2007 The Free Software Foundation, Inc.
 *
 * Portions Copyright (C) 1998-2007 Derek Price,
 *                                  Ximbiot LLC <http://ximbiot.com>,
 *                                  and others.
 *
 * Portions Copyright (C) 1994 david d `zoo' zuhn
 * Portions Copyright (C) 1992, Brian Berliner and Jeff Polk
 * Portions Copyright (C) 1989-1992, Brian Berliner
 * 
 * You may distribute under the terms of the GNU General Public License as
 * specified in the README file that comes with this  CVS source distribution.
 * 
 * version.c - the CVS version number
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "cvs.h"

static const char *const version_usage[] =
{
    "Usage: %s %s\n",
    NULL
};



/*
 * Output a version string for the client and server.
 *
 * This function will output the simple version number (for the '--version'
 * option) or the version numbers of the client and server (using the 'version'
 * command).
 */
int
version (int argc, char **argv)
{
    int err = 0;

    if (argc == -1)
	usage (version_usage);

    if (current_parsed_root && current_parsed_root->isremote)
        (void) fputs ("Client: ", stdout);

    /* Having the year here is a good idea, so people have
       some idea of how long ago their version of CVS was
       released.  */
    (void) fputs (PACKAGE_CONFIG, stdout);
    (void) fputc ('\n', stdout);

#ifdef CLIENT_SUPPORT
    if (current_parsed_root && current_parsed_root->isremote)
    {
	(void) fputs ("Server: ", stdout);
	start_server ();
	if (supported_request ("version"))
	    send_to_server ("version\012", 0);
	else
	{
	    send_to_server ("noop\012", 0);
	    fputs ("(unknown)\n", stdout);
	}
	err = get_responses_and_close ();
    }
#endif
    return err;
}
	
