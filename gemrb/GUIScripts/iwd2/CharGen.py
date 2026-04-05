# SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

#character generation (GUICG 0)
import GemRB
import CharOverview

def OnLoad():
	MyChar = GemRB.GetVar ("Slot")
	GemRB.CreatePlayer ("charbase", MyChar | 0x8000 )

	GemRB.GetView ("STARTWIN").SetDisabled (True)
	CharOverview.UpdateOverview(1)
	return

