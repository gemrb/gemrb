# -*-python-*-
# GemRB - Infinity Engine Emulator
# Copyright (C) 2003 The GemRB Project
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

from GUIDefines import *
from ie_stats import *
from ie_slots import *
import GemRB
from GUICommon import CloseOtherWindow
from GUICommonWindows import GetActorPortrait, SetSelectionChangeHandler, SetEncumbranceLabels


InventoryWindow = None
ItemInfoWindow = None
ItemAmountWindow = None
ItemIdentifyWindow = None
OverSlot = None

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
	global AvSlotsTable
	global InventoryWindow

	AvSlotsTable = GemRB.LoadTableObject ('avslots')

	if CloseOtherWindow (OpenInventoryWindow):
		GemRB.HideGUI ()
		if InventoryWindow:
			InventoryWindow.Unload ()
		InventoryWindow = None
		GemRB.SetVar ("OtherWindow", -1)
		GemRB.SetVar ("MessageLabel", -1)
		SetSelectionChangeHandler (None)
		GemRB.UnhideGUI ()
		return

	GemRB.HideGUI ()
	GemRB.LoadWindowPack ("GUIINV")
	InventoryWindow = Window = GemRB.LoadWindowObject (3)
	GemRB.SetVar ("OtherWindow", InventoryWindow.ID)
	GemRB.SetVar ("MessageLabel", Window.GetControl(0x1000003d).ID)

	# inventory slots
	for i in range (44):
		Button = Window.GetControl (i)
		Button.SetFlags (IE_GUI_BUTTON_ALIGN_RIGHT , OP_OR)
		Button.SetFont ("NUMBER2")
		Button.SetVarAssoc ("ItemButton", i)
		Button.SetBorder (0,0,0,0,0,128,128,255,64,0,1)
		Button.SetBorder (1,0,0,0,0,255,128,128,64,0,1)
		Button.SetEvent (IE_GUI_MOUSE_ENTER_BUTTON, "MouseEnterSlot")
		Button.SetEvent (IE_GUI_MOUSE_LEAVE_BUTTON, "MouseLeaveSlot")

	# Ground Item
	for i in range (10):
		Button = Window.GetControl (i+47)
		Button.SetVarAssoc ("GroundItemButton", i)
		Button.SetFlags (IE_GUI_BUTTON_ALIGN_RIGHT , OP_OR)
		Button.SetFont ("NUMBER2")
		Button.SetBorder (0,0,0,0,0,128,128,255,64,0,1)
		Button.SetBorder (1,0,0,0,0,255,128,128,64,0,1)
		Button.SetEvent (IE_GUI_MOUSE_ENTER_BUTTON, "MouseEnterGround")
		Button.SetEvent (IE_GUI_MOUSE_LEAVE_BUTTON, "MouseLeaveGround")

	ScrollBar = Window.GetControl (45)
	ScrollBar.SetEvent (IE_GUI_SCROLLBAR_ON_CHANGE, "RefreshInventoryWindow")

	for i in range (57, 64):
		Label = Window.GetControl (0x10000000 + i)
		Label.SetText (str (i))

	# portrait
	Button = Window.GetControl (44)
	Button.SetState (IE_GUI_BUTTON_LOCKED)
	Button.SetFlags(IE_GUI_BUTTON_NO_IMAGE | IE_GUI_BUTTON_PICTURE, OP_SET)
	Button.SetEvent (IE_GUI_BUTTON_ON_DRAG_DROP, "OnAutoEquip")

	# encumbrance
	Button = Window.GetControl (46)
	Button.SetFont ("NUMBER")
	Button.SetState (IE_GUI_BUTTON_LOCKED)
	Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_SET)

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
	SetSelectionChangeHandler (UpdateInventoryWindow)
	UpdateInventoryWindow ()

	GemRB.UnhideGUI()


