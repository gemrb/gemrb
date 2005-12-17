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
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
#
# $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/GUIScripts/tob/GUIINV.py,v 1.33 2005/12/17 21:02:49 avenger_teambg Exp $


# GUIINV.py - scripts to control inventory windows from GUIINV winpack

###################################################

import string
import GemRB
import GUICommonWindows
from GUIDefines import *
from ie_stats import *
from GUICommon import CloseOtherWindow
from GUICommonWindows import *

InventoryWindow = None
ItemInfoWindow = None
ItemAmountWindow = None
ItemIdentifyWindow = None
PortraitWindow = None
OptionsWindow = None
OldPortraitWindow = None
OldOptionsWindow = None

def OpenInventoryWindow ():
	global InventoryWindow, OptionsWindow, PortraitWindow
	global OldPortraitWindow, OldOptionsWindow
	
	if CloseOtherWindow (OpenInventoryWindow):
		GemRB.UnloadWindow (InventoryWindow)
		GemRB.UnloadWindow (OptionsWindow)
		GemRB.UnloadWindow (PortraitWindow)

		InventoryWindow = None
		GemRB.SetVar ("OtherWindow", -1)
		GemRB.SetVisible (0,1)
		GemRB.UnhideGUI ()
		GUICommonWindows.PortraitWindow = OldPortraitWindow
		OldPortraitWindow = None
		GUICommonWindows.OptionsWindow = OldOptionsWindow
		OldOptionsWindow = None
		SetSelectionChangeHandler (None)
		return

	GemRB.HideGUI ()
	GemRB.SetVisible (0,0)

	GemRB.LoadWindowPack ("GUIINV", 640, 480)
	InventoryWindow = Window = GemRB.LoadWindow (2)
	GemRB.SetVar ("OtherWindow", InventoryWindow)
	#saving the original portrait window
	OldOptionsWindow = GUICommonWindows.OptionsWindow
	OptionsWindow = GemRB.LoadWindow (0)
	SetupMenuWindowControls (OptionsWindow, 0, "OpenInventoryWindow")
	GemRB.SetWindowFrame (OptionsWindow)
	OldPortraitWindow = GUICommonWindows.PortraitWindow
	PortraitWindow = OpenPortraitWindow (0)

	# Ground Item
	for i in range (5):
		Button = GemRB.GetControl (Window, i+68)
		GemRB.SetTooltip (Window, Button, 12011)
		GemRB.SetVarAssoc (Window, Button, "GroundItemButton", i)
		GemRB.SetButtonFont (Window, Button, "NUMBER")

	# ground items scrollbar
	ScrollBar = GemRB.GetControl (Window, 66)
	GemRB.SetEvent (Window, ScrollBar, IE_GUI_SCROLLBAR_ON_CHANGE, "RefreshInventoryWindow")

	#major & minor clothing color
	Button = GemRB.GetControl (Window, 62)
	GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_PICTURE,OP_OR)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS,"MajorPress")
	GemRB.SetTooltip (Window, Button, 12007)

	Button = GemRB.GetControl (Window, 63)
	GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_PICTURE,OP_OR)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS,"MinorPress")
	GemRB.SetTooltip (Window, Button, 12008)

	# portrait
	Button = GemRB.GetControl (Window, 50)
	GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_LOCKED)
	GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_NO_IMAGE | IE_GUI_BUTTON_PICTURE, OP_SET)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_DRAG_DROP, "OnAutoEquip")

	# encumbrance
	Label = GemRB.CreateLabel (Window, 0x10000043, 5,385,60,15,"NUMBER","0:",IE_FONT_ALIGN_LEFT|IE_FONT_ALIGN_TOP)
	Label = GemRB.CreateLabel (Window, 0x10000044, 5,455,80,15,"NUMBER","0:",IE_FONT_ALIGN_RIGHT|IE_FONT_ALIGN_TOP)

	# armor class
	Label = GemRB.GetControl (Window, 0x10000038)
	GemRB.SetTooltip (Window, Label, 4197)

	# hp current
	Label = GemRB.GetControl (Window, 0x10000039)
	GemRB.SetTooltip (Window, Label, 4198)

	# hp max
	Label = GemRB.GetControl (Window, 0x1000003a)
	GemRB.SetTooltip (Window, Label, 4199)

	#info label, game paused, etc
	Label = GemRB.GetControl (Window, 0x1000003f)
	GemRB.SetText (Window, Label, "")

	for slot in range(38):
		SlotType = GemRB.GetSlotType (slot)
		if SlotType["Type"]:
			Button = GemRB.GetControl (Window, SlotType["ID"])
			GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_ALIGN_RIGHT | IE_GUI_BUTTON_ALIGN_TOP | IE_GUI_BUTTON_PICTURE, OP_OR)
			GemRB.SetVarAssoc (Window, Button, "ItemButton", slot)
			GemRB.SetButtonFont (Window, Button, "NUMBER")
	
	GemRB.SetVar ("TopIndex", 0)
	SetSelectionChangeHandler (UpdateInventoryWindow)
	UpdateInventoryWindow ()
	GemRB.SetVisible (OptionsWindow, 1)
	GemRB.SetVisible (Window, 3)
	GemRB.SetVisible (PortraitWindow, 1)
	return

