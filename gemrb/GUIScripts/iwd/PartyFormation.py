# GemRB - Infinity Engine Emulator
# Copyright (C) 2003-2005 The GemRB Project
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
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#


# PartyFormation.py - Single Player Party Formation

###################################################

import GemRB
from GUIDefines import *
import GameCheck
import GUICommonWindows

PartyFormationWindow = 0
CreateCharWindow = 0
ExitWindow = 0

def OnLoad ():
	global PartyFormationWindow

	PartyFormationWindow = GemRB.LoadWindow (0, "GUISP")

	ModifyCharsButton = PartyFormationWindow.GetControl (43)
	ModifyCharsButton.OnPress (None) #TODO: ModifyPress
	ModifyCharsButton.SetStatus (IE_GUI_BUTTON_DISABLED)
	ModifyCharsButton.SetText (18816)

	ExitButton = PartyFormationWindow.GetControl (30)
	ExitButton.OnPress (ExitPress)
	ExitButton.SetStatus (IE_GUI_BUTTON_ENABLED)
	ExitButton.SetText (13906)
	ExitButton.MakeEscape()

	DoneButton = PartyFormationWindow.GetControl (28)
	DoneButton.SetText (11973)
	Portraits = 0

	for i in range(18,24):
		Label = PartyFormationWindow.GetControl (0x10000012+i)
		#removing this label, it just disturbs us
		Label.SetSize (0, 0)
		Button = PartyFormationWindow.GetControl (i-12)
		portrait = GemRB.GetPlayerPortrait (i-17, 1)
		if portrait is None:
			Button.SetFlags (IE_GUI_BUTTON_NORMAL,OP_SET)
		else:
			Button.SetPicture (portrait["ResRef"], "NOPORTSM")
			Button.SetFlags (IE_GUI_BUTTON_PICTURE, OP_OR)
			Portraits = Portraits+1
		Button.SetState (IE_GUI_BUTTON_LOCKED)

		CreateCharButton = PartyFormationWindow.GetControl (i)
		CreateCharButton.SetVarAssoc ("Slot", i-17)
		CreateCharButton.OnPress (CreateCharPress)
		CreateCharButton.SetStatus (IE_GUI_BUTTON_ENABLED)
		CreateCharButton.SetFont ("NORMAL")
		if portrait is None:
			CreateCharButton.SetText (10264)
		else:
			CreateCharButton.SetText (GemRB.GetPlayerName (i-17,0) )

	if Portraits == 0:
		DoneButton.SetState (IE_GUI_BUTTON_DISABLED)
	else:
		DoneButton.SetState (IE_GUI_BUTTON_ENABLED)
		DoneButton.MakeDefault()
	DoneButton.OnPress (EnterGamePress)

	if not GameCheck.HasHOW():
		GemRB.SetVar ("PlayMode",0) #using second row??
	else:
		GemRB.SetGlobal ("EXPANSION_DOOR", "GLOBAL", 1) # entrance to the HOW start
		if GameCheck.HasTOTL():
			GemRB.SetGlobal ("9101_SPAWN_HOBART", "GLOBAL", 1)
		if GemRB.GetVar("ExpansionGame") == 1:
			GemRB.SetGlobal ("CHAPTER", "GLOBAL", 1)
			GemRB.SetVar ("PlayMode",2) #using second row
		else:
			GemRB.SetVar ("PlayMode",0) #using first row
	GemRB.SetToken ("SaveDir", "mpsave")
	PartyFormationWindow.ShowModal(MODAL_SHADOW_NONE)

	return

def CreateCharPress ():
	global PartyFormationWindow, CreateCharWindow

	CreateCharWindow = GemRB.LoadWindow (3, "GUISP")

	CreateButton = CreateCharWindow.GetControl (0)
	CreateButton.OnPress (CreateCharCreatePress)
	CreateButton.SetStatus (IE_GUI_BUTTON_ENABLED)
	CreateButton.SetText (13954)
	CreateButton.MakeDefault()

	DeleteButton = CreateCharWindow.GetControl (3)
	DeleteButton.OnPress (CreateCharDeletePress)
	DeleteButton.SetStatus (IE_GUI_BUTTON_DISABLED)
	DeleteButton.SetText (13957)

	CancelButton = CreateCharWindow.GetControl (4)
	CancelButton.OnPress (CreateCharCancelPress)
	CancelButton.SetStatus (IE_GUI_BUTTON_ENABLED)
	CancelButton.SetText (13727)
	CancelButton.MakeEscape()
	
	CreateCharWindow.ShowModal(MODAL_SHADOW_NONE)

	return

def CreateCharCreatePress ():
	global PartyFormationWindow, CreateCharWindow

	if CreateCharWindow:
		CreateCharWindow.Close ()
	if PartyFormationWindow:
		PartyFormationWindow.Close ()
	GemRB.SetNextScript ("CharGen")
	return

def CreateCharDeletePress ():
	return

def CreateCharCancelPress ():
	global PartyFormationWindow, CreateCharWindow

	if CreateCharWindow:
		CreateCharWindow.Close ()
	PartyFormationWindow.Focus()
	return

def ModifyCharsPress ():
	return

def EnterGamePress ():
	PartyFormationWindow.Close()
	GemRB.HardEndPL ()
	GemRB.EnterGame ()
	return

def ExitPress ():
	global PartyFormationWindow, ExitWindow

	ExitWindow = GemRB.LoadWindow (7, "GUISP")

	ExitButton = ExitWindow.GetControl (1)
	ExitButton.OnPress (ExitExitPress)
	ExitButton.SetText (13906)
	ExitButton.MakeDefault()

	CancelButton = ExitWindow.GetControl (2)
	CancelButton.OnPress (ExitCancelPress)
	CancelButton.SetText (13727)
	CancelButton.MakeEscape()

	TextArea = ExitWindow.GetControl (0)
	TextArea.SetText (11329)

	ExitWindow.ShowModal (MODAL_SHADOW_GRAY)
	return

def ExitCancelPress ():
	global PartyFormationWindow, ExitWindow

	if ExitWindow:
		ExitWindow.Close ()
	PartyFormationWindow.Focus()
	return

def ExitExitPress ():
	global PartyFormationWindow, ExitWindow

	if ExitWindow:
		ExitWindow.Close ()
	if PartyFormationWindow:
		PartyFormationWindow.Close ()
	GemRB.SetNextScript ("Start")
	return
