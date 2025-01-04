# GemRB - Infinity Engine Emulator
# Copyright (C) 2024 The GemRB Project
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

# ActionsWindow.py - functions to set up the main action bar (see UpdateActionsWindow)

import GemRB
import GameCheck
import GUICommon
import CommonTables
if not GameCheck.IsPST ():
  import Spellbook
from Clock import UpdateClock, CreateClockButton
from GUIDefines import *
from ie_action import *
from ie_modal import *
from ie_slots import SLOT_QUIVER
from ie_sounds import CHAN_HITS
from ie_stats import *

CurrentWindow = None
ActionBarControlOffset = 0
if GameCheck.IsIWD2 ():
	ActionBarControlOffset = 6 # portrait and action window were merged
fistDrawn = True

# The following four functions are currently unused in pst
def EmptyControls ():
	if GameCheck.IsPST():
		return
	Selected = GemRB.GetSelectedSize()
	if Selected==1:
		pc = GemRB.GameGetFirstSelectedActor ()
		# init spell list
		GemRB.SpellCast (pc, -1, 0)

	GemRB.SetVar ("ActionLevel", UAW_STANDARD)
	if not CurrentWindow:
		return # current case in our game demo (on right-click)
	for i in range (12):
		Button = CurrentWindow.GetControl (i+ActionBarControlOffset)
		Button.SetVisible (False)
	return

def SelectFormation (btn):
	GemRB.GameSetFormation (btn.Value)
	return

def SelectFormationPreset ():
	"""Choose the default formation."""

	GemRB.GameSetFormation (GemRB.GetVar ("Value"), GemRB.GetVar ("Formation")) # save
	GemRB.GameSetFormation (GemRB.GetVar ("Formation")) # set
	GroupControls ()
	return

def SetupFormation ():
	"""Opens the formation selection section."""

	for i in range (12):
		Button = CurrentWindow.GetControl (i+ActionBarControlOffset)
		Button.SetFlags (IE_GUI_BUTTON_NORMAL, OP_SET)
		Button.SetSprites ("GUIBTBUT", 0, 0, 1, 2, 3)
		Button.SetBAM ("FORM%x"%i, 0, 0, -1)
		Button.SetVarAssoc ("Value", i)
		Button.OnPress (SelectFormationPreset)
		Button.SetState (IE_GUI_BUTTON_UNPRESSED)
	return

def GroupControls ():
	"""Sections that control group actions."""

	GemRB.SetVar ("ActionLevel", UAW_STANDARD)
	Button = CurrentWindow.GetControl (ActionBarControlOffset)
	if GameCheck.IsBG2OrEE ():
		Button.SetActionIcon (globals(), 7, 1) # talk icon
	else:
		Button.SetActionIcon (globals(), 14, 1)# guard icon
	Button = CurrentWindow.GetControl (1 + ActionBarControlOffset)
	Button.SetActionIcon (globals(), 15, 2)
	Button = CurrentWindow.GetControl (2 + ActionBarControlOffset)
	Button.SetActionIcon (globals(), 21, 3)
	Button = CurrentWindow.GetControl (3 + ActionBarControlOffset)
	Button.SetActionIcon (globals(), -1, 4)
	Button = CurrentWindow.GetControl (4 + ActionBarControlOffset)
	Button.SetActionIcon (globals(), -1, 5)
	Button = CurrentWindow.GetControl (5 + ActionBarControlOffset)
	Button.SetActionIcon (globals(), -1, 6)
	Button = CurrentWindow.GetControl (6+ActionBarControlOffset)
	Button.SetActionIcon (globals(), -1, 7)

	formation = GemRB.GameGetFormation () # formation index
	GemRB.SetVar ("Formation", formation)
	for i in range (5):
		Button = CurrentWindow.GetControl (7 + ActionBarControlOffset + i)
		idx = GemRB.GameGetFormation (i)
		Button.SetFlags (IE_GUI_BUTTON_RADIOBUTTON | IE_GUI_BUTTON_NORMAL, OP_SET)
		# kill the previous sprites or they show through
		Button.SetSprites ("GUIBTBUT", 0, 0, 1, 2, 3)
		Button.SetBAM ("FORM%x" % idx, 0, 0, -1)
		Button.SetVarAssoc ("Formation", i)
		Button.OnPress (SelectFormation)
		Button.OnRightPress (SetupFormation)
		Button.SetTooltip (4935)
		# 0x90 = F1 key
		Button.SetHotKey (chr(7 + i + 0x90), 0, True)

	# work around radiobutton preselection issue
	for i in range (5):
		Button = CurrentWindow.GetControl (7 + ActionBarControlOffset + i)
		if i == formation:
			Button.SetState (IE_GUI_BUTTON_SELECTED)
		else:
			Button.SetState (IE_GUI_BUTTON_ENABLED)

	return

def OpenActionsWindowControls (Window):
	CreateClockButton(Window.GetControl (62))
	Window.AddAlias ("ACTWIN") # needs to happen before the first update or it will be a noop
	UpdateActionsWindow ()
	return

## not used in pst - not sure any items have abilities, but it is worth making a note to find out
def SelectItemAbility():
	pc = GemRB.GameGetFirstSelectedActor ()
	slot = GemRB.GetVar ("Slot")
	ability = GemRB.GetVar ("Ability")
	GemRB.SetupQuickSlot (pc, 0, slot, ability)
	GemRB.SetVar ("ActionLevel", UAW_STANDARD)
	return

# in pst only nordom has bolts and they show on the same floatmenu as quickweapons, so needs work here
def SelectQuiverSlot():
	pc = GemRB.GameGetFirstSelectedActor ()
	slot = GemRB.GetVar ("Slot")
	slot_item = GemRB.GetSlotItem (pc, slot)
	# HACK: implement SetEquippedAmmunition instead?
	if not GemRB.IsDraggingItem ():
		item = GemRB.GetItem (slot_item["ItemResRef"])
		GemRB.DragItem (pc, slot, item["ItemIcon"]) #, 0, 0)
		GemRB.DropDraggedItem (pc, slot)
	GemRB.SetVar ("ActionLevel", UAW_STANDARD)
	return

