# GemRB - Infinity Engine Emulator
# Copyright (C) 2010 The GemRB Project
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

import re

import GemRB
import CommonTables
import GameCheck
import GUICommon
import Spellbook
from GUIDefines import *
from ie_stats import *
from ie_slots import *
from ie_spells import *
from ie_sounds import DEF_IDENTIFY

UsedSlot = None
ItemAmountWindow = None
ColorPicker = None
StackAmount = 0
ClassKitUsability = None
AlignmentStancesTable = None

# A map that defines which inventory slots are used per character (PST)
SlotMap = None

# a runtime-only list of items already identified, so optionally, when you pick
# up the same item later, it's autoidentified
IdentifiedItems = set()

def InventoryClosed(win):
	GemRB.LeaveContainer()
	if GemRB.IsDraggingItem () == 1:
		pc = GemRB.GameGetSelectedPCSingle ()
		#store the item in the inventory before window is closed
		GemRB.DropDraggedItem (pc, -3)
		#dropping on ground if cannot store in inventory
		if GemRB.IsDraggingItem () == 1:
			GemRB.DropDraggedItem (pc, -2)
	
	import GUICommonWindows
	GUICommonWindows.TopWindowClosed(win)

def OnDragItemGround (btn):
	"""Drops and item to the ground."""
	
	pc = GemRB.GameGetSelectedPCSingle ()
	slot = btn.Value

	GemRB.GetView ("MsgSys").SetText ("")

	if GemRB.IsDraggingItem ()==0:
		slot_item = GemRB.GetContainerItem (pc, slot)
		item = GemRB.GetItem (slot_item["ItemResRef"])
		GemRB.DragItem (pc, slot, item["ItemIcon"], 0, 1) #container
		if GameCheck.IsPST():
			GemRB.PlaySound (item["DescIcon"])
	else:
		GemRB.DropDraggedItem (pc, -2) #dropping on ground

	return

def OnAutoEquip ():
	"""Auto-equips equipment if possible."""

	if GemRB.IsDraggingItem ()!=1:
		return

	pc = GemRB.GameGetSelectedPCSingle ()

	#-1 : drop stuff in equipable slots (but not inventory)
	GemRB.DropDraggedItem (pc, -1)

	if GemRB.IsDraggingItem ()==1:
		GemRB.PlaySound("GAM_47") #failed equip

	return

def OnDragItem (btn):
	"""Updates dragging."""

	#don't call when splitting items
	if ItemAmountWindow != None:
		return

	slot = btn.Value
	pc = GemRB.GameGetSelectedPCSingle ()
	slot_item = GemRB.GetSlotItem (pc, slot)
	SlotType = GemRB.GetSlotType (slot, pc)

	# bg2: no armor switching during combat
	if SlotType["Type"] == SLOT_ARMOUR and GemRB.EvaluateString ("ActuallyInCombat()", True):
		errorRef = -1
		if GameCheck.IsBG2OrEE ():
			errorRef = 62024
		elif GameCheck.IsBGEE ():
			errorRef = 24169
		elif GameCheck.IsPSTEE ():
			errorRef = 68554
		if errorRef != -1:
			GemRB.DisplayString (errorRef, ColorWhite)
			return

	GemRB.GetView ("MsgSys").SetText ("")

	if not GemRB.IsDraggingItem ():
		item = GemRB.GetItem (slot_item["ItemResRef"])
		GemRB.DragItem (pc, slot, item["ItemIcon"], 0, 0)
		if slot == 2: # reset disguise
			GemRB.SetGlobal ("APPEARANCE", "GLOBAL", 0)
	else:
		#special monk check
		if GemRB.GetPlayerStat (pc, IE_CLASS) == 20 and SlotType["Effects"] == TYPE_OFFHAND:
			SlotType["ResRef"] = ""
			GemRB.DisplayString (61355, ColorWhite)

		if SlotType["ResRef"]!="":
			if slot_item:
				item = GemRB.GetItem (slot_item["ItemResRef"])
				#drag items into a bag
				if item["Function"] & ITM_F_CONTAINER:
					#first swap them
					GemRB.DropDraggedItem (pc, slot)
					#enter the store
					GemRB.EnterStore (slot_item["ItemResRef"])
					#if it is possible to add, then do it
					ret = GemRB.IsValidStoreItem (pc, slot, 0)
					if ret&SHOP_SELL:
						GemRB.ChangeStoreItem (pc, slot, SHOP_SELL)
					else:
						msg = 9375
						if ret&SHOP_FULL:
							if GameCheck.IsIWD1() or GameCheck.IsIWD2():
								msg = 24893
							elif GameCheck.HasTOB():
								msg = 54692
						GemRB.DisplayString(msg, ColorWhite)
					#leave (save) store
					GemRB.LeaveStore()

			item = GemRB.GetSlotItem (0, 0)
			itemData = GemRB.GetItem (item['ItemResRef'])
			GemRB.DropDraggedItem (pc, slot)

			# handle pst disguises
			if slot == 2 and item['ItemResRef'] == 'dustrobe':
				GemRB.SetGlobal ("APPEARANCE", "GLOBAL", 2)
			elif slot < 21 and itemData['AnimationType'] != '':
				GemRB.SetGlobal ("APPEARANCE", "GLOBAL", 0)

			# drop item if it caused us to disable the inventory view (example: cursed berserking sword)
			if GemRB.GetPlayerStat (pc, IE_STATE_ID) & (STATE_BERSERK) and GemRB.IsDraggingItem ():
				GemRB.DropDraggedItem (pc, -3)

	return

def OnDropItemToPC (pc):
	"""Gives an item to another character."""

	if pc > GemRB.GetPartySize ():
		return

	GemRB.GetView ("MsgSys").SetText ("")

	#-3 : drop stuff in inventory (but not equippable slots)
	GemRB.DropDraggedItem (pc, -3)
	return

def DecreaseStackAmount ():
	"""Decreases the split size."""

	Text = ItemAmountWindow.GetControl (6)
	Amount = Text.QueryText ()
	number = int ("0"+Amount)-1
	if number<1:
		number=1
	Text.SetText (str (number))
	return

