# -*-python-*-
# GemRB - Infinity Engine Emulator
# Copyright (C) 2003-2004 The GemRB Project
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


# GUIINV.py - scripts to control inventory windows from GUIINV winpack

###################################################

import GemRB
import GUICommon
import CommonTables
import GUICommonWindows
import InventoryCommon
from GUIDefines import *
from ie_stats import *
from ie_slots import *

ItemAmountWindow = None

# Control ID's:
# 0 - 9 eye, rings, earrings, tattoos, etc
# 10-13 Quick Weapon
# 14-18 Quick Item
# 19-23 Quiver
# 24-43 Personal Item
# 47-56 Ground Item

# 44      - portrait
# 45      - ground scrollbar?
# 46      - encumbrance
# 0x10000000 + 57 - name
# 0x10000000 + 58 - AC
# 0x10000000 + 59 - hp
# 0x10000000 + 60 - hp max
# 0x10000000 + 61 - msg
# 0x10000000 + 62 - party gold
# 0x10000000 + 63 - class


def InitInventoryWindow (Window):
	"""Opens the inventory window."""

	global AvSlotsTable

	Window.AddAlias("WIN_INV")

	AvSlotsTable = GemRB.LoadTable ('avslots')
	Window.GetControl(0x1000003d).AddAlias("MsgSys", 1)


	# inventory slots
	SlotCount = GemRB.GetSlotType (-1)["Count"]
	for slot in range (SlotCount):
		SlotType = GemRB.GetSlotType (slot)
		Button = Window.GetControl (SlotType["ID"])
		if Button:
			Button.SetEvent (IE_GUI_MOUSE_ENTER_BUTTON, MouseEnterSlot)
			Button.SetEvent (IE_GUI_MOUSE_LEAVE_BUTTON, MouseLeaveSlot)
			Button.SetVarAssoc ("ItemButton", slot)

			Button.SetFlags (IE_GUI_BUTTON_ALIGN_RIGHT | IE_GUI_BUTTON_ALIGN_BOTTOM, OP_OR)
			Button.SetFont ("NUMBER")

			color = {'r' : 128, 'g' : 128, 'b' : 255, 'a' : 64}
			Button.SetBorder (0,color,0,1)
			color['r'], color['b'] = color['b'], color['r']
			Button.SetBorder (1,color,0,1)

	# Ground Item
	for i in range (10):
		Button = Window.GetControl (i+47)
		Button.SetVarAssoc ("ItemButton", i + 47)
		Button.SetFlags (IE_GUI_BUTTON_ALIGN_RIGHT | IE_GUI_BUTTON_ALIGN_BOTTOM, OP_OR)
		Button.SetFont ("NUMBER")

		color = {'r' : 128, 'g' : 128, 'b' : 255, 'a' : 64}
		Button.SetBorder (0,color,0,1)
		color['r'], color['b'] = color['b'], color['r']
		Button.SetBorder (1,color,0,1)

		Button.SetEvent (IE_GUI_MOUSE_ENTER_BUTTON, InventoryCommon.MouseEnterGround)
		Button.SetEvent (IE_GUI_MOUSE_LEAVE_BUTTON, InventoryCommon.MouseLeaveGround)

	ScrollBar = Window.GetControl (45)
	ScrollBar.SetEvent (IE_GUI_SCROLLBAR_ON_CHANGE, lambda: RefreshInventoryWindow(Window))

	for i in range (57, 64):
		Label = Window.GetControl (0x10000000 + i)
		Label.SetText (str (i))

	# portrait
	Button = Window.GetControl (44)
	Button.SetState (IE_GUI_BUTTON_LOCKED)
	Button.SetFlags(IE_GUI_BUTTON_NO_IMAGE | IE_GUI_BUTTON_PICTURE, OP_SET)
	Button.SetAction (OnAutoEquip, IE_ACT_DRAG_DROP_DST)

	# encumbrance
	Button = Window.GetControl (46)
	Button.SetState (IE_GUI_BUTTON_LOCKED)
	Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_SET)
	Button.SetFont ('NUMBER')

	# armor class
	Label = Window.GetControl (0x1000003a)
	Label.SetTooltip (4197)

	# hp current
	Label = Window.GetControl (0x1000003b)
	Label.SetTooltip (4198)

	# hp max
	Label = Window.GetControl (0x1000003c)
	Label.SetTooltip (4199)

	# info label, game paused, etc
	Label = Window.GetControl (0x1000003d)
	Label.SetTextColor({'r' : 255, 'g' : 255, 'b' : 255})
	Label.SetText ("")

	return

