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
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
#
# $Id$


# GUIINV.py - scripts to control inventory windows from GUIINV winpack

###################################################

import string

from GUIDefines import *
from ie_stats import *
from ie_slots import *
import GemRB
from GUICommon import CloseOtherWindow
from GUICommonWindows import GetActorPortrait, SetSelectionChangeHandler, SetEncumbranceButton


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

	AvSlotsTable = GemRB.LoadTable ('avslots')

	if CloseOtherWindow (OpenInventoryWindow):
		GemRB.HideGUI ()
		GemRB.UnloadWindow (InventoryWindow)
		InventoryWindow = None
		GemRB.SetVar ("OtherWindow", -1)
		GemRB.SetVar ("MessageLabel", -1)
		SetSelectionChangeHandler (None)
		GemRB.UnhideGUI ()
		return

	GemRB.HideGUI ()
	GemRB.LoadWindowPack ("GUIINV")
	InventoryWindow = Window = GemRB.LoadWindow (3)
	GemRB.SetVar ("OtherWindow", InventoryWindow)
	GemRB.SetVar ("MessageLabel", GemRB.GetControl(Window, 0x1000003d))

	# inventory slots
	for i in range (44):
		Button = GemRB.GetControl (Window, i)
		GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_ALIGN_RIGHT , OP_OR)
		GemRB.SetButtonFont (Window, Button, "NUMBER2")
		GemRB.SetVarAssoc (Window, Button, "ItemButton", i)
		GemRB.SetButtonBorder (Window, Button, 0,0,0,0,0,128,128,255,64,0,1)
		GemRB.SetButtonBorder (Window, Button, 1,0,0,0,0,255,128,128,64,0,1)
		GemRB.SetEvent (Window, Button, IE_GUI_MOUSE_ENTER_BUTTON, "MouseEnterSlot")
		GemRB.SetEvent (Window, Button, IE_GUI_MOUSE_LEAVE_BUTTON, "MouseLeaveSlot")

	# Ground Item
	for i in range (10):
		Button = GemRB.GetControl (Window, i+47)
		GemRB.SetVarAssoc (Window, Button, "GroundItemButton", i)
		GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_ALIGN_RIGHT , OP_OR)
		GemRB.SetButtonFont (Window, Button, "NUMBER2")
		GemRB.SetButtonBorder (Window, Button, 0,0,0,0,0,128,128,255,64,0,1)
		GemRB.SetButtonBorder (Window, Button, 1,0,0,0,0,255,128,128,64,0,1)
		GemRB.SetEvent (Window, Button, IE_GUI_MOUSE_ENTER_BUTTON, "MouseEnterGround")
		GemRB.SetEvent (Window, Button, IE_GUI_MOUSE_LEAVE_BUTTON, "MouseLeaveGround")

	ScrollBar = GemRB.GetControl (Window, 45)
	GemRB.SetEvent (Window, ScrollBar, IE_GUI_SCROLLBAR_ON_CHANGE, "RefreshInventoryWindow")

	for i in range (57, 64):
		Label = GemRB.GetControl (Window, 0x10000000 + i)
		GemRB.SetText (Window, Label, str (i))

	# portrait
	Button = GemRB.GetControl (Window, 44)
	GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_LOCKED)
	GemRB.SetButtonFlags(Window, Button, IE_GUI_BUTTON_NO_IMAGE | IE_GUI_BUTTON_PICTURE, OP_SET)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_DRAG_DROP, "OnAutoEquip")

	# encumbrance
	Button = GemRB.GetControl (Window, 46)
	GemRB.SetButtonFont (Window, Button, "NUMBER")
	GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_LOCKED)
	GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_NO_IMAGE, OP_SET)

	# armor class
	Label = GemRB.GetControl (Window, 0x1000003a)
	GemRB.SetTooltip (Window, Label, 4197)

	# hp current
	Label = GemRB.GetControl (Window, 0x1000003b)
	GemRB.SetTooltip (Window, Label, 4198)

	# hp max
	Label = GemRB.GetControl (Window, 0x1000003c)
	GemRB.SetTooltip (Window, Label, 4199)

	# info label, game paused, etc
	Label = GemRB.GetControl (Window, 0x1000003d)
	GemRB.SetLabelTextColor(Window, Label, 255, 255, 255)
	GemRB.SetText (Window, Label, "")
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
	ScrollBar = GemRB.GetControl (Window, 45)
	Count = Container['ItemCount']
	if Count<1:
		Count=1
	GemRB.SetVarAssoc (Window, ScrollBar, "TopIndex", Count)
	RefreshInventoryWindow ()
	# And now for the items ....

	ItemHash = {}

	# get a list which maps slot number to slot type/icon/tooltip
	row = GemRB.GetPlayerStat (pc, IE_SPECIFIC)
	slot_list = map (int, string.split (GemRB.GetTableValue (AvSlotsTable, row, 1, 0), ','))

	# populate inventory slot controls
	for i in range (46):
		UpdateSlot (pc, i)

