#!/bin/sh
#copy all plugins
cd `dirname $0`/plugins
ln -sf */.libs/lib*.so .

#remove the cuckoo's egg
rm libgemrb_core.so