def UpdateInventoryWindow (Window = None):
	"""Redraws the inventory window and resets TopIndex."""

	global slot_list

	if Window == None:
		Window = GemRB.GetView("WIN_INV")

	GUICommonWindows.UpdateAnimation ()

	pc = GemRB.GameGetSelectedPCSingle ()

	Container = GemRB.GetContainer (pc, 1)
	ScrollBar = Window.GetControl (45)
	Count = Container['ItemCount']
	if Count<1:
		Count=1
	ScrollBar.SetVarAssoc ("TopIndex", Count)
	RefreshInventoryWindow (Window)

	# PST uses unhardcoded/avslots.2da to decide which slots do what per character
	row = GemRB.GetPlayerStat (pc, IE_SPECIFIC)
	slot_list = map (int, AvSlotsTable.GetValue (row, 1, GTV_STR).split( ','))

	# populate inventory slot controls
	for i in range (46):
		UpdateSlot (pc, i)

ToggleInventoryWindow = GUICommonWindows.CreateTopWinLoader(3, "GUIINV", GUICommonWindows.ToggleWindow, InitInventoryWindow, UpdateInventoryWindow, WINDOW_TOP|WINDOW_HCENTER)
OpenInventoryWindow = GUICommonWindows.CreateTopWinLoader(3, "GUIINV", GUICommonWindows.OpenWindowOnce, InitInventoryWindow, UpdateInventoryWindow, WINDOW_TOP|WINDOW_HCENTER)

InventoryCommon.UpdateInventoryWindow = UpdateInventoryWindow

