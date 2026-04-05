# SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

#character generation (GUICG 0)
import GemRB
from ie_stats import *
import CharOverview

def OnLoad():
	MyChar = GemRB.GetVar("Slot")
	GemRB.SetPlayerStat (MyChar, IE_SEX, GemRB.GetVar ("Gender") )
	GemRB.SetPlayerStat (MyChar, IE_RACE, GemRB.GetVar ("BaseRace") )
	race = GemRB.GetVar ("Race")
	if race == GemRB.GetVar ("BaseRace"):
		race = 0 # no subrace
	GemRB.SetPlayerStat (MyChar, IE_SUBRACE, race & 255 )
	CharOverview.UpdateOverview(3)
	return
