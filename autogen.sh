#!/bin/sh

#export CXXFLAGS="-ggdb -I/usr/local/include"
#export LDFLAGS="-ggdb -L/usr/local/lib"

if [ "$1" = "" ]; then
  dest=$HOME/GemRB
else
  dest=$1
  shift
fi

echo Running libtoolize
libtoolize --force || exit 1

echo Running aclocal
aclocal || exit 1

echo Running autoconf
autoconf || exit 1

echo Running autoheader
autoheader || exit 1

echo Running automake
automake --add-missing || exit 1


echo Running configure

cmd="sh configure --prefix=$dest/ --bindir=$dest/ --sysconfdir=$dest/ --datadir=$dest/ --libdir=$dest/plugins/ --disable-subdirs"
$cmd

echo
echo "Configure was invoked as: $cmd"
