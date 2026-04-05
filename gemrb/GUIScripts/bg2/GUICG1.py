# SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

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
	MaleButton.OnPress (ClickedMale)
	FemaleButton.OnPress (ClickedFemale)
	DoneButton.OnPress (NextPress)
	BackButton.OnPress (BackPress)
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
		GenderWindow.Close ()
	GemRB.SetNextScript("CharGen")
	GemRB.SetVar("Gender",0)  #scrapping the gender value
	return

def NextPress():
	if GenderWindow:
		GenderWindow.Close ()

	Gender = GemRB.GetVar ("Gender")
	GemRB.SetPlayerStat (MyChar, IE_SEX, Gender)
	GemRB.SetNextScript("GUICG12") #appearance
	return
