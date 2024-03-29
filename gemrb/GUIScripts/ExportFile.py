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
import GameCheck
if GameCheck.IsBG1():
	import GUICommon
	import CharGenCommon

# export to a character file (.chr)
ExportWindow = 0
TextAreaControl = 0

def OnLoad():
	global ExportWindow, TextAreaControl

	ExportWindow = GemRB.LoadWindow (21, "GUICG")

	TextAreaControl = ExportWindow.GetControl (4)
	TextAreaControl.SetText (10962)

	TextAreaControl = ExportWindow.GetControl (2)
	TextAreaControl.ListResources (CHR_EXPORTS)

	FileNameEditBox = ExportWindow.GetControl (7)
	FileNameEditBox.OnChange (FileNameChange)

	DoneButton = ExportWindow.GetControl (0)
	DoneButton.SetText (11973)
	DoneButton.SetState (IE_GUI_BUTTON_DISABLED)
	DoneButton.MakeDefault()

	CancelButton = ExportWindow.GetControl (1)
	CancelButton.SetText (13727)
	CancelButton.MakeEscape()

	DoneButton.OnPress (DonePress)
	CancelButton.OnPress (CancelPress)
	TextAreaControl.OnSelect (SelectPress)
	ExportWindow.ShowModal(MODAL_SHADOW_NONE)
	FileNameEditBox.Focus()
	return

def SelectPress ():
	FileNameEditBox = ExportWindow.GetControl (7)
	FileName = TextAreaControl.QueryText ()
	FileNameEditBox.SetText(FileName)
	FileNameChange()
	return

def FileNameChange ():
	DoneButton = ExportWindow.GetControl (0)
	DoneButton.SetState (IE_GUI_BUTTON_ENABLED)
	return

def DonePress ():
	FileNameEditBox = ExportWindow.GetControl (7)
	FileName = FileNameEditBox.QueryText ()
	Slot = GemRB.GetVar ("Slot")
	GemRB.SaveCharacter (Slot, FileName)

	ExportWindow.Close()

	if GameCheck.IsBG1():
		CharGenCommon.close()
		GemRB.SetNextScript ("Start")
	else:
		GemRB.SetNextScript (GemRB.GetToken("NextScript"))
	return

def CancelPress ():
	ExportWindow.Close ()

	if GameCheck.IsBG1():
		CharGenCommon.jumpTo ("accept")
	else:
		GemRB.SetNextScript (GemRB.GetToken("NextScript"))
	return
