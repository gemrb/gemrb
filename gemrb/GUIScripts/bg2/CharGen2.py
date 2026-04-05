# SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

# character generation - gender; next race (CharGen2)
import GemRB
import CharGenCommon
from ie_stats import IE_RACE

def OnLoad():
	MyChar = GemRB.GetVar ("Slot")
	GemRB.SetVar ("Race",0) #race
	GemRB.SetPlayerStat (MyChar, IE_RACE, 0)

	CharGenCommon.DisplayOverview (2)

	return
