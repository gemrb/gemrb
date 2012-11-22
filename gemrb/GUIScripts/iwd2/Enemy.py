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
import GUICommon
import CommonTables
from GUIDefines import *

RaceWindow = 0
TextAreaControl = 0
DoneButton = 0
RacialEnemyTable = 0
RaceCount = 0
TopIndex = 0

def DisplayRaces():
	global TopIndex

	TopIndex=GemRB.GetVar("TopIndex")
	for i in range(11):
		Button = RaceWindow.GetControl(i+22)
		Val = RacialEnemyTable.GetValue(i+TopIndex,0)
		if Val==0:
			Button.SetText("")
			Button.SetState(IE_GUI_BUTTON_DISABLED)
		else:
			Button.SetText(Val)
			Button.SetState(IE_GUI_BUTTON_ENABLED)
			Button.SetEvent(IE_GUI_BUTTON_ON_PRESS, RacePress)
			Button.SetVarAssoc("HatedRace",RacialEnemyTable.GetValue(i+TopIndex,1) )
	return

def OnLoad():
	global RaceWindow, TextAreaControl, DoneButton
	global RacialEnemyTable, RaceCount, TopIndex
	
	GemRB.LoadWindowPack("GUICG", 800 ,600)
	RaceWindow = GemRB.LoadWindow(15)
	Class = GemRB.GetVar("BaseClass")
	ClassName = GUICommon.GetClassRowName (Class, "index")
	print "TODO: check if class %s really has an ID of %d" %(ClassName, Class)
	TableName = CommonTables.ClassSkills.GetValue(ClassName, "RANGERSKILL")
	if TableName == "*":
		GemRB.SetNextScript("Skills")
		return
	RacialEnemyTable = GemRB.LoadTable(TableName)
	RaceCount = RacialEnemyTable.GetRowCount()-11
	if RaceCount<0:
		RaceCount=0

	for i in range(11):
		Button = RaceWindow.GetControl(i+22)
		Button.SetFlags(IE_GUI_BUTTON_RADIOBUTTON,OP_OR)

	BackButton = RaceWindow.GetControl(10)
	BackButton.SetText(15416)
	BackButton.SetFlags(IE_GUI_BUTTON_CANCEL,OP_OR)

	DoneButton = RaceWindow.GetControl(11)
	DoneButton.SetText(11973)
	DoneButton.SetFlags(IE_GUI_BUTTON_DEFAULT,OP_OR)
	DoneButton.SetState(IE_GUI_BUTTON_DISABLED)

	TextAreaControl = RaceWindow.GetControl(8)
	TextAreaControl.SetText(17256)
	TopIndex = 0
	GemRB.SetVar("TopIndex",0)
	ScrollBarControl = RaceWindow.GetControl(1)
	ScrollBarControl.SetVarAssoc("TopIndex",RaceCount)
	ScrollBarControl.SetEvent(IE_GUI_SCROLLBAR_ON_CHANGE, DisplayRaces)

	DoneButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, NextPress)
	BackButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, BackPress)
	RaceWindow.SetVisible(WINDOW_VISIBLE)
	DisplayRaces()
	return

def RacePress():
	Race = GemRB.GetVar("HatedRace")
	Row = RacialEnemyTable.FindValue(1, Race)
	TextAreaControl.SetText(RacialEnemyTable.GetValue(Row, 2) )
	DoneButton.SetState(IE_GUI_BUTTON_ENABLED)
	return

def BackPress():
	if RaceWindow:
		RaceWindow.Unload()
	GemRB.SetVar("HatedRace",0)  #scrapping the race value
	GemRB.SetNextScript("CharGen6")
	return

def NextPress():
	if RaceWindow:
		RaceWindow.Unload()
	GemRB.SetNextScript("Skills")
	return
