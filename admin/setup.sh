#!/bin/bash

#This is a quick and dirty script to pass reasonable values to the 
#configure script. This script compensates for the fact that GemRB 
#expects certain files to be in paths relative to GemRB.

if [ "$1" = "" ]; then
	echo "Usage: $0 [installation directory]";
	echo "Example: $0 $HOME/GemRB"
	exit 1
else
	cmd="sh configure --prefix=$1/ --bindir=$1/ --datadir=$1/ --libdir=$1/plugins"
	$cmd
	echo
	echo "Configure was invoked as: $cmd"
fi
