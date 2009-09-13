# -*-python-*-
# GemRB - Infinity Engine Emulator
# Copyright (C) 2009 The GemRB Project
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
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#
# $Id:$
#
# LUCommon.py - common functions related to leveling up

import GemRB
from GUICommon import *
from ie_stats import *

def GetNextLevelExp (Level, Class):
	"""Returns the amount of XP required to gain the next level."""
	Row = NextLevelTable.GetRowIndex (Class)
	if Level < NextLevelTable.GetColumnCount (Row):
		return str (NextLevelTable.GetValue (Row, Level) )

	return 0

def CanLevelUp(actor):
	"""Returns true if the actor can level up."""

	# get our class and placements for Multi'd and Dual'd characters
	Class = GemRB.GetPlayerStat (actor, IE_CLASS)
	Class = ClassTable.FindValue (5, Class)
	Class = ClassTable.GetRowName (Class)
	Multi = IsMultiClassed (actor, 1)
	Dual = IsDualClassed (actor, 1)

	# get all the levels and overall xp here
	xp = GemRB.GetPlayerStat (actor, IE_XP)
	Levels = [GemRB.GetPlayerStat (actor, IE_LEVEL), GemRB.GetPlayerStat (actor, IE_LEVEL2),\
		GemRB.GetPlayerStat (actor, IE_LEVEL3)]

	#TODO: double-check this
	if GemRB.GetPlayerStat(actor, IE_LEVELDRAIN)>0:
		return 0

	if Multi[0] > 1: # multiclassed
		xp = xp/Multi[0] # divide the xp evenly between the classes
		for i in range (Multi[0]):
			# if any class can level, return 1
			ClassIndex = ClassTable.FindValue (5, Multi[i+1])
			tmpNext = int(GetNextLevelExp (Levels[i], ClassTable.GetRowName (ClassIndex) ) )
			if tmpNext != 0 and tmpNext <= xp:
				return 1

		# didn't find a class that could level
		return 0
	elif Dual[0] > 0: # dual classed
		# get the class we can level
		Class = ClassTable.GetRowName (Dual[2])
		if IsDualSwap(actor):
			Levels = [Levels[1], Levels[0], Levels[2]]

	# check the class that can be level (single or dual)
	tmpNext = int(GetNextLevelExp (Levels[0], Class) )
	return (tmpNext != 0 and tmpNext <= xp)

def SetupSavingThrows (pc, Level=None):
	"""Updates an actors saving throws based upon level.

	Level should contain the actors current level.
	If Level is None, it is filled with the actors current level."""

	#storing levels as an array makes them easier to deal with
	if not Level:
		Levels = [GemRB.GetPlayerStat (pc, IE_LEVEL)-1, \
			GemRB.GetPlayerStat (pc, IE_LEVEL2)-1, \
			GemRB.GetPlayerStat (pc, IE_LEVEL3)-1]
	else:
		Levels = []
		for level in Level:
			Levels.append (level-1)

	#get some basic values
	Class = [GemRB.GetPlayerStat (pc, IE_CLASS)]
	Race = GemRB.GetPlayerStat (pc, IE_RACE)

	#adjust the class for multi/dual chars
	Multi = IsMultiClassed (pc, 1)
	Dual = IsDualClassed (pc, 1)
	NumClasses = 1
	if Multi[0]>1: #get each of the multi-classes
		NumClasses = Multi[0]
		Class = [Multi[1], Multi[2], Multi[3]]
	elif Dual[0]: #only worry about the newer class
		Class = [ClassTable.GetValue (Dual[2], 5)]
		#assume Level is correct if passed
		if IsDualSwap(pc) and not Level:
			Levels = [Levels[1], Levels[0], Levels[2]]
	if NumClasses>len(Levels):
		return

	#see if we can add racial bonuses to saves
	#default return is -1 NOT "*", so we convert always convert to str
	#I'm leaving the "*" just in case
	Race = RaceTable.GetRowName (RaceTable.FindValue (3, Race) )
	RaceSaveTableName = str(RaceTable.GetValue (Race, "SAVE") )
	RaceSaveTable = None
	if RaceSaveTableName != "-1" and RaceSaveTableName != "*":
		Con = GemRB.GetPlayerStat (pc, IE_CON, 1)-1
		RaceSaveTable = GemRB.LoadTableObject (RaceSaveTableName)
		if Con >= RaceSaveTable.GetRowCount ():
			Con = RaceSaveTable.GetRowCount ()-1

	#preload our tables to limit multi-classed lookups
	SaveTables = []
	for i in range (NumClasses):
		SaveName = ClassTable.GetValue (ClassTable.FindValue (5, Class[i]), 3, 0)
		SaveTables.append (GemRB.LoadTableObject (SaveName) )
	if not len (SaveTables):
		return

	#make sure to limit the levels to the table allowable
	MaxLevel = SaveTables[0].GetColumnCount ()-1
	for i in range (len(Levels)):
		if Levels[i] > MaxLevel:
			Levels[i] = MaxLevel

	#save the saves
	for row in range (5):
		CurrentSave = GemRB.GetPlayerStat(pc, IE_SAVEVSDEATH+i, 1)
		for i in range (NumClasses):
			#loop through each class and update the save value if we have
			#a better save
			TmpSave = SaveTables[i].GetValue (row, Levels[i])
			if TmpSave and (TmpSave < CurrentSave or i == 0):
				CurrentSave = TmpSave

		#add racial bonuses if applicable (small pc's)
		if RaceSaveTable:
			CurrentSave += RaceSaveTable.GetValue (row, Con)
		GemRB.SetPlayerStat (pc, IE_SAVEVSDEATH+row, CurrentSave)
	return

