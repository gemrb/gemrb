# SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

#character generation, import (GUICG20)
import GemRB

import CharOverview
from GUIDefines import *

#import from a character sheet
MainWindow = 0
PortraitButton = 0
ImportWindow = 0
TextAreaControl = 0
DoneButton = 0

def OnLoad():
	global MainWindow, PortraitButton
	global ImportWindow, TextAreaControl, DoneButton

	MainWindow = GemRB.LoadWindow(0, "GUICG")

	PortraitButton = MainWindow.GetControl (12)
	PortraitButton.SetFlags(IE_GUI_BUTTON_PICTURE|IE_GUI_BUTTON_NO_IMAGE,OP_SET)

	ImportWindow = GemRB.LoadWindow(20, "GUICG")
	CharOverview.PositionCharGenWin (ImportWindow)

	TextAreaControl = ImportWindow.GetControl(4)
	TextAreaControl.SetText(10963)

	TextAreaControl = ImportWindow.GetControl(2)
	TextAreaControl.ListResources(CHR_EXPORTS)

	DoneButton = ImportWindow.GetControl(0)
	DoneButton.SetText(36789)
	DoneButton.SetState(IE_GUI_BUTTON_DISABLED)

	CancelButton = ImportWindow.GetControl(1)
	CancelButton.SetText(15416)
	CancelButton.MakeEscape()
	
	# disable the three extraneous buttons in the bottom row
	for i in [16, 13, 15]:
		TmpButton = MainWindow.GetControl(i)
		TmpButton.SetState(IE_GUI_BUTTON_DISABLED)

	DoneButton.OnPress (DonePress)
	CancelButton.OnPress (CancelPress)
	TextAreaControl.OnSelect (SelectFile)
	ImportWindow.Focus()
	return

def DonePress():
	if ImportWindow:
		ImportWindow.Close ()
	if MainWindow:
		MainWindow.Close ()
	#this part is fuzzy
	#we don't have the character as an object in the chargen
	#but we just imported a complete object
	#either we take the important stats and destroy the object
	#or start with an object from the beginning
	#or use a different script here???
	GemRB.SetVar ("ImportedChar", 1)
	GemRB.SetNextScript("CharGen7")
	return
	
def CancelPress():
	if ImportWindow:
		ImportWindow.Close ()
	if MainWindow:
		MainWindow.Close ()
	GemRB.SetNextScript(GemRB.GetToken("NextScript"))
	return

def SelectFile():
	FileName = TextAreaControl.QueryText()
	Slot = GemRB.GetVar("Slot")
	GemRB.CreatePlayer(FileName, Slot| 0x8000, 1)
	GemRB.SetToken ("CHARNAME", GemRB.GetPlayerName (Slot))
	Portrait = GemRB.GetPlayerPortrait (Slot, 0)
	GemRB.SetToken ("SmallPortrait", GemRB.GetPlayerPortrait (Slot, 1)["ResRef"])
	GemRB.SetToken ("LargePortrait", Portrait["ResRef"])

	PortraitButton.SetPicture (Portrait["Sprite"], "NOPORTLG")
	ImportWindow.Focus() #bring it to the front
	DoneButton.SetState(IE_GUI_BUTTON_ENABLED)
	return
