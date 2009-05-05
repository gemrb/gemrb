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
# $Id$
# character generation - ability; next skills/profs/spells (CharGen6)
import GemRB
from CharGenCommon import *
from GUICG7 import RemoveKnownSpells

def OnLoad():
	MyChar = GemRB.GetVar ("Slot")

	# nullify our thieving skills
	SkillsTable = GemRB.LoadTableObject ("skills")
	SkillsCount = SkillsTable.GetRowCount () - 2 # first 2 are starting values
	for i in range (SkillsCount):
		StatID = SkillsTable.GetValue (i+2, 2)
		GemRB.SetVar ("Skill "+str(i), 0)
		GemRB.SetPlayerStat (MyChar, StatID, 0)

	# nullify our proficiencies
	ProfsTable = GemRB.LoadTableObject ("weapprof")
	ProfsCount = ProfsTable.GetRowCount () - 8 # we don't want bg1 profs
	for i in range (ProfsCount):
		StatID = ProfsTable.GetValue (i+8, 0)
		GemRB.SetVar ("Prof "+str(i), 0)
		GemRB.SetPlayerStat (MyChar, StatID, 0)

	# nully other variables
	GemRB.SetVar ("HateRace", 0)

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

	RemoveKnownSpells (IE_SPELL_TYPE_WIZARD)
	RemoveKnownSpells (IE_SPELL_TYPE_PRIEST)

	# learn divine spells if appropriate
	ClassSkillsTable = GemRB.LoadTableObject ("clskills")
	#change this to GetPlayerStat once IE_CLASS is directly stored
	ClassTable = GemRB.LoadTableObject ("classes")
	ClassIndex = GemRB.GetVar ("Class")-1
	Class = ClassTable.GetValue (ClassIndex, 5)
	MyChar = GemRB.GetVar ("Slot")
	TableName = ClassSkillsTable.GetValue (Class, 1, 0)

	if TableName == "*":
		# it isn't a cleric or paladin, so check for druids and rangers
		TableName = ClassSkillsTable.GetValue (Class, 0, 0)
		ClassFlag = 0x4000
	else:
		ClassFlag = 0x8000
	# check for cleric/rangers, who get access to all the spells
	# possibly redundant block (see SPL bit 0x0020)
	if TableName == "MXSPLPRS" and ClassSkillsTable.GetValue (Class, 0, 0) != "*":
		ClassFlag = 0

	# nulify the memorizable spell counts
	for type in [ "MXSPLPRS", "MXSPLPAL", "MXSPLRAN", "MXSPLDRU" ]:
		UnsetupSpellLevels (MyChar, type, IE_SPELL_TYPE_PRIEST, 1)
	for type in [ "MXSPLWIZ", "MXSPLSRC", "MXSPLBRD" ]:
		UnsetupSpellLevels (MyChar, type, IE_SPELL_TYPE_WIZARD, 1)

	if TableName != "*":
		SetupSpellLevels(MyChar, TableName, IE_SPELL_TYPE_PRIEST, 1)
		Learnable = GetLearnablePriestSpells( ClassFlag, GemRB.GetVar ("Alignment"), 1)
		for i in range(len(Learnable) ):
			GemRB.LearnSpell (MyChar, Learnable[i], 0)

	return