# this doubles up as an ammo selector (not yet used in pst)
def SetupItemAbilities(pc, slot, only):
	slot_item = GemRB.GetSlotItem(pc, slot)
	if not slot_item:
		# CHIV: Could configure empty quickslots from the game screen ala spells heres
		return

	item = GemRB.GetItem (slot_item["ItemResRef"])
	Tips = item["Tooltips"]
	Locations = item["Locations"]

	# clear buttons here
	EmptyControls()

	# check A: whether ranged weapon and B: whether to bother at all
	ammotype = 0
	if item["Type"] == CommonTables.ItemType.GetRowIndex ("BOW"):
		ammotype = CommonTables.ItemType.GetRowIndex ("ARROW")
	elif item["Type"] == CommonTables.ItemType.GetRowIndex ("XBOW"):
		ammotype = CommonTables.ItemType.GetRowIndex ("BOLT")
	elif item["Type"] == CommonTables.ItemType.GetRowIndex ("SLING"):
		ammotype = CommonTables.ItemType.GetRowIndex ("BULLET")

	ammoSlotCount = 0
	if ammotype:
		ammoslots = GemRB.GetSlots(pc, SLOT_QUIVER, 1)
		currentammo = GemRB.GetEquippedAmmunition (pc)
		currentbutton = None
		for i in range (12):
			Button = CurrentWindow.GetControl (i + ActionBarControlOffset)
			if i < len(ammoslots):
				ammoslot = GemRB.GetSlotItem (pc, ammoslots[i])
				st = GemRB.GetSlotType (ammoslots[i])
				ammoitem = GemRB.GetItem (ammoslot['ItemResRef']) # needed to show the ammo count
				Tips = ammoitem["Tooltips"]
				# if this item is valid ammo and was really found in a quiver slot
				if ammoitem['Type'] == ammotype and st["Type"] == SLOT_QUIVER:
					ammoSlotCount += 1
					Button.SetFlags (IE_GUI_BUTTON_RADIOBUTTON | IE_GUI_BUTTON_ALIGN_BOTTOM | IE_GUI_BUTTON_ALIGN_RIGHT, OP_SET)
					Button.SetSprites ("GUIBTBUT", 0, 0, 1, 3, 5)
					Button.SetItemIcon (ammoslot['ItemResRef'])
					Button.SetText (str(ammoslot["Usages0"]))
					Button.OnPress (SelectQuiverSlot)
					Button.OnRightPress (SelectQuiverSlot)
					Button.SetVarAssoc ("Slot", ammoslots[i])
					if Tips[0] != -1:
						Button.SetTooltip (Tips[0])
					if currentammo == ammoslots[i]:
						currentbutton = Button

		if currentbutton:
			currentbutton.SetState (IE_GUI_BUTTON_SELECTED)

	# skip when there is only one choice
	if ammoSlotCount == 1:
		ammoSlotCount = 0

	# reset back to the main action bar if there are no extra headers or quivers
	reset = not ammoSlotCount

	# check for item abilities and skip irrelevant headers
	# for quick weapons only show weapon headers
	# for quick items only show the opposite
	# for scrolls only show the first (second header is for learning)
	# So depending on the context Staff of Magi will have none or 2
	if item["Type"] == CommonTables.ItemType.GetRowIndex ("SCROLL"):
		Tips = ()

	# skip launchers - handled above
	# gesen bow (bg2:bow19) has just a projectile header (not bow) for its special attack
	# TODO: we should append it to the list of ammo as a usability improvement
	if only == UAW_QWEAPONS and ammoSlotCount > 1:
		Tips = ()

	if len(Tips) > 0:
		reset = False
		rmax = min(len(Tips), 12-ammoSlotCount)

		# for mixed items, only show headers if there is more than one appropriate one
		weaps = sum([i == ITEM_LOC_WEAPON for i in Locations])
		if only == UAW_QWEAPONS and weaps == 1 and ammoSlotCount <= 1:
			rmax = 0
			reset = True
		abils = sum([i == ITEM_LOC_EQUIPMENT for i in Locations])
		if only == UAW_QITEMS and abils == 1:
			rmax = 0
			reset = True

		for i in range (rmax):
			if only == UAW_QITEMS:
				if Locations[i] != ITEM_LOC_EQUIPMENT:
					continue
			elif only == UAW_QWEAPONS:
				if Locations[i] != ITEM_LOC_WEAPON:
					continue
			Button = CurrentWindow.GetControl (i + ActionBarControlOffset + ammoSlotCount)
			Button.SetFlags (IE_GUI_BUTTON_RADIOBUTTON | IE_GUI_BUTTON_NORMAL, OP_SET)
			Button.SetSprites ("GUIBTBUT", 0, 0, 1, 2, 5)
			Button.SetItemIcon (slot_item['ItemResRef'], i + 6)
			Button.SetText ("")
			Button.OnPress (SelectItemAbility)
			Button.OnRightPress (SelectItemAbility)
			Button.SetVarAssoc ("Ability", i)
			Button.SetState (IE_GUI_BUTTON_ENABLED)
			if Tips[i] != -1:
				Button.SetTooltip ("F%d - %s" %(i, GemRB.GetString (Tips[i])))

	if reset:
		GemRB.SetVar ("ActionLevel", UAW_STANDARD)
		UpdateActionsWindow ()
	return

# iwd2 spell book/class selection
def SetupBookSelection ():
	pc = GemRB.GameGetFirstSelectedActor ()

	# get all the books that still have non-depleted memorisations
	# we need this list, so we can avoid holes in the action bar
	usableBooks = []
	for i in range (IE_IWD2_SPELL_SONG):
		bookClass = i
		if i == IE_IWD2_SPELL_INNATE: # shape stat comes later (8 vs 10)
			bookClass = IE_IWD2_SPELL_SHAPE

		enabled = False
		if i <= (IE_IWD2_SPELL_DOMAIN + 1): # booktypes up to and including domain + shapes
			spellCount = len(Spellbook.GetUsableMemorizedSpells (pc, bookClass))
			enabled = spellCount > 0
		if enabled:
			usableBooks.append(i)

	# if we have only one or only cleric+domain, skip to the spells
	bookCount = len(usableBooks)
	if bookCount == 1 or (bookCount == 2 and IE_IWD2_SPELL_CLERIC in usableBooks):
		GemRB.SetVar ("ActionLevel", UAW_SPELLS_DIRECT)
		UpdateActionsWindow ()
		return

	for i in range (12):
		Button = CurrentWindow.GetControl (i + ActionBarControlOffset)
		if i >= len(usableBooks):
			Button.SetActionIcon (globals(), -1)
			continue
		Button.SetActionIcon (globals(), 40 + usableBooks[i])
	return

# you can change this for custom skills, this is the original engine
skillbar = (ACT_STEALTH, ACT_SEARCH, ACT_THIEVING, ACT_WILDERNESS, ACT_TAMING, ACT_USE, ACT_CAST, 100, 100, 100, 100, 100)
def SetupSkillSelection ():
	pc = GemRB.GameGetFirstSelectedActor ()
	SetupControls (CurrentWindow, pc, ActionBarControlOffset, skillbar)
	return

