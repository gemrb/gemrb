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
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#


# GUISAVE.py - Save window

###################################################

import GemRB
import GameCheck
import GUICommon
import LoadScreen
from GameCheck import PARTY_SIZE
from GUIDefines import *

SaveWindow = 0
ConfirmWindow = 0
NameField = 0
SaveButton = 0
TextAreaControl = 0
Games = ()
ScrollBar = 0
# this lookup table is used only by bg2
str_chapter = (48007, 48006, 16205, 16206, 16207, 16208, 16209, 71020, 71021, 71022)
num_rows = 4
ctrl_offset = (26, 30, 40, 0x10000008, 0x10000010)
sav_version = 0

def OpenSaveWindow ():
	global SaveWindow, TextAreaControl, Games, ScrollBar
	global num_rows, ctrl_offset, sav_version

	if GUICommon.CloseOtherWindow (OpenSaveWindow):
		CloseSaveWindow ()
		return

	GemRB.HideGUI ()
	GUICommon.GameWindow.SetVisible(WINDOW_INVISIBLE)

	if GameCheck.IsIWD2():
		GemRB.LoadWindowPack ("GUISAVE", 800, 600)
		num_rows = 5
		ctrl_offset = (55, 60, 25, 0x10000005, 0x1000000a)
		sav_version = 22
	else:
		GemRB.LoadWindowPack ("GUISAVE", 640, 480)
	Window = SaveWindow = GemRB.LoadWindow (0)
	Window.SetFrame ()
	if GameCheck.IsIWD2():
		CancelButton=Window.GetControl (22)
	else:
		CancelButton=Window.GetControl (34)
	CancelButton.SetText (13727)
	CancelButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, OpenSaveWindow)
	CancelButton.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)
	GemRB.SetVar ("LoadIdx",0)

	for i in range(num_rows):
		Button = Window.GetControl (ctrl_offset[0]+i)
		Button.SetText (15588)
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, SavePress)
		Button.SetState (IE_GUI_BUTTON_DISABLED)
		Button.SetVarAssoc ("LoadIdx",i)

		Button = Window.GetControl (ctrl_offset[1]+i)
		Button.SetText (13957)
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, DeleteGamePress)
		Button.SetState (IE_GUI_BUTTON_DISABLED)
		Button.SetVarAssoc ("LoadIdx",i)

		#area previews
		Button = Window.GetControl (1+i)
		Button.SetState (IE_GUI_BUTTON_LOCKED)
		Button.SetFlags(IE_GUI_BUTTON_NO_IMAGE|IE_GUI_BUTTON_PICTURE,OP_SET)

		#PC portraits
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
		if ActPos<=len(Games):
			Button1.SetState (IE_GUI_BUTTON_ENABLED)
		else:
			Button1.SetState (IE_GUI_BUTTON_DISABLED)

		if ActPos<len(Games):
			Slotname = Games[ActPos].GetName()
			Button2.SetState (IE_GUI_BUTTON_ENABLED)
		elif ActPos == len(Games):
			Slotname = 15304
			Button2.SetState (IE_GUI_BUTTON_DISABLED)
		else:
			Slotname = ""
			Button2.SetState (IE_GUI_BUTTON_DISABLED)

		Label = Window.GetControl (ctrl_offset[3]+i)
		Label.SetText (Slotname)

		if ActPos<len(Games):
			Slotname = Games[ActPos].GetGameDate()
		else:
			Slotname = ""
		Label = Window.GetControl (ctrl_offset[4]+i)
		Label.SetText (Slotname)

		Button=Window.GetControl (1+i)
		if ActPos<len(Games):
			Button.SetSprite2D(Games[ActPos].GetPreview())
		else:
			Button.SetPicture("")
		for j in range(min(6, PARTY_SIZE)):
			Button=Window.GetControl (ctrl_offset[2] + i*min(6, PARTY_SIZE) + j)
			if ActPos<len(Games):
				Button.SetSprite2D(Games[ActPos].GetPortrait(j))
			else:
				Button.SetPicture("")
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
	SaveWindow.SetVisible (WINDOW_VISIBLE)
	return

