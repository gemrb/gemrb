# -*-python-*-
# GemRB - Infinity Engine Emulator
# Copyright (C) 2011 The GemRB Project
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
# a library of any functions for spell(book) managment

import GemRB
import CommonTables
from GUIDefines import *
from ie_stats import *
from ie_action import ACT_LEFT, ACT_RIGHT
from ie_spells import SP_IDENTIFY, SP_SURGE, LSR_KNOWN, LSR_LEVEL, LSR_STAT
from ie_restype import RES_2DA

#################################################################
# this is in the operator module of the standard python lib
def itemgetter(*items):
    if len(items) == 1:
        item = items[0]
        def g(obj):
            return obj[item]
    else:
        def g(obj):
            return tuple(obj[item] for item in items)
    return g
#################################################################
# routines for the actionbar spell access code

def GetUsableMemorizedSpells(actor, BookType):
	memorizedSpells = []
	spellResRefs = []
	for level in range (9):
		spellCount = GemRB.GetMemorizedSpellsCount (actor, BookType, level, False)
		for i in range (spellCount):
			Spell0 = GemRB.GetMemorizedSpell (actor, BookType, level, i)
			if not Spell0["Flags"]:
				# depleted, so skip
				continue
			if Spell0["SpellResRef"] in spellResRefs:
				# add another one, so we can get the count more cheaply later
				spellResRefs.append (Spell0["SpellResRef"])
				continue
			spellResRefs.append (Spell0["SpellResRef"])
			Spell = GemRB.GetSpell(Spell0["SpellResRef"])
			Spell['BookType'] = BookType # just another sorting key
			Spell['SpellIndex'] = GemRB.GetSpelldataIndex (actor, Spell["SpellResRef"], 1<<BookType) # crucial!
			if Spell['SpellIndex'] == -1:
				print "Error, memorized spell not found!", Spell["SpellResRef"]
			Spell['SpellIndex'] += 1000 * 1<<BookType
			memorizedSpells.append (Spell)

	if not len(memorizedSpells):
		return []

	# count and remove the duplicates
	memorizedSpells2 = []
	for spell in memorizedSpells:
		if spell["SpellResRef"] in spellResRefs:
			spell['MemoCount'] = spellResRefs.count(spell["SpellResRef"])
			while spell["SpellResRef"] in spellResRefs:
				spellResRefs.remove(spell["SpellResRef"])
			memorizedSpells2.append(spell)

	return memorizedSpells2

def GetKnownSpells(actor, BookType):
	knownSpells = []
	spellResRefs = []
	for level in range (9):
		spellCount = GemRB.GetKnownSpellsCount (actor, BookType, level)
		for i in range (spellCount):
			Spell0 = GemRB.GetKnownSpell (actor, BookType, level, i)
			if Spell0["SpellResRef"] in spellResRefs:
				continue
			spellResRefs.append (Spell0["SpellResRef"])
			Spell = GemRB.GetSpell(Spell0["SpellResRef"])
			Spell['BookType'] = BookType # just another sorting key
			Spell['MemoCount'] = 0
			Spell['SpellIndex'] = 1000 * 1<<BookType # this gets assigned properly later
			knownSpells.append (Spell)

	return knownSpells

def SortUsableSpells(memorizedSpells):
	# sort it by using the spldisp.2da table
	layout = CommonTables.SpellDisplay.GetValue ("USE_ROW", "ROWS")
	layout = CommonTables.SpellDisplay.GetRowName (layout)
	key1 = CommonTables.SpellDisplay.GetValue (layout, "KEY1")
	key2 = CommonTables.SpellDisplay.GetValue (layout, "KEY2")
	key3 = CommonTables.SpellDisplay.GetValue (layout, "KEY3")
	if key1:
		if key3 and key2:
			memorizedSpells = sorted(memorizedSpells, key=itemgetter(key1, key2, key3))
		elif key2:
			memorizedSpells = sorted(memorizedSpells, key=itemgetter(key1, key2))
		else:
			memorizedSpells = sorted(memorizedSpells, key=itemgetter(key1))

	return memorizedSpells

