sql-sync-pipe (ssp, pipe)
=============
Drush command for copying and importing source database to target database. Transfers as a gzipped pipe via SSH.


Standard `sql-sync` requires that you save the database dump on the server, transfer it via rsync and then import it
into your destination's database. There is actually no compression going on whatsoever. This normally would probably
be fine if you have a small site. But lets say that you have a DB that's huge, 1GB+ huge. This is a very inefficient
method of transportation.

Below are examples of syncing the same database using different methods for a 1GB database. Keep in mind that these
"benchmarks" are relative to the connection speed of both the source (server) and destination (your machine).

## Benchmarks
#### drush sql-sync
The standard `sql-sync` command shipped with drush.
NOTE: This benchmark simply measures the time elapsed from the start of the command to the end. There is no real way
to measure individual the individual dump, transfer and import steps are they are done silently. In theory you could
watch the debug `-d` output, if you wanted to... I didn't.
```
Command:            drush sql-sync @alias.dev @alias.sandbox --no-cache
Transfer Size:      1.05GiB (uncompressed)
Total Time Elapsed: 0 hours 46 minutes 47 seconds (because both the transfer and import are done silently, 
```

#### drush sql-sync-pipe
```
Command:                 drush sql-sync-pipe @alias.dev @alias.sandbox --progress
Transfer Size:           88.1MiB (compressed using gzip)
Import Size:             1.05GiB
Import & Transfer Time:  
Total Time Elapsed:      (includes time taken before initial transfer started)
```

#### drush sql-sync-pipe --dump
The new method using the `--dump` option. If you provide the `--dump` option, it is similiar to using sql-sync,
however there are a few distinct differences. First, `mysqldump` is passed the following options to make the quickest
and smallest dump as possible: `-CceKq --single-transaction`. Second, the dump is directly piped through `gzip` to 
further the compression of the transfer and then piped again via SSH so it saves directly on the destination's HDD
instead of the sources (server). This saves valuable time. Lastly, the dump file (located on the destination machine)
is uncompressed using `gunzip` and then piped and imported directly into the destinations database. It should be noted
that if you use the `--temp` option, the dumped file will be deleted afterwards.
```
Command:            drush sql-sync-pipe @alias.dev @alias.sandbox --progress --dump --temp
Transfer Size:      88.1MiB (compressed using gzip)
Transfer Time:      0 hours 5 minutes 14 seconds (includes time taken to gunzip)
Import Size:        1.05GiB
Import Time:        0 hours 29 minutes 23 seconds
Total Time Elapsed: 0 hours 37 minutes and 36 seconds (includes time taken before initial transfer started)
```

## Arguments & Options
```
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