def RefreshInventoryWindow (Window):
	"""Partial redraw without resetting TopIndex."""

	pc = GemRB.GameGetSelectedPCSingle ()

	# name
	Label = Window.GetControl (0x10000039)
	Label.SetText (GemRB.GetPlayerName (pc, 1))


	# portrait
	Button = Window.GetControl (44)
	Button.SetPicture (GUICommonWindows.GetActorPortrait (pc, 'INVENTORY'))

	Label = Window.GetControl (0x1000003a)
	GUICommon.SetEncumbranceLabels (Window, 46, None, pc, True)
	Label = Window.GetControl (0x1000003a)

	# armor class
	ac = GemRB.GetPlayerStat (pc, IE_ARMORCLASS)
	Label = Window.GetControl (0x1000003a)
	Label.SetText (str (ac))

	# hp current
	hp = GemRB.GetPlayerStat (pc, IE_HITPOINTS)
	Label = Window.GetControl (0x1000003b)
	Label.SetText (str (hp))

	# hp max
	hpmax = GemRB.GetPlayerStat (pc, IE_MAXHITPOINTS)
	Label = Window.GetControl (0x1000003c)
	Label.SetText (str (hpmax))

	# party gold
	Label = Window.GetControl (0x1000003e)
	Label.SetText (str (GemRB.GameGetPartyGold ()))

	# class
	text = CommonTables.Classes.GetValue (GUICommon.GetClassRowName (pc), "NAME_REF")

	Label = Window.GetControl (0x1000003f)
	Label.SetText (text)

	GUICommon.AdjustWindowVisibility (Window, pc, False)

	# update ground inventory slots
	Container = GemRB.GetContainer(pc, 1)
	TopIndex = GemRB.GetVar ("TopIndex")
	for i in range (10):
		Button = Window.GetControl (i+47)
		if GemRB.IsDraggingItem ():
			Button.SetState (IE_GUI_BUTTON_FAKEPRESSED)
		else:
			Button.SetState (IE_GUI_BUTTON_ENABLED)
		Button.SetAction (InventoryCommon.OnDragItemGround, IE_ACT_DRAG_DROP_DST)
		Slot = GemRB.GetContainerItem (pc, i+TopIndex)
		if Slot != None:
			item = GemRB.GetItem (Slot['ItemResRef'])
			identified = Slot["Flags"] & IE_INV_ITEM_IDENTIFIED

			Button.SetItemIcon (Slot['ItemResRef'])
			Button.SetFlags (IE_GUI_BUTTON_PICTURE, OP_OR)
			if item['MaxStackAmount'] > 1:
				Button.SetText (str (Slot['Usages0']))
			else:
				Button.SetText ('')
			if not identified or item["ItemNameIdentified"] == -1:
				Button.SetTooltip (item["ItemName"])
				Button.EnableBorder (0, 1)
			else:
				Button.SetTooltip (item["ItemNameIdentified"])
				Button.EnableBorder (0, 0)
			
			Button.SetAction(InventoryCommon.OnDragItemGround, IE_ACT_DRAG_DROP_CRT)
			Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, InventoryCommon.OnDragItemGround)
			Button.SetEvent (IE_GUI_BUTTON_ON_RIGHT_PRESS, InventoryCommon.OpenGroundItemInfoWindow)
			Button.SetEvent (IE_GUI_BUTTON_ON_SHIFT_PRESS, InventoryCommon.OpenGroundItemAmountWindow)

		else:
			Button.SetFlags (IE_GUI_BUTTON_PICTURE, OP_NAND)
			Button.SetTooltip (4273)
			Button.SetText ('')
			Button.EnableBorder (0, 0)
			Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, None)
			Button.SetEvent (IE_GUI_BUTTON_ON_RIGHT_PRESS, None)
			Button.SetEvent (IE_GUI_BUTTON_ON_SHIFT_PRESS, None)
 	return

