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
# $Id$
#
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
	
	GemRB.LoadWindowPack("GUICG")
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

	#there is only a specialist mage window for bg1
	KitWindow = GemRB.LoadWindowObject(12)

	for i in range(8):
		Button = KitWindow.GetControl(i+2)
		Button.SetState(IE_GUI_BUTTON_DISABLED)
		Button.SetFlags(IE_GUI_BUTTON_RADIOBUTTON, OP_OR)

	if not KitTable:
		RowCount = 1
	else:
		RowCount = KitTable.GetRowCount()

	for i in range(RowCount):
		Button = KitWindow.GetControl(i+2)
		if not KitTable:
			if ClassID == 1:
				Kit=GemRB.GetVar("MAGESCHOOL")
				KitName = SchoolList.GetValue(i, 0)
			else:
				Kit = 0
				KitName = ClassList.GetValue(GemRB.GetVar("Class")-1, 0)

		else:
			Kit = KitTable.GetValue(i,0)
			if ClassID == 1:
				if Kit:
					Kit = Kit - 21
				KitName = SchoolList.GetValue(Kit, 0)
			else:
				if Kit:
					KitName = KitList.GetValue(Kit, 1)
				else:
					KitName = ClassList.GetValue(GemRB.GetVar("Class")-1, 0)

		Button.SetState(IE_GUI_BUTTON_ENABLED)
		Button.SetText(KitName)
		Button.SetVarAssoc("Class Kit",Kit)
		if i==0:
			GemRB.SetVar("Class Kit",Kit)
		Button.SetEvent(IE_GUI_BUTTON_ON_PRESS, "KitPress")

	BackButton = KitWindow.GetControl(12)
	BackButton.SetText(15416)
	DoneButton = KitWindow.GetControl(0)
	DoneButton.SetText(11973)
	DoneButton.SetFlags(IE_GUI_BUTTON_DEFAULT,OP_OR)

	TextAreaControl = KitWindow.GetControl(11)
	TextAreaControl.SetText(17247)

	DoneButton.SetEvent(IE_GUI_BUTTON_ON_PRESS,"NextPress")
	BackButton.SetEvent(IE_GUI_BUTTON_ON_PRESS,"BackPress")
	KitPress()
	KitWindow.SetVisible(1)
	return

def KitPress():
	Kit = GemRB.GetVar("Class Kit")
	if Kit == 0:
		KitName = ClassList.GetValue(GemRB.GetVar("Class")-1, 1)
	else:
		if ClassID==1:
			KitName = SchoolList.GetValue(Kit, 1)
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
