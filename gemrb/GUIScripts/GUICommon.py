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
from ie_slots import SLOT_ANY
from ie_stats import *

OtherWindowFn = None
#global OtherWindowFn

# only used in SetEncumbranceLabels, but that is called very often
StrModTable = StrModExTable = None

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

def GameIsBG1 ():
	return GemRB.GameType == "bg1"


def GameIsBG2 ():
	return GemRB.GameType == "bg2"

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

def UpdateInventorySlot (pc, Button, Slot, Type):
	Button.SetFont ("NUMBER")
	Button.SetBorder (0, 0,0,0,0, 128,128,255,64, 0,1)
	Button.SetBorder (1, 2,2,2,2, 32,32,255,0, 0,0)
	Button.SetBorder (2, 0,0,0,0, 255,128,128,64, 0,1)
	Button.SetFlags (IE_GUI_BUTTON_ALIGN_RIGHT | IE_GUI_BUTTON_ALIGN_TOP | IE_GUI_BUTTON_PICTURE, OP_OR)
	Button.SetText ("")

	if Slot == None:
		Button.SetFlags (IE_GUI_BUTTON_PICTURE, OP_NAND)
		if Type == "inventory":
			Button.SetTooltip (12013) # Personal Item
		elif Type == "ground":
			Button.SetTooltip (12011) # Ground Item
		else:
			Button.SetTooltip ("")
		Button.EnableBorder (0, 0)
		Button.EnableBorder (1, 0)
		Button.EnableBorder (2, 0)
	else:
		item = GemRB.GetItem (Slot['ItemResRef'])
		identified = Slot["Flags"] & IE_INV_ITEM_IDENTIFIED
		magical = Slot["Flags"] & IE_INV_ITEM_MAGICAL

		# TODO: figure out this mess
#		if item["StackAmount"] > 1:
#			Button.SetText (str (item["StackAmount"])) # wrong for potions, correct for arrows
#		if Slot["Usages0"] > 1:
#			Button.SetText (str (Slot["Usages0"])) # this has the correct value for potions, but not for gems (0)
		if item["StackAmount"] > 1:
			Button.SetText (str (Slot["Usages0"]))

		if not identified or item["ItemNameIdentified"] == -1:
			Button.SetTooltip (item["ItemName"])
			Button.EnableBorder (0, 1)
			Button.EnableBorder (1, 0)
		else:
			Button.SetTooltip (item["ItemNameIdentified"])
			Button.EnableBorder (0, 0)
			if magical:
				Button.EnableBorder (1, 1)
			else:
				Button.EnableBorder (1, 0)

		if GemRB.CanUseItemType (SLOT_ANY, Slot['ItemResRef'], pc):
			Button.EnableBorder (2, 0)
		else:
			Button.EnableBorder (2, 1)

		Button.SetItemIcon (Slot['ItemResRef'], 0)

	return

def LearnPriestSpells (pc, level, mask):
	"""Learns all the priest spells through the given spell level.

	Mask distinguishes clerical and druidic spells."""
	if level > 7: # make sure we don't have too high a level
		level = 7

	# go through each level
	alignment = GemRB.GetPlayerStat (pc, IE_ALIGNMENT)
	for i in range (level):
		learnable = GetLearnablePriestSpells (mask, alignment, i+1)

		for spell in learnable:
			# if the spell isn't learned, learn it
			if HasSpell (pc, IE_SPELL_TYPE_PRIEST, i, spell) < 0:
				GemRB.LearnSpell (pc, spell)
	return

def SetEncumbranceLabels (Window, LabelID, Label2ID, pc):
	"""Displays the encumarance as a ratio of current to maximum."""
	global StrModTable, StrModExTable

	if not StrModTable:
		StrModTable = GemRB.LoadTableObject ("strmod")
		StrModExTable = GemRB.LoadTableObject ("strmodex")

	# Getting the character's strength
	sstr = GemRB.GetPlayerStat (pc, IE_STR)
	ext_str = GemRB.GetPlayerStat (pc, IE_STREXTRA)

	# encumbrance
	max_encumb = StrModTable.GetValue (sstr, 3) + StrModExTable.GetValue (ext_str, 3)
	encumbrance = GemRB.GetPlayerStat (pc, IE_ENCUMBRANCE)

	Label = Window.GetControl (LabelID)
	Label.SetText (str (encumbrance) + ":")

	Label2 = Window.GetControl (Label2ID)
	Label2.SetText (str (max_encumb) + ":")
	ratio = (0.0 + encumbrance) / max_encumb
	if ratio > 1.0:
		Label.SetTextColor (255, 0, 0)
		Label2.SetTextColor (255, 0, 0)
	elif ratio > 0.8:
		Label.SetTextColor (255, 255, 0)
		Label2.SetTextColor (255, 0, 0)
	else:
		Label.SetTextColor (255, 255, 255)
		Label2.SetTextColor (255, 0, 0)
	return

def GetActorClassTitle (actor):
	"""Returns the string representation of the actors class."""

	ClassTitle = GemRB.GetPlayerStat (actor, IE_TITLE1)

	if ClassTitle == 0:
		Class = GemRB.GetPlayerStat (actor, IE_CLASS)
		ClassIndex = ClassTable.FindValue ( 5, Class )
		KitIndex = GetKitIndex (actor)
		Multi = ClassTable.GetValue (ClassIndex, 4)
		Dual = IsDualClassed (actor, 1)

		if Multi and Dual[0] == 0: # true multi class
			ClassTitle = ClassTable.GetValue (ClassIndex, 2)
			ClassTitle = GemRB.GetString (ClassTitle)
		else:
			if Dual[0]: # dual class
				# first (previous) kit or class of the dual class
				if Dual[0] == 1:
					ClassTitle = KitListTable.GetValue (Dual[1], 2)
				elif Dual[0] == 2:
					ClassTitle = ClassTable.GetValue (Dual[1], 2)
				ClassTitle = GemRB.GetString (ClassTitle) + " / "
				ClassTitle += GemRB.GetString (ClassTable.GetValue (Dual[2], 2))
			else: # ordinary class or kit
				if KitIndex:
					ClassTitle = KitListTable.GetValue (KitIndex, 2)
				else:
					ClassTitle = ClassTable.GetValue (ClassIndex, 2)
				ClassTitle = GemRB.GetString (ClassTitle)

	if ClassTitle == "*":
		return 0
	return ClassTitle
