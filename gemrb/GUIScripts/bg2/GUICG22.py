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
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
#
# $Id$
#character generation, class kit (GUICG22)

import GemRB

KitWindow = 0
TextAreaControl = 0
DoneButton = 0
KitList = 0
ClassList = 0
SchoolList = 0
ClassID = 0

def OnLoad():
	global KitWindow, TextAreaControl, DoneButton
	global KitList, ClassList, SchoolList, ClassID
	
	GemRB.LoadWindowPack("GUICG", 640, 480)
	TmpTable = GemRB.LoadTableObject("races")
	RaceName = TmpTable.GetRowName(GemRB.GetVar("Race")-1 )
	TmpTable = GemRB.LoadTableObject("classes")
	Class = GemRB.GetVar("Class")-1
	ClassName = TmpTable.GetRowName(Class)
	ClassID = TmpTable.GetValue(Class, 5)
	ClassList = GemRB.LoadTableObject("classes")
	KitTable = GemRB.LoadTableObject("kittable")
	KitTableName = KitTable.GetValue(ClassName, RaceName)
	KitTable = GemRB.LoadTableObject(KitTableName,1)

	KitList = GemRB.LoadTableObject("kitlist")
	SchoolList = GemRB.LoadTableObject("magesch")

	#there is a specialist mage window, but it is easier to use 
	#the class kit window for both
	KitWindow = GemRB.LoadWindowObject(22)
	if ClassID == 1:
		Label = KitWindow.GetControl(0xfffffff)
		Label.SetText(595)

	for i in range(10):
		if i<4:
			Button = KitWindow.GetControl(i+1)
		else:
			Button = KitWindow.GetControl(i+5)
		Button.SetState(IE_GUI_BUTTON_DISABLED)
		Button.SetFlags(IE_GUI_BUTTON_RADIOBUTTON, OP_OR)

	if not KitTable:
		RowCount = 1
	else:
		RowCount = KitTable.GetRowCount()

	for i in range(RowCount):
		if i<4:
			Button = KitWindow.GetControl(i+1)
		else:
			Button = KitWindow.GetControl(i+5)

		if not KitTable:
			if ClassID == 1:
				Kit = GemRB.GetVar("MAGESCHOOL")
				KitName = SchoolList.GetValue(i, 0)
				Kit = SchoolList.GetValue (Kit, 3)
			else:
				Kit = 0
				KitName = ClassList.GetValue(GemRB.GetVar("Class")-1, 0)

		else:
			Kit = KitTable.GetValue(i,0)
			if Kit:
				KitName = KitList.GetValue(Kit, 1)
			else:
				KitName = ClassList.GetValue(GemRB.GetVar("Class")-1, 0)

		Button.SetState(IE_GUI_BUTTON_ENABLED)
		Button.SetText(KitName)
		if i==0:
			GemRB.SetVar("Class Kit",Kit)
		Button.SetVarAssoc("Class Kit",Kit)
		Button.SetEvent(IE_GUI_BUTTON_ON_PRESS, "KitPress")

	BackButton = KitWindow.GetControl(8)
	BackButton.SetText(15416)
	DoneButton = KitWindow.GetControl(7)
	DoneButton.SetText(11973)
	DoneButton.SetFlags(IE_GUI_BUTTON_DEFAULT,OP_OR)

	TextAreaControl = KitWindow.GetControl(5)
	TextAreaControl.SetText(17247)

	DoneButton.SetEvent(IE_GUI_BUTTON_ON_PRESS,"NextPress")
	BackButton.SetEvent(IE_GUI_BUTTON_ON_PRESS,"BackPress")
	KitPress()
	KitWindow.SetVisible(1)
	return

def KitPress():
	global ClassID

	Kit = GemRB.GetVar("Class Kit")
	if ClassID == 1 and Kit != 0:
		GemRB.SetVar("MAGESCHOOL", Kit-21) # hack: -21 to make the generalist 0
	else:
		GemRB.SetVar("MAGESCHOOL", 0) # so bards don't get schools
		
	if Kit == 0:
		KitName = ClassList.GetValue(GemRB.GetVar("Class")-1, 1)
	else:
		KitName = KitList.GetValue(Kit, 3)
	TextAreaControl.SetText(KitName)
	DoneButton.SetState(IE_GUI_BUTTON_ENABLED)
	return

def BackPress():
	GemRB.SetVar("Class Kit",0) #scrapping
	if KitWindow:
		KitWindow.Unload()
	GemRB.SetNextScript("GUICG2")
	return

def NextPress():
	if KitWindow:
		KitWindow.Unload()
	GemRB.SetNextScript("CharGen4") #abilities
	return
