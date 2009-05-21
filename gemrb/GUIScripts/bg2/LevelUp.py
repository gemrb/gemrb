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
# $Id:$

# LevelUp.py - scripts to control the level up functionality and windows
import GemRB
from GUIDefines import *
from ie_stats import *
from ie_restype import RES_2DA
from GUICommon import HasTOB, GetLearnablePriestSpells, GetMageSpells, HasSpell, AddClassAbilities
from GUIREC import GetStatOverview, UpdateRecordsWindow, GetActorClassTitle
from GUICommonWindows import IsDualClassed, IsMultiClassed, IsDualSwap, GetKitIndex
from LUSpellSelection import *
from LUHLASelection import *

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
Classes = 0
NumClasses = 0
DualSwap = 0
KitName = 0
IsDual = 0
IsMulti = 0
pc = 0
ClassSkillsTable = 0
ClassName = 0
RaceTable = 0
AvailableSkillIndices = []

# old values (so we don't add too much)
OldHP = 0		# << old current hitpoints
OldHPMax = 0		# << old maximum hitpoints
OldSaves = [0]*5	# << old saves
OldThaco = 0		# << old thac0 value
OldLore = 0		# << old lore value
OldDSpells = [0]*7	# << old divine spells per level
OldWSpells = [0]*9	# << old wizard spells per level
NewDSpells = [0]*7	# << new divine spells per level
NewWSpells = [0]*9	# << new wizard spells per level
DeltaDSpells = 0	# << total new divine spells
DeltaWSpells = 0	# << total new wizard spells

