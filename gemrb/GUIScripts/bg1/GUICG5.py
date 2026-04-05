# SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

#character generation, name (GUICG5)
import GemRB
from GUIDefines import *

import CharGenCommon
import GUICommon


NameWindow = 0
NameField = 0
DoneButton = 0

def OnLoad():
	global NameWindow, NameField, DoneButton
	
	NameWindow = GemRB.LoadWindow(5, "GUICG")

	BackButton = NameWindow.GetControl(3)
	BackButton.SetText(15416)

	DoneButton = NameWindow.GetControl(0)
	DoneButton.SetText(11973)
	DoneButton.MakeDefault()
	DoneButton.SetState(IE_GUI_BUTTON_DISABLED)

	NameField = NameWindow.GetControl(2)
	NameField.SetText (GemRB.GetToken ("CHARNAME"))
	EditChange ()

	DoneButton.OnPress (NextPress)
	BackButton.OnPress (lambda: CharGenCommon.back(NameWindow))
	NameField.OnChange (EditChange)
	NameWindow.ShowModal(MODAL_SHADOW_GRAY)
	NameField.Focus()
	return

def NextPress():
	NameWindow.Close()
	Name = NameField.QueryText()
	#check length?
	#seems like a good idea to store it here for the time being
	MyChar = GemRB.GetVar ("Slot")
	GemRB.SetPlayerName (MyChar, Name, 0)
	GemRB.SetToken ("CHARNAME", Name)
	CharGenCommon.next()
	return

def EditChange():
	Name = NameField.QueryText()
	if len(Name)==0:
		DoneButton.SetState(IE_GUI_BUTTON_DISABLED)
	else:
		DoneButton.SetState(IE_GUI_BUTTON_ENABLED)
	return
