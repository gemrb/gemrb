# -*-python-*-
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

import GemRB
import GameCheck
import GUICommon
import Spellbook
from GUIDefines import *
from ie_stats import *
from ie_slots import *
from ie_spells import *
from ie_sounds import DEF_IDENTIFY

UsedSlot = None
ItemInfoWindow = None
ItemAmountWindow = None
ItemIdentifyWindow = None
ItemAbilitiesWindow = None
ErrorWindow = None
ColorPicker = None
StackAmount = 0

UpdateInventoryWindow = None

def OnDragItemGround (btn, slot):
	"""Drops and item to the ground."""
	
	pc = GemRB.GameGetSelectedPCSingle ()
	slot = slot + GemRB.GetVar ("TopIndex") - btn.ControlID

	if GemRB.IsDraggingItem ()==0:
		slot_item = GemRB.GetContainerItem (pc, slot)
		item = GemRB.GetItem (slot_item["ItemResRef"])
		GemRB.DragItem (pc, slot, item["ItemIcon"], 0, 1) #container
		if GameCheck.IsPST():
			GemRB.PlaySound (item["DescIcon"])
	else:
		GemRB.DropDraggedItem (pc, -2) #dropping on ground

	UpdateInventoryWindow ()
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

	UpdateInventoryWindow ()
	return

def OnDragItem (btn, slot):
	"""Updates dragging."""

	#don't call when splitting items
	if ItemAmountWindow != None:
		return

	pc = GemRB.GameGetSelectedPCSingle ()
	slot_item = GemRB.GetSlotItem (pc, slot)
	
	if not GemRB.IsDraggingItem ():
		item = GemRB.GetItem (slot_item["ItemResRef"])
		GemRB.DragItem (pc, slot, item["ItemIcon"], 0, 0)
	else:
		SlotType = GemRB.GetSlotType (slot, pc)
		#special monk check
		if GemRB.GetPlayerStat (pc, IE_CLASS)==0x14:
			#offhand slot mark
			if SlotType["Effects"]==TYPE_OFFHAND:
				SlotType["ResRef"]=""
				GemRB.DisplayString (61355, 0xffffff)

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
						GemRB.DisplayString(msg, 0xffffff)
					#leave (save) store
					GemRB.LeaveStore()

			GemRB.DropDraggedItem (pc, slot)
			# drop item if it caused us to disable the inventory view (example: cursed berserking sword)
			if GemRB.GetPlayerStat (pc, IE_STATE_ID) & (STATE_BERSERK) and GemRB.IsDraggingItem ():
				GemRB.DropDraggedItem (pc, -3)

	UpdateInventoryWindow ()
	return

def OnDropItemToPC (pc):
	"""Gives an item to another character."""

	if pc > GemRB.GetPartySize ():
		return
	#-3 : drop stuff in inventory (but not equippable slots)
	GemRB.DropDraggedItem (pc, -3)
	UpdateInventoryWindow ()
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

def DragItemAmount ():
	"""Drag a split item."""

	pc = GemRB.GameGetSelectedPCSingle ()

	#emergency dropping
	if GemRB.IsDraggingItem()==1:
		GemRB.DropDraggedItem (pc, UsedSlot)
		UpdateSlot (pc, UsedSlot-1)

	slot_item = GemRB.GetSlotItem (pc, UsedSlot)

	#if dropping didn't help, don't die if slot_item isn't here
	if slot_item:
		Text = ItemAmountWindow.GetControl (6)
		Amount = Text.QueryText ()
		item = GemRB.GetItem (slot_item["ItemResRef"])
		GemRB.DragItem (pc, UsedSlot, item["ItemIcon"], int ("0"+Amount), 0)
	ItemAmountWindow.Close()
	return

def MouseEnterSlot (btn, slot):
	pc = GemRB.GameGetSelectedPCSingle ()

	if GemRB.IsDraggingItem ()==1:
		drag_item = GemRB.GetSlotItem (0,0)
		SlotType = UpdateSlot (pc, slot-1)

		if GemRB.CanUseItemType (SlotType["Type"], drag_item["ItemResRef"]):
			btn.SetState (IE_GUI_BUTTON_SELECTED)
		else:
			btn.SetState (IE_GUI_BUTTON_ENABLED)
		
	return

