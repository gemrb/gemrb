#!/bin/bash

#This is a quick and dirty script to pass reasonable values to the 
#configure script. This script compensates for the fact that GemRB 
#expects certain files to be in paths relative to GemRB.

if [ "$1" == "" ]; then
	echo "Usage: $0 [installation directory]";
	echo "Example: $0 $HOME/GemRB"
	exit 1
else
	sh configure --bindir=$1/ --datadir=$1/ --libdir=$1/plugins 
	echo "Configure was invoked as: sh configure --bindir=$1/ --datadir=$1 --libdir=$1/plugins"
fi
