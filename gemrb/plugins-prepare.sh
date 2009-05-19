#!/bin/sh
#copy all plugins
dir=$1
if test -z "$dir"; then
  cd `dirname $0`/plugins
else
  cd "$dir"
fi

if test -d Core/.libs; then
  ln -sf */.libs/lib*.so .
else
  # cmake; expect to be in the build dir since it is arbitrary
  if test -z "$dir"; then
    echo missing dir parameter - pass the path to the build dir
    exit 1
  fi
  cd gemrb/plugins
  ln -sf */lib*.so .
fi

#remove the cuckoo's egg
rm libgemrb_core.so