def ColorDonePress():
	pc = GemRB.GameGetSelectedPCSingle ()

	GemRB.UnloadWindow (ColorPicker)
	GemRB.SetVisible (InventoryWindow,1)
	ColorTable = GemRB.LoadTable ("clowncol")
	PickedColor=GemRB.GetTableValue (ColorTable, ColorIndex, GemRB.GetVar ("Selected"))
	GemRB.UnloadTable (ColorTable)
	if ColorIndex==2:
		GemRB.SetPlayerStat (pc, IE_MAJOR_COLOR, PickedColor)
		UpdateInventoryWindow ()
		return
	GemRB.SetPlayerStat (pc, IE_MINOR_COLOR, PickedColor)
	UpdateInventoryWindow ()
	return

def MajorPress():
	global ColorIndex, PickedColor

	pc = GemRB.GameGetSelectedPCSingle ()
	ColorIndex = 2
	PickedColor = GemRB.GetPlayerStat (pc, IE_MAJOR_COLOR)
	GetColor()
	return

def MinorPress():
	global ColorIndex, PickedColor

	pc = GemRB.GameGetSelectedPCSingle ()
	ColorIndex = 3
	PickedColor = GemRB.GetPlayerStat (pc, IE_MINOR_COLOR)
	GetColor()
	return

def GetColor():
	global ColorPicker

	ColorTable = GemRB.LoadTable ("clowncol")
	GemRB.SetVisible (InventoryWindow,2) #darken it
	ColorPicker=GemRB.LoadWindow (3)
	GemRB.SetVar ("Selected",-1)
	for i in range(34):
		Button = GemRB.GetControl (ColorPicker, i)
		GemRB.SetButtonState (ColorPicker, Button, IE_GUI_BUTTON_DISABLED)
		GemRB.SetButtonFlags (ColorPicker, Button, IE_GUI_BUTTON_PICTURE|IE_GUI_BUTTON_RADIOBUTTON,OP_OR)

	Selected = -1
	for i in range(34):
		MyColor = GemRB.GetTableValue (ColorTable, ColorIndex, i)
		if MyColor == "*":
			break
		Button = GemRB.GetControl (ColorPicker, i)
		GemRB.SetButtonBAM (ColorPicker, Button, "COLGRAD", 2, 0, MyColor)
		if PickedColor == MyColor:
			GemRB.SetVar ("Selected",i)
			Selected = i
		GemRB.SetButtonState (ColorPicker, Button, IE_GUI_BUTTON_ENABLED)
		GemRB.SetVarAssoc (ColorPicker, Button, "Selected",i)
		GemRB.SetEvent (ColorPicker, Button, IE_GUI_BUTTON_ON_PRESS, "ColorDonePress")
	GemRB.UnloadTable (ColorTable)
	GemRB.SetVisible (ColorPicker,1)
	return