def OpenLevelUpWindow():
	global LevelUpWindow, TextAreaControl, ProfPointsLeft, NewProfPoints
	global TopIndex, ScrollBarControl, DoneButton, WeapProfTable, ProfColumn
	global SkillTable, SkillPointsLeft, NewSkillPoints, KitName, LevelDiff, RaceTable
	global ClassTable, Level, Classes, NumClasses, DualSwap, ClassSkillsTable, IsMulti
	global OldHP, OldHPMax, OldSaves, OldLore, OldThaco, DeltaDSpells, DeltaWSpells
	global NewDSpells, NewWSpells, OldDSpells, OldWSpells, pc, HLACount, ClassName, IsDual
	global AvailableSkillIndices

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

	# some current values
	OldHP = GemRB.GetPlayerStat (pc, IE_HITPOINTS, 1)
	OldHPMax = GemRB.GetPlayerStat (pc, IE_MAXHITPOINTS, 1)
	OldThaco = GemRB.GetPlayerStat (pc, IE_THAC0, 1)
	OldLore = GemRB.GetPlayerStat (pc, IE_LORE, 1)
	for i in range (5):
		OldSaves[i] = GemRB.GetPlayerStat (pc, IE_SAVEVSDEATH+i, 1)

	# class
	Label = LevelUpWindow.GetControl (0x10000000+106)
	Label.SetText (GetActorClassTitle (pc))

	ClassTable = GemRB.LoadTableObject("classes")
	Class = GemRB.GetPlayerStat (pc, IE_CLASS)
	print "Class:",Class
	ClassIndex = ClassTable.FindValue (5, Class)
	ClassSkillsTable = GemRB.LoadTableObject ("clskills")
	SkillTable = GemRB.LoadTableObject("skills")

	# kit
	KitList = GemRB.LoadTableObject("kitlist")
	ClassName = ClassTable.GetRowName(ClassIndex)
	Kit = GetKitIndex (pc)
	WeapProfTable = GemRB.LoadTableObject ("weapprof")
	
	# need this for checking gnomes
	RaceTable = GemRB.LoadTableObject ("races")
	RaceName = GemRB.GetPlayerStat (pc, IE_RACE, 1)
	print "IE_RACE:",RaceName
	RaceName = RaceTable.FindValue (3, RaceName)
	RaceName = RaceTable.GetRowName (RaceName)
	print "Race:",RaceName

	# figure our our proficiency table and index
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

	# our multiclass variables
	IsMulti = IsMultiClassed (pc, 1)
	Classes = [IsMulti[1], IsMulti[2], IsMulti[3]]
	NumClasses = IsMulti[0] # 2 or 3 if IsMulti; 0 otherwise
	IsMulti = NumClasses > 1
	IsDual = 0
	
	# not multi, check dual
	if not IsMulti:
		print "Not Multi"

		# check if we're dual classed
		IsDual = IsDualClassed (pc, 1)
		Classes = [IsDual[2], IsDual[1]] # make sure the new class is first

		# either dual or single only care about 1 class
		NumClasses = 1
		
		# not dual, must be single
		if IsDual[0] == 0:
			print "Not Dual"
			Classes = [Class]
		else: # make sure Classes[1] is a class, not a kit
			if IsDual[0] == 1: # kit
				Classes[1] = KitList.GetValue (IsDual[1], 7)
			else: # class
				Classes[1] = ClassTable.GetValue (Classes[1], 5)

		# store a boolean for IsDual
		IsDual = IsDual[0] > 0

	Level = [0]*3
	LevelDiff = [0]*3

	# reorganize the leves if we're dc so the one we want to use is in Level[0]
	# and the old one is in Level[1] (used to regain old class abilities)
	if IsDual:
		# convert the classes from indicies to class id's
		DualSwap = IsDualSwap (pc)
		ClassName = ClassTable.GetRowName (Classes[0])
		KitName = ClassName # for simplicity throughout the code
		Classes[0] = ClassTable.GetValue (Classes[0], 5)
		# Class[1] is taken care of above
		ProfColumn = WeapProfTable.GetColumnIndex (ClassName) # or we'll get the multi progression


		# we need the old level as well
		if DualSwap:
			Level[1] = GemRB.GetPlayerStat (pc, IE_LEVEL)
		else:
			Level[1] = GemRB.GetPlayerStat (pc, IE_LEVEL2)

	hp = 0
	FastestProf = 0
	FastestRate = 100
	SkillIndex = -1
	HaveCleric = 0
	DeltaWSpells = 0
	DeltaDSpells = 0

	# get a bunch of different things each level
	for i in range(NumClasses):
		print "Class:",Classes[i]
		# we don't care about the current level, but about the to-be-achieved one
		# TODO: check MC (should be working) and DC (iffy) functionality
		# get the next level
		Level[i] = GetNextLevelFromExp (GemRB.GetPlayerStat (pc, IE_XP)/NumClasses, Classes[i])
		TmpIndex = ClassTable.FindValue (5, Classes[i])
		TmpName = ClassTable.GetRowName (TmpIndex) 

		print "Name:",TmpName

		# find the level diff for each classes (3 max, obviously)
		if i == 0:
			if DualSwap:
				LevelDiff[i] = Level[i] - GemRB.GetPlayerStat (pc, IE_LEVEL2)
			else:
				LevelDiff[i] = Level[i] - GemRB.GetPlayerStat (pc, IE_LEVEL)
		elif i == 1:
			LevelDiff[i] = Level[i] - GemRB.GetPlayerStat (pc, IE_LEVEL2)
		elif i == 2:
			LevelDiff[i] = Level[i] - GemRB.GetPlayerStat (pc, IE_LEVEL3)

		print "Level (",i,"):",Level[i]
		print "Level Diff (",i,"):",LevelDiff[i]

		# check this classes hp table for any gain
		HPTable = ClassTable.GetValue (TmpIndex, 6, 0)
		HPTable = GemRB.LoadTableObject (HPTable)

		# loop through each level gained
		for level in range(Level[i] - LevelDiff[i], Level[i]):
			# TODO: figure out how multiclass and dualclass rolls are done
			rolls = HPTable.GetValue (level, 1)
			bonus = HPTable.GetValue (level, 2)

			# we only do a roll if core diff or higher, or uncheck max
			if rolls:
				if GemRB.GetVar ("Difficulty Level") < 3:
					hp += GemRB.Roll (rolls, HPTable.GetValue (level, 0), bonus) / NumClasses
				else:
					hp += (rolls * HPTable.GetValue (level, 0) + bonus) / NumClasses
			else:
				hp += bonus / NumClasses

		# save our current and next spell amounts
		StartLevel = Level[i] - LevelDiff[i]
		DruidTable = ClassSkillsTable.GetValue (Classes[i], 0, 0)
		ClericTable = ClassSkillsTable.GetValue (Classes[i], 1, 0)
		MageTable = ClassSkillsTable.GetValue (Classes[i], 2, 0)
		
		# see if we have mage spells
		if MageTable != "*":
			# we get 1 extra spell per level if we're a specialist
			Specialist = 0
			if KitList.GetValue (Kit, 7) == 1: # see if we're a kitted mage (TODO: make gnomes ALWAYS be kitted, even multis)
				Specialist = 1
			MageTable = GemRB.LoadTableObject (MageTable)
			# loop through each spell level and save the amount possible to cast (current)
			for j in range (MageTable.GetColumnCount ()):
				NewWSpells[j] = MageTable.GetValue (str(Level[i]), str(j+1), 1)
				OldWSpells[j] = MageTable.GetValue (str(StartLevel), str(j+1), 1)
				if NewWSpells[j] > 0: # don't want specialist to get 1 in levels they should have 0
					NewWSpells[j] += Specialist
				if OldWSpells[j] > 0:
					OldWSpells[j] += Specialist
			DeltaWSpells = sum(NewWSpells)-sum(OldWSpells)
		elif ClericTable != "*":
			# check for cleric spells
			ClericTable = GemRB.LoadTableObject (ClericTable)
			HaveCleric = 1
			# same as above
			for j in range (ClericTable.GetColumnCount ()):
				NewDSpells[j] = ClericTable.GetValue (str(Level[i]), str(j+1), 1)
				OldDSpells[j] = ClericTable.GetValue (str(StartLevel), str(j+1), 1)
			DeltaDSpells = sum(NewDSpells)-sum(OldDSpells)
		elif DruidTable != "*" and GemRB.HasResource (DruidTable, RES_2DA):
			# clerics have precedence in multis (ranger/cleric)
			if HaveCleric == 0:
				# check druid spells
				DruidTable = GemRB.LoadTableObject (DruidTable)
				# same as above
				for j in range (DruidTable.GetColumnCount ()):
					NewDSpells[j] = DruidTable.GetValue (str(Level[i]), str(j+1), 1)
					OldDSpells[j] = DruidTable.GetValue (str(StartLevel), str(j+1), 1)
				DeltaDSpells = sum(NewDSpells)-sum(OldDSpells)

		# check for a better proficiency rate
		TmpRate = ProfCountTable.GetValue(Classes[i]-1, 1)
		if (TmpRate < FastestRate): # rate is how often you get it; therefore less is more
			FastestProf = i
			FastestRate = TmpRate

		# see if we have a thief (or monk)
		if (ClassSkillsTable.GetValue (Classes[i], 5, 0) != "*"):
			SkillIndex = i

		# setup class bonuses for this class
		if IsMulti or IsDual or Kit == 0:
			ABTable = ClassSkillsTable.GetValue (TmpName, "ABILITIES")
		else: # single-classed with a kit
			ABTable = KitList.GetValue (str(Kit), "ABILITIES")

		# add the abilites if we aren't a mage and have a table to ref
		if ABTable != "*" and ABTable[:6] != "CLABMA":
			AddClassAbilities (pc, ABTable, Level[i], LevelDiff[i])

	# save our new hp if it changed
	if (OldHPMax == GemRB.GetPlayerStat (pc, IE_MAXHITPOINTS, 1)):
		GemRB.SetPlayerStat (pc, IE_MAXHITPOINTS, OldHPMax + hp)
		GemRB.SetPlayerStat (pc, IE_HITPOINTS, OldHP + hp)
	
	# save our proficiencies
	ProfPointsLeft = LevelDiff[FastestProf]/FastestRate
	NewProfPoints = ProfPointsLeft

	# setup the indices/count of usable skills
	AvailableSkillIndices = []
	for i in range(SkillTable.GetRowCount()-2):
		SkillName = SkillTable.GetRowName (i+2)
		if SkillTable.GetValue (SkillName, KitName) != -1:
			AvailableSkillIndices.append(i)

	# see if we got a thief (or monk)
	if SkillIndex > -1:
		# KitName should be fine as all multis are in classes.2da
		# also allows for thief kits
		SkillPointsLeft = LevelDiff[SkillIndex] * SkillTable.GetValue("RATE", KitName)
		if SkillPointsLeft < 0:
			# really don't have an entry
			SkillPointsLeft = 0
		else:
			# get the skill values
			# TODO: get upgrades from clskills -> skill{brd,rng}
			for i in range(SkillTable.GetRowCount()-2):
				SkillID = SkillTable.GetValue (i+2, 2)
				SkillValue = GemRB.GetPlayerStat (pc, SkillID, 1)
				GemRB.SetVar("Skill "+str(i), SkillValue)
				GemRB.SetVar("SkillBase "+str(i), SkillValue)

	NewSkillPoints = SkillPointsLeft

	# use total levels for HLAs
	HLACount = 0
	if HasTOB(): # make sure SoA doesn't try to get it
		HLATable = GemRB.LoadTableObject("lunumab")
		if IsMulti:
			# we need to check each level against a multi value (this is kinda screwy)
			CanLearnHLAs = 1
			for i in range (NumClasses):
				# get the row name for lookup ex. MULTI2FIGHTER, MULTI3THIEF
				MultiName = ClassTable.FindValue (5, Classes[i])
				MultiName = ClassTable.GetRowName (MultiName)
				MultiName = "MULTI" + str(NumClasses) + MultiName

				print "HLA Multiclass:",MultiName

				# if we can't learn for this class, we can't learn at all
				if Level[i] < HLATable.GetValue (MultiName, "FIRST_LEVEL", 1):
					CanLearnHLAs = 0
					break
			
			# update the HLA count if we can learn them
			if CanLearnHLAs:
				HLACount = sum (LevelDiff) / HLATable.GetValue (ClassName, "RATE", 1)
		else: # dual or single classed
			print "HLA Class:",ClassName
			if HLATable.GetValue (ClassName, "FIRST_LEVEL", 1) <= Level[0]:
				HLACount = LevelDiff[0] / HLATable.GetValue (ClassName, "RATE", 1)

		# set values required by the hla level up code
		GemRB.SetVar ("HLACount", HLACount)
	HLAButton = LevelUpWindow.GetControl (126)
	if HLACount:
		HLAButton.SetText (4954)
		HLAButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, "LevelUpHLAPress")
	else:
		HLAButton.SetFlags (IE_GUI_BUTTON_DISABLED, OP_OR)

	RowCount = WeapProfTable.GetRowCount()-7  #we decrease it with the bg1 skills

	# proficiencies scrollbar
	GemRB.SetVar ("TopIndex", 0)
	ScrollBarControl = LevelUpWindow.GetControl(108)
	ScrollBarControl.SetEvent(IE_GUI_SCROLLBAR_ON_CHANGE, "ProfScrollBarPress")
	ScrollBarControl.SetDefaultScrollBar ()
	ProfCount = RowCount - 7 # decrease it with the number of controls

	# decrease it with the number of invalid proficiencies
	for i in range(RowCount):
		SkillName = WeapProfTable.GetValue (i+8, 1)
		if SkillName > 0x1000000 or SkillName < 0:
			ProfCount -= 1

		# we only need the low 3 bits for profeciencies
		# TODO: use the 6 bits the actual system uses for dual classes
		#	this code will remain compatible in the meantime
		currentprof = GemRB.GetPlayerStat (pc, WeapProfTable.GetValue (i+8, 0))&0x07
		GemRB.SetVar("Prof "+str(i), currentprof)
	ScrollBarControl.SetVarAssoc ("TopIndex", ProfCount)

	# skills scrollbar
	if len(AvailableSkillIndices) > 4:
		GemRB.SetVar ("SkillTopIndex", 0)
		ScrollBarControl = LevelUpWindow.GetControl (109)
		ScrollBarControl.SetEvent (IE_GUI_SCROLLBAR_ON_CHANGE, "SkillScrollBarPress")
		# decrease it with the number of controls on screen (list size) and two unrelated rows
		ScrollBarControl.SetVarAssoc ("SkillTopIndex", SkillTable.GetRowCount()-3-2)
	else:
		if len(AvailableSkillIndices):
			# autoscroll to the first valid skill; luckily all three monk ones are adjacent
			GemRB.SetVar ("SkillTopIndex", AvailableSkillIndices[0])

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

	for i in range(len(AvailableSkillIndices)):
		if i == 4:
			break
		Button = LevelUpWindow.GetControl(i+120)
		Button.SetVarAssoc("Skill",AvailableSkillIndices[i])
		Button.SetEvent(IE_GUI_BUTTON_ON_PRESS, "SkillJustPress")

		Button = LevelUpWindow.GetControl(i*2+17)
		Button.SetVarAssoc("Skill",AvailableSkillIndices[i])
		Button.SetEvent(IE_GUI_BUTTON_ON_PRESS, "SkillLeftPress")

		Button = LevelUpWindow.GetControl(i*2+18)
		Button.SetVarAssoc("Skill",AvailableSkillIndices[i])
		Button.SetEvent(IE_GUI_BUTTON_ON_PRESS, "SkillRightPress")

	TextAreaControl = LevelUpWindow.GetControl(110)
	TextAreaControl.SetText(GetLevelUpNews())

	TopIndex = 0
	RedrawSkills(1)
	GemRB.SetRepeatClickFlags (GEM_RK_DISABLE, OP_NAND)
	LevelUpWindow.ShowModal (MODAL_SHADOW_GRAY)
	
	# if we have a sorcerer who can learn spells, we need to do spell selection
	if (Classes[0] == 19) and (DeltaWSpells > 0): # open our sorc spell selection window
		OpenSpellsWindow (pc, "SPLSRCKN", Level[0], LevelDiff[0])

