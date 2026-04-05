# SPDX-FileCopyrightText: 2021 Contributors to the GemRB project <https://gemrb.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

import GemRB
from ie_restype import *
from AutodetectCommon import CheckFiles

# NB: python insists on at least two entries
files = (
	("PALETTE0", "PNG", RES_PNG),
	("PALETTE0", "PNG", RES_PNG)
)

if CheckFiles (files):
	GemRB.AddGameTypeHint ("test", 110)
