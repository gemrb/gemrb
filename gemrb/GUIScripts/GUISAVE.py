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
import GameCheck
import GUICommon
import LoadScreen
from GameCheck import PARTY_SIZE
from GUIDefines import *

SaveWindow = None
ConfirmWindow = None
NameField = 0
SaveButton = 0
Games = ()
ScrollBar = 0
# this lookup table is used only by bg2
str_chapter = (48007, 48006, 16205, 16206, 16207, 16208, 16209, 71020, 71021, 71022)
num_rows = 4
ctrl_offset = (26, 30, 40, 0x10000008, 0x10000010)
sav_version = 0

def OpenSaveWindow ():
	global SaveWindow, Games, ScrollBar
	global num_rows, ctrl_offset, sav_version

	if GUICommon.CloseOtherWindow (OpenSaveWindow):
		GemRB.SetVar ("OtherWindow", -1)
		CloseSaveWindow ()
		return

	GemRB.GamePause (1, 3)
	GemRB.HideGUI ()
	GUICommon.GameWindow.SetVisible(WINDOW_INVISIBLE)

	if GameCheck.IsIWD2():
		GemRB.LoadWindowPack ("GUISAVE", 800, 600)
		num_rows = 5
		ctrl_offset = (55, 60, 25, 0x10000005, 0x1000000a)
		sav_version = 22
	else:
		GemRB.LoadWindowPack ("GUISAVE", 640, 480)
	SaveWindow = Window = GemRB.LoadWindow (0)
	Window.SetFrame ()
	if GameCheck.IsIWD2():
		CancelButton=Window.GetControl (22)
	else:
		CancelButton=Window.GetControl (34)
	CancelButton.SetText (13727)
	CancelButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, OpenSaveWindow)
	CancelButton.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)
	GemRB.SetVar ("SaveIdx",0)

	for i in range(num_rows):
		Button = Window.GetControl (ctrl_offset[0]+i)
		Button.SetText (15588)
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, OpenConfirmWindow)
		Button.SetState (IE_GUI_BUTTON_DISABLED)
		Button.SetVarAssoc ("SaveIdx", i)

		Button = Window.GetControl (ctrl_offset[1]+i)
		Button.SetText (13957)
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, DeleteGamePress)
		Button.SetState (IE_GUI_BUTTON_DISABLED)
		Button.SetVarAssoc ("SaveIdx", i)

		# area previews
		Button = Window.GetControl (1+i)
		Button.SetState (IE_GUI_BUTTON_LOCKED)
		Button.SetFlags(IE_GUI_BUTTON_NO_IMAGE|IE_GUI_BUTTON_PICTURE,OP_SET)

		# PC portraits
		for j in range(min(6, PARTY_SIZE)):
			Button = Window.GetControl (ctrl_offset[2] + i*min(6, PARTY_SIZE) + j)
			Button.SetState (IE_GUI_BUTTON_LOCKED)
			Button.SetFlags(IE_GUI_BUTTON_NO_IMAGE|IE_GUI_BUTTON_PICTURE,OP_SET)

	if GameCheck.IsIWD2():
		ScrollBar=Window.GetControl (23)
	else:
		ScrollBar=Window.GetControl (25)
	ScrollBar.SetEvent (IE_GUI_SCROLLBAR_ON_CHANGE, ScrollBarPress)
	Games=GemRB.GetSaveGames ()
	TopIndex = max (0, len(Games) - num_rows + 1) #one more for the 'new game'
	GemRB.SetVar ("TopIndex",TopIndex)
	ScrollBar.SetVarAssoc ("TopIndex", TopIndex+1)
	ScrollBar.SetDefaultScrollBar ()
	ScrollBarPress ()
	Window.SetVisible (WINDOW_VISIBLE)
	return

