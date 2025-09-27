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
GUIBT_COUNT = 12

# The following four functions are currently unused in pst
def EmptyControls ():
	if GameCheck.IsPST():
		return
	Selected = GemRB.GetSelectedSize()
	if Selected==1:
		pc = GemRB.GameGetFirstSelectedActor ()
		# init spell list
		GemRB.SpellCast (pc, -1, 0)

	SetActionLevel (UAW_STANDARD)
	if not CurrentWindow:
		return # current case in our game demo (on right-click)
	for i in range (GUIBT_COUNT):
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

	for i in range (GUIBT_COUNT):
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

	SetActionLevel (UAW_STANDARD)
	Button = CurrentWindow.GetControl (ActionBarControlOffset)
	if GameCheck.IsBG2OrEE ():
		Button.SetActionIcon (globals(), ACT_TALK, 1)
	else:
		Button.SetActionIcon (globals(), ACT_DEFEND, 1)
	Button = CurrentWindow.GetControl (1 + ActionBarControlOffset)
	Button.SetActionIcon (globals(), ACT_ATTACK, 2)
	Button = CurrentWindow.GetControl (2 + ActionBarControlOffset)
	Button.SetActionIcon (globals(), ACT_STOP, 3)
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
	GemRB.SetupQuickItemSlot (pc, 0, slot, ability, 0 if GameCheck.IsIWD2 () else 1)
	SetActionLevel (UAW_STANDARD)
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
	SetActionLevel (UAW_STANDARD)
	return

# this doubles up as an ammo selector (not yet used in pst)
def SetupItemAbilities(pc, slot, only):
	slot_item = GemRB.GetSlotItem(pc, slot, 1 if GameCheck.IsIWD2() else 0)
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
		for i in range (GUIBT_COUNT):
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
					GUICommon.SetButtonAnchor (Button)
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
		rmax = min(len(Tips), GUIBT_COUNT - ammoSlotCount)

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
		SetActionLevel (UAW_STANDARD)
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
		SetActionLevel (UAW_SPELLS_DIRECT)
		UpdateActionsWindow ()
		return

	for i in range (GUIBT_COUNT):
		Button = CurrentWindow.GetControl (i + ActionBarControlOffset)
		if i >= len(usableBooks):
			Button.SetActionIcon (globals(), -1)
			continue
		Button.SetActionIcon (globals(), ACT_BARD + usableBooks[i])
	return

def SelectWeaponSet (setIdx):
	SetActionLevel (UAW_STANDARD)
	ActionQWeaponPressed (setIdx, False)

def SelectWeaponSetAbility (slot, fistSlot, pc):
	item = GemRB.GetSlotItem (pc, slot, 1)
	if item:
		launcherSlot = item["LauncherSlot"]
		if launcherSlot and launcherSlot != fistSlot:
			slot = launcherSlot

	ActionQWeaponRightPressedDirect (slot)

def SetupWeaponSets ():
	pc = GemRB.GameGetFirstSelectedActor ()
	pcStats = GemRB.GetPCStats (pc)
	if not pcStats:
		return

	invInfo = GemRB.GetInventoryInfo (pc)
	magicSlot = invInfo["MagicSlot"]
	usedSlot = invInfo["UsedSlot"]
	fistSlot = invInfo["FistSlot"]
	if usedSlot == magicSlot:
		return

	# display each set with 1 button as a gap
	for setIdx in range(4):
		ctrlOffset = setIdx * 3 + ActionBarControlOffset
		selected = False

		Button = CurrentWindow.GetControl (ctrlOffset)
		slot1 = pcStats["QuickWeaponSlots"][setIdx]
		Button.SetActionIcon (globals(), ACT_WEAPON1 + setIdx, ctrlOffset)
		Button.OnPress (lambda btn, idx = setIdx: SelectWeaponSet (idx))
		SetWeaponButton (Button, ACT_WEAPON1 + setIdx, pc, pcStats, invInfo)
		if usedSlot == slot1:
			Button.SetState (IE_GUI_BUTTON_SELECTED)
			selected = True
			Button.OnRightPress (lambda btn, idx = slot1: SelectWeaponSetAbility (idx, fistSlot, pc))
		else:
			Button.SetState (IE_GUI_BUTTON_ENABLED)

		Button = CurrentWindow.GetControl (ctrlOffset + 1)
		# slot2 would be pcStats["QuickWeaponSlots"][setIdx + 4]
		Button.SetActionIcon (globals(), ACT_OFFHAND, ctrlOffset + 1)
		Button.OnPress (lambda btn, idx = setIdx: SelectWeaponSet (idx))
		SetOffHandButton (Button, pc, pcStats, magicSlot, slot1, invInfo["FistSlot"])
		if selected:
			Button.SetState (IE_GUI_BUTTON_SELECTED)
			Button.OnRightPress (lambda btn, idx = slot1: SelectWeaponSetAbility (idx, fistSlot, pc))
		else:
			Button.SetState (IE_GUI_BUTTON_ENABLED)

		Button = CurrentWindow.GetControl (ctrlOffset + 2)
		Button.SetActionIcon (globals(), -1)
		Button.SetDisabled (True)
	return

