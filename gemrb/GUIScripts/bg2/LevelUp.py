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
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
#
# $Id:$

# LevelUp.py - scripts to control the level up functionality and windows
import GemRB
from GUIDefines import *
from ie_stats import *
from GUICommon import GameIsTOB
from GUIREC import GetStatOverview, UpdateRecordsWindow, GetActorClassTitle, GetKitIndex

LevelUpWindow = None
TopIndex = 0
SkillTopIndex = 0
DoneButton = 0
TextAreaControl = 0
ScrollBarControl = 0
WeapProfTable = 0
SkillTable = 0
ClassTable = 0
ProfColumn = 0
InfoCounter = 1
ProfPointsLeft = 0
NewProfPoints = 0
SkillPointsLeft = 0
NewSkillPoints = 0
ClickCount = 0
OldDirection = 0
OldPos = 0
LevelDiff = 0
Level = 0

# TODO: multiclass support
def OpenLevelUpWindow():
	global LevelUpWindow, TextAreaControl, ProfPointsLeft, NewProfPoints
	global TopIndex, ScrollBarControl, DoneButton, WeapProfTable, ProfColumn
	global SkillTable, SkillPointsLeft, NewSkillPoints, KitName, LevelDiff
	global ClassTable, Level

	LevelUpWindow = GemRB.LoadWindowObject (3)

	InfoButton = LevelUpWindow.GetControl (125)
	InfoButton.SetText (13707)
	InfoButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, "LevelUpInfoPress")

	DoneButton = LevelUpWindow.GetControl (0)
	DoneButton.SetText (11962)
	DoneButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, "LevelUpDonePress")
	DoneButton.SetState (IE_GUI_BUTTON_DISABLED)
	DoneButton.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)

	# hide "Character Generation"
	Label = LevelUpWindow.CreateLabel (0x1000007e, 0,0,0,0,"NUMBER","",1)

	# name
	pc = GemRB.GameGetSelectedPCSingle ()
	Label = LevelUpWindow.GetControl (0x10000000+90)
	Label.SetText (GemRB.GetPlayerName (pc))

	# class
	Label = LevelUpWindow.GetControl (0x10000000+106)
	Label.SetText (GetActorClassTitle (pc))

	ClassTable = GemRB.LoadTableObject("classes")
	Class = GemRB.GetPlayerStat (pc, IE_CLASS)
	ClassIndex = ClassTable.FindValue (5, Class)
	KitList = GemRB.LoadTableObject("kitlist")
	ClassName = ClassTable.GetRowName(ClassIndex)
	Kit = GetKitIndex (pc)
	WeapProfTable = GemRB.LoadTableObject ("weapprof")
	if Kit == 0:
		KitName = ClassName
		# sorcerers are missing from weapprof
		if ClassName == "SORCERER":
			ProfColumn = WeapProfTable.GetColumnIndex ("MAGE")
		else:
			ProfColumn = WeapProfTable.GetColumnIndex (ClassName)
	else:
		#rowname is just a number, the kitname is the first data column
		KitName = KitList.GetValue(Kit, 0)
		#this is the proficiency column number in kitlist
		ProfColumn = KitList.GetValue(Kit, 5)

	ProfCountTable = GemRB.LoadTableObject("profs")
	# we don't care about the current level, but about the to-be-achieved one
	# FIXME: get the correct xp/level for MC and DC chars
	Level = GetNextLevelFromExp(GemRB.GetPlayerStat (pc, IE_XP), ClassName)
	LevelDiff = Level - GemRB.GetPlayerStat (pc, IE_LEVEL)
	ProfPointsLeft = LevelDiff/ProfCountTable.GetValue(ClassName, "RATE")
	NewProfPoints = ProfPointsLeft

	SkillTable = GemRB.LoadTableObject("skills")
	SkillPointsLeft = LevelDiff * SkillTable.GetValue("RATE", KitName)
	if SkillPointsLeft < 0:
		# no entry for this kit/class (the default value is -1)
		SkillPointsLeft = 0
	else:
		# get the skill values
		# TODO: show skillbrd, skillrng, skilldex? monks (same as rangers?)?!
		for i in range(SkillTable.GetRowCount()-2):
			SkillName = SkillTable.GetValue (i+2, 2)
			SkillValue = GemRB.GetPlayerStat (pc, SkillName)
			GemRB.SetVar("Skill "+str(i), SkillValue)
	NewSkillPoints = SkillPointsLeft

	HLACount = 0
	if GameIsTOB():
		HLATable = GemRB.LoadTableObject("lunumab")
		if HLATable.GetValue (KitName, "FIRST_LEVEL") < Level: # the new level
			HLACount = LevelDiff / HLATable.GetValue (ClassName, "RATE")
	HLAButton = LevelUpWindow.GetControl (126)
	if HLACount:
		HLAButton.SetText (4954)
		HLAButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, "LevelUpHLAPress")
	else:
		HLAButton.SetFlags (IE_GUI_BUTTON_DISABLED, OP_OR)

	RowCount = WeapProfTable.GetRowCount()-7  #we decrease it with the bg1 skills

	GemRB.SetVar("TopIndex",0)
	ScrollBarControl = LevelUpWindow.GetControl(108)
	ScrollBarControl.SetEvent(IE_GUI_SCROLLBAR_ON_CHANGE, "ProfScrollBarPress")
	ScrollBarControl.SetDefaultScrollBar ()
	ProfCount = RowCount - 7 # decrease it with the number of controls
	# decrease it with the number of invalid proficiencies
	for i in range(RowCount):
		SkillName = WeapProfTable.GetValue (i+8, 1)
		if SkillName == -1:
			ProfCount -= 1
		GemRB.SetVar("Prof "+str(i), GemRB.GetPlayerStat (pc, WeapProfTable.GetValue (i+8, 0)))
	ScrollBarControl.SetVarAssoc ("TopIndex", ProfCount)

	for i in range(7):
		Button=LevelUpWindow.GetControl(i+112)
		Button.SetVarAssoc("Prof", i)
		Button.SetEvent(IE_GUI_BUTTON_ON_PRESS, "JustPress")

		Button=LevelUpWindow.GetControl(i*2+1)
		Button.SetVarAssoc("Prof", i)
		Button.SetEvent(IE_GUI_BUTTON_ON_PRESS, "LeftPress")

		Button=LevelUpWindow.GetControl(i*2+2)
		Button.SetVarAssoc("Prof", i)
		Button.SetEvent(IE_GUI_BUTTON_ON_PRESS, "RightPress")

		for j in range(5):
			Star=LevelUpWindow.GetControl(i*5+j+48)

	for i in range(4):
		Button = LevelUpWindow.GetControl(i+120)
		Button.SetVarAssoc("Skill",i)
		Button.SetEvent(IE_GUI_BUTTON_ON_PRESS, "SkillJustPress")

		Button = LevelUpWindow.GetControl(i*2+17)
		Button.SetVarAssoc("Skill",i)
		Button.SetEvent(IE_GUI_BUTTON_ON_PRESS, "SkillLeftPress")

		Button = LevelUpWindow.GetControl(i*2+18)
		Button.SetVarAssoc("Skill",i)
		Button.SetEvent(IE_GUI_BUTTON_ON_PRESS, "SkillRightPress")

	TextAreaControl = LevelUpWindow.GetControl(110)
	TextAreaControl.SetText(GetLevelUpNews())

	TopIndex = 0
	RedrawSkills(1)
	GemRB.SetRepeatClickFlags (GEM_RK_DISABLE, OP_NAND)
	LevelUpWindow.ShowModal (MODAL_SHADOW_GRAY)
	return

