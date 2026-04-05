# SPDX-FileCopyrightText: 2021 Contributors to the GemRB project <https://gemrb.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

import GemRB
from ie_restype import *
from AutodetectCommon import CheckFiles

files = (
	("itinfwin", "png", RES_PNG),
	("mapwinbg", "png", RES_PNG),
	("worldmap", "png", RES_PNG),
	("riddler", "dlg", RES_DLG)
)

if CheckFiles (files):
	GemRB.AddGameTypeHint ("demo", 110)