# you can change this for custom skills, this is the original engine
skillbar = (ACT_STEALTH, ACT_SEARCH, ACT_THIEVING, ACT_WILDERNESS, ACT_TAMING, ACT_NONE, ACT_NONE, ACT_NONE, ACT_NONE, ACT_NONE, ACT_NONE, ACT_NONE)
def SetupSkillSelection ():
	pc = GemRB.GameGetFirstSelectedActor ()
	SetupControls (CurrentWindow, pc, ActionBarControlOffset, skillbar)
	return

# original customization menu:
# skills, cast, use item (abilities), quick item (opened submenu to select 1 of 3, if any), innates, song
# at the right end: clear button, restore default buttons (restored all of them)
# we just list out the quickslots, since although there are many more, only 3 are settable via the inventory
customizationBar = (ACT_SKILLS, ACT_CAST, ACT_USE, ACT_IWDQITEM, ACT_IWDQITEM + 1, ACT_IWDQITEM + 2, ACT_INNATE, ACT_BARDSONG, ACT_NONE, ACT_NONE, ACT_CLEAR, ACT_RESTORE)
def SetupButtonChoices ():
	GemRB.SetVar ("SettingButtons", 1)
	pc = GemRB.GameGetFirstSelectedActor ()
	SetupControls (CurrentWindow, pc, ActionBarControlOffset, customizationBar)
	return

def SaveActionButton (actionIdx):
	pc = GemRB.GameGetFirstSelectedActor ()
	qslot = GemRB.GetVar ("QuickSlotButton")
	GemRB.SetupQuickSlot (pc, qslot, actionIdx)

	SetActionLevel (UAW_STANDARD)
	UpdateActionsWindow ()

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
	for i in range (GUIBT_COUNT):
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
	level = GemRB.GetVar ("ActionLevel") or UAW_STANDARD
	if GemRB.GetPlayerStat (pc, IE_EA) >= CONTROLLED:
		if GameCheck.IsBG2OrEE ():
			if level == UAW_STANDARD and GemRB.GetPlayerStat (pc, IE_SEX) != 7:
				level = UAW_BG2SUMMONS
		else:
			GroupControls ()
			return

	TopIndex = GemRB.GetVar ("TopIndex") or 0
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
			Button.SetActionIcon (globals(), ACT_ATTACK, 2)
			Button = CurrentWindow.GetControl (2 + ActionBarControlOffset)
			Button.SetActionIcon (globals(), -1, 0)
			# the games showed quick spells, but these actors don't have the data for it
			# we just disable the slots instead
			Button = CurrentWindow.GetControl (3 + ActionBarControlOffset)
			Button.SetActionIcon (globals(), ACT_QSPELL1, 4)
			Button.SetDisabled (True)
			Button = CurrentWindow.GetControl (4 + ActionBarControlOffset)
			Button.SetActionIcon (globals(), ACT_QSPELL2, 5)
			Button.SetDisabled (True)
			Button = CurrentWindow.GetControl (5 + ActionBarControlOffset)
			Button.SetActionIcon (globals(), ACT_QSPELL3, 6)
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
	elif level == UAW_WEAPONSETS:
		SetupWeaponSets ()
	elif level == UAW_CONFIGUREBAR:
		SetupButtonChoices ()
	else:
		print("Invalid action level:", level)
		SetActionLevel (UAW_STANDARD)
		UpdateActionsWindow ()
	return

def ActionQWeaponPressed (which, reset = True):
	"""Selects the given quickslot weapon if possible."""

	pc = GemRB.GameGetFirstSelectedActor ()
	qs = GemRB.GetEquippedQuickSlot (pc, 1)
	if GameCheck.IsIWD2 () and reset:
		# on the main action bar we always present the same slot
		which = qs

	# 38 is the magic slot
	if ((qs==which) or (qs==38)) and GemRB.GameControlGetTargetMode() != TARGET_MODE_ATTACK:
		GemRB.GameControlSetTargetMode (TARGET_MODE_ATTACK, GA_NO_DEAD | GA_NO_SELF | GA_NO_HIDDEN)
	else:
		GemRB.GameControlSetTargetMode (TARGET_MODE_NONE)
		GemRB.SetEquippedQuickSlot (pc, which, -1)

	SetupControls (CurrentWindow, pc, ActionBarControlOffset)
	UpdateActionsWindow ()
	return

