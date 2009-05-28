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
# $Id$
# character generation - ability; next skills/profs/spells (CharGen6)
import GemRB
from CharGenCommon import *
from GUICommonWindows import ClassSkillsTable
from LUSpellSelection import RemoveKnownSpells
from LUSkillsSelection import *
from LUProfsSelection import *

def OnLoad():
	MyChar = GemRB.GetVar ("Slot")

	# nullify our thieving skills
	SkillsNullify ()
	SkillsSave (MyChar)

	# nullify our proficiencies
	ProfsNullify ()

	# nully other variables
	GemRB.SetVar ("HatedRace", 0)

	# save our previous stats:
	# 	abilities
	AbilityTable = GemRB.LoadTableObject ("ability")
	AbilityCount = AbilityTable.GetRowCount ()

	# print our diagnostic as we loop (so as not to duplicate)
	print "CharGen6 output:"

	for i in range (AbilityCount):
		StatID = AbilityTable.GetValue (i, 3)
		StatName = AbilityTable.GetRowName (i)
		StatValue = GemRB.GetVar ("Ability "+str(i))
		GemRB.SetPlayerStat (MyChar, StatID, StatValue)
		print "\t",StatName,":\t", StatValue

	# TODO: don't all chars have an str mod, even if it isn't applied?
	#	so it should be the cores duty to decide whether or not the char
	#	has 18 str in game and adjust accordingly; you wouldn't want an
	#	18/00 char use draw upon holy might to boost his str, then have
	#	it re-roll when it comes back to normal
	# apply our extra str
	GemRB.SetPlayerStat (MyChar, IE_STREXTRA, GemRB.GetVar ("StrExtra"))
	print "\tSTREXTRA:\t",GemRB.GetVar ("StrExtra")
	
	DisplayOverview (6)

	RemoveKnownSpells (MyChar, IE_SPELL_TYPE_WIZARD, 1,9, 1)
	RemoveKnownSpells (MyChar, IE_SPELL_TYPE_PRIEST, 1,7, 1)

	# learn divine spells if appropriate
	Class = GemRB.GetPlayerStat (MyChar, IE_CLASS)
	TableName = ClassSkillsTable.GetValue (Class, 1, 0) # cleric spells

	if TableName == "*": # only druid spells or no spells at all
		TableName = ClassSkillsTable.GetValue (Class, 0, 0)
		ClassFlag = 0x4000
	elif ClassSkillsTable.GetValue (Class, 0, 0) != "*": # cleric and druid spells
		ClassFlag = 0
	else: # only cleric spells
		ClassFlag = 0x8000

	# nulify the memorizable spell counts
	for type in [ "MXSPLPRS", "MXSPLPAL", "MXSPLRAN", "MXSPLDRU" ]:
		UnsetupSpellLevels (MyChar, type, IE_SPELL_TYPE_PRIEST, 1)
	for type in [ "MXSPLWIZ", "MXSPLSRC", "MXSPLBRD" ]:
		UnsetupSpellLevels (MyChar, type, IE_SPELL_TYPE_WIZARD, 1)

	if TableName != "*":
		SetupSpellLevels (MyChar, TableName, IE_SPELL_TYPE_PRIEST, 1)
		Learnable = GetLearnablePriestSpells (ClassFlag, GemRB.GetPlayerStat (MyChar, IE_ALIGNMENT), 1)
		for i in range(len(Learnable) ):
			GemRB.LearnSpell (MyChar, Learnable[i], IE_SPELL_TYPE_PRIEST)

	return