def ScrollBarPress():
	Window = SaveWindow

	#draw load game portraits
	Pos = GemRB.GetVar ("TopIndex")
	for i in range(num_rows):
		ActPos = Pos + i

		Button1 = Window.GetControl (ctrl_offset[0]+i)
		Button2 = Window.GetControl (ctrl_offset[1]+i)
		if ActPos <= len(Games):
			Button1.SetState (IE_GUI_BUTTON_ENABLED)
		else:
			Button1.SetState (IE_GUI_BUTTON_DISABLED)

		if ActPos < len(Games):
			Slotname = Games[ActPos].GetName()
			Slottime = Games[ActPos].GetDate ()
			Button2.SetState (IE_GUI_BUTTON_ENABLED)
		elif ActPos == len(Games):
			Slotname = 15304
			Slottime = ""
			Button2.SetState (IE_GUI_BUTTON_DISABLED)
		else:
			Slotname = ""
			Slottime = ""
			Button2.SetState (IE_GUI_BUTTON_DISABLED)

		Label = Window.GetControl (ctrl_offset[3]+i)
		Label.SetText (Slotname)

		Label = Window.GetControl (ctrl_offset[4]+i)
		Label.SetText (Slottime)

		Button = Window.GetControl (1+i)
		if ActPos < len(Games):
			Button.SetSprite2D(Games[ActPos].GetPreview())
		else:
			Button.SetPicture ("")
		for j in range(min(6, PARTY_SIZE)):
			Button = Window.GetControl (ctrl_offset[2] + i*min(6, PARTY_SIZE) + j)
			if ActPos < len(Games):
				Button.SetSprite2D(Games[ActPos].GetPortrait(j))
			else:
				Button.SetPicture ("")
	return

def QuickSavePressed():
	Slot = 1

	if GameCheck.IsTOB():
		Slot = 4

	GemRB.SaveGame(Slot)
	return

def AbortedSaveGame():
	global ConfirmWindow

	if ConfirmWindow:
		ConfirmWindow.Unload ()
		ConfirmWindow = None
		GemRB.SetVar ("FloatWindow", -1)
	SaveWindow.SetVisible (WINDOW_VISIBLE)
	return

# User entered save name and pressed save/overwrite.
# Display progress bar screen and save the game, close the save windows
def ConfirmedSaveGame():
	global ConfirmWindow

	Pos = GemRB.GetVar ("TopIndex") + GemRB.GetVar ("SaveIdx")
	Label = ConfirmWindow.GetControl (3)
	Slotname = Label.QueryText ()
	Slotname = Slotname.replace ("/", "|") # can't have path separators in the name

	# Empty save name. We can get here if user presses Enter key
	if Slotname == "":
		return

	# We have to close floating window first
	OpenConfirmWindow ()
	#FIXME: make this work
	#LoadScreen.StartLoadScreen (LoadScreen.LS_TYPE_SAVING)
	CloseSaveWindow ()
	GemRB.HideGUI ()

	if Pos < len(Games):
		GemRB.SaveGame (Games[Pos], Slotname, sav_version)
	else:
		GemRB.SaveGame (None, Slotname, sav_version)
	GemRB.UnhideGUI ()
	return

