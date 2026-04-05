# SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

#character generation (GUICG 0)
import GemRB
import CharGenCommon
import GUICommonWindows

def OnLoad():
	GemRB.SetVar("Gender",0) #gender
	GemRB.SetVar("Race",0) #race
	GemRB.SetVar("Class",0) #class
	GemRB.SetVar("Class Kit",0) #class
	GemRB.SetVar("Alignment", None) #alignment

	GUICommonWindows.PortraitWindow = None

	MyChar = GemRB.GetVar ("Slot")
	GemRB.CreatePlayer ("charbase", MyChar | 0x8000, 0, 11 ) # 11 = force bg2
	GemRB.SetVar ("ImportedChar", 0)
	CharGenCommon.DisplayOverview (1)

	return
