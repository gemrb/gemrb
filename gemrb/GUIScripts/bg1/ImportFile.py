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

	GemRB.LoadWindowPack("GUICG", 640, 480)
	ImportWindow = GemRB.LoadWindow(20)

	GUICommon.CloseOtherWindow(ImportWindow.Unload)

	TextAreaControl = ImportWindow.GetControl(4)
	TextAreaControl.SetText(10963)

	TextAreaControl = ImportWindow.GetControl(2)
	TextAreaControl.GetCharacters()

	DoneButton = ImportWindow.GetControl(0)
	DoneButton.SetText (11973)

	DoneButton.SetState(IE_GUI_BUTTON_DISABLED)

	CancelButton = ImportWindow.GetControl(1)
	CancelButton.SetText (13727)

	DoneButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, DonePress)
	CancelButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, CancelPress)
	TextAreaControl.SetEvent(IE_GUI_TEXTAREA_ON_CHANGE, SelectPress)
	ImportWindow.ShowModal(MODAL_SHADOW_NONE)
	return

def SelectPress():
	DoneButton = ImportWindow.GetControl(0)
	DoneButton.SetState(IE_GUI_BUTTON_ENABLED)
	return

def DonePress():
	FileName = TextAreaControl.QueryText()
	Slot = GemRB.GetVar("Slot")
	GemRB.CreatePlayer(FileName, Slot| 0x8000, 1)
	GemRB.SetToken("SmallPortrait", GemRB.GetPlayerPortrait (Slot, 1) )
	GemRB.SetToken("LargePortrait", GemRB.GetPlayerPortrait (Slot, 0) )
	CharGenCommon.jumpTo("appearance")
	return

def CancelPress():
	GUICommon.CloseOtherWindow(None)
	GemRB.SetNextScript(GemRB.GetToken("NextScript"))
	return
