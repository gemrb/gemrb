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
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
#
# $Id$


# PartyFormation.py - Single Player Party Formation

###################################################

import GemRB

PartyFormationWindow = 0
CreateCharWindow = 0
ExitWindow = 0

def OnLoad ():
	global PartyFormationWindow

	GemRB.LoadWindowPack ("GUISP", 640, 480)
	PartyFormationWindow = GemRB.LoadWindow (0)
	GemRB.SetWindowFrame (PartyFormationWindow)

	ModifyCharsButton = GemRB.GetControl (PartyFormationWindow, 43)
	GemRB.SetEvent (PartyFormationWindow, ModifyCharsButton, IE_GUI_BUTTON_ON_PRESS, "ModifyPress")
	GemRB.SetControlStatus (PartyFormationWindow, ModifyCharsButton, IE_GUI_BUTTON_DISABLED)
	GemRB.SetText (PartyFormationWindow, ModifyCharsButton, 18816)

	ExitButton = GemRB.GetControl (PartyFormationWindow, 30)
	GemRB.SetEvent (PartyFormationWindow, ExitButton, IE_GUI_BUTTON_ON_PRESS, "ExitPress")
	GemRB.SetControlStatus (PartyFormationWindow, ExitButton, IE_GUI_BUTTON_ENABLED)
	GemRB.SetText (PartyFormationWindow, ExitButton, 13906)

	DoneButton = GemRB.GetControl (PartyFormationWindow, 28)
	GemRB.SetText (PartyFormationWindow, DoneButton, 11973)
	Portraits = 0

	for i in range(18,24):
		Label = GemRB.GetControl (PartyFormationWindow, 0x10000012+i)
		#removing this label, it just disturbs us
		GemRB.SetControlSize (PartyFormationWindow, Label, 0, 0)
		Button = GemRB.GetControl (PartyFormationWindow, i-12)
		ResRef = GemRB.GetPlayerPortrait (i-17, 1)
		if ResRef == "":
			GemRB.SetButtonFlags (PartyFormationWindow, Button, IE_GUI_BUTTON_NORMAL,OP_SET)
		else:
			GemRB.SetButtonPicture (PartyFormationWindow, Button, ResRef)
			GemRB.SetButtonFlags (PartyFormationWindow, Button, IE_GUI_BUTTON_PICTURE, OP_OR)
			Portraits = Portraits+1

		CreateCharButton = GemRB.GetControl (PartyFormationWindow,i)
		GemRB.SetVarAssoc (PartyFormationWindow, CreateCharButton, "Slot", i-17)
		GemRB.SetEvent (PartyFormationWindow, CreateCharButton, IE_GUI_BUTTON_ON_PRESS, "CreateCharPress")
		GemRB.SetControlStatus (PartyFormationWindow, CreateCharButton, IE_GUI_BUTTON_ENABLED)
		if ResRef == "":
			GemRB.SetText (PartyFormationWindow, CreateCharButton, 10264)
		else:
			GemRB.SetText (PartyFormationWindow, CreateCharButton, GemRB.GetPlayerName (i-17,0) )

	if Portraits == 0:
		GemRB.SetButtonState (PartyFormationWindow, DoneButton, IE_GUI_BUTTON_DISABLED)
	else:
		GemRB.SetButtonState (PartyFormationWindow, DoneButton, IE_GUI_BUTTON_ENABLED)
		GemRB.SetButtonFlags (PartyFormationWindow, DoneButton, IE_GUI_BUTTON_DEFAULT, OP_OR)
	GemRB.SetEvent (PartyFormationWindow,DoneButton, IE_GUI_BUTTON_ON_PRESS,"EnterGamePress")

	GemRB.SetVisible (PartyFormationWindow, 1)
	return

def CreateCharPress ():
	global PartyFormationWindow, CreateCharWindow

	GemRB.SetVisible (PartyFormationWindow, 0)
	CreateCharWindow = GemRB.LoadWindow (3)

	CreateButton = GemRB.GetControl (CreateCharWindow, 0)
	GemRB.SetEvent (CreateCharWindow, CreateButton, IE_GUI_BUTTON_ON_PRESS, "CreateCharCreatePress")
	GemRB.SetControlStatus (CreateCharWindow, CreateButton, IE_GUI_BUTTON_ENABLED)
	GemRB.SetText (CreateCharWindow, CreateButton, 13954)

	DeleteButton = GemRB.GetControl (CreateCharWindow, 3)
	GemRB.SetEvent (CreateCharWindow, DeleteButton, IE_GUI_BUTTON_ON_PRESS, "CreateCharDeletePress")
	GemRB.SetControlStatus (CreateCharWindow, DeleteButton, IE_GUI_BUTTON_DISABLED)
	GemRB.SetText (CreateCharWindow, DeleteButton, 13957)

	CancelButton = GemRB.GetControl (CreateCharWindow, 4)
	GemRB.SetEvent (CreateCharWindow, CancelButton, IE_GUI_BUTTON_ON_PRESS, "CreateCharCancelPress")
	GemRB.SetControlStatus (CreateCharWindow, CancelButton, IE_GUI_BUTTON_ENABLED)
	GemRB.SetText (CreateCharWindow, CancelButton, 13727)
	GemRB.SetButtonFlags (CreateCharWindow, CreateButton, IE_GUI_BUTTON_DEFAULT, OP_OR)

	GemRB.SetVisible (CreateCharWindow, 1)
	return

def CreateCharCreatePress ():
	global PartyFormationWindow, CreateCharWindow

	GemRB.UnloadWindow (CreateCharWindow)
	GemRB.UnloadWindow (PartyFormationWindow)
	GemRB.SetNextScript ("CharGen")
	return

def CreateCharDeletePress ():
	return

def CreateCharCancelPress ():
	global PartyFormationWindow, CreateCharWindow

	GemRB.UnloadWindow (CreateCharWindow)
	GemRB.SetVisible (PartyFormationWindow, 1)
	return

def ModifyCharsPress ():
	return

def EnterGamePress ():
	GemRB.SetVar ("PlayMode",2) #using mpsave and second row
	GemRB.EnterGame ()
	return

def ExitPress ():
	global PartyFormationWindow, ExitWindow

	GemRB.SetVisible (PartyFormationWindow, 0)
	ExitWindow = GemRB.LoadWindow (7)

	ExitButton = GemRB.GetControl (ExitWindow, 1)
	GemRB.SetEvent (ExitWindow, ExitButton, IE_GUI_BUTTON_ON_PRESS, "ExitExitPress")
	GemRB.SetText (ExitWindow, ExitButton, 13906)

	CancelButton = GemRB.GetControl (ExitWindow, 2)
	GemRB.SetEvent (ExitWindow, CancelButton, IE_GUI_BUTTON_ON_PRESS, "ExitCancelPress")
	GemRB.SetText (ExitWindow, CancelButton, 13727)

	TextArea = GemRB.GetControl (ExitWindow, 0)
	GemRB.SetText (ExitWindow, TextArea, 11329)

	GemRB.SetVisible (ExitWindow, 1)
	return

def ExitCancelPress ():
	global PartyFormationWindow, ExitWindow

	GemRB.UnloadWindow (ExitWindow)
	GemRB.SetVisible (PartyFormationWindow, 1)
	return

def ExitExitPress ():
	global PartyFormationWindow, ExitWindow

	GemRB.UnloadWindow (ExitWindow)
	GemRB.UnloadWindow (PartyFormationWindow)
	GemRB.SetNextScript ("Start")
	return
