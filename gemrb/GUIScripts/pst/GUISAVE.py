# -*-python-*-
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
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
#
# $Id$


# GUISAVE.py - Save game screen from GUISAVE winpack

###################################################

import GemRB
from GUIDefines import *
from LoadScreen import *

SaveWindow = None
SaveDetailWindow = None
OptionsWindow = None
GameCount = 0
ScrollBar = 0


def OpenSaveWindow ():
	global SaveWindow, OptionsWindow, GameCount, ScrollBar

	if SaveWindow:
		GemRB.HideGUI ()

		if SaveDetailWindow: OpenSaveDetailWindow ()
		
		GemRB.UnloadWindow (SaveWindow)
		SaveWindow = None
		# FIXME: LOAD GUIOPT?
		GemRB.SetVar ("OtherWindow", OptionsWindow)

		GemRB.UnhideGUI ()
		return

	GemRB.HideGUI ()
	GemRB.LoadWindowPack ("GUISAVE", 640, 480)
	SaveWindow = Window = GemRB.LoadWindow (0)
	OptionsWindow = GemRB.GetVar ("OtherWindow")
	GemRB.SetVar ("OtherWindow", SaveWindow)


	# Cancel button
	CancelButton = GemRB.GetControl (Window, 46)
	GemRB.SetText (Window, CancelButton, 4196)
	GemRB.SetEvent (Window, CancelButton, IE_GUI_BUTTON_ON_PRESS, "CancelPress")
	GemRB.SetVar ("SaveIdx", 0)

	for i in range (4):
		Button = GemRB.GetControl (Window, 14 + i)
		GemRB.SetText (Window, Button, 28645)
		GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "SaveGamePress")
		GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_DISABLED)
		GemRB.SetVarAssoc (Window, Button, "SaveIdx", i)

		Button = GemRB.GetControl (Window, 18 + i)
		GemRB.SetText (Window, Button, 28640)
		GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "DeleteGamePress")
		GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_DISABLED)
		GemRB.SetVarAssoc (Window, Button, "SaveIdx", i)

		# area previews
		Button = GemRB.GetControl (Window, 1 + i)
		GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_NO_IMAGE|IE_GUI_BUTTON_PICTURE, OP_SET)

		# PC portraits
		for j in range (6):
			Button = GemRB.GetControl (Window, 22 + i*6 + j)
			GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_NO_IMAGE|IE_GUI_BUTTON_PICTURE, OP_SET)

	ScrollBar = GemRB.GetControl(Window, 13)
	GemRB.SetEvent(Window, ScrollBar, IE_GUI_SCROLLBAR_ON_CHANGE, "ScrollBarPress")
	GameCount = GemRB.GetSaveGameCount () + 1 #one more for the 'new game'
	TopIndex = max (0, GameCount - 4)

	GemRB.SetVar ("TopIndex",TopIndex)
	GemRB.SetVarAssoc (Window, ScrollBar, "TopIndex", GameCount)
	ScrollBarPress ()

	GemRB.UnhideGUI ()


def ScrollBarPress():
	Window = SaveWindow
	# draw load game portraits
	Pos = GemRB.GetVar ("TopIndex")
	for i in range (4):
		ActPos = Pos + i

		Button1 = GemRB.GetControl (Window, 14 + i)
		Button2 = GemRB.GetControl (Window, 18 + i)
		if ActPos < GameCount:
			GemRB.SetButtonState(Window, Button1, IE_GUI_BUTTON_ENABLED)
			GemRB.SetButtonState(Window, Button2, IE_GUI_BUTTON_ENABLED)
		else:
			GemRB.SetButtonState(Window, Button1, IE_GUI_BUTTON_DISABLED)
			GemRB.SetButtonState(Window, Button2, IE_GUI_BUTTON_DISABLED)

		if ActPos < GameCount - 1:
			Slotname = GemRB.GetSaveGameAttrib (0, ActPos)
			Slottime = GemRB.GetSaveGameAttrib (3, ActPos)
		elif ActPos == GameCount-1:
			Slotname = 28647    # "Empty"
			Slottime = ""
		else:
			Slotname = ""
			Slottime = ""
			
		Label = GemRB.GetControl (Window, 0x10000004+i)
		GemRB.SetText (Window, Label, Slotname)

		Label = GemRB.GetControl (Window, 0x10000008 + i)
		GemRB.SetText (Window, Label, Slottime)

		Button = GemRB.GetControl (Window, 1 + i)
		if ActPos < GameCount - 1:
			GemRB.SetSaveGamePreview (Window, Button, ActPos)
		else:
			GemRB.SetButtonPicture (Window, Button, "")
			
		for j in range (6):
			Button = GemRB.GetControl (Window, 22 + i*6 + j)
			if ActPos < GameCount - 1:
				GemRB.SetSaveGamePortrait (Window, Button, ActPos,j)
			else:
				GemRB.SetButtonPicture (Window, Button, "")


def SaveGamePress ():
	OpenSaveDetailWindow ()


def DeleteGameConfirm():
	global GameCount

	TopIndex = GemRB.GetVar("TopIndex")
	Pos = TopIndex +GemRB.GetVar("SaveIdx")
	GemRB.DeleteSaveGame(Pos)
	if TopIndex>0:
		GemRB.SetVar("TopIndex",TopIndex-1)
	GameCount=GemRB.GetSaveGameCount()   #count of games in save folder?
	GemRB.SetVarAssoc(SaveWindow, ScrollBar, "TopIndex", GameCount)
	ScrollBarPress()
	GemRB.UnloadWindow(ConfirmWindow)
	GemRB.SetVisible(SaveWindow,1)
	return

