# -*-python-*-
# vim: set ts=4 sw=4 expandtab:
# GemRB - Infinity Engine Emulator
# Copyright (C) 2010 The GemRB Project
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
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#

import GemRB
from ie_restype import *
from AutodetectCommon import CheckFiles

files = (
    ("START", "CHU", RES_CHU),
    ("STARTPOS", "2DA", RES_2DA),
    ("STARTARE", "2DA", RES_2DA),

    ("EXPTABLE", "2DA", RES_2DA),
    ("MUSIC", "2DA", RES_2DA),
)

files_how = (
    ("CHMB1A1", "BAM", RES_BAM),
    ("TRACKING", "2DA", RES_2DA),
)

if CheckFiles(files):
    if CheckFiles(files_how):
        GemRB.AddGameTypeHint ("how", 95)
    else:
        GemRB.AddGameTypeHint ("iwd", 90)


