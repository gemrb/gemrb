# -*-python-*-
# GemRB - Infinity Engine Emulator
# Copyright (C) 2003-2004 The GemRB Project
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#
# LUSkillSelection.py - selection of thief skills

import GemRB
from GUIDefines import *
from ie_stats import *
import GameCheck
import GUICommon
import CommonTables

#constants
LUSKILLS_TYPE_LEVELUP = 1
LUSKILLS_TYPE_CHARGEN = 2
LUSKILLS_TYPE_DUALCLASS = 4
LUSKILLS_MAX = 250

#refs to the script calling this
SkillsWindow = 0
SkillsCallback = 0

#offsets to the various parts of all 3 windows
SkillsOffsetPress = 0
SkillsOffsetButton1 = 0
SkillsOffsetName = 0
SkillsOffsetPoints = 0
SkillsOffsetSum = 0
SkillsNumButtons = 0

#internal variables
SkillsIndices = []
SkillPointsLeft = 0
SkillsTopIndex = 0
SkillsTable = 0
SkillsOldPos = 0
SkillsClickCount = 0
SkillsOldDirection = 0
SkillsTextArea = 0
SkillsKitName = 0
SkillsAssignable = 0

#WARNING: This WILL NOT show the window, only access it. To see the return, call GemRB.GetVar ("SkillPointsLeft").
# If nothing can be assigned, it will return 0 prior to accessing any of the window methods.
def SetupSkillsWindow (pc, type, window, callback, level1=[0,0,0], level2=[1,1,1], classid=0, scroll=True):
	global SkillsWindow, SkillsCallback, SkillsOffsetPress, SkillsOffsetButton1, SkillsOffsetName
	global SkillsOffsetPoints, SkillsOffsetSum, SkillsIndices, SkillPointsLeft, SkillsTopIndex
	global SkillsTable, SkillsOldPos, SkillsClickCount, SkillsOldDirection, SkillsNumButtons
	global SkillsTextArea, SkillsKitName, SkillsAssignable

	#reset some basic values
	SkillsWindow = window
	SkillsCallback = callback
	SkillsOldPos = 0
	SkillsClickCount = 0
	SkillsOldDirection = 0
	SkillsAssignable = 0
	SkillsTable = GemRB.LoadTable ("skills")
	SkillPointsLeft = 0
	GemRB.SetVar ("SkillPointsLeft", 0)
	SkillsNullify ()

	#make sure we're within ranges
	if not window or not callback or len(level1)!=len(level2):
		return

	#setup the offsets
	if type == LUSKILLS_TYPE_LEVELUP and GameCheck.IsBG2():
		SkillsOffsetPress = 120
		SkillsOffsetButton1 = 17
		SkillsOffsetSum = 37
		SkillsOffsetName = 32
		SkillsOffsetPoints = 43
		SkillsNumButtons = 4
		SkillsTextArea = SkillsWindow.GetControl (110)
		ScrollBar = SkillsWindow.GetControl (109)
	elif type == LUSKILLS_TYPE_LEVELUP:
		SkillsOffsetPress = -1
		SkillsOffsetButton1 = 17
		SkillsOffsetSum = 37
		SkillsOffsetName = 32
		SkillsOffsetPoints = 43
		SkillsNumButtons = 4
		SkillsTextArea = SkillsWindow.GetControl (42)
		if (scroll):
			ScrollBar = SkillsWindow.GetControl (109)
	elif type == LUSKILLS_TYPE_DUALCLASS:
		SkillsOffsetPress = 5
		SkillsOffsetButton1 = 14
		SkillsOffsetSum = 8
		SkillsOffsetName = 0
		SkillsOffsetPoints = 9
		SkillsNumButtons = 4
		SkillsTextArea = SkillsWindow.GetControl (22)
		SkillsTextArea.SetText(17248)
		if (scroll):
			ScrollBar = SkillsWindow.GetControl (26)
			ScrollBar.SetDefaultScrollBar ()
	elif type == LUSKILLS_TYPE_CHARGEN:
		SkillsOffsetPress = 21
		SkillsOffsetButton1 = 11
		SkillsOffsetSum = 5
		SkillsOffsetName = 6
		SkillsOffsetPoints = 1
		SkillsNumButtons = 4
		SkillsTextArea = SkillsWindow.GetControl (19)
		SkillsTextArea.SetText(17248)
		if (scroll):
			ScrollBar = SkillsWindow.GetControl (26)
			ScrollBar.SetDefaultScrollBar ()
	else:
		return

	#get our class id and name
	IsDual = GUICommon.IsDualClassed (pc, 1)
	IsMulti = GUICommon.IsMultiClassed (pc, 1)
	if classid: #used when dual-classing
		Class = classid
	elif IsDual[0]: #only care about the current class
		Class = GUICommon.GetClassRowName(IsDual[2], "index")
		Class = CommonTables.Classes.GetValue (Class, "ID")
	else:
		Class = GemRB.GetPlayerStat (pc, IE_CLASS)
	ClassName = GUICommon.GetClassRowName(Class, "class")

	#get the number of classes
	if IsMulti[0]>1:
		NumClasses = IsMulti[0]
	else:
		NumClasses = 1
	if NumClasses > len (level2):
		return

	#figure out the kitname if we need it
	#protect against kitted multiclasses
	Kit = GUICommon.GetKitIndex (pc)
	if not Kit or type == LUSKILLS_TYPE_DUALCLASS or IsDual[0] or IsMulti[0]>1:
		SkillsKitName = ClassName
	else:
		SkillsKitName = CommonTables.KitList.GetValue (Kit, 0, GTV_STR)

	#figure out the correct skills table
	SkillIndex = -1
	for i in range (NumClasses):
		TmpClass = Class
		if NumClasses > 1:
			TmpClass = IsMulti[i+1]
		TmpClass = GUICommon.GetClassRowName (TmpClass, "class")
		if (CommonTables.ClassSkills.GetValue (TmpClass, "THIEFSKILL", 0) != "*"):
			SkillIndex = i
			break

	#see if we got a thief (or monk)
	SkillsIndices = []
	if SkillIndex >= 0:
		#SkillsKitName should be fine as all multis are in classes.2da
		#also allows for thief kits
		SkillsAssignable = 1
		for i in range(SkillsTable.GetRowCount()-2):
			# -2/+2 to compensate for the special first_level and rate rows
			SkillName = SkillsTable.GetRowName (i+2)
			if SkillsTable.GetValue (SkillName, SkillsKitName) != -1:
				SkillsIndices.append(i)

		LevelDiff = []
		for i in range (NumClasses):
			LevelDiff.append (level2[i]-level1[i])
		if level1[SkillIndex] == 0:
			SkillPointsLeft = SkillsTable.GetValue ("FIRST_LEVEL", SkillsKitName, GTV_INT)
			LevelDiff[SkillIndex] -= 1
		SkillPointsLeft += LevelDiff[SkillIndex] * SkillsTable.GetValue("RATE", SkillsKitName, GTV_INT)
		TotalSkillsAssignable = 0

		if SkillPointsLeft < 0:
			#really don't have an entry
			SkillPointsLeft = 0
		else:
			#get the skill values
			for i in range(SkillsTable.GetRowCount()-2):
				# -2/+2 to compensate for the special first_level and rate rows
				SkillName = SkillsTable.GetRowName (i+2)
				SkillID = SkillsTable.GetValue (SkillName, "ID")
				SkillValue = GemRB.GetPlayerStat (pc, SkillID)
				BaseSkillValue = GemRB.GetPlayerStat (pc, SkillID, 1)
				GemRB.SetVar("Skill "+str(i), SkillValue)
				GemRB.SetVar("SkillBase "+str(i), SkillValue)
				# display the modified stat to avoid confusion (account for dex, race and effect boni)
				GemRB.SetVar("SkillDisplayMod "+str(i), SkillValue-BaseSkillValue)
				TotalSkillsAssignable += LUSKILLS_MAX-SkillValue

		#protect against having more skills than we can assign
		if SkillPointsLeft > TotalSkillsAssignable:
			SkillPointsLeft = TotalSkillsAssignable
		GemRB.SetVar ("SkillPointsLeft", SkillPointsLeft)
	else: 
		#get ranger and bard skills
		SpecialSkillsMap = []
		for i in range(NumClasses):
			if IsMulti[0]>1:
				classname = IsMulti[i+1]
			else:
				classname = Class
			classname = GUICommon.GetClassRowName (classname, "class")
			for table in "RANGERSKILL", "BARDSKILL":
				SpecialSkillsTable = CommonTables.ClassSkills.GetValue (classname, table)
				if SpecialSkillsTable != "*":
					SpecialSkillsMap.append((SpecialSkillsTable, i))
					break
		for skills in SpecialSkillsMap:
			SpecialSkillsTable = GemRB.LoadTable (skills[0])
			for skill in range(SpecialSkillsTable.GetColumnCount ()):
				skillname = SpecialSkillsTable.GetColumnName (skill)
				value = SpecialSkillsTable.GetValue (str(level2[skills[1]]), skillname)
				skillindex = SkillsTable.GetRowIndex (skillname) - 2
				GemRB.SetVar ("Skill " + str(skillindex), value)
				SkillsIndices.append(skillindex)

	#we didn't find anything, so don't continue (will show as a return of 0)
	#or don't display if we aren't leveling and have a bard/ranger
	if not len (SkillsIndices) or (not SkillPointsLeft and type != LUSKILLS_TYPE_LEVELUP):
		SkillSumLabel = SkillsWindow.GetControl(0x10000000+SkillsOffsetSum)
		SkillSumLabel.SetText("")
		return
	
	#skills scrollbar
	SkillsTopIndex = 0
	GemRB.SetVar ("SkillsTopIndex", SkillsTopIndex)
	if len(SkillsIndices) > SkillsNumButtons:
		ScrollBar.SetEvent (IE_GUI_SCROLLBAR_ON_CHANGE, SkillScrollBarPress)
		#decrease it with the number of controls on screen (list size) and two unrelated rows
		ScrollBar.SetVarAssoc ("SkillsTopIndex", SkillsTable.GetRowCount()-SkillsNumButtons-2)
	else:
		if len(SkillsIndices) and SkillsAssignable:
			#autoscroll to the first valid skill; luckily all three monk ones are adjacent
			GemRB.SetVar ("SkillsTopIndex", SkillsIndices[0])

	#setup all the visible buttons
	for i in range(len(SkillsIndices)):
		if i == SkillsNumButtons:
			break
		if SkillsOffsetPress != -1:
			Button = SkillsWindow.GetControl(i+SkillsOffsetPress)
			Button.SetVarAssoc("Skill",SkillsIndices[i])
			Button.SetEvent(IE_GUI_BUTTON_ON_PRESS, SkillJustPress)

		Button = SkillsWindow.GetControl(i*2+SkillsOffsetButton1)
		Button.SetVarAssoc("Skill",SkillsIndices[i])
		Button.SetEvent(IE_GUI_BUTTON_ON_PRESS, SkillLeftPress)

		Button = SkillsWindow.GetControl(i*2+SkillsOffsetButton1+1)
		Button.SetVarAssoc("Skill",SkillsIndices[i])
		Button.SetEvent(IE_GUI_BUTTON_ON_PRESS, SkillRightPress)
	
	SkillsRedraw ()
	return