def UpdateActionsWindow ():
	"""Redraws the actions section of the window."""

	global CurrentWindow
	global level, TopIndex

	# don't do anything if the GameControl is temporarily unavailable
	try:
		GUICommon.GameControl.GetFrame()
	except RuntimeError:
		return

	PortraitWin = GemRB.GetView("PORTWIN")
	ActionsWindow = GemRB.GetView("ACTWIN")

	if not GameCheck.IsIWD2():
		CurrentWindow = ActionsWindow
		ActionBarControlOffset = 0
		UpdateClock ()
	else:
		CurrentWindow = PortraitWin
		ActionBarControlOffset = 6 # set it here too, since we get called before menu setup

	if GameCheck.IsPST():
		return

	if CurrentWindow == None:
		return

	Selected = GemRB.GetSelectedSize()

	# setting up the disabled button overlay (using the second border slot)
	for i in range (12):
		Button = CurrentWindow.GetControl (i + ActionBarControlOffset)
		if GameCheck.IsBG1():
			color = {'r' : 0, 'g' : 254, 'b' :0, 'a' : 255}
			Button.SetBorder (0, color, 0, 0, Button.GetInsetFrame(6, 6, 4, 4))

		color = {'r' : 50, 'g' : 30, 'b' :10, 'a' : 120}
		Button.SetBorder (1, color, 0, 1)
		if GameCheck.IsBG2EE ():
			# for some reason NUMBER doesn't display
			Button.SetFont ("NUMBER2")
		else:
			Button.SetFont ("NUMBER")
		Button.SetText ("")
		Button.SetTooltip("")
		Button.SetDisabled (False)

	if Selected == 0:
		EmptyControls ()
		return
	if Selected > 1:
		GroupControls ()
		return

	# we are sure there is only one actor selected
	pc = GemRB.GameGetFirstSelectedActor ()

	# all but bg2 summons all use the group actionbar!
	# bg2 has a different common format and exceptions for illusions/clones
	level = GemRB.GetVar ("ActionLevel")
	if GemRB.GetPlayerStat (pc, IE_EA) >= CONTROLLED:
		if GameCheck.IsBG2OrEE ():
			if level == UAW_STANDARD and GemRB.GetPlayerStat (pc, IE_SEX) != 7:
				level = UAW_BG2SUMMONS
		else:
			GroupControls ()
			return

	TopIndex = GemRB.GetVar ("TopIndex")
	if level == UAW_STANDARD:
		# this is based on class
		# cleric/thief wants 13 icons, so we push the extra under innates (original did picking, we do turning)
		SetupControls (CurrentWindow, pc, ActionBarControlOffset)
	elif level == UAW_EQUIPMENT:
		CurrentWindow.SetupEquipmentIcons(globals(), pc, TopIndex, ActionBarControlOffset)
	elif level == UAW_SPELLS or level == UAW_SPELLS_DIRECT: # spells
		if GameCheck.IsIWD2():
			if level == UAW_SPELLS:
				# set up book selection if appropriate
				SetupBookSelection ()
				return
			# otherwise just throw everything in a single list
			# everything but innates, songs and shapes
			spelltype = (1 << IE_IWD2_SPELL_INNATE) - 1
		else:
			spelltype = (1 << IE_SPELL_TYPE_PRIEST) + (1 << IE_SPELL_TYPE_WIZARD)
		GemRB.SetVar ("Type", spelltype)
		Spellbook.SetupSpellIcons(CurrentWindow, spelltype, TopIndex, ActionBarControlOffset)
	elif level == UAW_INNATES: # innates
		if GameCheck.IsIWD2():
			spelltype = (1 << IE_IWD2_SPELL_INNATE) + (1 << IE_IWD2_SPELL_SHAPE)
		else:
			spelltype = 1 << IE_SPELL_TYPE_INNATE
		GemRB.SetVar ("Type", spelltype)
		Spellbook.SetupSpellIcons(CurrentWindow, spelltype, TopIndex, ActionBarControlOffset)
	elif level == UAW_QWEAPONS or level == UAW_QITEMS: # quick weapon or quick item ability selection
		SetupItemAbilities(pc, GemRB.GetVar("Slot"), level)
	elif level == UAW_BG2SUMMONS:
			SetupControls (CurrentWindow, pc, ActionBarControlOffset)
			# use group attack icon and disable second weapon slot
			Button = CurrentWindow.GetControl (1 + ActionBarControlOffset)
			Button.SetActionIcon (globals(), 15, 2)
			Button = CurrentWindow.GetControl (2 + ActionBarControlOffset)
			Button.SetActionIcon (globals(), -1, 0)
			# the games showed quick spells, but these actors don't have the data for it
			# we just disable the slots instead
			Button = CurrentWindow.GetControl (3 + ActionBarControlOffset)
			Button.SetActionIcon (globals(), 3, 4)
			Button.SetDisabled (True)
			Button = CurrentWindow.GetControl (4 + ActionBarControlOffset)
			Button.SetActionIcon (globals(), 4, 5)
			Button.SetDisabled (True)
			Button = CurrentWindow.GetControl (5 + ActionBarControlOffset)
			Button.SetActionIcon (globals(), 5, 6)
			Button.SetDisabled (True)
	elif level == UAW_ALLMAGE: # all known mage spells
		GemRB.SetVar ("Type", -1)
		Spellbook.SetupSpellIcons(CurrentWindow, -1, TopIndex, ActionBarControlOffset)
	elif level == UAW_SKILLS: # iwd2 skills
		SetupSkillSelection()
	elif level == UAW_QSPELLS: # quickspells, but with innates too
		if GameCheck.IsIWD2():
			spelltype = (1 << IE_IWD2_SPELL_INNATE) - 1
			spelltype += (1 << IE_IWD2_SPELL_INNATE) + (1 << IE_IWD2_SPELL_SHAPE)
		else:
			spelltype = (1 << IE_SPELL_TYPE_PRIEST) + (1 << IE_SPELL_TYPE_WIZARD) + (1 << IE_SPELL_TYPE_INNATE)
		GemRB.SetVar ("Type", spelltype)
		Spellbook.SetupSpellIcons(CurrentWindow, spelltype, TopIndex, ActionBarControlOffset)
	elif level == UAW_QSHAPES: # shapes selection
		spelltype = 1 << IE_IWD2_SPELL_SHAPE
		GemRB.SetVar ("Type", spelltype)
		Spellbook.SetupSpellIcons(CurrentWindow, spelltype, TopIndex, ActionBarControlOffset)
	elif level == UAW_QSONGS: # songs selection
		spelltype = 1 << IE_IWD2_SPELL_SONG
		if GameCheck.IsIWD1():
			spelltype = 1 << IE_SPELL_TYPE_SONG
		GemRB.SetVar ("Type", spelltype)
		Spellbook.SetupSpellIcons(CurrentWindow, spelltype, TopIndex, ActionBarControlOffset)
	elif level == UAW_BOOK: # spellbook selection
		spelltype = GemRB.GetVar ("Type")
		Spellbook.SetupSpellIcons(CurrentWindow, spelltype, TopIndex, ActionBarControlOffset)
	elif level == UAW_2DASPELLS: # spells from a 2da (fx_select_spell)
		if GameCheck.IsIWD2():
			# everything but innates, songs and shapes
			spelltype = (1 << IE_IWD2_SPELL_INNATE) - 1
		else:
			spelltype = (1 << IE_SPELL_TYPE_PRIEST) + (1 << IE_SPELL_TYPE_WIZARD)
		GemRB.SetVar ("Type", spelltype)
		Spellbook.SetupSpellIcons (CurrentWindow, spelltype, TopIndex, ActionBarControlOffset)
	else:
		print("Invalid action level:", level)
		GemRB.SetVar ("ActionLevel", UAW_STANDARD)
		UpdateActionsWindow ()
	return

