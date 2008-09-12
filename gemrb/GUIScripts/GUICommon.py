# -*-python-*-
# GemRB - Infinity Engine Emulator
# Copyright (C) 2003 The GemRB Project
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
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
#
# $Id$
#
# GUICommon.py - common functions for GUIScripts of all game types

import GemRB
from ie_restype import *
from GUIDefines import *

OtherWindowFn = None
#global OtherWindowFn

def CloseOtherWindow (NewWindowFn):
	global OtherWindowFn

	GemRB.LeaveContainer()
	if OtherWindowFn and OtherWindowFn != NewWindowFn:
		OtherWindowFn ()
		OtherWindowFn = NewWindowFn
		return 0
	elif OtherWindowFn:
		OtherWindowFn = None
		return 1
	else:
		OtherWindowFn = NewWindowFn
		return 0

def GetMageSpells (Kit, Alignment, Level):
	MageSpells = []
	SpellType = 99
	Table = GemRB.LoadTable ("aligns")
	v = GemRB.FindTableValue (Table, 3, Alignment)
	Usability = Kit | GemRB.GetTableValue(Table, v, 5)

	for i in range(100):
		SpellName = "SPWI%d%02d"%(Level,i)
		ms = GemRB.GetSpell (SpellName, 1)
		if ms == None:
			continue

		if Usability & ms['SpellExclusion']:
			SpellType = 0
		else:
			SpellType = 1
			if Kit & (1 << ms['SpellSchool']+5): # of matching specialist school
				SpellType = 2
			# Wild mage spells are of normal schools, so we have to find them
			# separately. Generalists can learn any spell but the wild ones, so
			# we check if the mage is wild and if a generalist wouldn't be able
			# to learn the spell.
			if Kit == 0x8000 and (0x4000 & ms['SpellExclusion']):
				SpellType = 2
		MageSpells.append ([SpellName, SpellType])

	return MageSpells

def GetLearnableMageSpells (Kit, Alignment, Level):
	Learnable = []

	for Spell in GetMageSpells (Kit, Alignment, Level):
		if Spell[1]: 
			Learnable.append (Spell[0])
	return Learnable

def GetLearnablePriestSpells (Class, Alignment, Level):
	Learnable =[]

	Table=GemRB.LoadTable("aligns")
	v = GemRB.FindTableValue(Table, 3, Alignment)
	#usability is the bitset we look for
	Usability=GemRB.GetTableValue(Table, v, 5)

	for i in range(100):
		SpellName = "SPPR%d%02d"%(Level,i)
		ms = GemRB.GetSpell(SpellName, 1)
		if ms == None:
			continue
		if Class & ms['SpellDivine']:
			continue
		if Usability & ms['SpellExclusion']:
			continue
		Learnable.append (SpellName)
	return Learnable

def SetupSpellLevels (pc, TableName, Type, Level):
	Table=GemRB.LoadTable (TableName)
	for i in range(GemRB.GetTableColumnCount (0)):
		value = GemRB.GetTableValue (Table, Level, i)
		# specialist mages get an extra spell if they already know that level
		# FIXME: get a general routine to find specialists
		school = GemRB.GetVar("MAGESCHOOL")
		if Type == IE_SPELL_TYPE_WIZARD and school != 0:
			if value > 0:
				value += 1
		GemRB.SetMemorizableSpellsCount (pc, value, Type, i)
	return

def UnsetupSpellLevels (pc, TableName, Type, Level):

	#BG2 has no mxspldru table? (don't die on missing spell tables)
	if not GemRB.HasResource (TableName, RES_2DA):
		return

	Table=GemRB.LoadTable (TableName)
	for i in range(GemRB.GetTableColumnCount (0)):
		GemRB.SetMemorizableSpellsCount (pc, 0, Type, i)
	return

def SetColorStat (Actor, Stat, Value):
	t = Value & 0xFF
	t |= t << 8
	t |= t << 16
	GemRB.SetPlayerStat (Actor, Stat, t)
	return

def CheckStat100 (Actor, Stat, Diff):
	mystat = GemRB.GetPlayerStat (Actor, Stat)
	goal = GemRB.Roll (1,100, Diff)
	if mystat>=goal:
		return True
	return False

def CheckStat20 (Actor, Stat, Diff):
	mystat = GemRB.GetPlayerStat (Actor, Stat)
	goal = GemRB.Roll (1,20, Diff)
	if mystat>=goal:
		return True
	return False

def GameIsTOB ():
	return GemRB.HasResource ("worldm25", RES_WMP)
