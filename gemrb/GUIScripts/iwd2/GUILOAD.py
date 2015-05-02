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


# GUILOAD.py - Load window

###################################################

import GemRB
import LoadScreen
from GameCheck import PARTY_SIZE
from GUIDefines import *

LoadWindow = 0
TextAreaControl = 0
Games = ()
ScrollBar = 0

def OnLoad ():
	global LoadWindow, TextAreaControl, Games, ScrollBar

	GemRB.SetVar ("SaveDir",1) #iwd2 is always using 'mpsave'
	GemRB.LoadWindowPack ("GUILOAD", 800 ,600)
	LoadWindow = GemRB.LoadWindow (0)

	CancelButton=LoadWindow.GetControl (22)
	CancelButton.SetText (13727)
	CancelButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, CancelPress)
	CancelButton.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)

	GemRB.SetVar ("LoadIdx",0)
	for i in range (5):
		Button = LoadWindow.GetControl (55+i)
		Button.SetText (15590)
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, LoadGamePress)
		Button.SetState (IE_GUI_BUTTON_DISABLED)
		Button.SetVarAssoc ("LoadIdx",i)

		Button = LoadWindow.GetControl (60+i)
		Button.SetText (13957)
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, DeleteGamePress)
		Button.SetState (IE_GUI_BUTTON_DISABLED)
		Button.SetVarAssoc ("LoadIdx",i)

		#area previews
		Button = LoadWindow.GetControl (1+i)
		Button.SetState (IE_GUI_BUTTON_LOCKED)
		Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE|IE_GUI_BUTTON_PICTURE,OP_SET)

		#PC portraits
		for j in range (PARTY_SIZE):
			Button = LoadWindow.GetControl (25 + i*PARTY_SIZE + j)
			Button.SetState (IE_GUI_BUTTON_LOCKED)
			Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE|IE_GUI_BUTTON_PICTURE,OP_SET)
			Button.SetSize (21, 21)

	ScrollBar=LoadWindow.GetControl (23)
	ScrollBar.SetEvent (IE_GUI_SCROLLBAR_ON_CHANGE, ScrollBarPress)
	Games=GemRB.GetSaveGames()
	TopIndex = max (0, len(Games) - 5)

	ScrollBar.SetVarAssoc ("TopIndex", len(Games))
	ScrollBarPress ()
	LoadWindow.SetVisible (WINDOW_VISIBLE)
	return

def ScrollBarPress ():
	#draw load game portraits
	Pos = GemRB.GetVar ("TopIndex")
	for i in range (5):
		ActPos = Pos + i

		Button1 = LoadWindow.GetControl (55+i)
		Button2 = LoadWindow.GetControl (60+i)
		if ActPos<len(Games):
			Button1.SetState (IE_GUI_BUTTON_ENABLED)
			Button2.SetState (IE_GUI_BUTTON_ENABLED)
		else:
			Button1.SetState (IE_GUI_BUTTON_DISABLED)
			Button2.SetState (IE_GUI_BUTTON_DISABLED)

		if ActPos<len(Games):
			Slotname = Games[ActPos].GetName()
		else:
			Slotname = ""
		Label = LoadWindow.GetControl (0x10000005+i)
		Label.SetText (Slotname)

		if ActPos<len(Games):
			Slotname = Games[ActPos].GetGameDate()
		else:
			Slotname = ""
		Label = LoadWindow.GetControl (0x1000000a+i)
		Label.SetText (Slotname)

		if ActPos<len(Games):
			Slotname = Games[ActPos].GetDate()
		else:
			Slotname = ""
		Label = LoadWindow.GetControl (0x1000000f+i)
		Label.SetText (Slotname)

		Button=LoadWindow.GetControl (1+i)
		if ActPos<len(Games):
			Button.SetSprite2D(Games[ActPos].GetPreview())
		else:
			Button.SetPicture ("")
		for j in range (PARTY_SIZE):
			Button=LoadWindow.GetControl (25 + i*PARTY_SIZE + j)
			if ActPos<len(Games):
				Button.SetSprite2D(Games[ActPos].GetPortrait(j))
			else:
				Button.SetPicture ("")
	return

def LoadGamePress ():
	if LoadWindow:
		LoadWindow.Unload ()
	Pos = GemRB.GetVar ("TopIndex")+GemRB.GetVar ("LoadIdx")
	LoadScreen.StartLoadScreen()
	GemRB.LoadGame(Games[Pos]) #loads and enters savegame
	GemRB.EnterGame ()
	return

def DeleteGameConfirm():
	global Games

	TopIndex = GemRB.GetVar ("TopIndex")
	Pos = TopIndex +GemRB.GetVar ("LoadIdx")
	GemRB.DeleteSaveGame(Games[Pos])
	if TopIndex>0:
		GemRB.SetVar ("TopIndex",TopIndex-1)
	del Games[Pos]
	ScrollBar.SetVarAssoc ("TopIndex", len(Games))
	ScrollBarPress ()
	if ConfirmWindow:
		ConfirmWindow.Unload ()
	LoadWindow.SetVisible (WINDOW_VISIBLE)
	return

def DeleteGameCancel ():
	if ConfirmWindow:
		ConfirmWindow.Unload ()
	LoadWindow.SetVisible (WINDOW_VISIBLE)
	return

def DeleteGamePress ():
	global ConfirmWindow

	LoadWindow.SetVisible (WINDOW_INVISIBLE)
	ConfirmWindow=GemRB.LoadWindow (1)
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

def CancelPress ():
	if LoadWindow:
		LoadWindow.Unload ()
	GemRB.SetNextScript ("Start")
	return
