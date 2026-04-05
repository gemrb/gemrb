# SPDX-FileCopyrightText: 2010 Contributors to the GemRB project <https://gemrb.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

import GemRB
from ie_restype import *
from AutodetectCommon import CheckFiles

files = (
    ("START", "CHU", RES_CHU),
    ("STARTPOS", "2DA", RES_2DA),
    ("STARTARE", "2DA", RES_2DA),

    ("KITLIST", "2DA", RES_2DA),
    ("SONGLIST", "2DA", RES_2DA),
)

# using a dud resource type, since we rely on the manual GamePath searching
# in AutodetectCommon
demo_files = (
    ("benevent", "TTF", 1),
    ("feine22", "TTF", 1),
)

GemRB.BG2Demo = False
if CheckFiles(files):
    GemRB.AddGameTypeHint ("bg2", 90)
    if CheckFiles(demo_files):
        GemRB.BG2Demo = True
