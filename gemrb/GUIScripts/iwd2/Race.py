# SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

#character generation, race (GUICG2)
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
	
	RaceWindow = GemRB.LoadWindow (8, "GUICG")
	CharOverview.PositionCharGenWin(RaceWindow)

	SubRacesTable = GemRB.LoadTable ("SUBRACES")

	for i in range(2,9):
		Button = RaceWindow.GetControl (i)
		Button.SetFlags (IE_GUI_BUTTON_RADIOBUTTON,OP_OR)
		
	for i in range(7):
		Button = RaceWindow.GetControl (i+2)
		RaceName = CommonTables.Races.GetRowName (i)
		Button.SetText (CommonTables.Races.GetValue (RaceName, "NAME"))
		Button.SetState (IE_GUI_BUTTON_ENABLED)
		Button.OnPress (RacePress)
		Button.SetVarAssoc ("BaseRace", CommonTables.Races.GetValue (RaceName, "ID", GTV_INT))

	BackButton = RaceWindow.GetControl (11) 
	BackButton.SetText (15416)
	BackButton.MakeEscape()

	DoneButton = RaceWindow.GetControl (0)
	DoneButton.SetText (36789)
	DoneButton.SetState (IE_GUI_BUTTON_DISABLED)
	DoneButton.MakeDefault()

	TextAreaControl = RaceWindow.GetControl (9)
	TextAreaControl.SetText (17237)
	RaceWindow.SetEventProxy (TextAreaControl)

	DoneButton.OnPress (NextPress)
	BackButton.OnPress (BackPress)
	RaceWindow.Focus()
	return

def RacePress():
	global RaceWindow, SubRacesTable
	Race = GemRB.GetVar ("BaseRace")
	GemRB.SetVar ("Race", Race)
	RaceID = GUICommon.GetRaceRowName (GemRB.GetVar ("Slot"), Race)
	HasSubRaces = SubRacesTable.GetValue (RaceID, "PURE_RACE")
	if HasSubRaces == 0:
		TextAreaControl.SetText (CommonTables.Races.GetValue (RaceID, "DESCSTR"))
		DoneButton.SetState (IE_GUI_BUTTON_ENABLED)
		return
	if RaceWindow:
		RaceWindow.Close ()
	GemRB.SetNextScript ("SubRaces")
	return

def BackPress():
	if RaceWindow:
		RaceWindow.Close ()
	GemRB.SetNextScript ("CharGen2")
	GemRB.SetVar ("BaseRace",0)  #scrapping the race value
	return

def NextPress():
	if RaceWindow:
		RaceWindow.Close ()
	GemRB.SetNextScript ("CharGen3") #class
	return