def ActionQWeaponRightPressed (which):
	if GameCheck.IsIWD2 ():
		# display weapon set choice instead
		SetActionLevel (UAW_WEAPONSETS)
		UpdateActionsWindow ()
	else:
		ActionQWeaponRightPressedDirect (which)
	return

def ActionQWeaponRightPressedDirect (action):
	"""Selects the used ability of the quick weapon."""

	GemRB.SetVar ("Slot", action)
	SetActionLevel (UAW_QWEAPONS)
	UpdateActionsWindow ()
	return

def ActionOffhandRightPressed (action):
	"""Selects the used weapon set."""

	SetActionLevel (UAW_WEAPONSETS)
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
	if GameCheck.IsIWD2 ():
		StartBarConfiguration ()
	else:
		GemRB.SetVar ("QSpell", which)
		GemRB.SetVar ("TopIndex", 0)
		SetActionLevel (UAW_QSPELLS)
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

# iwd2 weapon slots are sparser due to weapon sets
if GameCheck.IsIWD2 ():
	for i in range(4):
		GenerateButtonActions(i, "QWeapon", globals(), 0, i)
		GenerateButtonActions(i, "QWeapon", globals(), 1, 10 + i)
else:
	for i in range(4):
		GenerateButtonActions(i, "QWeapon", globals(), 0)
		GenerateButtonActions(i, "QWeapon", globals(), 1, 10)

def ActionQSpecPressed (which):
	ActionQSpellPressed (which)

def ActionQSpecRightPressed (which):
	if GameCheck.IsIWD2 ():
		StartBarConfiguration ()
	else:
		GemRB.SetVar ("QSpell", which)
		GemRB.SetVar ("TopIndex", 0)
		SetActionLevel (UAW_INNATES)
		UpdateActionsWindow ()

def ActionQShapePressed (which):
	ActionQSpellPressed (which)

def ActionQShapeRightPressed (which):
	GemRB.SetVar ("QSpell", which)
	GemRB.SetVar ("TopIndex", 0)
	SetActionLevel (UAW_QSHAPES)
	UpdateActionsWindow ()

def ActionQSongPressed (which):
	SelectBardSong (which)
	ActionBardSongPressed ()

def ActionQSongRightPressed (which):
	if GemRB.GetVar ("ActionLevel") != UAW_QSONGS:
		StartBarConfiguration ()
		return

	GemRB.SetVar ("QSpell", which)
	GemRB.SetVar ("TopIndex", 0)
	SetActionLevel (UAW_QSONGS)
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
	SetActionLevel (UAW_QSHAPES)
	GemRB.SetVar ("TopIndex", 0)
	UpdateActionsWindow ()
	return

def SelectBardSong (which):
	songType = IE_IWD2_SPELL_SONG
	if GameCheck.IsIWD1 ():
		songType = IE_SPELL_TYPE_SONG

	# "which" is a mashup of the spell index with it's type
	idx = which % ((1 << songType) * 100)
	pc = GemRB.GameGetFirstSelectedActor ()
	if GemRB.GetVar ("SettingButtons"):
		SaveActionButton (ACT_IWDQSONG + idx)
		# also update PCStats->QuickSpells
		GemRB.SetupQuickSpell (pc, idx, idx, 1 << songType)
		return

	songs = Spellbook.GetKnownSpells (pc, songType)
	qsong = songs[idx]['SpellResRef']
	# the effect needs to be set each tick, so we use FX_DURATION_INSTANT_PERMANENT==1 timing mode
	# GemRB.SetModalState can also set the spell, but it wouldn't persist
	GemRB.ApplyEffect (pc, 'ChangeBardSong', 0, idx, qsong, "", "", "", 1)

def ActionBardSongRightPressed ():
	"""Selects a bardsong."""

	# do nothing if only 1 song is known
	pc = GemRB.GameGetFirstSelectedActor ()
	if GemRB.GetVar ("SettingButtons") and len(Spellbook.GetKnownSpells (pc, IE_IWD2_SPELL_SONG)) > 1:
		SetActionLevel (UAW_QSONGS)
		GemRB.SetVar ("TopIndex", 0)
		UpdateActionsWindow ()
	else:
		StartBarConfiguration ()

	return

