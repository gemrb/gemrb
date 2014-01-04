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


InventoryWindow = None
ItemAmountWindow = None
OverSlot = None

def OpenInventoryWindowClick ():
	tmp = GemRB.GetVar ("PressedPortrait")
	GemRB.GameSelectPC (tmp, True, SELECT_REPLACE)
	OpenInventoryWindow ()
	return


ItemHash = {}

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


def OpenInventoryWindow ():
	"""Opens the inventory window."""

	global AvSlotsTable
	global InventoryWindow

	AvSlotsTable = GemRB.LoadTable ('avslots')

	if GUICommon.CloseOtherWindow (OpenInventoryWindow):
		GUICommon.GameWindow.SetVisible(WINDOW_VISIBLE)
		if InventoryWindow:
			InventoryWindow.Unload ()
		InventoryWindow = None
		GemRB.SetVar ("OtherWindow", -1)
		GemRB.SetVar ("MessageLabel", -1)
		GUICommonWindows.SetSelectionChangeHandler (None)
		GemRB.UnhideGUI ()
		return

	GemRB.HideGUI ()
	GemRB.LoadWindowPack ("GUIINV")
	InventoryWindow = Window = GemRB.LoadWindow (3)
	GemRB.SetVar ("OtherWindow", InventoryWindow.ID)
	GemRB.SetVar ("MessageLabel", Window.GetControl(0x1000003d).ID)

	# inventory slots
	for i in range (44):
		Button = Window.GetControl (i)
		Button.SetFlags (IE_GUI_BUTTON_ALIGN_RIGHT | IE_GUI_BUTTON_ALIGN_BOTTOM, OP_OR)
		Button.SetFont ("NUMBER")
		Button.SetVarAssoc ("ItemButton", i)
		Button.SetBorder (0,0,0,0,0,128,128,255,64,0,1)
		Button.SetBorder (1,0,0,0,0,255,128,128,64,0,1)
		Button.SetEvent (IE_GUI_MOUSE_ENTER_BUTTON, MouseEnterSlot)
		Button.SetEvent (IE_GUI_MOUSE_LEAVE_BUTTON, MouseLeaveSlot)

	# Ground Item
	for i in range (10):
		Button = Window.GetControl (i+47)
		Button.SetVarAssoc ("GroundItemButton", i)
		Button.SetFlags (IE_GUI_BUTTON_ALIGN_RIGHT | IE_GUI_BUTTON_ALIGN_BOTTOM, OP_OR)
		Button.SetFont ("NUMBER")
		Button.SetBorder (0,0,0,0,0,128,128,255,64,0,1)
		Button.SetBorder (1,0,0,0,0,255,128,128,64,0,1)
		Button.SetEvent (IE_GUI_MOUSE_ENTER_BUTTON, InventoryCommon.MouseEnterGround)
		Button.SetEvent (IE_GUI_MOUSE_LEAVE_BUTTON, InventoryCommon.MouseLeaveGround)

	ScrollBar = Window.GetControl (45)
	ScrollBar.SetEvent (IE_GUI_SCROLLBAR_ON_CHANGE, RefreshInventoryWindow)

	for i in range (57, 64):
		Label = Window.GetControl (0x10000000 + i)
		Label.SetText (str (i))

	# portrait
	Button = Window.GetControl (44)
	Button.SetState (IE_GUI_BUTTON_LOCKED)
	Button.SetFlags(IE_GUI_BUTTON_NO_IMAGE | IE_GUI_BUTTON_PICTURE, OP_SET)
	Button.SetEvent (IE_GUI_BUTTON_ON_DRAG_DROP, OnAutoEquip)

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
	Label.SetTextColor(255, 255, 255)
	Label.SetText ("")
	GemRB.SetVar ("TopIndex", 0)
	GUICommonWindows.SetSelectionChangeHandler (UpdateInventoryWindow)
	UpdateInventoryWindow ()

	GUICommon.GameWindow.SetVisible(WINDOW_INVISIBLE)
	GUICommonWindows.PortraitWindow.SetVisible(WINDOW_VISIBLE)
	GUICommonWindows.OptionsWindow.SetVisible(WINDOW_VISIBLE)
	return

