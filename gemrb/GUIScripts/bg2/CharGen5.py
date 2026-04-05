# SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

#character generation - alignment; next abilities (GUICG 0)
import GemRB
import CharGenCommon
from ie_stats import IE_STREXTRA

def OnLoad():
	MyChar = GemRB.GetVar ("Slot")
	AbilityTable = GemRB.LoadTable ("ability")
	AbilityCount = AbilityTable.GetRowCount ()
	
	# set all our abilites to zero
	GemRB.SetVar ("Ability -1", 0)
	GemRB.SetVar ("StrExtra", 0)
	GemRB.SetPlayerStat (MyChar, IE_STREXTRA, 0)
	for i in range(AbilityCount):
		GemRB.SetVar ("Ability "+str(i), 0)
		StatID = AbilityTable.GetValue (i, 3)
		GemRB.SetPlayerStat (MyChar, StatID, 0)

	CharGenCommon.DisplayOverview (5)

	return