def ActionBardSongPressed ():
	"""Toggles the battle song."""

	if GemRB.GetVar ("SettingButtons"):
		SaveActionButton (ACT_BARDSONG)
		return

	# get the global ID
	pc = GemRB.GameGetFirstSelectedActor ()
	GemRB.SetModalState (pc, MS_BATTLESONG)
	GemRB.PlaySound ("act_01")
	SetActionLevel (UAW_STANDARD)
	UpdateActionsWindow ()
	return

def ActionSearchPressed ():
	"""Toggles detect traps."""

	if GemRB.GetVar ("SettingButtons"):
		SaveActionButton (ACT_SEARCH)
		return

	# get the global ID
	pc = GemRB.GameGetFirstSelectedActor ()
	GemRB.SetModalState (pc, MS_DETECTTRAPS)
	SetActionLevel (UAW_STANDARD)
	UpdateActionsWindow ()
	return

def ActionSearchRightPressed ():
	if GemRB.GetVar ("ActionLevel") != UAW_SKILLS:
		StartBarConfiguration ()
	return

def ActionStealthPressed ():
	"""Toggles stealth."""

	if GemRB.GetVar ("SettingButtons"):
		SaveActionButton (ACT_STEALTH)
		return

	pc = GemRB.GameGetFirstSelectedActor ()
	GemRB.SetModalState (pc, MS_STEALTH)
	GemRB.PlaySound ("act_07", CHAN_HITS)
	SetActionLevel (UAW_STANDARD)
	UpdateActionsWindow ()
	return

def ActionStealthRightPressed ():
	if GemRB.GetVar ("ActionLevel") != UAW_SKILLS:
		StartBarConfiguration ()
	return

def ActionTurnPressed ():
	"""Toggles turn undead."""

	pc = GemRB.GameGetFirstSelectedActor ()
	GemRB.SetModalState (pc, MS_TURNUNDEAD)
	GemRB.PlaySound ("act_06")
	SetActionLevel (UAW_STANDARD)
	UpdateActionsWindow ()
	return

def ActionTamingPressed ():
	if GemRB.GetVar ("SettingButtons"):
		SaveActionButton (ACT_TAMING)
		return

	pc = GemRB.GameGetFirstSelectedActor ()
	GemRB.SpellCast (pc, -3, 0, "spin108")
	SetActionLevel (UAW_STANDARD)
	UpdateActionsWindow ()
	return

def ActionTamingRightPressed ():
	if GemRB.GetVar ("ActionLevel") != UAW_SKILLS:
		StartBarConfiguration ()
	return

def ActionWildernessPressed ():
	if GemRB.GetVar ("SettingButtons"):
		SaveActionButton (ACT_WILDERNESS)
		return

	pc = GemRB.GameGetFirstSelectedActor ()
	GemRB.ApplyEffect (pc, "Reveal:Tracks", 0, 0)
	SetActionLevel (UAW_STANDARD)
	UpdateActionsWindow ()
	return

def ActionWildernessRightPressed ():
	if GemRB.GetVar ("ActionLevel") != UAW_SKILLS:
		StartBarConfiguration ()
	return

def ActionDancePressed ():
	pc = GemRB.GameGetFirstSelectedActor ()
	GemRB.SetModalState (pc, MS_DANCE)
	SetActionLevel (UAW_STANDARD)
	UpdateActionsWindow ()
	return

def ActionUseItemPressed ():
	if GemRB.GetVar ("SettingButtons"):
		SaveActionButton (ACT_USE)
		return

	GemRB.SetVar ("TopIndex", 0)
	SetActionLevel (UAW_EQUIPMENT)
	UpdateActionsWindow ()
	return

def ActionUseItemRightPressed ():
	StartBarConfiguration ()
	return

def ActionCastPressed ():
	"""Opens the spell choice scrollbar."""

	if GemRB.GetVar ("SettingButtons"):
		SaveActionButton (ACT_CAST)
		return

	GemRB.SetVar ("QSpell", None)
	GemRB.SetVar ("TopIndex", 0)
	SetActionLevel (UAW_SPELLS)
	UpdateActionsWindow ()
	return

def StartBarConfiguration ():
	if not GameCheck.IsIWD2 ():
		return # oops

	SetActionLevel (UAW_CONFIGUREBAR)
	UpdateActionsWindow ()
	return

def ActionCastRightPressed ():
	if GemRB.GetVar ("SettingButtons"):
		SetActionLevel (UAW_SPELLS)
		UpdateActionsWindow ()
		return

	StartBarConfiguration ()
	return

def ActionQItemPressed (action, idx):
	"""Uses the given quick item."""

	if GemRB.GetVar ("SettingButtons"):
		SaveActionButton (ACT_IWDQITEM + idx)
		return

	pc = GemRB.GameGetFirstSelectedActor ()
	# quick slot
	GemRB.UseItem (pc, -2, action, -1)
	return

