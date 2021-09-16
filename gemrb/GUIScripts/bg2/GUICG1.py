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
from ie_stats import *
from GUIDefines import *

import CharGenCommon

GenderWindow = 0
TextAreaControl = 0
DoneButton = 0
MyChar = 0

def OnLoad():
	global GenderWindow, TextAreaControl, DoneButton, MyChar
	
	MyChar = GemRB.GetVar ("Slot")

	GenderWindow = GemRB.LoadWindow(1, "GUICG")
	GenderWindow.SetFlags(WF_ALPHA_CHANNEL, OP_OR)
	CharGenCommon.PositionCharGenWin(GenderWindow)

	BackButton = GenderWindow.GetControl(6)
	BackButton.SetText(15416)
	BackButton.MakeEscape()
	DoneButton = GenderWindow.GetControl(0)
	DoneButton.SetText(11973)
	DoneButton.MakeDefault()

	TextAreaControl = GenderWindow.GetControl(5)
	TextAreaControl.SetText(17236)

	MaleButton = GenderWindow.GetControl(2)
	MaleButton.SetFlags(IE_GUI_BUTTON_RADIOBUTTON,OP_OR)

	FemaleButton = GenderWindow.GetControl(3)
	FemaleButton.SetFlags(IE_GUI_BUTTON_RADIOBUTTON,OP_OR)

	MaleButton.SetVarAssoc("Gender",1)
	FemaleButton.SetVarAssoc("Gender",2)
	MaleButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, ClickedMale)
	FemaleButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, ClickedFemale)
	DoneButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, NextPress)
	BackButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, BackPress)
	DoneButton.SetDisabled(True)
	GenderWindow.Focus()
	return

def ClickedMale():
	TextAreaControl.SetText(13083)
	DoneButton.SetDisabled(False)
	return

def ClickedFemale():
	TextAreaControl.SetText(13084)
	DoneButton.SetDisabled(False)
	return

def BackPress():
	if GenderWindow:
		GenderWindow.Unload()
	GemRB.SetNextScript("CharGen")
	return

def NextPress():
	Gender = GenderWindow.GetVar ("Gender")
	GemRB.SetPlayerStat (MyChar, IE_SEX, Gender)
	GemRB.SetNextScript("GUICG12") #appearance
	if GenderWindow:
		GenderWindow.Unload()
	return
