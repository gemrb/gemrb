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
#
# LUCommon.py - common functions related to leveling up

import GemRB
import GameCheck
import GUICommon
import CommonTables
from GUIDefines import *
from ie_stats import *
from ie_feats import *

def GetNextLevelExp (Level, Class):
	"""Returns the amount of XP required to gain the next level."""
	Row = CommonTables.NextLevel.GetRowIndex (Class)
	if Level < CommonTables.NextLevel.GetColumnCount (Row):
		return CommonTables.NextLevel.GetValue (Row, Level, GTV_STR)

	# we could display the current level's max, but likely nobody cares
	# if you change it, check that all callers can handle it
	return "0"

def CanLevelUp(actor):
	"""Returns true if the actor can level up."""

	# get our class and placements for Multi'd and Dual'd characters
	Class = GUICommon.GetClassRowName (actor)
	Multi = GUICommon.IsMultiClassed (actor, 1)
	Dual = GUICommon.IsDualClassed (actor, 1)

	# get all the levels and overall xp here
	xp = GemRB.GetPlayerStat (actor, IE_XP)
	Levels = [GemRB.GetPlayerStat (actor, IE_LEVEL), GemRB.GetPlayerStat (actor, IE_LEVEL2),\
		GemRB.GetPlayerStat (actor, IE_LEVEL3)]

	if GemRB.GetPlayerStat(actor, IE_LEVELDRAIN)>0:
		return 0

	if GameCheck.IsIWD2():
		import GUIREC
		levelsum = GemRB.GetPlayerStat (actor, IE_CLASSLEVELSUM)
		next = GUIREC.GetNextLevelExp(levelsum, GUIREC.GetECL(actor))
		return next <= xp

	if Multi[0] > 1: # multiclassed
		xp = xp/Multi[0] # divide the xp evenly between the classes
		for i in range (Multi[0]):
			# if any class can level, return 1
			TmpClassName = GUICommon.GetClassRowName (Multi[i+1], "class")
			tmpNext = int(GetNextLevelExp (Levels[i], TmpClassName))
			if (tmpNext != 0 or Levels[i] == 0) and tmpNext <= xp:
				return 1

		# didn't find a class that could level
		return 0
	elif Dual[0] > 0: # dual classed
		# get the class we can level
		if Dual[0] == 3:
			ClassID = CommonTables.KitList.GetValue (Dual[2], 7)
			Class = GUICommon.GetClassRowName (ClassID, "class")
		else:
			Class = GUICommon.GetClassRowName(Dual[2], "index")
		if GUICommon.IsDualSwap(actor):
			Levels = [Levels[1], Levels[0], Levels[2]]

	# check the class that can be level (single or dual)
	tmpNext = int(GetNextLevelExp (Levels[0], Class) )
	return ((tmpNext != 0 or Levels[0] == 0) and tmpNext <= xp)

# expects a list of character levels of all classes
# returns sparse list of class ids (of same length)
def GetAllClasses (Levels):
	Class = [0]*len(Levels)
	for c in range(len(Levels)):
		if Levels[c] > 0:
			# level stats are implicitly keyed by class id
			Class[c] = c + 1
	return Class

# internal shared function for the various Setup* stat updaters
def _SetupLevels (pc, Level, offset=0, noclass=0):
	#storing levels as an array makes them easier to deal with
	if not Level:
		Levels = [IE_LEVEL, IE_LEVEL2, IE_LEVEL3]
		if GameCheck.IsIWD2():
			import IDLUCommon
			Levels = IDLUCommon.Levels
		Levels = [ GemRB.GetPlayerStat (pc, l)+offset for l in Levels ]
	else:
		Levels = []
		for level in Level:
			Levels.append (level+offset)

	if noclass:
		return Levels, -1, -1

	# adjust the class for multi/dual chars
	Class = [GemRB.GetPlayerStat (pc, IE_CLASS)]
	Multi = GUICommon.IsMultiClassed (pc, 1)
	Dual = GUICommon.IsDualClassed (pc, 1)
	NumClasses = 1
	if Multi[0]>1: #get each of the multi-classes
		NumClasses = Multi[0]
		Class = [Multi[1], Multi[2], Multi[3]]
	elif Dual[0]: #only worry about the newer class
		if Dual[0] == 3:
			Class = [CommonTables.KitList.GetValue (Dual[2], 7)]
		else:
			ClassRow = GUICommon.GetClassRowName(Dual[2], "index")
			Class = [CommonTables.Classes.GetValue (ClassRow, "ID")]
		#assume Level is correct if passed
		if GUICommon.IsDualSwap(pc) and not Level:
			Levels = [Levels[1], Levels[0], Levels[2]]
	elif GameCheck.IsIWD2():
		Class = GetAllClasses (Levels)

	return Levels, NumClasses, Class