# Sets up all the (12) action buttons for a player character with different spell or innate icons.
# It also sets up the scroll buttons left and right if needed.
# If Start is supplied, it will skip the first few items (used when scrolling through the list)
# BookType is a spellbook type bitfield (1-mage, 2-priest, 4-innate)
# FIXME: iwd2 has even more types
# Offset is a control ID offset here for iwd2 purposes
def SetupSpellIcons(Window, BookType, Start=0, Offset=0):
	actor = GemRB.GameGetFirstSelectedActor ()

	# construct the spellbook of usable (not depleted) memorized spells
	# the getters expect the BookType as: 0 priest, 1 mage, 2 innate
	#  we almost need bitfield support for cleric/mages and the like
	if BookType == -1:
		# Nahal's reckless dweomer can use any known spell
		allSpells = GetKnownSpells (actor, IE_SPELL_TYPE_WIZARD)
	else:
		allSpells = []
		if BookType & (1<<IE_SPELL_TYPE_PRIEST): #1
			allSpells = GetUsableMemorizedSpells (actor, IE_SPELL_TYPE_PRIEST)
		if BookType & (1<<IE_SPELL_TYPE_WIZARD): #2
			allSpells += GetUsableMemorizedSpells (actor, IE_SPELL_TYPE_WIZARD)
		if BookType & (1<<IE_SPELL_TYPE_INNATE): #4
			allSpells += GetUsableMemorizedSpells (actor, IE_SPELL_TYPE_INNATE)

		if not len(allSpells):
			raise AttributeError ("Error, unknown BookType passed to SetupSpellIcons: %d! Bailing out!" %(BookType))
			return

	if BookType == -1:
		memorizedSpells = allSpells
		# reset Type, so we can choose the surgy spell instead of just getting a redraw of the action bar
		GemRB.SetVar("Type", 3)
	else:
		memorizedSpells = SortUsableSpells(allSpells)

	# start creating the controls
	import GUICommonWindows
	# TODO: ASCOL, ROWS
	#AsCol = CommonTables.SpellDisplay.GetValue (layout, "AS_COL")
	#Rows = CommonTables.SpellDisplay.GetValue (layout, "ROWS")
	More = len(memorizedSpells) > 12
	if not More and Start:
		More = True

	# scroll left button
	if More:
		Button = Window.GetControl (Offset)
		Button.SetText ("")
		if Start:
			#Button.SetActionIcon(globals(), ACT_LEFT, 0)
			GUICommonWindows.SetActionIconWorkaround (Button, ACT_LEFT, 0)
			Button.SetState (IE_GUI_BUTTON_UNPRESSED)
		else:
			Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_SET)
			Button.SetTooltip ("")
			Button.SetState (IE_GUI_BUTTON_DISABLED)

	# disable all spells if fx_disable_spellcasting was run with the same type
	# but only if there are any spells of that type to disable
	disabled_spellcasting = GemRB.GetPlayerStat(actor, IE_CASTING, 0)
	actionLevel = GemRB.GetVar ("ActionLevel")

	#order is: mage, cleric, innate, class, song, (defaults to 1, item)
	spellSections = [2, 4, 8, 16, 16]

	# create the spell icon buttons
	buttonCount = 12 - More # GUIBT_COUNT in PCStatsStruct
	for i in range (buttonCount):
		Button = Window.GetControl (i+Offset+More)

		if i+Start >= len(memorizedSpells):
			Button.SetState (IE_GUI_BUTTON_DISABLED)
			Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_SET)
			Button.SetText ("")
			Button.SetTooltip ("")
			continue
		Spell = memorizedSpells[i+Start]
		spellType = Spell['SpellType']
		if spellType > 4:
			spellType = 1
		else:
			spellType = spellSections[spellType]
		if BookType == -1:
			Button.SetVarAssoc ("Spell", Spell['SpellIndex']+i+Start)
		else:
			Button.SetVarAssoc ("Spell", Spell['SpellIndex'])

		# disable spells that should be cast from the inventory or can't be cast while silenced or ...
		# see splspec.2da for all the reasons; silence is handled elsewhere
		specialSpell = GemRB.CheckSpecialSpell(actor, Spell['SpellResRef'])
		specialSpell = (specialSpell & SP_IDENTIFY) or ((specialSpell & SP_SURGE) and actionLevel == 5)
		if specialSpell or (disabled_spellcasting&spellType):
			Button.SetState (IE_GUI_BUTTON_DISABLED)
			Button.EnableBorder(1, 0)
			#Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, GUICommonWindows.UpdateActionsWindow) # noop if it worked or not :)
		else:
			Button.SetState (IE_GUI_BUTTON_UNPRESSED)
			Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, GUICommonWindows.SpellPressed)

		if Spell['SpellbookIcon']:
			Button.SetSprites ("guibtbut", 0, 0,1,2,3)
			Button.SetSpellIcon (Spell['SpellResRef'], 1)
			Button.SetFlags (IE_GUI_BUTTON_PICTURE|IE_GUI_BUTTON_ALIGN_BOTTOM|IE_GUI_BUTTON_ALIGN_RIGHT, OP_SET)
			Button.SetTooltip (Spell['SpellName'])

			if Spell['MemoCount'] > 0 and BookType != -1:
				Button.SetText (str(Spell['MemoCount']))
			else:
				Button.SetText ("")

	# scroll right button
	if More:
		Button = Window.GetControl (Offset+buttonCount)
		GUICommonWindows.SetActionIconWorkaround (Button, ACT_RIGHT, buttonCount)
		Button.SetText ("")
		if len(memorizedSpells) - Start > 12:
			Button.SetState (IE_GUI_BUTTON_UNPRESSED)
		else:
			Button.SetState (IE_GUI_BUTTON_DISABLED)
			Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_SET)
			Button.SetTooltip ("")

