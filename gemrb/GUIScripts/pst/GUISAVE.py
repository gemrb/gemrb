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
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#


# GUISAVE.py - Save game screen from GUISAVE winpack

###################################################

import GemRB
import GUIClasses
import LoadScreen
import GUIOPT
from GUIDefines import *

SaveWindow = None
SaveDetailWindow = None
OptionsWindow = None
Games = ()
ScrollBar = 0


def OpenSaveWindow ():
	global SaveWindow, OptionsWindow, Games, ScrollBar

	if SaveWindow:
		if SaveDetailWindow: OpenSaveDetailWindow ()

		if SaveWindow:
			SaveWindow.Close ()
		SaveWindow = None
		# FIXME: LOAD GUIOPT?

		return

	SaveWindow = Window = GemRB.LoadWindow (0, "GUISAVE")
	OptionsWindow = GemRB.GetView("OPTWIN")

	# Cancel button
	CancelButton = Window.GetControl (46)
	CancelButton.SetText (4196)
	CancelButton.OnPress (CancelPress)
	CancelButton.MakeEscape()
	GemRB.SetVar ("SaveIdx", 0)

	for i in range (4):
		Button = Window.GetControl (14 + i)
		Button.SetText (28645)
		Button.OnPress (SaveGamePress)
		Button.SetState (IE_GUI_BUTTON_DISABLED)
		Button.SetVarAssoc ("SaveIdx", i)

		Button = Window.GetControl (18 + i)
		Button.SetText (28640)
		Button.OnPress (DeleteGamePress)
		Button.SetState (IE_GUI_BUTTON_DISABLED)
		Button.SetVarAssoc ("SaveIdx", i)

		# area previews
		Button = Window.GetControl (1 + i)
		Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE|IE_GUI_BUTTON_PICTURE, OP_SET)

		# PC portraits
		for j in range (6):
			Button = Window.GetControl (22 + i*6 + j)
			Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE|IE_GUI_BUTTON_PICTURE, OP_SET)

	ScrollBar = Window.GetControl (13)
	ScrollBar.OnChange (ScrollBarPress)
	Window.SetEventProxy (ScrollBar)
	Games = GemRB.GetSaveGames()
	TopIndex = max (0, len(Games) - 4 + 1) #one more for the 'new game'

	GemRB.SetVar ("TopIndex",TopIndex)
	ScrollBar.SetVarAssoc ("TopIndex", TopIndex)
	ScrollBarPress ()

def ScrollBarPress():
	Window = SaveWindow
	# draw load game portraits
	Pos = GemRB.GetVar ("TopIndex") or 0
	for i in range (4):
		ActPos = Pos + i

		Button1 = Window.GetControl (14 + i) # save
		Button2 = Window.GetControl (18 + i) # delete
		PreviewButton = Window.GetControl (1 + i)
		if ActPos < len(Games):
			Slotname = Games[ActPos].GetName ()
			Slottime = Games[ActPos].GetDate ()
			Button1.SetState (IE_GUI_BUTTON_ENABLED)
			Button2.SetState (IE_GUI_BUTTON_ENABLED)
			PreviewButton.SetPicture (Games[ActPos].GetPreview())
		elif ActPos == len(Games):
			Slotname = 28647 # "Empty"
			Slottime = ""
			Button1.SetState (IE_GUI_BUTTON_ENABLED)
			Button2.SetState (IE_GUI_BUTTON_DISABLED)
			PreviewButton.SetPicture (None)
		else:
			Slotname = ""
			Slottime = ""
			Button1.SetState (IE_GUI_BUTTON_DISABLED)
			Button2.SetState (IE_GUI_BUTTON_DISABLED)
			PreviewButton.SetPicture (None)

		Label = Window.GetControl (0x10000004+i)
		Label.SetText (Slotname)

		Label = Window.GetControl (0x10000008 + i)
		Label.SetText (Slottime)

		for j in range (6):
			Button = Window.GetControl (22 + i*6 + j)
			if ActPos < len(Games):
				Button.SetPicture (Games[ActPos].GetPortrait(j))
			else:
				Button.SetPicture (None)


def SaveGamePress ():
	OpenSaveDetailWindow ()
	return

def DeleteGameConfirm():
	global Games

	TopIndex = GemRB.GetVar("TopIndex")
	Pos = TopIndex +GemRB.GetVar("SaveIdx")
	GemRB.DeleteSaveGame (Games[Pos])
	del Games[Pos]
	if TopIndex>0:
		TopIndex -= 1
		GemRB.SetVar ("TopIndex", TopIndex)
	ScrollBar.SetVarAssoc("TopIndex", TopIndex, 0, max (0, len(Games) - 4 + 1))
	ScrollBarPress()
	if ConfirmWindow:
		ConfirmWindow.Close ()
	SaveWindow.Focus()
	return

