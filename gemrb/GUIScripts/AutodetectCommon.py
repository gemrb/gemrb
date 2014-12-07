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

import os
import GemRB
from GUIDefines import SV_GAMEPATH

fdict = {}

# Create dict of files in GamePath and GamePath/data
GamePath = GemRB.GetSystemVariable (SV_GAMEPATH)
for file in os.listdir (GamePath):
    ufile = file.upper()
    if ufile == 'DATA':
        for file2 in os.listdir (os.path.join(GamePath, file)):
            fdict[file2.upper()] = 1
    else:
        fdict[ufile] = 1


#
# Return True if all given files/resrefs exist
#
# FILES is a list of tuples NAME, EXTENSION, TYPE
#

def CheckFiles(files):
    for name, ext, type in files:
        res = (name+'.'+ext).upper() in fdict or GemRB.HasResource (name, type)
        #print name+'.'+ext, res
        if not res:
            return False

    return True

