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
import GUICommonWindows
import CommonTables
import InventoryCommon
import PaperDoll

from GUIDefines import *
from ie_stats import *
from ie_slots import *
from ie_spells import *

def InitInventoryWindow (Window):
	"""Opens the inventory window."""

	Window.AddAlias("WIN_INV")
	Window.GetControl (0x1000003f).AddAlias("MsgSys", 1)

	#ground items scrollbar
	ScrollBar = Window.GetControl (66)
	ScrollBar.OnChange (lambda: RefreshInventoryWindow(Window))

	#Ground Item
	for i in range (5):
		Button = Window.GetControl (i+68)
		Button.OnMouseEnter (InventoryCommon.MouseEnterGround)
		Button.OnMouseLeave (InventoryCommon.MouseLeaveGround)
		Button.SetSprites ("STONSLOT",0,0,2,4,3)
		
	def SelectColor(stat):
		pc = GemRB.GetVar("SELECTED_PC")
		Picker = PaperDoll.SelectColorForPC(stat, pc, "GUIINV")
		Picker.SetAction (lambda: GemRB.SetPlayerStat(pc, stat, Picker.GetVar("PickedColor")), ACTION_WINDOW_CLOSED)

	#major & minor clothing color
	Button = Window.GetControl (62)
	Button.SetSprites ("INVBUT",0,0,1,0,0)
	Button.SetFlags (IE_GUI_BUTTON_PICTURE,OP_OR)
	Button.OnPress (lambda: SelectColor(IE_MAJOR_COLOR))
	Button.SetTooltip (12007)

	Button = Window.GetControl (63)
	Button.SetSprites ("INVBUT",0,0,1,0,0)
	Button.SetFlags (IE_GUI_BUTTON_PICTURE,OP_OR)
	Button.OnPress (lambda: SelectColor(IE_MINOR_COLOR))
	Button.SetTooltip (12008)

	#portrait
	Button = Window.GetControl (50)
	Button.SetState (IE_GUI_BUTTON_LOCKED)
	Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE | IE_GUI_BUTTON_PICTURE, OP_SET)
	Button.SetAction (InventoryCommon.OnAutoEquip, IE_ACT_DRAG_DROP_DST)

	#encumbrance
	Button = Window.GetControl (67)
	Button.CreateLabel (0x10000043, "NUMBER", "0:", IE_FONT_ALIGN_LEFT|IE_FONT_ALIGN_TOP|IE_FONT_SINGLE_LINE)
	Button.CreateLabel (0x10000044, "NUMBER", "0:", IE_FONT_ALIGN_RIGHT|IE_FONT_ALIGN_BOTTOM|IE_FONT_SINGLE_LINE)

	# armor class handled on refresh

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
			Button.OnMouseEnter (InventoryCommon.MouseEnterSlot)
			Button.OnMouseLeave (InventoryCommon.MouseLeaveSlot)
			Button.SetVarAssoc ("ItemButton", slot+1)
			#keeping 2 in the original place, because it is how
			#the gui resource has it, but setting the other cycles
			Button.SetSprites ("STONSLOT",0,0,2,4,3)

	GemRB.SetVar ("TopIndex", 0)

	return

def UpdateInventoryWindow (Window = None):
	"""Redraws the inventory window and resets TopIndex."""

	if Window == None:
		Window = GemRB.GetView("WIN_INV")

	pc = GemRB.GetVar("SELECTED_PC")
	Container = GemRB.GetContainer (pc, 1)
	ScrollBar = Window.GetControl (66)
	Count = max (0, Container['ItemCount'] - 4)
	ScrollBar.SetVarAssoc ("TopIndex", Count, 0, Count)
	RefreshInventoryWindow (Window)
	#populate inventory slot controls
	SlotCount = GemRB.GetSlotType (-1)["Count"]

	for i in range (SlotCount):
		InventoryCommon.UpdateSlot (pc, i)
	return

ToggleInventoryWindow = GUICommonWindows.CreateTopWinLoader(2, "GUIINV", GUICommonWindows.ToggleWindow, InitInventoryWindow, UpdateInventoryWindow, GUICommonWindows.DefaultWinPos, True)
OpenInventoryWindow = GUICommonWindows.CreateTopWinLoader(2, "GUIINV", GUICommonWindows.OpenWindowOnce, InitInventoryWindow, UpdateInventoryWindow, GUICommonWindows.DefaultWinPos, True)

InventoryCommon.UpdateInventoryWindow = UpdateInventoryWindow

