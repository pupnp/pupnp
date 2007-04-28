#!/bin/sh

rm -fr autom4te.cache

# Equivalent to 
#	aclocal 
#	autoheader 
#	automake --add-missing --copy
#	autoconf

autoreconf --force --install -Wall -Wno-obsolete $* || exit 1

echo "Now run ./configure and then make."
exit 0