def IncreaseStackAmount ():
	"""Increases the split size."""

	Text = ItemAmountWindow.GetControl (6)
	Amount = Text.QueryText ()
	number = int ("0"+Amount)+1
	if number>StackAmount:
		number=StackAmount
	Text.SetText (str (number))
	return

def DragItemAmount (slot_item, location = "inventory"):
	"""Drag a split item."""

	pc = GemRB.GameGetSelectedPCSingle ()

	#emergency dropping
	if GemRB.IsDraggingItem()==1:
		GemRB.DropDraggedItem (pc, UsedSlot)
		UpdateSlot (pc, UsedSlot-1)

	#if dropping didn't help, don't die if slot_item isn't here
	if slot_item:
		Text = ItemAmountWindow.GetControl (6)
		Amount = Text.QueryInteger ()
		item = GemRB.GetItem (slot_item["ItemResRef"])
		GemRB.DragItem (pc, UsedSlot, item["ItemIcon"], Amount, location == "ground")
	ItemAmountWindow.Close()
	return

def MouseEnterSlot (Button):
	pc = GemRB.GameGetSelectedPCSingle ()

	if GemRB.IsDraggingItem ()==1:
		drag_item = GemRB.GetSlotItem (0, 0)
		SlotType = UpdateSlot (pc, Button.Value - 1)

		if GemRB.CanUseItemType (SlotType["Type"], drag_item["ItemResRef"]):
			Button.SetState (IE_GUI_BUTTON_SELECTED)
		else:
			Button.SetState (IE_GUI_BUTTON_ENABLED)
		
	return

def MouseLeaveSlot (Button):
	pc = GemRB.GameGetSelectedPCSingle ()

	UpdateSlot (pc, Button.Value - 1)
	return

def MouseEnterGround (Button):
	if GemRB.IsDraggingItem ()==1:
		Button.SetState (IE_GUI_BUTTON_SELECTED)
	return

def MouseLeaveGround (Button):
	if GemRB.IsDraggingItem ()==1:
		Button.SetState (IE_GUI_BUTTON_FAKEPRESSED)
	return

def GetItemDescription (item, itemtype):
	text = item["ItemDescIdentified"]
	if (itemtype & 2):
		text = item["ItemDesc"]

	if GameCheck.IsIWD2():
		usabilityMask = item["UsabilityBitmask"]

		# restrictions apply, so we list them
		if usabilityMask > 0:
			text = GemRB.GetString(text) + "\n\n"
			text += "[color=ffff00ff]" + GemRB.GetString(31530) + "[/color]\n  "

			if usabilityMask & 0x3F000: # alignment/stances
				text += GetIWD2RestrictedAlignmentsAndStances(usabilityMask)
			if usabilityMask & 0x3F800000: # race
				text += GetIWD2RestrictedRaces(usabilityMask)
			if usabilityMask & 0xFFF: # classes (incl. unknown)
				kitMask = item["KitUsability"]
				text += GetIWD2RestrictedClassesAndKits(usabilityMask, kitMask)

	if not GameCheck.IsPST ():
		return text

	# PST had "NOTE:" handling hardcoded, see string 49156 for explanation
	# we support translated versions visible at least in Czech as well (POZN)
	text = GemRB.GetString (text)
	searchRE = re.compile(r'^([A-Z][A-Z][A-Z][A-Z]+[: ]..[a-z"].*?($|\r?\n(\r?\n)?))', re.MULTILINE | re.DOTALL)
	replacement = r"[color=ffffff]\1[/color]"
	text = searchRE.sub(replacement, text)
	return text

def GetIWD2RestrictedAlignmentsAndStances(mask):
	global AlignmentStancesTable
	if not AlignmentStancesTable:
		AlignmentStancesTable = GemRB.LoadTable("aligstan")

	text = ""
	n = AlignmentStancesTable.GetRowCount()

	for i in range(n):
		rowName = AlignmentStancesTable.GetRowName(i)
		useMask = AlignmentStancesTable.GetValue(rowName, "USABILITY")
		nameRef = AlignmentStancesTable.GetValue(rowName, "NAME_REF")

		if useMask & mask:
			text += GemRB.GetString(nameRef) + "\n  "

	return text

def GetIWD2RestrictedRaces(mask):
	text = ""
	n = CommonTables.Races.GetRowCount()

	for i in range(n):
		rowName = CommonTables.Races.GetRowName(i)
		raceMask = CommonTables.Races.GetValue(rowName, "USABILITY")
		nameRef = CommonTables.Races.GetValue(rowName, "NAME_REF")

		if raceMask == 0x7FFFFFFF: # GetValue gives as signed value
			raceMask = 0x80000000

		if mask & raceMask:
			text += GemRB.GetString(nameRef) + "\n  "

	return text

def PrepareClassKitUsabilityMasks():
	global ClassKitUsability
	ClassKitUsability = {}
	n = CommonTables.Classes.GetRowCount()

	for i in range(n):
		rowName = CommonTables.Classes.GetRowName(i)
		parentClass = CommonTables.Classes.GetValue(rowName, "CLASS")
		if parentClass == 0:
			continue

		mask = ClassKitUsability.get(parentClass, 0)
		mask |= CommonTables.Classes.GetValue(rowName, "USABILITY")
		ClassKitUsability[parentClass] = mask


def GetIWD2RestrictedClassesAndKits(classesMask, kitsMask):
	global ClassKitUsability
	if not ClassKitUsability:
		PrepareClassKitUsabilityMasks()

	text = ""
	n = CommonTables.Classes.GetRowCount()
	for i in range(n):
		rowName = CommonTables.Classes.GetRowName(i)
		parentClass = CommonTables.Classes.GetValue(rowName, "CLASS")
		mask = CommonTables.Classes.GetValue(rowName, "USABILITY")
		nameRef = CommonTables.Classes.GetValue(rowName, "NAME_REF")
		kitMask = ClassKitUsability.get(i + 1, 0)

		if parentClass != 0:
			continue

		if classesMask & mask or (kitMask and kitsMask & kitMask):
			# there are exceptions for individual kits, so list them
			if kitMask and kitsMask & kitMask != kitMask:
				for j in range(n):
					kitName = CommonTables.Classes.GetRowName(j)
					parentClass = CommonTables.Classes.GetValue(kitName, "CLASS")
					mask = CommonTables.Classes.GetValue(kitName, "USABILITY")
					nameRef = CommonTables.Classes.GetValue(kitName, "NAME_REF")

					if parentClass != i + 1:
						continue

					if kitsMask & mask:
						text += GemRB.GetString(nameRef) + "\n  "
			else:
				text += GemRB.GetString(nameRef) + "\n  "

	return text

