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
from ie_stats import *

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

def SetupSavingThrows (pc, Level=None, Chargen=False):
	"""Updates an actors saving throws based upon level.

	Level should contain the actors current level.
	If Level is None, it is filled with the actors current level.
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