def ActionQWeaponPressed (which):
	"""Selects the given quickslot weapon if possible."""

	pc = GemRB.GameGetFirstSelectedActor ()
	qs = GemRB.GetEquippedQuickSlot (pc, 1)

	# 38 is the magic slot
	if ((qs==which) or (qs==38)) and GemRB.GameControlGetTargetMode() != TARGET_MODE_ATTACK:
		GemRB.GameControlSetTargetMode (TARGET_MODE_ATTACK, GA_NO_DEAD | GA_NO_SELF | GA_NO_HIDDEN)
	else:
		GemRB.GameControlSetTargetMode (TARGET_MODE_NONE)
		GemRB.SetEquippedQuickSlot (pc, which, -1)

	SetupControls (CurrentWindow, pc, ActionBarControlOffset)
	UpdateActionsWindow ()
	return

# TODO: implement full weapon set switching instead
def ActionQWeaponRightPressed (action):
	"""Selects the used ability of the quick weapon."""
	GemRB.SetVar ("Slot", action)
	GemRB.SetVar ("ActionLevel", UAW_QWEAPONS)
	UpdateActionsWindow ()
	return

###############################################
# quick icons for spells, innates, songs, shapes

def ActionQSpellPressed (which):
	pc = GemRB.GameGetFirstSelectedActor ()

	GemRB.SpellCast (pc, -2, which)
	UpdateActionsWindow ()
	return

def ActionQSpellRightPressed (which):
	GemRB.SetVar ("QSpell", which)
	GemRB.SetVar ("TopIndex", 0)
	GemRB.SetVar ("ActionLevel", UAW_QSPELLS)
	UpdateActionsWindow ()
	return

suf = ["", "Right"]
# this function is used to generate various action bar actions
# that should be available in 9 slots (iwd2)
def GenerateButtonActions(num, name, g, right = 0, offset = 0):
	dec = "def Action" + name + str(num + 1) + suf[right] + "Pressed():\n"
	dec += "\tAction" + name + suf[right] + "Pressed(" + str(num + offset) + ")"
	exec(dec, g) # pass on the same global dict, so we remain in the top scope

for i in range(9):
	GenerateButtonActions(i, "QSpec", globals(), 0)
	GenerateButtonActions(i, "QSpec", globals(), 1)
	GenerateButtonActions(i, "QSpell", globals(), 0)
	GenerateButtonActions(i, "QSpell", globals(), 1)
	GenerateButtonActions(i, "QShape", globals(), 0)
	GenerateButtonActions(i, "QShape", globals(), 1)
	GenerateButtonActions(i, "QSong", globals(), 0)
	GenerateButtonActions(i, "QSong", globals(), 1)
for i in range(4):
	GenerateButtonActions(i, "QWeapon", globals(), 0)
	GenerateButtonActions(i, "QWeapon", globals(), 1, 10)

def ActionQSpecPressed (which):
	ActionQSpellPressed (which)

def ActionQSpecRightPressed (which):
	GemRB.SetVar ("QSpell", which)
	GemRB.SetVar ("TopIndex", 0)
	GemRB.SetVar ("ActionLevel", UAW_INNATES)
	UpdateActionsWindow ()

def ActionQShapePressed (which):
	ActionQSpellPressed (which)

def ActionQShapeRightPressed (which):
	GemRB.SetVar ("QSpell", which)
	GemRB.SetVar ("TopIndex", 0)
	GemRB.SetVar ("ActionLevel", UAW_QSHAPES)
	UpdateActionsWindow ()

def ActionQSongPressed (which):
	SelectBardSong (which) # TODO: verify parameter once we have actionbar customisation
	ActionBardSongPressed ()

def ActionQSongRightPressed (which):
	GemRB.SetVar ("QSpell", which)
	GemRB.SetVar ("TopIndex", 0)
	GemRB.SetVar ("ActionLevel", UAW_QSONGS)
	UpdateActionsWindow ()

# can't pass the globals dictionary from another module
def SetActionIconWorkaround(Button, action, function):
	Button.SetActionIcon (globals(), action, function)

# no check needed because the button wouldn't be drawn if illegal
def ActionLeftPressed ():
	"""Scrolls the actions window left.

	Used primarily for spell selection."""

	TopIndex = GemRB.GetVar ("TopIndex")
	if TopIndex>10:
		TopIndex -= 10
	else:
		TopIndex = 0
	GemRB.SetVar ("TopIndex", TopIndex)
	UpdateActionsWindow ()
	return

# no check needed because the button wouldn't be drawn if illegal
def ActionRightPressed ():
	"""Scrolls the action window right.

	Used primarily for spell selection."""

	pc = GemRB.GameGetFirstSelectedActor ()
	TopIndex = GemRB.GetVar ("TopIndex")
	Type = GemRB.GetVar ("Type")
	print("Type:", Type)
	# Type is a bitfield if there is no level given
	# This is to make sure cleric/mages get all spells listed
	if GemRB.GetVar ("ActionLevel") == UAW_ALLMAGE:
		if Type == 3:
			Max = len(Spellbook.GetKnownSpells (pc, IE_SPELL_TYPE_PRIEST) + Spellbook.GetKnownSpells (pc, IE_SPELL_TYPE_WIZARD))
		else:
			Max = GemRB.GetKnownSpellsCount (pc, Type, -1) # this can handle only one type at a time
	else:
		Max = GemRB.GetMemorizedSpellsCount(pc, Type, -1, 1)
	print("Max:", Max)
	TopIndex += 10
	if TopIndex > Max - 10:
		if Max>10:
			if TopIndex > Max:
				TopIndex = Max - 10
		else:
			TopIndex = 0
	GemRB.SetVar ("TopIndex", TopIndex)
	UpdateActionsWindow ()
	return

def ActionMeleePressed ():
	""" switches to the most damaging melee weapon"""

	# get the party Index
	pc = GemRB.GameGetFirstSelectedPC ()
	GemRB.ExecuteString("EquipMostDamagingMelee()", pc)
	return

def ActionRangePressed ():
	""" switches to the most damaging ranged weapon"""

	# get the party Index
	pc = GemRB.GameGetFirstSelectedPC ()
	GemRB.ExecuteString("EquipRanged()", pc)
	return

def ActionShapeChangePressed ():
	GemRB.SetVar ("ActionLevel", UAW_QSHAPES)
	GemRB.SetVar ("TopIndex", 0)
	UpdateActionsWindow ()
	return

def SelectBardSong (which):
	songType = IE_IWD2_SPELL_SONG
	if GameCheck.IsIWD1 ():
		songType = IE_SPELL_TYPE_SONG
	pc = GemRB.GameGetFirstSelectedActor ()
	songs = Spellbook.GetKnownSpells (pc, songType)
	# "which" is a mashup of the spell index with it's type
	idx = which % ((1 << songType) * 100)
	qsong = songs[idx]['SpellResRef']
	# the effect needs to be set each tick, so we use FX_DURATION_INSTANT_PERMANENT==1 timing mode
	# GemRB.SetModalState can also set the spell, but it wouldn't persist
	GemRB.ApplyEffect (pc, 'ChangeBardSong', 0, idx, qsong, "", "", "", 1)

