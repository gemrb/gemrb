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


