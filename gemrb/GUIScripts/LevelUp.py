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

# LevelUp.py - scripts to control the level up functionality and windows
import GemRB
from GUIDefines import *
from ie_stats import *
from ie_restype import RES_2DA
import GameCheck
import GUICommon
import Spellbook
import CommonTables
import LUSpellSelection
import LUCommon
if GameCheck.HasTOB():
	import LUHLASelection
import LUProfsSelection
import LUSkillsSelection
import Actor

LevelUpWindow = None
DoneButton = 0
TextAreaControl = 0
InfoCounter = 1
NewProfPoints = 0
NewSkillPoints = 0
LevelDiff = 0
Level = 0
Classes = 0
NumClasses = 0
DualSwap = 0
KitName = 0
IsDual = 0
IsMulti = 0
pc = 0
ClassName = 0
actor = 0

# old values (so we don't add too much)
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
	"""Sets up the level up window."""
	import GUIREC

	global LevelUpWindow, TextAreaControl, NewProfPoints, actor
	global DoneButton
	global NewSkillPoints, KitName, LevelDiff
	global Level, Classes, NumClasses, DualSwap, IsMulti
	global OldHPMax, OldSaves, OldLore, OldThaco, DeltaDSpells, DeltaWSpells
	global NewDSpells, NewWSpells, OldDSpells, OldWSpells, pc, HLACount, ClassName, IsDual

	LevelUpWindow = GemRB.LoadWindow (3)

	if GameCheck.IsBG2():
		InfoButton = LevelUpWindow.GetControl (125)
		InfoButton.SetText (13707)
		InfoButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, LevelUpInfoPress)

	DoneButton = LevelUpWindow.GetControl (0)
	DoneButton.SetText (11962)
	DoneButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, LevelUpDonePress)
	DoneButton.SetState (IE_GUI_BUTTON_DISABLED)
	DoneButton.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)

	# hide "Character Generation"
	Label = LevelUpWindow.CreateLabel (0x1000007e, 0,0,0,0,"NUMBER","",IE_FONT_ALIGN_CENTER|IE_FONT_SINGLE_LINE)

	# name
	pc = GemRB.GameGetSelectedPCSingle ()
	actor = Actor.Actor(pc)
	Label = LevelUpWindow.GetControl (0x10000000+90)
	Label.SetText (GemRB.GetPlayerName (pc))

	if GameCheck.IsBG1() or GameCheck.IsIWD1():
		# armorclass
		Label = LevelUpWindow.GetControl (0x10000057)
		ac = GemRB.GetPlayerStat (pc, IE_ARMORCLASS)
		#This is a temporary solution, the core engine should set the stat correctly!
		ac += GemRB.GetAbilityBonus (IE_DEX, 2, GemRB.GetPlayerStat (pc, IE_DEX) )
		Label.SetText (str (ac))
		Label.SetTooltip (17183)

		# hp now
		Label = LevelUpWindow.GetControl (0x10000058)
		Label.SetText (str (GemRB.GetPlayerStat (pc, IE_HITPOINTS)))
		Label.SetTooltip (17184)

		# hp max
		Label = LevelUpWindow.GetControl (0x10000059)
		Label.SetText (str (GemRB.GetPlayerStat (pc, IE_MAXHITPOINTS)))
		Label.SetTooltip (17378)

	# some current values
	OldHPMax = GemRB.GetPlayerStat (pc, IE_MAXHITPOINTS, 1)
	OldThaco = GemRB.GetPlayerStat (pc, IE_TOHIT, 1)
	OldLore = GemRB.GetPlayerStat (pc, IE_LORE, 1)
	for i in range (5):
		OldSaves[i] = GemRB.GetPlayerStat (pc, IE_SAVEVSDEATH+i, 1)

	# class
	Label = LevelUpWindow.GetControl (0x10000000+106)
	Label.SetText (GUICommon.GetActorClassTitle (pc))
	print "Title:",GUICommon.GetActorClassTitle (pc),"\tActor Title:",actor.ClassTitle()

	Class = GemRB.GetPlayerStat (pc, IE_CLASS)
	print "Class:",Class,"\tActor Class:",actor.classid
	SkillTable = GemRB.LoadTable("skills")

	# kit
	ClassName = GUICommon.GetClassRowName (Class, "class")
	Kit = GUICommon.GetKitIndex (pc)
	print "Kit:", Kit, "\tActor Kit:",actor.KitIndex()
	print "ClassName:",ClassName,"\tActor ClassNames:",actor.ClassNames()

	# need this for checking gnomes
	RaceName = GemRB.GetPlayerStat (pc, IE_RACE, 1)
	RaceName = CommonTables.Races.FindValue (3, RaceName)
	RaceName = CommonTables.Races.GetRowName (RaceName)

	# figure our our proficiency table and index
	if Kit == 0:
		KitName = ClassName
	else:
		#rowname is just a number, the kitname is the first data column
		KitName = CommonTables.KitList.GetValue(Kit, 0)

	# our multiclass variables
	IsMulti = GUICommon.IsMultiClassed (pc, 1)
	Classes = [IsMulti[1], IsMulti[2], IsMulti[3]]
	NumClasses = IsMulti[0] # 2 or 3 if IsMulti; 0 otherwise
	IsMulti = NumClasses > 1
	IsDual = 0
	DualSwap = 0

	# not multi, check dual
	if not IsMulti:
		# check if we're dual classed
		IsDual = GUICommon.IsDualClassed (pc, 1)
		Classes = [IsDual[2], IsDual[1]] # make sure the new class is first

		# either dual or single only care about 1 class
		NumClasses = 1

		# not dual, must be single
		if IsDual[0] == 0:
			Classes = [Class]
		else: # make sure Classes[1] is a class, not a kit
			if IsDual[0] == 1: # kit
				Classes[1] = CommonTables.KitList.GetValue (IsDual[1], 7)
			else: # class
				TmpClassName = GUICommon.GetClassRowName (Classes[1], "index")
				Classes[1] = CommonTables.Classes.GetValue (TmpClassName, "ID")

		# store a boolean for IsDual
		IsDual = IsDual[0] > 0

	print "NumClasses:",NumClasses,"\tActor NumClasses:",actor.NumClasses()

	Level = [0]*3
	LevelDiff = [0]*3

	# reorganize the leves if we're dc so the one we want to use is in Level[0]
	# and the old one is in Level[1] (used to regain old class abilities)
	if IsDual:
		# convert the classes from indicies to class id's
		DualSwap = GUICommon.IsDualSwap (pc)
		ClassName = GUICommon.GetClassRowName (Classes[0], "index")
		KitName = ClassName # for simplicity throughout the code
		Classes[0] = CommonTables.Classes.GetValue (ClassName, "ID")
		# Class[1] is taken care of above

		# we need the old level as well
		if DualSwap:
			Level[1] = GemRB.GetPlayerStat (pc, IE_LEVEL)
		else:
			Level[1] = GemRB.GetPlayerStat (pc, IE_LEVEL2)

	print "Classes:",Classes,"\tActor Classes:",actor.Classes()
	print "IsDual:",IsDual>0,"\tActor IsDual",actor.isdual

	hp = 0
	HaveCleric = 0
	# clear some globals, since we may get called multiple times with different classes
	DeltaWSpells = 0
	DeltaDSpells = 0
	OldDSpells = [0]*7
	OldWSpells = [0]*9
	NewDSpells = [0]*7
	NewWSpells = [0]*9

	# get a bunch of different things each level
	for i in range(NumClasses):
