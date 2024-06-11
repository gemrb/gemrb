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
#character generation, skills (GUICG6)
import GemRB
from GUIDefines import *
import CharOverview
import CommonTables
import IDLUCommon
from ie_stats import IE_INT, IE_UNUSED_SKILLPTS, IE_CLASSLEVELSUM

SkillWindow = 0
TextAreaControl = 0
DoneButton = 0
SkillTable = 0
CostTable = 0
TopIndex = 0
PointsLeft = 0
Level = 0
ClassColumn = 0
CharGen = 0
StatLowerLimit = [0] * 16
ButtonCount = 0

def RedrawSkills():
	global CostTable, TopIndex

	SumLabel = SkillWindow.GetControl(0x1000000c)
	if PointsLeft == 0 or (not CharGen and PointsLeft == 1):
		DoneButton.SetState(IE_GUI_BUTTON_ENABLED)
		SumLabel.SetColor ({'r' : 255, 'g' : 255, 'b' : 255})
	else:
		SumLabel.SetColor ({'r' : 255, 'g' : 255, 'b' : 0})
	SumLabel.SetText(str(PointsLeft) )

	maxSkill = Level + 3
	# ^ crossclass skills max is handled implicitly by the higher cost
	for i in range(ButtonCount):
		Pos=TopIndex+i
		Cost = CostTable.GetValue(Pos, ClassColumn)
		# Skill cost is currently restricted to base classes. This means it is fairly easy
		# to add a class kit as it will use its parents skill cost values however it makes
		# it impossible to add a new class entirely. Support for whole new classes is
		# probably not massively important but could eventually allow Prestige classes to
		# be implemented
		SkillName = SkillTable.GetValue(Pos, 1)
		Label = SkillWindow.GetControl(0x10000001+i)
		ActPoint = GemRB.GetVar("Skill "+str(Pos) )
		if Cost>0:
			#we use this function to retrieve the string
			t = GemRB.GetString (SkillName).format(0, 0)
			Label.SetText("%s (%d)"%(t,Cost) )
			Label.SetColor ({'r' : 255, 'g' : 255, 'b' : 255})
			if PointsLeft < 1 or ActPoint * Cost >= maxSkill:
				Label.SetColor ({'r' : 150, 'g' : 150, 'b' : 150})
		else:
			Label.SetText(SkillName)
			Label.SetColor ({'r' : 150, 'g' : 150, 'b' : 150}) # Grey

		Button1 = SkillWindow.GetControl(i*2+14)
		Button2 = SkillWindow.GetControl(i*2+15)
		
		Btn1State = Btn2State = IE_GUI_BUTTON_ENABLED
		if Cost < 1:
			Btn1State = Btn2State = IE_GUI_BUTTON_DISABLED
		else:
			if ActPoint * Cost >= maxSkill or PointsLeft < 1:
				Btn1State = IE_GUI_BUTTON_DISABLED
			if ActPoint == StatLowerLimit[Pos]:
				Btn2State = IE_GUI_BUTTON_DISABLED
		Button1.SetState(Btn1State)
		Button2.SetState(Btn2State)

		Label = SkillWindow.GetControl(0x10000069+i)
		Label.SetText(str(ActPoint) )
		if ActPoint > StatLowerLimit[Pos]:
			Label.SetColor ({'r' : 0, 'g' : 255, 'b' : 255})
		elif Cost < 1 or PointsLeft < 1:
			Label.SetColor ({'r' : 150, 'g' : 150, 'b' : 150})
		else:
			Label.SetColor ({'r' : 255, 'g' : 255, 'b' : 255})

	return

def ScrollBarPress():
	global TopIndex

	newTopIndex = GemRB.GetVar("TopIndex")
	if newTopIndex + ButtonCount <= len(StatLowerLimit):
		TopIndex = newTopIndex
		RedrawSkills()

	return

def OnLoad():
	OpenSkillsWindow (1, 1)

