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

# GUILOAD.py - Load game window from GUILOAD winpack

###################################################

import GemRB
import GUICommon
import LoadScreen
from GameCheck import MAX_PARTY_SIZE
from GUIDefines import *

LoadWindow = 0
TextAreaControl = 0
Games = ()
ScrollBar = 0

def OnLoad ():
	global LoadWindow, TextAreaControl, Games, ScrollBar

	LoadWindow = GemRB.LoadWindow (0, "GUILOAD")
	CancelButton=LoadWindow.GetControl (46)
	CancelButton.SetText (4196)
	CancelButton.OnPress (LoadWindow.Close)
	CancelButton.MakeEscape()

	GemRB.SetVar ("LoadIdx",0)

	for i in range (4):
		Button = LoadWindow.GetControl (14+i)
		Button.SetText (28648)
		Button.OnPress (LoadGamePress)
		Button.SetState (IE_GUI_BUTTON_DISABLED)
		Button.SetVarAssoc ("LoadIdx",i)

		Button = LoadWindow.GetControl (18+i)
		Button.SetText (28640)
		Button.OnPress (DeleteGamePress)
		Button.SetState (IE_GUI_BUTTON_DISABLED)
		Button.SetVarAssoc ("LoadIdx",i)

		#area previews
		Button = LoadWindow.GetControl (1+i)
		Button.SetState (IE_GUI_BUTTON_LOCKED)
		Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE|IE_GUI_BUTTON_PICTURE,OP_SET)

		#PC portraits
		for j in range (min(6, MAX_PARTY_SIZE)):
			Button = LoadWindow.GetControl (22+i*min(6, MAX_PARTY_SIZE)+j)
			if not Button:
				continue
			Button.SetState (IE_GUI_BUTTON_LOCKED)
			Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE|IE_GUI_BUTTON_PICTURE,OP_SET)

	ScrollBar=LoadWindow.GetControl (13)
	ScrollBar.OnChange (ScrollBarPress)
	GUICommon.SetSaveDir ()
	Games=GemRB.GetSaveGames () #count of games in save folder?
	if len(Games)>3:
		TopIndex = len(Games)-4
	else:
		TopIndex = 0
	GemRB.SetVar ("TopIndex",TopIndex)
	ScrollBar.SetVarAssoc ("TopIndex", TopIndex, 0, TopIndex)
	ScrollBarPress ()

	LoadWindow.SetEventProxy(ScrollBar)
	LoadWindow.Focus()
	return

def ScrollBarPress ():
	#draw load game portraits
	Pos = GemRB.GetVar ("TopIndex")
	for i in range (4):
		ActPos = Pos + i

		Button1 = LoadWindow.GetControl (14+i)
		Button2 = LoadWindow.GetControl (18+i)
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
		Label = LoadWindow.GetControl (0x10000004+i)
		Label.SetText (Slotname)

		if ActPos<len(Games):
			Slotname = Games[ActPos].GetDate()
		else:
			Slotname = ""
		Label = LoadWindow.GetControl (0x10000008+i)
		Label.SetText (Slotname)

		Button=LoadWindow.GetControl (1+i)
		if ActPos<len(Games):
			Button.SetPicture (Games[ActPos].GetPreview())
		else:
			Button.SetPicture (None)
		for j in range (min(6, MAX_PARTY_SIZE)):
			Button = LoadWindow.GetControl (22+i*min(6, MAX_PARTY_SIZE)+j)
			if not Button:
				continue
			if ActPos<len(Games):
				Button.SetPicture (Games[ActPos].GetPortrait(j))
			else:
				Button.SetPicture (None)
	return

def LoadGamePress ():
	if LoadWindow:
		LoadWindow.Close ()
	Pos = GemRB.GetVar ("TopIndex")+GemRB.GetVar ("LoadIdx")
	LoadScreen.StartLoadScreen ()
	GemRB.LoadGame(Games[Pos]) # load & start game
	GemRB.EnterGame ()
	return

def DeleteGameConfirm ():
	global Games

	TopIndex = GemRB.GetVar ("TopIndex")
	Pos = TopIndex +GemRB.GetVar ("LoadIdx")
	GemRB.DeleteSaveGame(Games[Pos])
	del Games[Pos]
	if TopIndex>0:
		TopIndex = TopIndex - 1
	ScrollBar.SetVarAssoc ("TopIndex", TopIndex, 0, max(0, len(Games) - 4))
	ScrollBarPress ()
	if ConfirmWindow:
		ConfirmWindow.Close ()
	LoadWindow.Focus()
	return

def DeleteGameCancel ():
	if ConfirmWindow:
		ConfirmWindow.Close ()
	LoadWindow.Focus()
	return

def DeleteGamePress ():
	global ConfirmWindow

	ConfirmWindow=GemRB.LoadWindow (1, "GUILOAD")
	ConfirmWindow.SetFlags (WF_ALPHA_CHANNEL, OP_OR)

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
	ConfirmWindow.ShowModal (MODAL_SHADOW_GRAY)
	return