def UpdateInventoryWindow ():
	global ItemHash
	global slot_list

	Window = InventoryWindow

	GemRB.RunEventHandler("UpdateAnimation")

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
	Window = InventoryWindow
	pc = GemRB.GameGetSelectedPCSingle ()

	# name
	Label = Window.GetControl (0x10000039)
	Label.SetText (GemRB.GetPlayerName (pc, 1))


	# portrait
	Button = Window.GetControl (44)
	Button.SetPicture (GetActorPortrait (pc, 'INVENTORY'))

	SetEncumbranceLabels (Window, 46, None, pc)

	# armor class
	ac = GemRB.GetPlayerStat (pc, IE_ARMORCLASS)
	ac += GemRB.GetAbilityBonus (IE_DEX, 2, GemRB.GetPlayerStat (pc, IE_DEX) )
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
	ClassTable = GemRB.LoadTableObject ("classes")
	text = ClassTable.GetValue (GemRB.GetPlayerStat (pc, IE_CLASS) - 1, 0)

	Label = Window.GetControl (0x1000003f)
	Label.SetText (text)

	# update ground inventory slots
	Container = GemRB.GetContainer(pc, 1)
	TopIndex = GemRB.GetVar ("TopIndex")
	for i in range (10):
		Button = Window.GetControl (i+47)
		if GemRB.IsDraggingItem ():
			Button.SetState (IE_GUI_BUTTON_SECOND)
		else:
			Button.SetState (IE_GUI_BUTTON_ENABLED)
		Button.SetEvent (IE_GUI_BUTTON_ON_DRAG_DROP, "OnDragItemGround")
		Slot = GemRB.GetContainerItem (pc, i+TopIndex)
		if Slot != None:
			item = GemRB.GetItem (Slot['ItemResRef'])
			identified = Slot["Flags"] & IE_INV_ITEM_IDENTIFIED

			Button.SetItemIcon (Slot['ItemResRef'])
			Button.SetFlags (IE_GUI_BUTTON_PICTURE, OP_OR)
			if not identified or item["ItemNameIdentified"] == -1:
				Button.SetTooltip (item["ItemName"])
				Button.EnableBorder (0, 1)
			else:
				Button.SetTooltip (item["ItemNameIdentified"])
				Button.EnableBorder (0, 0)
			Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "OnDragItemGround")
			Button.SetEvent (IE_GUI_BUTTON_ON_RIGHT_PRESS, "OpenGroundItemInfoWindow")
			Button.SetEvent (IE_GUI_BUTTON_ON_SHIFT_PRESS, "OpenGroundItemAmountWindow")

		else:
			Button.SetFlags (IE_GUI_BUTTON_PICTURE, OP_NAND)
			Button.SetTooltip (4273)
			Button.EnableBorder (0, 0)
			Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "")
			Button.SetEvent (IE_GUI_BUTTON_ON_RIGHT_PRESS, "")
			Button.SetEvent (IE_GUI_BUTTON_ON_SHIFT_PRESS, "")
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
		if item['StackAmount'] > 1:
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
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "OnDragItem")
		Button.SetEvent (IE_GUI_BUTTON_ON_RIGHT_PRESS, "OpenItemInfoWindow")
		Button.SetEvent (IE_GUI_BUTTON_ON_SHIFT_PRESS, "OpenItemAmountWindow")
		Button.SetEvent (IE_GUI_BUTTON_ON_DRAG_DROP, "OnDragItem")
	else:
		Button.SetItemIcon ('')
		Button.SetText ('')

		if slot_list[i]>=0:
			Button.SetSprites (SlotType["ResRef"], 0, 0, 1, 2, 3)
			Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_NAND)
			Button.SetEvent (IE_GUI_BUTTON_ON_DRAG_DROP, "OnDragItem")
			Button.SetTooltip (SlotType["Tip"])
		else:
			Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_OR)
			Button.SetEvent (IE_GUI_BUTTON_ON_DRAG_DROP, '')
			Button.SetTooltip ('')
			itemname = ""

		Button.EnableBorder (0, 0)
		Button.EnableBorder (1, 0)
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "")
		Button.SetEvent (IE_GUI_BUTTON_ON_RIGHT_PRESS, "")
		Button.SetEvent (IE_GUI_BUTTON_ON_SHIFT_PRESS, "")

	if OverSlot == slot-1:
		if GemRB.CanUseItemType (SlotType["Type"], itemname):
			Button.SetState (IE_GUI_BUTTON_SELECTED)
		else:
			Button.SetState (IE_GUI_BUTTON_ENABLED)
	else:
		if (SlotType["Type"]&SLOT_INVENTORY) or not GemRB.CanUseItemType (SlotType["Type"], itemname):
			Button.SetState (IE_GUI_BUTTON_ENABLED)
		else:
			Button.SetState (IE_GUI_BUTTON_SECOND)

		if slot_item and (GemRB.GetEquippedQuickSlot (pc)==slot or GemRB.GetEquippedAmmunition (pc)==slot):
			Button.SetState (IE_GUI_BUTTON_THIRD)
	return

def OnDragItemGround ():
	pc = GemRB.GameGetSelectedPCSingle ()

	slot = GemRB.GetVar ("GroundItemButton") + GemRB.GetVar ("TopIndex")
	if not GemRB.IsDraggingItem ():
		slot_item = GemRB.GetContainerItem (pc, slot)
		item = GemRB.GetItem (slot_item["ItemResRef"])
		GemRB.DragItem (pc, slot, item["ItemIcon"], 0, 1) #container
		GemRB.PlaySound (item["DescIcon"])
	else:
		GemRB.DropDraggedItem (pc, -2) #dropping on ground

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