#		print "Class:",Classes[i]
		# we don't care about the current level, but about the to-be-achieved one
		# get the next level
		Level[i] = LUCommon.GetNextLevelFromExp (GemRB.GetPlayerStat (pc, IE_XP)/NumClasses, Classes[i])
		TmpClassName = GUICommon.GetClassRowName (Classes[i], "class")

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

#		print "Level (",i,"):",Level[i]
#		print "Level Diff (",i,"):",LevelDiff[i]

		# save our current and next spell amounts
		StartLevel = Level[i] - LevelDiff[i]
		DruidTable = CommonTables.ClassSkills.GetValue (TmpClassName, "DRUIDSPELL", GTV_STR)
		ClericTable = CommonTables.ClassSkills.GetValue (TmpClassName, "CLERICSPELL", GTV_STR)
		MageTable = CommonTables.ClassSkills.GetValue (TmpClassName, "MAGESPELL", GTV_STR)

		# see if we have mage spells
		if MageTable != "*":
			# we get 1 extra spell per level if we're a specialist
			Specialist = 0
			if CommonTables.KitList.GetValue (Kit, 7) == 1: # see if we're a kitted mage
				Specialist = 1
			MageTable = GemRB.LoadTable (MageTable)
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
			if not GemRB.HasResource(ClericTable, RES_2DA, 1):
				ClericTable = "MXSPLPRS" # iwd1 doesn't have a DRUIDSPELL column in the table
			ClericTable = GemRB.LoadTable (ClericTable)
			HaveCleric = 1
			# same as above
			for j in range (ClericTable.GetColumnCount ()):
				NewDSpells[j] = ClericTable.GetValue (str(Level[i]), str(j+1), 1)
				OldDSpells[j] = ClericTable.GetValue (str(StartLevel), str(j+1), 1)
			DeltaDSpells = sum(NewDSpells)-sum(OldDSpells)
		elif DruidTable != "*":
			# clerics have precedence in multis (ranger/cleric)
			if HaveCleric == 0:
				#use MXSPLPRS if we can't find the resource (SoA fix)
				if not GemRB.HasResource (DruidTable, RES_2DA):
					DruidTable = "MXSPLPRS"

				# check druid spells
				DruidTable = GemRB.LoadTable (DruidTable)
				# same as above
				for j in range (DruidTable.GetColumnCount ()):
					NewDSpells[j] = DruidTable.GetValue (str(Level[i]), str(j+1), 1)
					OldDSpells[j] = DruidTable.GetValue (str(StartLevel), str(j+1), 1)
				DeltaDSpells = sum(NewDSpells)-sum(OldDSpells)

		# setup class bonuses for this class
		if IsMulti or IsDual or Kit == 0:
			ABTable = CommonTables.ClassSkills.GetValue (TmpClassName, "ABILITIES")
		else: # single-classed with a kit
			ABTable = CommonTables.KitList.GetValue (str(Kit), "ABILITIES")

		# add the abilites if we have a table to ref
		if ABTable != "*" and GemRB.HasResource (ABTable, RES_2DA, 1):
			GUICommon.AddClassAbilities (pc, ABTable, Level[i], LevelDiff[i])

	print "Actor CurrentLevels:",actor.Levels()
	print "Levels:",Level,"Actor NextLevels:",actor.NextLevels()
	print "LevelDiffs:",LevelDiff,"Actor LevelDiffs:",actor.LevelDiffs()

	#update our saves, thaco, hp and lore
	LUCommon.SetupSavingThrows (pc, Level)
	LUCommon.SetupThaco (pc, Level)
	LUCommon.SetupLore (pc, LevelDiff)
	LUCommon.SetupHP (pc, Level, LevelDiff)

	# use total levels for HLAs
	HLACount = 0
	if GameCheck.HasTOB(): # make sure SoA doesn't try to get it
		HLATable = GemRB.LoadTable("lunumab")
		# we need to check each level against a multi value (this is kinda screwy)
		if actor.multiclass:
			print "Actor HLA Names:",["MULTI"+str(actor.NumClasses())+name \
				for name in actor.ClassNames()]
		else:
			print "Actor HLA Names:",actor.ClassNames()

		for i in range (NumClasses):
			if IsMulti:
				# get the row name for lookup ex. MULTI2FIGHTER, MULTI3THIEF
				MultiName = GUICommon.GetClassRowName (Classes[i], "class")
				MultiName = "MULTI" + str(NumClasses) + MultiName
			else:
				MultiName = ClassName

			# if we can't learn for this class, we can't learn at all
			FirstLevel = HLATable.GetValue (MultiName, "FIRST_LEVEL", GTV_INT)
			if Level[i] < FirstLevel:
				HLACount = 0
				break

			if (Level[i] - LevelDiff[i]) < FirstLevel:
				# count only from FirstLevel up
				HLACount += (Level[i] - FirstLevel + 1)
			else:
				HLACount += LevelDiff[i]

		# set values required by the hla level up code
		HLACount = HLACount / HLATable.GetValue (ClassName, "RATE", GTV_INT)
		GemRB.SetVar ("HLACount", HLACount)
	if GameCheck.IsBG2():
		HLAButton = LevelUpWindow.GetControl (126)
		if HLACount:
			HLAButton.SetText (4954)
			HLAButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, LevelUpHLAPress)
		else:
			HLAButton.SetFlags (IE_GUI_BUTTON_DISABLED, OP_OR)

	# setup our profs
	Level1 = []
	for i in range (len (Level)):
		Level1.append (Level[i]-LevelDiff[i])
	if GameCheck.IsBG2():
		LUProfsSelection.SetupProfsWindow (pc, LUProfsSelection.LUPROFS_TYPE_LEVELUP, LevelUpWindow, RedrawSkills, Level1, Level)
	else:
		LUProfsSelection.SetupProfsWindow (pc, LUProfsSelection.LUPROFS_TYPE_LEVELUP, LevelUpWindow, RedrawSkills, Level1, Level, 0, False, 0)
	NewProfPoints = GemRB.GetVar ("ProfsPointsLeft")

	#we autohide the skills and let SetupSkillsWindow show them if needbe
	for i in range (4):
		HideSkills (i)
	if GameCheck.IsBG2():
		LUSkillsSelection.SetupSkillsWindow (pc, LUSkillsSelection.LUSKILLS_TYPE_LEVELUP, LevelUpWindow, RedrawSkills, Level1, Level)
	else:
		LUSkillsSelection.SetupSkillsWindow (pc, LUSkillsSelection.LUSKILLS_TYPE_LEVELUP, LevelUpWindow, RedrawSkills, Level1, Level, 0, False)
	NewSkillPoints = GemRB.GetVar ("SkillPointsLeft")

	if GameCheck.IsBG2():
		TextAreaControl = LevelUpWindow.GetControl(110)
		TextAreaControl.SetText(GetLevelUpNews())
	else:
		TextAreaControl = LevelUpWindow.GetControl(42)
		TextAreaControl.SetText(GUIREC.GetStatOverview(pc, LevelDiff))

	RedrawSkills()
	GemRB.SetRepeatClickFlags (GEM_RK_DISABLE, OP_NAND)
	LevelUpWindow.ShowModal (MODAL_SHADOW_GRAY)

	# if we have a sorcerer who can learn spells, we need to do spell selection
	if (Classes[0] == 19) and (DeltaWSpells > 0): # open our sorc spell selection window
		LUSpellSelection.OpenSpellsWindow (pc, "SPLSRCKN", Level[0], LevelDiff[0])

