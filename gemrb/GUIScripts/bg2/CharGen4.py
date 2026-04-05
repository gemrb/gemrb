# SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

# character generation - classes+kits; next alignment/reputation(CharGen4.py)
import GemRB
import CharGenCommon
from ie_stats import IE_ALIGNMENT, IE_REPUTATION

def OnLoad():
	MyChar = GemRB.GetVar ("Slot")
	GemRB.SetVar ("Alignment", None) #alignment
	GemRB.SetPlayerStat (MyChar, IE_ALIGNMENT, 0)
	GemRB.SetPlayerStat (MyChar, IE_REPUTATION, 0)

	CharGenCommon.DisplayOverview (4)

	return