def UpdateInventoryWindow ():
	"""Redraws the inventory window and resets TopIndex."""
	global ItemHash
	global slot_list

	Window = InventoryWindow

	GUICommonWindows.UpdateAnimation ()

	pc = GemRB.GameGetSelectedPCSingle ()

	Container = GemRB.GetContainer (pc, 1)
	ScrollBar = Window.GetControl (45)
	Count = Container['ItemCount']
	if Count<1:
		Count=1
	ScrollBar.SetVarAssoc ("TopIndex", Count)
	RefreshInventoryWindow ()
	# And now for the items ....

	ItemHash = {}

	# get a list which maps slot number to slot type/icon/tooltip
	row = GemRB.GetPlayerStat (pc, IE_SPECIFIC)
	slot_list = map (int, AvSlotsTable.GetValue (row, 1, 0).split( ','))

	# populate inventory slot controls
	for i in range (46):
		UpdateSlot (pc, i)

def RefreshInventoryWindow ():
	"""Partial redraw without resetting TopIndex."""

	Window = InventoryWindow

	pc = GemRB.GameGetSelectedPCSingle ()

	# name
	Label = Window.GetControl (0x10000039)
	Label.SetText (GemRB.GetPlayerName (pc, 1))


	# portrait
	Button = Window.GetControl (44)
	Button.SetPicture (GUICommonWindows.GetActorPortrait (pc, 'INVENTORY'))

	GUICommon.SetEncumbranceLabels (Window, 46, None, pc, True)

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
		Button.SetEvent (IE_GUI_BUTTON_ON_DRAG_DROP, InventoryCommon.OnDragItemGround)
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
			Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, InventoryCommon.OnDragItemGround)
			Button.SetEvent (IE_GUI_BUTTON_ON_RIGHT_PRESS, InventoryCommon.OpenGroundItemInfoWindow)
			Button.SetEvent (IE_GUI_BUTTON_ON_SHIFT_PRESS, None) #TODO: implement OpenGroundItemAmountWindow

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
	Window = InventoryWindow

	# NOTE: there are invisible items (e.g. MORTEP) in inaccessible slots
	# used to assign powers and protections

	if slot_list[i]<0:
		slot = i+1
		SlotType = GemRB.GetSlotType (slot)
		slot_item = 0
	else:
		slot = slot_list[i]+1
		SlotType = GemRB.GetSlotType (slot)
		slot_item = GemRB.GetSlotItem (pc, slot)
		#PST displays the default weapon in the first slot if nothing else was equipped
		if slot_item == None and SlotType["ID"] == 10 and GemRB.GetEquippedQuickSlot(pc)==10:
			slot_item = GemRB.GetSlotItem (pc, 0)

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
	ItemHash[ControlID] = [slot, slot_item]
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
		Button.SetEvent (IE_GUI_BUTTON_ON_RIGHT_PRESS, InventoryCommon.OpenItemInfoWindow)
		Button.SetEvent (IE_GUI_BUTTON_ON_SHIFT_PRESS, OpenItemAmountWindow)
		Button.SetEvent (IE_GUI_BUTTON_ON_DRAG_DROP, OnDragItem)
	else:
		Button.SetItemIcon ('')
		Button.SetText ('')

		if slot_list[i]>=0:
			Button.SetSprites (SlotType["ResRef"], 0, 0, 1, 2, 3)
			Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_NAND)
			Button.SetEvent (IE_GUI_BUTTON_ON_DRAG_DROP, OnDragItem)
			Button.SetTooltip (SlotType["Tip"])
		else:
			Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_OR)
			Button.SetEvent (IE_GUI_BUTTON_ON_DRAG_DROP, None)
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

	if OverSlot == slot-1:
		if GemRB.CanUseItemType (SlotType["Type"], itemname):
			Button.SetState (IE_GUI_BUTTON_SELECTED)
		else:
			Button.SetState (IE_GUI_BUTTON_ENABLED)
	else:
		if (SlotType["Type"]&SLOT_INVENTORY) or not GemRB.CanUseItemType (SlotType["Type"], itemname):
			Button.SetState (IE_GUI_BUTTON_ENABLED)
		else:
			Button.SetState (IE_GUI_BUTTON_FAKEPRESSED)

		if slot_item and (GemRB.GetEquippedQuickSlot (pc)==slot or GemRB.GetEquippedAmmunition (pc)==slot):
			Button.SetState (IE_GUI_BUTTON_FAKEDISABLED)
	return

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

