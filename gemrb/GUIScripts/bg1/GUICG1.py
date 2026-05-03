# SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

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

	MaleButton = GenderWindow.GetControl(2)
	MaleButton.SetFlags(IE_GUI_BUTTON_RADIOBUTTON,OP_OR)

	FemaleButton = GenderWindow.GetControl(3)
	FemaleButton.SetFlags(IE_GUI_BUTTON_RADIOBUTTON,OP_OR)
	
	GemRB.SetVar("Gender",0)
	MaleButton.SetVarAssoc("Gender",1)
	FemaleButton.SetVarAssoc("Gender",2)
	MaleButton.OnPress (ClickedMale)
	FemaleButton.OnPress (ClickedFemale)
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
	Gender = GemRB.GetVar ("Gender")
	GemRB.SetPlayerStat (MyChar, IE_SEX, Gender)
	CharGenCommon.next()
	return
