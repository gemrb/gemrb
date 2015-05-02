# -*-python-*-
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
import LoadScreen

PartyFormationWindow = 0
CreateCharWindow = 0
ExitWindow = 0

def OnLoad ():
	global PartyFormationWindow

	GUICommonWindows.PortraitWindow = None
	GUICommonWindows.SelectionChangeHandler = None

	GemRB.LoadWindowPack ("GUISP", 640, 480)
	PartyFormationWindow = GemRB.LoadWindow (0)

	ModifyCharsButton = PartyFormationWindow.GetControl (43)
	ModifyCharsButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, None) #TODO: ModifyPress
	ModifyCharsButton.SetStatus (IE_GUI_BUTTON_DISABLED)
	ModifyCharsButton.SetText (18816)

	ExitButton = PartyFormationWindow.GetControl (30)
	ExitButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, ExitPress)
	ExitButton.SetStatus (IE_GUI_BUTTON_ENABLED)
	ExitButton.SetText (13906)
	ExitButton.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)

	DoneButton = PartyFormationWindow.GetControl (28)
	DoneButton.SetText (11973)
	Portraits = 0

	for i in range(18,24):
		Label = PartyFormationWindow.GetControl (0x10000012+i)
		#removing this label, it just disturbs us
		Label.SetSize (0, 0)
		Button = PartyFormationWindow.GetControl (i-12)
		ResRef = GemRB.GetPlayerPortrait (i-17, 1)
		if ResRef == "":
			Button.SetFlags (IE_GUI_BUTTON_NORMAL,OP_SET)
		else:
			Button.SetPicture (ResRef)
			Button.SetFlags (IE_GUI_BUTTON_PICTURE, OP_OR)
			Portraits = Portraits+1
		Button.SetState (IE_GUI_BUTTON_LOCKED)

		CreateCharButton = PartyFormationWindow.GetControl (i)
		CreateCharButton.SetVarAssoc ("Slot", i-17)
		CreateCharButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, CreateCharPress)
		CreateCharButton.SetStatus (IE_GUI_BUTTON_ENABLED)
		CreateCharButton.SetFont ("NORMAL")
		if ResRef == "":
			CreateCharButton.SetText (10264)
		else:
			CreateCharButton.SetText (GemRB.GetPlayerName (i-17,0) )

	if Portraits == 0:
		DoneButton.SetState (IE_GUI_BUTTON_DISABLED)
	else:
		DoneButton.SetState (IE_GUI_BUTTON_ENABLED)
		DoneButton.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)
	DoneButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, EnterGamePress)

	if not GameCheck.HasHOW():
		GemRB.SetVar ("SaveDir",1) #using mpsave??
		GemRB.SetVar ("PlayMode",0) #using second row??
	else:
		GemRB.SetGlobal ("EXPANSION_DOOR", "GLOBAL", 1) # entrance to the HOW start
		if GameCheck.HasTOTL():
			GemRB.SetGlobal ("9101_SPAWN_HOBART", "GLOBAL", 1)
		if GemRB.GetVar("ExpansionGame") == 1:
			GemRB.SetGlobal ("CHAPTER", "GLOBAL", 1)
			GemRB.SetVar ("SaveDir",1) #using mpsave
			GemRB.SetVar ("PlayMode",2) #using second row
		else:
			GemRB.SetVar ("SaveDir",1) #using mpsave
			GemRB.SetVar ("PlayMode",0) #using first row

	LoadScreen.CloseLoadScreen()
	PartyFormationWindow.SetVisible (WINDOW_VISIBLE)
	return

def CreateCharPress ():
	global PartyFormationWindow, CreateCharWindow

	PartyFormationWindow.SetVisible (WINDOW_INVISIBLE)
	CreateCharWindow = GemRB.LoadWindow (3)

	CreateButton = CreateCharWindow.GetControl (0)
	CreateButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, CreateCharCreatePress)
	CreateButton.SetStatus (IE_GUI_BUTTON_ENABLED)
	CreateButton.SetText (13954)
	CreateButton.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)

	DeleteButton = CreateCharWindow.GetControl (3)
	DeleteButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, CreateCharDeletePress)
	DeleteButton.SetStatus (IE_GUI_BUTTON_DISABLED)
	DeleteButton.SetText (13957)

	CancelButton = CreateCharWindow.GetControl (4)
	CancelButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, CreateCharCancelPress)
	CancelButton.SetStatus (IE_GUI_BUTTON_ENABLED)
	CancelButton.SetText (13727)
	CancelButton.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)

	CreateCharWindow.SetVisible (WINDOW_VISIBLE)
	return

def CreateCharCreatePress ():
	global PartyFormationWindow, CreateCharWindow

	if CreateCharWindow:
		CreateCharWindow.Unload ()
	if PartyFormationWindow:
		PartyFormationWindow.Unload ()
	GemRB.SetNextScript ("CharGen")
	return

def CreateCharDeletePress ():
	return

def CreateCharCancelPress ():
	global PartyFormationWindow, CreateCharWindow

	if CreateCharWindow:
		CreateCharWindow.Unload ()
	PartyFormationWindow.SetVisible (WINDOW_VISIBLE)
	return

def ModifyCharsPress ():
	return

def EnterGamePress ():
	GemRB.EnterGame ()
	return

def ExitPress ():
	global PartyFormationWindow, ExitWindow

	PartyFormationWindow.SetVisible (WINDOW_INVISIBLE)
	ExitWindow = GemRB.LoadWindow (7)

	ExitButton = ExitWindow.GetControl (1)
	ExitButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, ExitExitPress)
	ExitButton.SetText (13906)
	ExitButton.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)

	CancelButton = ExitWindow.GetControl (2)
	CancelButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, ExitCancelPress)
	CancelButton.SetText (13727)
	CancelButton.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)

	TextArea = ExitWindow.GetControl (0)
	TextArea.SetText (11329)

	ExitWindow.SetVisible (WINDOW_VISIBLE)
	return

def ExitCancelPress ():
	global PartyFormationWindow, ExitWindow

	if ExitWindow:
		ExitWindow.Unload ()
	PartyFormationWindow.SetVisible (WINDOW_VISIBLE)
	return

def ExitExitPress ():
	global PartyFormationWindow, ExitWindow

	if ExitWindow:
		ExitWindow.Unload ()
	if PartyFormationWindow:
		PartyFormationWindow.Unload ()
	GemRB.SetNextScript ("Start")
	return