def HideSkills(i):
	"""Hides the given skill label from view."""
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

def RedrawSkills():
	"""Redraws the entire window.

	Called whenever a state changes, such as a proficiency or skill being
	added or taken away."""

	global DoneButton, LevelUpWindow, HLACount

	# we need to disable the HLA button if we don't have any HLAs left
	HLACount = GemRB.GetVar ("HLACount")
	if GameCheck.IsBG2() and HLACount == 0:
		# turn the HLA button off
		HLAButton = LevelUpWindow.GetControl (126)
		HLAButton.SetState(IE_GUI_BUTTON_DISABLED)

	# enable the done button if they've allocated all points
	# sorcerer spell selection (if applicable) comes after hitting the done button
	ProfPointsLeft = GemRB.GetVar ("ProfsPointsLeft")
	SkillPointsLeft = GemRB.GetVar ("SkillPointsLeft")
	if ProfPointsLeft == 0 and SkillPointsLeft == 0 and HLACount == 0:
		DoneButton.SetState (IE_GUI_BUTTON_ENABLED)
	else:
		DoneButton.SetState (IE_GUI_BUTTON_DISABLED)
	return

def GetLevelUpNews():
	"""Returns a string containing improvements gain on level up.

	These include: HP, spells per level, and lore, among others."""

	News = GemRB.GetString (5259) + '\n\n'

	# display if our class has been reactivated
	if IsDual:
		if (Level[0] - LevelDiff[0]) <= Level[1] and Level[0] > Level[1]:
			News = GemRB.GetString (5261) + '\n\n'

	# 5271 - Additional weapon proficiencies
	if NewProfPoints > 0:
		News += GemRB.GetString (5271) + ": " + str(NewProfPoints) + '\n\n'

	# temps to compare all our new saves against (we get the best of our saving throws)
	LOHGain = 0
	BackstabMult = 0

	for i in range(NumClasses):
		# get the class name
		TmpClassName = GUICommon.GetClassRowName (Classes[i], "class")

		# backstab
		# NOTE: Stalkers and assassins should get the correct mods at the correct levels based
		#	on AP_SPCL332 in their respective CLAB files.
		# APND: Stalkers appear to get the correct mod at the correct levels; however, because they start
		#	at backstab multi x1, they are x1 too many at their respective levels.
		if Classes[i] == 4 and GemRB.GetPlayerStat (pc, IE_BACKSTABDAMAGEMULTIPLIER, 1) > 1: # we have a thief who can backstab (2 at level 1)
			# save the backstab multiplier if we have a thief
			BackstabTable = GemRB.LoadTable ("BACKSTAB")
			BackstabMult = BackstabTable.GetValue (0, Level[i])

		# lay on hands
		if (CommonTables.ClassSkills.GetValue (TmpClassName, "LAYHANDS") != "*"):
			# inquisitors and undead hunters don't get lay on hands out the chute, whereas cavaliers
			# and unkitted paladins do; therefore, we check for the existence of lay on hands to ensure
			# the character should get the new value; LoH is defined in GA_SPCL211 if anyone wants to
			# make a pally kit with LoH
			if (Spellbook.HasSpell (pc, IE_SPELL_TYPE_INNATE, 0, "SPCL211") >= 0):
				LOHTable = GemRB.LoadTable ("layhands")
				LOHGain = LOHTable.GetValue (0, Level[i]) - LOHTable.GetValue (0, Level[i]-LevelDiff[i])

	# saving throws
		# 5277 death
		# 5278 wand
		# 5279 polymorph
		# 5282 breath
		# 5292 spell
	# include in news if the save is updated
	Changed = 0
	for i in range (5):
		CurrentSave = GemRB.GetPlayerStat (pc, IE_SAVEVSDEATH+i, 1)
		SaveString = 5277+i
		if i == 3:
			SaveString = 5282
		elif i == 4:
			SaveString = 5292

		if CurrentSave < OldSaves[i]:
			News += GemRB.GetString (SaveString) + ": " + str(OldSaves[i]-CurrentSave) + '\n'
			Changed = 1
	if Changed:
		News += '\n'

	# 5305 - THAC0 Reduced by
	# only output if there is a change in thaco
	NewThaco = GemRB.GetPlayerStat (pc, IE_TOHIT, 1)
	if (NewThaco < OldThaco):
		News += GemRB.GetString (5305) + ": " + str(OldThaco-NewThaco) + '\n\n'

	# new spell slots
		# 5373 - Additional Priest Spells
		# 5374 - Additional Mage Spells
		# 61269 - Level <LEVEL> Spells
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
	BSGain = BackstabMult - GemRB.GetPlayerStat (pc, IE_BACKSTABDAMAGEMULTIPLIER, 1)
	if (BSGain > 0):
		News += GemRB.GetString (5375) + ": " + str(BSGain) + '\n\n'

	# 5376 - Lay on Hands increased
	if LOHGain > 0:
		News += GemRB.GetString (5376) + ": " + str(LOHGain) + '\n\n'

	# 5293 - HP increased by
	if (OldHPMax != GemRB.GetPlayerStat (pc, IE_MAXHITPOINTS, 1)):
		News += GemRB.GetString (5293) + ": " + str(GemRB.GetPlayerStat (pc, IE_MAXHITPOINTS, 1) - OldHPMax) + '\n'

	# 5377 - Lore Increased by
	# add the lore gain if we haven't done so already
	NewLore = GemRB.GetPlayerStat (pc, IE_LORE, 1)
	if NewLore > OldLore:
		News += GemRB.GetString (5377) + ": " + str(NewLore-OldLore) + '\n\n'

	# 5378 - Additional Skill Points
	# ranger and bard skill(point) gain is not mentioned here in the original
	if NewSkillPoints > 0:
		News += GemRB.GetString (5378) + ": " + str(NewSkillPoints) + '\n'

	return News

