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
    ("CHMB1A1", "BAM", RES_BAM),
    ("TRACKING", "2DA", RES_2DA),

    ("SUBRACES", "2DA", RES_2DA),
)


if CheckFiles(files):
    GemRB.AddGameTypeHint ("iwd2", 100)

