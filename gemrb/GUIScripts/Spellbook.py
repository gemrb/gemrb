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
import GameCheck
from GUIDefines import *
from ie_stats import *
from ie_action import ACT_LEFT, ACT_RIGHT
from ie_spells import *
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
	for level in range (20): # Saradas NPC teaches you a level 14 special ...
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
				print "Error, memorized spell not found!", Spell["SpellResRef"], 1<<BookType
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

def GetKnownSpellsDescription(actor, BookType):
	""" Gets text to display in the chargen spell listing. """

	info = ""
	spells = GetKnownSpells (actor, BookType)
	# reverse spells order grouped by SpellLevel
	spells.sort (lambda fst, snd: -1 if fst['SpellLevel'] == snd['SpellLevel'] else 0)
	for spell in spells:
		info += GemRB.GetString (spell['SpellName']) + "\n"
	return info

def GetKnownSpellsLevel(actor, BookType, level):
	knownSpells = []
	spellResRefs = []

	spellCount = GemRB.GetKnownSpellsCount (actor, BookType, level)
	for i in range (spellCount):
		Spell0 = GemRB.GetKnownSpell (actor, BookType, level, i)
		if Spell0["SpellResRef"] in spellResRefs:
			continue
		spellResRefs.append (Spell0["SpellResRef"])
		Spell = GemRB.GetSpell(Spell0["SpellResRef"])
		Spell['BookType'] = BookType # just another sorting key
		knownSpells.append (Spell)

	return knownSpells

def index (list, value):
	for i in range(len(list)):
		if list[i]==value:
			return i
	return -1

def GetMemorizedSpells(actor, BookType, level):
	memoSpells = []
	spellResRefs = []

	spellCount = GemRB.GetMemorizedSpellsCount (actor, BookType, level, False)
	for i in range (spellCount):
		Spell0 = GemRB.GetMemorizedSpell (actor, BookType, level, i)
		pos = index(spellResRefs,Spell0["SpellResRef"])
		if pos!=-1:
			memoSpells[pos]['KnownCount']+=1
			memoSpells[pos]['MemoCount']+=Spell0["Flags"]
			continue

		spellResRefs.append (Spell0["SpellResRef"])
		Spell = GemRB.GetSpell(Spell0["SpellResRef"])
		Spell['KnownCount'] = 1
		Spell['MemoCount'] = Spell0["Flags"]
		memoSpells.append (Spell)

	return memoSpells

# direct access to the spellinfo struct
# SpellIndex is the index of the spell in the struct, but we add a thousandfold of the spell type for later use in SpellPressed
def GetSpellinfoSpells(actor, BookType):
	memorizedSpells = []
	spellResRefs = GemRB.GetSpelldata (actor)
	i = 0
	for resref in spellResRefs:
		Spell = GemRB.GetSpell(resref)
		Spell['BookType'] = BookType # just another sorting key
		Spell['SpellIndex'] = i + 1000 * 255 # spoofing the type, so any table would work
		Spell['MemoCount'] = 1
		memorizedSpells.append (Spell)
		i += 1

	return memorizedSpells

def SortUsableSpells(memorizedSpells):
	# sort it by using the spldisp.2da table
	layout = CommonTables.SpellDisplay.GetValue ("USE_ROW", "ROWS")
	layout = CommonTables.SpellDisplay.GetRowName (layout)
	order = CommonTables.SpellDisplay.GetValue ("DESCENDING", "ROWS")
	key1 = CommonTables.SpellDisplay.GetValue (layout, "KEY1")
	key2 = CommonTables.SpellDisplay.GetValue (layout, "KEY2")
	key3 = CommonTables.SpellDisplay.GetValue (layout, "KEY3")
	if key1:
		if key3 and key2:
			memorizedSpells = sorted(memorizedSpells, key=itemgetter(key1, key2, key3), reverse=order)
		elif key2:
			memorizedSpells = sorted(memorizedSpells, key=itemgetter(key1, key2), reverse=order)
		else:
			memorizedSpells = sorted(memorizedSpells, key=itemgetter(key1), reverse=order)

	return memorizedSpells