def ItemPress(func, slotItem, *args):
	def Pressed(btn):
		func(slotItem, *args)
		btn.Window.Close()
	return Pressed

def DisplayItem (slotItem, ground = False):
	Window = GemRB.LoadWindow (5, "GUIINV")

	# the ground items are only displayable
	if slotItem["Flags"] & IE_INV_ITEM_IDENTIFIED:
		value = 0
	else:
		value = 2

	if not ground:
		value += 1

	UpdateItemDisplay(Window, slotItem, value)

def UpdateItemDisplay(Window, slotItem, itemtype):
	item = GemRB.GetItem (slotItem["ItemResRef"])
	if GameCheck.IsPST():
		strrefs = [ 1403, 4256, 4255, 4251, 4252, 4254, 4279 ]
	elif GameCheck.IsGemRBDemo ():
		strrefs = [ 84, 105, 106, 104, 107, item["DialogName"], 108 ]
	else:
		strrefs = [ 11973, 14133, 11960, 19392, 17104, item["DialogName"], 17108 ]

	# item name
	Label = Window.GetControl (0x10000000)
	if (itemtype & 2):
		text = item["ItemName"]
	else:
		text = item["ItemNameIdentified"]
	Label.SetText (text)

	#item icon
	Button = Window.GetControl (2)
	if GameCheck.IsPST():
		Button.SetFlags (IE_GUI_BUTTON_PICTURE | IE_GUI_BUTTON_NO_IMAGE, OP_SET)
		Button.SetItemIcon (slotItem["ItemResRef"])
	else:
		Button.SetFlags (IE_GUI_BUTTON_PICTURE, OP_OR)
		Button.SetItemIcon (slotItem["ItemResRef"], 0)
	Button.SetState (IE_GUI_BUTTON_LOCKED)

	#middle button
	Button = Window.GetControl (4)
	Button.SetText (strrefs[0])
	Button.OnPress (lambda: Window.Close())
	Button.MakeDefault()

	#textarea
	Text = Window.GetControl (5)
	if GameCheck.IsBG2OrEE (): # I believe only BG2 has special initials
		Text.SetColor (ColorWhitish, TA_COLOR_INITIALS)
	text = GetItemDescription (item, itemtype)
	Text.SetText (text)
	
	Window.SetEventProxy(Text)

	#left button
	Button = Window.GetControl(8)
	select = (itemtype & 1) and (item["Function"]&ITM_F_ABILITIES)
	pc = GemRB.GameGetSelectedPCSingle ()

	if itemtype & 2:
		Button.SetText (strrefs[1])
		Button.OnPress (lambda: IdentifyItemWindow(slotItem, pc))
		Button.SetFlags (IE_GUI_BUTTON_PICTURE, OP_SET)
	elif select and not GameCheck.IsPST():
		Button.SetText (strrefs[2])
		Button.OnPress (lambda: AbilitiesItemWindow(slotItem, pc))
		Button.SetFlags (IE_GUI_BUTTON_PICTURE, OP_SET)
	else:
		Button.SetText ("")
		Button.SetState (IE_GUI_BUTTON_LOCKED)
		Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_SET)
		Button.OnPress (None)

	# description icon (not present in iwds)
	if not GameCheck.IsIWD1() and not GameCheck.IsIWD2():
		Button = Window.GetControl (7)
		Button.SetFlags (IE_GUI_BUTTON_PICTURE | (0 if GameCheck.IsBG2EE() else IE_GUI_BUTTON_CENTER_PICTURES) | IE_GUI_BUTTON_NO_IMAGE, OP_OR)
		if GameCheck.IsPST():
			Button.SetItemIcon (slotItem["ItemResRef"], 1) # no DescIcon
		else:
			Button.SetItemIcon (slotItem["ItemResRef"], 2)
		Button.SetState (IE_GUI_BUTTON_LOCKED)

	#right button
	Button = Window.GetControl(9)
	Button.SetFlags (IE_GUI_BUTTON_PICTURE, OP_SET)
	drink = (itemtype & 1) and (item["Function"]&ITM_F_DRINK)
	read = (itemtype & 1) and (item["Function"]&ITM_F_READ)
	# only mages and some bards can learn spells (externalize this to clskills.2da?)
	ClassName = GUICommon.GetClassRowName (pc)
	if GameCheck.IsIWD2 ():
		if GemRB.GetPlayerStat (pc, IE_LEVELMAGE) == 0:
			read = 0
	else:
		if "MAGE" not in ClassName and "BARD" not in ClassName:
			read = 0
	# furthermore there might be school exclusions present
	if read:
		read = GemRB.CanUseItemType (SLOT_ALL, slotItem['ItemResRef'], pc, False)
	container = (itemtype & 1) and (item["Function"]&ITM_F_CONTAINER)
	dialog = (itemtype & 1) and (item["Dialog"]!="" and item["Dialog"]!="*")
	familiar = (itemtype & 1) and (item["Type"] == 38) and (slotItem["Flags"] & IE_INV_ITEM_UNDROPPABLE)

	# The "conversable" bit in PST actually means "usable", eg clot charm
	# unlike BG2 (which only has the bit set on SW2H14 Lilarcor)
	# Meanwhile IWD series has the bit set on important quest items
	# So the widely accepted name of this bit is misleading (see use of GFFlags::SELLABLE_CRITS_NO_CONV)

	# There are also items in PST, cube.itm and doll.itm 
	# that are not flagged usable but still have dialog attached.
	# this is how the original game draws the distinction between 'use item' and 'talk to item'

	# if the item has dialog and is flagged usable = use item string, open dialog
	# if the item has dialog and is not flagged usable = talk to item, open dialog
	# if the item has no dialog and is flagged usable = use item string, consume item

	if GameCheck.IsPST() and slotItem["Flags"] & IE_INV_ITEM_CONVERSABLE:

		drink = True # "Use"

	if drink and not dialog:
		# Standard consumable item
		Button.SetText (strrefs[3])
		Button.OnPress (ItemPress(ConsumeItem, slotItem, pc))
	elif read:
		Button.SetText (strrefs[4])
		Button.OnPress (ItemPress(ReadItemWindow, slotItem, pc))
	elif container:
		# Just skip the redundant info page and go directly to the container
		if GemRB.GetVar("GUIEnhancements")&GE_ALWAYS_OPEN_CONTAINER_ITEMS:
			OpenItemWindow(slotItem)
			Window.Close()
			return
		if GameCheck.IsIWD2() or GameCheck.IsHOW():
			Button.SetText (24891) # Open Container
		elif GameCheck.IsBG2OrEE ():
			Button.SetText (44002) # open container
		else:
			# a fallback, since the originals have nothing appropriate from not having any bags
			Button.SetText ("Open container")
		Button.OnPress (ItemPress(OpenItemWindow, slotItem))
	elif dialog:
		if drink:
			# Dialog item that is 'used'
			Button.SetText (strrefs[3])
		else:
			# Dialog item that is 'talked to'
			Button.SetText (strrefs[5])
		Button.OnPress (ItemPress(DialogItemWindow, slotItem, pc))
	elif familiar and not GameCheck.IsPST():
		# PST earings share a type with familiars, so no
		# mods that allow familiars would be possible in PST
		Button.SetText (4373)
		Button.OnPress (ItemPress(ReleaseFamiliar, slotItem, pc))
	else:
		Button.SetState (IE_GUI_BUTTON_LOCKED)
		Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_SET)
		Button.SetText ("")
		Button.OnPress (None)

	Label = Window.GetControl(0x1000000b)
	if Label:
		if (itemtype & 2):
			# NOT IDENTIFIED
			Label.SetText (strrefs[6])
		else:
			Label.SetText ("")

	# in pst one can cycle through all the items from the description window
	if GameCheck.IsPST():
		#left scroll
		Button = Window.GetControl (13)
		Button.OnPress (lambda: CycleDisplayItem(slotItem["Slot"], pc, -1))
		Button.SetHotKey (GEM_LEFT, 0, True)

		#right scroll
		Button = Window.GetControl (14)
		Button.OnPress (lambda: CycleDisplayItem(slotItem["Slot"], pc, 1))
		Button.SetHotKey (GEM_RIGHT, 0, True)

	Window.ShowModal(MODAL_SHADOW_GRAY)
	return