def ReactivateBaseClass ():
	"""Regains all abilities of the base dual-class.

	Proficiencies, THACO, saves, spells, and innate abilites,
	most noteably."""

	ClassIndex = ClassTable.FindValue (5, Classes[1])
	ClassName = ClassTable.GetRowName (ClassIndex)
	KitIndex = GetKitIndex (pc)

	# reactivate all our proficiencies
	TmpTable = GemRB.LoadTableObject ("weapprof")
	ProfCount = TmpTable.GetRowCount ()
	if GameIsBG2 ():
		ProfCount -= 8 # skip bg1 weapprof.2da proficiencies
	for i in range(ProfCount):
		StatID = TmpTable.GetValue (i+8, 0)
		Value = GemRB.GetPlayerStat (pc, StatID)
		OldProf = (Value & 0x38) >> 3
		NewProf = Value & 0x07
		if OldProf > NewProf:
			Value = (OldProf << 3) | OldProf
			print "Value:",Value
			GemRB.ApplyEffect (pc, "Proficiency", Value, StatID )

	# see if this thac0 is lower than our current thac0
	ThacoTable = GemRB.LoadTableObject ("THAC0")
	TmpThaco = ThacoTable.GetValue(Classes[1]-1, Level[1]-1, 1)
	if TmpThaco < GemRB.GetPlayerStat (pc, IE_TOHIT, 1):
		GemRB.SetPlayerStat (pc, IE_TOHIT, TmpThaco)

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
		if SpellTables[1] != "*": # clerical spells
			SpellTable = GemRB.LoadTableObject (SpellTables[1])
			ClassMask = 0x4000
		else: # druidic spells
			if not GameRB.HasResource(SpellTables[0], RES_2DA):
				SpellTables[0] = "MXSPLPRS"
			SpellTable = GemRB.LoadTableObject (SpellTables[0])
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
			Learnable = GetLearnablePriestSpells(ClassMask, GemRB.GetPlayerStat (pc, IE_ALIGNMENT), i+1)
			for k in range (len (Learnable)): # loop through all the learnable spells
				if HasSpell (pc, IE_SPELL_TYPE_PRIEST, i, Learnable[k]) < 0: # only write it if we don't yet know it
					GemRB.LearnSpell(pc, Learnable[k])

	# setup class bonuses for this class
	if KitIndex == 0: # no kit
		ABTable = ClassSkillsTable.GetValue (ClassName, "ABILITIES")
	else: # kit
		ABTable = KitListTable.GetValue (KitIndex, 4, 0)
	print "ABTable:",ABTable

	# add the abilites if we aren't a mage and have a table to ref
	if ABTable != "*" and ABTable[:6] != "CLABMA":
		AddClassAbilities (pc, ABTable, Level[1], Level[1]) # relearn class abilites

def GetNextLevelFromExp (XP, Class):
	"""Gets the next level based on current experience."""

	ClassIndex = ClassTable.FindValue (5, Class)
	ClassName = ClassTable.GetRowName (ClassIndex)
	Row = NextLevelTable.GetRowIndex (ClassName)
	for i in range(1, NextLevelTable.GetColumnCount()-1):
		if XP < NextLevelTable.GetValue (Row, i):
			return i
	# fix hacked characters that have more xp than the xp cap
	return 40

