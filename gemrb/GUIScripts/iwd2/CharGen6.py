# SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

#character generation (GUICG 0)
import GemRB
from GUIDefines import *
from ie_stats import *
import CharOverview

def OnLoad():
	#setting the stats so the feat code will work
	MyChar = GemRB.GetVar("Slot")
	TmpTable = GemRB.LoadTable ("ability")
	AbilityCount = TmpTable.GetRowCount ()
	for i in range (AbilityCount):
		StatID = TmpTable.GetValue (i, 3, GTV_STAT)
		GemRB.SetPlayerStat (MyChar, StatID, GemRB.GetVar ("Ability "+str(i) ) )

	CharOverview.UpdateOverview(6)
	return