def CycleDisplayItem(slot, pc, direction):
	#try the next slot for an item. if the slot is empty, loop until one is found.
	slot_item = None
	while not slot_item:
		slot += direction

		#wrap around if last slot is reached
		if slot > 53:
			slot = 0
		elif slot < 0:
			slot = 53

		slot_item = GemRB.GetSlotItem (pc, slot)

	if slot_item:
		OpenItemInfoWindow(slot_item, pc)

def OpenItemInfoWindow (slotItem, pc):
	slotType = GemRB.GetSlotType (slotItem["Slot"], pc)

	# PST: if the slot is empty but is also the first quick weapon slot, display the info for the "default" weapon
	if GameCheck.IsPST() and slotItem is None and slotType["ID"] == 10 and GemRB.GetEquippedQuickSlot(pc) == 10:
		DisplayItem (GemRB.GetSlotItem (pc, 0))
		return

	item = GemRB.GetItem (slotItem["ItemResRef"])

	TryAutoIdentification(pc, item, slotItem["Slot"], slotItem, True, True)
	DisplayItem (slotItem)
	return

def MarkAsIdentified(pc, slot, slotItem):
	global IdentifiedItems

	GemRB.ChangeItemFlag (pc, slot, IE_INV_ITEM_IDENTIFIED, OP_OR)
	slotItem["Flags"] |= IE_INV_ITEM_IDENTIFIED
	IdentifiedItems.add(slotItem["ItemResRef"].upper())
	return

# auto identify when lore is high enough
def TryAutoIdentification(pc, item, slot, slot_item, enabled, feedback = False):
	if not enabled or slot_item["Flags"] & IE_INV_ITEM_IDENTIFIED:
		return False

	if GemRB.GetVar ("GUIEnhancements") & GE_PERSISTENT_IDENTIFICATION and slot_item["ItemResRef"].upper() in IdentifiedItems:
		MarkAsIdentified (pc, slot, slot_item)
		return True

	if not GameCheck.IsIWD2 ():
		# simple logic for other games
		if item["LoreToID"] <= GemRB.GetPlayerStat (pc, IE_LORE):
			MarkAsIdentified (pc, slot, slot_item)
			return True
		return False

	# iwd2 has extra hidden feedback strings
	# they're directly usable by the old-style python formatter, however our tokenizer
	# replaces the specifiers with fmt-compatible {}
	feedback = feedback and GemRB.GetVar ("EnableRollFeedback")
	# 1. try general knowledge arcana
	lore = GemRB.GetPlayerStat (pc, IE_LORE)
	intBon = GUICommon.GetAbilityBonus(pc, IE_INT) # already included in lore
	itemIdDC = item["LoreToID"]
	success = lore >= itemIdDC
	msgArea = GemRB.GetView ("MsgSys")
	if success:
		MarkAsIdentified (pc, slot, slot_item)
	# @39263 = ~Failed identify item check! (Knowledge Arcana + Int mod) %d + %d vs. (item's lore) %d~
	# @39264 = ~Successful identify item check! (Knowledge Arcana + Int mod) %d + %d vs. (item's lore) %d~
	if feedback:
		msgArea.Append (GemRB.GetString(39263 + success).format(lore, intBon, itemIdDC) + "\n")
	if success:
		return True

	# 2. try alchemy for potions
	if item["Type"] == 9:
		alchemy = GemRB.GetPlayerStat (pc, IE_ALCHEMY)
		success = alchemy >= itemIdDC
		if success:
			MarkAsIdentified (pc, slot, slot_item)
		# @39261 = ~Successful identify potion check! (Alchemy + Int mod) %d + %d vs. (potion's lore) %d~
		# @39262 = ~Failed identify potion check! (Alchemy + Int mod) %d + %d vs. (potion's lore) %d~
		if feedback:
			msgArea.Append (GemRB.GetString(39262 - success).format(alchemy, intBon, itemIdDC) + "\n")
		if success:
			return True

	# 3. try bardic lore
	bardLevel = GemRB.GetPlayerStat (pc, IE_LEVELBARD)
	if bardLevel > 0:
		success = (bardLevel + intBon) >= itemIdDC
		if success:
			MarkAsIdentified (pc, slot, slot_item)
		# @39259 = ~Failed identify item check! Check Bardic Lore %d vs. item's lore %d (%d Intelligence Ability Mod)~
		# @39260 = ~Successful identify item check! Check Bardic Lore %d vs. item's lore %d (%d Intelligence Ability Mod)~
		if feedback:
			msgArea.Append (GemRB.GetString(39259 + success).format(bardLevel, itemIdDC, intBon) + "\n")

	return success