def HideSkills(i):
	global LevelUpWindow

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

def RedrawSkills(First=0, direction=0):
	global TopIndex, ScrollBarControl, DoneButton, LevelUpWindow, ProfPointsLeft
	global SkillPointsLeft, SkillTable, NewSkillPoints, AvailableSkillIndices
	global ClickCount, OldDirection, HLACount

	# we need to disable the HLA button if we don't have any HLAs left
	HLACount = GemRB.GetVar ("HLACount")
	if HLACount == 0:
		# turn the HLA button off
		HLAButton = LevelUpWindow.GetControl (126)
		HLAButton.SetState(IE_GUI_BUTTON_DISABLED)

	# enable the done button if they've allocated all points
	# sorcerer spell selection (if applicable) comes after hitting the done button
	if ProfPointsLeft == 0 and SkillPointsLeft == 0 and HLACount == 0:
		DoneButton.SetState(IE_GUI_BUTTON_ENABLED)

	# skill part of the window
	SkillSumLabel = LevelUpWindow.GetControl(0x10000000+37)
	if NewSkillPoints == 0:
		SkillSumLabel.SetText("")
		for i in range(4):
			HideSkills(i)
	else:
		SkillSumLabel.SetText(str(SkillPointsLeft))
		for i in range(4):
			if len(AvailableSkillIndices) <= i:
				HideSkills(i)
				continue
			Pos = AvailableSkillIndices[SkillTopIndex+i]
			SkillName = SkillTable.GetValue (Pos+2, 1)
			Label = LevelUpWindow.GetControl (0x10000000+32+i)
			Label.SetText (SkillName)

			SkillName = SkillTable.GetRowName (Pos+2)
			Ok = SkillTable.GetValue (SkillName, KitName)
			Button1 = LevelUpWindow.GetControl(i*2+17)
			Button2 = LevelUpWindow.GetControl(i*2+18)
			if Ok == -1:
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
			ActPoint = GemRB.GetVar("Skill "+str(Pos) )
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
		if SkillName > 0x1000000 or SkillName < 0:
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

	# display if our class has been reactivated
	if IsDual:
		if (Level[0] - LevelDiff[0]) <= Level[1] and Level[0] > Level[1]:
			News = GemRB.GetString (5261) + '\n\n'
	
	# 5271 - Additional weapon proficiencies
	if NewProfPoints > 0:
		News += GemRB.GetString (5271) + ": " + str(NewProfPoints) + '\n\n'

	# temps to compare all our new saves against (we get the best of our saving throws)
	ThacoTable = GemRB.LoadTableObject ("THAC0")
	LoreTable = GemRB.LoadTableObject ("lore")
	Saves = OldSaves
	NewThaco = OldThaco
	LoreGain = 0
	LOHGain = 0
	BackstabMult = 0

	for i in range(NumClasses):
		# get the class name
		TmpClassName = ClassTable.FindValue (5, Classes[i])
		TmpClassName = ClassTable.GetRowName (TmpClassName)

		# get each save from the table
		SaveTable = ClassTable.GetValue (TmpClassName, "SAVE", 0)
		SaveTable = GemRB.LoadTableObject (SaveTable)
		
		# loop through each save
		TmpIndex = Level[i]-1
		for j in range (5):
			# if the save is better, update the saves array
			TmpSave = SaveTable.GetValue (j, TmpIndex)
			if TmpSave < Saves[j]:
				Saves[j] = TmpSave

		# see if this classes has a lower thaco
		TmpThaco = ThacoTable.GetValue (Classes[i]-1, TmpIndex, 1)
		if (TmpThaco < NewThaco):
			NewThaco = TmpThaco

		# backstab
		# NOTE: Stalkers and assassins should get the correct mods at the correct levels based
		#	on AP_SPCL332 in their respective CLAB files.
		# APND: Stalers appear to get the correct mod at the correct levels; however, because they start
		#	at backstab multi x1, they are x1 too many at their respective levels.
		if Classes[i] == 4 and GemRB.GetPlayerStat (pc, IE_BACKSTABDAMAGEMULTIPLIER, 1) > 1: # we have a thief who can backstab (2 at level 1)
			# save the backstab multiplier if we have a thief
			BackstabTable = GemRB.LoadTableObject ("BACKSTAB")
			BackstabMult = BackstabTable.GetValue (0, Level[i])

		# lay on hands
		if (ClassSkillsTable.GetValue (Classes[i], 6) != "*"):
			# inquisitors and undead hunters don't get lay on hands out the chute, whereas cavaliers
			# and unkitted paladins do; therefore, we check for the existence of lay on hands to ensure
			# the character should get the new value; LoH is defined in GA_SPCL211 if anyone wants to
			# make a pally kit with LoH
			print "LoH Amount:",GemRB.GetPlayerStat (pc, IE_LAYONHANDSAMOUNT, 1) 
			if (HasSpell (pc, IE_SPELL_TYPE_INNATE, 0, "SPCL211") != -1):
				print "Doing a LoH lookup."
				LOHTable = GemRB.LoadTableObject ("layhands")
				LOHValue = LOHTable.GetValue (0, Level[i])
				LOHGain = LOHValue - LOHTable.GetValue (0, Level[i]-LevelDiff[i])

		# find the lore from each class
		# sorcerers are not defined in lore.2da, but get mage progression instead
		if TmpClassName == "SORCERER":
			TmpClassName = "MAGE"
		LoreClassIndex = LoreTable.GetRowIndex (TmpClassName)
		LoreGain = LoreGain + LoreTable.GetValue (LoreClassIndex, 0) * LevelDiff[i]

	# saving throws
		# 5277 death
		# 5278 wand
		# 5279 polymorph
		# 5282 breath
		# 5292 spell
	# include in news if the save is updated
	for i in range (5):
		GemRB.SetPlayerStat (pc, IE_SAVEVSDEATH+i, Saves[i]) # save the saves :D
		SaveString = 5277+i
		if i == 3:
			SaveString = 5282
		elif i == 4:
			SaveString = 5292

		if Saves[i] < OldSaves[i]:
			News += GemRB.GetString (SaveString) + ": " + str(OldSaves[i]-Saves[i]) + '\n'

	# 5305 - THAC0 Reduced by
	# only output if there is a change in thaco
	if (NewThaco < OldThaco):
		GemRB.SetPlayerStat (pc, IE_THAC0, NewThaco)
		News += GemRB.GetString (5305) + ": " + str(OldThaco-NewThaco) + '\n\n'

	# new spell slots
		# 5373 - Additional Priest Spells
		# 5374 - Additional Mage Spells
		#? 10345 - Level <SPELLLEVEL> (used in stats_overview for bonus priest spells)
		#? 11293 - <CLASS>: Level <LEVEL>
		#? 61269 - Level <LEVEL> Spells <- the one we use

	if DeltaDSpells > 0: # new divine spells
		News += GemRB.GetString (5373) + '\n'
		for i in range (len (NewDSpells)):
			# only display classes with new spells
			if (NewDSpells[i]-OldDSpells[i]) == 0:
				continue
			GemRB.SetToken("level", str(i+1))
			News += GemRB.GetString(61269)+": " + str(NewDSpells[i]-OldDSpells[i]) + '\n'
		News += '\n'
	if DeltaWSpells > 0: # new wizard spells
		News += GemRB.GetString (5374) + '\n'
		for i in range (len (NewWSpells)):
			# only display classes with new spells
			if (NewWSpells[i]-OldWSpells[i]) == 0:
				continue
			GemRB.SetToken("level", str(i+1))
			News += GemRB.GetString(61269)+": " + str(NewWSpells[i]-OldWSpells[i]) + '\n'
		News += '\n'

	# 5375 - Backstab Multiplier Increased by
	# this auto-updates... we just need to inform of the update
	if (BackstabMult > GemRB.GetPlayerStat (pc, IE_BACKSTABDAMAGEMULTIPLIER, 1)):
		News += GemRB.GetString (5375) + ": " + str(BackstabMult) + '\n\n'

	# 5376 - Lay on Hands increased
	if LOHGain > 0:
		GemRB.SetPlayerStat (pc, IE_LAYONHANDSAMOUNT, LOHValue)
		News += GemRB.GetString (5376) + ": " + str(LOHGain) + '\n\n'

	# 5293 - HP increased by
	if (OldHPMax != GemRB.GetPlayerStat (pc, IE_MAXHITPOINTS, 1)):
		News += GemRB.GetString (5293) + ": " + str(GemRB.GetPlayerStat (pc, IE_MAXHITPOINTS, 1) - OldHPMax) + '\n'

	# 5377 - Lore Increased by
	# add the lore gain if we haven't done so already
	if (LoreGain > 0) and (OldLore == GemRB.GetPlayerStat (pc, IE_LORE, 1)):
		GemRB.SetPlayerStat (pc, IE_LORE, OldLore + LoreGain)
		News += GemRB.GetString (5377) + ": " + str(LoreGain) + '\n\n'

	# 5378 - Additional Skill Points
	if NewSkillPoints > 0:
		News += GemRB.GetString (5378) + ": " + str(NewSkillPoints) + '\n'

	return News

