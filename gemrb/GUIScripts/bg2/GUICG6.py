# GemRB - Infinity Engine Emulator
# Copyright (C) 2003 The GemRB Project
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
#
#
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
			SkillWindow.Unload ()
		GemRB.SetNextScript("GUICG9")
		return
	
	BackButton = SkillWindow.GetControl(25)
	BackButton.SetText(15416)
	BackButton.MakeEscape()
	BackButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, BackPress)

	DoneButton = SkillWindow.GetControl(0)
	DoneButton.SetText(11973)
	DoneButton.MakeDefault()
	DoneButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, NextPress)
	DoneButton.SetDisabled(True)

	RedrawSkills ()
	SkillWindow.Focus()
	return

def BackPress():
	if SkillWindow:
		SkillWindow.Unload()
	GemRB.SetNextScript("CharGen6")
	return

def NextPress():
	if SkillWindow:
		SkillWindow.Unload()
	# save all skills

	LUSkillsSelection.SkillsSave (MyChar)

	GemRB.SetNextScript("GUICG9") #weapon proficiencies
	return
