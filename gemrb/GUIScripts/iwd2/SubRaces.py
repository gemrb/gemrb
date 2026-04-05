# SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

#character generation, SubRaces (GUICG54)
import GemRB
from GUIDefines import *
import CharOverview
import CommonTables
import GUICommon

RaceWindow = 0
TextAreaControl = 0
DoneButton = 0
SubRacesTable = 0

def OnLoad():
	global RaceWindow, TextAreaControl, DoneButton
	global SubRacesTable
	
	RaceWindow = GemRB.LoadWindow(54, "GUICG")
	CharOverview.PositionCharGenWin(RaceWindow)

	SubRacesTable = GemRB.LoadTable("SUBRACES")

	for i in range(1, 5):
		Button = RaceWindow.GetControl(i)
		Button.SetFlags(IE_GUI_BUTTON_RADIOBUTTON,OP_OR)

	RaceName = GUICommon.GetRaceRowName (GemRB.GetVar ("Slot"), GemRB.GetVar("BaseRace"))
	PureRace = SubRacesTable.GetValue(RaceName , "PURE_RACE")
	Button = RaceWindow.GetControl(1)
	RaceStrRef = CommonTables.Races.GetValue (PureRace, "UPPERCASE")
	Button.SetText(RaceStrRef )
	Button.SetState(IE_GUI_BUTTON_ENABLED)
	Button.OnPress (SubRacePress)
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
			RaceStrRef = CommonTables.Races.GetValue (HasSubRace, "UPPERCASE")
			Button.SetText(RaceStrRef )
			Button.SetState(IE_GUI_BUTTON_ENABLED)
			Button.OnPress (SubRacePress)
			RaceID = CommonTables.Races.GetValue(HasSubRace, "ID")
			Button.SetVarAssoc("Race",RaceID)

	BackButton = RaceWindow.GetControl(8) 
	BackButton.SetText(15416)
	BackButton.MakeEscape()

	DoneButton = RaceWindow.GetControl(0)
	DoneButton.SetText(36789)
	DoneButton.SetState(IE_GUI_BUTTON_ENABLED)
	DoneButton.MakeDefault()

	TextAreaControl = RaceWindow.GetControl(6)
	TextAreaControl.SetText(CommonTables.Races.GetValue(RaceName, "DESCSTR"))
	RaceWindow.SetEventProxy (TextAreaControl)

	DoneButton.OnPress (NextPress)
	BackButton.OnPress (BackPress)
	RaceWindow.Focus()
	return

def SubRacePress():
	global RaceWindow, TextAreaControl
	RaceName = GUICommon.GetRaceRowName (GemRB.GetVar ("Slot"), GemRB.GetVar("Race"))
	TextAreaControl.SetText (CommonTables.Races.GetValue (RaceName, "DESCSTR"))
	return

def BackPress():
	if RaceWindow:
		RaceWindow.Close ()
	GemRB.SetNextScript("Race")
	GemRB.SetVar("Race",0)  #scrapping the subrace value
	GemRB.SetVar("BaseRace",0)  #scrapping the race value
	return

def NextPress():
	if RaceWindow:
		RaceWindow.Close ()
	GemRB.SetNextScript("CharGen3") #class
	return
