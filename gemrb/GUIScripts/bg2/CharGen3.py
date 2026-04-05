# SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

# character generation - race; next class/kit (CharGen3)
import GemRB
import CharGenCommon
from ie_stats import *

def OnLoad():
	MyChar = GemRB.GetVar ("Slot")
	GemRB.SetVar("Class",0) #class
	GemRB.SetVar("Class Kit",0) #class kit
	GemRB.SetPlayerStat (MyChar, IE_CLASS, 0)
	GemRB.SetPlayerStat (MyChar, IE_KIT, 0)

	#reset all the levels (assigned in CharGen4)
	GemRB.SetPlayerStat (MyChar, IE_LEVEL, 0)
	GemRB.SetPlayerStat (MyChar, IE_LEVEL2, 0)
	GemRB.SetPlayerStat (MyChar, IE_LEVEL3, 0)

	CharGenCommon.DisplayOverview (3)

	return
