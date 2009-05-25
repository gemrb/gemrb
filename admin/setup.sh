#!/bin/bash
# GemRB - Infinity Engine Emulator
# Copyright (C) 2003 The GemRB Project
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
#
# $Id$
#
# This is a quick and dirty script to pass reasonable values to the
# configure script. This script compensates for the fact that GemRB
# expects certain files to be in paths relative to GemRB.

if [ "$1" = "" ]; then
	echo "Usage: $0 [installation directory]";
	echo "Example: $0 $HOME/GemRB"
	exit 1
else
	cmd="sh configure --prefix=$1/ --bindir=$1/ --sysconfdir=$1/ --datadir=$1/ --libdir=$1/plugins/ --disable-subdirs"
	$cmd
	echo
	echo "Configure was invoked as: $cmd"
fi
