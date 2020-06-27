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
from GameCheck import MAX_PARTY_SIZE
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
ctrl_offset = (26, 30, 40, 0x10000008, 0x10000010, 25, 34, 3, 0x10000004, 40, 7, 8, 2)
sav_version = 0
strs = { 'cancel' : 13727, 'save' : 15588, 'delete' : 13957, 'empty' : 15304, 'overwrite' : 15306, 'yousure' : 15305 }

def OpenSaveWindow ():
	global SaveWindow, Games, ScrollBar
	global num_rows, ctrl_offset, sav_version, strs

	if GUICommon.CloseOtherWindow (OpenSaveWindow):
		CloseSaveWindow ()
		return

	GemRB.HideGUI ()
	GUICommon.GameWindow.SetVisible(WINDOW_INVISIBLE)

	if GameCheck.IsIWD2():
		GemRB.LoadWindowPack ("GUISAVE", 800, 600)
		num_rows = 5
		ctrl_offset = (55, 60, 25, 0x10000005, 0x1000000a, 23, 22, 3, 0x10000004, 40, 7, 8, 2)
		sav_version = 22
	else:
		GemRB.LoadWindowPack ("GUISAVE", 640, 480)
		if GameCheck.IsPST():
			ctrl_offset = (14, 18, 22, 0x10000004, 0x10000008, 13, 46, 1, 0x10000002, 6, 4, 5, 3)
			strs = { 'cancel' : 4196, 'save' : 28645, 'delete' : 28640, 'empty' : 28647, 'overwrite' : 28644, 'yousure' : 28639 }

	SaveWindow = Window = GemRB.LoadWindow (0)
	Window.SetFrame ()
	GemRB.SetVar ("OtherWindow", SaveWindow.ID)

	# Cancel button
	CancelButton = Window.GetControl (ctrl_offset[6])
	CancelButton.SetText (strs['cancel'])
	CancelButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, OpenSaveWindow)
	CancelButton.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)
	GemRB.SetVar ("SaveIdx", 0)

	for i in range(num_rows):
		Button = Window.GetControl (ctrl_offset[0]+i)
		Button.SetText (strs['save'])
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, OpenConfirmWindow)
		Button.SetState (IE_GUI_BUTTON_DISABLED)
		Button.SetVarAssoc ("SaveIdx", i)

		Button = Window.GetControl (ctrl_offset[1]+i)
		Button.SetText (strs['delete'])
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, DeleteGamePress)
		Button.SetState (IE_GUI_BUTTON_DISABLED)
		Button.SetVarAssoc ("SaveIdx", i)

		# area previews
		Button = Window.GetControl (1+i)
		Button.SetState (IE_GUI_BUTTON_LOCKED)
		Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE|IE_GUI_BUTTON_PICTURE, OP_SET)

		# PC portraits
		for j in range(min(6, MAX_PARTY_SIZE)):
			Button = Window.GetControl (ctrl_offset[2] + i*min(6, MAX_PARTY_SIZE) + j)
			Button.SetState (IE_GUI_BUTTON_LOCKED)
			Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE|IE_GUI_BUTTON_PICTURE, OP_SET)

	ScrollBar = Window.GetControl (ctrl_offset[5])
	ScrollBar.SetEvent (IE_GUI_SCROLLBAR_ON_CHANGE, ScrollBarPress)
	Games = GemRB.GetSaveGames ()
	TopIndex = max (0, len(Games) - num_rows + 1) #one more for the 'new game'
	GemRB.SetVar ("TopIndex",TopIndex)
	ScrollBar.SetVarAssoc ("TopIndex", TopIndex)
	ScrollBar.SetDefaultScrollBar ()
	ScrollBarPress ()
	Window.SetVisible (WINDOW_VISIBLE)
	return

def ScrollBarPress():
	Window = SaveWindow

	# draw load game portraits
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
			Slottime = Games[ActPos].GetGameDate ()
			Button2.SetState (IE_GUI_BUTTON_ENABLED)
		elif ActPos == len(Games):
			Slotname = strs['empty']
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

		for j in range(min(6, MAX_PARTY_SIZE)):
			Button = Window.GetControl (ctrl_offset[2] + i*min(6, MAX_PARTY_SIZE) + j)
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