def OpenGroundItemInfoWindow (btn):
	pc = GemRB.GameGetSelectedPCSingle ()
	slot = btn.Value
	slot_item = GemRB.GetContainerItem (pc, slot)

	DisplayItem(slot_item, True)
	return

def OpenGroundItemAmountWindow (btn):
	OpenItemAmountWindow (btn, "ground")

def ItemAmountWindowClosed(win):
	global ItemAmountWindow, UsedSlot

	ItemAmountWindow = None
	UsedSlot = None

def OpenItemAmountWindow (btn, location = "inventory"):
	"""Open the split window."""

	global UsedSlot, OverSlot
	global ItemAmountWindow, StackAmount

	pc = GemRB.GameGetSelectedPCSingle ()

	UsedSlot = btn.Value
	if GemRB.IsDraggingItem ()==1:
		GemRB.DropDraggedItem (pc, UsedSlot)
		#redraw slot
		UpdateSlot (pc, UsedSlot-1)
		# disallow splitting while holding split items (double splitting)
		if GemRB.IsDraggingItem () == 1:
			return

	if location == "inventory":
		slot_item = GemRB.GetSlotItem (pc, UsedSlot)
	else:
		slot_item = GemRB.GetContainerItem (pc, UsedSlot)

	item = GemRB.GetItem (slot_item['ItemResRef'])
	if item['MaxStackAmount'] <= 1: # is it stackable at all?
		return

	if slot_item:
		StackAmount = slot_item["Usages0"]
	else:
		StackAmount = 0
	if StackAmount<=1:
		UpdateSlot (pc, UsedSlot-1)
		return

	ItemAmountWindow = Window = GemRB.LoadWindow (4, "GUIINV")
	Window.SetFlags(WF_ALPHA_CHANNEL, OP_OR)
	Window.OnClose (ItemAmountWindowClosed)

	strings = { 'Done': 11973, "Cancel": 13727}
	if GameCheck.IsPST():
		strings = { 'Done': 1403, "Cancel": 4196}
	elif GameCheck.IsGemRBDemo ():
		strings = { 'Done': 84, 'Cancel': 103}

	# item icon
	Icon = Window.GetControl (0)
	Icon.SetFlags (IE_GUI_BUTTON_PICTURE | IE_GUI_BUTTON_NO_IMAGE, OP_SET)
	Icon.SetItemIcon (slot_item['ItemResRef'])

	# item amount
	Text = Window.GetControl (6)
	if GameCheck.IsIWD2():
		Text.SetSize (40, 40)
	Text.SetText (str (StackAmount//2))
	Text.SetFlags (IE_GUI_TEXTEDIT_ALPHACHARS, OP_NAND)
	Text.Focus()

	# Decrease
	Button = Window.GetControl (4)
	Button.OnPress (DecreaseStackAmount)
	Button.SetActionInterval (200)

	# Increase
	Button = Window.GetControl (3)
	Button.OnPress (IncreaseStackAmount)
	Button.SetActionInterval (200)

	# Done
	Button = Window.GetControl (2)
	Button.SetText (strings['Done'])
	Button.OnPress (lambda: DragItemAmount(slot_item, location))
	Button.MakeDefault()

	# Cancel
	Button = Window.GetControl (1)
	Button.SetText (strings['Cancel'])
	Button.OnPress (Window.Close)
	Button.MakeEscape()

	Window.ShowModal (MODAL_SHADOW_GRAY)
	return

def UpdateSlot (pc, slot):
	"""Updates a specific slot."""

	Window = GemRB.GetView("WIN_INV")

	using_fists = slot_item = SlotType = None

	if GameCheck.IsPST():
		if slot >= len(SlotMap):
			#this prevents accidental out of range errors from the avslots list
			return GemRB.GetSlotType (slot+1)
		elif SlotMap[slot] == -1:
			# This decides which icon to display in the empty slot
			# NOTE: there are invisible items (e.g. MORTEP) in inaccessible slots
			# used to assign powers and protections

			SlotType = GemRB.GetSlotType (slot+1, pc)
			slot_item = None
		else:
			SlotType = GemRB.GetSlotType (SlotMap[slot]+1)
			slot_item = GemRB.GetSlotItem (pc, SlotMap[slot]+1)
			#PST displays the default weapon in the first slot if nothing else was equipped
			if slot_item is None and SlotType["ID"] == 10 and GemRB.GetEquippedQuickSlot(pc) == 10:
				slot_item = GemRB.GetSlotItem (pc, 0)
				using_fists = 1
	else:
		SlotType = GemRB.GetSlotType (slot+1, pc)
		slot_item = GemRB.GetSlotItem (pc, slot+1)

	ControlID = SlotType["ID"]

	if ControlID == -1:
		return None

	if GemRB.IsDraggingItem ()==1:
		#get dragged item
		drag_item = GemRB.GetSlotItem (0,0)
		itemname = drag_item["ItemResRef"]
	else:
		itemname = ""

	Button = Window.GetControl (ControlID)

	# It is important to check for a control - some games use CID 0 for slots with no control, others -1
	if not Button:
		return

	Button.SetAction (OnDragItem, IE_ACT_DRAG_DROP_DST)
	Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_NAND)

	# characters should auto-identify any item they receive
	if slot_item:
		item = GemRB.GetItem (slot_item["ItemResRef"])
		TryAutoIdentification(pc, item, slot+1, slot_item, GemRB.GetVar("GUIEnhancements")&GE_TRY_IDENTIFY_ON_TRANSFER)

	UpdateInventorySlot (pc, Button, slot_item, "inventory", SlotType["Type"]&SLOT_INVENTORY == 0)

	if slot_item:
		Button.SetAction(OnDragItem, IE_ACT_DRAG_DROP_CRT)
		Button.OnPress (OnDragItem)
		Button.OnRightPress (lambda: OpenItemInfoWindow (slot_item, pc))
		Button.OnShiftPress (OpenItemAmountWindow)
		#If the slot is being used to display the 'default' weapon, disable dragging.
		if SlotType["ID"] == 10 and using_fists:
			Button.OnPress (None)
			#dropping is ok, because it will drop in the quick weapon slot and not the default weapon slot.
		# pst shows the inventory-bag icon bg behind items
		if GameCheck.IsPST ():
			Button.SetSprites ("IVSLOT", 0, 0, 0, 0, 0)
		elif GameCheck.IsIWD2 () and ControlID in [15, 16, 17]:
			# perhaps we should do it for all, but so far only the quiver was identified as problematic
			Button.SetSprites ("STONSLOT", 3 + (ControlID - 15), 0, 1, 2, 3)
	else:
		if SlotType["ResRef"]=="*":
			Button.SetBAM ("",0,0)
			Button.SetTooltip (SlotType["Tip"])
			itemname = ""
		elif SlotType["ResRef"]=="":
			Button.SetBAM ("",0,0)
			Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_OR)
			Button.SetTooltip ("")
			itemname = ""
		else:
			if SlotType["Flags"] & 2:
				Button.SetPicture (SlotType["ResRef"])
			else:
				Button.SetBAM (SlotType["ResRef"], 0, 0)
			Button.SetTooltip (SlotType["Tip"])

		if SlotMap and SlotMap[slot]<0:
			Button.SetBAM ("",0,0)
			Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_OR)
			Button.SetTooltip ("")
			itemname = ""

		Button.OnPress (None)
		Button.OnRightPress (None)
		Button.OnShiftPress (None)
		Button.OnDoublePress (OpenItemAmountWindow)
	if not slot_item:
		Button.SetState (IE_GUI_BUTTON_LOCKED)
	elif (SlotType["Type"]&SLOT_INVENTORY) or not GemRB.CanUseItemType (SlotType["Type"], itemname):
		Button.SetState (IE_GUI_BUTTON_ENABLED)
	else:
		Button.SetState (IE_GUI_BUTTON_FAKEPRESSED)

	# highlight equipped weapons and ammo
	if slot_item and (GemRB.GetEquippedQuickSlot (pc)==slot+1 or GemRB.GetEquippedAmmunition (pc)==slot+1):
		Button.SetState (IE_GUI_BUTTON_FAKEDISABLED)

	return SlotType

