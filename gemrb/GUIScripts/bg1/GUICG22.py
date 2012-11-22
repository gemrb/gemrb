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
#character generation, class kit (GUICG22)
import GemRB
from GUIDefines import *
from ie_stats import *
import CharGenCommon
import GUICommon
import CommonTables


KitWindow = 0
TextAreaControl = 0
DoneButton = 0
SchoolList = 0
ClassName = 0

def OnLoad():
	global KitWindow, TextAreaControl, DoneButton
	global SchoolList, ClassName

	if GUICommon.CloseOtherWindow(OnLoad):
		if(KitWindow):
			KitWindow.Unload()
			KitWindow = None
		return

	GemRB.LoadWindowPack("GUICG", 640, 480)
	RaceName = CommonTables.Races.GetRowName(GemRB.GetVar("Race")-1 )
	ClassName = GUICommon.GetClassRowName (GemRB.GetVar ("Class")-1, "index")
	KitTable = GemRB.LoadTable("kittable")
	KitTableName = KitTable.GetValue(ClassName, RaceName)
	KitTable = GemRB.LoadTable(KitTableName,1)

	SchoolList = GemRB.LoadTable("magesch")

	#there is only a specialist mage window for bg1
	KitWindow = GemRB.LoadWindow(12)

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
			if ClassName == "MAGE":
				Kit=GemRB.GetVar("MAGESCHOOL")
				KitName = SchoolList.GetValue(i, 0)
			else:
				Kit = 0
				KitName = CommonTables.Classes.GetValue(ClassName, "NAME_REF")

		else:
			Kit = KitTable.GetValue(i,0)
			if ClassName == "MAGE":
				if Kit:
					Kit = Kit - 21
				KitName = SchoolList.GetValue(Kit, 0)
			else:
				if Kit:
					KitName = CommonTables.KitList.GetValue(Kit, 1)
				else:
					KitName = CommonTables.Classes.GetValue (ClassName, "NAME_REF")

		Button.SetState(IE_GUI_BUTTON_ENABLED)
		Button.SetText(KitName)
		Button.SetVarAssoc("Class Kit",Kit)
		if i==0:
			GemRB.SetVar("Class Kit",Kit)
		Button.SetEvent(IE_GUI_BUTTON_ON_PRESS, KitPress)

	BackButton = KitWindow.GetControl(12)
	BackButton.SetText(15416)
	DoneButton = KitWindow.GetControl(0)
	DoneButton.SetText(11973)
	DoneButton.SetFlags(IE_GUI_BUTTON_DEFAULT,OP_OR)

	TextAreaControl = KitWindow.GetControl(11)
	TextAreaControl.SetText(17247)

	DoneButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, NextPress)
	BackButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, CharGenCommon.BackPress)
	#KitPress()
	KitWindow.ShowModal(MODAL_SHADOW_NONE)
	return

def KitPress():
	Kit = GemRB.GetVar("Class Kit")
	if Kit == 0:
		KitName = CommonTables.Classes.GetValue(ClassName, "DESC_REF")
	else:
		if ClassName == "MAGE":
			KitName = SchoolList.GetValue(Kit, 1)
		else:
			KitName = CommonTables.KitList.GetValue(Kit, 3)
	TextAreaControl.SetText(KitName)
	DoneButton.SetState(IE_GUI_BUTTON_ENABLED)
	return

def NextPress():
	#class	
	Class = CommonTables.Classes.GetValue (ClassName, "ID")
	MyChar = GemRB.GetVar ("Slot")
	GemRB.SetPlayerStat (MyChar, IE_CLASS, Class)
	KitIndex = GemRB.GetVar ("Class Kit")
	if ClassName == "MAGE":
		GemRB.SetVar("MAGESCHOOL", KitIndex)
	#the same as the unusable field
	Kit = CommonTables.KitList.GetValue(KitIndex, 6)
	GemRB.SetPlayerStat (MyChar, IE_KIT, Kit)
	CharGenCommon.next()
