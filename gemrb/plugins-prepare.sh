#!/bin/sh
#copy all plugins
(cd plugins; ln -sf */.libs/lib*.so .)
#remove the cuckoo's egg
rm plugins/libcore.so