def MouseLeaveSlot (btn, slot):
	pc = GemRB.GameGetSelectedPCSingle ()

	UpdateSlot (pc, slot-1)
	return

def MouseEnterGround (Button):
	if GemRB.IsDraggingItem ()==1:
		Button.SetState (IE_GUI_BUTTON_SELECTED)
	return

def MouseLeaveGround (Button):
	if GemRB.IsDraggingItem ()==1:
		Button.SetState (IE_GUI_BUTTON_FAKEPRESSED)
	return

def CloseItemInfoWindow ():
	if ItemInfoWindow:
		ItemInfoWindow.Unload ()
	UpdateInventoryWindow ()
	return

def DisplayItem (slotItem, type):
	global ItemInfoWindow

	item = GemRB.GetItem (slotItem["ItemResRef"])
	ItemInfoWindow = Window = GemRB.LoadWindow (5)

	if GameCheck.IsPST():
		strrefs = [ 1403, 4256, 4255, 4251, 4252, 4254, 4279 ]
	else:
		strrefs = [ 11973, 14133, 11960, 19392, 17104, item["DialogName"], 17108 ]

	# item name
	Label = Window.GetControl (0x10000000)
	if (type&2):
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
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, CloseItemInfoWindow)
	Button.MakeDefault()

	#textarea
	Text = Window.GetControl (5)
	if (type&2):
		text = item["ItemDesc"]
	else:
		text = item["ItemDescIdentified"]

	Text.Clear ()
	Text.Append (text)
	
	Window.SetEventProxy(Text)

	#left button
	Button = Window.GetControl(8)
	select = (type&1) and (item["Function"]&ITM_F_ABILITIES)

	if type&2:
		Button.SetText (strrefs[1])
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, IdentifyItemWindow)
	elif select and not GameCheck.IsPST():
		Button.SetText (strrefs[2])
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, AbilitiesItemWindow)
	else:
		Button.SetState (IE_GUI_BUTTON_LOCKED)
		Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_SET)

	# description icon (not present in iwds)
	if not GameCheck.IsIWD1() and not GameCheck.IsIWD2():
		Button = Window.GetControl (7)
		Button.SetFlags (IE_GUI_BUTTON_PICTURE | IE_GUI_BUTTON_CENTER_PICTURES | IE_GUI_BUTTON_NO_IMAGE, OP_OR)
		if GameCheck.IsPST():
			Button.SetItemIcon (slotItem["ItemResRef"], 1) # no DescIcon
		else:
			Button.SetItemIcon (slotItem["ItemResRef"], 2)
		Button.SetState (IE_GUI_BUTTON_LOCKED)

	#right button
	Button = Window.GetControl(9)
	drink = (type&1) and (item["Function"]&ITM_F_DRINK)
	read = (type&1) and (item["Function"]&ITM_F_READ)
	# sorcerers cannot learn spells
	pc = GemRB.GameGetSelectedPCSingle ()
	if Spellbook.HasSorcererBook (pc):
		read = 0
	container = (type&1) and (item["Function"]&ITM_F_CONTAINER)
	dialog = (type&1) and (item["Dialog"]!="" and item["Dialog"]!="*")
	familiar = (type&1) and (item["Type"] == 38)
	# perhaps it's true everywhere, but it's definitely needed in pst
	# and yes, the check is reversed, so the bit name is a misnomer in this case
	if GameCheck.IsPST() and slotItem["Flags"] & IE_INV_ITEM_CONVERSABLE:
		dialog = False
		drink = True # "Use"

	if drink:
		Button.SetText (strrefs[3])
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, ConsumeItem)
	elif read:
		Button.SetText (strrefs[4])
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, ReadItemWindow)
	elif container:
		# Just skip the redundant info page and go directly to the container
		if GemRB.GetVar("GUIEnhancements")&GE_ALWAYS_OPEN_CONTAINER_ITEMS:
			OpenItemWindow()
			return
		if GameCheck.IsIWD2() or GameCheck.IsHOW():
			Button.SetText (24891) # Open Container
		elif GameCheck.IsBG2():
			Button.SetText (44002) # open container
		else:
			# a fallback, since the originals have nothing appropriate
			# FIXME: where do mods add the new string? This is untranslatable
			Button.SetText ("Open container")
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, OpenItemWindow)
	elif dialog:
		Button.SetText (strrefs[5])
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, DialogItemWindow)
	elif familiar and not GameCheck.IsPST():
		# PST earings share a type with familiars, so no
		# mods that allow familiars would be possible in PST
		Button.SetText (4373)
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, ReleaseFamiliar)
	else:
		Button.SetState (IE_GUI_BUTTON_LOCKED)
		Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_SET)

	Label = Window.GetControl(0x1000000b)
	if Label:
		if (type&2):
			# NOT IDENTIFIED
			Label.SetText (strrefs[6])
		else:
			Label.SetText ("")

	# in pst one can cycle through all the items from the description window
	if GameCheck.IsPST():
		def DisplayNext(dir):
			import GUIINV

			id = GemRB.GetVar('ItemButton') + dir;
			while id not in GUIINV.ItemHash or not GUIINV.ItemHash[id][1]:
				id += dir;
				if id < 0:
					id = 43
				elif id > 43:
					id = 0

			GemRB.SetVar('ItemButton', id)
			CloseItemInfoWindow()
			OpenItemInfoWindow(None, GUIINV.ItemHash[id][0])

		#left scroll
		Button = Window.GetControl (13)
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, lambda: DisplayNext(-1))

		#right scroll
		Button = Window.GetControl (14)
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, lambda: DisplayNext(1))

	ItemInfoWindow.ShowModal(MODAL_SHADOW_GRAY)
	return