# TODO: sorcerer spell selection
def RedrawSkills(First=0, direction=0):
	global TopIndex, ScrollBarControl, DoneButton, LevelUpWindow, ProfPointsLeft
	global SkillPointsLeft, SkillTable, NewSkillPoints
	global ClickCount, OldDirection

	#TODO: and not a sorcerer
	#TODO: and no more hlas
	if ProfPointsLeft == 0 and SkillPointsLeft == 0:
		DoneButton.SetState(IE_GUI_BUTTON_ENABLED)

	# skill part of the window
	SkillSumLabel = LevelUpWindow.GetControl(0x10000000+37)
	if NewSkillPoints == 0:
		SkillSumLabel.SetText("")
		# FIXME: hide the scrollbar
		#LevelUpWindow.DeleteControl(109)
		for i in range(4):
			Label = LevelUpWindow.GetControl (0x10000000+32+i)
			Label.SetText ("")
			Button1 = LevelUpWindow.GetControl(i*2+17)
			Button1.SetState(IE_GUI_BUTTON_DISABLED)
			Button1.SetFlags(IE_GUI_BUTTON_NO_IMAGE,OP_OR)
			Button2 = LevelUpWindow.GetControl(i*2+18)
			Button2.SetState(IE_GUI_BUTTON_DISABLED)
			Button2.SetFlags(IE_GUI_BUTTON_NO_IMAGE,OP_OR)
			Label = LevelUpWindow.GetControl(0x10000000+43+i)
			Label.SetText("")
	else:
		SkillSumLabel.SetText(str(SkillPointsLeft))
		for i in range(4):
			SkillName = SkillTable.GetValue (i+2, 1)
			Label = LevelUpWindow.GetControl (0x10000000+32+i)
			Label.SetText (SkillName)
	
			SkillName = SkillTable.GetRowName (i+2)
			Ok = SkillTable.GetValue (SkillName, KitName)
			Button1 = LevelUpWindow.GetControl(i*2+17)
			Button2 = LevelUpWindow.GetControl(i*2+18)
			if Ok == 0:
				Button1.SetState(IE_GUI_BUTTON_DISABLED)
				Button2.SetState(IE_GUI_BUTTON_DISABLED)
				Button1.SetFlags(IE_GUI_BUTTON_NO_IMAGE,OP_OR)
				Button2.SetFlags(IE_GUI_BUTTON_NO_IMAGE,OP_OR)
			else:
				Button1.SetState(IE_GUI_BUTTON_ENABLED)
				Button2.SetState(IE_GUI_BUTTON_ENABLED)
				Button1.SetFlags(IE_GUI_BUTTON_NO_IMAGE,OP_NAND)
				Button2.SetFlags(IE_GUI_BUTTON_NO_IMAGE,OP_NAND)
			
			Label = LevelUpWindow.GetControl(0x10000000+43+i)
			ActPoint = GemRB.GetVar("Skill "+str(i) )
			Label.SetText(str(ActPoint))

	# proficiencies part of the window
	ProfSumLabel = LevelUpWindow.GetControl(0x10000000+36)
	ProfSumLabel.SetText(str(ProfPointsLeft))
	SkipProfs = []
	for i in range(7):
		Pos=TopIndex+i
		SkillName = WeapProfTable.GetValue(Pos+8, 1) #we add the bg1 skill count offset
		MaxProf = WeapProfTable.GetValue(Pos+8, ProfColumn) #we add the bg1 skill count offset

		#invalid entry, adjusting scrollbar
		if SkillName == -1:
			GemRB.SetVar("TopIndex",TopIndex)
			ScrollBarControl.SetVarAssoc("TopIndex",Pos-7)
			break

		Button1=LevelUpWindow.GetControl(i*2+1)
		Button2=LevelUpWindow.GetControl(i*2+2)
		if MaxProf == 0:
			Button1.SetState(IE_GUI_BUTTON_DISABLED)
			Button2.SetState(IE_GUI_BUTTON_DISABLED)
			Button1.SetFlags(IE_GUI_BUTTON_NO_IMAGE,OP_OR)
			Button2.SetFlags(IE_GUI_BUTTON_NO_IMAGE,OP_OR)
			# skip proficiencies only if all the previous ones were skipped too
			if i == 0 or ((i-1) in SkipProfs):
				SkipProfs.append(i)
		else:
			Button1.SetState(IE_GUI_BUTTON_ENABLED)
			Button2.SetState(IE_GUI_BUTTON_ENABLED)
			Button1.SetFlags(IE_GUI_BUTTON_NO_IMAGE,OP_NAND)
			Button2.SetFlags(IE_GUI_BUTTON_NO_IMAGE,OP_NAND)
		
		Label=LevelUpWindow.GetControl(0x10000000+24+i)
		Label.SetText(SkillName)

		ActPoint = GemRB.GetVar("Prof "+str(Pos) )
		for j in range(5):  #5 is maximum distributable
			Star=LevelUpWindow.GetControl(i*5+j+48)
			if ActPoint > j:
				Star.SetFlags(IE_GUI_BUTTON_NO_IMAGE,OP_NAND)
			else:
				Star.SetFlags(IE_GUI_BUTTON_NO_IMAGE,OP_OR)

	# skip unavaliable proficiencies on the first run
	if len(SkipProfs) > 0 and First == 1:
		TopIndex += SkipProfs[len(SkipProfs)-1] + 1
		GemRB.SetVar("TopIndex",TopIndex)
		RedrawSkills()

	if direction:
		if OldDirection == direction:
			ClickCount = ClickCount + 1
			if ClickCount>10:
				GemRB.SetRepeatClickFlags(GEM_RK_DOUBLESPEED, OP_OR)
			return

	OldDirection = direction
	ClickCount = 0
	GemRB.SetRepeatClickFlags(GEM_RK_DOUBLESPEED, OP_NAND)

	return