def DeleteGameCancel():
	global ConfirmWindow

	if ConfirmWindow:
		ConfirmWindow.Close ()
		ConfirmWindow = None
	SaveWindow.Focus()
	return

def DeleteGamePress():
	global ConfirmWindow

	SaveWindow.SetVisible(False)
	ConfirmWindow=GemRB.LoadWindow (3, "GUISAVE")

	Text=ConfirmWindow.GetControl (0)
	Text.SetText (28639)

	DeleteButton=ConfirmWindow.GetControl (1)
	DeleteButton.SetText (28640)
	DeleteButton.OnPress (DeleteGameConfirm)
	DeleteButton.MakeDefault()

	CancelButton=ConfirmWindow.GetControl (2)
	CancelButton.SetText (4196)
	CancelButton.OnPress (DeleteGameCancel)
	CancelButton.MakeEscape()

	ConfirmWindow.Focus()
	return

def CancelPress():
	OpenSaveWindow ()
	return

def OpenSaveDetailWindow ():
	global SaveDetailWindow

	if SaveDetailWindow != None:
		if SaveDetailWindow:
			SaveDetailWindow.Close ()
		SaveDetailWindow = None

		return

	SaveDetailWindow = Window = GemRB.LoadWindow (1, "GUISAVE")

	Pos = GemRB.GetVar ("TopIndex") + GemRB.GetVar ("SaveIdx")


	# Save/Overwrite
	Button = Window.GetControl (4)
	if Pos < len(Games):
		Button.SetText (28644) # Overwrite
	else:
		Button.SetText (28645) # Save
	Button.OnPress (ConfirmedSaveGame)
	Button.MakeDefault()

	# Cancel
	Button = Window.GetControl (5)
	Button.SetText (4196)
	Button.OnPress (OpenSaveDetailWindow)
	Button.MakeEscape()

	# Slot name and time
	if Pos < len(Games):
		Slotname = Games[Pos].GetName ()
		Slottime = Games[Pos].GetGameDate ()
	else:
		Slotname = ""
		Slottime = ""

	Edit = Window.GetControl (1)
	Edit.SetText (Slotname)
	Edit.OnChange (CheckSaveName)
	Edit.OnDone (ConfirmedSaveGame)

	Label = Window.GetControl (0x10000002)
	Label.SetText (Slottime)


	# Areapreview
	Button = Window.GetControl (0)
	Button.SetPicture (GemRB.GetGamePreview())

	# PC portraits
	from GameCheck import MAX_PARTY_SIZE
	for j in range (MAX_PARTY_SIZE):
		Button = Window.GetControl (6 + j)
		if Button:
			Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE | IE_GUI_BUTTON_PICTURE, OP_SET)

	CheckSaveName ()
	Window.ShowModal (MODAL_SHADOW_GRAY)
	Edit.Focus() # ShowModal will happily reset this..


# Disable Save/Overwrite button if the save slotname is empty,
# else enable it
def CheckSaveName ():
	Window = SaveDetailWindow
	Button = Window.GetControl (4)
	Edit = Window.GetControl (1)
	Name = Edit.QueryText ()

	if Name == "":
		Button.SetState (IE_GUI_BUTTON_DISABLED)
	else:
		Button.SetState (IE_GUI_BUTTON_ENABLED)



# User entered save name and pressed save/overwrite.
# Display progress bar screen and save the game, close the save windows
def ConfirmedSaveGame ():
	Window = SaveDetailWindow

	Pos = GemRB.GetVar ("TopIndex") + GemRB.GetVar ("SaveIdx")
	Label = Window.GetControl (1)
	Slotname = Label.QueryText ()

	# Empty save name. We can get here if user presses Enter key
	if Slotname == "":
		return

	# We have to close floating window first
	OpenSaveDetailWindow ()
	#FIXME: make this work
	#LoadScreen.StartLoadScreen (LoadScreen.LS_TYPE_SAVING)
	CloseSaveWindow ()
	if Pos < len(Games):
		GemRB.SaveGame (Games[Pos], Slotname)
	else:
		GemRB.SaveGame (None, Slotname)

def QuickSavePressed():
	GemRB.SaveGame(1)

# Exit either back to game or to the Start window
def CloseSaveWindow ():
	OpenSaveWindow ()
	if GemRB.GetVar ("QuitAfterSave"):
		GemRB.QuitGame ()
		GemRB.SetNextScript ("Start")
		return

	GUIOPT.OpenOptionsWindow ()
