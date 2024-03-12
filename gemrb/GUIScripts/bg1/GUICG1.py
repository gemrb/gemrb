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
#character generation, gender (GUICG1)
import GemRB
from GUIDefines import *
from ie_stats import *

import CharGenCommon
import GUICommon

GenderWindow = 0
TextAreaControl = 0
DoneButton = 0

def OnLoad():
	global GenderWindow, TextAreaControl, DoneButton
		
	GenderWindow = GemRB.LoadWindow(1, "GUICG")

	BackButton = GenderWindow.GetControl(6)
	BackButton.SetText(15416)
	DoneButton = GenderWindow.GetControl(0)
	DoneButton.SetText(11973)
	DoneButton.MakeDefault()

	TextAreaControl = GenderWindow.GetControl(5)
	TextAreaControl.SetText(17236)

	GemRB.SetVar("Gender", 0)

	def SetGenderButton(controlID, var, onPress):
		Button = GenderWindow.GetControl(controlID)
		Button.SetVarAssoc("Gender", var)
		Button.SetFlags(IE_GUI_BUTTON_RADIOBUTTON, OP_OR)
		Button.SetState(IE_GUI_BUTTON_ENABLED) # reset from SELECTED after SetVarAssoc
		Button.OnPress(onPress)

	SetGenderButton(2, 1, ClickedMale)
	SetGenderButton(3, 2, ClickedFemale)

	# restore after SetVarAssoc
	GemRB.SetVar("Gender", 0)

	DoneButton.OnPress (NextPress)
	BackButton.OnPress (lambda: CharGenCommon.back(GenderWindow))
	DoneButton.SetState(IE_GUI_BUTTON_DISABLED)
	GenderWindow.ShowModal(MODAL_SHADOW_GRAY)
	return

def ClickedMale():
	TextAreaControl.SetText(13083)
	DoneButton.SetState(IE_GUI_BUTTON_ENABLED)
	return

def ClickedFemale():
	TextAreaControl.SetText(13084)
	DoneButton.SetState(IE_GUI_BUTTON_ENABLED)
	return

def NextPress():
	GenderWindow.Close()
	MyChar = GemRB.GetVar ("Slot")
	#GemRB.CreatePlayer ("charbase", MyChar | 0x8000 )
	Gender = GemRB.GetVar ("Gender")
	GemRB.SetPlayerStat (MyChar, IE_SEX, Gender)
	CharGenCommon.next()
	return
