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

# The appearance and usability of inventory slots in PST
# varies from character to character and is defined
# by unhardcoded/avslots.2da
SlotMap = None


def InitInventoryWindow (Window):
	"""Opens the inventory window."""

	global AvSlotsTable

	Window.AddAlias("WIN_INV")
	Window.OnFocus(UpdateInventoryWindow)

	AvSlotsTable = GemRB.LoadTable ('avslots')
	Window.GetControl(0x1000003d).AddAlias("MsgSys", 1)

	# Reset the ScrollBar index, since it is shared with other windows eg GUILOAD
	GemRB.SetVar("TopIndex", 0)

	# inventory slots
	SlotCount = GemRB.GetSlotType (-1)["Count"]
	for slot in range (SlotCount):
		SlotType = GemRB.GetSlotType (slot)
		Button = Window.GetControl (SlotType["ID"])
		if Button:
			Button.OnMouseEnter (InventoryCommon.MouseEnterSlot)
			Button.OnMouseLeave (InventoryCommon.MouseLeaveSlot)
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
		Button.SetFlags (IE_GUI_BUTTON_ALIGN_RIGHT | IE_GUI_BUTTON_ALIGN_BOTTOM, OP_OR)
		Button.SetFont ("NUMBER")

		color = {'r' : 128, 'g' : 128, 'b' : 255, 'a' : 64}
		Button.SetBorder (0,color,0,1)
		color['r'], color['b'] = color['b'], color['r']
		Button.SetBorder (1,color,0,1)

		Button.OnMouseEnter (InventoryCommon.MouseEnterGround)
		Button.OnMouseLeave (InventoryCommon.MouseLeaveGround)

	ScrollBar = Window.GetControl (45)
	ScrollBar.OnChange (lambda: RefreshInventoryWindow(Window))

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
	Label.SetColor ({'r' : 255, 'g' : 255, 'b' : 255})
	Label.SetText ("")

	return

def UpdateInventoryWindow (Window = None):
	"""Redraws the inventory window and resets TopIndex."""

	global SlotMap

	if Window == None:
		Window = GemRB.GetView("WIN_INV")

	Window.OnClose(InventoryCommon.InventoryClosed)
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
	SlotMap = list(map (int, AvSlotsTable.GetValue (row, 1, GTV_STR).split( ',')))
	InventoryCommon.SlotMap = SlotMap

	# populate inventory slot controls
	for i in range (46):
		InventoryCommon.UpdateSlot (pc, i)

ToggleInventoryWindow = GUICommonWindows.CreateTopWinLoader(3, "GUIINV", GUICommonWindows.ToggleWindow, InitInventoryWindow, UpdateInventoryWindow)
OpenInventoryWindow = GUICommonWindows.CreateTopWinLoader(3, "GUIINV", GUICommonWindows.OpenWindowOnce, InitInventoryWindow, UpdateInventoryWindow)

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

	# there's a label at 0x1000003a, but we don't need it
	GUICommon.SetEncumbranceLabels (Window, 46, None, pc)

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
			
			Button.SetValue (i + TopIndex)
			Button.SetAction(InventoryCommon.OnDragItemGround, IE_ACT_DRAG_DROP_CRT)
			Button.OnPress (InventoryCommon.OnDragItemGround)
			Button.OnRightPress (InventoryCommon.OpenGroundItemInfoWindow)
			Button.OnShiftPress (InventoryCommon.OpenGroundItemAmountWindow)
			Button.OnDoublePress (InventoryCommon.OpenGroundItemAmountWindow)
		else:
			Button.SetFlags (IE_GUI_BUTTON_PICTURE, OP_NAND)
			Button.SetTooltip (4273)
			Button.SetText ('')
			Button.EnableBorder (0, 0)
			Button.OnPress (None)
			Button.OnRightPress (None)
			Button.OnShiftPress (None)
			Button.OnDoublePress (None)
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

###################################################
# End of file GUIINV.py
