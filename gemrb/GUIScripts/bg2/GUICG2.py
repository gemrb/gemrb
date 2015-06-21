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
#character generation, class (GUICG2)
import GemRB
import GameCheck
import GUICommon
import CommonTables
import LUCommon
from ie_stats import *
from GUIDefines import *

ClassWindow = 0
TextAreaControl = 0
DoneButton = 0
MyChar = 0

def OnLoad():
	global ClassWindow, TextAreaControl, DoneButton, MyChar
	
	ClassWindow = GemRB.LoadWindow(2, "GUICG")

	MyChar = GemRB.GetVar ("Slot")
	Race = CommonTables.Races.FindValue (3, GemRB.GetPlayerStat (MyChar, IE_RACE) )
	RaceName = CommonTables.Races.GetRowName(Race)

	ClassCount = CommonTables.Classes.GetRowCount()+1

	j = 0
	#radiobutton groups must be set up before doing anything else to them
	for i in range(1,ClassCount):
		ClassName = CommonTables.Classes.GetRowName (i-1)
		if CommonTables.Classes.GetValue (ClassName, "MULTI"):
			continue
		if j>7:
			Button = ClassWindow.GetControl(j+7)
		else:
			Button = ClassWindow.GetControl(j+2)
		Button.SetFlags(IE_GUI_BUTTON_RADIOBUTTON, OP_OR)
		Button.SetState(IE_GUI_BUTTON_DISABLED)
		j = j+1

	j = 0
	GemRB.SetVar("MAGESCHOOL",0) 
	HasMulti = 0
	for i in range(1,ClassCount):
		ClassName = CommonTables.Classes.GetRowName(i-1)
		Allowed = CommonTables.Classes.GetValue(ClassName, RaceName)
		if CommonTables.Classes.GetValue (ClassName, "MULTI"):
			if Allowed!=0:
				HasMulti = 1
			continue
		if j>7:
			Button = ClassWindow.GetControl(j+7)
		else:
			Button = ClassWindow.GetControl(j+2)
		j = j+1
		t = CommonTables.Classes.GetValue(ClassName, "NAME_REF")
		Button.SetText(t )

		if Allowed==0:
			continue
		if Allowed==2:
			GemRB.SetVar("MAGESCHOOL",5) #illusionist
		Button.SetState(IE_GUI_BUTTON_ENABLED)
		Button.SetEvent(IE_GUI_BUTTON_ON_PRESS, ClassPress)
		Button.SetVarAssoc("Class", i)

	MultiClassButton = ClassWindow.GetControl(10)
	MultiClassButton.SetText(11993)
	if HasMulti == 0:
		MultiClassButton.SetState(IE_GUI_BUTTON_DISABLED)

	BackButton = ClassWindow.GetControl(14)
	BackButton.SetText(15416)
	BackButton.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)
	DoneButton = ClassWindow.GetControl(0)
	DoneButton.SetText(11973)
	DoneButton.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)

	TextAreaControl = ClassWindow.GetControl(13)

	ClassName = GUICommon.GetClassRowName (GemRB.GetVar ("Class")-1, "index")
	if ClassName == "":
		TextAreaControl.SetText(17242)
		DoneButton.SetState(IE_GUI_BUTTON_DISABLED)
	else:
		TextAreaControl.SetText (CommonTables.Classes.GetValue (ClassName, "DESC_REF"))
		DoneButton.SetState(IE_GUI_BUTTON_ENABLED)

	MultiClassButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, MultiClassPress)
	DoneButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, NextPress)
	BackButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, BackPress)
	ClassWindow.SetVisible(WINDOW_VISIBLE)
	return

def BackPress():
	if ClassWindow:
		ClassWindow.Unload()
	GemRB.SetNextScript("CharGen3")
	GemRB.SetVar("Class",0)  #scrapping the class value
	return

def SetClass():
	# find the class from the class table
	ClassIndex = GemRB.GetVar ("Class") - 1
	ClassName = GUICommon.GetClassRowName (ClassIndex, "index")
	Class = CommonTables.Classes.GetValue (ClassName, "ID")
	GemRB.SetPlayerStat (MyChar, IE_CLASS, Class)
	KitIndex = GemRB.GetVar ("Class Kit")
	MageSchool = GemRB.GetVar ("MAGESCHOOL")
	#multiclassed gnomes 
	if MageSchool and not KitIndex:
		SchoolTable = GemRB.LoadTable ("magesch")
		KitIndex = CommonTables.KitList.FindValue (6, SchoolTable.GetValue (MageSchool, 3) )
		KitValue = (0x4000 + KitIndex)
		GemRB.SetPlayerStat (MyChar, IE_KIT, KitValue)

	# protect against barbarians; this stat will be overwritten later
	GemRB.SetPlayerStat (MyChar, IE_HITPOINTS, ClassIndex)

	#assign the correct XP
	if ClassName == "BARBARIAN":
		ClassName = "FIGHTER"
	if GameCheck.IsTOB():
		GemRB.SetPlayerStat (MyChar, IE_XP, CommonTables.ClassSkills.GetValue (ClassName, "STARTXP2"))
	else:
		GemRB.SetPlayerStat (MyChar, IE_XP, CommonTables.ClassSkills.GetValue (ClassName, "STARTXP"))

	#create an array to get all the classes from
	NumClasses = 1
	IsMulti = GUICommon.IsMultiClassed (MyChar, 1)
	if IsMulti[0] > 1:
		NumClasses = IsMulti[0]
		Classes = [IsMulti[1], IsMulti[2], IsMulti[3]]
	else:
		Classes = [GemRB.GetPlayerStat (MyChar, IE_CLASS)]

	#loop through each class and update it's level
	xp = GemRB.GetPlayerStat (MyChar, IE_XP)/NumClasses
	for i in range (NumClasses):
		CurrentLevel = LUCommon.GetNextLevelFromExp (xp, Classes[i])
		if i == 0:
			GemRB.SetPlayerStat (MyChar, IE_LEVEL, CurrentLevel)
		elif i <= 2:
			GemRB.SetPlayerStat (MyChar, IE_LEVEL2+i-1, CurrentLevel)

def MultiClassPress():
	GemRB.SetVar("Class Kit",0)
	if ClassWindow:
		ClassWindow.Unload()
	GemRB.SetNextScript("GUICG10")
	return

def ClassPress():
	SetClass()
	if ClassWindow:
		ClassWindow.Unload()
	GemRB.SetNextScript("GUICG22")
	return

def NextPress ():
	SetClass()
	if ClassWindow:
		ClassWindow.Unload()
	GemRB.SetNextScript("CharGen4") #alignment
	return
