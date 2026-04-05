# SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

#character generation, import (GUICG24)
import GemRB
import CharGenCommon

#import from a character sheet
ImportWindow = 0
TextAreaControl = 0

def OnLoad():
	global ImportWindow, TextAreaControl

	ImportWindow = GemRB.LoadWindow(20, "GUICG")
	CharGenCommon.PositionCharGenWin(ImportWindow)

	TextAreaControl = ImportWindow.GetControl(4)
	TextAreaControl.SetText(10963)

	TextAreaControl = ImportWindow.GetControl(2)
	TextAreaControl.ListResources(CHR_EXPORTS)
 
	DoneButton = ImportWindow.GetControl(0)
	DoneButton.SetText(2610)
	DoneButton.SetDisabled(True)
	DoneButton.MakeDefault()

	CancelButton = ImportWindow.GetControl(1)
	CancelButton.SetText(15416)
	CancelButton.MakeEscape()

	DoneButton.OnPress (DonePress)
	CancelButton.OnPress (CancelPress)
	TextAreaControl.OnSelect (SelectPress)
	ImportWindow.Focus()
	return

def SelectPress():
	DoneButton = ImportWindow.GetControl(0)
	DoneButton.SetDisabled(False)
	return

def DonePress():
	FileName = TextAreaControl.QueryText()
	Slot = GemRB.GetVar("Slot")
	GemRB.CreatePlayer(FileName, Slot| 0x8000, 1, 11) # 11 = force bg2
	GemRB.SetToken ("CHARNAME", GemRB.GetPlayerName (Slot))
	if ImportWindow:
		ImportWindow.Close ()
	# the medium portrait isn't available, so we copy the original hack
	SmallPortrait = GemRB.GetPlayerPortrait (Slot, 1)["ResRef"]
	MediumPortrait = SmallPortrait[0:-1] + "M"
	GemRB.SetToken ("SmallPortrait", SmallPortrait)
	GemRB.SetToken ("LargePortrait", MediumPortrait)
	GemRB.SetNextScript("CharGen7")
	GemRB.SetVar ("ImportedChar", 1)
	return
	
def CancelPress():
	if ImportWindow:
		ImportWindow.Close ()
	GemRB.SetNextScript("CharGen")
	return 