def SetupSavingThrows (pc, Level=None):
	"""Updates an actors saving throws based upon level.

	Level should contain the actors current level.
	If Level is None, it is filled with the actors current level."""

	Levels, NumClasses, Class = _SetupLevels (pc, Level, -1)
	if NumClasses > len(Levels):
		return

	#get some basic values
	Race = GemRB.GetPlayerStat (pc, IE_RACE)

	#see if we can add racial bonuses to saves
	Race = CommonTables.Races.GetRowName (CommonTables.Races.FindValue (3, Race) )
	RaceSaveTableName = CommonTables.Races.GetValue (Race, "SAVE", GTV_STR)
	RaceSaveTable = None
	if RaceSaveTableName != "-1" and RaceSaveTableName != "*":
		Con = GemRB.GetPlayerStat (pc, IE_CON, 1)-1
		RaceSaveTable = GemRB.LoadTable (RaceSaveTableName)
		if Con >= RaceSaveTable.GetColumnCount ():
			Con = RaceSaveTable.GetColumnCount () - 1

	#preload our tables to limit multi-classed lookups
	SaveTables = []
	ClassBonus = 0
	for i in range (NumClasses):
		RowName = GUICommon.GetClassRowName (Class[i], "class")
		SaveName = CommonTables.Classes.GetValue (RowName, "SAVE", GTV_STR)
		SaveTables.append (GemRB.LoadTable (SaveName) )
		#use numeric value
		ClassBonus += CommonTables.ClassSkills.GetValue (RowName, "SAVEBONUS", GTV_INT)

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
			CurrentSave -= RaceSaveTable.GetValue (row, Con)

		#add class bonuses if applicable (paladin)
		CurrentSave -= ClassBonus
		GemRB.SetPlayerStat (pc, IE_SAVEVSDEATH+row, CurrentSave)
	return

def GetNextLevelFromExp (XP, Class):
	"""Gets the next level based on current experience."""

	ClassName = GUICommon.GetClassRowName (Class, "class")
	Row = CommonTables.NextLevel.GetRowIndex (ClassName)
	NLNumCols = CommonTables.NextLevel.GetColumnCount()
	for i in range(1, NLNumCols):
		if XP < CommonTables.NextLevel.GetValue (Row, i):
			return i
	# fix hacked characters that have more xp than the xp cap
	return NLNumCols

def SetupThaco (pc, Level=None):
	"""Updates an actors THAC0 based upon level.

	Level should contain the actors current level.
	If Level is None it is filled with the actors current level."""

	Levels, NumClasses, Class = _SetupLevels (pc, Level, -1)
	if NumClasses > len(Levels):
		return

	#get some basic values
	ThacoTable = GemRB.LoadTable ("THAC0")

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
		ClassName = GUICommon.GetClassRowName (Class[i], "class")
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

	LevelDiffs, NumClasses, Class = _SetupLevels (pc, LevelDiff)
	if NumClasses > len(LevelDiffs):
		return

	#get some basic values
	LoreTable = GemRB.LoadTable ("lore")

	#loop through each class and update the lore value if we have
	CurrentLore = GemRB.GetPlayerStat (pc, IE_LORE, 1)
	for i in range (NumClasses):
		#correct unlisted progressions
		ClassName = GUICommon.GetClassRowName (Class[i], "class")
		if ClassName == "SORCERER":
			ClassName = "MAGE"
		elif ClassName == "MONK": #monks have a rate of 1, so this is arbitrary
			ClassName = "CLERIC"

		#add the lore from this class to the total lore
		TmpLore = LevelDiffs[i] * LoreTable.GetValue (ClassName, "RATE", GTV_INT)
		if TmpLore:
			CurrentLore += TmpLore

	#update our lore value
	GemRB.SetPlayerStat (pc, IE_LORE, CurrentLore)
	return

