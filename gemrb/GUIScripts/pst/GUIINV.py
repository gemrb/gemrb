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

	# Ground Item
	for i in range (47, 57):
		Button = GemRB.GetControl (Window, i)
		GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_ALIGN_RIGHT , OP_OR)
		GemRB.SetButtonFont (Window, Button, "NUMBER2")
		GemRB.SetButtonBorder (Window, Button, 0,0,0,0,0,128,128,255,64,0,1)
		GemRB.SetButtonBorder (Window, Button, 1,0,0,0,0,255,128,128,64,0,1)

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

	SetSelectionChangeHandler (UpdateInventoryWindow)
	UpdateInventoryWindow ()

	GemRB.UnhideGUI()


def UpdateInventoryWindow ():
	global ItemHash
	global slot_list

	Window = InventoryWindow

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
	#usability = GemRB.GetTableValue (AvSlotsTable, row, 0)
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
		GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_DRAG_DROP, "OnDragItemGround")
		Slot = GemRB.GetContainerItem (pc, i+TopIndex)
		if Slot != None:
			Item = GemRB.GetItem (Slot['ItemResRef'])
			identified = Slot["Flags"] & IE_INV_ITEM_IDENTIFIED
			GemRB.SetItemIcon (Window, Button, Slot['ItemResRef'],0)
			GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_PICTURE, OP_OR)
			if not identified or Item["ItemNameIdentified"] == -1:
				GemRB.SetTooltip (Window, Button, Item["ItemName"])
				GemRB.EnableButtonBorder (Window, Button, 0, 1)
			else:
				GemRB.SetTooltip (Window, Button, Item["ItemNameIdentified"])
				GemRB.EnableButtonBorder (Window, Button, 0, 0)
			GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OnDragItemGround")
			GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_RIGHT_PRESS, "OpenItemInfoGroundWindow")
			GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_SHIFT_PRESS, "OpenItemAmountGroundWindow")

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

	Button = GemRB.GetControl (Window, ControlID)
	ItemHash[ControlID] = [slot, slot_item]
	GemRB.SetButtonSprites (Window, Button, 'IVSLOT', 0, 0, 0, 0, 0)
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
			GemRB.SetButtonSprites (Window, Button, SlotType["ResRef"], 0, 0, 0, 0, 0)
			GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_NO_IMAGE, OP_NAND)
			GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_DRAG_DROP, "OnDragItem")
			GemRB.SetTooltip (Window, Button, SlotType["Tip"])
		else:
			GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_NO_IMAGE, OP_OR)
			GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_DRAG_DROP, '')
			GemRB.SetTooltip (Window, Button, '')

		GemRB.EnableButtonBorder (Window, Button, 0, 0)
		GemRB.EnableButtonBorder (Window, Button, 1, 0)
		GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "")
		GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_RIGHT_PRESS, "")
		GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_SHIFT_PRESS, "")
	return

def OnDragItemGround ():
	pc = GemRB.GameGetSelectedPCSingle ()

	slot = GemRB.GetVar ("GroundItemButton")
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

	GemRB.DropDraggedItem (pc, -1)
	#item = GemRB.GetItem (ResRef)
	#GemRB.PlaySound (item["BrokenItem"])
	UpdateInventoryWindow ()
	return

def OnDragItem ():
	pc = GemRB.GameGetSelectedPCSingle ()

	slot, slot_item = ItemHash[GemRB.GetVar ('ItemButton')]

	if not GemRB.IsDraggingItem ():
		ResRef = slot_item['ItemResRef']
		item = GemRB.GetItem (ResRef)
		GemRB.DragItem (pc, slot, item["ItemIcon"], 0, 0)
	else:
		GemRB.DropDraggedItem (pc, slot)

	UpdateInventoryWindow ()

def OnDropItemToPC ():
	pc = GemRB.GetVar ("PressedPortrait") + 1
	GemRB.DropDraggedItem (pc, -3)
	UpdateInventoryWindow ()