def LevelUpInfoPress():
	"""Displays new abilites gained on level up.

	Alternates between overall and modified stats."""
	import GUIREC
	global LevelUpWindow, TextAreaControl, InfoCounter, LevelDiff

	if InfoCounter % 2:
		# call GetStatOverview with the new levels, so the future overview is shown
		# TODO: show only xp, levels, thac0, #att, lore, reputation, backstab, saving throws
		TextAreaControl.SetText(GUIREC.GetStatOverview(pc, LevelDiff))
	else:
		TextAreaControl.SetText(GetLevelUpNews())
	InfoCounter += 1
	return

# save the results
def LevelUpDonePress():
	"""Updates the PC with the new choices.

	Closes the window when finished."""
	import GUICommonWindows
	import GUIREC
	global SkillTable

	# proficiencies
	LUProfsSelection.ProfsSave (pc)

	# skills
	LUSkillsSelection.SkillsSave (pc)

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
			# bonus spells don't count in determining if we can use this level
			if GemRB.GetMemorizableSpellsCount (pc, IE_SPELL_TYPE_PRIEST, i, 0) > 0: # we can memorize spells of this level
				for j in range(NumClasses): # loop through each class
					TmpClassName = GUICommon.GetClassRowName (Classes[j], "class")
					IsDruid = CommonTables.ClassSkills.GetValue (TmpClassName, "DRUIDSPELL", GTV_STR)
					IsCleric = CommonTables.ClassSkills.GetValue (TmpClassName, "CLERICSPELL", GTV_STR)
					if IsCleric == "*" and IsDruid == "*": # no divine spells (so mage/cleric multis don't screw up)
						continue
					elif IsCleric == "*": # druid spells
						ClassFlag = 0x8000
					else: # cleric spells
						ClassFlag = 0x4000

					Learnable = Spellbook.GetLearnablePriestSpells(ClassFlag, GemRB.GetPlayerStat (pc, IE_ALIGNMENT), i+1)
					for k in range(len(Learnable)): # loop through all the learnable spells
						if Spellbook.HasSpell (pc, IE_SPELL_TYPE_PRIEST, i, Learnable[k]) < 0: # only write it if we don't yet know it
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
	GUICommonWindows.UpdatePortraitWindow ()
	GUIREC.UpdateRecordsWindow()

	GemRB.SetRepeatClickFlags (GEM_RK_DISABLE, OP_OR)
	return

