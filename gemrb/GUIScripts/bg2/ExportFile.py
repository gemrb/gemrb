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
#character generation, export (GUICG21)
import GemRB

#import from a character sheet
ExportWindow = 0
TextAreaControl = 0

def OnLoad():
	global ExportWindow, TextAreaControl

	GemRB.LoadWindowPack ("GUICG",640,480)
	ExportWindow = GemRB.LoadWindow (21)

	TextAreaControl = ExportWindow.GetControl (4)
	TextAreaControl.SetText (10962)

	TextAreaControl = ExportWindow.GetControl (2)
	TextAreaControl.ListResources (CHR_EXPORTS)
 
	DoneButton = ExportWindow.GetControl (0)
	DoneButton.SetText (2610)
	DoneButton.SetState (IE_GUI_BUTTON_DISABLED)
	DoneButton.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)

	CancelButton = ExportWindow.GetControl (1)
	CancelButton.SetText (15416)
	CancelButton.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)

	DoneButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, DonePress)
	CancelButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, CancelPress)
	TextAreaControl.SetEvent (IE_GUI_TEXTAREA_ON_SELECT, SelectPress)
	ExportWindow.SetVisible (WINDOW_VISIBLE)
	return

def SelectPress ():
	DoneButton = ExportWindow.GetControl (0)
	DoneButton.SetState (IE_GUI_BUTTON_ENABLED)
	return

def DonePress ():
	FileName = TextAreaControl.QueryText ()
	Slot = GemRB.GetVar ("Slot")
	GemRB.SaveCharacter (Slot, FileName)
	if ExportWindow:
		ExportWindow.Unload ()
	GemRB.SetNextScript (GemRB.GetToken("NextScript"))
	return
	
def CancelPress ():
	if ExportWindow:
		ExportWindow.Unload ()
	GemRB.SetNextScript (GemRB.GetToken("NextScript"))
	return 
