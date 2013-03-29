sql-sync-pipe (ssp, pipe)
=============
Drush command for copying and importing source database to target database. Transfers as a gzipped pipe via SSH.


Standard `sql-sync` requires that you save the database dump on the server, transfer it via rsync and then import it
into your destination's database. There is actually no compression going on whatsoever. This normally would probably
be fine if you have a small site. But lets say that you have a DB that's huge, 1GB+ huge. This is a very inefficient
method of transportation.

Below are examples of syncing the same database using different methods for the same 1.05GiB database. Keep in mind
that these "benchmarks" are relative to the connection speed of both the source (server) and destination (local
machine). It should also be mentioned that even while, yes, the time it takes to sync a database improves, the real
value of this command lies in the reduced bandwidth consumption (transfer size) between the source and destination.

## Benchmarks
#### drush sql-sync
The standard `sql-sync` command shipped with drush.
NOTE: This benchmark simply measures the time elapsed from the start of the command to the end. There is no real way
to measure individual the individual dump, transfer and import steps are they are done silently. In theory you could
watch the debug `-d` output, if you wanted to... I didn't.
```
Command:            drush sql-sync @alias.dev @alias.sandbox --no-cache
Transfer Size:      1.05GiB (uncompressed)
Total Time Elapsed: 46 minutes 47 seconds
```

#### drush sql-sync-pipe
This command pipes the entire syncing process. `mysqldump` is passed the following options to make the quickest
and smallest dump as possible: `-CceKq --single-transaction`. It then pipes the dump via `gzip`, passes through the
ssh tunnel, `gunzip` and then imports directly into the destination's database.
```
Command:                 drush sql-sync-pipe @alias.dev @alias.sandbox --progress
Transfer Size:           88.1MiB (compressed using gzip)
Import Size:             1.05GiB
Import & Transfer Time:  27 minutes 05 seconds
Total Time Elapsed:      30 minutes and 35 seconds (includes time taken before initial transfer started)
```

#### drush sql-sync-pipe --dump
If you need to save the dump or simply wish to give an ETA estimate when using the `--progress` option, specify the
`--dump` option. It is similiar to using sql-sync, however there are a few distinct differences. It will still pass
the same options to `mysqldump` as above. Where it differs, is that it will `gzip` the dump and then pass it via the
ssh tunnel so it saves to the destination's HDD (local machine) instead of the sources (server). After the dump has
been saved, it will then pipe the `gunzip` and the import process. 

NOTE: If the destination alias has specificed a dump file, that will be used and remain on the destination's HDD. If
the destination does not have a dump file specified or the `--temp` option is used, a temporary file will be created
and then removed afterwards.
```
Command:            drush sql-sync-pipe @alias.dev @alias.sandbox --progress --dump --temp
Transfer Size:      88.1MiB (compressed using gzip)
Transfer Time:      5 minutes 14 seconds (includes time taken to gunzip)
Import Size:        1.05GiB
Import Time:        29 minutes 23 seconds
Total Time Elapsed: 37 minutes and 36 seconds (includes time taken before initial transfer started)
```

## drush --help sql-sync-pipe
```
Copy and import source database to target database. Transfers as a gzipped pipe via SSH.

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
 --ssh-options                             A string of extra options that will be passed to the ssh
                                           command (e.g. "-p 100")
 --temp                                    Use a temporary file to hold dump files.


Aliases: ssp, pipe
```
