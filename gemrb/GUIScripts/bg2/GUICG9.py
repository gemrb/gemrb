# SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

#character generation, proficiencies (GUICG9)
import GemRB
from GUIDefines import *
from ie_stats import *
import LUProfsSelection

SkillWindow = 0
DoneButton = 0
MyChar = 0

def RedrawSkills():
	ProfsPointsLeft = GemRB.GetVar ("ProfsPointsLeft")
	DoneButton.SetDisabled(ProfsPointsLeft > 0)
	return

def OnLoad():
	global SkillWindow, DoneButton, MyChar
	
	SkillWindow = GemRB.LoadWindow(9, "GUICG")
	MyChar = GemRB.GetVar ("Slot")
	Levels = [GemRB.GetPlayerStat (MyChar, IE_LEVEL), GemRB.GetPlayerStat (MyChar, IE_LEVEL2), \
			GemRB.GetPlayerStat (MyChar, IE_LEVEL3)]
	LUProfsSelection.SetupProfsWindow (MyChar, \
		LUProfsSelection.LUPROFS_TYPE_CHARGEN, SkillWindow, RedrawSkills, [0,0,0], Levels)

	BackButton = SkillWindow.GetControl(77)
	BackButton.SetText(15416)
	BackButton.MakeEscape()
	BackButton.OnPress (BackPress)

	DoneButton = SkillWindow.GetControl(0)
	DoneButton.SetText(11973)
	DoneButton.MakeDefault()
	DoneButton.OnPress (NextPress)

	RedrawSkills()
	SkillWindow.Focus()
	return

def BackPress():
	if SkillWindow:
		SkillWindow.Close ()
	GemRB.SetNextScript("CharGen6")
	#scrap skills
	return

def NextPress():
	if SkillWindow:
		SkillWindow.Close ()

	LUProfsSelection.ProfsSave (MyChar, LUProfsSelection.LUPROFS_TYPE_CHARGEN)

	GemRB.SetNextScript("CharGen7") #appearance
	return
