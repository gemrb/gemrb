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

OverSlot = None
UsedSlot = None
ItemInfoWindow = None
ItemAmountWindow = None
ItemIdentifyWindow = None
ItemAbilitiesWindow = None
ErrorWindow = None
StackAmount = 0
pause = None

def OnDragItemGround ():
	"""Drops and item to the ground."""

	pc = GemRB.GameGetSelectedPCSingle ()
	slot = GemRB.GetVar ("GroundItemButton") + GemRB.GetVar ("TopIndex")

	if GemRB.IsDraggingItem ()==0:
		slot_item = GemRB.GetContainerItem (pc, slot)
		item = GemRB.GetItem (slot_item["ItemResRef"])
		GemRB.DragItem (pc, slot, item["ItemIcon"], 0, 1) #container
		if GameCheck.IsPST():
			GemRB.PlaySound (item["DescIcon"])
	else:
		GemRB.DropDraggedItem (pc, -2) #dropping on ground

	GUIINV.UpdateInventoryWindow ()
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

	GUIINV.UpdateInventoryWindow ()
	return

def OnDragItem ():
	"""Updates dragging."""

	if GemRB.IsDraggingItem()==2:
		return

	#don't call when splitting items
	if ItemAmountWindow != None:
		return

	pc = GemRB.GameGetSelectedPCSingle ()
	slot = GemRB.GetVar ("ItemButton")
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
				if item["Function"]&4:
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

	GUIINV.UpdateInventoryWindow ()
	return

def OnDropItemToPC ():
	"""Gives an item to another character."""

	pc = GemRB.GetVar ("PressedPortrait")

	#-3 : drop stuff in inventory (but not equippable slots)
	GemRB.DropDraggedItem (pc, -3)
	GUIINV.UpdateInventoryWindow ()
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
	OpenItemAmountWindow ()
	return

def MouseEnterSlot ():
	global OverSlot

	pc = GemRB.GameGetSelectedPCSingle ()
	OverSlot = GemRB.GetVar ("ItemButton")
	if GemRB.IsDraggingItem ()==1:
		UpdateSlot (pc, OverSlot-1)
	return

def MouseLeaveSlot ():
	global OverSlot

	pc = GemRB.GameGetSelectedPCSingle ()
	slot = GemRB.GetVar ("ItemButton")
	if slot == OverSlot or not GemRB.IsDraggingItem ():
		OverSlot = None
	UpdateSlot (pc, slot-1)
	return

def MouseEnterGround ():
	Window = GUIINV.InventoryWindow

	if GameCheck.IsPST():
		offset = 47
	else:
		offset = 68
	i = GemRB.GetVar ("GroundItemButton")
	Button = Window.GetControl (i+offset)

	if GemRB.IsDraggingItem ()==1:
		Button.SetState (IE_GUI_BUTTON_SELECTED)
	return

def MouseLeaveGround ():
	Window = GUIINV.InventoryWindow

	if GameCheck.IsPST():
		offset = 47
	else:
		offset = 68
	i = GemRB.GetVar ("GroundItemButton")
	Button = Window.GetControl (i+offset)

	if GemRB.IsDraggingItem ()==1:
		Button.SetState (IE_GUI_BUTTON_FAKEPRESSED)
	return

def CloseItemInfoWindow ():
	if ItemInfoWindow:
		ItemInfoWindow.Unload ()
	GUIINV.UpdateInventoryWindow ()
	return

