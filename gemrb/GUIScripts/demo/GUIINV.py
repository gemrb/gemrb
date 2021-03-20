# -*-python-*-
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

def OpenInventoryWindowClick ():
	tmp = GemRB.GetVar ("PressedPortrait")
	GemRB.GameSelectPC (tmp, True, SELECT_REPLACE)
	OpenInventoryWindow ()
	return

def OpenInventoryWindow ():
	global InventoryWindow

	if GUICommon.CloseOtherWindow (OpenInventoryWindow):
		if GemRB.IsDraggingItem () == 1:
			pc = GemRB.GameGetSelectedPCSingle ()
			#store the item in the inventory before window is closed
			GemRB.DropDraggedItem (pc, -3)
			#dropping on ground if cannot store in inventory
			if GemRB.IsDraggingItem () == 1:
				GemRB.DropDraggedItem (pc, -2)

		InventoryCommon.CloseIdentifyItemWindow ()
		InventoryCommon.CloseAbilitiesItemWindow ()
		InventoryCommon.CancelColor()
		if InventoryCommon.ItemInfoWindow:
			InventoryCommon.ItemInfoWindow.Unload ()
			InventoryCommon.ItemInfoWindow = None
		if InventoryCommon.ItemAmountWindow:
			InventoryCommon.ItemAmountWindow.Unload ()
			InventoryCommon.ItemAmountWindow = None
		if InventoryCommon.ErrorWindow:
			InventoryCommon.ErrortWindow.Unload ()
			InventoryCommon.ErrortWindow = None

		if InventoryWindow:
			InventoryWindow.Unload ()

		InventoryWindow = None
		GemRB.SetVar ("OtherWindow", -1)
		GUICommon.GameWindow.SetVisible(WINDOW_VISIBLE)
		GemRB.UnhideGUI ()
		GUICommonWindows.SetSelectionChangeHandler (None)
		return

	GemRB.HideGUI ()
	GUICommon.GameWindow.SetVisible(WINDOW_INVISIBLE)

	GemRB.LoadWindowPack ("GUIINV", 800, 600)
	InventoryWindow = Window = GemRB.LoadWindow (2)
	GemRB.SetVar ("OtherWindow", InventoryWindow.ID)
	Window.SetFrame ()

	#ground items scrollbar
	ScrollBar = Window.GetControl (66)
	ScrollBar.SetEvent (IE_GUI_SCROLLBAR_ON_CHANGE, RefreshInventoryWindow)

	# Ground Items (6)
	for i in range (5):
		Button = Window.GetControl (i+68)
		Button.SetEvent (IE_GUI_MOUSE_ENTER_BUTTON, InventoryCommon.MouseEnterGround)
		Button.SetEvent (IE_GUI_MOUSE_LEAVE_BUTTON, InventoryCommon.MouseLeaveGround)
		Button.SetVarAssoc ("GroundItemButton", i)
		Button.SetFont ("NUMBER")

	Button = Window.GetControl (81)
	Button.SetTooltip (83)
	Button.SetVarAssoc ("GroundItemButton", 6)
	Button.SetFont ("NUMBER")
	Button.SetFlags (IE_GUI_BUTTON_ALIGN_RIGHT | IE_GUI_BUTTON_ALIGN_BOTTOM | IE_GUI_BUTTON_PICTURE, OP_OR)

	#major & minor clothing color
	Button = Window.GetControl (62)
	Button.SetFlags (IE_GUI_BUTTON_PICTURE,OP_OR)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, InventoryCommon.MajorPress)
	Button.SetTooltip ("Major color")

	Button = Window.GetControl (63)
	Button.SetFlags (IE_GUI_BUTTON_PICTURE,OP_OR)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, InventoryCommon.MinorPress)
	Button.SetTooltip ("Minor color")

	#hair & skin color
	Button = Window.GetControl (82)
	Button.SetFlags (IE_GUI_BUTTON_PICTURE,OP_OR)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, InventoryCommon.HairPress)
	Button.SetTooltip ("Hair color")

	Button = Window.GetControl (83)
	Button.SetFlags (IE_GUI_BUTTON_PICTURE,OP_OR)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, InventoryCommon.SkinPress)
	Button.SetTooltip ("Skin color")

	# paperdoll
	Button = Window.GetControl (50)
	Button.SetState (IE_GUI_BUTTON_LOCKED)
	Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE | IE_GUI_BUTTON_PICTURE | IE_GUI_BUTTON_ANIMATED, OP_SET)
	Button.SetEvent (IE_GUI_BUTTON_ON_DRAG_DROP, InventoryCommon.OnAutoEquip)

	# portrait
	Button = Window.GetControl (84)
	Button.SetState (IE_GUI_BUTTON_LOCKED)
	Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE | IE_GUI_BUTTON_PICTURE, OP_SET)

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
	Label.SetText ("")

	SlotCount = GemRB.GetSlotType (-1)["Count"]
	for slot in range (SlotCount):
		SlotType = GemRB.GetSlotType (slot+1)
		if SlotType["ID"] and Window.HasControl (SlotType["ID"]):
			Button = Window.GetControl (SlotType["ID"])
			Button.SetEvent (IE_GUI_MOUSE_ENTER_BUTTON, InventoryCommon.MouseEnterSlot)
			Button.SetEvent (IE_GUI_MOUSE_LEAVE_BUTTON, InventoryCommon.MouseLeaveSlot)
			Button.SetVarAssoc ("ItemButton", slot+1)
			Button.SetFont ("NUMBER")

	GemRB.SetVar ("TopIndex", 0)

	for i in range (0, 3, 2):
		Button = Window.GetControl (109+i)
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, ChangeWeaponPressed)
		Button.SetVarAssoc("Equipped", i/2)
		Button.SetFlags (IE_GUI_BUTTON_RADIOBUTTON, OP_OR)

	# create a button so we can map it do ESC for quit exiting
	Button = Window.CreateButton (999, 0, 0, 1, 1)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, OpenInventoryWindow)
	Button.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)

	GUICommonWindows.SetSelectionChangeHandler (UpdateInventoryWindow)

	UpdateInventoryWindow ()
	return