def ConfirmedSaveGame():
	global ConfirmWindow

	Pos = GemRB.GetVar ("TopIndex")+GemRB.GetVar ("LoadIdx")
	Label = ConfirmWindow.GetControl (3)
	Slotname = Label.QueryText ()
	Slotname = Slotname.replace ("/", "|") # can't have path separators in the name
	#FIXME: make this work
	#LoadScreen.StartLoadScreen()
	if ConfirmWindow:
		ConfirmWindow.Unload ()
		ConfirmWindow = None
	OpenSaveWindow() # close window
	GemRB.HideGUI ()
	if Pos < len(Games):
		GemRB.SaveGame(Games[Pos], Slotname, sav_version)
	else:
		GemRB.SaveGame(None, Slotname, sav_version)
	GemRB.UnhideGUI ()
	return

def SavePress():
	global ConfirmWindow, NameField, SaveButton

	Pos = GemRB.GetVar ("TopIndex")+GemRB.GetVar ("LoadIdx")
	ConfirmWindow = GemRB.LoadWindow (1)

	#slot name
	if Pos<len(Games):
		Slotname = Games[Pos].GetName()
		save_strref = 15306
	else:
		Slotname = ""
		save_strref = 15588
	NameField = ConfirmWindow.GetControl (3)
	NameField.SetText (Slotname)
	NameField.SetEvent (IE_GUI_EDIT_ON_CHANGE, EditChange)

	#game hours (should be generated from game)
	if Pos<len(Games):
		if GameCheck.IsBG2():
			Chapter = GemRB.GetGameVar ("CHAPTER") & 0x7fffffff
			Slotname = GemRB.GetString(str_chapter[Chapter-1]) + " " + Games[Pos].GetGameDate()
		else:
			Slotname = Games[Pos].GetGameDate()
	else:
		Slotname = ""
	Label = ConfirmWindow.GetControl (0x10000004)
	Label.SetText (Slotname)

	#areapreview
	if not GameCheck.IsIWD2():
		Button=ConfirmWindow.GetControl (0)
		if Pos<len(Games):
			Button.SetSprite2D(Games[Pos].GetPreview())
		else:
			Button.SetPicture("")

	#portraits
		for j in range(min(6, PARTY_SIZE)):
			Button=ConfirmWindow.GetControl (40+j)
			if Pos<len(Games):
				Button.SetSprite2D(Games[Pos].GetPortrait(j))
			else:
				Button.SetPicture("")

	#save
	SaveButton=ConfirmWindow.GetControl (7)
	SaveButton.SetText (save_strref)
	SaveButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, ConfirmedSaveGame)
	SaveButton.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)
	#SaveButton.SetState (IE_GUI_BUTTON_DISABLED)
	if Slotname == "":
		SaveButton.SetState (IE_GUI_BUTTON_DISABLED)

	#cancel
	CancelButton=ConfirmWindow.GetControl (8)
	CancelButton.SetText (13727)
	CancelButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, AbortedSaveGame)
	CancelButton.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)

	ConfirmWindow.SetVisible (WINDOW_VISIBLE)
	ConfirmWindow.ShowModal (MODAL_SHADOW_NONE)
	NameField.SetStatus (IE_GUI_CONTROL_FOCUSED)
	return

def EditChange():
	Name = NameField.QueryText ()
	if len(Name)==0:
		SaveButton.SetState (IE_GUI_BUTTON_DISABLED)
	else:
		SaveButton.SetState (IE_GUI_BUTTON_ENABLED)
	return

def DeleteGameConfirm():
	global Games, ConfirmWindow

	TopIndex = GemRB.GetVar ("TopIndex")
	Pos = TopIndex +GemRB.GetVar ("LoadIdx")
	GemRB.DeleteSaveGame(Games[Pos])
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
	CancelButton=ConfirmWindow.GetControl (2)
	CancelButton.SetText (13727)
	CancelButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, DeleteGameCancel)
	CancelButton.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)

	ConfirmWindow.SetVisible (WINDOW_VISIBLE)
	return

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