def DisplayItem (itemresref, type):
	global ItemInfoWindow

	item = GemRB.GetItem (itemresref)
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
		Button.SetItemIcon (itemresref)
	else:
		Button.SetFlags (IE_GUI_BUTTON_PICTURE, OP_OR)
		Button.SetItemIcon (itemresref,0)
	Button.SetState (IE_GUI_BUTTON_LOCKED)

	#middle button
	Button = Window.GetControl (4)
	Button.SetText (strrefs[0])
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, CloseItemInfoWindow)
	Button.SetFlags (IE_GUI_BUTTON_CANCEL|IE_GUI_BUTTON_DEFAULT, OP_OR)

	#textarea
	Text = Window.GetControl (5)
	if (type&2):
		text = item["ItemDesc"]
	else:
		text = item["ItemDescIdentified"]

	Text.Clear ()
	Text.Append (text)

	#left button
	Button = Window.GetControl(8)
	select = (type&1) and (item["Function"]&8)

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
			Button.SetItemIcon (itemresref, 1) # no DescIcon
		else:
			Button.SetItemIcon (itemresref, 2)
		Button.SetState (IE_GUI_BUTTON_LOCKED)

	#right button
	Button = Window.GetControl(9)
	drink = (type&1) and (item["Function"]&1)
	read = (type&1) and (item["Function"]&2)
	# sorcerers cannot learn spells
	# FIXME: unhardcode
	pc = GemRB.GameGetSelectedPCSingle ()
	if GemRB.GetPlayerStat (pc, IE_CLASS) == 19:
		read = 0
	container = (type&1) and (item["Function"]&4)
	dialog = (type&1) and (item["Dialog"]!="")
	familiar = (type&1) and (item["Type"] == 38)
	if drink:
		Button.SetText (strrefs[3])
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, DrinkItemWindow)
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
	elif familiar:
		Button.SetText (4373)
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, ReleaseFamiliar)
	else:
		Button.SetState (IE_GUI_BUTTON_LOCKED)
		Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_SET)

	Label = Window.HasControl(0x1000000b)
	if Label:
		Label = Window.GetControl (0x1000000b)
		if (type&2):
			# NOT IDENTIFIED
			Label.SetText (strrefs[6])
		else:
			Label.SetText ("")

	# in pst one can cycle through all the items from the description window
	if Window.HasControl (14):
		#left scroll
		Button = Window.GetControl (13)
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, GUIINV.LeftItemScroll)

		#right scroll
		Button = Window.GetControl (14)
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, GUIINV.RightItemScroll)

	ItemInfoWindow.ShowModal(MODAL_SHADOW_GRAY)
	return

def OpenItemInfoWindow (slot = None):
	pc = GemRB.GameGetSelectedPCSingle ()

	if slot == None:
		if GameCheck.IsPST():
			slot, slot_item = GUIINV.ItemHash[GemRB.GetVar ('ItemButton')]
		else:
			slot = GemRB.GetVar ("ItemButton")
			slot_item = GemRB.GetSlotItem (pc, slot)
	else:
		slot_item = GemRB.GetSlotItem (pc, slot)

	item = GemRB.GetItem (slot_item["ItemResRef"])

	if TryAutoIdentification(pc, item, slot, slot_item, True):
		GUIINV.UpdateInventoryWindow ()

	if slot_item["Flags"] & IE_INV_ITEM_IDENTIFIED:
		value = 1
	else:
		value = 3
	DisplayItem (slot_item["ItemResRef"], value)
	return

#auto identify when lore is high enough
def TryAutoIdentification(pc, item, slot, slot_item, enabled=0):
	if enabled and item["LoreToID"]<=GemRB.GetPlayerStat (pc, IE_LORE):
		GemRB.ChangeItemFlag (pc, slot, IE_INV_ITEM_IDENTIFIED, OP_OR)
		slot_item["Flags"] |= IE_INV_ITEM_IDENTIFIED
		return True
	return False

def OpenGroundItemInfoWindow ():
	global ItemInfoWindow

	pc = GemRB.GameGetSelectedPCSingle ()
	slot = GemRB.GetVar("TopIndex")+GemRB.GetVar("GroundItemButton")
	slot_item = GemRB.GetContainerItem (pc, slot)

	#the ground items are only displayable
	if slot_item["Flags"] & IE_INV_ITEM_IDENTIFIED:
		value = 0
	else:
		value = 2
	DisplayItem(slot_item["ItemResRef"], value)
	return

