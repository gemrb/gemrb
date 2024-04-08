# GemRB - Infinity Engine Emulator
# Copyright (C) 2020 The GemRB Project
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
import InventoryCommon
from GUIDefines import *
from ie_stats import *
from ie_slots import *
from ie_spells import *
from ie_restype import RES_BAM

InventoryWindow = None

# ground slots are transposed with one outlier, so map them to something normal
# 68 71 -> 68 69
# 69 72 -> 70 71
# 70 81 -> 72 81
# the original seems to have stored the order in the Cycle field (0-5)
HorizontalSlots = { 68: 68,  69: 70, 70: 72, 71: 69, 72: 71, 81: 81 }

def InitInventoryWindow (Window):
	global InventoryWindow

	Window.AddAlias("WIN_INV")
	InventoryWindow = Window

	#ground items scrollbar
	ScrollBar = Window.GetControl (66)
	ScrollBar.OnChange (RefreshInventoryWindow)

	# Ground Items (6)
	for cid in HorizontalSlots:
		Button = Window.GetControl (cid)
		Button.OnMouseEnter (InventoryCommon.MouseEnterGround)
		Button.OnMouseLeave (InventoryCommon.MouseLeaveGround)
		Button.SetFont ("NUMBER")

	Button = Window.GetControl (81)
	Button.SetTooltip (83)
	Button.SetVarAssoc ("ItemButton", 6 + 81)
	Button.SetFont ("NUMBER")
	Button.SetFlags (IE_GUI_BUTTON_ALIGN_RIGHT | IE_GUI_BUTTON_ALIGN_BOTTOM | IE_GUI_BUTTON_PICTURE, OP_OR)

	#major & minor clothing color
	Button = Window.GetControl (62)
	Button.SetFlags (IE_GUI_BUTTON_PICTURE,OP_OR)
	Button.OnPress (InventoryCommon.MajorPress)
	Button.SetTooltip ("Major color")

	Button = Window.GetControl (63)
	Button.SetFlags (IE_GUI_BUTTON_PICTURE,OP_OR)
	Button.OnPress (InventoryCommon.MinorPress)
	Button.SetTooltip ("Minor color")

	#hair & skin color
	Button = Window.GetControl (82)
	Button.SetFlags (IE_GUI_BUTTON_PICTURE,OP_OR)
	Button.OnPress (InventoryCommon.HairPress)
	Button.SetTooltip ("Hair color")

	Button = Window.GetControl (83)
	Button.SetFlags (IE_GUI_BUTTON_PICTURE,OP_OR)
	Button.OnPress (InventoryCommon.SkinPress)
	Button.SetTooltip ("Skin color")

	# paperdoll
	Button = Window.GetControl (50)
	Button.SetState (IE_GUI_BUTTON_LOCKED)
	Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE | IE_GUI_BUTTON_PICTURE, OP_SET)
	Button.SetAction (InventoryCommon.OnAutoEquip, IE_ACT_DRAG_DROP_DST)

	# portrait
	Button = Window.GetControl (84)
	Button.SetState (IE_GUI_BUTTON_LOCKED)
	Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE | IE_GUI_BUTTON_PICTURE, OP_SET)
	Button.OnPress (GUICommonWindows.CloseTopWindow)
	Button.SetHotKey ('i')

	# eXit
	Button = Window.GetControl (200)
	Button.SetText ("X")
	Button.OnPress (GUICommonWindows.CloseTopWindow)

	# armor class
	Label = Window.GetControl (0x10000038)
	Label.SetTooltip ("Armor class")

	# hp current
	Label = Window.GetControl (0x10000039)
	Label.SetTooltip ("Hitpoints")

	# hp max
	Label = Window.GetControl (0x1000003a)
	Label.SetTooltip ("Max hitpoints")

	# info label, game paused, etc
	Label = Window.GetControl (0x1000003f)
	Window.RemoveSubview (Label, True)

	SlotCount = GemRB.GetSlotType (-1)["Count"]
	for slot in range (SlotCount):
		SlotType = GemRB.GetSlotType (slot+1)
		Button = Window.GetControl (SlotType["ID"])
		if SlotType["ID"] and Button:
			Button.OnMouseEnter (InventoryCommon.MouseEnterSlot)
			Button.OnMouseLeave (InventoryCommon.MouseLeaveSlot)
			Button.SetVarAssoc ("ItemButton", slot+1)
			Button.SetFont ("NUMBER")

	GemRB.SetVar ("TopIndex", 0)

	for i in range (0, 3, 2):
		Button = Window.GetControl (109+i)
		Button.OnPress (ChangeWeaponPressed)
		Button.SetVarAssoc("Equipped", i // 2)
		Button.SetFlags (IE_GUI_BUTTON_RADIOBUTTON, OP_OR)

def ChangeWeaponPressed ():
	pc = GemRB.GameGetSelectedPCSingle ()
	Equipped = GemRB.GetVar ("Equipped")
	GemRB.SetEquippedQuickSlot (pc, Equipped, -1)
	return

#complete update
def UpdateInventoryWindow (Window = None):
	if Window == None:
		Window = GemRB.GetView("WIN_INV")

	pc = GemRB.GameGetSelectedPCSingle ()
	Container = GemRB.GetContainer (pc, 1)
	ScrollBar = Window.GetControl (66)
	Count = Container['ItemCount']
	# account for two columns
	ScrollBar.SetVarAssoc ("TopIndex", max(0, (Count - 6 + 1) // 2))
	Equipped = GemRB.GetEquippedQuickSlot (pc, 1)
	GemRB.SetVar ("Equipped", Equipped)
	for i in range (0, 3, 2):
		Button = Window.GetControl (109+i)
		Button.SetVarAssoc("Equipped", i // 2)
	RefreshInventoryWindow ()
	# populate inventory slot controls
	SlotCount = GemRB.GetSlotType (-1)["Count"]
	for i in range (SlotCount):
		InventoryCommon.UpdateSlot (pc, i)
	return

InventoryCommon.UpdateInventoryWindow = UpdateInventoryWindow

ToggleInventoryWindow = GUICommonWindows.CreateTopWinLoader(2, "GUIINV", GUICommonWindows.ToggleWindow, InitInventoryWindow, UpdateInventoryWindow)
OpenInventoryWindow = GUICommonWindows.CreateTopWinLoader(2, "GUIINV", GUICommonWindows.OpenWindowOnce, InitInventoryWindow, UpdateInventoryWindow)

def RefreshInventoryWindow ():
	Window = InventoryWindow

	pc = GemRB.GameGetSelectedPCSingle ()

	# name
	Label = Window.GetControl (0x10000032)
	Label.SetText (GemRB.GetPlayerName (pc, 0))

	# paperdoll
	Button = Window.GetControl (50)

	Button.SetFlags (IE_GUI_BUTTON_CENTER_PICTURES, OP_OR)
	pdoll = GUICommonWindows.GetActorPaperDoll (pc)+"WK"
	# FIXME: animation bleeds through top window
	# NOTE: regenerate invenwin.png with a transparent hole if we
	# can ever keep drawing the gamecontrol underneath ...
	# and then remove this block
	if GemRB.HasResource (pdoll, RES_BAM):
		pal = [GemRB.GetPlayerStat (pc, c) for c in range(IE_METAL_COLOR, IE_HAIR_COLOR + 1)]
		Button.SetAnimation (pdoll, 0, 8, pal)

	# portrait
	Button = Window.GetControl (84)
	# NOTE: use FillPlayerInfo or code to switch pics iff the party will grow
	#Button.SetPicture (GemRB.GetPlayerPortrait (pc,0))
	Button.SetPicture ("portr1L")

	# encumbrance
	GUICommon.SetEncumbranceLabels (Window, 0x10000042, None, pc)

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

	Button = Window.GetControl (62)
	Color = GemRB.GetPlayerStat (pc, IE_MAJOR_COLOR, 1) & 0xFF
	Button.SetBAM ("COLGRAD", 0, 0, Color)

	Button = Window.GetControl (63)
	Color = GemRB.GetPlayerStat (pc, IE_MINOR_COLOR, 1) & 0xFF
	Button.SetBAM ("COLGRAD", 0, 0, Color)

	Button = Window.GetControl (82)
	Color = GemRB.GetPlayerStat (pc, IE_HAIR_COLOR, 1) & 0xFF
	Button.SetBAM ("COLGRAD", 0, 0, Color)

	Button = Window.GetControl (83)
	Color = GemRB.GetPlayerStat (pc, IE_SKIN_COLOR, 1) & 0xFF
	Button.SetBAM ("COLGRAD", 0, 0, Color)

	# update ground inventory slots
	TopIndex = GemRB.GetVar ("TopIndex") * 2
	for (i, cid) in enumerate(HorizontalSlots):
		Button = Window.GetControl (cid)
		# use a different item, so the order is LTR
		i = min(5, HorizontalSlots[cid] - 68)
		slotID = i + TopIndex

		if GemRB.IsDraggingItem ()==1:
			Button.SetState (IE_GUI_BUTTON_FAKEPRESSED)
		else:
			Button.SetState (IE_GUI_BUTTON_ENABLED)
		Button.SetAction (InventoryCommon.OnDragItemGround, IE_ACT_DRAG_DROP_DST)

		Slot = GemRB.GetContainerItem (pc, slotID)
		if Slot == None:
			Button.OnPress (None)
			Button.OnRightPress (None)
			Button.OnShiftPress (None)
			Button.OnDoublePress (None)
		else:
			Button.SetValue (slotID)
			Button.OnPress (InventoryCommon.OnDragItemGround)
			Button.OnRightPress (InventoryCommon.OpenGroundItemInfoWindow)
			Button.OnShiftPress (InventoryCommon.OpenGroundItemAmountWindow)
			Button.OnDoublePress (InventoryCommon.OpenGroundItemAmountWindow)

		InventoryCommon.UpdateInventorySlot (pc, Button, Slot, "ground")

	#if actor is uncontrollable, make this grayed
	GUICommon.AdjustWindowVisibility (Window, pc, False)
	return

###################################################
# End of file GUIINV.py
