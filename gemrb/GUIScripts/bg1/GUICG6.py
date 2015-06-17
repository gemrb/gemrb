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

	if GUICommon.CloseOtherWindow (OnLoad):
		if(SkillWindow):
			SkillWindow.Unload()
			SkillWindow = None
		return

	GemRB.LoadWindowPack("GUICG", 640, 480)
	SkillWindow = GemRB.LoadWindow(6)

	MyChar = GemRB.GetVar ("Slot")

	Levels = [GemRB.GetPlayerStat (MyChar, IE_LEVEL), \
			GemRB.GetPlayerStat (MyChar, IE_LEVEL2), \
			GemRB.GetPlayerStat (MyChar, IE_LEVEL3)]
	LUSkillsSelection.SetupSkillsWindow (MyChar, LUSkillsSelection.LUSKILLS_TYPE_CHARGEN, SkillWindow, RedrawSkills, [0,0,0], Levels,0,False)
	
	BackButton = SkillWindow.GetControl(25)
	BackButton.SetText(15416)
	BackButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, CharGenCommon.BackPress)

	DoneButton = SkillWindow.GetControl(0)
	DoneButton.SetText(11973)
	DoneButton.SetFlags(IE_GUI_BUTTON_DEFAULT,OP_OR)
	DoneButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, NextPress)
	DoneButton.SetState(IE_GUI_BUTTON_DISABLED)

	RedrawSkills()
	SkillWindow.ShowModal(MODAL_SHADOW_NONE)
	return

def NextPress():
	MyChar = GemRB.GetVar ("Slot")
	LUSkillsSelection.SkillsSave(MyChar)
	CharGenCommon.next()