def ChangeWeaponPressed ():
	pc = GemRB.GameGetSelectedPCSingle ()
	Equipped = GemRB.GetVar ("Equipped")
	GemRB.SetEquippedQuickSlot (pc, Equipped, -1)
	return

#complete update
def UpdateInventoryWindow ():
	Window = InventoryWindow

	pc = GemRB.GameGetSelectedPCSingle ()
	Container = GemRB.GetContainer (pc, 1)
	ScrollBar = Window.GetControl (66)
	Count = Container['ItemCount']
	if Count<1:
		Count=1
	ScrollBar.SetVarAssoc ("TopIndex", Count)
	Equipped = GemRB.GetEquippedQuickSlot (pc, 1)
	GemRB.SetVar ("Equipped", Equipped)
	for i in range (0, 3, 2):
		Button = Window.GetControl (109+i)
		Button.SetVarAssoc("Equipped", i/2)
	RefreshInventoryWindow ()
	# populate inventory slot controls
	SlotCount = GemRB.GetSlotType (-1)["Count"]
	for i in range (SlotCount):
		InventoryCommon.UpdateSlot (pc, i)
	return

def RefreshInventoryWindow ():
	Window = InventoryWindow

	pc = GemRB.GameGetSelectedPCSingle ()

	# name
	Label = Window.GetControl (0x10000032)
	Label.SetText (GemRB.GetPlayerName (pc, 0))

	# paperdoll
	Button = Window.GetControl (50)
	Color1 = GemRB.GetPlayerStat (pc, IE_METAL_COLOR)
	Color2 = GemRB.GetPlayerStat (pc, IE_MINOR_COLOR)
	Color3 = GemRB.GetPlayerStat (pc, IE_MAJOR_COLOR)
	Color4 = GemRB.GetPlayerStat (pc, IE_SKIN_COLOR)
	Color5 = GemRB.GetPlayerStat (pc, IE_LEATHER_COLOR)
	Color6 = GemRB.GetPlayerStat (pc, IE_ARMOR_COLOR)
	Color7 = GemRB.GetPlayerStat (pc, IE_HAIR_COLOR)

	Button.SetFlags (IE_GUI_BUTTON_CENTER_PICTURES, OP_OR)
	pdoll = GUICommonWindows.GetActorPaperDoll (pc)+"WK"
	# FIXME: animation bleeds through top window
	# NOTE: regenerate invenwin.png with a transparent hole if we
	# can ever keep drawing the gamecontrol underneath ...
	# and then remove this block
	if GemRB.HasResource (pdoll, RES_BAM):
		Button.SetAnimation (pdoll)
		Button.SetAnimationPalette (Color1, Color2, Color3, Color4, Color5, Color6, Color7, 0)

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
	Container = GemRB.GetContainer(pc, 1)
	TopIndex = GemRB.GetVar ("TopIndex")
	for i in range (6):
		if i<5:
			Button = Window.GetControl (i+68)
		else:
			Button = Window.GetControl (i+76)

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
			Button.SetEvent (IE_GUI_BUTTON_ON_SHIFT_PRESS, InventoryCommon.OpenGroundItemAmountWindow)

		InventoryCommon.UpdateInventorySlot (pc, Button, Slot, "ground")

	#if actor is uncontrollable, make this grayed
	GUICommon.AdjustWindowVisibility (Window, pc, False)
	return

###################################################
# End of file GUIINV.py
