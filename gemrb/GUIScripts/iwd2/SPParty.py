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
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
#
#
#Single Player Party Select
import GemRB
from GameCheck import PARTY_SIZE
from GUIDefines import *

PartySelectWindow = 0
TextArea = 0
PartyCount = 0
ScrollBar = 0

def OnLoad():
	global PartySelectWindow, TextArea, PartyCount, ScrollBar
	GemRB.LoadWindowPack("GUISP", 800, 600)
	
	PartyCount = GemRB.GetINIPartyCount()
	
	PartySelectWindow = GemRB.LoadWindow(10)
	PartySelectWindow.SetFrame( )
	TextArea = PartySelectWindow.GetControl(6)
	ScrollBar = PartySelectWindow.GetControl(8)
	ScrollBar.SetEvent(IE_GUI_SCROLLBAR_ON_CHANGE, ScrollBarPress)
	ScrollBar.SetVarAssoc("TopIndex", PartyCount)
	
	ModifyButton = PartySelectWindow.GetControl(12)
	ModifyButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, ModifyPress)
	ModifyButton.SetText(10316)

	CancelButton = PartySelectWindow.GetControl(11)
	CancelButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, CancelPress)
	CancelButton.SetText(13727)
	CancelButton.SetFlags(IE_GUI_BUTTON_CANCEL,OP_OR)

	DoneButton = PartySelectWindow.GetControl(10)
	DoneButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, DonePress)
	DoneButton.SetText(11973)
	DoneButton.SetFlags(IE_GUI_BUTTON_DEFAULT,OP_OR)
	
	GemRB.SetVar("PartyIdx",0)
	GemRB.SetVar("TopIndex",0)
	
	for i in range(0,PARTY_SIZE):
		Button = PartySelectWindow.GetControl(i)
		Button.SetFlags(IE_GUI_BUTTON_RADIOBUTTON, OP_OR)
		Button.SetEvent(IE_GUI_BUTTON_ON_PRESS, PartyButtonPress)
	
	ScrollBarPress()
	PartyButtonPress()
	
	PartySelectWindow.SetVisible(WINDOW_VISIBLE)
	
	return

def ScrollBarPress():
	global PartySelectWindow, PartyCount
	Pos = GemRB.GetVar("TopIndex")
	for i in range(0,PARTY_SIZE):
		ActPos = Pos + i
		Button = PartySelectWindow.GetControl(i)
		Button.SetText("")
		Button.SetVarAssoc("PartyIdx",-1)
		if ActPos<PartyCount:
			Button.SetState(IE_GUI_BUTTON_ENABLED)
		else:
			Button.SetState(IE_GUI_BUTTON_DISABLED)

	for i in range(0,PARTY_SIZE):
		ActPos = Pos + i
		Button = PartySelectWindow.GetControl(i)
		if ActPos<PartyCount:
			Button.SetVarAssoc("PartyIdx",ActPos)
			Tag = "Party " + str(ActPos)
			PartyDesc = GemRB.GetINIPartyKey(Tag, "Name", "")					
			Button.SetText(PartyDesc)
	return

def ModifyPress():
	Pos = GemRB.GetVar("PartyIdx")
	if Pos == 0: # first entry - behaves same as pressing on done
		if PartySelectWindow:
			PartySelectWindow.Unload()
		GemRB.LoadGame(None, 22)
		GemRB.SetNextScript("SPPartyFormation")
	#else: # here come the real modifications

def DonePress():
	global PartySelectWindow
	Pos = GemRB.GetVar("PartyIdx")
	if Pos == 0:
		if PartySelectWindow:
			PartySelectWindow.Unload()
		GemRB.LoadGame(None, 22)
		GemRB.SetNextScript("SPPartyFormation")
	else:
		if PartySelectWindow:
			PartySelectWindow.Unload()
		GemRB.LoadGame(None, 22)
		#here we should load the party characters
		#but gemrb engine limitations require us to
		#return to the main engine (loadscreen)
		GemRB.SetNextScript("SPParty2")
	return	
	
def CancelPress():
	global PartySelectWindow
	if PartySelectWindow:
		PartySelectWindow.Unload()
	GemRB.SetNextScript("Start")
	return
	
def PartyButtonPress():
	global PartySelectWindow, TextArea
	i = GemRB.GetVar("PartyIdx")
	Tag = "Party " + str(i)
	PartyDesc = ""
	for j in range(1, 9):
		Key = "Descr" + str(j)
		NewString = GemRB.GetINIPartyKey(Tag, Key, "")
		if NewString != "":
			NewString = NewString + "\n\n"
			PartyDesc = PartyDesc + NewString
	
	TextArea.SetText(PartyDesc)
	return

#loading characters from party.ini
def LoadPartyCharacters():
	i = GemRB.GetVar("PartyIdx")
	Tag = "Party " + str(i)
	for j in range(1, PARTY_SIZE+1):
		Key = "Char"+str(j)
		CharName = GemRB.GetINIPartyKey(Tag, Key, "")
		print Tag, Key, CharName
		if CharName !="":
			GemRB.CreatePlayer(CharName, j, 1)
	return