def OpenItemAmountWindow ():
	"""Open the split window."""

	global UsedSlot, OverSlot
	global ItemAmountWindow, StackAmount

	pc = GemRB.GameGetSelectedPCSingle ()

	if ItemAmountWindow != None:
		if ItemAmountWindow:
			ItemAmountWindow.Unload ()
		ItemAmountWindow = None
		GemRB.SetRepeatClickFlags (GEM_RK_DISABLE, OP_OR)
		UsedSlot = None
		OverSlot = None
		GUIINV.UpdateInventoryWindow()
		return

	UsedSlot = GemRB.GetVar ("ItemButton")
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
	Text.SetStatus (IE_GUI_EDIT_NUMBER|IE_GUI_CONTROL_FOCUSED)

	# Decrease
	Button = Window.GetControl (4)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, DecreaseStackAmount)

	# Increase
	Button = Window.GetControl (3)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, IncreaseStackAmount)

	# Done
	Button = Window.GetControl (2)
	Button.SetText (11973)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, DragItemAmount)
	Button.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)

	# Cancel
	Button = Window.GetControl (1)
	Button.SetText (13727)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, OpenItemAmountWindow)
	Button.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)

	#GemRB.UnhideGUI ()
	GemRB.SetRepeatClickFlags (GEM_RK_DISABLE, OP_NAND)
	Window.ShowModal (MODAL_SHADOW_GRAY)
	return

def UpdateSlot (pc, slot):
	"""Updates a specific slot."""

	Window = GUIINV.InventoryWindow
	SlotType = GemRB.GetSlotType (slot+1, pc)
	ControlID = SlotType["ID"]

	if not ControlID:
		return

	if GemRB.IsDraggingItem ()==1:
		#get dragged item
		drag_item = GemRB.GetSlotItem (0,0)
		itemname = drag_item["ItemResRef"]
		drag_item = GemRB.GetItem (itemname)
	else:
		itemname = ""

	Button = Window.GetControl (ControlID)
	slot_item = GemRB.GetSlotItem (pc, slot+1)

	Button.SetEvent (IE_GUI_BUTTON_ON_DRAG_DROP, OnDragItem)
	Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_NAND)

	# characters should auto-identify any item they recieve
	if slot_item:
		item = GemRB.GetItem (slot_item["ItemResRef"])
		TryAutoIdentification(pc, item, slot+1, slot_item, GemRB.GetVar("GUIEnhancements")&GE_TRY_IDENTIFY_ON_TRANSFER)

	GUICommon.UpdateInventorySlot (pc, Button, slot_item, "inventory", SlotType["Type"]&SLOT_INVENTORY == 0)

	if slot_item:
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

	if OverSlot == slot+1:
		if GemRB.CanUseItemType (SlotType["Type"], itemname):
			Button.SetState (IE_GUI_BUTTON_SELECTED)
		else:
			Button.SetState (IE_GUI_BUTTON_ENABLED)
	else:
		if (SlotType["Type"]&SLOT_INVENTORY) or not GemRB.CanUseItemType (SlotType["Type"], itemname):
			Button.SetState (IE_GUI_BUTTON_ENABLED)
		else:
			Button.SetState (IE_GUI_BUTTON_FAKEPRESSED)

		if slot_item and (GemRB.GetEquippedQuickSlot (pc)==slot+1 or GemRB.GetEquippedAmmunition (pc)==slot+1):
			Button.SetState (IE_GUI_BUTTON_FAKEDISABLED)

	return

def CancelColor():
	if ColorPicker:
		ColorPicker.Unload ()
	GUIINV.InventoryWindow.SetVisible (WINDOW_VISIBLE)
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
	GUIINV.UpdateInventoryWindow ()
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
	GUIINV.InventoryWindow.SetVisible (WINDOW_GRAYED) #darken it
	ColorPicker=GemRB.LoadWindow (3)
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
			Button.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)
		else:
			Button.SetBAM ("COLGRAD", 2, 0, MyColor)
			Button.SetFlags (IE_GUI_BUTTON_PICTURE|IE_GUI_BUTTON_RADIOBUTTON, OP_OR)
			Button.SetState (IE_GUI_BUTTON_ENABLED)
		Button.SetVarAssoc ("Selected",i)
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, ColorDonePress)
	ColorPicker.SetVisible (WINDOW_VISIBLE)
	return

def ReleaseFamiliar ():
	"""Simple Use Item"""

	pc = GemRB.GameGetSelectedPCSingle ()
	slot = GemRB.GetVar ("ItemButton")
	# the header is always the first, target is always self
	GemRB.UseItem (pc, slot, 0, 5)
	CloseItemInfoWindow ()
	return