def SkillsRedraw (direction=0):
	global SkillsOldDirection, SkillsClickCount

	#update how many skill points are left and call the callback function
	SkillSumLabel = SkillsWindow.GetControl(0x10000000+SkillsOffsetSum)
	SkillSumLabel.SetText(str(SkillPointsLeft))

	for i in range (SkillsNumButtons):
		if len(SkillsIndices) <= i:
			SkillsHide (i)
			continue

		#show the current skills name
		Pos = SkillsIndices[SkillsTopIndex+i]
		SkillName = SkillsTable.GetValue (SkillsTable.GetRowName (Pos+2), "CAP_REF")
		Label = SkillsWindow.GetControl (0x10000000+SkillsOffsetName+i)
		Label.SetText (SkillName)

		#enable/disable the button if we can(not) get the skills
		SkillName = SkillsTable.GetRowName (Pos+2)
		Ok = SkillsTable.GetValue (SkillName, SkillsKitName) and SkillsAssignable
		Button1 = SkillsWindow.GetControl(i*2+SkillsOffsetButton1)
		Button2 = SkillsWindow.GetControl(i*2+SkillsOffsetButton1+1)
		if not Ok:
			Button1.SetState(IE_GUI_BUTTON_DISABLED)
			Button2.SetState(IE_GUI_BUTTON_DISABLED)
			Button1.SetFlags(IE_GUI_BUTTON_NO_IMAGE,OP_OR)
			Button2.SetFlags(IE_GUI_BUTTON_NO_IMAGE,OP_OR)
		else:
			Button1.SetState(IE_GUI_BUTTON_ENABLED)
			Button2.SetState(IE_GUI_BUTTON_ENABLED)
			Button1.SetFlags(IE_GUI_BUTTON_NO_IMAGE,OP_NAND)
			Button2.SetFlags(IE_GUI_BUTTON_NO_IMAGE,OP_NAND)
	
		#show how many points are allocated to this skill		
		Label = SkillsWindow.GetControl(0x10000000+SkillsOffsetPoints+i)
		ActPoint = GemRB.GetVar("Skill "+str(Pos) )
		Label.SetText(str(ActPoint))

	#setup doublespeed
	if SkillsOldDirection == direction:
		SkillsClickCount = SkillsClickCount + 1
		if SkillsClickCount>10:
			GemRB.SetRepeatClickFlags(GEM_RK_QUADRUPLESPEED, OP_OR)
		return

	SkillsOldDirection = direction
	SkillsClickCount = 0
	GemRB.SetRepeatClickFlags(GEM_RK_QUADRUPLESPEED, OP_NAND)
	return