def RefreshInventoryWindow ():
	Window = InventoryWindow
	pc = GemRB.GameGetSelectedPCSingle ()

	# name
	Label = GemRB.GetControl (Window, 0x10000039)
	GemRB.SetText (Window, Label, GemRB.GetPlayerName (pc, 1))


	# portrait
	Button = GemRB.GetControl (Window, 44)
	GemRB.SetButtonPicture (Window, Button, GetActorPortrait (pc, 'INVENTORY'))

	SetEncumbranceButton (Window, 46, pc)

	# armor class
	ac = GemRB.GetPlayerStat (pc, IE_ARMORCLASS)
	Label = GemRB.GetControl (Window, 0x1000003a)
	GemRB.SetText (Window, Label, str (ac))

	# hp current
	hp = GemRB.GetPlayerStat (pc, IE_HITPOINTS)
	Label = GemRB.GetControl (Window, 0x1000003b)
	GemRB.SetText (Window, Label, str (hp))

	# hp max
	hpmax = GemRB.GetPlayerStat (pc, IE_MAXHITPOINTS)
	Label = GemRB.GetControl (Window, 0x1000003c)
	GemRB.SetText (Window, Label, str (hpmax))

	# party gold
	Label = GemRB.GetControl (Window, 0x1000003e)
	GemRB.SetText (Window, Label, str (GemRB.GameGetPartyGold ()))

	# class
	ClassTable = GemRB.LoadTable ("classes")
	text = GemRB.GetTableValue (ClassTable, GemRB.GetPlayerStat (pc, IE_CLASS) - 1, 0)
	GemRB.UnloadTable (ClassTable)

	Label = GemRB.GetControl (Window, 0x1000003f)
	GemRB.SetText (Window, Label, text)

	# update ground inventory slots
	Container = GemRB.GetContainer(pc, 1)
	TopIndex = GemRB.GetVar ("TopIndex")
	for i in range (10):
		Button = GemRB.GetControl (Window, i+47)
		if GemRB.IsDraggingItem ():
			GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_SECOND)
		else:
			GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_ENABLED)
		GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_DRAG_DROP, "OnDragItemGround")
		Slot = GemRB.GetContainerItem (pc, i+TopIndex)
		if Slot != None:
			item = GemRB.GetItem (Slot['ItemResRef'])
			identified = Slot["Flags"] & IE_INV_ITEM_IDENTIFIED

			GemRB.SetItemIcon (Window, Button, Slot['ItemResRef'])
			GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_PICTURE, OP_OR)
			if not identified or item["ItemNameIdentified"] == -1:
				GemRB.SetTooltip (Window, Button, item["ItemName"])
				GemRB.EnableButtonBorder (Window, Button, 0, 1)
			else:
				GemRB.SetTooltip (Window, Button, item["ItemNameIdentified"])
				GemRB.EnableButtonBorder (Window, Button, 0, 0)
			GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OnDragItemGround")
			GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_RIGHT_PRESS, "OpenGroundItemInfoWindow")
			GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_SHIFT_PRESS, "OpenGroundItemAmountWindow")

		else:
			GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_PICTURE, OP_NAND)
			GemRB.SetTooltip (Window, Button, 4273)
			GemRB.EnableButtonBorder (Window, Button, 0, 0)
			GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "")
			GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_RIGHT_PRESS, "")
			GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_SHIFT_PRESS, "")
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

	Button = GemRB.GetControl (Window, ControlID)
	ItemHash[ControlID] = [slot, slot_item]
	GemRB.SetButtonSprites (Window, Button, 'IVSLOT', 0, 0, 1, 2, 3)
	if slot_item:
		item = GemRB.GetItem (slot_item['ItemResRef'])
		identified = slot_item['Flags'] & IE_INV_ITEM_IDENTIFIED

		GemRB.SetItemIcon (Window, Button, slot_item['ItemResRef'])
		if item['StackAmount'] > 1:
			GemRB.SetText (Window, Button, str (slot_item['Usages0']))
		else:
			GemRB.SetText (Window, Button, '')

		if not identified or item['ItemNameIdentified'] == -1:
			GemRB.SetTooltip (Window, Button, item['ItemName'])
			GemRB.EnableButtonBorder (Window, Button, 0, 1)
		else:
			GemRB.SetTooltip (Window, Button, item['ItemNameIdentified'])
			GemRB.EnableButtonBorder (Window, Button, 0, 0)

		if GemRB.CanUseItemType (SLOT_ANY, slot_item['ItemResRef'], pc):
			GemRB.EnableButtonBorder (Window, Button, 1, 0)
		else:
			GemRB.EnableButtonBorder (Window, Button, 1, 1)

		GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_NO_IMAGE, OP_NAND)
		GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OnDragItem")
		GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_RIGHT_PRESS, "OpenItemInfoWindow")
		GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_SHIFT_PRESS, "OpenItemAmountWindow")
		GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_DRAG_DROP, "OnDragItem")
	else:
		GemRB.SetItemIcon (Window, Button, '')
		GemRB.SetText (Window, Button, '')

		if slot_list[i]>=0:
			GemRB.SetButtonSprites (Window, Button, SlotType["ResRef"], 0, 0, 1, 2, 3)
			GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_NO_IMAGE, OP_NAND)
			GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_DRAG_DROP, "OnDragItem")
			GemRB.SetTooltip (Window, Button, SlotType["Tip"])
		else:
			GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_NO_IMAGE, OP_OR)
			GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_DRAG_DROP, '')
			GemRB.SetTooltip (Window, Button, '')
			itemname = ""

		GemRB.EnableButtonBorder (Window, Button, 0, 0)
		GemRB.EnableButtonBorder (Window, Button, 1, 0)
		GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "")
		GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_RIGHT_PRESS, "")
		GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_SHIFT_PRESS, "")

	if OverSlot == slot-1:
		if GemRB.CanUseItemType (SlotType["Type"], itemname):
			GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_SELECTED)
		else:
			GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_ENABLED)
	else:
		if (SlotType["Type"]&SLOT_INVENTORY) or not GemRB.CanUseItemType (SlotType["Type"], itemname):
			GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_ENABLED)
		else:
			GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_SECOND)

		if slot_item and (GemRB.GetEquippedQuickSlot (pc)==slot or GemRB.GetEquippedAmmunition (pc)==slot):
			GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_THIRD)
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