def SetupHP (pc, Level=None, LevelDiff=None):
	"""Updates an actors hp based upon level.

	Level should contain the actors current level.
	LevelDiff should contain the change in levels.
	Level and LevelDiff must be of the same length.
	If either are None, they are filled with the actors current level."""

	Levels, NumClasses, Class = _SetupLevels (pc, Level, noclass=1)
	LevelDiffs, NumClasses, Class = _SetupLevels (pc, LevelDiff, noclass=1)
	if len (Levels) != len (LevelDiffs):
		return

	#adjust the class for multi/dual chars
	Class = [GemRB.GetPlayerStat (pc, IE_CLASS)]
	Multi = GUICommon.IsMultiClassed (pc, 1)
	Dual = GUICommon.IsDualClassed (pc, 1)
	NumClasses = 1
	if Multi[0]>1: #get each of the multi-classes
		NumClasses = Multi[0]
		Class = [Multi[1], Multi[2], Multi[3]]
	elif Dual[0]: #only worry about the newer class
		#we only get the hp bonus if the old class is reactivated
		if (Levels[0]<=Levels[1]):
			return
		if Dual[0] == 3:
			Class = [CommonTables.KitList.GetValue (Dual[2], 7)]
		else:
			ClassRow = GUICommon.GetClassRowName(Dual[2], "index")
			Class = [CommonTables.Classes.GetValue (ClassRow, "ID")]
		#if Level and LevelDiff are passed, we assume it is correct
		if GUICommon.IsDualSwap(pc) and not Level and not LevelDiff:
			LevelDiffs = [LevelDiffs[1], LevelDiffs[0], LevelDiffs[2]]
	elif GameCheck.IsIWD2():
		Class = GetAllClasses (Levels)
	if NumClasses>len(Levels):
		return

	#get the correct hp for barbarians
	Kit = GUICommon.GetKitIndex (pc)
	ClassName = None
	if Kit and not Dual[0] and Multi[0]<2:
		KitName = CommonTables.KitList.GetValue (Kit, 0, GTV_STR)
		if CommonTables.Classes.GetRowIndex (KitName) >= 0:
			ClassName = KitName

	# determine the minimum hp roll
	ConBonTable = GemRB.LoadTable ("hpconbon")
	MinRoll = ConBonTable.GetValue (GemRB.GetPlayerStat (pc, IE_CON)-1, 2) # MIN_ROLL column

	#loop through each class and update the hp
	OldHP = GemRB.GetPlayerStat (pc, IE_MAXHITPOINTS, 1)
	CurrentHP = 0
	Divisor = float (NumClasses)
	if GameCheck.IsIWD2():
		# hack around so we can reuse more of the main loop
		NumClasses = len(Levels)
		Divisor = 1.0
	for i in range (NumClasses):
		if GameCheck.IsIWD2() and not Class[i]:
			continue

		#check this classes hp table for any gain
		if not ClassName or NumClasses > 1:
			ClassName = GUICommon.GetClassRowName (Class[i], "class")
		HPTable = CommonTables.Classes.GetValue (ClassName, "HP")
		HPTable = GemRB.LoadTable (HPTable)

		#make sure we are within table ranges
		MaxLevel = HPTable.GetRowCount()-1
		LowLevel = Levels[i]-LevelDiffs[i]
		HiLevel = Levels[i]
		if LowLevel >= HiLevel:
			continue
		if LowLevel < 0:
			LowLevel = 0
		elif LowLevel > MaxLevel:
			LowLevel = MaxLevel
		if HiLevel < 0:
			HiLevel = 0
		elif HiLevel > MaxLevel:
			HiLevel = MaxLevel

		#add all the hp for the given level
		#we use ceil to ensure each class gets hp
		for level in range(LowLevel, HiLevel):
			sides = HPTable.GetValue (level, 0)
			rolls = HPTable.GetValue (level, 1)
			bonus = HPTable.GetValue (level, 2)

			# we only do a roll on core difficulty or higher
			# and if maximum HP rolls (bg2 and later) are disabled
			# and/or if it is bg1 chargen (I guess too many testers got annoyed)
			# BUT when we do roll, constitution gives a kind of a luck bonus to the roll
			if rolls:
				if GemRB.GetVar ("Difficulty Level") >= 3 and not GemRB.GetVar ("Maximum HP") \
				and not (GameCheck.IsBG1() and LowLevel == 0) and MinRoll < sides:
					if MinRoll > 1:
						roll = GemRB.Roll (rolls, sides, bonus)
						if roll-bonus < MinRoll:
							roll = MinRoll + bonus
						AddedHP = int (roll / Divisor + 0.5)
					else:
						AddedHP = int (GemRB.Roll (rolls, sides, bonus) / Divisor + 0.5)
				else:
					AddedHP = int ((rolls * sides + bonus) / Divisor + 0.5)
			else:
				AddedHP = int (bonus / Divisor + 0.5)
			# ensure atleast 1hp is given
			# this is safe for inactive dualclass levels too (handled above)
			if AddedHP == 0:
				AddedHP = 1
			CurrentHP += AddedHP

	#update our hp values
	GemRB.SetPlayerStat (pc, IE_MAXHITPOINTS, CurrentHP+OldHP)
	# HACK: account also for the new constitution bonus for the current hitpoints
	GemRB.SetPlayerStat (pc, IE_HITPOINTS, GemRB.GetPlayerStat (pc, IE_HITPOINTS, 1)+CurrentHP+5)
	return