def ActionQItem1Pressed ():
	ActionQItemPressed (ACT_QSLOT1, 0)
	return

def ActionQItem2Pressed ():
	ActionQItemPressed (ACT_QSLOT2, 1)
	return

def ActionQItem3Pressed ():
	ActionQItemPressed (ACT_QSLOT3, 2)
	return

def ActionQItem4Pressed ():
	ActionQItemPressed (ACT_QSLOT4, 3)
	return

def ActionQItem5Pressed ():
	ActionQItemPressed (ACT_QSLOT5, 4)
	return

def ActionQItemRightPressed (which):
	"""Selects the used ability of the quick item."""

	# iwd2 did not allow for ability selection, but we can be better
	if GemRB.GetVar ("SettingButtons") or not GameCheck.IsIWD2 ():
		if GameCheck.IsIWD2 ():
			which = 15 + which - 19 # the 15 is from slottype.2da, the first inventory quickslot
		GemRB.SetVar ("Slot", which)
		SetActionLevel (UAW_QITEMS)
		UpdateActionsWindow ()
	else:
		StartBarConfiguration ()
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

	if GemRB.GetVar ("SettingButtons"):
		SaveActionButton (ACT_INNATE)
		return

	GemRB.SetVar ("QSpell", None)
	GemRB.SetVar ("TopIndex", 0)
	SetActionLevel (UAW_INNATES)
	UpdateActionsWindow ()
	return

def ActionInnateRightPressed ():
	if GemRB.GetVar ("ActionLevel") == UAW_STANDARD:
		StartBarConfiguration ()
		return

	GemRB.SetVar ("QSpell", None)
	GemRB.SetVar ("TopIndex", 0)
	SetActionLevel (UAW_INNATES)
	UpdateActionsWindow ()
	return

def ActionSkillsPressed ():
	if GemRB.GetVar ("SettingButtons"):
		SaveActionButton (ACT_SKILLS)
		return

	GemRB.SetVar ("TopIndex", 0)
	SetActionLevel (UAW_SKILLS)
	UpdateActionsWindow ()
	return

def ActionSkillsRightPressed ():
	if GemRB.GetVar ("SettingButtons"):
		SetActionLevel (UAW_SKILLS)
		UpdateActionsWindow ()
	else:
		StartBarConfiguration ()
	return

# the iwd2 spell types work nicely as indices up to domain spells
# e.g. IE_IWD2_SPELL_BARD = 0 -> ACT_BARD = 40 ... IE_IWD2_SPELL_DOMAIN = 7 -> ACT_DOMAIN = 47
# BUT IE_IWD2_SPELL_INNATE = 8 -> ACT_INNATE=13
# songs and shapes we skip; the former are configurable through bardsong, the latter are innates
def TypeSpellPressed (spellType):
	if GemRB.GetVar ("SettingButtons"):
		if spellType <= IE_IWD2_SPELL_DOMAIN:
			actionType = spellType + ACT_BARD
		else:
			actionType = ACT_INNATE
		SaveActionButton (actionType)
		return

	GemRB.SetVar ("TopIndex", 0)
	GemRB.SetVar ("Type", 1 << spellType)
	SetActionLevel (UAW_BOOK)
	UpdateActionsWindow ()
	return

def TypeSpellRightPressed (spellType):
	if GemRB.GetVar ("SettingButtons"):
		GemRB.SetVar ("TopIndex", 0)
		GemRB.SetVar ("Type", 1 << spellType)
		SetActionLevel (UAW_BOOK)
		UpdateActionsWindow ()
	else:
		StartBarConfiguration ()
	return

# generate the callbacks for individual spellbook buttons
# innates have their own button, so no need to complicate matters by adding them here
name2BookType = { "Bard": IE_IWD2_SPELL_BARD, "Cleric": IE_IWD2_SPELL_CLERIC, "Druid": IE_IWD2_SPELL_DRUID,
								 "Paladin": IE_IWD2_SPELL_PALADIN, "Ranger": IE_IWD2_SPELL_RANGER, "Sorcerer": IE_IWD2_SPELL_SORCERER,
								 "Wizard": IE_IWD2_SPELL_WIZARD, "Domain": IE_IWD2_SPELL_DOMAIN, "WildShapes": IE_IWD2_SPELL_SHAPE }
def GenerateSpellButtonActions(name, g, bookType, right = 0):
	dec = "def Action" + name + "Spell" + suf[right] + "Pressed():\n"
	dec += "\tTypeSpell" + suf[right] + "Pressed(" + str(bookType) + ")"
	exec(dec, g) # pass on the same global dict, so we remain in the top scope