def OnDropItemToPC ():
	pc = GemRB.GetVar ("PressedPortrait") + 1
	GemRB.DropDraggedItem (pc, -3)
	UpdateInventoryWindow ()
	return

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

	ItemAmountWindow = Window = GemRB.LoadWindowObject (4)
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
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "DragItemAmount")
	Button.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)

	# Cancel
	Button = Window.GetControl (1)
	Button.SetText (4196)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "OpenItemAmountWindow")
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
	#one for close, one for reopen
	CloseItemInfoWindow ()
	OpenItemInfoWindow ()
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
	#one for close, one for reopen
	CloseItemInfoWindow ()
	OpenItemInfoWindow ()
	return

def DrinkItemWindow ():
	pc = GemRB.GameGetSelectedPCSingle ()
	slot = GemRB.GetVar ("ItemButton")
	# the drink item header is always the first
	GemRB.UseItem (pc, slot, 0)
	if ItemInfoWindow:
		ItemInfoWindow.Unload ()
	return

def ReadItemWindow ():
	pc = GemRB.GameGetSelectedPCSingle ()
	slot = GemRB.GetVar ("ItemButton")
	# the learn scroll header is always the second
	GemRB.UseItem (pc, slot, 1)
	if ItemInfoWindow:
		ItemInfoWindow.Unload ()
	return

def OpenItemWindow ():
	#close inventory
	GemRB.SetVar ("Inventory", 1)
	if ItemInfoWindow:
		ItemInfoWindow.Unload ()
	OpenInventoryWindow ()
	pc = GemRB.GameGetSelectedPCSingle ()
	slot = GemRB.GetVar ("ItemButton")
	slot_item = GemRB.GetSlotItem (pc, slot)
	ResRef = slot_item['ItemResRef']
	#the store will have to reopen the inventory
	GemRB.EnterStore (ResRef)
	return

def DialogItemWindow ():

	CloseItemInfoWindow ()
	OpenInventoryWindow ()
	pc = GemRB.GameGetSelectedPCSingle ()
	slot, slot_item = ItemHash[GemRB.GetVar ('ItemButton')]
	ResRef = slot_item['ItemResRef']
	item = GemRB.GetItem (ResRef)
	dialog=item['Dialog']
	GemRB.ExecuteString ("StartDialog(\""+dialog+"\",Myself)", pc)
	return

def IdentifyUseSpell ():
	global ItemIdentifyWindow

	pc = GemRB.GameGetSelectedPCSingle ()
	slot = GemRB.GetVar ("ItemButton")
	if ItemIdentifyWindow:
		ItemIdentifyWindow.Unload ()
	GemRB.HasSpecialSpell (pc, 1, 1)
	GemRB.ChangeItemFlag (pc, slot, IE_INV_ITEM_IDENTIFIED, OP_OR)
	CloseIdentifyItemWindow()
	return

def IdentifyUseScroll ():
	global ItemIdentifyWindow

	pc = GemRB.GameGetSelectedPCSingle ()
	slot = GemRB.GetVar ("ItemButton")
	if ItemIdentifyWindow:
		ItemIdentifyWindow.Unload ()
	if GemRB.HasSpecialItem (pc, 1, 1):
		GemRB.ChangeItemFlag (pc, slot, IE_INV_ITEM_IDENTIFIED, OP_OR)
	CloseIdentifyItemWindow()
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

	ItemIdentifyWindow = Window = GemRB.LoadWindowObject (9)

	# how would you like to identify ....
	Text = Window.GetControl (3)
	Text.SetText (4258)

	# Spell
	Button = Window.GetControl (0)
	Button.SetText (4259)
	if not GemRB.HasSpecialSpell (pc, 1, 0):
		Button.SetState (IE_GUI_BUTTON_DISABLED)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "IdentifyUseSpell")

	# Scroll
	Button = Window.GetControl (1)
	Button.SetText (4260)
	if not GemRB.HasSpecialItem (pc, 1, 0):
		Button.SetState (IE_GUI_BUTTON_DISABLED)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "IdentifyUseScroll")

	# Cancel
	Button = Window.GetControl (2)
	Button.SetText (4196)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "CloseIdentifyItemWindow")
	Button.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)

	Window.ShowModal (MODAL_SHADOW_GRAY)
	return

def CloseItemInfoWindow ():
	if ItemInfoWindow:
		ItemInfoWindow.Unload ()
	UpdateInventoryWindow()
	return

