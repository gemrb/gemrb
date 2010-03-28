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

from CharGenCommon import * 
from GUICommon import CloseOtherWindow, ClassSkillsTable


RaceWindow = 0
TextAreaControl = 0
DoneButton = 0
RaceTable = 0
RaceCount = 0
TopIndex = 0
#the size of the selection list
LISTSIZE = 6

def DisplayRaces():
	global TopIndex

	TopIndex=GemRB.GetVar("TopIndex")
	for i in range(LISTSIZE):
		Button = RaceWindow.GetControl(i+2)
		Val = RaceTable.GetValue(i+TopIndex,0)
		if Val==0:
			Button.SetText("")
			Button.SetState(IE_GUI_BUTTON_DISABLED)
		else:
			Button.SetText(Val)
			Button.SetState(IE_GUI_BUTTON_ENABLED)
			Button.SetEvent(IE_GUI_BUTTON_ON_PRESS,"RacePress")
			Button.SetVarAssoc("HatedRace",RaceTable.GetValue(i+TopIndex,1) )
	return

def OnLoad():
	global RaceWindow, TextAreaControl, DoneButton
	global RaceTable, RaceCount, TopIndex

	if CloseOtherWindow (OnLoad):
		if(RaceWindow):
			RaceWindow.Unload()
			RaceWindow = None
		return
	
	GemRB.SetVar ("HatedRace",0)
	
	ClassTable = GemRB.LoadTableObject("classes")
	ClassRow = GemRB.GetVar("Class")-1
	Class = ClassTable.GetValue(ClassRow, 5)
	ClassName = ClassSkillsTable.GetRowName(Class)
	TableName = ClassSkillsTable.GetValue(ClassName, "HATERACE")
	
	GemRB.LoadWindowPack("GUICG")
	RaceWindow = GemRB.LoadWindowObject(15)
	RaceTable = GemRB.LoadTableObject(TableName)
	RaceCount = RaceTable.GetRowCount()-LISTSIZE+1
	if RaceCount<0:
		RaceCount=0

	for i in range(LISTSIZE):
		Button = RaceWindow.GetControl(i+2)
		Button.SetFlags(IE_GUI_BUTTON_RADIOBUTTON,OP_OR)
		Button.SetSprites("GUIHRC",i,0,1,2,3)

	BackButton = RaceWindow.GetControl(10)
	BackButton.SetText(15416)
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
	ScrollBarControl.SetEvent(IE_GUI_SCROLLBAR_ON_CHANGE, "DisplayRaces")

	DoneButton.SetEvent(IE_GUI_BUTTON_ON_PRESS,"NextPress")
	BackButton.SetEvent(IE_GUI_BUTTON_ON_PRESS,"BackPress")
	RaceWindow.ShowModal(MODAL_SHADOW_NONE)
	DisplayRaces()
	return

def RacePress():
	Race = GemRB.GetVar("HatedRace")
 	Row = RaceTable.FindValue(1, Race)
	TextAreaControl.SetText(RaceTable.GetValue(Row, 2) )
	DoneButton.SetState(IE_GUI_BUTTON_ENABLED)
	return

def NextPress():
	MyChar = GemRB.GetVar ("Slot")
	GemRB.SetPlayerStat (MyChar, IE_HATEDRACE, GemRB.GetVar ("HatedRace") )
	next()
