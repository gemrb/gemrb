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
from GameCheck import MAX_PARTY_SIZE
from GUIDefines import *

PartySelectWindow = 0
TextArea = 0
PartyCount = 0

def OnLoad():
	global PartySelectWindow, TextArea, PartyCount
	
	PartyCount = GemRB.GetINIPartyCount()
	
	PartySelectWindow = GemRB.LoadWindow(10, "GUISP")
	TextArea = PartySelectWindow.GetControl(6)
	
	ModifyButton = PartySelectWindow.GetControl(12)
	ModifyButton.OnPress (ModifyPress)
	ModifyButton.SetText(10316)

	CancelButton = PartySelectWindow.GetControl(11)
	CancelButton.OnPress (PartySelectWindow.Close)
	CancelButton.SetText(13727)
	CancelButton.MakeEscape()

	DoneButton = PartySelectWindow.GetControl(10)
	DoneButton.OnPress (DonePress)
	DoneButton.SetText(11973)
	DoneButton.MakeDefault()
	
	LeftScrollBar = PartySelectWindow.GetControl (8)
	LeftScrollBar.SetVarAssoc ("TopIndex", 0)
	
	for i in range(0, 6):
		Button = PartySelectWindow.GetControl(i)
		Button.SetFlags(IE_GUI_BUTTON_RADIOBUTTON, OP_OR)
		Button.OnPress (PartyButtonPress)
		Button.SetVarAssoc ("PartyIdx", i)
	
	GemRB.SetVar("PartyIdx", 0) # reset back, so we don't preselect the last button
	ScrollBarPress()
	PartyButtonPress()
	
	PartySelectWindow.Focus()
	
	return

def ScrollBarPress():
	global PartySelectWindow, PartyCount
	Pos = GemRB.GetVar("TopIndex") or 0
	for i in range(0, 6):
		ActPos = Pos + i
		Button = PartySelectWindow.GetControl(i)
		if ActPos<PartyCount:
			if i == GemRB.GetVar ("PartyIdx"):
				Button.SetState (IE_GUI_BUTTON_SELECTED)
			else:
				Button.SetState (IE_GUI_BUTTON_ENABLED)
			Tag = "Party " + str(ActPos)
			PartyDesc = GemRB.GetINIPartyKey(Tag, "Name", "")					
			Button.SetText(PartyDesc)
		else:
			Button.SetState (IE_GUI_BUTTON_DISABLED)
			Button.SetText ("")
	return

def ModifyPress():
	Pos = GemRB.GetVar ("PartyIdx") + GemRB.GetVar ("TopIndex")
	if Pos == 0: # first entry - behaves same as pressing on done
		if PartySelectWindow:
			PartySelectWindow.Close ()
		GemRB.LoadGame(None, 22)
		GemRB.SetNextScript("SPPartyFormation")
	#else: # here come the real modifications

def DonePress():
	global PartySelectWindow
	Pos = GemRB.GetVar("PartyIdx")
	if Pos == 0:
		if PartySelectWindow:
			PartySelectWindow.Close ()
		GemRB.LoadGame(None, 22)
		GemRB.SetNextScript("SPPartyFormation")
	else:
		if PartySelectWindow:
			PartySelectWindow.Close ()
		GemRB.LoadGame(None, 22)
		#here we should load the party characters
		#but gemrb engine limitations require us to
		#return to the main engine (loadscreen)
		GemRB.SetNextScript("SPParty2")
	return
	
def PartyButtonPress():
	global PartySelectWindow, TextArea
	i = GemRB.GetVar ("PartyIdx") + GemRB.GetVar ("TopIndex")
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
	i = GemRB.GetVar ("PartyIdx") + GemRB.GetVar ("TopIndex")
	Tag = "Party " + str(i)
	for j in range(1, min(6, MAX_PARTY_SIZE)+1):
		Key = "Char"+str(j)
		CharName = GemRB.GetINIPartyKey(Tag, Key, "")
		if CharName !="":
			GemRB.CreatePlayer(CharName, j, 1)
	return
