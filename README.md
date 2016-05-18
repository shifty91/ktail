# About #

My own tail implementation. Supports -f flag. This tool runs on Linux and FreeBSD.

Ktail tries to use `kqueue` or `inotify` for handling the follow option. If both
mechanisms are not available then polling with `sleep` is used.

# Build #

    $ mkdir build
    $ cd build
    $ cmake ..
    $ make
    $ (sudo make install)

# License #

GPL v3

# Author #

Copyright (C) 2016 Kurt Kanzenbach <kurt@kmk-computers.de>