#complete update
def UpdateInventoryWindow ():
	Window = InventoryWindow

	pc = GemRB.GameGetSelectedPCSingle ()
	Container = GemRB.GetContainer (pc, 1)
	ScrollBar = GemRB.GetControl (Window, 66)
	Count = Container['ItemCount']
	if Count<1:
		Count=1
	GemRB.SetVarAssoc (Window, ScrollBar, "TopIndex", Count)
	RefreshInventoryWindow ()

#partial update without altering TopIndex
def RefreshInventoryWindow ():
	Window = InventoryWindow

	pc = GemRB.GameGetSelectedPCSingle ()

	# name
	Label = GemRB.GetControl (Window, 0x10000032)
	GemRB.SetText (Window, Label, GemRB.GetPlayerName (pc, 0))

	# portrait
	Button = GemRB.GetControl (Window, 50)
	Color1 = GemRB.GetPlayerStat (pc, IE_METAL_COLOR)
	Color2 = GemRB.GetPlayerStat (pc, IE_MINOR_COLOR)
	Color3 = GemRB.GetPlayerStat (pc, IE_MAJOR_COLOR)
	Color4 = GemRB.GetPlayerStat (pc, IE_SKIN_COLOR)
	Color5 = GemRB.GetPlayerStat (pc, IE_LEATHER_COLOR)
	Color6 = GemRB.GetPlayerStat (pc, IE_ARMOR_COLOR)
	Color7 = GemRB.GetPlayerStat (pc, IE_HAIR_COLOR)
	GemRB.SetButtonPLT (Window, Button, GetActorPaperDoll (pc),
		Color1, Color2, Color3, Color4, Color5, Color6, Color7, 0)

	# encumbrance
	SetEncumbranceLabels( Window, 0x10000043, 0x10000044, pc)

	# armor class
	ac = GemRB.GetPlayerStat (pc, IE_ARMORCLASS)
	Label = GemRB.GetControl (Window, 0x10000038)
	GemRB.SetText (Window, Label, str (ac))

	# hp current
	hp = GemRB.GetPlayerStat (pc, IE_HITPOINTS)
	Label = GemRB.GetControl (Window, 0x10000039)
	GemRB.SetText (Window, Label, str (hp))

	# hp max
	hpmax = GemRB.GetPlayerStat (pc, IE_MAXHITPOINTS)
	Label = GemRB.GetControl (Window, 0x1000003a)
	GemRB.SetText (Window, Label, str (hpmax))

	# party gold
	Label = GemRB.GetControl (Window, 0x10000040)
	GemRB.SetText (Window, Label, str (GemRB.GameGetPartyGold ()))

	# class
	ClassTitle = GetActorClassTitle (pc)
	Label = GemRB.GetControl (Window, 0x10000042)
	GemRB.SetText (Window, Label, ClassTitle)

	Button = GemRB.GetControl (Window, 62)
	Color = GemRB.GetPlayerStat (pc, IE_MAJOR_COLOR)
	GemRB.SetButtonBAM (Window, Button, "COLGRAD", 0, 0, Color)

	Button = GemRB.GetControl (Window, 63)
	Color = GemRB.GetPlayerStat (pc, IE_MINOR_COLOR)
	GemRB.SetButtonBAM (Window, Button, "COLGRAD", 0, 0, Color)

	# update ground inventory slots
	Container = GemRB.GetContainer(pc, 1)
	TopIndex = GemRB.GetVar ("TopIndex")
	for i in range (5):
		Button = GemRB.GetControl (Window, i+68)
		GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_DRAG_DROP, "OnDragItemGround")
		Slot = GemRB.GetContainerItem (pc, i+TopIndex)
		if Slot != None:
			Item = GemRB.GetItem (Slot['ItemResRef'])
			GemRB.SetItemIcon (Window, Button, Slot['ItemResRef'],0)
			GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_PICTURE, OP_OR)
			GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OnDragItemGround")
			GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_RIGHT_PRESS, "OpenItemInfoGroundWindow")
			GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_SHIFT_PRESS, "OpenItemAmountGroundWindow")

		else:
			GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_PICTURE, OP_NAND)
			GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "")
			GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_RIGHT_PRESS, "")
			GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_SHIFT_PRESS, "")

	# populate inventory slot controls
	for i in range (38):
		UpdateSlot (pc, i)

	#making window visible/shaded depending on the pc's state
	GemRB.SetVisible (Window, 1)
	return

