#!/bin/bash
rm configure.in configure.files configure.in.in gemrb.kdevprj 
rm gemrb.kdevses gemrb.lsm linux_compile.sh prepare-config.sh 
rm *.m4 configure
rm stamp-h*
rm libtool
rm subdirs
rm gemrb/plugins-prepare.sh
for i in $(find -name "config.*" -print); do rm $i;done
for i in $(find -name "Makefile*" -print); do rm $i;done
for i in $(find -name "admin" -print); do rm -rf $i;done
