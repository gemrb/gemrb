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
#character generation, class (GUICG2)
import GemRB
from LUCommon import *

ClassWindow = 0
TextAreaControl = 0
DoneButton = 0
MyChar = 0

def OnLoad():
	global ClassWindow, TextAreaControl, DoneButton, MyChar
	
	GemRB.LoadWindowPack("GUICG", 640, 480)
	ClassWindow = GemRB.LoadWindowObject(2)

	MyChar = GemRB.GetVar ("Slot")
	Race = RaceTable.FindValue (3, GemRB.GetPlayerStat (MyChar, IE_RACE) )
	RaceName = RaceTable.GetRowName(Race)

	ClassCount = ClassTable.GetRowCount()+1

	j = 0
	#radiobutton groups must be set up before doing anything else to them
	for i in range(1,ClassCount):
		if ClassTable.GetValue(i-1,4):
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
		ClassName = ClassTable.GetRowName(i-1)
		Allowed = ClassTable.GetValue(ClassName, RaceName)
		if ClassTable.GetValue(i-1,4):
			if Allowed!=0:
				HasMulti = 1
			continue
		if j>7:
			Button = ClassWindow.GetControl(j+7)
		else:
			Button = ClassWindow.GetControl(j+2)
		j = j+1
		t = ClassTable.GetValue(i-1, 0)
		Button.SetText(t )

		if Allowed==0:
			continue
		if Allowed==2:
			GemRB.SetVar("MAGESCHOOL",5) #illusionist
		Button.SetState(IE_GUI_BUTTON_ENABLED)
		Button.SetEvent(IE_GUI_BUTTON_ON_PRESS,  "ClassPress")
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

	Class = GemRB.GetVar("Class")-1
	if Class<0:
		TextAreaControl.SetText(17242)
		DoneButton.SetState(IE_GUI_BUTTON_DISABLED)
	else:
		TextAreaControl.SetText(ClassTable.GetValue(Class,1) )
		DoneButton.SetState(IE_GUI_BUTTON_ENABLED)

	MultiClassButton.SetEvent(IE_GUI_BUTTON_ON_PRESS,"MultiClassPress")
	DoneButton.SetEvent(IE_GUI_BUTTON_ON_PRESS,"NextPress")
	BackButton.SetEvent(IE_GUI_BUTTON_ON_PRESS,"BackPress")
	ClassWindow.SetVisible(1)
	return

def BackPress():
	if ClassWindow:
		ClassWindow.Unload()
	GemRB.SetNextScript("CharGen3")
	GemRB.SetVar("Class",0)  #scrapping the class value
	return

def SetClass():
	if ClassWindow:
		ClassWindow.Unload()

	# find the class from the class table
	ClassIndex = GemRB.GetVar ("Class") - 1
	Class = ClassTable.GetValue (ClassIndex, 5)
	#protect against barbarians
	ClassName = ClassTable.GetRowName (ClassTable.FindValue (5, Class) )
	GemRB.SetPlayerStat (MyChar, IE_CLASS, Class)
	#assign the correct XP
	if GameIsTOB():
		GemRB.SetPlayerStat (MyChar, IE_XP, ClassSkillsTable.GetValue (ClassName, "STARTXP2"))
	else:
		GemRB.SetPlayerStat (MyChar, IE_XP, ClassSkillsTable.GetValue (ClassName, "STARTXP"))

	#create an array to get all the classes from
	NumClasses = 1
	IsMulti = IsMultiClassed (MyChar, 1)
	if IsMulti[0] > 1:
		NumClasses = IsMulti[0]
		Classes = [IsMulti[1], IsMulti[2], IsMulti[3]]
	else:
		Classes = [GemRB.GetPlayerStat (MyChar, IE_CLASS)]

	#loop through each class and update it's level
	xp = GemRB.GetPlayerStat (MyChar, IE_XP)/NumClasses
	for i in range (NumClasses):
		CurrentLevel = GetNextLevelFromExp (xp, Classes[i])
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
	GemRB.SetNextScript("GUICG22")
	return

def NextPress ():
	SetClass()
	GemRB.SetNextScript("CharGen4") #alignment
	return
