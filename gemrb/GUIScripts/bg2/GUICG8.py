# SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

#character generation, race (GUICG2)
import GemRB
import CommonTables
from ie_stats import IE_RACE
from GUIDefines import *

import CharGenCommon

RaceWindow = 0
TextAreaControl = 0
DoneButton = 0
MyChar = 0

def OnLoad():
	global RaceWindow, TextAreaControl, DoneButton, MyChar
	
	RaceWindow = GemRB.LoadWindow(8, "GUICG")
	CharGenCommon.PositionCharGenWin(RaceWindow)

	MyChar = GemRB.GetVar ("Slot")
	RaceCount = CommonTables.Races.GetRowCount()

	for i in range(2,RaceCount+2):
		#hack to stop if the race table has more entries than the gui resource
		#this needs to be done because the race table has non-selectable entries
		Button = RaceWindow.GetControl(i)
		if not Button:
			RaceCount = i-2
			break
		Button.SetFlags(IE_GUI_BUTTON_RADIOBUTTON,OP_OR)

	GemRB.SetVar ("Race", None)
	for i in range(2, RaceCount+2):
		Button = RaceWindow.GetControl(i)
		RaceName = CommonTables.Races.GetRowName (i - 2)
		Button.SetText (CommonTables.Races.GetValue (RaceName, "NAME"))
		Button.SetState(IE_GUI_BUTTON_ENABLED)
		Button.OnPress (RacePress)
		Button.SetVarAssoc("Race", i-2 )

	BackButton = RaceWindow.GetControl(i+2)  #i=8 now (when race count is 7)
	BackButton.SetText(15416)
	BackButton.MakeEscape()
	DoneButton = RaceWindow.GetControl(0)
	DoneButton.SetText(11973)
	DoneButton.MakeDefault()
	DoneButton.SetDisabled(True)

	TextAreaControl = RaceWindow.GetControl(12)
	TextAreaControl.SetText(17237)

	DoneButton.OnPress (NextPress)
	BackButton.OnPress (BackPress)
	RaceWindow.Focus()
	return

def RacePress():
	Race = GemRB.GetVar("Race")
	RaceName = CommonTables.Races.GetRowName (Race)
	TextAreaControl.SetText (CommonTables.Races.GetValue (RaceName, "DESCSTR"))
	DoneButton.SetDisabled(False)
	return

def BackPress():
	if RaceWindow:
		RaceWindow.Close ()
	GemRB.SetNextScript("CharGen2")
	GemRB.SetVar("Race",0)  #scrapping the race value
	return

def NextPress():
	if RaceWindow:
		RaceWindow.Close ()

	Race = GemRB.GetVar ("Race")
	RaceName = CommonTables.Races.GetRowName (Race)
	GemRB.SetPlayerStat (MyChar, IE_RACE, CommonTables.Races.GetValue (RaceName, "ID"))

	GemRB.SetNextScript("CharGen3") #class
	return
