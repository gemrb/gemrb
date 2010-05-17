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
from LoadScreen import *
from GUICommon import CloseOtherWindow
from GUICommon import GameWindow

SaveWindow = 0
ConfirmWindow = 0
NameField = 0
SaveButton = 0
TextAreaControl = 0
GameCount = 0
ScrollBar = 0

def OpenSaveWindow ():
	global SaveWindow, TextAreaControl, GameCount, ScrollBar

	if CloseOtherWindow (OpenSaveWindow):
		CloseSaveWindow ()
		return

	GemRB.HideGUI ()
	GameWindow.SetVisible(WINDOW_INVISIBLE)

	GemRB.LoadWindowPack ("GUISAVE", 800, 600)
	Window = SaveWindow = GemRB.LoadWindowObject (0)
	Window.SetFrame ()
	CancelButton=Window.GetControl (22)
	CancelButton.SetText (13727)
	CancelButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, "OpenSaveWindow")
	CancelButton.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)
	GemRB.SetVar ("LoadIdx",0)

	for i in range(5):
		Button = Window.GetControl (55+i)
		Button.SetText (15588)
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "SavePress")
		Button.SetState (IE_GUI_BUTTON_DISABLED)
		Button.SetVarAssoc ("LoadIdx",i)

		Button = Window.GetControl (60+i)
		Button.SetText (13957)
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "DeleteGamePress")
		Button.SetState (IE_GUI_BUTTON_DISABLED)
		Button.SetVarAssoc ("LoadIdx",i)

		#area previews
		Button = Window.GetControl (1+i)
		Button.SetState (IE_GUI_BUTTON_LOCKED)
		Button.SetFlags(IE_GUI_BUTTON_NO_IMAGE|IE_GUI_BUTTON_PICTURE,OP_SET)

		#PC portraits
		for j in range(PARTY_SIZE):
			Button = Window.GetControl (25+i*PARTY_SIZE+j)
			Button.SetState (IE_GUI_BUTTON_LOCKED)
			Button.SetFlags(IE_GUI_BUTTON_NO_IMAGE|IE_GUI_BUTTON_PICTURE,OP_SET)

	ScrollBar=Window.GetControl (23)
	ScrollBar.SetEvent (IE_GUI_SCROLLBAR_ON_CHANGE, "ScrollBarPress")
	GameCount=GemRB.GetSaveGameCount ()+1 #one more for the 'new game'
	if GameCount>4:
		TopIndex = GameCount-4
	else:
		TopIndex = 0
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
	for i in range(5):
		ActPos = Pos + i

		Button1 = Window.GetControl (55+i)
		Button2 = Window.GetControl (60+i)
		if ActPos<GameCount:
			Button1.SetState (IE_GUI_BUTTON_ENABLED)
		else:
			Button1.SetState (IE_GUI_BUTTON_DISABLED)

		if ActPos<GameCount-1:
			Slotname = GemRB.GetSaveGameAttrib (0,ActPos)
			Button2.SetState (IE_GUI_BUTTON_ENABLED)
		elif ActPos == GameCount-1:
			Slotname = 15304
			Button2.SetState (IE_GUI_BUTTON_DISABLED)
		else:
			Slotname = ""
			Button2.SetState (IE_GUI_BUTTON_DISABLED)

		Label = Window.GetControl (0x10000005+i)
		Label.SetText (Slotname)

		if ActPos<GameCount-1:
			Slotname = GemRB.GetSaveGameAttrib (4,ActPos)
		else:
			Slotname = ""
		Label = Window.GetControl (0x1000000a+i)
		Label.SetText (Slotname)

		Button=Window.GetControl (1+i)
		if ActPos<GameCount-1:
			Button.SetSaveGamePreview(ActPos)
		else:
			Button.SetPicture("")
		for j in range(PARTY_SIZE):
			Button=Window.GetControl (25+i*PARTY_SIZE+j)
			if ActPos<GameCount-1:
				Button.SetSaveGamePortrait(ActPos,j)
			else:
				Button.SetPicture("")
	return

def AbortedSaveGame():
	if ConfirmWindow:
		ConfirmWindow.Unload ()
	SaveWindow.SetVisible (WINDOW_VISIBLE)
	return