def UpdateSlot (pc, i):
	Window = GemRB.GetView("WIN_INV")

	# NOTE: there are invisible items (e.g. MORTEP) in inaccessible slots
	# used to assign powers and protections

	using_fists = 0
	if i >= len(slot_list):
		#this prevents accidental out of range errors from the avslots list
		#any slots beyond the table are treated as unchanging
		return GemRB.GetSlotType (i+1)
	elif slot_list[i]<0:
		slot = i+1
		SlotType = GemRB.GetSlotType (slot)
		slot_item = 0
	else:
		slot = slot_list[i]+1
		SlotType = GemRB.GetSlotType (slot)
		slot_item = GemRB.GetSlotItem (pc, slot)
		#PST displays the default weapon in the first slot if nothing else was equipped
		if slot_item is None and SlotType["ID"] == 10 and GemRB.GetEquippedQuickSlot(pc) == 10:
			slot_item = GemRB.GetSlotItem (pc, 0)
			using_fists = 1

	ControlID = SlotType["ID"]
	if ControlID<0:
		return

	if GemRB.IsDraggingItem ():
		#get dragged item
		drag_item = GemRB.GetSlotItem (0,0)
		itemname = drag_item["ItemResRef"]
		drag_item = GemRB.GetItem (itemname)
	else:
		itemname = ""

	Button = Window.GetControl (ControlID)
	Button.SetSprites ('IVSLOT', 0, 0, 1, 2, 3)
	if slot_item:
		item = GemRB.GetItem (slot_item['ItemResRef'])
		identified = slot_item['Flags'] & IE_INV_ITEM_IDENTIFIED

		Button.SetItemIcon (slot_item['ItemResRef'])
		if item['MaxStackAmount'] > 1:
			Button.SetText (str (slot_item['Usages0']))
		else:
			Button.SetText ('')

		if not identified or item['ItemNameIdentified'] == -1:
			Button.SetTooltip (item['ItemName'])
			Button.EnableBorder (0, 1)
		else:
			Button.SetTooltip (item['ItemNameIdentified'])
			Button.EnableBorder (0, 0)

		if GemRB.CanUseItemType (SLOT_ANY, slot_item['ItemResRef'], pc):
			Button.EnableBorder (1, 0)
		else:
			Button.EnableBorder (1, 1)

		Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_NAND)
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, OnDragItem)
		Button.SetEvent (IE_GUI_BUTTON_ON_RIGHT_PRESS, lambda: InventoryCommon.OpenItemInfoWindow(None, slot=slot))
		Button.SetEvent (IE_GUI_BUTTON_ON_SHIFT_PRESS, InventoryCommon.OpenItemAmountWindow)
		Button.SetAction (OnDragItem, IE_ACT_DRAG_DROP_DST)
		#If the slot is being used to display the 'default' weapon, disable dragging.
		if SlotType["ID"] == 10 and using_fists:
			Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, None)
			#dropping is ok, because it will drop in the quick weapon slot and not the default weapon slot.
	else:
		Button.SetItemIcon ('')
		Button.SetText ('')

		if slot_list[i]>=0:
			Button.SetSprites (SlotType["ResRef"], 0, 0, 1, 2, 3)
			Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_NAND)
			Button.SetAction (OnDragItem, IE_ACT_DRAG_DROP_DST)
			Button.SetTooltip (SlotType["Tip"])
		else:
			Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_OR)
			Button.SetAction (None, IE_ACT_DRAG_DROP_DST)
			Button.SetTooltip ('')
			itemname = ""

		Button.EnableBorder (0, 0)
		Button.EnableBorder (1, 0)

		if SlotType["Effects"]==4:
			Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, DefaultWeapon)
		else:
			Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, None)
		Button.SetEvent (IE_GUI_BUTTON_ON_RIGHT_PRESS, None)
		Button.SetEvent (IE_GUI_BUTTON_ON_SHIFT_PRESS, None)

	if (SlotType["Type"]&SLOT_INVENTORY) or not GemRB.CanUseItemType (SlotType["Type"], itemname):
		Button.SetState (IE_GUI_BUTTON_ENABLED)
	else:
		Button.SetState (IE_GUI_BUTTON_FAKEPRESSED)

	if slot_item and (GemRB.GetEquippedQuickSlot (pc)==slot or GemRB.GetEquippedAmmunition (pc)==slot):
		Button.SetState (IE_GUI_BUTTON_FAKEDISABLED)

	return SlotType

def DefaultWeapon ():
	pc = GemRB.GameGetFirstSelectedActor ()
	GemRB.SetEquippedQuickSlot (pc, 1000, -1)
	UpdateInventoryWindow ()
	return

def OnAutoEquip ():
	if not GemRB.IsDraggingItem ():
		return

	pc = GemRB.GameGetSelectedPCSingle ()

	item = GemRB.GetSlotItem (0,0)
	ret = GemRB.DropDraggedItem (pc, -1)
	#this is exactly the same hack as the blackisle guys did it
	#quite lame, but we should copy their efforts the best
	if ret == 2 and item and (item['ItemResRef'] == "dustrobe"):
		GemRB.SetGlobal("APPEARANCE","GLOBAL",2)

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
		if not slot_item:
			return
		item = GemRB.GetItem (slot_item["ItemResRef"])
		GemRB.DragItem (pc, slot, item["ItemIcon"], 0, 0)
		if slot == 2:
			GemRB.SetGlobal("APPEARANCE","GLOBAL",0)
	else:
		item = GemRB.GetSlotItem (0,0)
		itemdata = GemRB.GetItem(item['ItemResRef'])
		GemRB.DropDraggedItem (pc, slot)
		if slot == 2 and item['ItemResRef'] == 'dustrobe':
			GemRB.SetGlobal("APPEARANCE","GLOBAL",2)
		elif slot < 21 and itemdata['AnimationType'] != '':
			GemRB.SetGlobal("APPEARANCE","GLOBAL",0)

	UpdateInventoryWindow ()
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

###################################################
# End of file GUIINV.py
