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
import InventoryCommon
from GUIDefines import *
from ie_stats import *
from ie_slots import *
from ie_spells import *

InventoryWindow = None
PortraitWindow = None
OptionsWindow = None
OldPortraitWindow = None
OldOptionsWindow = None

def OpenInventoryWindowClick (btn):
	GemRB.GameSelectPC (btn.ID + 1, True, SELECT_REPLACE)
	OpenInventoryWindow ()
	return

def OpenInventoryWindow ():
	"""Opens the inventory window."""

	import GUICommonWindows

	global InventoryWindow, OptionsWindow, PortraitWindow
	global OldPortraitWindow, OldOptionsWindow

	if GUICommon.CloseOtherWindow (OpenInventoryWindow):
		if GemRB.IsDraggingItem () == 1:
			pc = GemRB.GameGetSelectedPCSingle ()
			#store the item in the inventory before window is closed
			GemRB.DropDraggedItem (pc, -3)
			#dropping on ground if cannot store in inventory
			if GemRB.IsDraggingItem () == 1:
				GemRB.DropDraggedItem (pc, -2)

		if InventoryWindow:
			InventoryWindow.Unload ()
		if OptionsWindow:
			OptionsWindow.Unload ()
		if PortraitWindow:
			PortraitWindow.Unload ()

		InventoryWindow = None
		GemRB.SetVar ("OtherWindow", -1)
		GemRB.SetVar ("MessageLabel", -1)
		GUICommonWindows.PortraitWindow = OldPortraitWindow
		GUICommonWindows.UpdatePortraitWindow ()
		OldPortraitWindow = None
		GUICommonWindows.OptionsWindow = OldOptionsWindow
		OldOptionsWindow = None
		#don't go back to multi selection mode when going to the store screen
		if not GemRB.GetVar ("Inventory"):
			GUICommon.GameWindow.SetVisible(WINDOW_VISIBLE)
			GemRB.UnhideGUI ()
			GUICommonWindows.SetSelectionChangeHandler (None)
		return

	GemRB.HideGUI ()
	GUICommon.GameWindow.SetVisible(WINDOW_INVISIBLE)

	InventoryWindow = Window = GemRB.LoadWindow (2, "GUIINV")
	GemRB.SetVar ("OtherWindow", InventoryWindow.ID)
	GemRB.SetVar ("MessageLabel", Window.GetControl (0x1000003f).ID )
	OldOptionsWindow = GUICommonWindows.OptionsWindow
	OptionsWindow = GemRB.LoadWindow (0)
	GUICommonWindows.MarkMenuButton (OptionsWindow)
	GUICommonWindows.SetupMenuWindowControls (OptionsWindow, 0, OpenInventoryWindow)
	#saving the original portrait window
	OldPortraitWindow = GUICommonWindows.PortraitWindow
	PortraitWindow = GUICommonWindows.OpenPortraitWindow (0)

	#ground items scrollbar
	ScrollBar = Window.GetControl (66)
	ScrollBar.SetEvent (IE_GUI_SCROLLBAR_ON_CHANGE, RefreshInventoryWindow)

	#Ground Item
	for i in range (5):
		Button = Window.GetControl (i+68)
		Button.SetEvent (IE_GUI_MOUSE_ENTER_BUTTON, InventoryCommon.MouseEnterGround)
		Button.SetEvent (IE_GUI_MOUSE_LEAVE_BUTTON, InventoryCommon.MouseLeaveGround)
		Button.SetVarAssoc ("GroundItemButton", i)
		Button.SetSprites ("STONSLOT",0,0,2,4,3)

	#major & minor clothing color
	Button = Window.GetControl (62)
	Button.SetSprites ("INVBUT",0,0,1,0,0)
	Button.SetFlags (IE_GUI_BUTTON_PICTURE,OP_OR)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, InventoryCommon.MajorPress)
	Button.SetTooltip (12007)

	Button = Window.GetControl (63)
	Button.SetSprites ("INVBUT",0,0,1,0,0)
	Button.SetFlags (IE_GUI_BUTTON_PICTURE,OP_OR)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, InventoryCommon.MinorPress)
	Button.SetTooltip (12008)

	#portrait
	Button = Window.GetControl (50)
	Button.SetState (IE_GUI_BUTTON_LOCKED)
	Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE | IE_GUI_BUTTON_PICTURE, OP_SET)
	Button.SetEvent (IE_GUI_BUTTON_ON_DRAG_DROP, InventoryCommon.OnAutoEquip)

	#encumbrance
	Button = Window.GetControl (67)
	r = Button.GetFrame ()
	Label = Window.CreateLabel (0x10000043, r["x"],r["y"],r["w"],20,
		"NUMBER","0:",IE_FONT_ALIGN_LEFT|IE_FONT_ALIGN_TOP|IE_FONT_SINGLE_LINE)
	Label = Window.CreateLabel (0x10000044, r["x"],r["y"]+r["h"]-20,r["w"],20,
		"NUMBER","0:",IE_FONT_ALIGN_RIGHT|IE_FONT_ALIGN_BOTTOM|IE_FONT_SINGLE_LINE)

	# armor class
	Label = Window.GetControl (0x10000038)
	Label.SetTooltip (17183)

	# hp current
	Label = Window.GetControl (0x10000039)
	Label.SetTooltip (17184)

	# hp max
	Label = Window.GetControl (0x1000003a)
	Label.SetTooltip (17378)

	# info label, game paused, etc
	Label = Window.GetControl (0x1000003f)
	Label.SetText ("")

	SlotCount = GemRB.GetSlotType (-1)["Count"]
	for slot in range (SlotCount):
		SlotType = GemRB.GetSlotType (slot+1)
		if SlotType["ID"]:
			Button = Window.GetControl (SlotType["ID"])
			Button.SetEvent (IE_GUI_MOUSE_ENTER_BUTTON, InventoryCommon.MouseEnterSlot)
			Button.SetEvent (IE_GUI_MOUSE_LEAVE_BUTTON, InventoryCommon.MouseLeaveSlot)
			Button.SetVarAssoc ("ItemButton", slot+1)
			#keeping 2 in the original place, because it is how
			#the gui resource has it, but setting the other cycles
			Button.SetSprites ("STONSLOT",0,0,2,4,3)

	GemRB.SetVar ("TopIndex", 0)
	GUICommonWindows.SetSelectionChangeHandler (UpdateInventoryWindow)
	UpdateInventoryWindow ()
	OptionsWindow.SetVisible (WINDOW_VISIBLE)
	Window.SetVisible (WINDOW_FRONT)
	PortraitWindow.SetVisible (WINDOW_VISIBLE)
	return

