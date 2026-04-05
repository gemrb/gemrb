# SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later


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

PaperDoll.StanceAnim = "G11"

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
		#This IS different from BG2
		Button.SetSprites ("STONSLOT",0,0,1,2,3)
		Button.SetFont ("NUMBER")

		color = {'r' : 128, 'g' : 128, 'b' : 255, 'a' : 64}
		Button.SetBorder (0, color, 0, 1)
		color = {'r' : 32, 'g' : 32, 'b' : 255, 'a' : 0}
		Button.SetBorder (1, color, 0, 0, Button.GetInsetFrame(2))
		color = {'r' : 255, 'g' : 128, 'b' : 128, 'a' : 64}
		Button.SetBorder (2, color, 0, 1)

		Button.SetFlags (IE_GUI_BUTTON_ALIGN_RIGHT | IE_GUI_BUTTON_ALIGN_BOTTOM | IE_GUI_BUTTON_PICTURE, OP_OR)

	#major & minor clothing color
	Button = Window.GetControl (62)
	Button.SetFlags (IE_GUI_BUTTON_PICTURE,OP_OR)
	Button.OnPress (lambda: PaperDoll.SelectPickerColor (IE_MAJOR_COLOR))
	Button.SetTooltip (12007)

	Button = Window.GetControl (63)
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
	Button.CreateLabel (0x10000043, "NUMBER", "0:",
		IE_FONT_ALIGN_LEFT|IE_FONT_ALIGN_TOP|IE_FONT_SINGLE_LINE)
	Button.CreateLabel (0x10000044, "NUMBER", "0:",
		IE_FONT_ALIGN_RIGHT|IE_FONT_ALIGN_BOTTOM|IE_FONT_SINGLE_LINE)

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
			Button.OnMouseEnter (InventoryCommon.MouseEnterSlot)
			Button.OnMouseLeave (InventoryCommon.MouseLeaveSlot)
			Button.SetVarAssoc ("ItemButton", slot+1)
			#keeping 1 in the original place, because it is how
			#the gui resource has it, but setting the other cycles
			#This IS different from BG2
			Button.SetSprites ("STONSLOT",0,0,1,2,3)
			Button.SetFont ("NUMBER")

			color = {'r' : 128, 'g' : 128, 'b' : 255, 'a' : 64}
			Button.SetBorder (0, color, 0, 1)
			color = {'r' : 255, 'g' : 128, 'b' : 128, 'a' : 64}
			Button.SetBorder (2, color, 0, 1)
			color = {'r' : 32, 'g' : 32, 'b' : 255, 'a' : 0}
			Button.SetBorder (1, color, 0, 0, Button.GetInsetFrame(2))

	GemRB.SetVar ("TopIndex", 0)

	# force unpause the game
	GemRB.GamePause(0, 0)
	return

def UpdateInventoryWindow (Window):
	"""Redraws the inventory window and resets TopIndex."""

	Window.OnClose(InventoryCommon.InventoryClosed)
	pc = GemRB.GameGetSelectedPCSingle ()
	Container = GemRB.GetContainer (pc, 1)
	ScrollBar = Window.GetControl (66)
	Count = max (0, Container['ItemCount'] - 5)
	ScrollBar.SetVarAssoc ("TopIndex", GemRB.GetVar ("TopIndex"), 0, Count)
	RefreshInventoryWindow (Window)
	#populate inventory slot controls
	SlotCount = GemRB.GetSlotType (-1)["Count"]

	for i in range (SlotCount):
		InventoryCommon.UpdateSlot (pc, i)
	return

ToggleInventoryWindow = GUICommonWindows.CreateTopWinLoader(2, "GUIINV", GUICommonWindows.ToggleWindow, InitInventoryWindow, UpdateInventoryWindow)
OpenInventoryWindow = GUICommonWindows.CreateTopWinLoader(2, "GUIINV", GUICommonWindows.OpenWindowOnce, InitInventoryWindow, UpdateInventoryWindow)

def RefreshInventoryWindow (Window):
	"""Partial redraw without resetting TopIndex."""

	pc = GemRB.GameGetSelectedPCSingle ()

	# name
	Label = Window.GetControl (0x10000032)
	Label.SetText (GemRB.GetPlayerName (pc, 0))

	# portrait
	Button = Window.GetControl (50)

	anim_id = GemRB.GetPlayerStat (pc, IE_ANIMATION_ID)
	row = "0x%04X" %anim_id
	size = CommonTables.Pdolls.GetValue (row, "SIZE")

	if size == "*":
		Button.SetPLT (PaperDoll.GetActorPaperDoll (pc),
			-1, 0, 0, 0, 0, 0, 0, 0, 0)
	else:
		stats = PaperDoll.ColorStatsFromPC (pc)
		Button.SetPLT (PaperDoll.GetActorPaperDoll (pc), stats, 0)
		PaperDoll.SetupEquipment (pc, Button, size, stats)

	# encumbrance
	GUICommon.SetEncumbranceLabels ( Window, 0x10000043, 0x10000044, pc)

	# armor class
	ac = GemRB.GetPlayerStat (pc, IE_ARMORCLASS)
	Label = Window.GetControl (0x10000038)
	Label.SetText (str (ac))

	# hp current
	hp = GemRB.GetPlayerStat (pc, IE_HITPOINTS)
	Label = Window.GetControl (0x10000039)
	Label.SetText (str (hp))

	# hp max
	hpmax = GemRB.GetPlayerStat (pc, IE_MAXHITPOINTS)
	Label = Window.GetControl (0x1000003a)
	Label.SetText (str (hpmax))

	# party gold
	Label = Window.GetControl (0x10000040)
	Label.SetText (str (GemRB.GameGetPartyGold ()))

	# class
	ClassTitle = GUICommon.GetActorClassTitle (pc)
	Label = Window.GetControl (0x10000042)
	Label.SetText (ClassTitle)

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