def OpenItemAmountWindow ():
	global ItemAmountWindow

	GemRB.HideGUI ()

	if ItemAmountWindow != None:
		GemRB.UnloadWindow (ItemAmountWindow)
		ItemAmountWindow = None
		GemRB.SetVar ("FloatWindow", -1)

		GemRB.UnhideGUI ()
		return

	ItemAmountWindow = Window = GemRB.LoadWindow (4)
	GemRB.SetVar ("FloatWindow", ItemAmountWindow)

	ResRef = slot_item['ItemResRef']
	item = GemRB.GetItem (ResRef)

	# item icon
	Icon = GemRB.GetControl (Window, 0)
	GemRB.SetButtonFlags (Window, Icon, IE_GUI_BUTTON_PICTURE | IE_GUI_BUTTON_NO_IMAGE, OP_SET)
	GemRB.SetItemIcon (Window, Icon, ResRef)

	# item amount
	Text = GemRB.GetControl (Window, 6)
	GemRB.SetControlSize (Window, Text, 40, 40)
	GemRB.SetText (Window, Text, "1")

	# Done
	Button = GemRB.GetControl (Window, 2)
	GemRB.SetText (Window, Button, 1403)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenItemAmountWindow")

	# Cancel
	Button = GemRB.GetControl (Window, 1)
	GemRB.SetText (Window, Button, 4196)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenItemAmountWindow")

	# 0 bmp
	# 1,2 done/cancel?
	# 3 +
	# 4 -
	# 6 text

	GemRB.UnhideGUI ()
	GemRB.ShowModal (Window, MODAL_SHADOW_GRAY)

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
	GemRB.UnloadWindow (ItemInfoWindow)
	return

