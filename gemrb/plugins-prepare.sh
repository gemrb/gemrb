#copy all plugins
cp -f plugins/*/.libs/lib*.so plugins/
#remove the cuckoo's egg
rm plugins/libcore.so
