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
# character generation, racial enemy (GUICG15)
# this file can't handle leveling up over two boundaries (more than 1 new
# racial enemy), but that will only affect testers (6-10 level gain or more)
import GemRB
import GUICommon
import CharOverview
import CommonTables
from GUIDefines import *
from ie_stats import IE_CLASS, IE_LEVELRANGER, IE_HATEDRACE, IE_HATEDRACE2

RaceWindow = 0
TextAreaControl = 0
DoneButton = 0
RacialEnemyTable = 0
RaceCount = 0
TopIndex = 0
CharGen = 0
RacialEnemies = [255] * 9
RacialStats = [0] * 9

def DisplayRaces():
	global TopIndex

	TopIndex=GemRB.GetVar("TopIndex")
	for i in range(11):
		Button = RaceWindow.GetControl(i+22)
		Val = RacialEnemyTable.GetValue(i+TopIndex,0)
		raceIDS = RacialEnemyTable.GetValue (i+TopIndex, 1)
		if Val == 0:
			Button.SetText("")
			Button.SetState(IE_GUI_BUTTON_DISABLED)
		# also disable already picked ones
		elif raceIDS in RacialEnemies:
			Button.SetText (Val)
			Button.SetState (IE_GUI_BUTTON_DISABLED)
		else:
			Button.SetText(Val)
			Button.SetState(IE_GUI_BUTTON_ENABLED)
			Button.OnPress (RacePress)
			Button.SetVarAssoc ("HatedRace", raceIDS)
	return

def OnLoad():
	OpenEnemyWindow (1)

def OpenEnemyWindow(chargen=0):
	global RaceWindow, TextAreaControl, DoneButton
	global RacialEnemyTable, RaceCount, TopIndex
	global CharGen

	CharGen = chargen

	rankDiff = 0
	if chargen:
		RaceWindow = GemRB.LoadWindow (15, "GUICG")
		CharOverview.PositionCharGenWin (RaceWindow)
		pc = GemRB.GetVar ("Slot")
		Class = GemRB.GetPlayerStat (pc, IE_CLASS)
	else:
		RaceWindow = GemRB.LoadWindow (16, "GUIREC")
		pc = GemRB.GameGetSelectedPCSingle ()
		Class = (GemRB.GetVar ("LUClass") or 0) + 1
		LevelDiff = GemRB.GetVar ("LevelDiff") or 0
		rangerLevel = GemRB.GetPlayerStat (pc, IE_LEVELRANGER)
		rankDiff = (rangerLevel+LevelDiff)//5 - rangerLevel//5

	ClassName = GUICommon.GetClassRowName (Class, "class")
	TableName = CommonTables.ClassSkills.GetValue(ClassName, "HATERACE")
	if TableName == "*":
		print("Skipping Racial enemies: chosen class doesn't know the concept!")
		NextPress (0)
		return
	# at this point it is already guaranteed that we have a ranger
	# but they get new racial enemies only at level 5 and each 5th level
	if not chargen and rankDiff == 0:
		print("Skipping Racial enemies: iwd2 gives them every 5th level!")
		NextPress (0)
		return

	RacialEnemyTable = GemRB.LoadTable(TableName)
	RaceCount = RacialEnemyTable.GetRowCount()-11
	if RaceCount<0:
		RaceCount=0
	GenerateHateLists (pc)

	for i in range(11):
		Button = RaceWindow.GetControl(i+22)
		Button.SetFlags(IE_GUI_BUTTON_RADIOBUTTON,OP_OR)

	if chargen:
		BackButton = RaceWindow.GetControl (10)
		BackButton.SetText (15416)
		BackButton.MakeEscape()
		BackButton.OnPress (BackPress)
	else:
		RaceWindow.DeleteControl (10)

	DoneButton = RaceWindow.GetControl(11)
	DoneButton.SetText(11973)
	DoneButton.MakeDefault()
	DoneButton.SetState(IE_GUI_BUTTON_DISABLED)

	TextAreaControl = RaceWindow.GetControl(8)
	TextAreaControl.SetText(17256)
	TopIndex = 0
	GemRB.SetVar("TopIndex",0)
	ScrollBarControl = RaceWindow.GetControl(1)
	ScrollBarControl.SetVarAssoc("TopIndex",RaceCount)
	ScrollBarControl.OnChange (DisplayRaces)

	DoneButton.OnPress (NextPress)

	if not chargen:
		RaceWindow.ShowModal (MODAL_SHADOW_GRAY)
	else:
		RaceWindow.Focus()
	DisplayRaces()
	return

def GenerateHateLists (pc):
	global RacialEnemies, RacialStats

	RacialEnemies[0] = GemRB.GetPlayerStat (pc, IE_HATEDRACE, 1)
	RacialStats[0] = IE_HATEDRACE
	for i in range(1, len(RacialEnemies)-1):
		RacialStats[i] = IE_HATEDRACE2 + i-1
		RacialEnemies[i] = GemRB.GetPlayerStat (pc, RacialStats[i], 1) % 255

def RacePress():
	Race = GemRB.GetVar("HatedRace")
	Row = RacialEnemyTable.FindValue(1, Race)
	TextAreaControl.SetText(RacialEnemyTable.GetValue(Row, 2) )
	DoneButton.SetState(IE_GUI_BUTTON_ENABLED)
	return

def BackPress():
	if RaceWindow:
		RaceWindow.Close ()
	GemRB.SetVar("HatedRace",0)  #scrapping the race value
	GemRB.SetNextScript("CharGen6")
	return

def NextPress(save=1):
	if RaceWindow:
		RaceWindow.Close ()
	if CharGen:
		GemRB.SetNextScript("Skills")
		return

	if save:
		# find the index past the last set stat
		last = RacialEnemies.index (0)

		# save, but note that racial enemies are stored in many stats
		pc = GemRB.GameGetSelectedPCSingle ()
		newHated = GemRB.GetVar ("HatedRace") or 0
		GemRB.SetPlayerStat (pc, RacialStats[last], newHated)

	# open up the next levelup window
	import Skills
	Skills.OpenSkillsWindow (0)
	return
