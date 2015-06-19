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
#character generation, SubRaces (GUICG54)
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
	
	GemRB.LoadWindowPack("GUICG", 800, 600)
	RaceWindow = GemRB.LoadWindow(54)

	RaceCount = CommonTables.Races.GetRowCount()
	
	SubRacesTable = GemRB.LoadTable("SUBRACES")

	for i in range(1, 5):
		Button = RaceWindow.GetControl(i)
		Button.SetFlags(IE_GUI_BUTTON_RADIOBUTTON,OP_OR)
		
	Race = GemRB.GetVar("BaseRace")
	RaceName = CommonTables.Races.GetRowName(CommonTables.Races.FindValue(3, Race) )

	PureRace = SubRacesTable.GetValue(RaceName , "PURE_RACE")
	Button = RaceWindow.GetControl(1)
	RaceStrRef = CommonTables.Races.GetValue(PureRace, "CAP_REF")
	Button.SetText(RaceStrRef )
	Button.SetState(IE_GUI_BUTTON_ENABLED)
	Button.SetEvent(IE_GUI_BUTTON_ON_PRESS, SubRacePress)
	RaceID = CommonTables.Races.GetValue(PureRace, "ID")
	Button.SetVarAssoc("Race",RaceID)
	
	#if you want a fourth subrace you can safely add a control id #5 to
	#the appropriate window (guicg#54), and increase 4 to 5
	for i in range(1,4):
		Label = "SUBRACE"+str(i)
		HasSubRace = SubRacesTable.GetValue(RaceName, Label)
		Button = RaceWindow.GetControl(i+1)
		if HasSubRace == 0:
			Button.SetState(IE_GUI_BUTTON_DISABLED)
			Button.SetFlags(IE_GUI_BUTTON_NO_IMAGE, OP_OR)
			Button.SetText("")
		else:
			HasSubRace = PureRace+"_"+HasSubRace
			RaceStrRef = CommonTables.Races.GetValue(HasSubRace, "CAP_REF")
			Button.SetText(RaceStrRef )
			Button.SetState(IE_GUI_BUTTON_ENABLED)
			Button.SetEvent(IE_GUI_BUTTON_ON_PRESS, SubRacePress)
			RaceID = CommonTables.Races.GetValue(HasSubRace, "ID")
			Button.SetVarAssoc("Race",RaceID)

	BackButton = RaceWindow.GetControl(8) 
	BackButton.SetText(15416)
	BackButton.SetFlags(IE_GUI_BUTTON_CANCEL,OP_OR)

	DoneButton = RaceWindow.GetControl(0)
	DoneButton.SetText(36789)
	DoneButton.SetState(IE_GUI_BUTTON_ENABLED)
	DoneButton.SetFlags(IE_GUI_BUTTON_DEFAULT,OP_OR)

	TextAreaControl = RaceWindow.GetControl(6)
	TextAreaControl.SetText(CommonTables.Races.GetValue(RaceName, "DESC_REF"))

	DoneButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, NextPress)
	BackButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, BackPress)
	RaceWindow.Focus()
	return

def SubRacePress():
	global RaceWindow, TextAreaControl
	Race = CommonTables.Races.FindValue(3, GemRB.GetVar("Race") )
	TextAreaControl.SetText(CommonTables.Races.GetValue(Race, 1))
	return

def BackPress():
	if RaceWindow:
		RaceWindow.Unload()
	GemRB.SetNextScript("Race")
	GemRB.SetVar("Race",0)  #scrapping the subrace value
	GemRB.SetVar("BaseRace",0)  #scrapping the race value
	return

def NextPress():
	if RaceWindow:
		RaceWindow.Unload()
	GemRB.SetNextScript("CharGen3") #class
	return
