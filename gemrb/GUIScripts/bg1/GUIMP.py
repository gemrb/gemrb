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
#Single Player Party Formation
import GemRB
from GUIDefines import *

PartyFormationWindow = 0
ExitWindow = 0

def OnLoad():
	global PartyFormationWindow

	PartyFormationWindow = GemRB.LoadWindow(0, "GUIMP")

	ExitButton = PartyFormationWindow.GetControl(30)
	ExitButton.SetText(13906)
	ExitButton.OnPress (ExitPress)
	ExitButton.MakeEscape()

	ModifyCharactersButton = PartyFormationWindow.GetControl(43)
	ModifyCharactersButton.SetText(18816)
	ModifyCharactersButton.SetState(IE_GUI_BUTTON_DISABLED)
	#TODO: implement ModifyCharacters
	#ModifyCharactersButton.OnPress (ModifyCharactersPress)

	DoneButton = PartyFormationWindow.GetControl(28)
	DoneButton.SetText(11973)
	Portraits = 0

	for i in range(18,24):
		Label = PartyFormationWindow.GetControl(0x10000012+i)
		#removing this label, it just disturbs us
		Label.SetSize(0, 0)
		Button = PartyFormationWindow.GetControl(i-12)
		portrait = GemRB.GetPlayerPortrait (i-17, 1)
		if portrait is None:
			Button.SetFlags(IE_GUI_BUTTON_NORMAL,OP_SET)
		else:
			Button.SetPicture(portrait["ResRef"])
			Button.SetFlags(IE_GUI_BUTTON_PICTURE, OP_OR)
			Portraits = Portraits+1

		Button.SetVarAssoc("Slot",i-17)
		Button.OnPress (GeneratePress)

		Button = PartyFormationWindow.GetControl(i)
		Button.SetVarAssoc("Slot",i-17)
		if portrait is None:
			Button.SetText(10264)
		else:
			Button.SetText(GemRB.GetPlayerName(i-17,0) )

		Button.OnPress (GeneratePress)

	if Portraits == 0:
		DoneButton.SetState(IE_GUI_BUTTON_DISABLED)
	else:
		DoneButton.SetState(IE_GUI_BUTTON_ENABLED)
	DoneButton.OnPress (EnterGamePress)

	PartyFormationWindow.Focus()
	return

def ExitPress():
	global PartyFormationWindow, ExitWindow
	ExitWindow = GemRB.LoadWindow(7, "GUIMP")

	TextArea = ExitWindow.GetControl(0)
	TextArea.SetText(11329)

	CancelButton = ExitWindow.GetControl(2)
	CancelButton.SetText(13727)
	CancelButton.OnPress (ExitCancelPress)
	CancelButton.MakeEscape()

	DoneButton = ExitWindow.GetControl(1)
	DoneButton.SetText(11973)
	DoneButton.OnPress (ExitWindow.Close)
	DoneButton.MakeDefault()

	ExitWindow.Focus()
	return

def ExitCancelPress():
	global PartyFormationWindow, ExitWindow
	if ExitWindow:
		ExitWindow.Close ()
	PartyFormationWindow.Focus()
	return

def GeneratePress():
	global PartyFormationWindow
	slot = GemRB.GetVar("Slot")
	portrait = GemRB.GetPlayerPortrait (slot, 0)
	if portrait:
		print("Already existing slot, we should drop it")
	if PartyFormationWindow:
		PartyFormationWindow.Close ()
	GemRB.SetNextScript("CharGen")
	return

def EnterGamePress():
	GemRB.EnterGame()
	return