def ActionBardSongRightPressed ():
	"""Selects a bardsong."""
	GemRB.SetVar ("ActionLevel", UAW_QSONGS)
	GemRB.SetVar ("TopIndex", 0)
	UpdateActionsWindow ()
	return

def ActionBardSongPressed ():
	"""Toggles the battle song."""

	# get the global ID
	pc = GemRB.GameGetFirstSelectedActor ()
	GemRB.SetModalState (pc, MS_BATTLESONG)
	GemRB.PlaySound ("act_01")
	GemRB.SetVar ("ActionLevel", UAW_STANDARD)
	UpdateActionsWindow ()
	return

def ActionSearchPressed ():
	"""Toggles detect traps."""

	# get the global ID
	pc = GemRB.GameGetFirstSelectedActor ()
	GemRB.SetModalState (pc, MS_DETECTTRAPS)
	GemRB.SetVar ("ActionLevel", UAW_STANDARD)
	UpdateActionsWindow ()
	return

def ActionStealthPressed ():
	"""Toggles stealth."""

	pc = GemRB.GameGetFirstSelectedActor ()
	GemRB.SetModalState (pc, MS_STEALTH)
	GemRB.PlaySound ("act_07", CHAN_HITS)
	GemRB.SetVar ("ActionLevel", UAW_STANDARD)
	UpdateActionsWindow ()
	return

def ActionTurnPressed ():
	"""Toggles turn undead."""

	pc = GemRB.GameGetFirstSelectedActor ()
	GemRB.SetModalState (pc, MS_TURNUNDEAD)
	GemRB.PlaySound ("act_06")
	GemRB.SetVar ("ActionLevel", UAW_STANDARD)
	UpdateActionsWindow ()
	return

def ActionTamingPressed ():
	pc = GemRB.GameGetFirstSelectedActor ()
	GemRB.SpellCast (pc, -3, 0, "spin108")
	GemRB.SetVar ("ActionLevel", UAW_STANDARD)
	UpdateActionsWindow ()
	return

def ActionWildernessPressed ():
	pc = GemRB.GameGetFirstSelectedActor ()
	GemRB.ApplyEffect (pc, "Reveal:Tracks", 0, 0)
	GemRB.SetVar ("ActionLevel", UAW_STANDARD)
	UpdateActionsWindow ()
	return

def ActionDancePressed ():
	pc = GemRB.GameGetFirstSelectedActor ()
	GemRB.SetModalState (pc, MS_DANCE)
	GemRB.SetVar ("ActionLevel", UAW_STANDARD)
	UpdateActionsWindow ()
	return

def ActionUseItemPressed ():
	GemRB.SetVar ("TopIndex", 0)
	GemRB.SetVar ("ActionLevel", UAW_EQUIPMENT)
	UpdateActionsWindow ()
	return

def ActionCastPressed ():
	"""Opens the spell choice scrollbar."""

	GemRB.SetVar ("QSpell", -1)
	GemRB.SetVar ("TopIndex", 0)
	GemRB.SetVar ("ActionLevel", UAW_SPELLS)
	UpdateActionsWindow ()
	return

def ActionQItemPressed (action):
	"""Uses the given quick item."""

	pc = GemRB.GameGetFirstSelectedActor ()
	# quick slot
	GemRB.UseItem (pc, -2, action, -1)
	return

def ActionQItem1Pressed ():
	ActionQItemPressed (ACT_QSLOT1)
	return

def ActionQItem2Pressed ():
	ActionQItemPressed (ACT_QSLOT2)
	return

def ActionQItem3Pressed ():
	ActionQItemPressed (ACT_QSLOT3)
	return

def ActionQItem4Pressed ():
	ActionQItemPressed (ACT_QSLOT4)
	return

def ActionQItem5Pressed ():
	ActionQItemPressed (ACT_QSLOT5)
	return

def ActionQItemRightPressed (action):
	"""Selects the used ability of the quick item."""

	GemRB.SetVar ("Slot", action)
	GemRB.SetVar ("ActionLevel", UAW_QITEMS)
	UpdateActionsWindow ()
	return

def ActionQItem1RightPressed ():
	ActionQItemRightPressed (19)

def ActionQItem2RightPressed ():
	ActionQItemRightPressed (20)

def ActionQItem3RightPressed ():
	ActionQItemRightPressed (21)

def ActionQItem4RightPressed ():
	ActionQItemRightPressed (22)

def ActionQItem5RightPressed ():
	ActionQItemRightPressed (23)

def ActionInnatePressed ():
	"""Opens the innate spell scrollbar."""

	GemRB.SetVar ("QSpell", -1)
	GemRB.SetVar ("TopIndex", 0)
	GemRB.SetVar ("ActionLevel", UAW_INNATES)
	UpdateActionsWindow ()
	return

def ActionSkillsPressed ():
	GemRB.SetVar ("TopIndex", 0)
	GemRB.SetVar ("ActionLevel", UAW_SKILLS)
	UpdateActionsWindow ()
	return

def TypeSpellPressed (spelltype):
	GemRB.SetVar ("Type", 1 << spelltype)
	GemRB.SetVar ("ActionLevel", UAW_BOOK)
	UpdateActionsWindow ()
	return

def ActionBardSpellPressed ():
	TypeSpellPressed(IE_IWD2_SPELL_BARD)
	return

def ActionClericSpellPressed ():
	TypeSpellPressed(IE_IWD2_SPELL_CLERIC)
	return

def ActionDruidSpellPressed ():
	TypeSpellPressed(IE_IWD2_SPELL_DRUID)
	return

def ActionPaladinSpellPressed ():
	TypeSpellPressed(IE_IWD2_SPELL_PALADIN)
	return

def ActionRangerSpellPressed ():
	TypeSpellPressed(IE_IWD2_SPELL_RANGER)
	return

def ActionSorcererSpellPressed ():
	TypeSpellPressed(IE_IWD2_SPELL_SORCERER)
	return

def ActionWizardSpellPressed ():
	TypeSpellPressed(IE_IWD2_SPELL_WIZARD)
	return

def ActionDomainSpellPressed ():
	TypeSpellPressed(IE_IWD2_SPELL_DOMAIN)
	return

def ActionWildShapesPressed ():
	TypeSpellPressed(IE_IWD2_SPELL_SHAPE)
	return

