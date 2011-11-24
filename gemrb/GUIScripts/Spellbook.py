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
from ie_stats import IE_CASTING
from ie_action import ACT_LEFT, ACT_RIGHT
from ie_spells import SP_IDENTIFY, SP_SURGE

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
			Spell['SpellIndex'] = -1 # this gets assigned properly later
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

# Start is used as an offset in the spell list to show "pages" > 1
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
		if BookType == 3:
			allSpells = GetUsableMemorizedSpells (actor, IE_SPELL_TYPE_PRIEST) + GetUsableMemorizedSpells (actor, IE_SPELL_TYPE_WIZARD)
		elif BookType == 4:
			allSpells = GetUsableMemorizedSpells (actor, IE_SPELL_TYPE_INNATE)
		else:
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
			Button.SetVarAssoc ("Spell", i+Start)
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