def ReadItemWindow ():
	pc = GemRB.GameGetSelectedPCSingle ()
	slot = GemRB.GetVar ("ItemButton")
	# the learn scroll header is always the second
	GemRB.UseItem (pc, slot, 1)
	GemRB.UnloadWindow (ItemInfoWindow)
	return

def OpenItemWindow ():
	#close inventory
	GemRB.SetVar ("Inventory", 1)
	GemRB.UnloadWindow (ItemInfoWindow)
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
	GemRB.UnloadWindow (ItemIdentifyWindow)
	GemRB.HasSpecialSpell (pc, 1, 1)
	GemRB.ChangeItemFlag (pc, slot, IE_INV_ITEM_IDENTIFIED, OP_OR)
	CloseIdentifyItemWindow()
	return

def IdentifyUseScroll ():
	global ItemIdentifyWindow

	pc = GemRB.GameGetSelectedPCSingle ()
	slot = GemRB.GetVar ("ItemButton")
	GemRB.UnloadWindow (ItemIdentifyWindow)
	GemRB.HasSpecialItem (pc, 1, 1)
	GemRB.ChangeItemFlag (pc, slot, IE_INV_ITEM_IDENTIFIED, OP_OR)
	CloseIdentifyItemWindow()
	return

def CloseIdentifyItemWindow ():
	global ItemIdentifyWindow

	GemRB.UnloadWindow (ItemIdentifyWindow)
	ItemIdentifyWindow = None
	return

def IdentifyItemWindow ():
	global ItemIdentifyWindow

	pc = GemRB.GameGetSelectedPCSingle ()

	ItemIdentifyWindow = Window = GemRB.LoadWindow (9)

	# how would you like to identify ....
	Text = GemRB.GetControl (Window, 3)
	GemRB.SetText (Window, Text, 4258)

	# Spell
	Button = GemRB.GetControl (Window, 0)
	GemRB.SetText (Window, Button, 4259)
	if not GemRB.HasSpecialSpell (pc, 1, 0):
		GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_DISABLED)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "IdentifyUseSpell")

	# Scroll
	Button = GemRB.GetControl (Window, 1)
	GemRB.SetText (Window, Button, 4260)
	if not GemRB.HasSpecialItem (pc, 1, 0):
		GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_DISABLED)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "IdentifyUseScroll")

	# Cancel
	Button = GemRB.GetControl (Window, 2)
	GemRB.SetText (Window, Button, 4196)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "CloseIdentifyItemWindow")

	GemRB.ShowModal (Window, MODAL_SHADOW_GRAY)
	return

