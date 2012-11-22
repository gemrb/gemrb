# -*-python-*-
# GemRB - Infinity Engine Emulator
# Copyright (C) 2003-2007 The GemRB Project
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
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#
#character generation, class kit (GUICG22)

import GemRB
import CommonTables
from ie_stats import *
from GUIDefines import *

KitWindow = 0
TextAreaControl = 0
DoneButton = 0
SchoolList = 0
ClassName = 0
TopIndex = 0
RowCount = 10
KitTable = 0
Init = 0
MyChar = 0
KitSelected = 0 #store clicked kit on redraw as number within RowCount
EnhanceGUI = GemRB.GetVar("GUIEnhancements")&GE_SCROLLBARS #extra kit button and scroll bar toggle

def OnLoad():
	global KitWindow, TextAreaControl, DoneButton
	global SchoolList, ClassName
	global RowCount, TopIndex, KitTable, Init, MyChar

	GemRB.LoadWindowPack("GUICG", 640, 480)
	MyChar = GemRB.GetVar ("Slot")
	Race = GemRB.GetPlayerStat (MyChar, IE_RACE)
	RaceName = CommonTables.Races.GetRowName(CommonTables.Races.FindValue (3, Race) )

	ClassName = CommonTables.Classes.GetRowName (GemRB.GetPlayerStat (MyChar, IE_HITPOINTS)) # barbarian hack

	KitTable = GemRB.LoadTable("kittable")
	KitTableName = KitTable.GetValue(ClassName, RaceName)
	KitTable = GemRB.LoadTable(KitTableName,1)

	SchoolList = GemRB.LoadTable("magesch")

	#there is a specialist mage window, but it is easier to use
	#the class kit window for both
	KitWindow = GemRB.LoadWindow(22)
	if ClassName == "MAGE":
		Label = KitWindow.GetControl(0xfffffff)
		Label.SetText(595)

	for i in range(10):
		if i<4:
			Button = KitWindow.GetControl(i+1)
		else:
			Button = KitWindow.GetControl(i+5)
		Button.SetState(IE_GUI_BUTTON_DISABLED)
		Button.SetFlags(IE_GUI_BUTTON_RADIOBUTTON, OP_OR)

	if not KitTable: # sorcerer or monk
		RowCount = 1
	else:
		if ClassName == "MAGE": # mages
			RowCount = SchoolList.GetRowCount()
		else:
			RowCount = KitTable.GetRowCount()

	TopIndex = 0
	GemRB.SetVar("TopIndex", 0)
	if EnhanceGUI:
		tmpRowCount = RowCount
		if RowCount>10: #create 11 kit button
			KitWindow.CreateButton (15, 18, 250, 271, 20)
			extrakit = KitWindow.GetControl(15)
			extrakit.SetState(IE_GUI_BUTTON_DISABLED)
			extrakit.SetFlags(IE_GUI_BUTTON_RADIOBUTTON, OP_OR)
			extrakit.SetSprites("GUICGBC",0, 0,1,2,3)
			RowCount = 11
		if tmpRowCount>11: #create scroll bar
			KitWindow.CreateScrollBar(1000, 290, 50, 16, 220)
			ScrollBar = KitWindow.GetControl (1000)
			ScrollBar.SetSprites("GUISCRCW", 0, 0,1,2,3,5,4)
			ScrollBar.SetVarAssoc("TopIndex",tmpRowCount-10)
			ScrollBar.SetEvent(IE_GUI_SCROLLBAR_ON_CHANGE, RedrawKits)
			ScrollBar.SetDefaultScrollBar()
	elif not EnhanceGUI and RowCount>10:
		RowCount = 10

	for i in range(RowCount):
		if i<4:
			Button = KitWindow.GetControl(i+1)
		else:
			Button = KitWindow.GetControl(i+5)
		Button.SetVarAssoc("ButtonPressed", i)
		Button.SetEvent(IE_GUI_BUTTON_ON_PRESS, KitPress)

	BackButton = KitWindow.GetControl(8)
	BackButton.SetText(15416)
	BackButton.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)
	DoneButton = KitWindow.GetControl(7)
	DoneButton.SetText(11973)
	DoneButton.SetFlags(IE_GUI_BUTTON_DEFAULT,OP_OR)

	TextAreaControl = KitWindow.GetControl(5)
	TextAreaControl.SetText(17247)

	DoneButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, NextPress)
	BackButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, BackPress)
	Init = 1
	RedrawKits()
	KitPress()
	KitWindow.SetVisible(WINDOW_VISIBLE)
	return

