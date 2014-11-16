# -*-python-*-
# GemRB - Infinity Engine Emulator
# Copyright (C) 2014 The GemRB Project
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
# IDLUCommon.py - common functions related to leveling up in iwd2,
#                 that are incompatible with the other games

import GemRB
import CommonTables
import GUICommon
import Spellbook
from ie_stats import *
from GUIDefines import *

# barbarian, bard, cleric, druid, fighter, monk, paladin, ranger, rogue, sorcerer, wizard
# same order as in classes.2da / class IDs
Levels = [ IE_LEVELBARBARIAN, IE_LEVELBARD, IE_LEVELCLERIC, IE_LEVELDRUID, \
	IE_LEVEL, IE_LEVELMONK, IE_LEVELPALADIN, IE_LEVELRANGER, IE_LEVEL3, \
	IE_LEVELSORCERER, IE_LEVEL2 ]

def AddResistances(MyChar, rowname, table):
	resistances = GemRB.LoadTable (table)
	# add it to dmgtypes.2da if we ever need it elsewhere
	titles = { IE_RESISTFIRE:"FIRE", IE_RESISTCOLD:"COLD", IE_RESISTELECTRICITY:"ELEC", \
		IE_RESISTACID:"ACID", IE_RESISTMAGIC:"SPELL", IE_RESISTMAGICFIRE:"MAGIC_FIRE", \
		IE_RESISTMAGICCOLD:"MAGIC_COLD", IE_RESISTSLASHING:"SLASHING", \
		IE_RESISTCRUSHING:"BLUDGEONING", IE_RESISTPIERCING:"PIERCING", IE_RESISTMISSILE:"MISSILE" }

	for resistance in titles:
		base = GemRB.GetPlayerStat (MyChar, resistance, 1)
		extra = resistances.GetValue (rowname, titles[resistance])
		GemRB.SetPlayerStat (MyChar, resistance, base+extra)
	return

# returns the race or subrace
def GetRace (pc):
	Race = GemRB.GetPlayerStat (pc, IE_RACE)
	Subrace = GemRB.GetPlayerStat (pc, IE_SUBRACE)
	if Subrace:
		Race = Race<<16 | Subrace
	return CommonTables.Races.FindValue (3, Race)

def SetupSavingThrows (pc, Class, Chargen=False):
	"""Updates an actors saving throws based upon level.

	Class should contain the class to consider for saving throw level-up.
	If Chargen is true, it will also add any racial boni."""

	# check if there are any racial bonuses to saves
	# they are constant in iwd2, regardless of level
	if Chargen:
		RaceSaveTable = GemRB.LoadTable ("SAVERACE")
		RaceName = CommonTables.Races.GetRowName (GetRace (pc))
		Race = RaceSaveTable.GetRowIndex (RaceName)
		# fortitude, reflex, will: colnames match stat order
		for i in range(3):
			RacialBonus = RaceSaveTable.GetValue (Race, i)
			GemRB.SetPlayerStat (pc, IE_SAVEFORTITUDE+i, RacialBonus)

	# class-based saving throw progression
	# get the actual xp level of the passed class
	Level = GemRB.GetPlayerStat(pc, Levels[Class-1])

	RowName = GUICommon.GetClassRowName (Class, "class")
	SaveName = CommonTables.Classes.GetValue (RowName, "SAVE", 0)
	ClassSaveTable = GemRB.LoadTable (SaveName)
	if Level > ClassSaveTable.GetRowCount():
		print "SetupSavingThrows: too high level, ignoring!"
		return

	# save the saves
	# fortitude, reflex, will: colnames match stat order
	# the tables contain absolute values, so there's no need to compute a diff
	# HOWEVER, permanent effects' effects would get obliterated if we didn't
	for i in range(3):
		Base = GemRB.GetPlayerStat (pc, IE_SAVEFORTITUDE+i, 1)
		OldClassBonus = ClassSaveTable.GetValue (Level-2, i) # default is 0 if we underflow
		ClassBonus = ClassSaveTable.GetValue (Level-1, i)
		GemRB.SetPlayerStat (pc, IE_SAVEFORTITUDE+i, Base + ClassBonus-OldClassBonus)

def LearnAnySpells (pc, BaseClassName):
	MageTable = CommonTables.ClassSkills.GetValue (BaseClassName, "MAGESPELL", 0)
	ClericTable = CommonTables.ClassSkills.GetValue (BaseClassName, "CLERICSPELL", 0)

	# sorcerer types need this hack to not get too many spells
	# for them the castable and known count progress differently
	if MageTable == "MXSPLSOR":
		MageTable = "SPLSRCKN"
	elif MageTable == "MXSPLBRD":
		MageTable = "SPLBRDKN"

	for table in MageTable, ClericTable:
		if table == "*":
			continue

		booktype = CommonTables.ClassSkills.GetValue (BaseClassName, "SPLTYPE")
		level = 1
		# set our memorizable counts
		Spellbook.SetupSpellLevels (pc, table, booktype, level)

		if table == MageTable:
			continue

		# learn all our priest spells up to the level we can learn
		for i in 1, 2:
			# also take care of domain spells
			if i == 2 and booktype == IE_IWD2_SPELL_CLERIC:
				booktype = IE_IWD2_SPELL_DOMAIN
				Spellbook.SetupSpellLevels (pc, table, booktype, level)
			elif i == 2:
				continue
			for level in range (9):
				print 111, level, booktype, GemRB.GetMemorizableSpellsCount (pc, booktype, level, 0)
				if GemRB.GetMemorizableSpellsCount (pc, booktype, level, 0) <= 0:
					# actually checks level+1 (runs if level-1 has memorizations)
					Spellbook.LearnPriestSpells (pc, level, booktype)
					break