def DeleteGameCancel():
	GemRB.UnloadWindow(ConfirmWindow)
	GemRB.SetVisible(SaveWindow,1)
	return

def DeleteGamePress():
	global ConfirmWindow

	GemRB.SetVisible(SaveWindow, 0)
	ConfirmWindow=GemRB.LoadWindow(1)

	Text=GemRB.GetControl(ConfirmWindow, 0)
	GemRB.SetText(ConfirmWindow, Text, 15305)

	DeleteButton=GemRB.GetControl(ConfirmWindow, 1)
	GemRB.SetText(ConfirmWindow, DeleteButton, 13957)
	GemRB.SetEvent(ConfirmWindow, DeleteButton, IE_GUI_BUTTON_ON_PRESS, "DeleteGameConfirm")

	CancelButton=GemRB.GetControl(ConfirmWindow, 2)
	GemRB.SetText(ConfirmWindow, CancelButton, 4196)
	GemRB.SetEvent(ConfirmWindow, CancelButton, IE_GUI_BUTTON_ON_PRESS, "DeleteGameCancel")

	GemRB.SetVisible(ConfirmWindow,1)
	return
	
def CancelPress():
	OpenSaveWindow ()


def OpenSaveDetailWindow ():
	global SaveDetailWindow

	GemRB.HideGUI ()
	
	if SaveDetailWindow != None:
		GemRB.UnloadWindow (SaveDetailWindow)
		SaveDetailWindow = None
		GemRB.SetVar ("FloatWindow", -1)
		
		GemRB.UnhideGUI ()
		return
		
	SaveDetailWindow = Window = GemRB.LoadWindow (1)
	GemRB.SetVar ("FloatWindow", SaveDetailWindow)	


	Pos = GemRB.GetVar ("TopIndex") + GemRB.GetVar ("SaveIdx")


	# Save/Overwrite
	Button = GemRB.GetControl (Window, 4)
	if Pos < GameCount - 1:
		GemRB.SetText (Window, Button, 28644)   # Overwrite
	else:
		GemRB.SetText (Window, Button, 28645)   # Save
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "ConfirmedSaveGame")

	# Cancel
	Button = GemRB.GetControl (Window, 5)
	GemRB.SetText (Window, Button, 4196)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenSaveDetailWindow")


	# Slot name and time
	if Pos < GameCount - 1:
		Slotname = GemRB.GetSaveGameAttrib (0, Pos)
		Slottime = GemRB.GetSaveGameAttrib (4, Pos)
	else:
		Slotname = ""
		Slottime = ""
		
	Edit = GemRB.GetControl (Window, 1)
	GemRB.SetText (Window, Edit, Slotname)
	GemRB.SetControlStatus (Window, Edit, IE_GUI_CONTROL_FOCUSED)
	GemRB.SetEvent (Window, Edit, IE_GUI_EDIT_ON_CHANGE, "CheckSaveName")
	GemRB.SetEvent (Window, Edit, IE_GUI_EDIT_ON_DONE, "ConfirmedSaveGame")

	Label = GemRB.GetControl (Window, 0x10000002)
	GemRB.SetText (Window, Label, Slottime)
	

	# Areapreview
	Button = GemRB.GetControl (Window, 0)
	GemRB.SetGamePreview (Window, Button)

	# PC portraits
	for j in range (PARTY_SIZE):
		Button = GemRB.GetControl (Window, 6 + j)
		GemRB.SetGamePortraitPreview (Window, Button, j)


	CheckSaveName ()
	GemRB.UnhideGUI ()
	GemRB.ShowModal (Window, MODAL_SHADOW_GRAY)


# Disable Save/Overwrite button if the save slotname is empty,
#   else enable it
def CheckSaveName ():
	Window = SaveDetailWindow
	Button = GemRB.GetControl (Window, 4)
	Edit = GemRB.GetControl (Window, 1)
	Name = GemRB.QueryText (Window, Edit)

	if Name == "":
		GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_DISABLED)
	else:
		GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_ENABLED)
		
	

# User entered save name and pressed save/overwrite.
# Display progress bar screen and save the game, close the save windows
def ConfirmedSaveGame ():
	Window = SaveDetailWindow
	
	Pos = GemRB.GetVar ("TopIndex") + GemRB.GetVar ("SaveIdx")
	Label = GemRB.GetControl (Window, 1)
	Slotname = GemRB.QueryText (Window, Label)

	# Empty save name. We can get here if user presses Enter key
	if Slotname == "":
		return

	# We have to close floating window first
	OpenSaveDetailWindow ()
	StartLoadScreen (LS_TYPE_SAVING)
	GemRB.SaveGame (Pos, Slotname)
	CloseSaveWindow ()


# Exit either back to game or to the Start window
def CloseSaveWindow ():
	OpenSaveWindow ()
	if GemRB.GetVar ("QuitAfterSave"):
		GemRB.QuitGame ()
		GemRB.SetNextScript ("Start")
		return

	GemRB.RunEventHandler ("OpenOptionsWindow")

