# SPDX-FileCopyrightText: 2010 Contributors to the GemRB project <https://gemrb.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

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
    for name, ext, ftype in files:
        res = (name + '.' + ext).upper() in fdict or GemRB.HasResource (name, ftype, True)
        if not res:
            return False

    return True