def RefreshInventoryWindow (Window):
	"""Partial redraw without resetting TopIndex."""

	pc = GemRB.GetVar("SELECTED_PC")

	# name
	Label = Window.GetControl (0x10000032)
	Label.SetText (GemRB.GetPlayerName (pc, 0))
	
	# class
	Label = Window.GetControl (0x10000042)
	Label.SetText (GUICommon.GetActorClassTitle (pc))

	# portrait
	Button = Window.GetControl (50)
	stats = PaperDoll.ColorStatsFromPC(pc)
	# disable coloring and equipment for non-humanoid dolls (shapes/morphs, mod additions)
	# ... which doesn't include ogres and flinds, who are also clown-colored
	# if one more exception needs to be added, rather externalise this to a new pdolls.2da flags column
	anim_id = GemRB.GetPlayerStat (pc, IE_ANIMATION_ID)
	if (anim_id < 0x5000 or anim_id >= 0x7000 or anim_id == 0x6404) and (anim_id != 0x8000 and anim_id != 0x9000):
		stats[IE_METAL_COLOR] = -1
	Button.SetPLT (PaperDoll.GetActorPaperDoll (pc), stats, 0)
	# disable equipment for flinds and ogres and Sarevok (Recovery Mod)
	if anim_id == 0x8000 or anim_id == 0x9000 or anim_id == 0x6404:
		stats[IE_METAL_COLOR] = -1

	row = "0x%04X" %anim_id
	size = CommonTables.Pdolls.GetValue (row, "SIZE")

	# Weapon
	slot_item = GemRB.GetSlotItem (pc, GemRB.GetEquippedQuickSlot (pc) )
	if slot_item and stats[IE_METAL_COLOR] != -1:
		item = GemRB.GetItem (slot_item["ItemResRef"])
		if (item['AnimationType'] != ''):
			Button.SetPLT ("WP" + size + item['AnimationType'] + "INV", stats, 1)

	# Shield
	slot_item = GemRB.GetSlotItem (pc, 3)
	if slot_item and stats[IE_METAL_COLOR] != -1:
		itemname = slot_item["ItemResRef"]
		item = GemRB.GetItem (itemname)
		if (item['AnimationType'] != ''):
			if (GemRB.CanUseItemType (SLOT_WEAPON, itemname)):
				#off-hand weapon
				Button.SetPLT ("WP" + size + item['AnimationType'] + "OIN", stats, 2)
			else:
				#shield
				Button.SetPLT ("WP" + size + item['AnimationType'] + "INV", stats, 2)

	# Helmet
	slot_item = GemRB.GetSlotItem (pc, 1)
	if slot_item and stats[IE_METAL_COLOR] != -1:
		# halflings use gnome helmet files
		# only weapons exist with H, so this is fine for everyone
		if size == "H":
			size = "S"
		item = GemRB.GetItem (slot_item["ItemResRef"])
		if (item['AnimationType'] != ''):
			Button.SetPLT ("WP" + size + item['AnimationType'] + "INV", stats, 3)

	# encumbrance
	GUICommon.SetEncumbranceLabels ( Window, 0x10000043, 0x10000044, pc)

	# armor class
	GUICommon.DisplayAC (pc, Window, 0x10000038)

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
	TopIndex = GemRB.GetVar ("TopIndex")
	for i in range (5):
		Button = Window.GetControl (i+68)
		if GemRB.IsDraggingItem ()==1:
			Button.SetState (IE_GUI_BUTTON_FAKEPRESSED)
		else:
			Button.SetState (IE_GUI_BUTTON_ENABLED)
		
		Button.SetAction (InventoryCommon.OnDragItemGround, IE_ACT_DRAG_DROP_DST)
		Slot = GemRB.GetContainerItem (pc, i+TopIndex)

		if Slot == None:
			Button.OnPress (None)
			Button.OnRightPress (None)
			Button.OnShiftPress (None)
			Button.OnDoublePress (None)
		else:
			Button.SetValue (i + TopIndex)
			Button.SetAction(InventoryCommon.OnDragItemGround, IE_ACT_DRAG_DROP_CRT)
			Button.OnPress (InventoryCommon.OnDragItemGround)
			Button.OnRightPress (InventoryCommon.OpenGroundItemInfoWindow)
			Button.OnShiftPress (InventoryCommon.OpenGroundItemAmountWindow)
			Button.OnDoublePress (InventoryCommon.OpenGroundItemAmountWindow)

		InventoryCommon.UpdateInventorySlot (pc, Button, Slot, "ground")

	# making window visible/shaded depending on the pc's state
	GUICommon.AdjustWindowVisibility (Window, pc, False)
	return

###################################################
# End of file GUIINV.py