def ConfirmedSaveGame():
	global ConfirmWindow

	Pos = GemRB.GetVar ("TopIndex")+GemRB.GetVar ("LoadIdx")
	Label = ConfirmWindow.GetControl (3)
	Slotname = Label.QueryText ()
	StartLoadScreen()
	GemRB.SaveGame(Pos, Slotname, 22) #saves a game with version 2.2
	if ConfirmWindow:
		ConfirmWindow.Unload ()
	SaveWindow.SetVisible (WINDOW_VISIBLE)
	return

def SavePress():
	global ConfirmWindow, NameField, SaveButton

	Pos = GemRB.GetVar ("TopIndex")+GemRB.GetVar ("LoadIdx")
	ConfirmWindow = GemRB.LoadWindowObject (1)

	#slot name
	if Pos<GameCount-1:
		Slotname = GemRB.GetSaveGameAttrib (0,Pos)
	else:
		Slotname = ""
	NameField = ConfirmWindow.GetControl (3)
	NameField.SetText (Slotname)
	NameField.SetEvent (IE_GUI_EDIT_ON_CHANGE,"EditChange")

	#game hours (should be generated from game)
	if Pos<GameCount-1:
		Slotname = GemRB.GetSaveGameAttrib (4,Pos)
	else:
		Slotname = ""
	Label = ConfirmWindow.GetControl (0x10000004)
	Label.SetText (Slotname)

	#areapreview
	#Button=ConfirmWindow.GetControl (0)
	#if Pos<GameCount-1:
	#	Button.SetSaveGamePreview(Pos)
	#else:
	#	Button.SetPicture("")

	#portraits
	#for j in range(PARTY_SIZE):
	#	Button=ConfirmWindow.GetControl (25+j)
	#	if Pos<GameCount-1:
	#		Button.SetSaveGamePortrait(Pos,j)
	#	else:
	#		Button.SetPicture("")

	#save
	SaveButton=ConfirmWindow.GetControl (7)
	SaveButton.SetText (15588)
	SaveButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, "ConfirmedSaveGame")
	SaveButton.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)
	#SaveButton.SetState (IE_GUI_BUTTON_DISABLED)
	if Slotname == "":
		SaveButton.SetState (IE_GUI_BUTTON_DISABLED)

	#cancel
	CancelButton=ConfirmWindow.GetControl (8)
	CancelButton.SetText (13727)
	CancelButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, "AbortedSaveGame")
	CancelButton.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)

	ConfirmWindow.SetVisible (WINDOW_VISIBLE)
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
	global GameCount

	TopIndex = GemRB.GetVar ("TopIndex")
	Pos = TopIndex +GemRB.GetVar ("LoadIdx")
	GemRB.DeleteSaveGame(Pos)
	if TopIndex>0:
		GemRB.SetVar ("TopIndex",TopIndex-1)
	GameCount=GemRB.GetSaveGameCount () #count of games in save folder?
	ScrollBar.SetVarAssoc ("TopIndex", GameCount)
	ScrollBarPress()
	if ConfirmWindow:
		ConfirmWindow.Unload ()
	SaveWindow.SetVisible (WINDOW_VISIBLE)
	return

def DeleteGameCancel():
	if ConfirmWindow:
		ConfirmWindow.Unload ()
	SaveWindow.SetVisible (WINDOW_VISIBLE)
	return

def DeleteGamePress():
	global ConfirmWindow

	SaveWindow.SetVisible (WINDOW_INVISIBLE)
	ConfirmWindow=GemRB.LoadWindowObject (2)
	Text=ConfirmWindow.GetControl (0)
	Text.SetText (15305)
	DeleteButton=ConfirmWindow.GetControl (1)
	DeleteButton.SetText (13957)
	DeleteButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, "DeleteGameConfirm")
	CancelButton=ConfirmWindow.GetControl (2)
	CancelButton.SetText (13727)
	CancelButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, "DeleteGameCancel")
	CancelButton.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)

	ConfirmWindow.SetVisible (WINDOW_VISIBLE)
	return

def CloseSaveWindow ():
	if SaveWindow:
		SaveWindow.Unload ()
	if GemRB.GetVar ("QuitAfterSave"):
		GemRB.QuitGame ()
		GemRB.SetNextScript ("Start")
		return

	GameWindow.SetVisible(WINDOW_VISIBLE) #enabling the game control screen
	GemRB.UnhideGUI () #enabling the other windows
	return