# Sets up all the (12) action buttons for a player character with different spell or innate icons.
# It also sets up the scroll buttons left and right if needed.
# If Start is supplied, it will skip the first few items (used when scrolling through the list)
# BookType is a spellbook type bitfield (1-mage, 2-priest, 4-innate and others in iwd2)
# Offset is a control ID offset here for iwd2 purposes
def SetupSpellIcons(Window, BookType, Start=0, Offset=0):
	actor = GemRB.GameGetFirstSelectedActor ()

	# check if we're dealing with a temporary spellbook
	if GemRB.GetVar("ActionLevel") == UAW_2DASPELLS:
		allSpells = GetSpellinfoSpells (actor, BookType)
	else:
		# construct the spellbook of usable (not depleted) memorized spells
		# the getters expect the BookType as: 0 priest, 1 mage, 2 innate
		if BookType == -1:
			# Nahal's reckless dweomer can use any known spell
			allSpells = GetKnownSpells (actor, IE_SPELL_TYPE_WIZARD)
		else:
			allSpells = []
			for i in range(16):
				if BookType & (1<<i):
					allSpells += GetUsableMemorizedSpells (actor, i)
			if not len(allSpells):
				raise AttributeError ("Error, unknown BookType passed to SetupSpellIcons: %d! Bailing out!" %(BookType))

	if BookType == -1:
		memorizedSpells = allSpells
		# reset Type, so we can choose the surge spell instead of just getting a redraw of the action bar
		GemRB.SetVar("Type", 3)
	else:
		memorizedSpells = SortUsableSpells(allSpells)

	# start creating the controls
	import GUICommonWindows
	# TODO: ASCOL, ROWS
	#AsCol = CommonTables.SpellDisplay.GetValue (layout, "AS_COL")
	#Rows = CommonTables.SpellDisplay.GetValue (layout, "ROWS")
	More = len(memorizedSpells) > 12 or Start > 0

	# scroll left button
	if More:
		Button = Window.GetControl (Offset)
		Button.SetText ("")
		if Start:
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
	buttonCount = 12 - More * 2 # GUIBT_COUNT in PCStatsStruct
	for i in range (buttonCount):
		Button = Window.GetControl (i+Offset+More)
		Button.SetEvent (IE_GUI_BUTTON_ON_RIGHT_PRESS, None)

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
		specialSpell = (specialSpell & SP_IDENTIFY) or ((specialSpell & SP_SURGE) and actionLevel == UAW_ALLMAGE)
		if specialSpell & SP_SILENCE and Spell['HeaderFlags'] & 0x20000: # SF_IGNORES_SILENCE
			specialSpell ^= SP_SILENCE
		if specialSpell or (disabled_spellcasting&spellType):
			Button.SetState (IE_GUI_BUTTON_DISABLED)
			Button.EnableBorder(1, 0)
		else:
			Button.SetState (IE_GUI_BUTTON_UNPRESSED)
			Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, GUICommonWindows.SpellPressed)
			Button.SetEvent (IE_GUI_BUTTON_ON_SHIFT_PRESS, GUICommonWindows.SpellShiftPressed)

		if Spell['SpellResRef']:
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
		Button = Window.GetControl (Offset+buttonCount+1)
		GUICommonWindows.SetActionIconWorkaround (Button, ACT_RIGHT, buttonCount)
		Button.SetText ("")
		if len(memorizedSpells) - Start > 10:
			Button.SetState (IE_GUI_BUTTON_UNPRESSED)
		else:
			Button.SetState (IE_GUI_BUTTON_DISABLED)
			Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_SET)
			Button.SetTooltip ("")

#################################################################
# routines used during character generation and levelup
#################################################################

