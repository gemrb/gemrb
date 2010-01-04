#!/bin/sh
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
# link all plugins to the plugin dir, so gemrb can run from the build dir
dir=$1
if test -z "$dir"; then
  cd `dirname $0`/plugins
else
  cd "$dir"
fi

if test -d Core/.libs; then
  ln -sf */.libs/*.so .
else
  # cmake; expect to be in the build dir since it is arbitrary
  if test -z "$dir"; then
    echo missing dir parameter - pass the path to the build dir
    exit 1
  fi
  cd gemrb/plugins
  ln -sf */*.so .
fi

#remove the cuckoo's egg
rm libgemrb_core.so
