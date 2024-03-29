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