def RedrawKits():
	global TopIndex, Init, KitSelected

	TopIndex=GemRB.GetVar("TopIndex")
	EnabledButtons = []
	for i in range(RowCount):
		if i<4:
			Button = KitWindow.GetControl(i+1)
		else:
			Button = KitWindow.GetControl(i+5)
		Button.SetState(IE_GUI_BUTTON_DISABLED)
		if not KitTable:
			if ClassName == "MAGE":
				# TODO: check if this is ever reached
				Kit = GemRB.GetVar("MAGESCHOOL")
				KitName = SchoolList.GetValue(i+TopIndex, 0)
				Kit = SchoolList.GetValue (Kit, 3)
			else:
				Kit = 0
				KitName = CommonTables.Classes.GetValue (ClassName, "NAME_REF")
		else:
			Kit = KitTable.GetValue (i+TopIndex,0)
			if ClassName == "MAGE":
				KitName = SchoolList.GetValue (i+TopIndex, 0)
				if Kit == 0:
					KitName = SchoolList.GetValue (0, 0)
					Button.SetState(IE_GUI_BUTTON_ENABLED)
					if Init: #preselection of mage plain kit
						Button.SetState(IE_GUI_BUTTON_SELECTED)
						KitSelected = i+TopIndex
						Init=0
				if Kit != "*":
					EnabledButtons.append(Kit-21)
			else:
				if Kit:
					KitName = CommonTables.KitList.GetValue(Kit, 1)
				else:
					KitName = CommonTables.Classes.GetValue (ClassName, "NAME_REF")
		Button.SetText(KitName)
		if not EnabledButtons or i+TopIndex in EnabledButtons:
			Button.SetState(IE_GUI_BUTTON_ENABLED)
			if Init and i+TopIndex>0:
				Button.SetState(IE_GUI_BUTTON_SELECTED)
				KitSelected = i+TopIndex
				Init=0
		if Kit == "*":
			continue
		if Init and i+TopIndex==0:
			if EnabledButtons:
				GemRB.SetVar("ButtonPressed", EnabledButtons[0]) #but leave Init==1
			else:
				GemRB.SetVar("ButtonPressed", 0)
				Button.SetState(IE_GUI_BUTTON_SELECTED) 
				KitSelected = i+TopIndex
				Init=0
		if not Init and i+TopIndex == KitSelected: #remark selection state on redraw
			Button.SetState(IE_GUI_BUTTON_SELECTED)
	return

def KitPress():
	global KitSelected

	ButtonPressed=GemRB.GetVar("ButtonPressed")
	KitSelected = ButtonPressed + TopIndex
	if not KitTable:
		if ClassName == "MAGE":
			# TODO: this seems to be never reached
			Kit = GemRB.GetVar("MAGESCHOOL")
			Kit = SchoolList.GetValue (Kit, 3)
		else:
			Kit = 0
	else:
		Kit = KitTable.GetValue (ButtonPressed+TopIndex, 0)
		if ClassName == "MAGE":
			if ButtonPressed + TopIndex == 0:
				Kit = 0
			else:
				Kit = ButtonPressed + TopIndex + 21

	if ClassName == "MAGE" and Kit != 0:
		GemRB.SetVar("MAGESCHOOL", Kit-21) # hack: -21 to make the generalist 0
	else:
		GemRB.SetVar("MAGESCHOOL", 0) # so bards don't get schools

	if Kit == 0:
		KitDescription = CommonTables.Classes.GetValue (ClassName, "DESC_REF")
	else:
		KitDescription = CommonTables.KitList.GetValue(Kit, 3)

	TextAreaControl.SetText(KitDescription)
	DoneButton.SetState(IE_GUI_BUTTON_ENABLED)

	GemRB.SetVar("Class Kit", Kit)

	return

def BackPress():
	GemRB.SetVar("Class Kit", 0) # reverting the value so we are idempotent
	GemRB.SetVar("MAGESCHOOL", 0)
	if KitWindow:
		KitWindow.Unload()
	GemRB.SetNextScript("GUICG2")
	return

def NextPress():
	if KitWindow:
		KitWindow.Unload()

	#make gnomes always kitted
	KitIndex = GemRB.GetVar ("Class Kit")
	MageSchool = GemRB.GetVar ("MAGESCHOOL")
	if MageSchool and not KitIndex:
		SchoolTable = GemRB.LoadTable ("magesch")
		KitIndex = CommonTables.KitList.FindValue (6, SchoolTable.GetValue (MageSchool, 3) )

	#save the kit
	KitValue = (0x4000 + KitIndex)
	KitName = CommonTables.KitList.GetValue (KitIndex, 0)
	GemRB.SetPlayerStat (MyChar, IE_KIT, KitValue)

	GemRB.SetNextScript("CharGen4") #abilities
	return