def DisplayItem (itemresref, type):
	global ItemInfoWindow

	item = GemRB.GetItem (itemresref)
	ItemInfoWindow = Window = GemRB.LoadWindowObject (5)

	# Done
	Button = Window.GetControl (4)
	Button.SetText (1403)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "CloseItemInfoWindow")

	# Identify
	Button = Window.GetControl (8)
	if type&2:
		Button.SetText (4256)
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "IdentifyItemWindow")
	else:
		Button.SetState (IE_GUI_BUTTON_LOCKED)
		Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_SET)

	# Talk to Item/Use
	Button = Window.GetControl (9)
	drink = (type&1) and (item["Function"]&1)
	read = (type&1) and (item["Function"]&2)
	container = (type&1) and (item["Function"]&4)
	dialog = (type&1) and (item["Dialog"]!="")

	Button.SetState (IE_GUI_BUTTON_ENABLED)
	Button.SetFlags (IE_GUI_BUTTON_NORMAL, OP_SET)
	if drink:
		#no drink/food in PST, this is the closest strref in stock pst
		Button.SetText (4251)
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "DrinkItemWindow")
	elif read:
		Button.SetText (4252)
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "ReadItemWindow")
	elif container:
		#no containers in PST, this is the closest strref in stock pst
		Button.SetText (52399)
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "OpenItemWindow")
	elif dialog:
		Button.SetText (4254)
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "DialogItemWindow")
	else:
		Button.SetText ('')
		Button.SetState (IE_GUI_BUTTON_LOCKED)
		Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_SET)

	Label = Window.GetControl (0x10000000)
	if type&2:
		Label.SetText (GemRB.GetString (item['ItemName']).upper())
	else:
		Label.SetText (GemRB.GetString (item['ItemNameIdentified']).upper())

	Icon = Window.GetControl (2)
	Icon.SetFlags (IE_GUI_BUTTON_PICTURE | IE_GUI_BUTTON_NO_IMAGE, OP_SET)
	Icon.SetItemIcon (itemresref)

	Text = Window.GetControl (5)
	if type&2:
		Text.SetText (item['ItemDesc'])
	else:
		Text.SetText (item['ItemDescIdentified'])

	IdText = Window.GetControl (0x1000000b)
	if type&2:
		# 4279 - This item has not been identified
		IdText.SetText (4279)
	else:
		IdText.SetText ('')

	Icon = Window.GetControl (7)
	Icon.SetFlags (IE_GUI_BUTTON_PICTURE | IE_GUI_BUTTON_NO_IMAGE, OP_SET)
	Icon.SetItemIcon (itemresref, 1)

	#left scroll
	Button = Window.GetControl (13)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "LeftItemScroll")

	#right scroll
	Button = Window.GetControl (14)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "RightItemScroll")

	Window.ShowModal (MODAL_SHADOW_GRAY)
	return

def OpenItemInfoWindow ():
	pc = GemRB.GameGetSelectedPCSingle ()
	slot, slot_item = ItemHash[GemRB.GetVar ('ItemButton')]
	ResRef = slot_item['ItemResRef']
	item = GemRB.GetItem (slot_item["ItemResRef"])

	#auto identify when lore is high enough
	if item["LoreToID"]<=GemRB.GetPlayerStat (pc, IE_LORE):
		GemRB.ChangeItemFlag (pc, slot, IE_INV_ITEM_IDENTIFIED, OP_OR)
		slot_item["Flags"] |= IE_INV_ITEM_IDENTIFIED
		UpdateInventoryWindow ()

	if slot_item["Flags"] & IE_INV_ITEM_IDENTIFIED:
		value = 1
	else:
		value = 3
	DisplayItem (slot_item["ItemResRef"], value)
	return

def OpenGroundItemInfoWindow ():
	global ItemInfoWindow

	pc = GemRB.GameGetSelectedPCSingle ()
	slot = GemRB.GetVar ("TopIndex") + GemRB.GetVar ("GroundItemButton")
	slot_item = GemRB.GetContainerItem (pc, slot)

	#the ground items are only displayable
	if slot_item["Flags"] & IE_INV_ITEM_IDENTIFIED:
		value = 0
	else:
		value = 2
	DisplayItem (slot_item["ItemResRef"], value)
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

def MouseEnterGround ():
	Window = InventoryWindow
	i = GemRB.GetVar ("GroundItemButton")
	Button = Window.GetControl (i+47)
	if GemRB.IsDraggingItem ():
		Button.SetState (IE_GUI_BUTTON_SELECTED)
	return

def MouseLeaveGround ():
	Window = InventoryWindow
	i = GemRB.GetVar ("GroundItemButton")
	Button = Window.GetControl (i+47)
	if GemRB.IsDraggingItem ():
		Button.SetState (IE_GUI_BUTTON_SECOND)
	return

###################################################
# End of file GUIINV.py