def SkillJustPress():
	Pos = GemRB.GetVar("Skill")+SkillsTopIndex
	SkillsTextArea.SetText (SkillsTable.GetValue (SkillsTable.GetRowName (Pos+2), "DESC_REF"))
	return

def SkillRightPress():
	global SkillPointsLeft, SkillsClickCount, SkillsOldPos

	Pos = GemRB.GetVar("Skill")+SkillsTopIndex
	SkillsTextArea.SetText (SkillsTable.GetValue (SkillsTable.GetRowName (Pos+2), "DESC_REF"))
	ActPoint = GemRB.GetVar("Skill "+str(Pos) )
	BasePoint = GemRB.GetVar("SkillBase "+str(Pos) )
	if ActPoint <= 0 or ActPoint <= BasePoint:
		return
	GemRB.SetVar("Skill "+str(Pos),ActPoint-1)
	SkillPointsLeft = SkillPointsLeft + 1
	if SkillsOldPos != Pos:
		SkillsOldPos = Pos
		SkillsClickCount = 0

	GemRB.SetVar ("SkillPointsLeft", SkillPointsLeft)
	SkillsRedraw(2)
	SkillsCallback ()
	return

def SkillLeftPress():
	global SkillPointsLeft, SkillsClickCount, SkillsOldPos

	Pos = GemRB.GetVar("Skill")+SkillsTopIndex
	SkillsTextArea.SetText (SkillsTable.GetValue (SkillsTable.GetRowName (Pos+2), "DESC_REF"))
	if SkillPointsLeft == 0:
		return
	ActPoint = GemRB.GetVar("Skill "+str(Pos) )
	if ActPoint >= LUSKILLS_MAX:
		return
	GemRB.SetVar("Skill "+str(Pos), ActPoint+1)
	SkillPointsLeft = SkillPointsLeft - 1
	if SkillsOldPos != Pos:
		SkillsOldPos = Pos
		SkillsClickCount = 0

	GemRB.SetVar ("SkillPointsLeft", SkillPointsLeft)
	SkillsRedraw(1)
	SkillsCallback ()
	return