def OpenSkillsWindow(chargen, level=0):
	global SkillWindow, TextAreaControl, DoneButton, TopIndex
	global SkillTable, CostTable, PointsLeft
	global Level, ClassColumn
	global CharGen, StatLowerLimit, ButtonCount

	CharGen = chargen

	SkillPtsTable = GemRB.LoadTable ("skillpts")
	if chargen:
		pc = GemRB.GetVar ("Slot")
		Level = level
		LevelDiff = 1
		ClassColumn = GemRB.GetVar ("BaseClass") - 1
	else:
		pc = GemRB.GameGetSelectedPCSingle ()
		LevelDiff = GemRB.GetVar ("LevelDiff") or 0
		Level = GemRB.GetPlayerStat (pc, IE_CLASSLEVELSUM) + LevelDiff
		BaseClass = GemRB.GetVar ("LUClass") or 0
		ClassColumn = BaseClass

	PointsLeft = SkillPtsTable.GetValue (0, ClassColumn)
	IntBonus = GemRB.GetPlayerStat (pc, IE_INT) // 2 - 5
	PointsLeft += IntBonus * LevelDiff

	# at least 1 skillpoint / level advanced
	if PointsLeft < 1:
		PointsLeft = 1

	# start with 4x as many skills points initially
	if chargen:
		PointsLeft *= 4

	PointsLeft *= LevelDiff

	# Humans receive +2 skill points at level 1 and +1 skill points each level thereafter
	RaceBonusTable = GemRB.LoadTable ("racskill")
	RaceName = CommonTables.Races.GetRowName (IDLUCommon.GetRace (pc))
	PointsLeft += RaceBonusTable.GetValue (RaceName, "BONUS") * LevelDiff
	if Level < 2:
		PointsLeft += RaceBonusTable.GetValue (RaceName, "LEVEL1_BONUS")

	PointsLeft += GemRB.GetPlayerStat (pc, IE_UNUSED_SKILLPTS)

	SkillTable = GemRB.LoadTable("skills")
	RowCount = SkillTable.GetRowCount()

	CostTable = GemRB.LoadTable("skilcost")

	TmpTable = GemRB.LoadTable ("skillsta")
	for i in range(RowCount):
		# Racial/Class bonuses don't factor in char-gen or leveling, so can be safely ignored
		StatID = TmpTable.GetValue (i, 0, GTV_STAT)
		if CharGen:
			GemRB.SetPlayerStat (pc, StatID, 0)
		StatLowerLimit[i] = GemRB.GetPlayerStat (pc, StatID, 1)
		GemRB.SetVar ("Skill "+str(i), StatLowerLimit[i])

	GemRB.SetToken("number",str(PointsLeft) )
	if CharGen:
		SkillWindow = GemRB.LoadWindow (6, "GUICG")
		ButtonCount = 10
		CharOverview.PositionCharGenWin (SkillWindow)
	else:
		SkillWindow = GemRB.LoadWindow (55, "GUIREC")
		ButtonCount = 9

	for i in range(ButtonCount):
		Button = SkillWindow.GetControl(i+93)
		Button.SetVarAssoc("Skill",i)
		Button.OnPress (JustPress)

		Button = SkillWindow.GetControl(i*2+14)
		Button.SetVarAssoc("Skill",i)
		Button.OnPress (LeftPress)
		Button.SetActionInterval (200)

		Button = SkillWindow.GetControl(i*2+15)
		Button.SetVarAssoc("Skill",i)
		Button.OnPress (RightPress)
		Button.SetActionInterval (200)

	if chargen:
		BackButton = SkillWindow.GetControl (105)
		BackButton.SetText (15416)
		BackButton.MakeEscape()
		BackButton.OnPress (BackPress)
	else:
		SkillWindow.DeleteControl (105)

	DoneButton = SkillWindow.GetControl(0)
	DoneButton.SetText(36789)
	DoneButton.MakeDefault()

	TextAreaControl = SkillWindow.GetControl(92)
	TextAreaControl.SetText(17248)

	ScrollBarControl = SkillWindow.GetControl(104)
	ScrollBarControl.OnChange (ScrollBarPress)
	SkillWindow.SetEventProxy(ScrollBarControl)
	#decrease it with the number of controls on screen (list size)
	TopIndex = 0
	GemRB.SetVar("TopIndex",0)
	ScrollBarControl.SetVarAssoc("TopIndex",RowCount-10+1)

	DoneButton.OnPress (NextPress)
	DoneButton.SetState(IE_GUI_BUTTON_DISABLED)
	RedrawSkills()

	if not chargen:
		SkillWindow.ShowModal (MODAL_SHADOW_GRAY)
	else:
		SkillWindow.Focus()

	return