def SpellShiftPressed ():
	Spell = GemRB.GetVar ("Spell") # spellindex from spellbook jumbled with booktype
	Type =  Spell // 1000
	SpellIndex = Spell % 1000

	# try spontaneous casting
	pc = GemRB.GameGetFirstSelectedActor ()
	ClassRowName = GUICommon.GetClassRowName (pc)
	SponCastTableName = CommonTables.ClassSkills.GetValue (ClassRowName, "SPONCAST")
	if SponCastTableName == "*":
		# proceed as if nothing happened
		SpellPressed ()
		return

	SponCastTable = GemRB.LoadTable (SponCastTableName, True, True)
	if not SponCastTable:
		print("SpellShiftPressed: skipping, non-existent spontaneous casting table used! ResRef:", SponCastTableName)
		SpellPressed ()
		return

	# determine the column number (spell variety) depending on alignment
	CureOrHarm = GemRB.GetPlayerStat (pc, IE_ALIGNMENT)
	if CureOrHarm % 16 == 3: # evil
		CureOrHarm = 1
	else:
		CureOrHarm = 0

	# get the unshifted booktype
	BaseType = 0
	tmp = Type
	while tmp > 1:
		tmp = tmp >> 1
		BaseType += 1

	# figure out the spell's details
	# TODO: find a simpler way
	Spell = None
	MemorisedSpells = Spellbook.GetSpellinfoSpells (pc, BaseType)
	for spell in MemorisedSpells:
		if spell['SpellIndex'] % (255000) == SpellIndex: # 255 is the engine value of Type
			Spell = spell
			break

	# rownames==level; col1: good+neutral; col2: evil resref
	Level = Spell['SpellLevel']
	ReplacementSpell = SponCastTable.GetValue (Level - 1, CureOrHarm).upper()
	if ReplacementSpell != Spell['SpellResRef'].upper():
		SpellIndex = GemRB.PrepareSpontaneousCast (pc, Spell['SpellResRef'], Spell['BookType'], Level, ReplacementSpell)
		GemRB.SetVar ("Spell", SpellIndex + 1000 * Type)
		if GameCheck.IsIWD2 ():
			GemRB.DisplayString (39742, ColorWhite, pc) # Spontaneous Casting

	SpellPressed ()

# This is the endpoint for spellcasting, finally calling SpellCast. This always happens at least
# twice though, the second time to reset the action bar (more if wild magic or subspell selection is involved).
# Spell and Type (spellbook type) are set during the spell bar construction/use, which is in turn
# affected by ActionLevel (see UpdateActionsWindow of this module).
# Keep in mind, that the core resets Type and/or ActionLevel in the case of subspells (fx_select_spell).
def SpellPressed ():
	"""Prepares a spell to be cast."""

	pc = GemRB.GameGetFirstSelectedActor ()

	Spell = GemRB.GetVar ("Spell")
	Type = GemRB.GetVar ("Type")

	if Type == 1 << IE_IWD2_SPELL_SONG or (Type == 1 << IE_SPELL_TYPE_SONG and not GameCheck.IsIWD2()):
		SelectBardSong (Spell)
		ActionBardSongPressed()
		return

	GemRB.GameControlSetTargetMode (TARGET_MODE_CAST)
	if Type != -1:
		Type = Spell // 1000
	Spell = Spell % 1000
	slot = GemRB.GetVar ("QSpell")
	if slot is not None:
		# setup quickspell slot
		# if spell has no target, return
		# otherwise continue with casting
		Target = GemRB.SetupQuickSpell (pc, slot, Spell, Type)
		# sabotage the immediate casting of self targeting spells
		if Target == 5 or Target == 7:
			Type = -1
			GemRB.GameControlSetTargetMode (TARGET_MODE_NONE)

	if Type==-1:
		GemRB.SetVar ("ActionLevel", UAW_STANDARD)
		GemRB.SetVar("Type", 0)
	GemRB.SpellCast (pc, Type, Spell)
	if GemRB.GetVar ("Type") is not None:
		GemRB.SetVar ("ActionLevel", UAW_STANDARD)
		# init spell list
		GemRB.SpellCast (pc, -1, 0)
	GemRB.SetVar ("TopIndex", 0)
	UpdateActionsWindow ()
	return

def EquipmentPressed ():
	pc = GemRB.GameGetFirstSelectedActor ()

	GemRB.GameControlSetTargetMode (TARGET_MODE_CAST)
	Item = GemRB.GetVar ("Equipment")
	# equipment index
	GemRB.UseItem (pc, -1, Item, -1)
	GemRB.SetVar ("ActionLevel", UAW_STANDARD)
	UpdateActionsWindow ()
	return

def ActionStopPressed ():
	for i in GemRB.GetSelectedActors():
		GemRB.ClearActions (i)
	return

def ActionTalkPressed ():
	GemRB.GameControlSetTargetMode (TARGET_MODE_TALK, GA_NO_DEAD | GA_NO_ENEMY | GA_NO_HIDDEN)

def ActionAttackPressed ():
	GemRB.GameControlSetTargetMode (TARGET_MODE_ATTACK, GA_NO_DEAD | GA_NO_SELF | GA_NO_HIDDEN)

def ActionDefendPressed ():
	GemRB.GameControlSetTargetMode (TARGET_MODE_DEFEND, GA_NO_SELF | GA_NO_ENEMY | GA_NO_HIDDEN)

def ActionThievingPressed ():
	GemRB.GameControlSetTargetMode (TARGET_MODE_PICK, GA_NO_DEAD | GA_NO_SELF | GA_NO_ENEMY | GA_NO_HIDDEN)

def SetupControls (Window, pc, actionOffset, customBar = None):
	global fistDrawn

	if customBar:
		actionRow = customBar
	else:
		actionRow = GemRB.GetPlayerActionRow (pc)

	pcStats = GemRB.GetPCStats (pc) # will be None for summons, familiars!
	invInfo = GemRB.GetInventoryInfo (pc)
	fistDrawn = True

	GUIBT_COUNT = 12
	for i in range(GUIBT_COUNT):
		btn = Window.GetControl (i + actionOffset)
		if not btn:
			raise RuntimeError("Missing action buttons!")

		action = actionRow[i]
		if action == ACT_NONE:
			action = -1

		btn.SetFlags (IE_GUI_BUTTON_NO_IMAGE | IE_GUI_BUTTON_ALIGN_BOTTOM | IE_GUI_BUTTON_ALIGN_RIGHT, OP_SET)
		ret = btn.SetActionIcon (globals(), action, i + 1)
		if ret is not None:
			raise RuntimeError("Cannot set action button {} to {}!".format(i, action))

		if action != -1:
			# reset it to the first one, so we can handle them more easily below
			if (action >= ACT_IWDQSPELL) and (action <= ACT_IWDQSPELL + 9):
				action = ACT_IWDQSPELL
			elif (action >= ACT_IWDQITEM) and (action <= ACT_IWDQITEM + 9):
				action = ACT_IWDQITEM
			elif (action >= ACT_IWDQSPEC) and (action <= ACT_IWDQSPEC + 9):
				action = ACT_IWDQSPEC
			elif (action >= ACT_IWDQSONG) and (action <= ACT_IWDQSONG + 9):
				action = ACT_IWDQSONG

		state = SetupActionButton (pc, action, btn, i, pcStats, invInfo)

		disabledButton = GemRB.GetPlayerStat (pc, IE_DISABLEDBUTTON)
		disablableAction = (action <= ACT_SKILLS) or (action >= ACT_QSPELL1 and action <= ACT_QSPELL3)
		# two entries don't match between the effect param and the constants
		# param          our values
		# 10 bardsong    ACT_QSLOT4=10  ACT_BARDSONG=20
		# 14 find traps  ACT_DEFEND=14  ACT_SEARCH=22
		if disabledButton & (1 << 10) and action == 20:
			action = 10
		elif disabledButton & (1 << 14) and action == 22:
			action = 14
		if action < 0 or (disablableAction and disabledButton & (1 << action)):
			state = IE_GUI_BUTTON_DISABLED

		btn.SetState (state)
		# you have to set this overlay up
		# this state check looks bizarre, but without it most buttons get misrendered
		btn.EnableBorder (1, state == IE_GUI_BUTTON_DISABLED)