def OpenConfirmWindow ():
	global ConfirmWindow, NameField, SaveButton

	if ConfirmWindow != None:
		ConfirmWindow.Unload ()
		ConfirmWindow = None
		GemRB.SetVar ("FloatWindow", -1)
		return

	Pos = GemRB.GetVar ("TopIndex") + GemRB.GetVar ("SaveIdx")
	ConfirmWindow = GemRB.LoadWindow (1)

	# Slot name
	if Pos < len(Games):
		Slotname = Games[Pos].GetName()
		save_strref = 15306
	else:
		Slotname = ""
		save_strref = 15588

	NameField = ConfirmWindow.GetControl (3)
	NameField.SetText (Slotname)
	NameField.SetEvent (IE_GUI_EDIT_ON_CHANGE, EditChange)

	#game hours (should be generated from game)
	if Pos < len(Games):
		if GameCheck.IsBG2():
			Chapter = GemRB.GetGameVar ("CHAPTER") & 0x7fffffff
			Slotname = GemRB.GetString(str_chapter[Chapter-1]) + " " + Games[Pos].GetGameDate()
		else:
			Slotname = Games[Pos].GetGameDate()
	else:
		Slotname = ""
	Label = ConfirmWindow.GetControl (0x10000004)
	Label.SetText (Slotname)

	# Areapreview
	if not GameCheck.IsIWD2():
		Button = ConfirmWindow.GetControl (0)
		if Pos<len(Games):
			Button.SetSprite2D(Games[Pos].GetPreview())
		else:
			Button.SetPicture("")

		# PC portraits
		for j in range(min(6, PARTY_SIZE)):
			Button = ConfirmWindow.GetControl (40+j)
			if Pos<len(Games):
				Button.SetSprite2D(Games[Pos].GetPortrait(j))
			else:
				Button.SetPicture("")

	# Save/Overwrite
	SaveButton = ConfirmWindow.GetControl (7)
	SaveButton.SetText (save_strref)
	SaveButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, ConfirmedSaveGame)
	SaveButton.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)
	if Slotname == "":
		SaveButton.SetState (IE_GUI_BUTTON_DISABLED)

	# Cancel
	CancelButton = ConfirmWindow.GetControl (8)
	CancelButton.SetText (13727)
	CancelButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, AbortedSaveGame)
	CancelButton.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)

	ConfirmWindow.SetVisible (WINDOW_VISIBLE)
	ConfirmWindow.ShowModal (MODAL_SHADOW_NONE)
	NameField.SetStatus (IE_GUI_CONTROL_FOCUSED) # ShowModal will happily reset this..
	return

# Disable Save/Overwrite button if the save slotname is empty,
# else enable it
def EditChange():
	Name = NameField.QueryText ()
	if len(Name) == 0:
		SaveButton.SetState (IE_GUI_BUTTON_DISABLED)
	else:
		SaveButton.SetState (IE_GUI_BUTTON_ENABLED)
	return

def DeleteGameConfirm():
	global Games, ConfirmWindow

	TopIndex = GemRB.GetVar ("TopIndex")
	Pos = TopIndex + GemRB.GetVar ("SaveIdx")
	GemRB.DeleteSaveGame (Games[Pos])
	del Games[Pos]
	if TopIndex>0:
		GemRB.SetVar ("TopIndex",TopIndex-1)
	ScrollBar.SetVarAssoc ("TopIndex", len(Games))
	ScrollBarPress()
	if ConfirmWindow:
		ConfirmWindow.Unload ()
		ConfirmWindow = None
	SaveWindow.SetVisible (WINDOW_VISIBLE)
	return

def DeleteGameCancel():
	global ConfirmWindow

	if ConfirmWindow:
		ConfirmWindow.Unload ()
		ConfirmWindow = None
	SaveWindow.SetVisible (WINDOW_VISIBLE)
	return

def DeleteGamePress():
	global ConfirmWindow

	SaveWindow.SetVisible (WINDOW_INVISIBLE)
	ConfirmWindow=GemRB.LoadWindow (2)

	Text=ConfirmWindow.GetControl (0)
	Text.SetText (15305)

	DeleteButton=ConfirmWindow.GetControl (1)
	DeleteButton.SetText (13957)
	DeleteButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, DeleteGameConfirm)
	DeleteButton.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)

	CancelButton=ConfirmWindow.GetControl (2)
	CancelButton.SetText (13727)
	CancelButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, DeleteGameCancel)
	CancelButton.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)

	ConfirmWindow.SetVisible (WINDOW_VISIBLE)
	return

# Exit either back to game or to the Start window
def CloseSaveWindow ():
	global SaveWindow

	if SaveWindow:
		SaveWindow.Unload ()
		SaveWindow = None
	if GemRB.GetVar ("QuitAfterSave"):
		GemRB.QuitGame ()
		GemRB.SetNextScript ("Start")
		return

	GUICommon.GameWindow.SetVisible(WINDOW_VISIBLE) #enabling the game control screen
	GemRB.UnhideGUI () #enabling the other windows
	return
