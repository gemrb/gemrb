# SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

#character generation, proficiencies (GUICG9)
import GemRB
import GUICommon
from GUIDefines import *
from ie_stats import *
import CharGenCommon
import LUProfsSelection

SkillWindow = 0
DoneButton = 0


def RedrawSkills():
	ProfsPointsLeft = GemRB.GetVar ("ProfsPointsLeft")
	if not ProfsPointsLeft:
		DoneButton.SetState(IE_GUI_BUTTON_ENABLED)
	else:
		DoneButton.SetState(IE_GUI_BUTTON_DISABLED)
	return

def OnLoad():
	global SkillWindow, DoneButton
	
	SkillWindow = GemRB.LoadWindow(9, "GUICG")

	MyChar = GemRB.GetVar ("Slot")
	Levels = [GemRB.GetPlayerStat (MyChar, IE_LEVEL), GemRB.GetPlayerStat (MyChar, IE_LEVEL2), \
			GemRB.GetPlayerStat (MyChar, IE_LEVEL3)]
	LUProfsSelection.SetupProfsWindow (MyChar, LUProfsSelection.LUPROFS_TYPE_CHARGEN, SkillWindow, RedrawSkills, [0,0,0], Levels,scroll=False,profTableOffset=0)

	BackButton = SkillWindow.GetControl(77)
	BackButton.SetText(15416)
	BackButton.MakeEscape()
	BackButton.OnPress (lambda: CharGenCommon.back(SkillWindow))

	DoneButton = SkillWindow.GetControl(0)
	DoneButton.SetText(11973)
	DoneButton.MakeDefault()
	DoneButton.OnPress (NextPress)
	DoneButton.SetState(IE_GUI_BUTTON_DISABLED)

	SkillWindow.ShowModal(MODAL_SHADOW_GRAY)
	return

def NextPress():
	SkillWindow.Close()
	MyChar = GemRB.GetVar ("Slot")
	LUProfsSelection.ProfsSave(MyChar, LUProfsSelection.LUPROFS_TYPE_CHARGEN)
	CharGenCommon.next()
