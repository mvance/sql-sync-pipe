sql-sync-pipe (ssp, pipe)
=============

Drush command for copying and importing source database to target database. Transfers as a gzipped pipe via SSH.

```
Example: drush pipe @alias.dev @alias.sandbox --progress --dump --temp --sanitize --confirm-sanitizations


Arguments:
 source                                    Name of the site-alias to use as a source.
 destination                               Name of the site-alias to use as the destination.
 
 
Options:
 --dump                                    Dump to a local file, this will help determine an ETA if the
                                           using the progress option.
 --progress                                If the Pipe Viewer command is installed, show the progress of
                                           the sync. See: http://www.ivarch.com/programs/pv.shtml. Common
                                           installs: UNIX - "yum install pv" or "apt-get install pv",
                                           MacPorts - "port install pv", HomeBrew - "brew install pv". See
                                           URL for a complete list of installation instructions.
 --sanitize                                Obscure email addresses and reset passwords in the user table
                                           post-sync. Optional.
   --sanitize-password                     The password to assign to all accounts in the sanitization
                                           operation, or "no" to keep passwords unchanged.  Default is
                                           "password".
   --sanitize-email                        The pattern for test email addresses in the sanitization
                                           operation, or "no" to keep email addresses unchanged.  May
                                           contain replacement patterns %uid, %mail or %name.  Default is
                                           "user+%uid@localhost".
   --confirm-sanitizations                 Prompt yes/no after importing the database, but before running
                                           the sanitizations
 --ssh-options                             A string of extra options that will be passed to the ssh command
                                           (e.g. "-p 100")
 --temp                                    Use a temporary file to hold dump files.
```