def OpenItemInfoWindow ():
	global ItemInfoWindow

	GemRB.HideGUI ()

	if ItemInfoWindow != None:
		GemRB.UnloadWindow (ItemInfoWindow)
		ItemInfoWindow = None
		GemRB.SetVar ("FloatWindow", -1)

		GemRB.UnhideGUI ()
		return

	ItemInfoWindow = Window = GemRB.LoadWindow (5)
	GemRB.SetVar ("FloatWindow", ItemInfoWindow)

	slot, slot_item = ItemHash[GemRB.GetVar ('ItemButton')]

	identified = slot_item['Flags'] & IE_INV_ITEM_IDENTIFIED

	ResRef = slot_item['ItemResRef']
	item = GemRB.GetItem (ResRef)

	# Done
	Button = GemRB.GetControl (Window, 4)
	GemRB.SetText (Window, Button, 1403)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenItemInfoWindow")

	# Identify
	Button = GemRB.GetControl (Window, 8)
	GemRB.SetText (Window, Button, 4256)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenItemIdentifyWindow")
	# FIXME: the button should be hidden if not needed
	if identified:
		GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_DISABLED)
	else:
		GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_ENABLED)

	# Talk to Item/Use
	Button = GemRB.GetControl (Window, 9)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenItemDialogWindow")
	if item['Dialog']:
		GemRB.SetText (Window, Button, 4254)
		GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_ENABLED)
	else:
		GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_DISABLED)

	Label = GemRB.GetControl (Window, 0x10000000)
	if not identified or item['ItemNameIdentified'] == -1:
		GemRB.SetText (Window, Label, string.upper (GemRB.GetString (item['ItemName'])))
	else:
		GemRB.SetText (Window, Label, string.upper (GemRB.GetString (item['ItemNameIdentified'])))

	Icon = GemRB.GetControl (Window, 2)
	GemRB.SetButtonFlags (Window, Icon, IE_GUI_BUTTON_PICTURE | IE_GUI_BUTTON_NO_IMAGE, OP_SET)
	GemRB.SetItemIcon (Window, Icon, ResRef)

	Text = GemRB.GetControl (Window, 5)
	if not identified or item['ItemDescIdentified'] == -1:
		GemRB.SetText (Window, Text, item['ItemDesc'])
	else:
		GemRB.SetText (Window, Text, item['ItemDescIdentified'])

	IdText = GemRB.GetControl (Window, 0x1000000b)
	if identified:
		GemRB.SetText (Window, IdText, '')
	else:
		# 4279 - This item has not been identified
		GemRB.SetText (Window, IdText, 4279)

	Icon = GemRB.GetControl (Window, 7)
	GemRB.SetButtonFlags (Window, Icon, IE_GUI_BUTTON_PICTURE | IE_GUI_BUTTON_NO_IMAGE, OP_SET)
	GemRB.SetItemIcon (Window, Icon, ResRef, 1)

	# 9, 8, 4

	GemRB.UnhideGUI ()
	GemRB.ShowModal (Window, MODAL_SHADOW_GRAY)

# 4254 Talk to item/4251 Use 4256 Identify Done
# 4252 Copy Spell (also 4250, 4249)
# 4253 eat berries

# 4258 - How would you like to identify
# 4259 Spell/4260 Scroll//Cancel

# amnt dialog:
# Done/Cancel

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


def OpenItemDialogWindow ():

	OpenItemInfoWindow ()
	OpenInventoryWindow ()
	pc = GemRB.GameGetSelectedPCSingle ()
	slot, slot_item = ItemHash[GemRB.GetVar ('ItemButton')]
	ResRef = slot_item['ItemResRef']
	item = GemRB.GetItem (ResRef)
	dialog=item['Dialog']
	GemRB.ExecuteString ("StartDialog(\""+dialog+"\",Myself)", pc)

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

def OpenItemIdentifyWindow ():
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

###################################################
# End of file GUIINV.py
