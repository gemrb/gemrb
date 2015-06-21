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
#character generation, import (GUICG24)
import GemRB

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
	DoneButton.SetText(2610)
	DoneButton.SetState(IE_GUI_BUTTON_DISABLED)
	DoneButton.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)

	CancelButton = ImportWindow.GetControl(1)
	CancelButton.SetText(15416)
	CancelButton.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)

	DoneButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, DonePress)
	CancelButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, CancelPress)
	TextAreaControl.SetEvent(IE_GUI_TEXTAREA_ON_SELECT, SelectPress)
	ImportWindow.SetVisible(WINDOW_VISIBLE)
	return

def SelectPress():
	DoneButton = ImportWindow.GetControl(0)
	DoneButton.SetState(IE_GUI_BUTTON_ENABLED)
	return

def DonePress():
	FileName = TextAreaControl.QueryText()
	Slot = GemRB.GetVar("Slot")
	GemRB.CreatePlayer(FileName, Slot| 0x8000, 1, 11) # 11 = force bg2
	if ImportWindow:
		ImportWindow.Unload()
	# the medium portrait isn't available, so we copy the original hack
	MediumPortrait = GemRB.GetPlayerPortrait (Slot, 1)[0:-1] + "M"
	GemRB.SetToken("SmallPortrait", GemRB.GetPlayerPortrait (Slot, 1) )
	GemRB.SetToken("LargePortrait", MediumPortrait )
	GemRB.SetNextScript("CharGen7")
	return
	
def CancelPress():
	if ImportWindow:
		ImportWindow.Unload()
	GemRB.SetNextScript("CharGen")
	return 
