# -*-python-*-
# vim: set ts=4 sw=4 expandtab:

import GemRB
from ie_restype import *
from AutodetectCommon import CheckFiles

files = (
    ("START", "CHU", RES_CHU),
    ("STARTPOS", "2DA", RES_2DA),
    ("STARTARE", "2DA", RES_2DA),

    ("VAR", "VAR", RES_VAR),
    ("RESDATA", "INI", RES_INI),
    ("BEAST", "INI", RES_INI),
)


if CheckFiles(files):
    GemRB.AddGameTypeHint ("pst", 100)