for name, bookType in name2BookType.items():
	GenerateSpellButtonActions(name, globals(), bookType)
	GenerateSpellButtonActions(name, globals(), bookType, 1)

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
	BaseType = UnshiftBookType (Type)

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

	Target = None
	if Type != -1:
		Type = Spell // 1000
	Spell = Spell % 1000
	slot = GemRB.GetVar ("QSpell")
	if GemRB.GetVar ("SettingButtons"):
		slot = GemRB.GetVar ("QuickSlotButton") - 3 # 12 buttons, 9 quickspells, first 3 buttons hardcoded
	if slot is not None:
		# setup quickspell slot
		# if spell has no target, return
		# otherwise continue with casting
		Target = GemRB.SetupQuickSpell (pc, slot, Spell, Type)

	if GemRB.GetVar ("SettingButtons"):
		SaveActionButton (ACT_IWDQSPELL + slot)
		return

	GemRB.GameControlSetTargetMode (TARGET_MODE_CAST)
	# sabotage the immediate casting of self targeting spells
	if Target == 5 or Target == 7:
		Type = -1
		GemRB.GameControlSetTargetMode (TARGET_MODE_NONE)

	if Type==-1:
		SetActionLevel (UAW_STANDARD)
		GemRB.SetVar("Type", 0)
	GemRB.SpellCast (pc, Type, Spell)
	if GemRB.GetVar ("Type") is not None:
		SetActionLevel (UAW_STANDARD)
		# init spell list
		GemRB.SpellCast (pc, -1, 0)
	GemRB.SetVar ("TopIndex", 0)
	UpdateActionsWindow ()
	return

