# SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

#character generation (GUICG 0)
import CharOverview
from ie_stats import IE_SEX

def OnLoad():
	MyChar = GemRB.GetVar ("Slot")
	GemRB.SetPlayerStat (MyChar, IE_SEX, GemRB.GetVar ("Gender"))
	CharOverview.UpdateOverview(2)
	return