# TODO: constructs a string with the gains that the new levels bring
def GetLevelUpNews():
	global NewProfPoints, NewSkillPoints

	News = GemRB.GetString (5259) + '\n\n'
	# FIXME: order

	# 5293 - Additional Hit Points Gained
	hp = 0
	Class = GemRB.GetPlayerStat (GemRB.GameGetSelectedPCSingle (), IE_CLASS)
	HPTable = ClassTable.GetValue (ClassTable.FindValue (5, Class), 6)
	# HACK: fake mc/dc support
	if HPTable == "*":
		HPTable = "HPROG"
	HPTable = GemRB.LoadTableObject (HPTable)
	for level in range(Level - LevelDiff, Level):
		rolls = HPTable.GetValue (level, 1)
		bonus = HPTable.GetValue (level, 2)
		if rolls:
			hp += GemRB.Roll (rolls, HPTable.GetValue (level, 0), bonus)
		else:
			hp += bonus
	News += GemRB.GetString (5293) + ": " + str(hp) + '\n\n'

	# saving throws
		# 5277 death
		# 5278 wand
		# 5279 polymorph
		# 5282 breath
		# 5292 spell

	# 5271 - Additional weapon proficiencies
	if NewProfPoints > 0:
		News += GemRB.GetString (5271) + ": " + str(NewProfPoints) + '\n\n'

	# new spell slots
		# 5373 - Additional Priest Spells
		# 5374 - Additional Mage Spells
		#? 10345 - Level <SPELLLEVEL> (used in stats_overview for bonus priest spells)
		#? 11293 - <CLASS>: Level <LEVEL>
		#? 61269 - Level <LEVEL> Spells
	# 5261 - Regained abilities from inactive class
	# 5305 - THAC0 Reduced by
	# 5375 - Backstab Multiplier Increased by
	# 5376 - Lay On Hands Increased by
	# 5377 - Lore Increased by
	# 5378 - Additional Skill Points
	if NewSkillPoints > 0:
		News += GemRB.GetString (5378) + ": " + str(NewSkillPoints) + '\n'
	return News