def DrinkItemWindow ():
	"""Drink the potion"""

	pc = GemRB.GameGetSelectedPCSingle ()
	slot = GemRB.GetVar ("ItemButton")
	# the drink item header is always the first
	GemRB.UseItem (pc, slot, 0)
	CloseItemInfoWindow ()
	return

def OpenErrorWindow (strref):
	"""Opens the error window and displays the string."""

	global ErrorWindow
	pc = GemRB.GameGetSelectedPCSingle ()

	ErrorWindow = Window = GemRB.LoadWindow (7)
	Button = Window.GetControl (0)
	Button.SetText (11973)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, CloseErrorWindow)
	Button.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)

	TextArea = Window.GetControl (3)
	TextArea.SetText (strref)
	Window.ShowModal (MODAL_SHADOW_GRAY)
	return

def CloseErrorWindow ():
	if ErrorWindow:
		ErrorWindow.Unload ()
	GUIINV.UpdateInventoryWindow ()
	return

def ReadItemWindow ():
	global pause
	"""Tries to learn the mage scroll."""

	pc = GemRB.GameGetSelectedPCSingle ()
	slot = GemRB.GetVar ("ItemButton")
	ret = Spellbook.CannotLearnSlotSpell()

	if ret:
		# couldn't find any strrefs for the other undhandled values (stat, level)
		if ret == LSR_KNOWN and GameCheck.HasTOB():
			strref = 72873
		elif ret == LSR_KNOWN and GameCheck.IsPST():
			strref = 50394
		elif ret == LSR_FULL and GameCheck.IsBG2():
			strref = 32097
		elif ret == LSR_FULL and GameCheck.IsPST():
			strref = 50395
		elif GameCheck.IsPST():
			strref = 4250
		else:
			strref = 10831

		CloseItemInfoWindow ()
		OpenErrorWindow (strref)

	else:
		if GameCheck.IsPST():
			slot, slot_item = GUIINV.ItemHash[GemRB.GetVar ('ItemButton')]

		pause = GemRB.GamePause(3,1) #query the pause state
		GemRB.GamePause(0,1)
		GemRB.UseItem (pc, slot, 1, 5)
		GemRB.SetTimedEvent(DelayedReadItemWindow, 1)

	return ret

def DelayedReadItemWindow ():
	#restore the pause state
	if pause:
		GemRB.GamePause(1,1)

	CloseItemInfoWindow ()
	if GameCheck.IsPST():
		strref = 4249
	else:
		strref = 10830
	OpenErrorWindow (strref)
	return

def OpenItemWindow ():
	"""Displays information about the item."""

	#close inventory
	GemRB.SetVar ("Inventory", 1)
	slot = GemRB.GetVar ("ItemButton") #get this before closing win
	if ItemInfoWindow:
		ItemInfoWindow.Unload ()
	GUIINV.OpenInventoryWindow ()
	pc = GemRB.GameGetSelectedPCSingle ()
	slot_item = GemRB.GetSlotItem (pc, slot)
	ResRef = slot_item['ItemResRef']
	#the store will have to reopen the inventory
	GemRB.EnterStore (ResRef)
	return

def DialogItemWindow ():
	"""Converse with an item."""

	if ItemInfoWindow:
		ItemInfoWindow.Unload ()
	GUIINV.OpenInventoryWindow ()
	pc = GemRB.GameGetSelectedPCSingle ()
	if GameCheck.IsPST():
		slot, slot_item = GUIINV.ItemHash[GemRB.GetVar ('ItemButton')]
	else:
		slot = GemRB.GetVar ("ItemButton")
		slot_item = GemRB.GetSlotItem (pc, slot)
	ResRef = slot_item['ItemResRef']
	item = GemRB.GetItem (ResRef)
	dialog=item["Dialog"]
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
	OpenItemInfoWindow(slot)
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
	OpenItemInfoWindow(slot)
	return

def CloseIdentifyItemWindow ():
	global ItemIdentifyWindow

	if ItemIdentifyWindow:
		ItemIdentifyWindow.Unload ()
		ItemIdentifyWindow = None
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
	Button.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)

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
	Button.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)

	Button = Window.GetControl (10)
	Button.SetText (13727)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, CloseAbilitiesItemWindow)
	Button.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)
	Window.ShowModal (MODAL_SHADOW_GRAY)
	return

import GUIINV
