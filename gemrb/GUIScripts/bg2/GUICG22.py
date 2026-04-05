# SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

#character generation, class kit (GUICG22)

import GemRB
import CommonTables
import GameCheck
import GUICommon
from ie_stats import *
from GUIDefines import *

import CharGenCommon

KitWindow = 0
TextAreaControl = 0
DoneButton = 0
SchoolList = 0
ClassName = 0
RaceName = 0
TopIndex = 0
RowCount = 10
KitTable = 0
Init = 0
MyChar = 0
KitSelected = 0 #store clicked kit on redraw as number within RowCount
ClassRaceTable = GemRB.LoadTable ("clsrcreq", False, True)

def OnLoad():
	global KitWindow, TextAreaControl, DoneButton
	global SchoolList, ClassName, RaceName
	global RowCount, TopIndex, KitTable, Init, MyChar

	MyChar = GemRB.GetVar ("Slot")
	RaceName = GUICommon.GetRaceRowName (MyChar)

	ClassName = CommonTables.Classes.GetRowName (GemRB.GetPlayerStat (MyChar, IE_HITPOINTS)) # barbarian hack

	KitTable = GemRB.LoadTable("kittable")
	KitTableName = KitTable.GetValue(ClassName, RaceName)
	KitTable = GemRB.LoadTable(KitTableName,1)

	SchoolList = GemRB.LoadTable("magesch")

	#there is a specialist mage window, but it is easier to use
	#the class kit window for both
	KitWindow = GemRB.LoadWindow(22, "GUICG")
	CharGenCommon.PositionCharGenWin(KitWindow)
	
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

	tmpRowCount = RowCount
	if RowCount > 10: # create 11th kit button
		extrakit = KitWindow.CreateButton (15, 18, 250, 271, 20)
		extrakit.SetState (IE_GUI_BUTTON_DISABLED)
		extrakit.SetFlags (IE_GUI_BUTTON_RADIOBUTTON | IE_GUI_BUTTON_CAPS, OP_OR)
		extrakit.SetSprites ("GUICGBC", 0, 0, 1, 2, 3)
		RowCount = 11

	if tmpRowCount > 11: # create scrollbar
		ScrollBar = KitWindow.CreateScrollBar (1000, {'x' : 290, 'y' : 50, 'w' : 16, 'h' : 220}, "GUISCRCW")
		ScrollBar.SetVarAssoc ("TopIndex", tmpRowCount - 10, 0, tmpRowCount - 10)
		ScrollBar.OnChange (RedrawKits)
		KitWindow.SetEventProxy (ScrollBar)

	for i in range(RowCount):
		if i<4:
			Button = KitWindow.GetControl(i+1)
		else:
			Button = KitWindow.GetControl(i+5)
		Button.SetVarAssoc("ButtonPressed", i)
		Button.OnPress (KitPress)

	BackButton = KitWindow.GetControl(8)
	BackButton.SetText(15416)
	BackButton.MakeEscape()
	DoneButton = KitWindow.GetControl(7)
	DoneButton.SetText(11973)
	DoneButton.MakeDefault()

	TextAreaControl = KitWindow.GetControl(5)
	TextAreaControl.SetText(17247)

	DoneButton.OnPress (NextPress)
	BackButton.OnPress (BackPress)
	Init = 1
	RedrawKits()
	KitPress()
	KitWindow.Focus()
	return

def RaceAllowsKit (KitName):
	CRKindex = ClassRaceTable.GetRowIndex (KitName)
	if CRKindex is None: # no kit entry, look at class
		CRKitName = ClassName
	else:
		CRKitName = KitName
	return ClassRaceTable.GetValue (CRKitName, RaceName, GTV_INT)

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
			KitIndex = 0
			KitName = CommonTables.ClassText.GetValue (ClassName, "LOWER", GTV_REF)
		else:
			KitIndex = KitTable.GetValue (i + TopIndex, 0)
			if ClassName == "MAGE":
				KitName = SchoolList.GetValue (i + TopIndex, 0, GTV_REF)
				if KitIndex == 0:
					KitName = SchoolList.GetValue ("GENERALIST", "NAME_REF", GTV_REF)
					Button.SetState(IE_GUI_BUTTON_ENABLED)
					if Init: #preselection of mage plain kit
						Button.SetState(IE_GUI_BUTTON_SELECTED)
						KitSelected = i+TopIndex
						Init=0
				if KitIndex != "*" and RaceAllowsKit (KitName):
					EnabledButtons.append (KitIndex - 21)
			else:
				if KitIndex and KitIndex != "*":
					KitName = CommonTables.KitList.GetValue (KitIndex, 1, GTV_REF)
				else:
					KitName = CommonTables.ClassText.GetValue (ClassName, "LOWER", GTV_REF)
		Button.SetText(KitName)
		if (not EnabledButtons and RaceAllowsKit (KitName)) or i+TopIndex in EnabledButtons:
			Button.SetState(IE_GUI_BUTTON_ENABLED)
			if Init and i+TopIndex>0:
				Button.SetState(IE_GUI_BUTTON_SELECTED)
				KitSelected = i+TopIndex
				Init=0
		if KitIndex == "*":
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
		KitIndex = 0
	else:
		KitIndex = KitTable.GetValue (ButtonPressed + TopIndex, 0)
		if ClassName == "MAGE":
			if ButtonPressed + TopIndex == 0:
				KitIndex = 0
			else:
				KitIndex = ButtonPressed + TopIndex + 21

	if ClassName == "MAGE" and KitIndex != 0:
		GemRB.SetVar ("MAGESCHOOL", KitIndex - 21) # hack: -21 to make the generalist 0
	else:
		GemRB.SetVar("MAGESCHOOL", 0) # so bards don't get schools

	if KitIndex == 0:
		KitDescription = CommonTables.ClassText.GetValue (ClassName, "DESCSTR")
	else:
		KitDescription = CommonTables.KitList.GetValue (KitIndex, 3)

	TextAreaControl.SetText(KitDescription)
	DoneButton.SetState(IE_GUI_BUTTON_ENABLED)

	GemRB.SetVar ("Class Kit", KitIndex)

	return

def BackPress():
	GemRB.SetVar("Class Kit", 0) # reverting the value so we are idempotent
	GemRB.SetVar("MAGESCHOOL", 0)
	if KitWindow:
		KitWindow.Close ()
	GemRB.SetNextScript("GUICG2")
	return

def NextPress():
	if KitWindow:
		KitWindow.Close ()

	#make gnomes always kitted
	KitIndex = GemRB.GetVar ("Class Kit")
	MageSchool = GemRB.GetVar ("MAGESCHOOL")
	if MageSchool and not KitIndex:
		KitValue = SchoolList.GetValue (MageSchool, 3)
	elif KitIndex:
		# ees added another column, simplifying the mage mess
		# all new kits rely on it
		KitRowName = CommonTables.KitList.GetRowName (KitIndex)
		if GameCheck.IsAnyEE ():
			KitValue = CommonTables.KitList.GetValue (KitRowName, "KITIDS")
		else:
			KitValue = CommonTables.KitList.GetValue (KitRowName, "UNUSABLE")
	else:
		KitValue = 0

	#save the kit
	GemRB.SetPlayerStat (MyChar, IE_KIT, KitValue)

	GemRB.SetNextScript("CharGen4") #abilities
	return