def OpenItemInfoWindow (btn, slot):
	pc = GemRB.GameGetSelectedPCSingle ()

	slot_item = GemRB.GetSlotItem (pc, slot)
	item = GemRB.GetItem (slot_item["ItemResRef"])

	if TryAutoIdentification(pc, item, slot, slot_item, True):
		UpdateInventoryWindow ()

	if slot_item["Flags"] & IE_INV_ITEM_IDENTIFIED:
		value = 1
	else:
		value = 3
	DisplayItem (slot_item, value)
	return

#auto identify when lore is high enough
def TryAutoIdentification(pc, item, slot, slot_item, enabled=0):
	if enabled and item["LoreToID"]<=GemRB.GetPlayerStat (pc, IE_LORE):
		GemRB.ChangeItemFlag (pc, slot, IE_INV_ITEM_IDENTIFIED, OP_OR)
		slot_item["Flags"] |= IE_INV_ITEM_IDENTIFIED
		return True
	return False

def OpenGroundItemInfoWindow (btn, slot):
	global ItemInfoWindow

	pc = GemRB.GameGetSelectedPCSingle ()
	slot = slot + GemRB.GetVar("TopIndex") - btn.ControlID
	slot_item = GemRB.GetContainerItem (pc, slot)

	#the ground items are only displayable
	if slot_item["Flags"] & IE_INV_ITEM_IDENTIFIED:
		value = 0
	else:
		value = 2
	DisplayItem(slot_item, value)
	return

# TODO: implement, reuse OpenItemAmountWindow, but be careful about any other uses of ItemButton
def OpenGroundItemAmountWindow ():
	pass
	
def ItemAmountWindowClosed(win):
	global ItemAmountWindow, UsedSlot

	ItemAmountWindow = None
	UsedSlot = None
	UpdateInventoryWindow()

