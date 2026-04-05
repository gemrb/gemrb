# SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

#character generation, (thief) skills (GUICG6)
import GemRB
import LUSkillsSelection
from GUIDefines import *
from ie_stats import *

import CharGenCommon

SkillWindow = 0
TextAreaControl = 0
DoneButton = 0
MyChar = 0

def RedrawSkills():
	PointsLeft = GemRB.GetVar ("SkillPointsLeft")
	if PointsLeft == 0:
		DoneButton.SetDisabled(False)
	else:
		DoneButton.SetDisabled(True)
	return

def OnLoad():
	global SkillWindow, DoneButton, MyChar
	
	SkillWindow = GemRB.LoadWindow(6, "GUICG")
	CharGenCommon.PositionCharGenWin(SkillWindow)
	
	MyChar = GemRB.GetVar ("Slot")

	Levels = [GemRB.GetPlayerStat (MyChar, IE_LEVEL), \
			GemRB.GetPlayerStat (MyChar, IE_LEVEL2), \
			GemRB.GetPlayerStat (MyChar, IE_LEVEL3)]
	LUSkillsSelection.SetupSkillsWindow (MyChar, \
		LUSkillsSelection.LUSKILLS_TYPE_CHARGEN, SkillWindow, RedrawSkills, [0,0,0], Levels)

	# can't be moved earlier as this var will be set in the setup call above
	if not GemRB.GetVar ("SkillPointsLeft"): #skipping
		if SkillWindow:
			SkillWindow.Close ()
		GemRB.SetNextScript("GUICG9")
		return
	
	BackButton = SkillWindow.GetControl(31 if GameCheck.IsAnyEE () else 25)
	BackButton.SetText(15416)
	BackButton.MakeEscape()
	BackButton.OnPress (BackPress)

	DoneButton = SkillWindow.GetControl(0)
	DoneButton.SetText(11973)
	DoneButton.MakeDefault()
	DoneButton.OnPress (NextPress)
	DoneButton.SetDisabled(True)

	RedrawSkills ()
	SkillWindow.Focus()
	return

def BackPress():
	if SkillWindow:
		SkillWindow.Close ()
	GemRB.SetNextScript("CharGen6")
	return

def NextPress():
	if SkillWindow:
		SkillWindow.Close ()
	# save all skills

	LUSkillsSelection.SkillsSave (MyChar)

	GemRB.SetNextScript("GUICG9") #weapon proficiencies
	return