def SkillScrollBarPress():
	global SkillsTopIndex

	SkillsTopIndex = GemRB.GetVar("SkillsTopIndex")
	SkillsRedraw ()
	return

# saves all the skills
# TODO: change the layout of the iwd/how skills table to match the rest
def SkillsSave (pc):
	global SkillsTable
	if not SkillsTable:
		SkillsTable = GemRB.LoadTable ("skills")

	for i in range(SkillsTable.GetRowCount() - 2):
		SkillName = SkillsTable.GetRowName (i+2)
		SkillID = SkillsTable.GetValue (SkillName, "ID")
		SkillValue = GemRB.GetVar ("Skill "+str(i)) - GemRB.GetVar("SkillDisplayMod "+str(i))
		if SkillValue > 0:
			GemRB.SetPlayerStat (pc, SkillID, SkillValue)

def SkillsNullify ():
	global SkillsTable
	if not SkillsTable:
		SkillsTable = GemRB.LoadTable ("skills")

	for i in range(SkillsTable.GetRowCount()-2):
		GemRB.SetVar ("Skill "+str(i), 0)
		GemRB.SetVar ("SkillBase "+str(i), 0)

def SkillsHide (i):
	Label = SkillsWindow.GetControl (0x10000000+SkillsOffsetName+i)
	Label.SetText ("")
	Button1 = SkillsWindow.GetControl(i*2+SkillsOffsetButton1)
	Button1.SetState(IE_GUI_BUTTON_DISABLED)
	Button1.SetFlags(IE_GUI_BUTTON_NO_IMAGE,OP_OR)
	Button2 = SkillsWindow.GetControl(i*2+SkillsOffsetButton1+1)
	Button2.SetState(IE_GUI_BUTTON_DISABLED)
	Button2.SetFlags(IE_GUI_BUTTON_NO_IMAGE,OP_OR)
	Label = SkillsWindow.GetControl(0x10000000+SkillsOffsetPoints+i)
	Label.SetText("")
