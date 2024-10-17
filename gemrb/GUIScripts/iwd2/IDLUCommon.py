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
from ie_feats import *
from ie_restype import RES_SPL
from ie_spells import LS_MEMO
from GUIDefines import *

FeatSPLs = []

# barbarian, bard, cleric, druid, fighter, monk, paladin, ranger, rogue, sorcerer, wizard
# same order as in classes.2da / class IDs
Levels = [ IE_LEVELBARBARIAN, IE_LEVELBARD, IE_LEVELCLERIC, IE_LEVELDRUID, \
	IE_LEVEL, IE_LEVELMONK, IE_LEVELPALADIN, IE_LEVELRANGER, IE_LEVEL3, \
	IE_LEVELSORCERER, IE_LEVEL2 ]

def AddResistances(MyChar, rowname, table):
	resistances = GemRB.LoadTable (table)
	# add it to dmginfo.2da if we ever need it elsewhere
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
	LevelDiff = GemRB.GetVar ("LevelDiff") or 0 # only set during level-up
	Level = GemRB.GetPlayerStat(pc, Levels[Class-1]) + LevelDiff
	Level -= 1 # we'll use it as row index only

	RowName = GUICommon.GetClassRowName (Class, "class")
	SaveName = CommonTables.Classes.GetValue (RowName, "SAVE", GTV_STR)
	ClassSaveTable = GemRB.LoadTable (SaveName)
	if Level+1 > ClassSaveTable.GetRowCount():
		print("SetupSavingThrows: too high level, ignoring!")
		return

	# save the saves
	# fortitude, reflex, will: colnames match stat order
	# the tables contain absolute values, so there's no need to compute a diff
	# HOWEVER, permanent effects' effects would get obliterated if we didn't
	for i in range(3):
		Base = GemRB.GetPlayerStat (pc, IE_SAVEFORTITUDE+i, 1)
		OldClassBonus = ClassSaveTable.GetValue (Level-LevelDiff-1, i) # default is 0 if we underflow
		ClassBonus = ClassSaveTable.GetValue (Level, i)
		GemRB.SetPlayerStat (pc, IE_SAVEFORTITUDE+i, Base + ClassBonus-OldClassBonus)

def LearnAnySpells (pc, BaseClassName, chargen=1):
	MageTable = CommonTables.ClassSkills.GetValue (BaseClassName, "MAGESPELL", GTV_STR)
	ClericTable = CommonTables.ClassSkills.GetValue (BaseClassName, "CLERICSPELL", GTV_STR)

	# sorcerer types need this hack to not get too many spells
	# for them the castable and known count progress differently
	if MageTable == "MXSPLSOR":
		MageTable = "SPLSRCKN"
	elif MageTable == "MXSPLBRD":
		MageTable = "SPLBRDKN"

	# this is the current caster level to update to (for SetupSpellLevels)
	level = 1
	if not chargen:
		# LevelDiff + whatever caster level of this type we already had
		level = GemRB.GetVar ("LevelDiff") or 0
		classIndex = CommonTables.Classes.GetRowIndex (BaseClassName)
		level += GemRB.GetPlayerStat (pc, Levels[classIndex], 1)

	bonus = 0 # ignore high-stat granted bonus spell slots
	for table in MageTable, ClericTable:
		if table == "*":
			continue

		booktype = CommonTables.ClassSkills.GetValue (BaseClassName, "SPLTYPE")
		# set our memorizable counts
		Spellbook.SetupSpellLevels (pc, table, booktype, level)

		if table == MageTable:
			continue

		# learn all our priest spells up to the level we can learn
		for slevel in range (9):
			if GemRB.GetMemorizableSpellsCount (pc, booktype, slevel, bonus) <= 0:
				# actually checks level+1 (runs if level-1 has memorizations)
				Spellbook.LearnPriestSpells (pc, slevel, booktype)
				break

		# FIXME: healing spell hack until we have a separate memo window/phase
		idx = Spellbook.HasSpell (pc, booktype, 0, "sppr103")
		if idx != -1:
			GemRB.MemorizeSpell (pc, booktype, 0, idx, 1)

		# set up shapes for druids
		if not chargen and booktype == IE_IWD2_SPELL_DRUID:
			ShapeTable = GemRB.LoadTable ("mxdrdshp")
			newUses = ShapeTable.GetValue (str(level), "USES_PER_DAY")
			booktype = IE_IWD2_SPELL_SHAPE
			extras = GemRB.HasFeat (pc, FEAT_EXTRA_SHAPESHIFTING)
			GemRB.SetMemorizableSpellsCount (pc, newUses+extras, booktype, 0)

		if booktype != IE_IWD2_SPELL_CLERIC:
			return

		# learn any domain spells too
		booktype = IE_IWD2_SPELL_DOMAIN
		Spellbook.SetupSpellLevels (pc, table, booktype, level)

		for slevel in range (9):
			if GemRB.GetMemorizableSpellsCount (pc, booktype, slevel, bonus) <= 0:
				# actually checks level+1 (runs if level-1 has memorizations)
				Spellbook.LearnPriestSpells (pc, slevel, booktype, BaseClassName)
				break

def SetSpell(pc, SpellName, Feat):
	if GemRB.HasFeat (pc, Feat):
		MakeSpellCount(pc, SpellName, 1)
	else:
		GemRB.RemoveSpell(pc, SpellName)
	return