def CancelColor():
	global ColorPicker
	if ColorPicker:
		ColorPicker.Close ()
	InventoryWindow = GemRB.GetView ("WIN_INV")
	InventoryWindow.Focus ()
	return

def ColorDonePress():
	"""Saves the selected colors."""

	pc = GemRB.GameGetSelectedPCSingle ()

	if ColorPicker:
		ColorPicker.Close ()

	ColorTable = GemRB.LoadTable ("clowncol")
	PickedColor=ColorTable.GetValue (ColorIndex, GemRB.GetVar ("Selected"))
	if ColorIndex==0:
		GUICommon.SetColorStat (pc, IE_HAIR_COLOR, PickedColor)
	elif ColorIndex==1:
		GUICommon.SetColorStat (pc, IE_SKIN_COLOR, PickedColor)
	elif ColorIndex==2:
		GUICommon.SetColorStat (pc, IE_MAJOR_COLOR, PickedColor)
	else:
		GUICommon.SetColorStat (pc, IE_MINOR_COLOR, PickedColor)
	return

def HairPress():
	global ColorIndex, PickedColor

	pc = GemRB.GameGetSelectedPCSingle ()
	ColorIndex = 0
	PickedColor = GemRB.GetPlayerStat (pc, IE_HAIR_COLOR, 1) & 0xFF
	GetColor()
	return

def SkinPress():
	global ColorIndex, PickedColor

	pc = GemRB.GameGetSelectedPCSingle ()
	ColorIndex = 1
	PickedColor = GemRB.GetPlayerStat (pc, IE_SKIN_COLOR, 1) & 0xFF
	GetColor()
	return

def MajorPress():
	"""Selects the major color."""
	global ColorIndex, PickedColor

	pc = GemRB.GameGetSelectedPCSingle ()
	ColorIndex = 2
	PickedColor = GemRB.GetPlayerStat (pc, IE_MAJOR_COLOR, 1) & 0xFF
	GetColor()
	return

def MinorPress():
	"""Selects the minor color."""
	global ColorIndex, PickedColor

	pc = GemRB.GameGetSelectedPCSingle ()
	ColorIndex = 3
	PickedColor = GemRB.GetPlayerStat (pc, IE_MINOR_COLOR, 1) & 0xFF
	GetColor()
	return

def GetColor():
	"""Opens the color selection window."""

	global ColorPicker

	ColorTable = GemRB.LoadTable ("clowncol")
	InventoryWindow = GemRB.GetView ("WIN_INV")
	InventoryWindow.SetDisabled (True) #darken it
	ColorPicker = GemRB.LoadWindow (3, "GUIINV")
	GemRB.SetVar ("Selected", None)
	if GameCheck.IsIWD2 () or GameCheck.IsGemRBDemo ():
		Button = ColorPicker.GetControl (35)
		Button.OnPress (CancelColor)
		Button.SetText (103)
		if GameCheck.IsIWD2 ():
			Button.SetText (13727)

	for i in range (34):
		Button = ColorPicker.GetControl (i)
		MyColor = ColorTable.GetValue (ColorIndex, i)
		if MyColor == "*":
			Button.SetState (IE_GUI_BUTTON_LOCKED)
			continue
		if PickedColor == MyColor:
			GemRB.SetVar ("Selected",i)
			Button.SetState (IE_GUI_BUTTON_LOCKED)
			Button.MakeEscape()
		else:
			Button.SetBAM ("COLGRAD", 2, 0, MyColor)
			Button.SetFlags (IE_GUI_BUTTON_PICTURE|IE_GUI_BUTTON_RADIOBUTTON, OP_OR)
			Button.SetState (IE_GUI_BUTTON_ENABLED)
		Button.SetVarAssoc ("Selected",i)
		Button.OnPress (ColorDonePress)
	ColorPicker.Focus()
	return

