# SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

#character generation, import  (GUICG24)
import GemRB
import CharGenCommon

#import from a game 
ImportWindow = 0

def OnLoad():
	global ImportWindow

	ImportWindow = GemRB.LoadWindow(20, "GUICG")
	CharGenCommon.PositionCharGenWin(ImportWindow)

	TextAreaControl = ImportWindow.GetControl(4)
	TextAreaControl.SetText(53774)

	TextAreaControl = ImportWindow.GetControl(2)
#Fill TextArea Control with character sheets, make textarea a listbox

	DoneButton = ImportWindow.GetControl(0)
	DoneButton.SetText(2610)
	DoneButton.SetDisabled(True)
	DoneButton.MakeDefault()

	CancelButton = ImportWindow.GetControl(1)
	CancelButton.SetText(15416)
	CancelButton.MakeEscape()

	DoneButton.OnPress (Done1Press)
	CancelButton.OnPress (CancelPress)
	ImportWindow.Focus()
	return

def Done1Press():
	DoneButton = ImportWindow.GetControl(0)
	DoneButton.SetText(11973)
	DoneButton.SetState(IE_GUI_BUTTON_DISABLED)
	DoneButton.OnPress (Done2Press)
	return
	
def Done2Press():
	if ImportWindow:
		ImportWindow.Close ()
	GemRB.SetNextScript("Start")
	return

def CancelPress():
	if ImportWindow:
		ImportWindow.Close ()
	GemRB.SetNextScript("CharGen")
	return
