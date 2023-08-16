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
#character generation, racial enemy (GUICG15)
import GemRB
import CharGenCommon
import CommonTables
import GUICommon
from GUIDefines import *
from ie_stats import *

RaceWindow = 0
TextAreaControl = 0
DoneButton = 0
RaceTable = 0
RaceCount = 0
TopIndex = 0
MyChar = 0
#the size of the selection list
LISTSIZE = 11

def DisplayRaces():
	global TopIndex

	TopIndex=GemRB.GetVar("TopIndex")
	for i in range(LISTSIZE):
		Button = RaceWindow.GetControl(i+6)
		Val = RaceTable.GetValue(i+TopIndex,0)
		if Val==0:
			Button.SetText("")
			Button.SetDisabled(True)
		else:
			Button.SetText(Val)
			Button.SetDisabled(False)
			Button.OnPress (RacePress)
			Button.SetVarAssoc("HatedRace",RaceTable.GetValue(i+TopIndex,1) )
	return

def OnLoad():
	global RaceWindow, TextAreaControl, DoneButton
	global RaceTable, RaceCount, TopIndex, MyChar

	MyChar = GemRB.GetVar ("Slot")
	ClassName = GUICommon.GetClassRowName (MyChar)
	TableName = CommonTables.ClassSkills.GetValue(ClassName, "HATERACE")
	if TableName == "*":
		GemRB.SetNextScript("GUICG7")
		return
	RaceWindow = GemRB.LoadWindow(15, "GUICG")
	CharGenCommon.PositionCharGenWin (RaceWindow)
	RaceTable = GemRB.LoadTable(TableName)
	RaceCount = RaceTable.GetRowCount()-LISTSIZE
	if RaceCount<0:
		RaceCount=0

	TopIndex = 0
	GemRB.SetVar("TopIndex", 0)
	ScrollBarControl = RaceWindow.GetControl(1)
	ScrollBarControl.SetVarAssoc("TopIndex", RaceCount)
	ScrollBarControl.OnChange (DisplayRaces)
	RaceWindow.SetEventProxy(ScrollBarControl)

	for i in range(LISTSIZE):
		Button = RaceWindow.GetControl(i+6)
		Button.SetFlags(IE_GUI_BUTTON_RADIOBUTTON,OP_OR)

	GemRB.SetVar("HatedRace",0)

	BackButton = RaceWindow.GetControl(4)
	BackButton.SetText(15416)
	BackButton.MakeEscape()
	DoneButton = RaceWindow.GetControl(5)
	DoneButton.SetText(11973)
	DoneButton.MakeDefault()
	DoneButton.SetState(IE_GUI_BUTTON_DISABLED)

	TextAreaControl = RaceWindow.GetControl(2)
	TextAreaControl.SetText(17256)

	DoneButton.OnPress (NextPress)
	BackButton.OnPress (BackPress)
	RaceWindow.Focus()
	DisplayRaces()
	return

def RacePress():
	Race = GemRB.GetVar("HatedRace") or 0
	Row = RaceTable.FindValue(1, Race)
	TextAreaControl.SetText(RaceTable.GetValue(Row, 2) )
	DoneButton.SetState(IE_GUI_BUTTON_ENABLED)
	return

def BackPress():
	if RaceWindow:
		RaceWindow.Close ()
	GemRB.SetPlayerStat (MyChar, IE_HATEDRACE, 0) #scrapping the race value
	GemRB.SetNextScript("CharGen6")
	return

def NextPress():
	if RaceWindow:
		RaceWindow.Close ()
	# save the hated race
	GemRB.SetPlayerStat (MyChar, IE_HATEDRACE, GemRB.GetVar ("HatedRace"))

	GemRB.SetNextScript("GUICG7") #mage spells
	return
