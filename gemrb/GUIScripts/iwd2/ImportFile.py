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

#import from a character sheet
MainWindow = 0
PortraitButton = 0
ImportWindow = 0
TextAreaControl = 0
DoneButton = 0

def OnLoad():
	global MainWindow, PortraitButton
	global ImportWindow, TextAreaControl, DoneButton

	GemRB.LoadWindowPack("GUICG", 800, 600)
	MainWindow = GemRB.LoadWindow(0)
	MainWindow.SetFrame()

	PortraitButton = MainWindow.GetControl (12)
	PortraitButton.SetFlags(IE_GUI_BUTTON_PICTURE|IE_GUI_BUTTON_NO_IMAGE,OP_SET)

	ImportWindow = GemRB.LoadWindow(20)

	TextAreaControl = ImportWindow.GetControl(4)
	TextAreaControl.SetText(10963)

	TextAreaControl = ImportWindow.GetControl(2)
	TextAreaControl.ListResources(CHR_EXPORTS)

	DoneButton = ImportWindow.GetControl(0)
	DoneButton.SetText(36789)
	DoneButton.SetState(IE_GUI_BUTTON_DISABLED)

	CancelButton = ImportWindow.GetControl(1)
	CancelButton.SetText(15416)
	CancelButton.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)
	
	# disable the three extraneous buttons in the bottom row
	for i in [16, 13, 15]:
		TmpButton = MainWindow.GetControl(i)
		TmpButton.SetState(IE_GUI_BUTTON_DISABLED)

	DoneButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, DonePress)
	CancelButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, CancelPress)
	TextAreaControl.SetEvent(IE_GUI_TEXTAREA_ON_CHANGE, SelectFile)
	MainWindow.SetVisible(WINDOW_VISIBLE)
	ImportWindow.SetVisible(WINDOW_VISIBLE)
	return

def DonePress():
	if ImportWindow:
		ImportWindow.Unload()
	if MainWindow:
		MainWindow.Unload()
	#this part is fuzzy
	#we don't have the character as an object in the chargen
	#but we just imported a complete object
	#either we take the important stats and destroy the object
	#or start with an object from the beginning
	#or use a different script here???
	GemRB.SetNextScript("CharGen7")
	return
	
def CancelPress():
	if ImportWindow:
		ImportWindow.Unload()
	if MainWindow:
		MainWindow.Unload()
	GemRB.SetNextScript(GemRB.GetToken("NextScript"))
	return

def SelectFile():
	FileName = TextAreaControl.QueryText()
	Slot = GemRB.GetVar("Slot")
	GemRB.CreatePlayer(FileName, Slot| 0x8000, 1)
	Portrait = GemRB.GetPlayerPortrait (Slot,0)
	PortraitButton.SetPicture (Portrait, "NOPORTLG") 
	ImportWindow.SetVisible(WINDOW_FRONT) #bring it to the front
	DoneButton.SetState(IE_GUI_BUTTON_ENABLED)
	return
