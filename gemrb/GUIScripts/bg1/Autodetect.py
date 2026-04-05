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
)


if CheckFiles(files):
    GemRB.AddGameTypeHint ("bg1", 80)