def LevelUpHLAPress ():
	"""Opens the HLA selection window."""

	# we can turn the button off and temporarily set HLACount to 0
	# because there is no cancel button on the HLA window; therefore,
	# it's guaranteed to come back as 0
	TmpCount = GemRB.GetVar ("HLACount")
	GemRB.SetVar ("HLACount", 0)
	RedrawSkills ()
	GemRB.SetVar ("HLACount", TmpCount)

	LUHLASelection.OpenHLAWindow (pc, NumClasses, Classes, Level)
	return

def ReactivateBaseClass ():
	"""Regains all abilities of the base dual-class.

	Proficiencies, THACO, saves, spells, and innate abilites,
	most noteably."""

	# we construct the Classes array, so that the active class is always first and the base is second
	ClassName = GUICommon.GetClassRowName (Classes[1], "class")
	KitIndex = GUICommon.GetKitIndex (pc)

	# reactivate all our proficiencies
	TmpTable = GemRB.LoadTable ("weapprof")
	ProfsTableOffset = 0
	if GameCheck.IsBG2 ():
		ProfsTableOffset = 8 # skip bg1 weapprof.2da proficiencies
	ProfCount = TmpTable.GetRowCount () - ProfsTableOffset
	for i in range(ProfCount):
		ProfID = TmpTable.GetValue (i+ProfsTableOffset, 0)
		if GameCheck.IsBG1():
			ProfID = ProfID + IE_PROFICIENCYBASTARDSWORD
		Value = GemRB.GetPlayerStat (pc, ProfID)
		OldProf = (Value & 0x38) >> 3
		NewProf = Value & 0x07
		if OldProf > NewProf:
			Value = (OldProf << 3) | OldProf
			print "Value:",Value
			if GameCheck.IsBG2():
				GemRB.ApplyEffect (pc, "Proficiency", Value, ProfID )
			else:
				GemRB.SetPlayerStat (pc, ProfID, Value)

	# see if this thac0 is lower than our current thac0
	ThacoTable = GemRB.LoadTable ("THAC0")
	TmpThaco = ThacoTable.GetValue(Classes[1]-1, Level[1]-1, GTV_INT)
	if TmpThaco < GemRB.GetPlayerStat (pc, IE_TOHIT, 1):
		GemRB.SetPlayerStat (pc, IE_TOHIT, TmpThaco)

	# see if all our saves are lower than our current saves
	SavesTable = CommonTables.Classes.GetValue (ClassName, "SAVE", GTV_STR)
	SavesTable = GemRB.LoadTable (SavesTable)
	for i in range (5):
		# see if this save is lower than our old save
		TmpSave = SavesTable.GetValue (i, Level[1]-1)
		if TmpSave < GemRB.GetPlayerStat (pc, IE_SAVEVSDEATH+i, 1):
			GemRB.SetPlayerStat (pc, IE_SAVEVSDEATH+i, TmpSave)

	# see if we're a caster
	SpellTables = [CommonTables.ClassSkills.GetValue (ClassName, "DRUIDSPELL", 0), CommonTables.ClassSkills.GetValue (ClassName, "CLERICSPELL", 0), CommonTables.ClassSkills.GetValue (ClassName, "MAGESPELL", 0)]
	if SpellTables[2] != "*": # casts mage spells
		# set up our memorizations
		SpellTable = GemRB.LoadTable (SpellTables[2])
		for i in range (9):
			# if we can cast more spells at this level (should be always), then update
			NumSpells = SpellTable.GetValue (Level[1]-1, i)
			if NumSpells > GemRB.GetMemorizableSpellsCount (pc, IE_SPELL_TYPE_WIZARD, i, 1):
				GemRB.SetMemorizableSpellsCount (pc, NumSpells, IE_SPELL_TYPE_WIZARD, i)
	elif SpellTables[1] != "*" or SpellTables[0] != "*": # casts priest spells
		# get the correct table and mask
		if SpellTables[1] != "*": # clerical spells
			SpellTable = GemRB.LoadTable (SpellTables[1])
			ClassMask = 0x4000
		else: # druidic spells
			if not GemRB.HasResource(SpellTables[0], RES_2DA):
				SpellTables[0] = "MXSPLPRS"
			SpellTable = GemRB.LoadTable (SpellTables[0])
			ClassMask = 0x8000

		# loop through each spell level
		for i in range (7):
			# update if we can cast more spells at this level
			NumSpells = SpellTable.GetValue (str(Level[1]), str(i+1), 1)
			if not NumSpells:
				continue
			if NumSpells > GemRB.GetMemorizableSpellsCount (pc, IE_SPELL_TYPE_PRIEST, i, 1):
				GemRB.SetMemorizableSpellsCount (pc, NumSpells, IE_SPELL_TYPE_PRIEST, i)

			# also re-learn the spells if we have to
			# WARNING: this fixes the error whereby rangers dualed to clerics still got all druid spells
			#	they will now only get druid spells up to the level they could cast
			#	this should probably be noted somewhere (ranger/cleric multis still function the same,
			#	but that could be remedied if desired)
			Learnable = Spellbook.GetLearnablePriestSpells(ClassMask, GemRB.GetPlayerStat (pc, IE_ALIGNMENT), i+1)
			for k in range (len (Learnable)): # loop through all the learnable spells
				if Spellbook.HasSpell (pc, IE_SPELL_TYPE_PRIEST, i, Learnable[k]) < 0: # only write it if we don't yet know it
					GemRB.LearnSpell(pc, Learnable[k])

	# setup class bonuses for this class
	if KitIndex == 0: # no kit
		ABTable = CommonTables.ClassSkills.GetValue (ClassName, "ABILITIES")
	else: # kit
		ABTable = CommonTables.KitList.GetValue (KitIndex, 4, GTV_STR)
	print "ABTable:",ABTable

	# add the abilites if we have a table to ref
	if ABTable != "*" and GemRB.HasResource (ABTable, RES_2DA, 1):
		GUICommon.AddClassAbilities (pc, ABTable, Level[1], Level[1]) # relearn class abilites