#################################################################
# routines used during character generation and levelup
#################################################################
def GetMageSpells (Kit, Alignment, Level):
	MageSpells = []
	SpellType = 99
	Table = GemRB.LoadTable ("aligns")
	v = Table.FindValue (3, Alignment)
	Usability = Kit | Table.GetValue(v, 5)

	SpellsTable = GemRB.LoadTable ("spells")
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

	Table=GemRB.LoadTable("aligns")
	v = Table.FindValue(3, Alignment)
	#usability is the bitset we look for
	Usability=Table.GetValue(v, 5)

	SpellsTable = GemRB.LoadTable ("spells")
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

# there is no separate druid spell table in the originals
#FIXME: try to do this in a non-hard way?
def GetPriestSpellTable(tablename):
	if not GemRB.HasResource (tablename, RES_2DA):
		if tablename == "MXSPLDRU":
			return "MXSPLPRS"
	return tablename

def SetupSpellLevels (pc, TableName, Type, Level):
	#don't die on a missing reference
	tmp = GetPriestSpellTable(TableName)
	if tmp != TableName:
		SetupSpellLevels (pc, tmp, Type, Level)
		return

	Table = GemRB.LoadTable (TableName)
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
	tmp = GetPriestSpellTable(TableName)
	if tmp != TableName:
		UnsetupSpellLevels (pc, tmp, Type, Level)
		return

	Table = GemRB.LoadTable (TableName)
	for i in range(Table.GetColumnCount ()):
		GemRB.SetMemorizableSpellsCount (pc, 0, Type, i)
	return

# Returns -1 if not found; otherwise, the index of the spell
def HasSpell (Actor, SpellType, Level, Ref):
	# loop through each spell in the spell level and check for a matching ref
	for i in range (GemRB.GetKnownSpellsCount (Actor, SpellType, Level)):
		Spell = GemRB.GetKnownSpell(Actor, SpellType, Level, i)
		if Spell["SpellResRef"].upper() == Ref.upper(): # ensure case is the same
			return i

	# not found
	return -1