# TODO: can the GUISTORE be consolidate with this one?
def OpenItemAmountWindow (btn, slot):
	"""Open the split window."""

	global UsedSlot, OverSlot
	global ItemAmountWindow, StackAmount

	pc = GemRB.GameGetSelectedPCSingle ()

	UsedSlot = slot
	if GemRB.IsDraggingItem ()==1:
		GemRB.DropDraggedItem (pc, UsedSlot)
		#redraw slot
		UpdateSlot (pc, UsedSlot-1)
		# disallow splitting while holding split items (double splitting)
		if GemRB.IsDraggingItem () == 1:
			return

	slot_item = GemRB.GetSlotItem (pc, UsedSlot)

	if slot_item:
		StackAmount = slot_item["Usages0"]
	else:
		StackAmount = 0
	if StackAmount<=1:
		UpdateSlot (pc, UsedSlot-1)
		return

	ResRef = slot_item['ItemResRef']
	item = GemRB.GetItem (ResRef)

	ItemAmountWindow = Window = GemRB.LoadWindow (4)
	Window.SetFlags(WF_ALPHA_CHANNEL, OP_OR)
	Window.SetAction(ItemAmountWindowClosed, ACTION_WINDOW_CLOSED)

	strings = { 'Done': 11973, "Cancel": 13727}
	if GameCheck.IsPST():
		strings = { 'Done': 1403, "Cancel": 4196}

	# item icon
	Icon = Window.GetControl (0)
	Icon.SetFlags (IE_GUI_BUTTON_PICTURE | IE_GUI_BUTTON_NO_IMAGE, OP_SET)
	Icon.SetItemIcon (ResRef)

	# item amount
	Text = Window.GetControl (6)
	# FIXME: use a proper size
	# FIXME: fix it for all the games
	if GameCheck.IsIWD2():
		Text.SetSize (40, 40)
	Text.SetText (str (StackAmount//2))
	Text.SetStatus (IE_GUI_EDIT_NUMBER)
	Text.Focus()

	# Decrease
	Button = Window.GetControl (4)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, DecreaseStackAmount)
	Button.SetActionInterval (200)

	# Increase
	Button = Window.GetControl (3)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, IncreaseStackAmount)
	Button.SetActionInterval (200)

	# Done
	Button = Window.GetControl (2)
	Button.SetText (strings['Done'])
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, DragItemAmount)
	Button.MakeDefault()

	# Cancel
	Button = Window.GetControl (1)
	Button.SetText (strings['Cancel'])
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, lambda: Window.Close())
	Button.MakeEscape()

	Window.ShowModal (MODAL_SHADOW_GRAY)
	return

def UpdateSlot (pc, slot):
	"""Updates a specific slot."""

	Window = GemRB.GetView("WIN_INV")

	SlotType = GemRB.GetSlotType (slot+1, pc)
	ControlID = SlotType["ID"]

	if not ControlID:
		return None

	if GemRB.IsDraggingItem ()==1:
		#get dragged item
		drag_item = GemRB.GetSlotItem (0,0)
		itemname = drag_item["ItemResRef"]
		drag_item = GemRB.GetItem (itemname)
	else:
		itemname = ""

	Button = Window.GetControl (ControlID)
	slot_item = GemRB.GetSlotItem (pc, slot+1)

	Button.SetAction (OnDragItem, IE_ACT_DRAG_DROP_DST)
	Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_NAND)

	# characters should auto-identify any item they recieve
	if slot_item:
		item = GemRB.GetItem (slot_item["ItemResRef"])
		TryAutoIdentification(pc, item, slot+1, slot_item, GemRB.GetVar("GUIEnhancements")&GE_TRY_IDENTIFY_ON_TRANSFER)

	UpdateInventorySlot (pc, Button, slot_item, "inventory", SlotType["Type"]&SLOT_INVENTORY == 0)

	if slot_item:
		Button.SetAction(OnDragItem, IE_ACT_DRAG_DROP_CRT)
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, OnDragItem)
		Button.SetEvent (IE_GUI_BUTTON_ON_RIGHT_PRESS, OpenItemInfoWindow)
		Button.SetEvent (IE_GUI_BUTTON_ON_SHIFT_PRESS, OpenItemAmountWindow)
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
			Button.SetBAM (SlotType["ResRef"],0,0)
			Button.SetTooltip (SlotType["Tip"])

		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, None)
		Button.SetEvent (IE_GUI_BUTTON_ON_RIGHT_PRESS, None)
		Button.SetEvent (IE_GUI_BUTTON_ON_SHIFT_PRESS, None)
		Button.SetEvent (IE_GUI_BUTTON_ON_DOUBLE_PRESS, OpenItemAmountWindow)

	if (SlotType["Type"]&SLOT_INVENTORY) or not GemRB.CanUseItemType (SlotType["Type"], itemname):
		Button.SetState (IE_GUI_BUTTON_ENABLED)
	else:
		Button.SetState (IE_GUI_BUTTON_FAKEPRESSED)

	if slot_item and (GemRB.GetEquippedQuickSlot (pc)==slot+1 or GemRB.GetEquippedAmmunition (pc)==slot+1):
		Button.SetState (IE_GUI_BUTTON_FAKEDISABLED)

	return SlotType