def JustPress():
	Pos = GemRB.GetVar("Skill")+TopIndex
	TextAreaControl.SetText(SkillTable.GetValue(Pos,2) )
	return

def RightPress():
	global PointsLeft

	Pos = GemRB.GetVar("Skill")+TopIndex
	Cost = CostTable.GetValue(Pos, ClassColumn)
	if Cost==0:
		return

	TextAreaControl.SetText(SkillTable.GetValue(Pos,2) )
	ActPoint = GemRB.GetVar("Skill "+str(Pos) )
	if ActPoint <= StatLowerLimit[Pos]:
		return
	GemRB.SetVar("Skill "+str(Pos),ActPoint-1)
	PointsLeft = PointsLeft + Cost
	RedrawSkills()
	return

def LeftPress():
	global PointsLeft

	Pos = GemRB.GetVar("Skill")+TopIndex
	Cost = CostTable.GetValue(Pos, ClassColumn)
	if Cost==0:
		return

	TextAreaControl.SetText(SkillTable.GetValue(Pos,2) )
	if PointsLeft < Cost:
		return
	ActPoint = GemRB.GetVar("Skill "+str(Pos) )
	if Cost*ActPoint >= Level+3: # maxSkill
		return
	GemRB.SetVar("Skill "+str(Pos), ActPoint+1)
	PointsLeft = PointsLeft - Cost
	RedrawSkills()
	return

def BackPress():
	MyChar = GemRB.GetVar("Slot")
	TmpTable = GemRB.LoadTable ("skillsta")
	for i in range(TmpTable.GetRowCount()):
		GemRB.SetVar("Skill "+str(i),0)
		StatID=TmpTable.GetValue (i, 0, GTV_STAT)
		GemRB.SetPlayerStat (MyChar, StatID, 0)
	if SkillWindow:
		SkillWindow.Close ()
	GemRB.SetNextScript("CharGen6")
	return

def NextPress():
	if CharGen:
		MyChar = GemRB.GetVar("Slot")

		# deal with racial boni too (skillrac.2da is ignored)
		RaceIndex = IDLUCommon.GetRace (MyChar)
		RaceName = CommonTables.Races.GetRowName (RaceIndex)
		# the column holds the index into feats.2da, which has one less intro column
		RaceColumn = CommonTables.Races.GetValue(RaceName, "SKILL_COLUMN") + 1
	else:
		MyChar = GemRB.GameGetSelectedPCSingle ()
		RacialBonus = 0

	#setting skills
	TmpTable = GemRB.LoadTable ("skillsta")
	SkillCount = TmpTable.GetRowCount ()
	for i in range (SkillCount):
		StatID=TmpTable.GetValue (i, 0, GTV_STAT)
		if CharGen:
			RacialBonus = SkillTable.GetValue (i, RaceColumn)
		newValue = GemRB.GetVar ("Skill "+str(i)) + RacialBonus
		GemRB.SetPlayerStat (MyChar, StatID, newValue)

	if SkillWindow:
		SkillWindow.Close ()
	if CharGen:
		GemRB.SetNextScript("Feats") #feats
	else:
		GemRB.SetPlayerStat (MyChar, IE_UNUSED_SKILLPTS, PointsLeft)
		# open up the next levelup window
		import Feats
		Feats.OpenFeatsWindow ()
	return
