#!/bin/bash

set -x
aclocal
autoheader
automake --foreign --add-missing --copy
autoconf
./configure --enable-debug=yes
make
make install