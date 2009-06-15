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
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#
# $Id$
#
# GUICommon.py - common functions for GUIScripts of all game types

import GemRB
from ie_restype import *
from ie_spells import LS_MEMO
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
	Table = GemRB.LoadTableObject ("aligns")
	v = Table.FindValue (3, Alignment)
	Usability = Kit | Table.GetValue(v, 5)

	SpellsTable = GemRB.LoadTableObject ("spells")
	for i in range(SpellsTable.GetValue ("MAGE", str(Level), 1) ):
		SpellName = "SPWI%d%02d"%(Level,i+1)
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

	Table=GemRB.LoadTableObject("aligns")
	v = Table.FindValue(3, Alignment)
	#usability is the bitset we look for
	Usability=Table.GetValue(v, 5)

	SpellsTable = GemRB.LoadTableObject ("spells")
	for i in range(SpellsTable.GetValue ("PRIEST", str (Level), 1) ):
		SpellName = "SPPR%d%02d"%(Level,i+1)
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
	#don't die on a missing reference
	#FIXME: try to do this in a non-hard way?
	if not GemRB.HasResource (TableName, RES_2DA):
		if TableName == "MXSPLDRU":
			SetupSpellLevels (pc, "MXSPLPRS", Type, Level)
		return

	Table = GemRB.LoadTableObject (TableName)
	for i in range(Table.GetColumnCount ()):
		# do a string lookup since some tables don't have entries for all levels
		value = Table.GetValue (str(Level), str(i+1), 1)
		# specialist mages get an extra spell if they already know that level
		# FIXME: get a general routine to find specialists
		school = GemRB.GetVar("MAGESCHOOL")
		if Type == IE_SPELL_TYPE_WIZARD and school != 0:
			if value > 0:
				value += 1
		GemRB.SetMemorizableSpellsCount (pc, value, Type, i)
	return

def UnsetupSpellLevels (pc, TableName, Type, Level):
	#don't die on a missing reference
	#FIXME: try to do this in a non-hard way?
	if not GemRB.HasResource (TableName, RES_2DA):
		if TableName == "MXSPLDRU":
			UnsetupSpellLevels (pc, "MXSPLPRS", Type, Level)
		return

	Table = GemRB.LoadTableObject (TableName)
	for i in range(Table.GetColumnCount ()):
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

# only differentiates between bg1 and bg2 (maybe others too; by chance)
def GameIsBG1 ():
	return not GemRB.HasResource ("AR3900", RES_ARE)

def GameIsTOB ():
	return GemRB.HasResource ("worldm25", RES_WMP) and GemRB.GetVar("oldgame") == 0

def HasTOB ():
	return GemRB.HasResource ("worldm25", RES_WMP)

def GameIsHOW ():
	return GemRB.HasResource ("expmap", RES_WMP)

def GetIWDSpellButtonCount ():
	if GameIsHOW():
		return 24
	else:
		return 20

def SetGamedaysAndHourToken ():
	currentTime = GemRB.GetGameTime()
	days = currentTime / 7200
	hours = (currentTime % 7200) / 300
	GemRB.SetToken ('GAMEDAYS', str (days))
	GemRB.SetToken ('HOUR', str (hours))

# Returns -1 if not found; otherwise, the index of the spell
def HasSpell (Actor, SpellType, Level, Ref):
	# loop through each spell in the spell level and check for a matching ref
	for i in range (GemRB.GetKnownSpellsCount (Actor, SpellType, Level)):
		Spell = GemRB.GetKnownSpell(Actor, SpellType, Level, i)
		if Spell["SpellResRef"].upper() == Ref.upper(): # ensure case is the same
			return i

	# not found
	return -1

# Adds class/kit abilities
def AddClassAbilities (pc, table, Level=1, LevelDiff=1, align=-1):
	TmpTable = GemRB.LoadTableObject (table)

	# gotta stay positive
	if Level-LevelDiff < 0:
		return

	# we're doing alignment additions
	if align == -1:
		iMin = 0
		iMax = TmpTable.GetRowCount ()
	else:
		# alignment is expected to be the row required
		iMin = align
		iMax = align+1

	# make sure we don't go out too far
	jMin = Level-LevelDiff
	jMax = Level
	if jMax > TmpTable.GetColumnCount ():
		jMax = TmpTable.GetColumnCount ()

	for i in range(iMin, iMax):
		# apply each spell from each new class
		for j in range (jMin, jMax):
			ab = TmpTable.GetValue (i, j, 0)
			if ab and ab != "****":
				# seems all SPINs act like GA_*
				if ab[:4] == "SPIN":
					ab = "GA_" + ab

				# apply spell (AP_) or gain spell (GA_)
				if ab[:2] == "AP":
					GemRB.ApplySpell (pc, ab[3:])
				elif ab[:2] == "GA":
					SpellIndex = HasSpell (pc, IE_SPELL_TYPE_INNATE, 0, ab[3:])
					if SpellIndex < 0: # don't know it yet
						GemRB.LearnSpell (pc, ab[3:], LS_MEMO)
					else: # memorize another one
						GemRB.MemorizeSpell (pc, IE_SPELL_TYPE_INNATE, 0, SpellIndex)
				else:
					print "ERROR, unknown class ability (type): ", ab

# remove all class abilities up to a give level
# for dual-classing mainly
def RemoveClassAbilities (pc, table, Level):
	TmpTable = GemRB.LoadTableObject (table)

	# gotta stay positive
	if Level < 0:
		return

	# make sure we don't go out too far
	jMax = Level
	if jMax > TmpTable.GetColumnCount ():
		jMax = TmpTable.GetColumnCount ()

	for i in range(TmpTable.GetRowCount ()):
		for j in range (jMax):
			ab = TmpTable.GetValue (i, j, 0)
			if ab and ab != "****":
				# get the index
				SpellIndex = HasSpell (pc, IE_SPELL_TYPE_INNATE, 0, ab[3:])

				# seems all SPINs act like GA_*
				if ab[:4] == "SPIN":
					ab = "GA_" + ab

				# apply spell (AP_) or gain spell (GA_)?
				if ab[:2] == "AP":
					# TODO: implement
					GemRB.RemoveEffects (pc, ab[3:])
				elif ab[:2] == "GA":
					if SpellIndex >= 0:
						# TODO: get the correct counts to avoid removing an innate ability
						# given by more than one thing?
						GemRB.UnmemorizeSpell (pc, IE_SPELL_TYPE_INNATE, 0, SpellIndex)
						GemRB.RemoveSpell (pc, IE_SPELL_TYPE_INNATE, 0, SpellIndex)
				else:
					print "ERROR, unknown class ability (type): ", ab

