#!/bin/sh
set -x
gnome-doc-prepare --force --copy || exit 1
aclocal -I m4 || exit 1
libtoolize --force
# autoheader
autoconf || exit 1
automake -a || exit 1
echo "./autogen.sh $@" > autoregen.sh
chmod +x autoregen.sh
./configure $@
