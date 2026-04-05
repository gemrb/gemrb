# SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

#character generation, skills (GUICG6)
import GemRB
from GUIDefines import *
from ie_stats import *
import CharGenCommon
import GUICommon
import LUSkillsSelection


SkillWindow = 0
TextAreaControl = 0
DoneButton = 0

def RedrawSkills():
	global TopIndex
	if GemRB.GetVar ("SkillPointsLeft") == 0:
		DoneButton.SetState(IE_GUI_BUTTON_ENABLED)
	else:
		DoneButton.SetState(IE_GUI_BUTTON_DISABLED)
	return

def OnLoad():
	global SkillWindow, DoneButton

	SkillWindow = GemRB.LoadWindow(6, "GUICG")

	MyChar = GemRB.GetVar ("Slot")

	Levels = [GemRB.GetPlayerStat (MyChar, IE_LEVEL), \
			GemRB.GetPlayerStat (MyChar, IE_LEVEL2), \
			GemRB.GetPlayerStat (MyChar, IE_LEVEL3)]
	LUSkillsSelection.SetupSkillsWindow (MyChar, LUSkillsSelection.LUSKILLS_TYPE_CHARGEN, SkillWindow, RedrawSkills, [0,0,0], Levels,0,False)
	
	BackButton = SkillWindow.GetControl(25)
	BackButton.SetText(15416)
	BackButton.OnPress (lambda: CharGenCommon.back(SkillWindow))

	DoneButton = SkillWindow.GetControl(0)
	DoneButton.SetText(11973)
	DoneButton.MakeDefault()
	DoneButton.OnPress (NextPress)
	DoneButton.SetState(IE_GUI_BUTTON_DISABLED)

	RedrawSkills()
	SkillWindow.ShowModal(MODAL_SHADOW_GRAY)
	return

def NextPress():
	SkillWindow.Close()
	MyChar = GemRB.GetVar ("Slot")
	LUSkillsSelection.SkillsSave(MyChar)
	CharGenCommon.next()
