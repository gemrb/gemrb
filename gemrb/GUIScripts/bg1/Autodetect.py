# -*-python-*-
# vim: set ts=4 sw=4 expandtab:

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

