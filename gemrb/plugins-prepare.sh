#!/bin/sh
#copy all plugins
(cd plugins; ln -s */.libs/lib*.so .)
#remove the cuckoo's egg
rm plugins/libcore.so
