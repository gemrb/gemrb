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


RaceWindow = 0
TextAreaControl = 0
DoneButton = 0

def OnLoad():
	global RaceWindow, TextAreaControl, DoneButton
	global RaceTable

	if GUICommon.CloseOtherWindow (OnLoad):
		if(RaceWindow):
			RaceWindow.Unload()
			RaceWindow = None
		return
	
	GemRB.SetVar("Race",0) 

	GemRB.LoadWindowPack("GUICG")
	RaceWindow = GemRB.LoadWindow(8)

	RaceCount = GUICommon.RaceTable.GetRowCount()

	for i in range(2,RaceCount+2):
		Button = RaceWindow.GetControl(i)
		Button.SetFlags(IE_GUI_BUTTON_RADIOBUTTON,OP_OR)
	for i in range(2, RaceCount+2):
		Button = RaceWindow.GetControl(i)
		Button.SetText(GUICommon.RaceTable.GetValue(i-2,0) )
		Button.SetState(IE_GUI_BUTTON_ENABLED)
		Button.SetEvent(IE_GUI_BUTTON_ON_PRESS, RacePress)
		Button.SetVarAssoc("Race",i-1)

	BackButton = RaceWindow.GetControl(10)
	BackButton.SetText(15416)
	DoneButton = RaceWindow.GetControl(0)
	DoneButton.SetText(11973)
	DoneButton.SetFlags(IE_GUI_BUTTON_DEFAULT,OP_OR)
	DoneButton.SetState(IE_GUI_BUTTON_DISABLED)

	TextAreaControl = RaceWindow.GetControl(8)
	TextAreaControl.SetText(17237)

	DoneButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, NextPress)
	BackButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, CharGenCommon.BackPress)
	RaceWindow.ShowModal(MODAL_SHADOW_NONE)
	return

def RacePress():
	Race = GemRB.GetVar("Race")-1
	TextAreaControl.SetText(GUICommon.RaceTable.GetValue(Race,1) )
	DoneButton.SetState(IE_GUI_BUTTON_ENABLED)
	return

def NextPress():
	Race = GemRB.GetVar ("Race") - 1
	MyChar = GemRB.GetVar ("Slot")
	GemRB.SetPlayerStat (MyChar, IE_RACE, GUICommon.RaceTable.GetValue (Race, 3))
	CharGenCommon.next()