def OnDragItem ():
	pc = GemRB.GameGetSelectedPCSingle ()

	slot, slot_item = ItemHash[GemRB.GetVar ('ItemButton')]

	if not GemRB.IsDraggingItem ():
		ResRef = slot_item['ItemResRef']
		item = GemRB.GetItem (ResRef)
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

def DragItemAmount ():
	"""Drag a split item."""

	pc = GemRB.GameGetSelectedPCSingle ()
	slot, slot_item = ItemHash[GemRB.GetVar ('ItemButton')]
	Text = ItemAmountWindow.GetControl (6)
	Amount = Text.QueryText ()
	item = GemRB.GetItem (slot_item["ItemResRef"])
	GemRB.DragItem (pc, slot, item["ItemIcon"], int ("0"+Amount), 0)
	OpenItemAmountWindow ()
	return

def OpenItemAmountWindow ():
	global ItemAmountWindow

	GemRB.HideGUI ()

	if ItemAmountWindow != None:
		if ItemAmountWindow:
			ItemAmountWindow.Unload ()
		ItemAmountWindow = None
		GemRB.SetVar ("FloatWindow", -1)

		GemRB.UnhideGUI ()
		return

	ItemAmountWindow = Window = GemRB.LoadWindow (4)
	GemRB.SetVar ("FloatWindow", ItemAmountWindow.ID)

	pc = GemRB.GameGetSelectedPCSingle ()
	slot, slot_item = ItemHash[GemRB.GetVar ('ItemButton')]
	ResRef = slot_item['ItemResRef']
	item = GemRB.GetItem (ResRef)

	# item icon
	Icon = Window.GetControl (0)
	Icon.SetFlags (IE_GUI_BUTTON_PICTURE | IE_GUI_BUTTON_NO_IMAGE, OP_SET)
	Icon.SetItemIcon (ResRef)

	# item amount
	Text = Window.GetControl (6)
	Text.SetSize (40, 40)
	Text.SetText ("1")
	Text.SetStatus (IE_GUI_EDIT_NUMBER|IE_GUI_CONTROL_FOCUSED)

	# Done
	Button = Window.GetControl (2)
	Button.SetText (1403)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, DragItemAmount)
	Button.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)

	# Cancel
	Button = Window.GetControl (1)
	Button.SetText (4196)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, OpenItemAmountWindow)
	Button.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)

	# 0 bmp
	# 1,2 done/cancel?
	# 3 +
	# 4 -
	# 6 text

	GemRB.UnhideGUI ()
	Window.ShowModal (MODAL_SHADOW_GRAY)

def LeftItemScroll ():
	tmp = GemRB.GetVar ('ItemButton') - 1
	if tmp < 0:
		tmp = 43

	while tmp not in ItemHash or not ItemHash[tmp][1]:
		tmp = tmp - 1
		if tmp < 0:
			tmp = 43

	GemRB.SetVar ('ItemButton', tmp)
	InventoryCommon.CloseItemInfoWindow ()
	InventoryCommon.OpenItemInfoWindow ()
	return

def RightItemScroll ():
	tmp = GemRB.GetVar ('ItemButton') + 1
	if tmp > 43:
		tmp = 0

	while tmp not in ItemHash or not ItemHash[tmp][1]:
		tmp = tmp + 1
		if tmp > 43:
			tmp = 0

	GemRB.SetVar ('ItemButton', tmp)
	InventoryCommon.CloseItemInfoWindow ()
	InventoryCommon.OpenItemInfoWindow ()
	return

def MouseEnterSlot ():
	global OverSlot

	pc = GemRB.GameGetSelectedPCSingle ()
	slot = GemRB.GetVar ('ItemButton')
	if slot not in ItemHash:
		return
	OverSlot = ItemHash[slot][0]-1
	if GemRB.IsDraggingItem ():
		for i in range(len(slot_list)):
			if slot_list[i]==OverSlot:
				UpdateSlot (pc, i)
	return

def MouseLeaveSlot ():
	global OverSlot

	pc = GemRB.GameGetSelectedPCSingle ()
	slot = GemRB.GetVar ('ItemButton')
	if slot not in ItemHash:
		return
	slot = ItemHash[slot][0]-1
	if slot == OverSlot or not GemRB.IsDraggingItem ():
		OverSlot = None

	for i in range(len(slot_list)):
		if slot_list[i]==slot:
			UpdateSlot (pc, i)
	return

###################################################
# End of file GUIINV.py
