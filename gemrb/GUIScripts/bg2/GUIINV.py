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
import GameCheck
import GUICommon
import GUICommonWindows
import CommonTables
import InventoryCommon
import PaperDoll
from GUIDefines import *
from ie_stats import *
from ie_slots import *
from ie_spells import *

PaperDoll.StanceAnim = "G11"

def InitInventoryWindow (Window):
	"""Opens the inventory window."""

	Window.AddAlias("WIN_INV")

	# info label, game paused, etc
	if GameCheck.IsBG2EE ():
		# text area instead of label
		Feedback = Window.GetControl (64)
	else:
		Feedback = Window.GetControl (0x1000003f)
	Feedback.AddAlias ("MsgSys", 1)
	Feedback.SetText ("")

	#ground items scrollbar
	ScrollBar = Window.GetControl (66)
	ScrollBar.OnChange (lambda: RefreshInventoryWindow(Window))

	#Ground Item
	for i in range (5 + GameCheck.IsAnyEE () * 3):
		Button = Window.GetControl (i+68)
		Button.OnMouseEnter (InventoryCommon.MouseEnterGround)
		Button.OnMouseLeave (InventoryCommon.MouseLeaveGround)
		Button.SetSprites ("STONSLOT",0,0,2,4,3)

	#major & minor clothing color
	Button = Window.GetControl (62)
	Button.SetSprites ("INVBUT",0,0,1,0,0)
	Button.SetFlags (IE_GUI_BUTTON_PICTURE,OP_OR)
	Button.OnPress (lambda: PaperDoll.SelectPickerColor (IE_MAJOR_COLOR))
	Button.SetTooltip (12007)

	Button = Window.GetControl (63)
	Button.SetSprites ("INVBUT",0,0,1,0,0)
	Button.SetFlags (IE_GUI_BUTTON_PICTURE,OP_OR)
	Button.OnPress (lambda: PaperDoll.SelectPickerColor (IE_MINOR_COLOR))
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

	# TODO: ee, by default ees load the dragging window, where stats are
	# displayed before and after equipping an item
	# loading the normal INVSTATS bam as an overlay is done by ui.menu,
	# which also takes care of the missing bg of the AC/HP/THAC0/damage buttons

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

def UpdateInventoryWindow (Window):
	"""Redraws the inventory window and resets TopIndex."""

	Window.OnClose(InventoryCommon.InventoryClosed)
	pc = GemRB.GameGetSelectedPCSingle ()
	if GemRB.GetPlayerStat (pc, IE_STATE_ID) & STATE_DEAD:
		Count = 0
	else:
		Container = GemRB.GetContainer (pc, 1)
		Count = max (0, Container['ItemCount'] - 4)
	ScrollBar = Window.GetControl (66)
	ScrollBar.SetVarAssoc ("TopIndex", GemRB.GetVar ("TopIndex"), 0, Count)
	RefreshInventoryWindow (Window)
	#populate inventory slot controls
	SlotCount = GemRB.GetSlotType (-1)["Count"]

	for i in range (SlotCount):
		InventoryCommon.UpdateSlot (pc, i)
	return

ToggleInventoryWindow = GUICommonWindows.CreateTopWinLoader(2, "GUIINV", GUICommonWindows.ToggleWindow, InitInventoryWindow, UpdateInventoryWindow, True)
OpenInventoryWindow = GUICommonWindows.CreateTopWinLoader(2, "GUIINV", GUICommonWindows.OpenWindowOnce, InitInventoryWindow, UpdateInventoryWindow, True)

def RefreshInventoryWindow (Window):
	"""Partial redraw without resetting TopIndex."""

	pc = GemRB.GameGetSelectedPCSingle ()

	# name
	Label = Window.GetControl (0x10000032)
	Label.SetText (GemRB.GetPlayerName (pc, 0))
	
	# class
	Label = Window.GetControl (0x10000042)
	Label.SetText (GUICommon.GetActorClassTitle (pc))

	# portrait
	Button = Window.GetControl (50)
	stats = PaperDoll.ColorStatsFromPC (pc)
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

	if stats[IE_METAL_COLOR] != -1:
		PaperDoll.SetupEquipment (pc, Button, size, stats)

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
	if GemRB.GetPlayerStat (pc, IE_STATE_ID) & STATE_DEAD:
		GUICommon.AdjustWindowVisibility (Window, pc, False)
		return

	TopIndex = GemRB.GetVar ("TopIndex")
	for i in range (5 + GameCheck.IsAnyEE () * 3):
		Button = Window.GetControl (i+68)
		Slot = GemRB.GetContainerItem (pc, i+TopIndex)
		if GemRB.IsDraggingItem ()==1:
			Button.SetState (IE_GUI_BUTTON_FAKEPRESSED)
		elif not Slot:
			Button.SetState (IE_GUI_BUTTON_LOCKED)
		else:
			Button.SetState (IE_GUI_BUTTON_ENABLED)
		
		Button.SetAction (InventoryCommon.OnDragItemGround, IE_ACT_DRAG_DROP_DST)

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