# Used for bards (level-up), sorcerers and mages (cg and level-up).
# Used for rangers and paladins (level-up) and clerics and druids (both).
# While iwd2 still has spells.2da, it is actually unused and gives
# some wrong spells. We need to iterate the whole listspll.2da,
# looking at our class column and matching on (target) level
def GetIWD2Spells (kit, usability, level, baseClass = -1):
	spells = []
	if baseClass == -1:
		print "Error, didn't pass a base class in iwd2!"
		return spells
	# to use baseClass as a column index, we first need to convert it,
	# since only casters are in the table. But only casters have spell stats too
	rowName = CommonTables.Classes.GetRowName (baseClass-1)
	baseClass = CommonTables.ClassSkills.GetValue (rowName, "SPLTYPE", GTV_INT)

	# iwd2 has only per-kit exclusion, spells can't override it
	badSchools = 0
	if rowName == "WIZARD":
		exclusionTable = GemRB.LoadTable ("magesch")
		kitRow = exclusionTable.FindValue ("KIT", kit)
		kitRow = exclusionTable.GetRowName (kitRow)
		badSchools = exclusionTable.GetValue (kitRow, "EXCLUSION", GTV_INT)
		if badSchools == -1:
			badSchools = 0
	else:
		# only wizards have alignment and school restrictions
		kit = usability = 0

	spellsTable = GemRB.LoadTable ("listspll")
	spellCount = spellsTable.GetRowCount ()
	for i in range(spellCount):
		# at which level is the spell given to the actor?
		atLevel = spellsTable.GetValue (i, baseClass)
		if atLevel != level:
			continue

		spellRow = spellsTable.GetRowName (i)
		spellName = spellsTable.GetValue (spellRow, "SPELL_RES_REF")
		ms = GemRB.GetSpell (spellName, 1)
		if ms == None:
			continue
		# ms['SpellDivine'] is unused in iwd2, since it has separate types for all caster classes

		if usability & ms['SpellExclusion']:
			spellType = 0
		elif badSchools & (1<<ms['SpellSchool']+5):
			spellType = 0
		else:
			spellType = 1
			if kit & (1 << ms['SpellSchool']+5): # of matching specialist school
				spellType = 2

		spells.append([spellName, spellType])

	return spells

def GetMageSpells (Kit, Alignment, Level, baseClass = -1):
	MageSpells = []
	SpellType = 99
	v = CommonTables.Aligns.FindValue (3, Alignment)
	Usability = Kit | CommonTables.Aligns.GetValue(v, 5)
	WildMages = True

	if GameCheck.IsIWD2():
		return GetIWD2Spells (Kit, Usability, Level, baseClass)

	SpellsTable = GemRB.LoadTable ("spells")
	for i in range(SpellsTable.GetValue ("MAGE", str(Level), GTV_INT)):
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
			if WildMages and Kit == 0x8000 and (0x4000 & ms['SpellExclusion']):
				SpellType = 2
		MageSpells.append ([SpellName, SpellType])

	return MageSpells

def GetLearnableMageSpells (Kit, Alignment, Level):
	Learnable = []

	for Spell in GetMageSpells (Kit, Alignment, Level):
		if Spell[1]:
			Learnable.append (Spell[0])
	return Learnable

def GetLearnableDomainSpells (pc, Level, baseClassName = -1):
	import GUICommon
	import GUICommonWindows
	Learnable =[]

	# only clerics have domains due to listdom.2da restrictions
	# no need to double check, as we only call this for IE_IWD2_SPELL_CLERIC
	if baseClassName == -1:
		baseClassName = GUICommon.GetClassRowName (pc)
	BaseClassIndex = CommonTables.Classes.GetRowIndex (baseClassName)
	# columns correspond to kits in the same order
	KitIndex = GUICommonWindows.GetKitIndex (pc, BaseClassIndex)
	if KitIndex == -1:
		print "GetLearnableDomainSpells: couldn't determine the kit, bailing out!"
		return Learnable
	# calculate the offset from the first cleric kit
	KitIndex -= CommonTables.Classes.FindValue ("CLASS", BaseClassIndex+1)

	DomainSpellTable = GemRB.LoadTable ("listdomn")
	# check everything in case someone wants to mod the spell amount
	for i in range(DomainSpellTable.GetRowCount ()):
		if DomainSpellTable.GetValue (i, KitIndex) == Level:
			SpellName = DomainSpellTable.GetRowName (i)
			SpellName = DomainSpellTable.GetValue (SpellName, "DOMAIN_RESREF")
			Learnable.append (SpellName)

	return Learnable