def CloseItemInfoWindow ():
	GemRB.UnloadWindow (ItemInfoWindow)
	UpdateInventoryWindow()
	return

def DisplayItem (itemresref, type):
	global ItemInfoWindow

	item = GemRB.GetItem (itemresref)
	ItemInfoWindow = Window = GemRB.LoadWindow (5)

	# Done
	Button = GemRB.GetControl (Window, 4)
	GemRB.SetText (Window, Button, 1403)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "CloseItemInfoWindow")

	# Identify
	Button = GemRB.GetControl (Window, 8)
	if type&2:
		GemRB.SetText (Window, Button, 4256)
		GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "IdentifyItemWindow")
	else:
		GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_LOCKED)
		GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_NO_IMAGE, OP_SET)

	# Talk to Item/Use
	Button = GemRB.GetControl (Window, 9)
	drink = (type&1) and (item["Function"]&1)
	read = (type&1) and (item["Function"]&2)
	container = (type&1) and (item["Function"]&4)
	dialog = (type&1) and (item["Dialog"]!="")

	GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_ENABLED)
	GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_NORMAL, OP_SET)
	if drink:
		#no drink/food in PST, this is the closest strref in stock pst
		GemRB.SetText (Window, Button, 4251)
		GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "DrinkItemWindow")
	elif read:
		GemRB.SetText (Window, Button, 4252)
		GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "ReadItemWindow")
	elif container:
		#no containers in PST, this is the closest strref in stock pst
		GemRB.SetText (Window, Button, 52399)
		GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenItemWindow")
	elif dialog:
		GemRB.SetText (Window, Button, 4254)
		GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "DialogItemWindow")
	else:
		GemRB.SetText (Window, Button, '')
		GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_LOCKED)
		GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_NO_IMAGE, OP_SET)

	Label = GemRB.GetControl (Window, 0x10000000)
	if type&2:
		GemRB.SetText (Window, Label, string.upper (GemRB.GetString (item['ItemName'])))
	else:
		GemRB.SetText (Window, Label, string.upper (GemRB.GetString (item['ItemNameIdentified'])))

	Icon = GemRB.GetControl (Window, 2)
	GemRB.SetButtonFlags (Window, Icon, IE_GUI_BUTTON_PICTURE | IE_GUI_BUTTON_NO_IMAGE, OP_SET)
	GemRB.SetItemIcon (Window, Icon, itemresref)

	Text = GemRB.GetControl (Window, 5)
	if type&2:
		GemRB.SetText (Window, Text, item['ItemDesc'])
	else:
		GemRB.SetText (Window, Text, item['ItemDescIdentified'])

	IdText = GemRB.GetControl (Window, 0x1000000b)
	if type&2:
		# 4279 - This item has not been identified
		GemRB.SetText (Window, IdText, 4279)
	else:
		GemRB.SetText (Window, IdText, '')

	Icon = GemRB.GetControl (Window, 7)
	GemRB.SetButtonFlags (Window, Icon, IE_GUI_BUTTON_PICTURE | IE_GUI_BUTTON_NO_IMAGE, OP_SET)
	GemRB.SetItemIcon (Window, Icon, itemresref, 1)

	#left scroll
	Button = GemRB.GetControl (Window, 13)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "LeftItemScroll")

	#right scroll
	Button = GemRB.GetControl (Window, 14)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "RightItemScroll")

	GemRB.ShowModal (Window, MODAL_SHADOW_GRAY)
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
	Button = GemRB.GetControl (Window, i+47)
	if GemRB.IsDraggingItem ():
		GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_SELECTED)
	return

def MouseLeaveGround ():
	Window = InventoryWindow
	i = GemRB.GetVar ("GroundItemButton")
	Button = GemRB.GetControl (Window, i+47)
	if GemRB.IsDraggingItem ():
		GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_SECOND)
	return

###################################################
# End of file GUIINV.py