def ReleaseFamiliar (item, pc):
	"""Simple Use Item"""

	# the header is always the first, target is always self
	GemRB.UseItem (pc, item["Slot"], 0, 5)
	return

def ConsumeItem (item, pc):
	"""Drink the potion"""

	# the drink item header is always the first
	# pst also requires forcing the target (eg. clot charms), which doesn't hurt elsewhere
	GemRB.UseItem (pc, item["Slot"], 0, 5)
	return

def OpenErrorWindow (strref):
	"""Opens the error window and displays the string."""

	Window = GemRB.LoadWindow (7, "GUIINV")
	Button = Window.GetControl (0)
	if GameCheck.IsPST():
		Button.SetText (1403)
	else:
		Button.SetText (11973)
	Button.OnPress (lambda: Window.Close())
	Button.MakeDefault()

	TextArea = Window.GetControl (3)
	TextArea.SetText (strref)
	Window.ShowModal (MODAL_SHADOW_GRAY)
	return

def ReadItemWindow (item, pc):
	"""Tries to learn the mage scroll."""

	ret = Spellbook.CannotLearnSlotSpell(item, pc)
	if ret:
		# these failures are soft - the scroll is not destroyed
		if ret == LSR_KNOWN and GameCheck.HasTOB():
			strref = 72873
		elif ret == LSR_KNOWN and GameCheck.IsPST():
			strref = 50394
		elif ret == LSR_STAT and GameCheck.HasTOB():
			# reached for sorcerers, schools/usability is handled before
			strref = 72874 # your spell school does not permit you to learn
		elif ret == LSR_LEVEL and GameCheck.HasTOB():
			strref = 48806 # inadequate intelligence (good enough approximation)
		elif ret == LSR_FULL and GameCheck.IsBG2OrEE ():
			strref = 32097
		elif ret == LSR_FULL and GameCheck.IsPST():
			strref = 50395
		elif GameCheck.IsPST():
			strref = 4250
		else:
			strref = 10831

		GemRB.PlaySound ("EFF_M10") # failure!
		OpenErrorWindow (strref)
		return

	# we already checked for most failures, but we can still fail with bad % rolls vs intelligence
	ret = Spellbook.LearnFromScroll (pc, item["Slot"])
	if ret == LSR_OK:
		GemRB.PlaySound ("GAM_44") # success!
		if GameCheck.IsPST():
			strref = GetPSTPersonalizedRef (pc, 35645) # 4249 - "You succeeded in learning this spell!" / 35645 - "Spell has been copied"
			GemRB.GetString (strref, 2) # play the attached sound
		else:
			strref = 10830
	else:
		GemRB.PlaySound ("EFF_M10") # failure!
		if GameCheck.IsPST():
			strref = GetPSTPersonalizedRef (pc, 35629) # 4250 - "You failed to learn this spell!", but 35629 - "Spell has failed to have been copied" has sounds
			GemRB.GetString (strref, 2) # play the attached sound
		else:
			strref = 10831

	OpenErrorWindow (strref)

def GetPSTPersonalizedRef(pc, baseRef):
	"""Calculate strref based on base strref and party member"""

	# figure out PC index (TNO, Morte, Annah, Dakkon, FFG, Nordom, Ignus, Vhailor)
	# the original compared script names, but specifics should be fine
	spec2offset = [ 0, 7, 5, 6, 4, 3, 2, 1 ]
	pcOffset = spec2offset[GemRB.GetPlayerStat (pc, IE_SPECIFIC) - 2]
	return baseRef + pcOffset

def OpenItemWindow (slot_item):
	"""Displays information about the item."""

	ResRef = slot_item['ItemResRef']
	GemRB.EnterStore (ResRef)
	return

def DialogItemWindow (slot_item, pc):
	"""Converse with an item."""

	ResRef = slot_item['ItemResRef']
	item = GemRB.GetItem (ResRef)
	dialog=item["Dialog"]

	GemRB.ExecuteString ("StartDialogOverride(\""+dialog+"\",Myself,0,0,1)", pc)
	return

def IdentifyUseSpell (slotItem, pc):
	"""Identifies the item with a memorized spell."""

	GemRB.HasSpecialSpell (pc, SP_IDENTIFY, 1)
	MarkAsIdentified (pc, slotItem["Slot"], slotItem)
	if GameCheck.IsPST ():
		strRef = GetPSTPersonalizedRef (pc, 35685)
		GemRB.GetString (strRef, 2) # play the attached sound
	else:
		GemRB.PlaySound (DEF_IDENTIFY)
	OpenItemInfoWindow(slotItem, pc)
	return

def IdentifyUseScroll (slotItem, pc):
	"""Identifies the item with a scroll or other item."""

	if GemRB.HasSpecialItem (pc, 1, 1):
		MarkAsIdentified (pc, slotItem["Slot"], slotItem)
	if GameCheck.IsPST ():
		strRef = GetPSTPersonalizedRef (pc, 35685)
		GemRB.GetString (strRef, 2) # play the attached sound
	else:
		GemRB.PlaySound (DEF_IDENTIFY)
	OpenItemInfoWindow(slotItem, pc)
	return

def IdentifyItemWindow (slotItem, pc):
	Window = GemRB.LoadWindow (9, "GUIINV")
	Button = Window.GetControl (0)
	if GameCheck.IsPST():
		Button.SetText (4259)
	else:
		Button.SetText (17105)
	Button.OnPress (ItemPress(IdentifyUseSpell, slotItem, pc))
	if not GemRB.HasSpecialSpell (pc, SP_IDENTIFY, 0):
		Button.SetState (IE_GUI_BUTTON_DISABLED)

	Button = Window.GetControl (1)
	if GameCheck.IsPST():
		Button.SetText (4260)
	else:
		Button.SetText (17106)
	Button.OnPress (ItemPress(IdentifyUseScroll, slotItem, pc))
	if not GemRB.HasSpecialItem (pc, 1, 0):
		Button.SetState (IE_GUI_BUTTON_DISABLED)

	Button = Window.GetControl (2)
	if GameCheck.IsPST():
		Button.SetText (4196)
	else:
		Button.SetText (13727)
	Button.OnPress (lambda: Window.Close())
	Button.MakeEscape()

	TextArea = Window.GetControl (3)
	if GameCheck.IsPST():
		TextArea.SetText (4258)
	else:
		TextArea.SetText (19394)
	Window.ShowModal (MODAL_SHADOW_GRAY)
	return