def GetLearnablePriestSpells (Class, Alignment, Level, booktype=0):
	Learnable =[]

	v = CommonTables.Aligns.FindValue(3, Alignment)
	#usability is the bitset we look for
	Usability = CommonTables.Aligns.GetValue(v, 5)
	SpellListTable = None
	if GameCheck.IsIWD2():
		row = CommonTables.ClassSkills.FindValue ("SPLTYPE", booktype)
		rowName = CommonTables.ClassSkills.GetRowName (row)
		Class = CommonTables.Classes.GetValue (rowName, "ID", GTV_INT)
		spells = GetIWD2Spells (0, Usability, Level, Class)
		spells = map(lambda e: e[0], spells) # ignore the second member
		return spells

	SpellsTable = GemRB.LoadTable ("spells")
	for i in range(SpellsTable.GetValue ("PRIEST", str (Level), GTV_INT)):
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
# however Tweaks Anthology adds it for all other games and EE does the same
# so we can't just change the value in the tables and be done with it
def GetPriestSpellTable(tablename):
	if GameCheck.IsIWD2():
		return tablename # no need for this folly

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
	kit = GemRB.GetPlayerStat (pc, IE_KIT)
	for i in range(Table.GetColumnCount ()):
		# do a string lookup since some tables don't have entries for all levels
		value = Table.GetValue (str(Level), str(i+1), GTV_INT)
		# specialist mages get an extra spell if they already know that level
		# FIXME: get a general routine to find specialists
		school = GemRB.GetVar("MAGESCHOOL")
		if (Type == IE_SPELL_TYPE_WIZARD and school != 0) or \
			(GameCheck.IsIWD2() and Type == IE_IWD2_SPELL_WIZARD and not (kit&0x4000)):
			if value > 0:
				value += 1
		elif Type == IE_IWD2_SPELL_DOMAIN:
			if value > 0:
				value = 1 # since we're reusing the main cleric table
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

# is this a sorcerer-style learning/memo spellbook?
def IsSorcererBook (bookmode):
	return bookmode & 2

def HasSorcererBook (pc, cls=-1):
	import GUICommon

	ClassName = GUICommon.GetClassRowName (pc)
	if cls != -1:
		ClassName = GUICommon.GetClassRowName (cls, "class")
	SorcererBook = CommonTables.ClassSkills.GetValue (ClassName, "BOOKTYPE")
	if SorcererBook == "*":
		return False
	return IsSorcererBook (SorcererBook)

def CannotLearnSlotSpell ():
	pc = GemRB.GameGetSelectedPCSingle ()

	# disqualify sorcerers immediately
	if HasSorcererBook (pc):
		return LSR_STAT

	booktype = IE_SPELL_TYPE_WIZARD
	if GameCheck.IsIWD2():
		booktype = IE_IWD2_SPELL_WIZARD

	if GameCheck.IsPST():
		import GUIINV
		slot, slot_item = GUIINV.ItemHash[GemRB.GetVar ('ItemButton')]
	else:
		slot_item = GemRB.GetSlotItem (pc, GemRB.GetVar ("ItemButton"))
	spell_ref = GemRB.GetItem (slot_item['ItemResRef'], pc)['Spell']
	spell = GemRB.GetSpell (spell_ref)
	level = spell['SpellLevel']

	# school conflicts are handled before this is called from inventory
	# add them here if a need arises

	# maybe she already knows this spell
	if HasSpell (pc, booktype, level-1, spell_ref) != -1:
		return LSR_KNOWN

	# level check (needs enough intelligence for this level of spell)
	dumbness = GemRB.GetPlayerStat (pc, IE_INT)
	if level > GemRB.GetAbilityBonus (IE_INT, 1, dumbness):
		return LSR_LEVEL

	spell_count = GemRB.GetKnownSpellsCount (pc, booktype, level-1)
	if spell_count >= GemRB.GetAbilityBonus (IE_INT, 2, dumbness):
		return LSR_FULL

	return 0