def CannotLearnSlotSpell ():
	pc = GemRB.GameGetSelectedPCSingle ()

	# disqualify sorcerors immediately
	if GemRB.GetPlayerStat (pc, IE_CLASS) == 19:
		return LSR_STAT

	if GUICommon.GameIsPST():
		import GUIINV
		slot, slot_item = GUIINV.ItemHash[GemRB.GetVar ('ItemButton')]
	else:
		slot_item = GemRB.GetSlotItem (pc, GemRB.GetVar ("ItemButton"))
	spell_ref = GemRB.GetItem (slot_item['ItemResRef'], pc)['Spell']
	spell = GemRB.GetSpell (spell_ref)

	# maybe she already knows this spell
	if HasSpell (pc, IE_SPELL_TYPE_WIZARD, spell['SpellLevel']-1, spell_ref) != -1:
		return LSR_KNOWN

	# level check (needs enough intelligence for this level of spell)
	dumbness = GemRB.GetPlayerStat (pc, IE_INT)
	if spell['SpellLevel'] > GemRB.GetAbilityBonus (IE_INT, 1, dumbness):
		return LSR_LEVEL

	return 0

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


def RemoveKnownSpells (pc, type, level1=1, level2=1, noslots=0, kit=0):
	"""Removes all known spells of a given type between two spell levels.

	If noslots is true, all memorization counts are set to 0.
	Kit is used to identify the priest spell mask of the spells to be removed;
	this is only used when removing spells in a dualclass."""

	# choose the correct limit based upon class type
	if type == IE_SPELL_TYPE_WIZARD:
		limit = 9
	elif type == IE_SPELL_TYPE_PRIEST:
		limit = 7

		# make sure that we get the original kit, if we have one
		if kit:
			originalkit = GetKitIndex (pc)

			if originalkit: # kitted; find the class value
				originalkit = CommonTables.KitList.GetValue (originalkit, 7)
			else: # just get the class value
				originalkit = GemRB.GetPlayerStat (pc, IE_CLASS)

			# this is is specifically for dual-classes and will not work to remove only one
			# spell type from a ranger/cleric multi-class
			if CommonTables.ClassSkills.GetValue (originalkit, 0, 0) != "*": # knows druid spells
				originalkit = 0x8000
			elif CommonTables.ClassSkills.GetValue (originalkit, 1, 0) != "*": # knows cleric spells
				originalkit = 0x4000
			else: # don't know any other spells
				originalkit = 0

			# don't know how this would happen, but better to be safe
			if originalkit == kit:
				originalkit = 0
	elif type == IE_SPELL_TYPE_INNATE:
		limit = 1
	else: # can't do anything if an improper spell type is sent
		return 0

	# make sure we're within parameters
	if level1 < 1 or level2 > limit or level1 > level2:
		return 0

	# remove all spells for each level
	for level in range (level1-1, level2):
		# we need the count because we remove each spell in reverse order
		count = GemRB.GetKnownSpellsCount (pc, type, level)
		mod = count-1

		for spell in range (count):
			# see if we need to check for kit
			if type == IE_SPELL_TYPE_PRIEST and kit:
				# get the spell's ref data
				ref = GemRB.GetKnownSpell (pc, type, level, mod-spell)
				ref = GemRB.GetSpell (ref['SpellResRef'], 1)

				# we have to look at the originalkit as well specifically for ranger/cleric dual-classes
				# we wouldn't want to remove all cleric spells and druid spells if we lost our cleric class
				# only the cleric ones
				if kit&ref['SpellDivine'] or (originalkit and not originalkit&ref['SpellDivine']):
					continue

			# remove the spell
			GemRB.RemoveSpell (pc, type, level, mod-spell)

		# remove memorization counts if desired
		if noslots:
			GemRB.SetMemorizableSpellsCount (pc, 0, type, level)

	# return success
	return 1

