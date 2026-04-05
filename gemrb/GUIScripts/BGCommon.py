# SPDX-FileCopyrightText: 2010 Contributors to the GemRB project <https://gemrb.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

import GemRB
import CommonTables
import GameCheck
from ie_stats import IE_ALIGNMENT, IE_RACE, IE_REPUTATION, IE_SEX

def SetReputation ():
	MyChar = GemRB.GetVar ("Slot")

	# If Rep > 0 then MyChar is an imported character with a saved reputation,
	# in which case the reputation shouldn't be overwritten (bg1, but not bg2).
	Rep = GemRB.GetPlayerStat (MyChar, IE_REPUTATION)

	if Rep <= 0 or not GameCheck.IsBG1 ():
		# use the alignment to apply starting reputation
		Alignment = GemRB.GetPlayerStat (MyChar, IE_ALIGNMENT)
		AlignmentAbbrev = CommonTables.Aligns.FindValue (3, Alignment)
		RepTable = GemRB.LoadTable ("repstart")
		Rep = RepTable.GetValue (AlignmentAbbrev, 0) * 10
		GemRB.SetPlayerStat (MyChar, IE_REPUTATION, Rep)

	# set the party rep if this is the main char
	if MyChar == 1:
		GemRB.GameSetReputation (Rep)
