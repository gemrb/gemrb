#!/bin/bash
# GemRB - Infinity Engine Emulator
# Copyright (C) 2009 The GemRB Project
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
#
# only works for autotools builds
# see gemrb/plugins-prepare.sh

if [[ ! -d gemrb/plugins ]]; then
  echo You must call me from the top gemrb dir!
  exit 1
fi

if [[ $1 == on ]]; then
  ln -sf "$PWD"/gemrb/plugins/OpenALAudio/.libs/libOpenALAudio.so  "$PWD"/gemrb/plugins/libOpenALAudio.so
  rm -f "$PWD"/gemrb/plugins/libNullSound.so
elif [[ $1 == off ]]; then
  ln -sf "$PWD"/gemrb/plugins/NullSound/.libs/libNullSound.so  "$PWD"/gemrb/plugins/libNullSound.so
  rm -f "$PWD"/gemrb/plugins/libOpenALAudio.so
else
  echo Missing or bad parameter!
  echo USAGE: $0 on\|off
  exit 2
fi