def CancelColor():
	global ColorPicker
	if ColorPicker:
		ColorPicker.Unload ()
	InventoryWindow = GemRB.GetView ("WIN_INV")
	InventoryWindow.Focus ()
	return

def ColorDonePress():
	"""Saves the selected colors."""

	pc = GemRB.GameGetSelectedPCSingle ()

	if ColorPicker:
		ColorPicker.Unload ()

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
	UpdateInventoryWindow ()
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
	ColorPicker = GemRB.LoadWindow (3)
	GemRB.SetVar ("Selected",-1)
	if GameCheck.IsIWD2():
		Button = ColorPicker.GetControl (35)
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, CancelColor)
		Button.SetText(13727)

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
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, ColorDonePress)
	ColorPicker.Focus()
	return

def ReleaseFamiliar ():
	"""Simple Use Item"""

	pc = GemRB.GameGetSelectedPCSingle ()
	slot = GemRB.GetVar ("ItemButton")
	# the header is always the first, target is always self
	GemRB.UseItem (pc, slot, 0, 5)
	CloseItemInfoWindow ()
	return

def ConsumeItem ():
	"""Drink the potion"""

	pc = GemRB.GameGetSelectedPCSingle ()
	slot = GemRB.GetVar ("ItemButton")
	# the drink item header is always the first
	# pst also requires forcing the target (eg. clot charms), which doesn't hurt elsewhere
	GemRB.UseItem (pc, slot, 0, 5)
	CloseItemInfoWindow ()
	return

def OpenErrorWindow (strref):
	"""Opens the error window and displays the string."""

	global ErrorWindow
	pc = GemRB.GameGetSelectedPCSingle ()

	ErrorWindow = Window = GemRB.LoadWindow (7)
	Button = Window.GetControl (0)
	if GameCheck.IsPST():
		Button.SetText (1403)
	else:
		Button.SetText (11973)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, CloseErrorWindow)
	Button.MakeDefault()

	TextArea = Window.GetControl (3)
	TextArea.SetText (strref)
	Window.ShowModal (MODAL_SHADOW_GRAY)
	return

def CloseErrorWindow ():
	if ErrorWindow:
		ErrorWindow.Unload ()
	UpdateInventoryWindow ()
	return

def ReadItemWindow ():
	"""Tries to learn the mage scroll."""

	pc = GemRB.GameGetSelectedPCSingle ()
	slot = GemRB.GetVar ("ItemButton")
	ret = Spellbook.CannotLearnSlotSpell()

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
		elif ret == LSR_FULL and GameCheck.IsBG2():
			strref = 32097
		elif ret == LSR_FULL and GameCheck.IsPST():
			strref = 50395
		elif GameCheck.IsPST():
			strref = 4250
		else:
			strref = 10831

		CloseItemInfoWindow ()
		GemRB.PlaySound ("EFF_M10") # failure!
		OpenErrorWindow (strref)
		return

	# we already checked for most failures, but we can still fail with bad % rolls vs intelligence
	ret = Spellbook.LearnFromScroll (pc, slot)
	if ret == LSR_OK:
		GemRB.PlaySound ("GAM_44") # success!
		if GameCheck.IsPST():
			strref = 4249
		else:
			strref = 10830
	else:
		GemRB.PlaySound ("EFF_M10") # failure!
		if GameCheck.IsPST():
			strref = 4250
		else:
			strref = 10831

	CloseItemInfoWindow ()
	OpenErrorWindow (strref)

