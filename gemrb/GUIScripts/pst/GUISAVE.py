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
# $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/GUIScripts/pst/GUISAVE.py,v 1.2 2004/04/26 12:48:51 edheldil Exp $


# GUISAVE.py - Save game screen from GUISAVE winpack

###################################################

import GemRB

SaveWindow = 0
TextAreaControl = 0
GameCount = 0
ScrollBar = 0

def OnLoad():
	global SaveWindow, TextAreaControl, GameCount, ScrollBar

	GemRB.LoadWindowPack("GUISAVE")
	SaveWindow = GemRB.LoadWindow(0)
	CancelButton=GemRB.GetControl(SaveWindow,46)
	GemRB.SetText(SaveWindow, CancelButton, 4196)
	GemRB.SetEvent(SaveWindow,CancelButton,IE_GUI_BUTTON_ON_PRESS, "CancelPress")
	GemRB.SetVar("SaveIdx",0)
	GemRB.SetVar("TopIndex",0)

	for i in range(0,4):
		Button = GemRB.GetControl(SaveWindow,14+i)
		GemRB.SetText(SaveWindow, Button, 28645)
		GemRB.SetEvent(SaveWindow, Button, IE_GUI_BUTTON_ON_PRESS, "SaveGamePress")
		GemRB.SetButtonState(SaveWindow, Button, IE_GUI_BUTTON_DISABLED)
		GemRB.SetVarAssoc(SaveWindow, Button, "SaveIdx",i)

		Button = GemRB.GetControl(SaveWindow, 18+i)
		GemRB.SetText(SaveWindow, Button, 28640)
		GemRB.SetEvent(SaveWindow, Button, IE_GUI_BUTTON_ON_PRESS, "DeleteGamePress")
		GemRB.SetButtonState(SaveWindow, Button, IE_GUI_BUTTON_DISABLED)
		GemRB.SetVarAssoc(SaveWindow, Button, "SaveIdx",i)

		#area previews
		Button = GemRB.GetControl(SaveWindow, 1+i)
		GemRB.SetButtonFlags(SaveWindow, Button, IE_GUI_BUTTON_NO_IMAGE|IE_GUI_BUTTON_PICTURE,OP_SET)

		#PC portraits
		for j in range(0,6):
			Button = GemRB.GetControl(SaveWindow,22+i*6+j)
			GemRB.SetButtonFlags(SaveWindow, Button, IE_GUI_BUTTON_NO_IMAGE|IE_GUI_BUTTON_PICTURE,OP_SET)

	ScrollBar=GemRB.GetControl(SaveWindow, 13)
	GemRB.SetEvent(SaveWindow, ScrollBar, IE_GUI_SCROLLBAR_ON_CHANGE, "ScrollBarPress")
	GameCount=GemRB.GetSaveGameCount()   #count of games in save folder?
	GemRB.SetVarAssoc(SaveWindow, ScrollBar, "TopIndex", GameCount)
	ScrollBarPress()
	GemRB.SetVisible(SaveWindow,1)
	return

def ScrollBarPress():
	#draw load game portraits
	Pos = GemRB.GetVar("TopIndex")
	for i in range(0,4):
		ActPos = Pos + i

		Button1 = GemRB.GetControl(SaveWindow,14+i)
		Button2 = GemRB.GetControl(SaveWindow, 18+i)
		if ActPos<GameCount:
			GemRB.SetButtonState(SaveWindow, Button1, IE_GUI_BUTTON_ENABLED)
			GemRB.SetButtonState(SaveWindow, Button2, IE_GUI_BUTTON_ENABLED)
		else:
			GemRB.SetButtonState(SaveWindow, Button1, IE_GUI_BUTTON_DISABLED)
			GemRB.SetButtonState(SaveWindow, Button2, IE_GUI_BUTTON_DISABLED)

		if ActPos<GameCount:
			Slotname = GemRB.GetSaveGameAttrib(0,ActPos)
		else:
			Slotname = ""
		Label = GemRB.GetControl(SaveWindow, 0x10000004+i)
		GemRB.SetText(SaveWindow, Label, Slotname)

		if ActPos<GameCount:
			Slotname = GemRB.GetSaveGameAttrib(3,ActPos)
		else:
			Slotname = ""
		Label = GemRB.GetControl(SaveWindow, 0x10000008+i)
		GemRB.SetText(SaveWindow, Label, Slotname)

		Button=GemRB.GetControl(SaveWindow, 1+i)
		if ActPos<GameCount:
			GemRB.SetSaveGamePreview(SaveWindow, Button, ActPos)
		else:
			GemRB.SetButtonPicture(SaveWindow, Button, "")
		for j in range(0,6):
			Button=GemRB.GetControl(SaveWindow, 22 + i*6 + j)
			if ActPos<GameCount:
				GemRB.SetSaveGamePortrait(SaveWindow, Button, ActPos,j)
			else:
				GemRB.SetButtonPicture(SaveWindow, Button, "")
	return

def SaveGamePress():
	Pos = GemRB.GetVar("TopIndex")+GemRB.GetVar("SaveIdx")
	return

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
	GemRB.UnloadWindow(SaveWindow)
	GemRB.SetNextScript("Start")
	return