def MakeSpellCount (pc, spell, count):
	have = GemRB.CountSpells (pc, spell, -1, 1)
	if count <= have:
		return
	# only used for innates, which are all level 1
	Spellbook.LearnSpell (pc, spell, IE_IWD2_SPELL_INNATE, 0, count - have, LS_MEMO)
	return

def SetSpellFocus (pc):
	SPLFocusTable = GemRB.LoadTable ("splfocus")
	for i in range(SPLFocusTable.GetRowCount()):
		Row = SPLFocusTable.GetRowName (i)
		Stat = SPLFocusTable.GetValue (Row, "STAT", GTV_STAT)
		if not Stat:
			continue
		Column = GemRB.GetPlayerStat (pc, Stat)
		if not Column:
			continue
		bonus = SPLFocusTable.GetValue (i, Column)
		if not bonus:
			continue
		# ok, so we have the feat and it does have a bonus assigned in the table
		# make sure we don't duplicate the effect, since it is cummulative
		focused = GemRB.CountEffects (pc, "SpellFocus", bonus, i)
		if focused:
			continue
		# what if we're upgrading to greater spell focus? Remove and readd
		focused = GemRB.CountEffects (pc, "SpellFocus", -1, i)
		if focused:
			GemRB.DispelEffect (pc, "SpellFocus", i)
		GemRB.ApplyEffect (pc, "SpellFocus", bonus, i, "", "", "", "SPLFOCUS", 1)

# make sure this function remains idempotent
def LearnFeatInnates (pc, party, setup):
	global FeatSPLs

	# npcs don't have these feat spells yet, eg. 00solbas is missing power attack
	# party members are handled more sanely during cg/level up
	if setup or not party:
		SetSpell (pc, "SPIN111", FEAT_WILDSHAPE_BOAR)
		SetSpell (pc, "SPIN197", FEAT_MAXIMIZED_ATTACKS)
		SetSpell (pc, "SPIN231", FEAT_ENVENOM_WEAPON)
		SetSpell (pc, "SPIN245", FEAT_WILDSHAPE_PANTHER)
		SetSpell (pc, "SPIN246", FEAT_WILDSHAPE_SHAMBLER)
		SetSpell (pc, "SPIN275", FEAT_POWER_ATTACK)
		SetSpell (pc, "SPIN276", FEAT_EXPERTISE)
		SetSpell (pc, "SPIN277", FEAT_ARTERIAL_STRIKE)
		SetSpell (pc, "SPIN278", FEAT_HAMSTRING)
		SetSpell (pc, "SPIN279", FEAT_RAPID_SHOT)
		LearnExtraFeats (pc)

		# recheck if we need to do anything about new spell focus feats
		SetSpellFocus (pc)

	# apply the rest on area entry
	if setup == True:
		return

	# cache feat spell existence checks, since this gets called on every actor
	if not FeatSPLs:
		for i in range(96): # MAX_FEATS
			featSPL = "FEAT{:02x}".format(i)
			if GemRB.HasResource (featSPL, RES_SPL, 1):
				FeatSPLs.append (featSPL)

	# feats with spell payloads
	# these spells have non-permanent timing, don't survive saves, so
	# we have to reapply them on everyone
	for i in range(96): # MAX_FEATS
		level = GemRB.HasFeat (pc, i)
		if not level:
			continue
		featSPL = "FEAT{:02x}".format(i)
		if featSPL not in FeatSPLs:
			continue
		# make sure it's not applied already if called from level up
		if setup != 2 or GemRB.CountEffects (pc, "", -1, -1, "", featSPL) == 0:
			GemRB.ApplySpell (pc, featSPL, pc)

# make sure this function remains idempotent
def LearnExtraFeats (MyChar):
	# extra rage
	level = GemRB.GetPlayerStat (MyChar, IE_LEVELBARBARIAN)
	if level > 0:
		if level >= 15:
			GemRB.RemoveSpell (MyChar, "SPIN236")
			Spell = "SPIN260"
		else:
			GemRB.RemoveSpell (MyChar, "SPIN260")
			Spell = "SPIN236"
		cnt = GemRB.GetPlayerStat (MyChar, IE_FEAT_EXTRA_RAGE) + (level + 3) // 4
		MakeSpellCount (MyChar, Spell, cnt)
	else:
		GemRB.RemoveSpell(MyChar, "SPIN236")
		GemRB.RemoveSpell(MyChar, "SPIN260")

	# extra smiting
	level = GemRB.GetPlayerStat(MyChar, IE_LEVELPALADIN)
	if level > 1:
		cnt = GemRB.GetPlayerStat (MyChar, IE_FEAT_EXTRA_SMITING) + 1
		MakeSpellCount (MyChar, "SPIN152", cnt)
	else:
		GemRB.RemoveSpell (MyChar, "SPIN152")

	# extra turning
	level = GemRB.GetPlayerStat (MyChar, IE_TURNUNDEADLEVEL)
	if level>0:
		cnt = GUICommon.GetAbilityBonus (MyChar, IE_CHR) + 3
		if cnt<1: cnt = 1
		cnt += GemRB.GetPlayerStat (MyChar, IE_FEAT_EXTRA_TURNING)
		MakeSpellCount (MyChar, "SPIN970", cnt)
	else:
		GemRB.RemoveSpell (MyChar, "SPIN970")

	# stunning fist
	if GemRB.HasFeat (MyChar, FEAT_STUNNING_FIST):
		cnt = GemRB.GetPlayerStat(MyChar, IE_CLASSLEVELSUM) // 4
		MakeSpellCount (MyChar, "SPIN232", cnt)
	else:
		GemRB.RemoveSpell (MyChar, "SPIN232")