def LevelUpInfoPress():
	global LevelUpWindow, TextAreaControl, InfoCounter, LevelDiff

	if InfoCounter % 2:
		# call GetStatOverview with the new levels, so the future overview is shown
		# TODO: show only xp, levels, thac0, #att, lore, reputation, backstab, saving throws
		TextAreaControl.SetText(GetStatOverview(pc, LevelDiff))
	else:
		TextAreaControl.SetText(GetLevelUpNews())
	InfoCounter += 1
	return

# save the results
def LevelUpDonePress():
	global SkillTable

	# proficiencies
	TmpTable=GemRB.LoadTableObject ("weapprof")
	ProfCount = TmpTable.GetRowCount ()
	for i in range(ProfCount-8): # skip bg1 weapprof.2da proficiencies
		StatID = TmpTable.GetValue (i+8, 0)
		Value = GemRB.GetVar ("Prof "+str(i))
		if Value:
			GemRB.ApplyEffect (pc, "Proficiency", Value, StatID )

	# skills
	for i in range(SkillTable.GetRowCount()-2):
		SkillName = SkillTable.GetValue (i+2, 2)
		SkillValue = GemRB.GetVar ("Skill "+str(i))
		GemRB.SetPlayerStat (pc, SkillName, SkillValue )
	
	# level
	if DualSwap: # swap the IE_LEVELs around if a backward dual
		GemRB.SetPlayerStat (pc, IE_LEVEL2, Level[0])
		GemRB.SetPlayerStat (pc, IE_LEVEL, Level[1])
	else:
		GemRB.SetPlayerStat (pc, IE_LEVEL, Level[0])
		GemRB.SetPlayerStat (pc, IE_LEVEL2, Level[1])
	GemRB.SetPlayerStat (pc, IE_LEVEL3, Level[2])

	print "Levels:",Level[0],"/",Level[1],"/",Level[2]

	# save our number of memorizable spells per level
	if DeltaWSpells > 0:
		# loop through each wizard spell level
		for i in range(len(NewWSpells)):
			if NewWSpells[i] > 0: # we have new spells, so update
				GemRB.SetMemorizableSpellsCount(pc, NewWSpells[i], IE_SPELL_TYPE_WIZARD, i)
	
	# save our number of memorizable priest spells
	if DeltaDSpells > 0: # druids and clerics count
		for i in range (len(NewDSpells)):
			# get each update
			if NewDSpells[i] > 0:
				GemRB.SetMemorizableSpellsCount (pc, NewDSpells[i], IE_SPELL_TYPE_PRIEST, i)

			# learn all the spells we're given, but don't have, up to our given casting level
			if GemRB.GetMemorizableSpellsCount (pc, IE_SPELL_TYPE_PRIEST, i, 1) > 0: # we can memorize spells of this level
				for j in range(NumClasses): # loop through each class
					IsDruid = ClassSkillsTable.GetValue (Classes[j], 0, 0)
					IsCleric = ClassSkillsTable.GetValue (Classes[j], 1, 0)
					if IsCleric == "*" and IsDruid == "*": # no divine spells (so mage/cleric multis don't screw up)
						continue
					elif IsCleric == "*": # druid spells
						ClassFlag = 0x8000
					else: # cleric spells
						ClassFlag = 0x4000

					Learnable = GetLearnablePriestSpells(ClassFlag, GemRB.GetPlayerStat (pc, IE_ALIGNMENT), i+1)
					for k in range(len(Learnable)): # loop through all the learnable spells
						if HasSpell (pc, IE_SPELL_TYPE_PRIEST, i, Learnable[k]) < 0: # only write it if we don't yet know it
							GemRB.LearnSpell(pc, Learnable[k])

	# hlas
	# level, xp and other stuff by the core?

	# 5261 - Regained abilities from inactive class
	if IsDual: # we're dual classed
		print "activation?"
		if (Level[0] - LevelDiff[0]) <= Level[1] and Level[0] > Level[1]: # our new classes now surpasses our old class
			print "reactivating base class"
			ReactivateBaseClass ()
	
	if LevelUpWindow:
		LevelUpWindow.Unload()
	UpdateRecordsWindow()

	GemRB.SetRepeatClickFlags (GEM_RK_DISABLE, OP_OR)
	return