def ApplyFeats(MyChar):

	#don't mess with feats outside of IWD2
	if not GameCheck.IsIWD2():
		return

	#feats giving a single innate ability
	SetSpell(MyChar, "SPIN111", FEAT_WILDSHAPE_BOAR)
	SetSpell(MyChar, "SPIN197", FEAT_MAXIMIZED_ATTACKS)
	SetSpell(MyChar, "SPIN231", FEAT_ENVENOM_WEAPON)
	SetSpell(MyChar, "SPIN245", FEAT_WILDSHAPE_PANTHER)
	SetSpell(MyChar, "SPIN246", FEAT_WILDSHAPE_SHAMBLER)
	SetSpell(MyChar, "SPIN275", FEAT_POWER_ATTACK)
	SetSpell(MyChar, "SPIN276", FEAT_EXPERTISE)
	SetSpell(MyChar, "SPIN277", FEAT_ARTERIAL_STRIKE)
	SetSpell(MyChar, "SPIN278", FEAT_HAMSTRING)
	SetSpell(MyChar, "SPIN279", FEAT_RAPID_SHOT)

	#extra rage
	level = GemRB.GetPlayerStat(MyChar, IE_LEVELBARBARIAN)
	if level>0:
		if level>=15:
			GemRB.RemoveSpell(MyChar, "SPIN236")
			Spell = "SPIN260"
		else:
			GemRB.RemoveSpell(MyChar, "SPIN260")
			Spell = "SPIN236"
		cnt = GemRB.GetPlayerStat (MyChar, IE_FEAT_EXTRA_RAGE)+(level+3)/4
		GUICommon.MakeSpellCount(MyChar, Spell, cnt)
	else:
		GemRB.RemoveSpell(MyChar, "SPIN236")
		GemRB.RemoveSpell(MyChar, "SPIN260")

	#extra smiting
	level = GemRB.GetPlayerStat(MyChar, IE_LEVELPALADIN)
	if level>1:
		cnt = GemRB.GetPlayerStat (MyChar, IE_FEAT_EXTRA_SMITING) + 1
		GUICommon.MakeSpellCount(MyChar, "SPIN152", cnt)
	else:
		GemRB.RemoveSpell(MyChar, "SPIN152")

	#extra turning
	level = GemRB.GetPlayerStat(MyChar, IE_TURNUNDEADLEVEL)
	if level>0:
		cnt = GUICommon.GetAbilityBonus(MyChar, IE_CHR) + 3
		if cnt<1: cnt = 1
		cnt += GemRB.GetPlayerStat (MyChar, IE_FEAT_EXTRA_TURNING)
		GUICommon.MakeSpellCount(MyChar, "SPIN970", cnt)
	else:
		GemRB.RemoveSpell(MyChar, "SPIN970")

	#stunning fist
	if GemRB.HasFeat (MyChar, FEAT_STUNNING_FIST):
		cnt = GemRB.GetPlayerStat(MyChar, IE_CLASSLEVELSUM) / 4
		GUICommon.MakeSpellCount(MyChar, "SPIN232", cnt)
	else:
		GemRB.RemoveSpell(MyChar, "SPIN232")

	#remove any previous SPLFOCUS
	#GemRB.ApplyEffect(MyChar, "RemoveEffects",0,0,"SPLFOCUS")
	#spell focus stats
	SPLFocusTable = GemRB.LoadTable ("splfocus")
	for i in range(SPLFocusTable.GetRowCount()):
		Row = SPLFocusTable.GetRowName(i)
		Stat = SPLFocusTable.GetValue(Row, "STAT", GTV_STAT)
		if Stat:
			Column = GemRB.GetPlayerStat(MyChar, Stat)
			if Column:
				Value = SPLFocusTable.GetValue(i, Column)
				if Value:
					#add the effect, value could be 2 or 4, timing mode is 8 - so it is not saved
					GemRB.ApplyEffect(MyChar, "SpellFocus", Value, i,"","","","SPLFOCUS", 8)
	return

def SetSpell(pc, SpellName, Feat):
	if GemRB.HasFeat (pc, Feat):
		GUICommon.MakeSpellCount(pc, SpellName, 1)
	else:
		GemRB.RemoveSpell(pc, SpellName)
	return