def SpellRightPressed ():
	spell = GemRB.GetVar ("Spell")
	bookType = GemRB.GetVar ("Type")
	if bookType == -1:
		return
	bookType = UnshiftBookType (spell // 1000)

	pc = GemRB.GameGetFirstSelectedActor ()
	memorized = Spellbook.GetUsableMemorizedSpells(pc, bookType)
	for mem in memorized:
		if mem["SpellIndex"] == spell:
			spellRef = mem["SpellResRef"]
			break
	else:
		return

	if GameCheck.IsIWD2 ():
		# fake a button
		btn = lambda: None
		btn.VarName = "ActionBar"
		btn.SpellRef = spellRef
		import GUISPL
		GUISPL.OpenSpellBookSpellInfoWindow (btn)
	else:
		import GUIMG
		spell = GemRB.GetSpell (spellRef)
		GUIMG.OpenMageSpellInfoWindow (spell, "Memorized")

def EquipmentPressed ():
	pc = GemRB.GameGetFirstSelectedActor ()

	GemRB.GameControlSetTargetMode (TARGET_MODE_CAST)
	Item = GemRB.GetVar ("Equipment")
	# equipment index
	GemRB.UseItem (pc, -1, Item, -1)
	SetActionLevel (UAW_STANDARD)
	UpdateActionsWindow ()
	return

def ActionClearPressed ():
	SaveActionButton (ACT_NONE)
	return

def ActionRestorePressed ():
	pc = GemRB.GameGetFirstSelectedActor ()
	GemRB.SetupQuickSlot (pc, -1, 0)
	SetActionLevel (UAW_STANDARD)
	UpdateActionsWindow ()
	return

def EmptiedButtonRightPress ():
	StartBarConfiguration ()
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
	if GemRB.GetVar ("SettingButtons"):
		SaveActionButton (ACT_THIEVING)
		return

	GemRB.GameControlSetTargetMode (TARGET_MODE_PICK, GA_NO_DEAD | GA_NO_SELF | GA_NO_ENEMY | GA_NO_HIDDEN)

def ActionThievingRightPressed ():
	if GemRB.GetVar ("ActionLevel") != UAW_SKILLS:
		StartBarConfiguration ()
	return

def SetupControls (Window, pc, actionOffset, customBar = None):
	global fistDrawn

	if customBar:
		actionRow = customBar
	else:
		actionRow = GemRB.GetPlayerActionRow (pc)

	pcStats = GemRB.GetPCStats (pc) # will be None for summons, familiars!
	invInfo = GemRB.GetInventoryInfo (pc)
	fistDrawn = True

	for i in range(GUIBT_COUNT):
		btn = Window.GetControl (i + actionOffset)
		if not btn:
			raise RuntimeError("Missing action buttons!")

		action = actionRow[i]
		action2 = action
		if action == ACT_NONE:
			action2 = -1

		btn.SetFlags (IE_GUI_BUTTON_NO_IMAGE | IE_GUI_BUTTON_ALIGN_BOTTOM | IE_GUI_BUTTON_ALIGN_RIGHT, OP_SET)
		ret = btn.SetActionIcon (globals(), action2, i + 1)
		if ret is not None:
			raise RuntimeError("Cannot set action button {} to {}!".format(i, action))

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
		# but let various quick spells handle this themselves
		if action < ACT_IWDQSPELL and not action in [ACT_QSPELL1, ACT_QSPELL2, ACT_QSPELL3]:
			btn.EnableBorder (1, state == IE_GUI_BUTTON_DISABLED)

def SetupActionButton (pc, action, btn, i, pcStats, invInfo):
	global fistDrawn

	state = IE_GUI_BUTTON_UNPRESSED
	magicSlot = invInfo["MagicSlot"]
	fistSlot = invInfo["FistSlot"]
	usedslot = invInfo["UsedSlot"]
	modalState = GemRB.GetModalState (pc)

	def SetQSpellBtn (btn, pc, tmp):
		if not pcStats:
			return IE_GUI_BUTTON_DISABLED

		poi = pcStats["QuickSpells"][tmp]
		if poi == "":
			# unset quick spell slot, just waiting to be used
			return IE_GUI_BUTTON_FAKEDISABLED

		btn.SetSpellIcon (poi, 1, 1, i + 1)
		memorized = Spellbook.HasSpellinfoSpell (pc, poi)

		# sigh, get unshifted value back
		bookType = pcStats["QuickSpellsBookType"][tmp]
		if bookType > 0:
			bookType = UnshiftBookType (bookType)

		# 1 << IE_IWD2_SPELL_SONG is too big for QuickSpellBookType :(
		# unfortunately so is 1 << IE_IWD2_SPELL_INNATE, so we can't distinguish them
		if bookType == 0 and modalState == MS_BATTLESONG:
			return IE_GUI_BUTTON_SELECTED
		if memorized:
			btn.EnableBorder(1, False)
		else:
			btn.EnableBorder(1, True)
			return IE_GUI_BUTTON_FAKEDISABLED # so right-click can still be invoked to change the binding

		if GameCheck.IsIWD2 () and bookType == 0:
			bookType = IE_IWD2_SPELL_INNATE
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
		levels = levels + GemRB.GetPlayerLevel (pc, ISCLERIC) + GemRB.GetPlayerLevel (pc, ISDRUID) + GemRB.GetPlayerLevel (pc, ISSHAMAN)
		# first spells at level: 9 (bg1), 6 (iwd), 4 (iwd2)
		# lowest works here, since we also check for spell presence
		# — either you have spells from the class or the other one provides them
		levels = levels + int(GemRB.GetPlayerLevel (pc, ISPALADIN) >= 4)
		# first spells at level: 8 (bg1), 6 (iwd), 4 (iwd2)
		# we could look it up in the tables instead
		levels = levels + int(GemRB.GetPlayerLevel(pc, ISRANGER) >= 4)
		return levels > 0

	########################################################################
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
	elif action == ACT_NONE:
		btn.OnRightPress (EmptiedButtonRightPress)
		state = IE_GUI_BUTTON_FAKEDISABLED
	elif action == ACT_USE:
		# returns true if there is ANY equipment with a usable header
		if not invInfo["HasEquippedAbilities"]:
			state = IE_GUI_BUTTON_DISABLED
	elif action == ACT_BARDSONG:
		if not GemRB.GetPlayerLevel (pc, ISBARD):
			state = IE_GUI_BUTTON_DISABLED
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
		realAction = action
		if GameCheck.IsIWD2 () and pcStats and usedslot in pcStats["QuickWeaponSlots"]:
			# pick the selected weapon set instead, unless the magic slot is in use
			realAction = ACT_WEAPON1 + pcStats["QuickWeaponSlots"].index(usedslot)
		state = SetWeaponButton (btn, realAction, pc, pcStats, invInfo)
	elif action == ACT_OFFHAND: # only used in iwd2
		state = SetOffHandButton (btn, pc, pcStats, magicSlot, usedslot, fistSlot)
	elif (action >= ACT_IWDQSPELL) and (action <= ACT_IWDQSPELL + 9):
		btn.SetBAM ("stonspel", 0, 0)
		tmp = action - ACT_IWDQSPELL
		state = SetQSpellBtn (btn, pc, tmp)
	elif (action >= ACT_IWDQSONG) and (action <= ACT_IWDQSONG + 9):
		btn.SetBAM ("stonsong", 0, 0)
		tmp = action - ACT_IWDQSONG
		state = SetQSpellBtn (btn, pc, tmp)
	elif (action >= ACT_IWDQSPEC) and (action <= ACT_IWDQSPEC + 9):
		btn.SetBAM ("stonspec", 0, 0)
		tmp = action - ACT_IWDQSPEC
		state = SetQSpellBtn (btn, pc, tmp)
	elif action in [ACT_QSPELL1, ACT_QSPELL2, ACT_QSPELL3]:
		btn.SetBAM ("stonspel", 0, 0)
		tmp = action - ACT_QSPELL1
		state = SetQSpellBtn (btn, pc, tmp)
	elif (action >= ACT_IWDQITEM) and (action <= ACT_IWDQITEM + 9):
		tmp = action - ACT_IWDQITEM
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

def SetItemText (btn, charges, oneIsNone):
	if not btn:
		return

	GUICommon.SetButtonAnchor (btn)
	if charges and (charges > 1 or not oneIsNone):
		btn.SetText (str(charges))
	else:
		btn.SetText (None)

def SetWeaponButton (btn, action, pc, pcStats, invInfo):
	global fistDrawn

	magicSlot = invInfo["MagicSlot"]
	fistSlot = invInfo["FistSlot"]
	weaponSlot = invInfo["WeaponSlot"]
	usedSlot = invInfo["UsedSlot"]

	btn.SetBAM ("stonweap", 0, 0)
	state = IE_GUI_BUTTON_UNPRESSED
	if magicSlot == None:
		if pcStats:
			slot = pcStats["QuickWeaponSlots"][action - ACT_WEAPON1]
		else:
			slot = weaponSlot + (action - ACT_WEAPON1)
	else:
		slot = magicSlot
		action = ACT_WEAPON1 # avoid oob-ing QuickWeaponHeaders

	item2ResRef = ""
	if slot == 0xffff:
		return state

	item = GemRB.GetSlotItem (pc, slot, 1)
	if not item:
		return state

	# no slot translation required
	launcherSlot = item["LauncherSlot"]
	if launcherSlot and launcherSlot != fistSlot:
		# launcher/projectile in this slot
		item2 = GemRB.GetSlotItem (pc, launcherSlot, 1)
		item2ResRef = item2["ItemResRef"]

	if slot == fistSlot:
		if fistDrawn:
			fistDrawn = False
		else:
			# empty weapon slot, already drawn
			return state

	btn.SetItemIcon (item["ItemResRef"], 4, 2 if item["Flags"] & IE_INV_ITEM_IDENTIFIED else 1, i + 1, item2ResRef)
	if pcStats:
		SetItemText (btn, item["Usages" + str(pcStats["QuickWeaponHeaders"][action - ACT_WEAPON1])], True)
	else:
		SetItemText (btn, 0, True)

	if usedSlot == slot:
		btn.EnableBorder (0, True)
		if GemRB.GameControlGetTargetMode () == TARGET_MODE_ATTACK:
			state = IE_GUI_BUTTON_SELECTED
		else:
			state = IE_GUI_BUTTON_FAKEDISABLED
	else:
		btn.EnableBorder (0, False)

	return state

def SetOffHandButton (btn, pc, pcStats, magicSlot, usedSlot, fistSlot):
	if magicSlot == None:
		offhandSlot = usedSlot + 1
	else:
		offhandSlot = magicSlot

	item = GemRB.GetSlotItem (pc, offhandSlot, 1)
	state = IE_GUI_BUTTON_LOCKED
	if item and usedSlot != fistSlot and item["LauncherSlot"] == 0:
		btn.SetItemIcon (item["ItemResRef"], 4, 2 if item["Flags"] & IE_INV_ITEM_IDENTIFIED else 1, i + 1)
		if pcStats and offhandSlot in pcStats["QuickWeaponSlots"]:
			qwIdx = pcStats["QuickWeaponSlots"].index(offhandSlot)
			SetItemText (btn, item["Usages" + str(pcStats["QuickWeaponHeaders"][qwIdx])], True)
		else:
			SetItemText (btn, 0, True)
		btn.EnableBorder (0, True)
		if GemRB.GameControlGetTargetMode () == TARGET_MODE_ATTACK:
			state = IE_GUI_BUTTON_SELECTED
		else:
			state = IE_GUI_BUTTON_FAKEDISABLED
	else:
		btn.SetBAM ("stonshil", 0, 0)
	return state

def SetActionLevel (level):
	GemRB.SetVar ("ActionLevel", level)
	if level == UAW_STANDARD:
		GemRB.SetVar ("SettingButtons", 0)

def UnshiftBookType (bookType):
	return bin(bookType)[::-1].index("1")