def OpenItemWindow ():
	"""Displays information about the item."""

	#close inventory
	GemRB.SetVar ("Inventory", 1)
	slot = GemRB.GetVar ("ItemButton") #get this before closing win
	if ItemInfoWindow:
		ItemInfoWindow.Unload ()

	pc = GemRB.GameGetSelectedPCSingle ()
	slot_item = GemRB.GetSlotItem (pc, slot)
	ResRef = slot_item['ItemResRef']
	#the store will have to reopen the inventory
	GemRB.EnterStore (ResRef)
	return

def DialogItemWindow ():
	"""Converse with an item."""

	pc = GemRB.GameGetSelectedPCSingle ()
	if GameCheck.IsPST():
		slot, slot_item = GUIINV.ItemHash[GemRB.GetVar ('ItemButton')]
	else:
		slot = GemRB.GetVar ("ItemButton")
		slot_item = GemRB.GetSlotItem (pc, slot)
	ResRef = slot_item['ItemResRef']
	item = GemRB.GetItem (ResRef)
	dialog=item["Dialog"]
	if ItemInfoWindow:
		ItemInfoWindow.Unload ()
		
	import GUIINV
	GUIINV.OpenInventoryWindow ()
	GemRB.ExecuteString ("StartDialogOverride(\""+dialog+"\",Myself,0,0,1)", pc)
	return

def IdentifyUseSpell ():
	"""Identifies the item with a memorized spell."""

	global ItemIdentifyWindow

	pc = GemRB.GameGetSelectedPCSingle ()
	slot = GemRB.GetVar ("ItemButton")
	if ItemIdentifyWindow:
		ItemIdentifyWindow.Unload ()
	GemRB.HasSpecialSpell (pc, SP_IDENTIFY, 1)
	if ItemInfoWindow:
		ItemInfoWindow.Unload ()
	GemRB.ChangeItemFlag (pc, slot, IE_INV_ITEM_IDENTIFIED, OP_OR)
	GemRB.PlaySound(DEF_IDENTIFY)
	OpenItemInfoWindow(None, slot)
	return

def IdentifyUseScroll ():
	"""Identifies the item with a scroll or other item."""

	global ItemIdentifyWindow

	pc = GemRB.GameGetSelectedPCSingle ()
	slot = GemRB.GetVar ("ItemButton")
	if ItemIdentifyWindow:
		ItemIdentifyWindow.Unload ()
	if ItemInfoWindow:
		ItemInfoWindow.Unload ()
	if GemRB.HasSpecialItem (pc, 1, 1):
		GemRB.ChangeItemFlag (pc, slot, IE_INV_ITEM_IDENTIFIED, OP_OR)
	GemRB.PlaySound(DEF_IDENTIFY)
	OpenItemInfoWindow(None, slot)
	return

def CloseIdentifyItemWindow ():
	global ItemIdentifyWindow, ItemInfoWindow

	if ItemIdentifyWindow:
		ItemIdentifyWindow.Unload ()
		ItemIdentifyWindow = None
	if ItemInfoWindow:
		ItemInfoWindow.ShowModal (MODAL_SHADOW_GRAY)
	return

