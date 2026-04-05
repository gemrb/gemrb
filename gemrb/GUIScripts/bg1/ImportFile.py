# SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

#character generation, import (GUICG20)
import GemRB
from GUIDefines import *
import GUICommon
import CharGenCommon

#import from a character sheet
ImportWindow = 0
TextAreaControl = 0

def OnLoad():
	global ImportWindow, TextAreaControl

	ImportWindow = GemRB.LoadWindow(20, "GUICG")

	TextAreaControl = ImportWindow.GetControl(4)
	TextAreaControl.SetText(10963)

	TextAreaControl = ImportWindow.GetControl(2)
	TextAreaControl.ListResources(CHR_EXPORTS)

	DoneButton = ImportWindow.GetControl(0)
	DoneButton.SetText (11973)

	DoneButton.SetState(IE_GUI_BUTTON_DISABLED)

	CancelButton = ImportWindow.GetControl(1)
	CancelButton.SetText (13727)

	DoneButton.OnPress (DonePress)
	CancelButton.OnPress (CancelPress)
	TextAreaControl.OnSelect (SelectPress)
	ImportWindow.ShowModal(MODAL_SHADOW_GRAY)
	return

def SelectPress():
	DoneButton = ImportWindow.GetControl(0)
	DoneButton.SetState(IE_GUI_BUTTON_ENABLED)
	return

def DonePress():
	ImportWindow.Close()
	FileName = TextAreaControl.QueryText()
	Slot = GemRB.GetVar("Slot")
	GemRB.CreatePlayer(FileName, Slot| 0x8000, 1)

	GemRB.SetToken ("CHARNAME", GemRB.GetPlayerName (Slot))
	GemRB.SetToken ("SmallPortrait", GemRB.GetPlayerPortrait (Slot, 1)["ResRef"])
	GemRB.SetToken ("LargePortrait", GemRB.GetPlayerPortrait (Slot, 0)["ResRef"])

	GemRB.SetVar ("ImportedChar", 1)
	CharGenCommon.jumpTo("appearance")
	return

def CancelPress():
	ImportWindow.Close()
	GemRB.SetNextScript(GemRB.GetToken("NextScript"))
	return