def UpdateSlot (pc, slot):

	Window = InventoryWindow
	SlotType = GemRB.GetSlotType (slot)
	if not SlotType["Type"]:
		return

	Button = GemRB.GetControl (Window, SlotType["ID"])
	slot_item = GemRB.GetSlotItem (pc, slot)

	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_DRAG_DROP, "OnDragItem")
	if slot_item:
		item = GemRB.GetItem (slot_item["ItemResRef"])
		identified = slot_item["Flags"] & IE_INV_ITEM_IDENTIFIED

		GemRB.SetItemIcon (Window, Button, slot_item["ItemResRef"])
		if item["StackAmount"] > 1:
			GemRB.SetText (Window, Button, str (slot_item["Usages0"]))
		else:
			GemRB.SetText (Window, Button, "")

		if not identified or item["ItemNameIdentified"] == -1:
			GemRB.SetTooltip (Window, Button, item["ItemName"])
		else:
			GemRB.SetTooltip (Window, Button, item["ItemNameIdentified"])

		GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OnDragItem")
		GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_RIGHT_PRESS, "OpenItemInfoWindow")
		GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_SHIFT_PRESS, "OpenItemAmountWindow")
	else:

		if SlotType["ResRef"]=="*":
			GemRB.SetButtonBAM (Window, Button, "",0,0,0)
		else:
			GemRB.SetButtonBAM (Window, Button, SlotType["ResRef"],0,0,0)
		GemRB.SetText (Window, Button, "")
		GemRB.SetTooltip (Window, Button, SlotType["Tip"])

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
	else:
		GemRB.DropDraggedItem (pc, -2) #dropping on ground

	UpdateInventoryWindow ()
	return

def OnAutoEquip ():
	if not GemRB.IsDraggingItem ():
		return

	pc = GemRB.GameGetSelectedPCSingle ()
	#don't try to put stuff in the inventory
	for i in range (21):
		GemRB.DropDraggedItem (pc, i)
		if not GemRB.IsDraggingItem ():
			break

	if GemRB.IsDraggingItem ():
		GemRB.PlaySound("GAM_47")  #failed equip

	UpdateInventoryWindow ()
	return

def OnDragItem ():
	pc = GemRB.GameGetSelectedPCSingle ()

	slot = GemRB.GetVar ("ItemButton")
	if not GemRB.IsDraggingItem ():
		slot_item = GemRB.GetSlotItem (pc, slot)
		item = GemRB.GetItem (slot_item["ItemResRef"])
		GemRB.DragItem (pc, slot, item["ItemIcon"], 0, 0)
	else:
		GemRB.DropDraggedItem (pc, slot)
		if GemRB.IsDraggingItem ():
			GemRB.PlaySound("GAM_47")  #failed equip

	UpdateInventoryWindow ()
	return

def OnDropItemToPC ():
	pc = GemRB.GetVar ("PressedPortrait") + 1

	GemRB.DropDraggedItem (pc, -1)
	if GemRB.IsDraggingItem ():
		GemRB.PlaySound("GAM_47")  #failed equip
	UpdateInventoryWindow ()
	return

###################################################
# End of file GUIINV.py
