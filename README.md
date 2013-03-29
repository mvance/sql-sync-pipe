drush sql-sync-pipe (ssp, pipe)
=============
Drush command for copying and importing source database to target database. Transfers via a gzipped pipe and the SSH
tunnel.


Standard `sql-sync` requires that you save the database dump on the server, transfer it via rsync and then import it
into your destination's database. There is actually no compression going on whatsoever. This normally would probably
be fine if you have a small site. But lets say that you have a database that's huge, 1GB+ huge. This is a very
inefficient method of transportation.

Below are examples of syncing the same 1.05Gib database using the two different methods. Keep in mind that these
"benchmarks" are relative to the connection speed of both the source (server) and destination (local machine). It
should also be mentioned that even while, yes, the time it takes to sync a database improves, the real
value of this command lies in the reduced bandwidth consumption (transfer size) between the source and destination
using `gzip`.

## Benchmarks
#### drush sql-sync
The standard `sql-sync` command shipped with drush.

NOTE: This benchmark simply measures the time elapsed from the start of the command to the end. There is no real way
to measure the individual dump, transfer and import steps as they are done silently. In theory you _could_ watch the
debug `-d` output, if you wanted to... I didn't.
```
Command:            drush sql-sync @alias.dev @alias.sandbox --no-cache
Transfer size:      1.05GiB (uncompressed)
Total time elapsed: 46 minutes 47 seconds
```

#### drush sql-sync-pipe
This command pipes the entire syncing process. `mysqldump` is passed the following options to make the quickest
and smallest dump as possible: `-CceKq --single-transaction`. It then pipes the dump via `gzip`, passes through the
ssh tunnel, `gunzip` the pipe and then imports directly into the destination's database.

NOTE: The total time elapsed includes the intial time it takes for the `mysqldump` to initialize the database for
transfer.
```
Command:                 drush sql-sync-pipe @alias.dev @alias.sandbox --progress
Transfer size:           88.1MiB (sent compressed using gzip)
Import size:             1.05GiB
Import & transfer time:  27 minutes 05 seconds
Total time elapsed:      30 minutes and 35 seconds
```

#### drush sql-sync-pipe --dump
If you need to save the dump or simply wish to give an ETA estimate when using the `--progress` option, specify the
`--dump` option. It is similiar to using sql-sync, however there are a few distinct differences. It will still pass
the same options to `mysqldump` as above. Where it differs, is that it will `gzip` the dump and then pass it via the
ssh tunnel so it saves to the destination's HDD (local machine) instead of the source's (server). After the dump has
been saved, it will uncompress the dump using `gunzip` and then pipe it to mysql for import.

NOTE: If the destination alias has a specificed dump file, that will be used and remain on the destination's HDD. If
the destination does not have a dump file specified or the `--temp` option is used, a temporary file will be created
and then removed afterwards.

NOTE: The total time elapsed includes the intial time it takes for the `mysqldump` to initialize the database for
transfer.
```
Command:            drush sql-sync-pipe @alias.dev @alias.sandbox --progress --dump --temp
Transfer Size:      88.1MiB (compressed using gzip)
Transfer Time:      5 minutes 14 seconds (includes time taken to gunzip)
Import Size:        1.05GiB
Import Time:        29 minutes 23 seconds
Total Time Elapsed: 37 minutes and 36 seconds (includes time taken before initial transfer started)
```

## drush sql-sync-pipe --progress
If you have ![Pipe Viewer](http://www.ivarch.com/programs/pv.shtml) installed, you can pass the `--progress` option to show syncing progress between the source and destination.
Due to how the default functionality of this command pipes the entire syncing process, an ETA will not be known and
only generic information will be shown (such as data process and time elapsed). If you pass the `--dump` option, the
initial dump transfer ETA will not be known, but the import process ETA will be shown.

Below are some common commands if you wish to install the ![Pipe Viewer](http://www.ivarch.com/programs/pv.shtml) command (see link for other options):
* For UNIX: `yum install pv` or `apt-get install pv`
* For Mac: `port install pv` or `brew install pv`

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