def DoneAbilitiesItemWindow (slot_item, pc):
	GemRB.SetupQuickItemSlot (pc, 0, slot_item["Slot"], GemRB.GetVar ("Ability"))
	return

def AbilitiesItemWindow (slot_item, pc):
	Window = GemRB.LoadWindow (6, "GUIINV")

	item = GemRB.GetItem (slot_item["ItemResRef"])
	Tips = item["Tooltips"]

	GemRB.SetVar ("Ability", slot_item["Header"])
	for i in range(3):
		Button = Window.GetControl (i+1)
		if GameCheck.IsBG2OrEE ():
			Button.SetSprites ("GUIBTBUT",i,0,1,2,0)
		Button.SetFlags (IE_GUI_BUTTON_RADIOBUTTON, OP_OR)
		Button.SetVarAssoc ("Ability",i)
		Text = Window.GetControl (i+0x10000003)
		if i<len(Tips):
			Button.SetItemIcon (slot_item['ItemResRef'],i+6)
			Text.SetText (Tips[i])
		else:
			#disable button
			Button.SetItemIcon ("",3)
			Text.SetText ("")
			Button.SetState (IE_GUI_BUTTON_DISABLED)

	TextArea = Window.GetControl (8)
	TextArea.SetText (11322)

	Button = Window.GetControl (7)
	Button.SetText (11973)
	Button.OnPress (ItemPress(DoneAbilitiesItemWindow, slot_item, pc))
	Button.MakeDefault()

	Button = Window.GetControl (10)
	Button.SetText (13727)
	Button.OnPress (lambda: Window.Close())
	Button.MakeEscape()
	Window.ShowModal (MODAL_SHADOW_GRAY)
	return

def UpdateInventorySlot (pc, Button, Slot, Type, Equipped=False):
	Button.SetFont ("NUMBER")

	color = {'r' : 128, 'g' : 128, 'b' : 255, 'a' : 64}
	Button.SetBorder (0, color, 0,1)
	colorUnusable = {'r' : 255, 'g' : 128, 'b' : 128, 'a' : 64}
	Button.SetBorder (2, colorUnusable, 0, 1)
	colorUMD = {'r' : 255, 'g' : 255, 'b' : 0, 'a' : 64}
	colorGreen =  {'r' : 0, 'g' : 255, 'b' : 0, 'a' : 64}

	Button.SetText ("")
	GUICommon.SetButtonAnchor (Button)
	Button.SetFlags (IE_GUI_BUTTON_ALIGN_RIGHT | IE_GUI_BUTTON_ALIGN_BOTTOM | IE_GUI_BUTTON_PICTURE, OP_OR)

	if Slot == None:
		Button.SetFlags (IE_GUI_BUTTON_PICTURE, OP_NAND)
		tooltips = { "inventory": 12013, "ground": 12011, "container": "" }
		if GameCheck.IsGemRBDemo ():
			tooltips = { "inventory": 82, "ground": 83, "container": "" }
		Button.SetTooltip (tooltips[Type])
		Button.EnableBorder (0, 0)
		Button.EnableBorder (1, 0)
		Button.EnableBorder (2, 0)
		return

	item = GemRB.GetItem (Slot['ItemResRef'])
	identified = Slot["Flags"] & IE_INV_ITEM_IDENTIFIED
	magical = item["Enchantment"] > 0

	# gemrb extension based on ImprovedGUI - mark scrolls that the pc hasn't learned yet
	scroll = False
	knowsScroll = False
	if Slot["Flags"] & IE_INV_ITEM_IDENTIFIED and GemRB.GetVar ("GUIEnhancements") & GE_MARK_SCROLLS:
		scroll = item["Function"] & ITM_F_READ
	if scroll and not Spellbook.HasSorcererBook (pc):
		knowsScroll = Spellbook.CannotLearnSlotSpell (Slot, pc) == LSR_KNOWN

	# MaxStackAmount holds the *maximum* item count in the stack while Usages0 holds the actual
	if item["MaxStackAmount"] > 1:
		Button.SetText (str (Slot["Usages0"]))

	# auto-identify mundane items; the actual identification will happen on transfer
	if not identified and item["LoreToID"] == 0:
		identified = True

	if not identified or item["ItemNameIdentified"] == -1:
		Button.SetTooltip (item["ItemName"])
		Button.EnableBorder (0, 1)
	else:
		Button.SetTooltip (item["ItemNameIdentified"])
		Button.EnableBorder (0, 0)

	usable = GemRB.CanUseItemType (SLOT_ALL, Slot['ItemResRef'], pc, Equipped)
	if usable:
		# enable yellow overlay for "use magical device"
		if usable & 0x100000:
			Button.SetBorder (2, colorUMD, 1, 1)
			# unset green border
			Button.EnableBorder (1, 0)
		elif scroll and not knowsScroll:
			# set green border
			Button.SetBorder (1, colorGreen, 1)
		else:
			Button.EnableBorder (2, 0)
			# unset green border
			Button.EnableBorder (1, 0)
	else:
		Button.SetBorder (2, colorUnusable, 1, 1)
		Button.EnableBorder (1, 0)

	if magical and GameCheck.IsIWD2 ():
		Button.SetFlags (IE_GUI_BUTTON_HORIZONTAL, OP_OR)
		Button.SetItemIcon (Slot['ItemResRef'], 0, 0, 0, "", "STORTIN3")
	else:
		Button.SetFlags (IE_GUI_BUTTON_HORIZONTAL, OP_NAND)
		Button.SetItemIcon (Slot['ItemResRef'], 0)
	return
