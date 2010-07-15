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
#character generation, race (GUICG2)
import GemRB
from GUIDefines import *
import CommonTables

RaceWindow = 0
TextAreaControl = 0
DoneButton = 0
SubRacesTable = 0

def OnLoad():
	global RaceWindow, TextAreaControl, DoneButton
	global SubRacesTable
	
	GemRB.LoadWindowPack("GUICG", 800 ,600)
	RaceWindow = GemRB.LoadWindow (8)

	RaceCount = CommonTables.RaceTable.GetRowCount ()
	
	SubRacesTable = GemRB.LoadTable ("SUBRACES")

	for i in range(2,9):
		Button = RaceWindow.GetControl (i)
		Button.SetFlags (IE_GUI_BUTTON_RADIOBUTTON,OP_OR)
		
	for i in range(7):
		Button = RaceWindow.GetControl (i+2)
		Button.SetText (CommonTables.RaceTable.GetValue (i,0) )
		Button.SetState (IE_GUI_BUTTON_ENABLED)
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, RacePress)
		Button.SetVarAssoc ("BaseRace", CommonTables.RaceTable.GetValue (i, 3) )

	BackButton = RaceWindow.GetControl (11) 
	BackButton.SetText (15416)
	BackButton.SetFlags(IE_GUI_BUTTON_CANCEL,OP_OR)

	DoneButton = RaceWindow.GetControl (0)
	DoneButton.SetText (36789)
	DoneButton.SetState (IE_GUI_BUTTON_DISABLED)
	DoneButton.SetFlags (IE_GUI_BUTTON_DEFAULT,OP_OR)

	TextAreaControl = RaceWindow.GetControl (9)
	TextAreaControl.SetText (17237)

	DoneButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, NextPress)
	BackButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, BackPress)
	RaceWindow.SetVisible(WINDOW_VISIBLE)
	return

def RacePress():
	global RaceWindow, SubRacesTable
	Race = GemRB.GetVar ("BaseRace")
	GemRB.SetVar ("Race", Race)
	RaceID = CommonTables.RaceTable.GetRowName (CommonTables.RaceTable.FindValue (3, Race) )
	HasSubRaces = SubRacesTable.GetValue (RaceID, "PURE_RACE")
	if HasSubRaces == 0:
		TextAreaControl.SetText (CommonTables.RaceTable.GetValue (RaceID,"DESC_REF") )
		DoneButton.SetState (IE_GUI_BUTTON_ENABLED)
		return
	if RaceWindow:
		RaceWindow.Unload ()
	GemRB.SetNextScript ("SubRaces")
	return

def BackPress():
	if RaceWindow:
		RaceWindow.Unload ()
	GemRB.SetNextScript ("CharGen2")
	GemRB.SetVar ("BaseRace",0)  #scrapping the race value
	return

def NextPress():
	if RaceWindow:
		RaceWindow.Unload ()
	GemRB.SetNextScript ("CharGen3") #class
	return