def LearnPriestSpells (pc, level, mask, baseClassName = -1):
	"""Learns all the priest spells through the given spell level.

	Mask distinguishes clerical and druidic spells."""

	# make sure we don't have too high a level
	booktype = IE_SPELL_TYPE_PRIEST
	if GameCheck.IsIWD2():
		level = min(9, level)
		booktype = mask
		mask = 0 # no classflags restrictions like in others (differentiating cleric/rangers)
	else:
		level = min(7, level)

	# go through each level
	alignment = GemRB.GetPlayerStat (pc, IE_ALIGNMENT)
	for i in range (level):
		if booktype == IE_IWD2_SPELL_DOMAIN:
			learnable = GetLearnableDomainSpells (pc, i+1, baseClassName)
		else:
			learnable = GetLearnablePriestSpells (mask, alignment, i+1, booktype)

		for spell in learnable:
			# if the spell isn't learned, learn it
			if HasSpell (pc, booktype, i, spell) < 0:
				if GameCheck.IsIWD2():
					if booktype == IE_IWD2_SPELL_DOMAIN:
						GemRB.LearnSpell (pc, spell, 0, 1<<booktype, i)
					else:
						# perhaps forcing would be fine here too, but it's untested and
						# iwd2 cleric schools grant certain spells at different levels
						GemRB.LearnSpell (pc, spell, 0, 1<<booktype)
				else:
					GemRB.LearnSpell (pc, spell)
	return


def RemoveKnownSpells (pc, type, level1=1, level2=1, noslots=0, kit=0):
	"""Removes all known spells of a given type between two spell levels.

	If noslots is true, all memorization counts are set to 0.
	Kit is used to identify the priest spell mask of the spells to be removed;
	this is only used when removing spells in a dualclass."""

	# choose the correct limit based upon class type
	if type == IE_SPELL_TYPE_WIZARD or GameCheck.IsIWD2():
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
			originalkit = GUICommon.GetClassRowName (originalkit, "class")

			# this is is specifically for dual-classes and will not work to remove only one
			# spell type from a ranger/cleric multi-class
			if CommonTables.ClassSkills.GetValue (originalkit, "DRUIDSPELL", GTV_STR) != "*": # knows druid spells
				originalkit = 0x8000
			elif CommonTables.ClassSkills.GetValue (originalkit, "CLERICSPELL", GTV_STR) != "*": # knows cleric spells
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

	if GameCheck.IsIWD2():
		kit = 0 # just skip the dualclass logic

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

# learning/memorization wrapper for when you want to give more than 1 instance
# learn a spell if we don't know it yet, otherwise just increase the memo count
def LearnSpell(pc, spellref, booktype, level, count, flags=0):
	SpellIndex = HasSpell (pc, booktype, level, spellref)
	if SpellIndex < 0:
		ret = GemRB.LearnSpell (pc, spellref, flags, booktype)
		if ret != LSR_OK and ret != LSR_KNOWN:
			raise RuntimeError, "Failed learning spell: %s !" %(spellref)

		SpellIndex = HasSpell (pc, booktype, level, spellref)
		count -= 1

	if count <= 0:
		return

	if SpellIndex == -1:
		# should never happen
		raise RuntimeError, "LearnSpell: Severe spellbook problems: %s !" %(spellref)

	for j in range(count):
		GemRB.MemorizeSpell (pc, booktype, level, SpellIndex, flags&LS_MEMO)

def LearnFromScroll (pc, slot):
	slot_item = GemRB.GetSlotItem (pc, slot)
	spell_ref = GemRB.GetItem (slot_item['ItemResRef'], pc)['Spell']

	ret = GemRB.LearnSpell (pc, spell_ref, LS_STATS|LS_ADDXP)

	# destroy the scroll, just one in case of a stack
	GemRB.RemoveItem (pc, slot, 1)

	return ret