def LevelUpInfoPress():
	global LevelUpWindow, TextAreaControl, InfoCounter, LevelDiff

	if InfoCounter % 2:
		pc = GemRB.GameGetSelectedPCSingle ()
		# call GetStatOverview with the new levels, so the future overview is shown
		# TODO: also take in effect the current prof and skill selection
		TextAreaControl.SetText(GetStatOverview(pc, LevelDiff))
	else:
		TextAreaControl.SetText(GetLevelUpNews())
	InfoCounter += 1
	return

def LevelUpDonePress():
	global SkillTable
	# TODO: save the results
	# proficiencies
	TmpTable=GemRB.LoadTableObject ("weapprof")
	ProfCount = TmpTable.GetRowCount ()
	for i in range(ProfCount-8): # skip bg1 weapprof.2da proficiencies
		StatID = TmpTable.GetValue (i+8, 0)
		Value = GemRB.GetVar ("Prof "+str(i))
		if Value:
			GemRB.ApplyEffect (GemRB.GameGetSelectedPCSingle (), "Proficiency", Value, StatID )

	# skills
	pc = GemRB.GameGetSelectedPCSingle ()
	for i in range(SkillTable.GetRowCount()-2):
		SkillName = SkillTable.GetValue (i+2, 2)
		SkillValue = GemRB.GetVar ("Skill "+str(i))
		GemRB.SetPlayerStat (pc, SkillName, SkillValue )

	# hlas
	# level, xp and other stuff by the core?
	
	if LevelUpWindow:
		LevelUpWindow.Unload()
	GemRB.SetRepeatClickFlags (GEM_RK_DISABLE, OP_OR)
	UpdateRecordsWindow()
	return

