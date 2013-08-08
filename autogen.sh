#!/bin/sh

#
# Wheelimote - Generate Build Files
# Written by David Wilkins
# GPL
#

set -e
mkdir -p m4/
autoreconf -i

if test ! "x$NOCONFIGURE" = "x1" ; then
        ./configure --enable-debug $*
fi
