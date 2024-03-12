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
#character generation, race (GUICG2)
import GemRB
from GUIDefines import *
from ie_stats import *
import CharGenCommon
import GUICommon
import CommonTables


RaceWindow = 0
TextAreaControl = 0
DoneButton = 0

def OnLoad():
	global RaceWindow, TextAreaControl, DoneButton
	global RaceTable

	RaceWindow = GemRB.LoadWindow(8, "GUICG")

	RaceCount = CommonTables.Races.GetRowCount()

	GemRB.SetVar("Race", 0)

	for i in range(2, RaceCount+2):
		Button = RaceWindow.GetControl(i)
		Button.SetVarAssoc("Race", i-1)
		Button.SetFlags(IE_GUI_BUTTON_RADIOBUTTON, OP_OR)
		Button.SetState(IE_GUI_BUTTON_ENABLED) # reset from SELECTED after SetVarAssoc
		Button.SetText(CommonTables.Races.GetValue(i-2, 0))
		Button.OnPress(RacePress)

	# restore after SetVarAssoc
	GemRB.SetVar("Race", 0)

	BackButton = RaceWindow.GetControl(10)
	BackButton.SetText(15416)
	DoneButton = RaceWindow.GetControl(0)
	DoneButton.SetText(11973)
	DoneButton.MakeDefault()
	DoneButton.SetState(IE_GUI_BUTTON_DISABLED)

	TextAreaControl = RaceWindow.GetControl(8)
	TextAreaControl.SetText(17237)

	DoneButton.OnPress (NextPress)
	BackButton.OnPress (lambda: CharGenCommon.back(RaceWindow))
	RaceWindow.ShowModal(MODAL_SHADOW_GRAY)
	return

def RacePress():
	Race = GemRB.GetVar("Race")-1
	TextAreaControl.SetText(CommonTables.Races.GetValue(Race,1) )
	DoneButton.SetState(IE_GUI_BUTTON_ENABLED)
	return

def NextPress():
	RaceWindow.Close()
	Race = GemRB.GetVar ("Race") - 1
	MyChar = GemRB.GetVar ("Slot")
	GemRB.SetPlayerStat (MyChar, IE_RACE, CommonTables.Races.GetValue (Race, 3))
	CharGenCommon.next()