# TODO: MC and DC support
def GetNextLevelFromExp (XP, Class):
	NextLevelTable = GemRB.LoadTableObject ("XPLEVEL")
	Row = NextLevelTable.GetRowIndex (Class)
	for i in range(1, NextLevelTable.GetColumnCount()-1):
		if XP < NextLevelTable.GetValue (Row, i):
			return i
	# fix hacked characters that have more xp than the xp cap
	return 40

def SkillJustPress():
	Pos = GemRB.GetVar("Skill")+SkillTopIndex
	TextAreaControl.SetText(SkillTable.GetValue(Pos+2,0) )
	return

def SkillRightPress():
	global SkillPointsLeft, ClickCount, OldPos

	Pos = GemRB.GetVar("Skill")+SkillTopIndex
	TextAreaControl.SetText(SkillTable.GetValue(Pos+2,0) )
	ActPoint = GemRB.GetVar("Skill "+str(Pos) )
	if ActPoint <= 0:
		return
	GemRB.SetVar("Skill "+str(Pos),ActPoint-1)
	SkillPointsLeft = SkillPointsLeft + 1
	if OldPos != Pos:
		OldPos = Pos
		ClickCount = 0

	RedrawSkills(0,2)
	return

def SkillLeftPress():
	global SkillPointsLeft, ClickCount, OldPos

	Pos = GemRB.GetVar("Skill")+SkillTopIndex
	TextAreaControl.SetText(SkillTable.GetValue(Pos+2,0) )
	if SkillPointsLeft == 0:
		return
	ActPoint = GemRB.GetVar("Skill "+str(Pos) )
	if ActPoint >= 200:
		return
	GemRB.SetVar("Skill "+str(Pos), ActPoint+1)
	SkillPointsLeft = SkillPointsLeft - 1
	if OldPos != Pos:
		OldPos = Pos
		ClickCount = 0

	RedrawSkills(0,1)
	return

def ProfScrollBarPress():
	global TopIndex

	TopIndex = GemRB.GetVar("TopIndex")
	RedrawSkills()
	return

def JustPress():
	global TextAreaControl
	Pos = GemRB.GetVar("Prof")+TopIndex
	TextAreaControl.SetText(WeapProfTable.GetValue(Pos+8, 2) )
	return
	
def RightPress():
	global ProfPointsLeft

	Pos = GemRB.GetVar("Prof")+TopIndex
	TextAreaControl.SetText(WeapProfTable.GetValue(Pos+8, 2) )
	ActPoint = GemRB.GetVar("Prof "+str(Pos) )
	if ActPoint <= 0:
		return
	GemRB.SetVar("Prof "+str(Pos),ActPoint-1)
	ProfPointsLeft = ProfPointsLeft + 1
	DoneButton.SetState(IE_GUI_BUTTON_DISABLED)
	RedrawSkills()
	return

def LeftPress():
	global ProfPointsLeft

	Pos = GemRB.GetVar("Prof")+TopIndex
	TextAreaControl.SetText(WeapProfTable.GetValue(Pos+8, 2) )
	if ProfPointsLeft == 0:
		return
	MaxProf = WeapProfTable.GetValue(Pos+8, ProfColumn) #we add the bg1 skill count offset
	if MaxProf>5:
		MaxProf = 5

	ActPoint = GemRB.GetVar("Prof "+str(Pos) )
	if ActPoint >= MaxProf:
		return
	GemRB.SetVar("Prof "+str(Pos),ActPoint+1)
	ProfPointsLeft = ProfPointsLeft - 1
	RedrawSkills()
	return