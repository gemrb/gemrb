# -*-python-*-
# vim: set ts=4 sw=4 expandtab:

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