def SetupActionButton (pc, action, btn, i, pcStats, invInfo):
	global fistDrawn

	state = IE_GUI_BUTTON_UNPRESSED
	magicSlot = invInfo["MagicSlot"]
	fistSlot = invInfo["FistSlot"]
	weaponSlot = invInfo["WeaponSlot"]
	usedslot = invInfo["UsedSlot"]
	modalState = GemRB.GetModalState (pc)

	def SetItemText (btn, charges, oneIsNone):
		if not btn:
			return

		if charges and (charges > 1 or not oneIsNone):
			btn.SetText (str(charges))
		else:
			btn.SetText (None)

	def SetQSpellBtn (btn, pc, tmp):
		if not pcStats:
			return IE_GUI_BUTTON_DISABLED

		poi = pcStats["QuickSpells"][tmp]
		if poi == "":
			return IE_GUI_BUTTON_FAKEDISABLED

		btn.SetSpellIcon (poi, 1, 1, i + 1)
		memorized = Spellbook.HasSpellinfoSpell (pc, poi)
		if not memorized:
			return IE_GUI_BUTTON_FAKEDISABLED # so right-click can still be invoked to change the binding

		# sigh, get unshifted value back
		bookType = pcStats["QuickSpellsBookType"][tmp]
		if bookType > 0:
			bookType = bin(pcStats["QuickSpellsBookType"][tmp])[::-1].index("1")
		memorizedSpells = Spellbook.GetUsableMemorizedSpells (pc, bookType)
		memorizedCount = 0
		for sp in memorizedSpells:
			if sp["SpellResRef"] == poi:
				memorizedCount = sp["MemoCount"]
				break
		if memorizedCount:
			SetItemText (btn, memorizedCount, True)
			return IE_GUI_BUTTON_UNPRESSED
		else:
			return IE_GUI_BUTTON_FAKEDISABLED

	def SetQSlotBtn (btn, pc, tmp, action):
		btn.SetBAM ("stonitem", 0, 0)
		if not pcStats:
			return IE_GUI_BUTTON_DISABLED
		slot = pcStats["QuickItemSlots"][tmp]
		if slot != 0xffff:
			# no slot translation required
			item = GemRB.GetSlotItem (pc, slot, 1)
			if item:
				# MISC3H (horn of blasting) is not displayed when it is out of usages
				header = pcStats["QuickItemHeaders"][tmp]
				usages = item["Usages" + str(header)]
				# I don't like this feature, if the goal is full IE compatibility
				# make the next two lines conditional on usages != 0
				# SetItemIcon parameter needs header + 6 to display extended header icons
				btn.SetItemIcon (item["ItemResRef"], header + 6, 2 if item["Flags"] & IE_INV_ITEM_IDENTIFIED else 1, i + 1)
				SetItemText (btn, usages, False)
			elif action == ACT_IWDQITEM:
				return IE_GUI_BUTTON_DISABLED
		elif action == ACT_IWDQITEM:
			return IE_GUI_BUTTON_DISABLED
		return IE_GUI_BUTTON_UNPRESSED

	def CanUseActionButton (pc, btnType):
		capability = -1
		if GameCheck.IsIWD2 ():
			if btnType == ACT_STEALTH:
				# iwd2 will automatically use the GetSkill checks for trained skills
				capability = GemRB.GetPlayerStat (pc, IE_STEALTH) + GemRB.GetPlayerStat (pc, IE_HIDEINSHADOWS)
			elif btnType == ACT_THIEVING:
				capability = GemRB.GetPlayerStat (pc, IE_LOCKPICKING) + GemRB.GetPlayerStat (pc, IE_PICKPOCKET)
			elif btnType == ACT_SEARCH:
				capability = 1 # everyone can try to search
			else:
				print("Unknown action (button) type: ", btnType)
		else:
			# use levels instead, so inactive dualclasses work as expected
			if btnType == ACT_STEALTH:
				capability = GemRB.GetPlayerLevel (pc, ISTHIEF) + GemRB.GetPlayerLevel (pc, ISMONK) + GemRB.GetPlayerLevel (pc, ISRANGER)
			elif btnType == ACT_THIEVING:
				capability = GemRB.GetPlayerLevel (pc, ISTHIEF) + GemRB.GetPlayerLevel (pc, ISBARD)
			elif btnType == ACT_SEARCH:
				capability = GemRB.GetPlayerLevel (pc, ISTHIEF) + GemRB.GetPlayerLevel (pc, ISSHAMAN)
			else:
				print("Unknown action (button) type: ", btnType)
		return capability > 0

	def HasAnyActiveCasterLevel (pc):
		levels = GemRB.GetPlayerLevel (pc, ISMAGE) + GemRB.GetPlayerLevel (pc, ISSORCERER) + GemRB.GetPlayerLevel (pc, ISBARD)
		levels = levels + GemRB.GetPlayerLevel (pc, ISCLERIC) + GemRB.GetPlayerLevel (pc, ISDRUID)
		# first spells at level: 9 (bg1), 6 (iwd), 4 (iwd2)
		# lowest works here, since we also check for spell presence
		# — either you have spells from the class or the other one provides them
		levels = levels + int(GemRB.GetPlayerLevel (pc, ISPALADIN) >= 4)
		# first spells at level: 8 (bg1), 6 (iwd), 4 (iwd2)
		# we could look it up in the tables instead
		levels = levels + int(GemRB.GetPlayerLevel(pc, ISRANGER) >= 4)
		return levels > 0

	SetItemText (btn, 0, False)

	########################################################################
	# big main switch
	if action == ACT_INNATE:
		if GameCheck.IsIWD2 ():
			spellType = (1 << IE_IWD2_SPELL_INNATE) | (1 << IE_IWD2_SPELL_SHAPE)
		else:
			spellType = 1 << IE_SPELL_TYPE_INNATE

		if len(GemRB.GetSpelldata (pc, spellType)) == 0:
			state = IE_GUI_BUTTON_DISABLED
	elif action == ACT_CAST:
		# luckily the castable spells in IWD2 are all bits below INNATE, so we can do this trick
		if GameCheck.IsIWD2 ():
			spellType = (1 << IE_IWD2_SPELL_INNATE) - 1
		else:
			spellType = (1 << IE_SPELL_TYPE_INNATE) - 1

		# returns true if there are ANY spells to cast
		if len(GemRB.GetSpelldata (pc, spellType)) == 0 or not HasAnyActiveCasterLevel (pc):
			state = IE_GUI_BUTTON_DISABLED
	elif action in [ACT_BARD, ACT_CLERIC, ACT_DRUID, ACT_PALADIN, ACT_RANGER, ACT_SORCERER, ACT_WIZARD, ACT_DOMAIN]:
		if GameCheck.IsIWD2 ():
			spellType = 1 << (action - ACT_BARD)
		else:
			# only cleric or wizard switch exists in the bg engine
			if action == ACT_WIZARD:
				spellType = 1 << IE_SPELL_TYPE_WIZARD
			else:
				spellType = 1 << IE_SPELL_TYPE_PRIEST

		# returns true if there is ANY spell
		if len(GemRB.GetSpelldata (pc, spellType)) == 0:
			state = IE_GUI_BUTTON_DISABLED
	elif action == ACT_WILDSHAPE or action == ACT_SHAPE:
		if GameCheck.IsIWD2 ():
			spellType = 1 << IE_IWD2_SPELL_SHAPE
		else:
			spellType = 0 # no separate shapes in old spellbook

		# returns true if there is ANY shape
		if len(GemRB.GetSpelldata (pc, spellType)) == 0:
			state = IE_GUI_BUTTON_DISABLED
	elif action == ACT_USE:
		# returns true if there is ANY equipment with a usable header
		if not invInfo["HasEquippedAbilities"]:
			state = IE_GUI_BUTTON_DISABLED
	elif action == ACT_BARDSONG:
		if GameCheck.IsIWD2 ():
			spellType = IE_IWD2_SPELL_SONG
			if not GemRB.GetKnownSpellsCount (pc, spellType):
				state = IE_GUI_BUTTON_DISABLED
			elif modalState == MS_BATTLESONG:
				state = IE_GUI_BUTTON_SELECTED
		elif modalState == MS_BATTLESONG:
			state = IE_GUI_BUTTON_SELECTED
	elif action == ACT_TURN:
		if GemRB.GetPlayerStat (pc, IE_TURNUNDEADLEVEL) < 1:
			state = IE_GUI_BUTTON_DISABLED
		elif modalState == MS_TURNUNDEAD:
			state = IE_GUI_BUTTON_SELECTED
	elif action == ACT_STEALTH:
		if not CanUseActionButton(pc, action):
			state = IE_GUI_BUTTON_DISABLED
		elif modalState == MS_STEALTH:
			state = IE_GUI_BUTTON_SELECTED
	elif action == ACT_SEARCH:
		# in IWD2 everyone can try to search, in bg2 only thieves get the icon
		if not CanUseActionButton(pc, action):
			state = IE_GUI_BUTTON_DISABLED
		elif modalState == MS_DETECTTRAPS:
			state = IE_GUI_BUTTON_SELECTED
	elif action == ACT_THIEVING:
		if not CanUseActionButton(pc, action):
			state = IE_GUI_BUTTON_DISABLED
	elif action == ACT_DANCE:
		if modalState == MS_DANCE:
			state = IE_GUI_BUTTON_SELECTED
	elif action == ACT_TAMING:
		if GemRB.GetPlayerStat (pc, IE_ANIMALS) <= 0:
			state = IE_GUI_BUTTON_DISABLED
	elif action in [ACT_WEAPON1, ACT_WEAPON2, ACT_WEAPON3, ACT_WEAPON4]:
		btn.SetBAM ("stonweap", 0, 0)
		if magicSlot == None:
			if pcStats:
				slot = pcStats["QuickWeaponSlots"][action - ACT_WEAPON1]
			else:
				slot = weaponSlot + (action - ACT_WEAPON1)
		else:
			slot = magicSlot

		item2ResRef = ""
		skip = False
		if slot == 0xffff:
			skip = True
		if not skip:
			item = GemRB.GetSlotItem (pc, slot, 1)
			if item:
				# no slot translation required
				launcherSlot = item["LauncherSlot"]
				if launcherSlot and launcherSlot != fistSlot:
					# launcher/projectile in this slot
					item2 = GemRB.GetSlotItem (pc, launcherSlot, 1)
					item2ResRef = item2["ItemResRef"]
			else:
				skip = True
		if not skip:
			if slot == fistSlot:
				if fistDrawn:
					fistDrawn = False
				else:
					# empty weapon slot, already drawn
					skip = True

		if not skip:
			btn.SetItemIcon (item["ItemResRef"], 4, 2 if item["Flags"] & IE_INV_ITEM_IDENTIFIED else 1, i + 1, item2ResRef)
			if pcStats:
				SetItemText (btn, item["Usages" + str(pcStats["QuickWeaponHeaders"][action - ACT_WEAPON1])], True)
			else:
				SetItemText (btn, 0, True)
			if usedslot == slot:
				btn.EnableBorder (0, True)
				if GemRB.GameControlGetTargetMode () == TARGET_MODE_ATTACK:
					state = IE_GUI_BUTTON_SELECTED
				else:
					state = IE_GUI_BUTTON_FAKEDISABLED
			else:
				btn.EnableBorder (0, False)
	elif action == ACT_IWDQSPELL:
		btn.SetBAM ("stonspel", 0, 0)
		if GameCheck.IsIWD2 () and i > 3:
			tmp = i - 3
		else:
			tmp = 0
		state = SetQSpellBtn (btn, pc, tmp)
	elif action == ACT_IWDQSONG:
		btn.SetBAM ("stonsong", 0, 0)
		if GameCheck.IsIWD2 () and i > 3:
			tmp = i - 3
		else:
			tmp = 0
		state = SetQSpellBtn (btn, pc, tmp)
	elif action == ACT_IWDQSPEC:
		btn.SetBAM ("stonspec", 0, 0)
		if GameCheck.IsIWD2 () and i > 3:
			tmp = i - 3
		else:
			tmp = 0
		state = SetQSpellBtn (btn, pc, tmp)
	elif action in [ACT_QSPELL1, ACT_QSPELL2, ACT_QSPELL3]:
		btn.SetBAM ("stonspel", 0, 0)
		tmp = action - ACT_QSPELL1
		state = SetQSpellBtn (btn, pc, tmp)
	elif action == ACT_IWDQITEM:
		if i > 3:
			tmp = (i + 1) % 3
		else:
			tmp = 0
		state = SetQSlotBtn (btn, pc, tmp, action)
	elif action == ACT_QSLOT1:
		tmp = 0
		state = SetQSlotBtn (btn, pc, tmp, action)
	elif action == ACT_QSLOT2:
		tmp = 1
		state = SetQSlotBtn (btn, pc, tmp, action)
	elif action == ACT_QSLOT3:
		tmp = 2
		state = SetQSlotBtn (btn, pc, tmp, action)
	elif action == ACT_QSLOT4:
		tmp = 3
		state = SetQSlotBtn (btn, pc, tmp, action)
	elif action == ACT_QSLOT5:
		tmp = 4
		state = SetQSlotBtn (btn, pc, tmp, action)

	return state