def IdentifyItemWindow ():
	global ItemIdentifyWindow

	pc = GemRB.GameGetSelectedPCSingle ()

	ItemIdentifyWindow = Window = GemRB.LoadWindow (9)
	Button = Window.GetControl (0)
	if GameCheck.IsPST():
		Button.SetText (4259)
	else:
		Button.SetText (17105)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, IdentifyUseSpell)
	if not GemRB.HasSpecialSpell (pc, SP_IDENTIFY, 0):
		Button.SetState (IE_GUI_BUTTON_DISABLED)

	Button = Window.GetControl (1)
	if GameCheck.IsPST():
		Button.SetText (4260)
	else:
		Button.SetText (17106)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, IdentifyUseScroll)
	if not GemRB.HasSpecialItem (pc, 1, 0):
		Button.SetState (IE_GUI_BUTTON_DISABLED)

	Button = Window.GetControl (2)
	if GameCheck.IsPST():
		Button.SetText (4196)
	else:
		Button.SetText (13727)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, CloseIdentifyItemWindow)
	Button.MakeEscape()

	TextArea = Window.GetControl (3)
	if GameCheck.IsPST():
		TextArea.SetText (4258)
	else:
		TextArea.SetText (19394)
	Window.ShowModal (MODAL_SHADOW_GRAY)
	return

def DoneAbilitiesItemWindow ():
	pc = GemRB.GameGetSelectedPCSingle ()
	slot = GemRB.GetVar ("ItemButton")
	GemRB.SetupQuickSlot (pc, 0, slot, GemRB.GetVar ("Ability") )
	CloseAbilitiesItemWindow ()
	return

def CloseAbilitiesItemWindow ():
	global ItemAbilitiesWindow, ItemInfoWindow

	if ItemAbilitiesWindow:
		ItemAbilitiesWindow.Unload ()
		ItemAbilitiesWindow = None
	if ItemInfoWindow:
		ItemInfoWindow.ShowModal (MODAL_SHADOW_GRAY)
	return

def AbilitiesItemWindow ():
	global ItemAbilitiesWindow

	ItemAbilitiesWindow = Window = GemRB.LoadWindow (6)

	pc = GemRB.GameGetSelectedPCSingle ()
	slot = GemRB.GetVar ("ItemButton")
	slot_item = GemRB.GetSlotItem (pc, slot)
	item = GemRB.GetItem (slot_item["ItemResRef"])
	Tips = item["Tooltips"]

	GemRB.SetVar ("Ability", slot_item["Header"])
	for i in range(3):
		Button = Window.GetControl (i+1)
		if GameCheck.IsBG2(): # TODO: check pst
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
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, DoneAbilitiesItemWindow)
	Button.MakeDefault()

	Button = Window.GetControl (10)
	Button.SetText (13727)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, CloseAbilitiesItemWindow)
	Button.MakeEscape()
	Window.ShowModal (MODAL_SHADOW_GRAY)
	return

def UpdateInventorySlot (pc, Button, Slot, Type, Equipped=False):
	Button.SetFont ("NUMBER")

	color = {'r' : 128, 'g' : 128, 'b' : 255, 'a' : 64}
	Button.SetBorder (0, color, 0,1)
	color = {'r' : 32, 'g' : 32, 'b' : 255, 'a' : 255}
	Button.SetBorder (1, color, 0,0, Button.GetInsetFrame(2))
	color = {'r' : 255, 'g' : 128, 'b' : 128, 'a' : 64}
	Button.SetBorder (2, color, 0,1)

	Button.SetText ("")
	Button.SetFlags (IE_GUI_BUTTON_ALIGN_RIGHT | IE_GUI_BUTTON_ALIGN_BOTTOM | IE_GUI_BUTTON_PICTURE, OP_OR)

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

		# MaxStackAmount holds the *maximum* item count in the stack while Usages0 holds the actual
		if item["MaxStackAmount"] > 1:
			Button.SetText (str (Slot["Usages0"]))
		else:
			Button.SetText ("")

		# auto-identify mundane items; the actual indentification will happen on transfer
		if not identified and item["LoreToID"] == 0:
			identified = True

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

		if GemRB.CanUseItemType (SLOT_ALL, Slot['ItemResRef'], pc, Equipped):
			Button.EnableBorder (2, 0)
		else:
			Button.EnableBorder (2, 1)

		Button.SetItemIcon (Slot['ItemResRef'], 0)

	return