def GetNextLevelFromExp (XP, Class):
	NextLevelTable = GemRB.LoadTableObject ("XPLEVEL")
	ClassTable = GemRB.LoadTableObject ("classes")
	ClassIndex = ClassTable.FindValue (5, Class)
	ClassName = ClassTable.GetRowName (ClassIndex)
	Row = NextLevelTable.GetRowIndex (ClassName)
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
	BasePoint = GemRB.GetVar("SkillBase "+str(Pos) )
	if ActPoint <= 0 or ActPoint == BasePoint:
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

def SkillScrollBarPress():
	global SkillTopIndex

	SkillTopIndex = GemRB.GetVar("SkillTopIndex")
	RedrawSkills()
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

# regains all the benifits of the former class
def ReactivateBaseClass ():
	# things that change:
	#	thac0
	#	saves
	#	class abilities (need to relearn)
	#	spell casting

	ClassIndex = ClassTable.FindValue (5, Classes[1])
	ClassName = ClassTable.GetRowName (ClassIndex)
	KitIndex = GetKitIndex (pc)
	KitList = GemRB.LoadTableObject ("kitlist")

	# see if this thac0 is lower than our current thac0
	ThacoTable = GemRB.LoadTableObject ("THAC0")
	TmpThaco = ThacoTable.GetValue(Classes[1]-1, Level[1]-1, 1)
	if TmpThaco < GemRB.GetPlayerStat (pc, IE_THAC0, 1):
		GemRB.SetPlayerStat (pc, IE_THAC0, TmpThaco)

	# see if all our saves are lower than our current saves
	SavesTable = ClassTable.GetValue (ClassIndex, 3, 0)
	SavesTable = GemRB.LoadTableObject (SavesTable)
	for i in range (5):
		# see if this save is lower than our old save
		TmpSave = SavesTable.GetValue (i, Level[1]-1)
		if TmpSave < GemRB.GetPlayerStat (pc, IE_SAVEVSDEATH+i, 1):
			GemRB.SetPlayerStat (pc, IE_SAVEVSDEATH+i, TmpSave)

	# see if we're a caster
	SpellTables = [ClassSkillsTable.GetValue (Classes[1], 0, 0), ClassSkillsTable.GetValue (Classes[1], 1, 0), ClassSkillsTable.GetValue (Classes[1], 2, 0)]
	if SpellTables[2] != "*": # casts mage spells
		# set up our memorizations
		SpellTable = GemRB.LoadTableObject (SpellTables[2])
		for i in range (9):
			# if we can cast more spells at this level (should be always), then update
			NumSpells = SpellTable.GetValue (Level[1]-1, i)
			if NumSpells > GemRB.GetMemorizableSpellsCount (pc, IE_SPELL_TYPE_WIZARD, i, 1):
				GemRB.SetMemorizableSpellsCount (pc, NumSpells, IE_SPELL_TYPE_WIZARD, i)
	elif SpellTables[1] != "*" or SpellTables[0] != "*": # casts priest spells
		# get the correct table and mask
		if SpellsTables[1] != "*": # clerical spells
			SpellTable = GemRB.LoadTableObject (SpellTables[1])
			ClassMask = 0x8000
		else: # druidic spells
			SpellTable = GemRB.LoadTableObject (SpellTables[0])
			ClassMask = 0x4000

		# loop through each spell level
		for i in range (7):
			# update if we can cast more spells at this level
			NumSpells = SpellTable.GetValue (Level[1]-1, i)
			if NumSpells > GemRB.GetMemorizableSpellsCount (pc, IE_SPELL_TYPE_PRIEST, i, 1):
				GemRB.SetMemorizableSpellsCount (pc, NumSpells, IE_SPELL_TYPE_PRIEST, i)

			# also re-learn the spells if we have to
			# WARNING: this fixes the error whereby rangers dualed to clerics still got all druid spells
			#	they will now only get druid spells up to the level they could cast
			#	this should probably be noted somewhere (ranger/cleric multis still function the same,
			#	but that could be remedied if desired)
			Learnable = GetLearnablePriestSpells(ClassMask, GemRB.GetPlayerStat (pc, IE_ALIGNMENT), i+1)
			for k in range (len (Learnable)): # loop through all the learnable spells
				if HasSpell (pc, IE_SPELL_TYPE_PRIEST, i, Learnable[k]) < 0: # only write it if we don't yet know it
					GemRB.LearnSpell(pc, Learnable[k])

	# setup class bonuses for this class
	if KitIndex == 0: # no kit
		ABTable = ClassSkillsTable.GetValue (ClassName, "ABILITIES")
	else: # kit
		ABTable = KitList.GetValue (KitIndex, 4, 0)
	print "ABTable:",ABTable

	# add the abilites if we aren't a mage and have a table to ref
	if ABTable != "*" and ABTable[:6] != "CLABMA":
		AddClassAbilities (pc, ABTable, Level[1], Level[1]) # relearn class abilites

def LevelUpHLAPress ():
	# we can turn the button off and temporarily set HLACount to 0
	# because there is no cancel button on the HLA window; therefore,
	# it's guaranteed to come back as 0
	TmpCount = GemRB.GetVar ("HLACount")
	GemRB.SetVar ("HLACount", 0)
	RedrawSkills ()
	GemRB.SetVar ("HLACount", TmpCount)

	OpenHLAWindow (pc, NumClasses, Classes, Level)
	return
