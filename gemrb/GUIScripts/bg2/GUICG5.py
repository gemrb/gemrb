# SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

#character generation, name (GUICG5)
import GemRB

NameWindow = 0
NameField = 0
DoneButton = 0

def OnLoad():
	global NameWindow, NameField, DoneButton
	
	NameWindow = GemRB.LoadWindow(5, "GUICG")

	BackButton = NameWindow.GetControl(3)
	BackButton.SetText(15416)
	BackButton.OnPress (NameWindow.Close)

	DoneButton = NameWindow.GetControl(0)
	DoneButton.SetText(11973)
	DoneButton.MakeDefault()
	DoneButton.SetDisabled(True)

	NameField = NameWindow.GetControl(2)
	NameField.SetText (GemRB.GetToken ("CHARNAME"))
	EditChange ()

	DoneButton.OnPress (NextPress)
	NameField.OnChange (EditChange)

	NameWindow.ShowModal (MODAL_SHADOW_GRAY)
	NameField.Focus()
	return

def NextPress():
	Name = NameField.QueryText()
	#check length?
	#seems like a good idea to store it here for the time being
	GemRB.SetToken("CHARNAME",Name) 
	if NameWindow:
		NameWindow.Close ()
	GemRB.SetNextScript("CharGen9")
	return

def EditChange():
	Name = NameField.QueryText()
	if len(Name)==0:
		DoneButton.SetDisabled(True)
	else:
		DoneButton.SetDisabled(False)
	return