def UpdateInventoryWindow ():
	"""Redraws the inventory window and resets TopIndex."""

	Window = InventoryWindow

	pc = GemRB.GameGetSelectedPCSingle ()
	Container = GemRB.GetContainer (pc, 1)
	ScrollBar = Window.GetControl (66)
	Count = max (0, Container['ItemCount'] - 5)
	ScrollBar.SetVarAssoc ("TopIndex", Count)
	RefreshInventoryWindow ()
	#populate inventory slot controls
	SlotCount = GemRB.GetSlotType (-1)["Count"]

	for i in range (SlotCount):
		InventoryCommon.UpdateSlot (pc, i)
	return

def RefreshInventoryWindow ():
	"""Partial redraw without resetting TopIndex."""

	Window = InventoryWindow

	pc = GemRB.GameGetSelectedPCSingle ()

	# name
	Label = Window.GetControl (0x10000032)
	Label.SetText (GemRB.GetPlayerName (pc, 0))
	
	# class
	Label = Window.GetControl (0x10000042)
	Label.SetText (GUICommon.GetActorClassTitle (pc))

	# portrait
	Button = Window.GetControl (50)
	Color1 = GemRB.GetPlayerStat (pc, IE_METAL_COLOR)
	Color2 = GemRB.GetPlayerStat (pc, IE_MINOR_COLOR)
	Color3 = GemRB.GetPlayerStat (pc, IE_MAJOR_COLOR)
	Color4 = GemRB.GetPlayerStat (pc, IE_SKIN_COLOR)
	Color5 = GemRB.GetPlayerStat (pc, IE_LEATHER_COLOR)
	Color6 = GemRB.GetPlayerStat (pc, IE_ARMOR_COLOR)
	Color7 = GemRB.GetPlayerStat (pc, IE_HAIR_COLOR)
	Button.SetPLT (GUICommon.GetActorPaperDoll (pc),
		Color1, Color2, Color3, Color4, Color5, Color6, Color7, 0, 0)

	anim_id = GemRB.GetPlayerStat (pc, IE_ANIMATION_ID)
	row = "0x%04X" %anim_id
	size = CommonTables.Pdolls.GetValue (row, "SIZE")

	# Weapon
	slot_item = GemRB.GetSlotItem (pc, GemRB.GetEquippedQuickSlot (pc) )
	if slot_item:
		item = GemRB.GetItem (slot_item["ItemResRef"])
		if (item['AnimationType'] != ''):
			Button.SetPLT ("WP" + size + item['AnimationType'] + "INV", Color1, Color2, Color3, Color4, Color5, Color6, Color7, 0, 1)

	# Shield
	slot_item = GemRB.GetSlotItem (pc, 3)
	if slot_item:
		itemname = slot_item["ItemResRef"]
		item = GemRB.GetItem (itemname)
		if (item['AnimationType'] != ''):
			if (GemRB.CanUseItemType (SLOT_WEAPON, itemname)):
				#off-hand weapon
				Button.SetPLT ("WP" + size + item['AnimationType'] + "OIN", Color1, Color2, Color3, Color4, Color5, Color6, Color7, 0, 2)
			else:
				#shield
				Button.SetPLT ("WP" + size + item['AnimationType'] + "INV", Color1, Color2, Color3, Color4, Color5, Color6, Color7, 0, 2)

	# Helmet
	slot_item = GemRB.GetSlotItem (pc, 1)
	if slot_item:
		item = GemRB.GetItem (slot_item["ItemResRef"])
		if (item['AnimationType'] != ''):
			Button.SetPLT ("WP" + size + item['AnimationType'] + "INV", Color1, Color2, Color3, Color4, Color5, Color6, Color7, 0, 3)

	# encumbrance
	GUICommon.SetEncumbranceLabels ( Window, 0x10000043, 0x10000044, pc)

	# armor class
	ac = GemRB.GetPlayerStat (pc, IE_ARMORCLASS)
	Label = Window.GetControl (0x10000038)
	Label.SetText (str (ac))
	Label.SetTooltip (10339)

	# hp current
	hp = GemRB.GetPlayerStat (pc, IE_HITPOINTS)
	Label = Window.GetControl (0x10000039)
	Label.SetText (str (hp))
	Label.SetTooltip (17184)

	# hp max
	hpmax = GemRB.GetPlayerStat (pc, IE_MAXHITPOINTS)
	Label = Window.GetControl (0x1000003a)
	Label.SetText (str (hpmax))
	Label.SetTooltip (17378)

	# party gold
	Label = Window.GetControl (0x10000040)
	Label.SetText (str (GemRB.GameGetPartyGold ()))

	Button = Window.GetControl (62)
	Color = GemRB.GetPlayerStat (pc, IE_MAJOR_COLOR, 1) & 0xFF
	Button.SetBAM ("COLGRAD", 0, 0, Color)

	Button = Window.GetControl (63)
	Color = GemRB.GetPlayerStat (pc, IE_MINOR_COLOR, 1) & 0xFF
	Button.SetBAM ("COLGRAD", 0, 0, Color)

	# update ground inventory slots
	Container = GemRB.GetContainer (pc, 1)
	TopIndex = GemRB.GetVar ("TopIndex")
	for i in range (5):
		Button = Window.GetControl (i+68)
		if GemRB.IsDraggingItem ()==1:
			Button.SetState (IE_GUI_BUTTON_FAKEPRESSED)
		else:
			Button.SetState (IE_GUI_BUTTON_ENABLED)
		Button.SetEvent (IE_GUI_BUTTON_ON_DRAG_DROP, InventoryCommon.OnDragItemGround)
		Slot = GemRB.GetContainerItem (pc, i+TopIndex)

		if Slot == None:
			Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, None)
			Button.SetEvent (IE_GUI_BUTTON_ON_RIGHT_PRESS, None)
			Button.SetEvent (IE_GUI_BUTTON_ON_SHIFT_PRESS, None)
		else:
			Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, InventoryCommon.OnDragItemGround)
			Button.SetEvent (IE_GUI_BUTTON_ON_RIGHT_PRESS, InventoryCommon.OpenGroundItemInfoWindow)
			Button.SetEvent (IE_GUI_BUTTON_ON_SHIFT_PRESS, None) #TODO: implement OpenGroundItemAmountWindow

		GUICommon.UpdateInventorySlot (pc, Button, Slot, "ground")

	# making window visible/shaded depending on the pc's state
	GUICommon.AdjustWindowVisibility (Window, pc, False)
	return

###################################################
# End of file GUIINV.py