def CloseConfirmWindow():
	global ConfirmWindow

	if ConfirmWindow:
		ConfirmWindow.Unload ()
		ConfirmWindow = None
		GemRB.SetVar ("FloatWindow", -1)
	SaveWindow.SetVisible (WINDOW_VISIBLE)
	return

def AbortedSaveGame():
	CloseConfirmWindow ()
	return

# User entered save name and pressed save/overwrite.
# Display progress bar screen and save the game, close the save windows
def ConfirmedSaveGame():
	global ConfirmWindow

	Pos = GemRB.GetVar ("TopIndex") + GemRB.GetVar ("SaveIdx")
	Label = ConfirmWindow.GetControl (ctrl_offset[7])
	Slotname = Label.QueryText ()
	Slotname = Slotname.replace ("/", "|") # can't have path separators in the name

	# Empty save name. We can get here if user presses Enter key
	if Slotname == "":
		return

	# We have to close floating window first
	OpenConfirmWindow ()
	#FIXME: make this work
	#LoadScreen.StartLoadScreen (LoadScreen.LS_TYPE_SAVING)
	OpenSaveWindow ()
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
	GemRB.SetVar ("FloatWindow", ConfirmWindow.ID)

	# Slot name
	if Pos < len(Games):
		Slotname = Games[Pos].GetName()
		save_strref = strs['overwrite']
	else:
		Slotname = ""
		save_strref = strs['save']

	NameField = ConfirmWindow.GetControl (ctrl_offset[7])
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
	Label = ConfirmWindow.GetControl (ctrl_offset[8])
	Label.SetText (Slotname)

	# Areapreview
	if not GameCheck.IsIWD2():
		Button = ConfirmWindow.GetControl (0)
		if Pos<len(Games):
			Button.SetSprite2D(Games[Pos].GetPreview())
		else:
			Button.SetPicture("")

		# PC portraits
		for j in range(min(6, MAX_PARTY_SIZE)):
			Button = ConfirmWindow.GetControl (ctrl_offset[9]+j)
			if Pos<len(Games):
				Button.SetSprite2D(Games[Pos].GetPortrait(j))
			else:
				Button.SetPicture("")

	# Save/Overwrite
	SaveButton = ConfirmWindow.GetControl (ctrl_offset[10])
	SaveButton.SetText (save_strref)
	SaveButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, ConfirmedSaveGame)
	SaveButton.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)
	if Slotname == "":
		SaveButton.SetState (IE_GUI_BUTTON_DISABLED)

	# Cancel
	CancelButton = ConfirmWindow.GetControl (ctrl_offset[11])
	CancelButton.SetText (strs['cancel'])
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
	global Games

	TopIndex = GemRB.GetVar ("TopIndex")
	Pos = TopIndex + GemRB.GetVar ("SaveIdx")
	GemRB.DeleteSaveGame (Games[Pos])
	del Games[Pos]
	if TopIndex>0:
		GemRB.SetVar ("TopIndex",TopIndex-1)
	ScrollBar.SetVarAssoc ("TopIndex", len(Games))
	ScrollBarPress()

	CloseConfirmWindow ()
	return

def DeleteGameCancel():
	CloseConfirmWindow ()
	return

def DeleteGamePress():
	global ConfirmWindow

	SaveWindow.SetVisible (WINDOW_INVISIBLE)
	ConfirmWindow=GemRB.LoadWindow (ctrl_offset[12])

	Text=ConfirmWindow.GetControl (0)
	Text.SetText (strs['yousure'])

	DeleteButton=ConfirmWindow.GetControl (1)
	DeleteButton.SetText (strs['delete'])
	DeleteButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, DeleteGameConfirm)
	DeleteButton.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)

	CancelButton=ConfirmWindow.GetControl (2)
	CancelButton.SetText (strs['cancel'])
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
		GemRB.SetVar ("OtherWindow", -1)
	if GemRB.GetVar ("QuitAfterSave"):
		GemRB.QuitGame ()
		GemRB.SetNextScript ("Start")
		return

	GUICommon.GameWindow.SetVisible(WINDOW_VISIBLE) #enabling the game control screen
	GemRB.UnhideGUI () #enabling the other windows
	return