def SetupThaco (pc, Level=None):
	"""Updates an actors THAC0 based upon level.

	Level should contain the actors current level.
	If Level is None it is filled with the actors current level."""

	#storing levels as an array makes them easier to deal with
	if not Level:
		Levels = [GemRB.GetPlayerStat (pc, IE_LEVEL)-1, \
			GemRB.GetPlayerStat (pc, IE_LEVEL2)-1, \
			GemRB.GetPlayerStat (pc, IE_LEVEL3)-1]
	else:
		Levels = []
		for level in Level:
			Levels.append (level-1)

	#get some basic values
	Class = [GemRB.GetPlayerStat (pc, IE_CLASS)]
	ThacoTable = GemRB.LoadTableObject ("THAC0")

	#adjust the class for multi/dual chars
	Multi = IsMultiClassed (pc, 1)
	Dual = IsDualClassed (pc, 1)
	NumClasses = 1
	if Multi[0]>1: #get each of the multi-classes
		NumClasses = Multi[0]
		Class = [Multi[1], Multi[2], Multi[3]]
	elif Dual[0]: #only worry about the newer class
		Class = [ClassTable.GetValue (Dual[2], 5)]
		#assume Level is correct if passed
		if IsDualSwap(pc) and not Level:
			Levels = [Levels[1], Levels[0], Levels[2]]
	if NumClasses>len(Levels):
		return

	#make sure to limit the levels to the table allowable
	MaxLevel = ThacoTable.GetColumnCount ()-1
	for i in range (len(Levels)):
		if Levels[i] > MaxLevel:
			Levels[i] = MaxLevel

	CurrentThaco = GemRB.GetPlayerStat (pc, IE_TOHIT, 1)
	NewThaco = 0
	for i in range (NumClasses):
		#loop through each class and update the save value if we have
		#a better thac0
		ClassName = ClassTable.GetRowName (ClassTable.FindValue (5, Class[i]))
		TmpThaco = ThacoTable.GetValue (ClassName, str(Levels[i]+1))
		if TmpThaco < CurrentThaco:
			NewThaco = 1
			CurrentThaco = TmpThaco

	#only update if we have a better thac0
	if NewThaco:
		GemRB.SetPlayerStat (pc, IE_TOHIT, CurrentThaco)
	return

def SetupLore (pc, LevelDiff=None):
	"""Updates an actors lore based upon level.

	Level should contain the actors current level.
	LevelDiff should contain the change in levels.
	Level and LevelDiff must be of the same length.
	If either are None, they are filled with the actors current level."""

	#storing levels as an array makes them easier to deal with
	if not LevelDiff:
		LevelDiffs = [GemRB.GetPlayerStat (pc, IE_LEVEL), \
			GemRB.GetPlayerStat (pc, IE_LEVEL2), \
			GemRB.GetPlayerStat (pc, IE_LEVEL3)]
	else:
		LevelDiffs = []
		for diff in LevelDiff:
			LevelDiffs.append (diff)

	#get some basic values
	Class = [GemRB.GetPlayerStat (pc, IE_CLASS)]
	LoreTable = GemRB.LoadTableObject ("lore")

	#adjust the class for multi/dual chars
	Multi = IsMultiClassed (pc, 1)
	Dual = IsDualClassed (pc, 1)
	NumClasses = 1
	if Multi[0]>1: #get each of the multi-classes
		NumClasses = Multi[0]
		Class = [Multi[1], Multi[2], Multi[3]]
	elif Dual[0]: #only worry about the newer class
		Class = [ClassTable.GetValue (Dual[2], 5)]
		#if LevelDiff is passed, we assume it is correct
		if IsDualSwap(pc) and not LevelDiff:
			LevelDiffs = [LevelDiffs[1], LevelDiffs[0], LevelDiffs[2]]
	if NumClasses>len(LevelDiffs):
		return

	#loop through each class and update the lore value if we have
	CurrentLore = GemRB.GetPlayerStat (pc, IE_LORE, 1)
	for i in range (NumClasses):
		#correct unlisted progressions
		ClassName = ClassTable.GetRowName (ClassTable.FindValue (5, Class[i]) )
		if ClassName == "SORCERER":
			ClassName = "MAGE"
		elif ClassName == "MONK": #monks have a rate of 1, so this is arbitrary
			ClassName = "CLERIC"

		#add the lore from this class to the total lore
		TmpLore = LevelDiffs[i] * LoreTable.GetValue (ClassName, "RATE", 1)
		if TmpLore:
			CurrentLore += TmpLore

	#update our lore value
	GemRB.SetPlayerStat (pc, IE_LORE, CurrentLore)
	return


