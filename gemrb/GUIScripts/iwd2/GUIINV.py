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
# $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/GUIScripts/iwd2/GUIINV.py,v 1.2 2004/11/26 23:00:29 avenger_teambg Exp $


# GUIINV.py - scripts to control inventory windows from GUIINV winpack

###################################################

import string

from GUIDefines import *
from ie_stats import *
import GemRB
from GUICommonWindows import GetActorClassTitle, GetActorPaperDoll
from GUICommonWindows import SetSelectionChangeHandler

InventoryWindow = None
ItemInfoWindow = None
ItemAmountWindow = None
ItemIdentifyWindow = None

def OpenInventoryWindow ():
	global InventoryWindow
	
	GemRB.HideGUI()
	
	if InventoryWindow != None:
		
		GemRB.UnloadWindow(InventoryWindow)
		InventoryWindow = None
		GemRB.SetVar("OtherWindow", -1)
		SetSelectionChangeHandler (None)
		GemRB.UnhideGUI()
		return
		
	GemRB.LoadWindowPack ("GUIINV")
	InventoryWindow = Window = GemRB.LoadWindow(2)
	GemRB.SetVar("OtherWindow", InventoryWindow)

	# Ground Item
	for i in range (68, 72):
		Button = GemRB.GetControl (Window, i)
		GemRB.SetTooltip (Window, Button, 12011)
	Button = GemRB.GetControl (Window, 81)
	GemRB.SetTooltip (Window, Button, 12011)
	
	#major & minor clothing color
	Button = GemRB.GetControl (Window, 62)
	GemRB.SetButtonFlags(Window, Button, IE_GUI_BUTTON_PICTURE,OP_OR)
	GemRB.SetEvent(Window, Button, IE_GUI_BUTTON_ON_PRESS,"MajorPress")
	GemRB.SetTooltip (Window, Button, 12007)

	Button = GemRB.GetControl (Window, 63)
	GemRB.SetButtonFlags(Window, Button, IE_GUI_BUTTON_PICTURE,OP_OR)
	GemRB.SetEvent(Window, Button, IE_GUI_BUTTON_ON_PRESS,"MinorPress")
	GemRB.SetTooltip (Window, Button, 12008)

	#hair & skin color
	Button = GemRB.GetControl (Window, 82)
	GemRB.SetButtonFlags(Window, Button, IE_GUI_BUTTON_PICTURE,OP_OR)
	GemRB.SetEvent(Window, Button, IE_GUI_BUTTON_ON_PRESS,"HairPress")
	GemRB.SetTooltip (Window, Button, 37560)

	Button = GemRB.GetControl (Window, 83)
	GemRB.SetButtonFlags(Window, Button, IE_GUI_BUTTON_PICTURE,OP_OR)
	GemRB.SetEvent(Window, Button, IE_GUI_BUTTON_ON_PRESS,"SkinPress")
	GemRB.SetTooltip (Window, Button, 37559)

	# paperdoll
	Button = GemRB.GetControl (Window, 50)
	GemRB.SetButtonState(Window, Button, IE_GUI_BUTTON_LOCKED)
	GemRB.SetButtonFlags(Window, Button, IE_GUI_BUTTON_NO_IMAGE | IE_GUI_BUTTON_PICTURE, OP_SET)

	# portrait
	Button = GemRB.GetControl (Window, 84)
	GemRB.SetButtonState(Window, Button, IE_GUI_BUTTON_LOCKED)
	GemRB.SetButtonFlags(Window, Button, IE_GUI_BUTTON_NO_IMAGE | IE_GUI_BUTTON_PICTURE, OP_SET)

	# encumbrance
	#Label = GemRB.GetControl (Window, 0x10000042)
	#GemRB.SetTooltip (Window, Label, ?)

	# armor class
	Label = GemRB.GetControl (Window, 0x10000038)
	GemRB.SetTooltip (Window, Label, 4197)

	# hp current
	Label = GemRB.GetControl (Window, 0x10000039)
	GemRB.SetTooltip (Window, Label, 4198)

	# hp max
	Label = GemRB.GetControl (Window, 0x1000003a)
	GemRB.SetTooltip (Window, Label, 4199)

	#ground icons scrollbar
	ScrollBar = GemRB.GetControl (Window, 66)
	
	#info label, game paused, etc
	Label = GemRB.GetControl (Window, 0x1000003f)
	GemRB.SetText (Window, Label, "")

	SetSelectionChangeHandler (UpdateInventoryWindow)
	for slot in range(50):
		SlotType = GemRB.GetSlotType(slot)
		if SlotType["Type"]:
			Button = GemRB.GetControl (Window, SlotType["ID"])
			GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_ALIGN_RIGHT | IE_GUI_BUTTON_ALIGN_BOTTOM | IE_GUI_BUTTON_PICTURE, OP_OR)
			GemRB.SetVarAssoc (Window, Button, "ItemButton", slot)

	UpdateInventoryWindow ()

	GemRB.UnhideGUI()
	return

def CancelColor():
	GemRB.UnloadWindow(ColorPicker)
	GemRB.SetVisible(InventoryWindow,1)
	return

def ColorDonePress():
	pc = GemRB.GameGetSelectedPCSingle ()

	GemRB.UnloadWindow(ColorPicker)
	GemRB.SetVisible(InventoryWindow,1)
	ColorTable = GemRB.LoadTable("clowncol")
	PickedColor=GemRB.GetTableValue(ColorTable, ColorIndex, GemRB.GetVar("Selected"))
	GemRB.UnloadTable(ColorTable)
	if ColorIndex==0:
		GemRB.SetPlayerStat (pc, IE_HAIR_COLOR, PickedColor)
		UpdateInventoryWindow()
		return
	if ColorIndex==1:
		GemRB.SetPlayerStat (pc, IE_SKIN_COLOR, PickedColor)
		UpdateInventoryWindow()
		return
	if ColorIndex==2:
		GemRB.SetPlayerStat (pc, IE_MAJOR_COLOR, PickedColor)
		UpdateInventoryWindow()
		return
	GemRB.SetPlayerStat (pc, IE_MINOR_COLOR, PickedColor)
	UpdateInventoryWindow()
	return

def HairPress():
	global ColorIndex, PickedColor

	pc = GemRB.GameGetSelectedPCSingle ()
	ColorIndex = 0
	PickedColor = GemRB.GetPlayerStat (pc, IE_HAIR_COLOR)
	GetColor()
	return

def SkinPress():
	global ColorIndex, PickedColor

	pc = GemRB.GameGetSelectedPCSingle ()
	ColorIndex = 1
	PickedColor = GemRB.GetPlayerStat (pc, IE_SKIN_COLOR)
	GetColor()
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

	ColorTable = GemRB.LoadTable("clowncol")
	GemRB.SetVisible(InventoryWindow,2) #darken it
	ColorPicker=GemRB.LoadWindow(3)
	GemRB.SetVar("Selected",-1)
	Button = GemRB.GetControl(ColorPicker, 35)
	GemRB.SetEvent(ColorPicker, Button, IE_GUI_BUTTON_ON_PRESS, "CancelColor")
	GemRB.SetText(ColorPicker, Button, 13727)

	for i in range(0,34):
		Button = GemRB.GetControl(ColorPicker, i)
		GemRB.SetButtonState(ColorPicker, Button, IE_GUI_BUTTON_DISABLED)
		GemRB.SetButtonFlags(ColorPicker, Button, IE_GUI_BUTTON_PICTURE|IE_GUI_BUTTON_RADIOBUTTON,OP_OR)

	Selected = -1
	for i in range(0,34):
		MyColor = GemRB.GetTableValue(ColorTable, ColorIndex, i)
		if MyColor == "*":
			break
		Button = GemRB.GetControl(ColorPicker, i)
		GemRB.SetButtonBAM(ColorPicker, Button, "COLGRAD", 2, 0, MyColor)
		if PickedColor == MyColor:
			GemRB.SetVar("Selected",i)
			Selected = i
		GemRB.SetButtonState(ColorPicker, Button, IE_GUI_BUTTON_ENABLED)
		GemRB.SetVarAssoc(ColorPicker, Button, "Selected",i)
		GemRB.SetEvent(ColorPicker, Button, IE_GUI_BUTTON_ON_PRESS, "ColorDonePress")
	GemRB.UnloadTable(ColorTable)
	GemRB.SetVisible(ColorPicker,1)
	return

def UpdateInventoryWindow ():
	Window = InventoryWindow

	pc = GemRB.GameGetSelectedPCSingle ()

	# name
	Label = GemRB.GetControl (Window, 0x10000032)
	GemRB.SetText (Window, Label, GemRB.GetPlayerName (pc, 0))

	# paperdoll
	Button = GemRB.GetControl (Window, 50)
	GemRB.SetButtonBAM(Window, Button, GetActorPaperDoll (pc)+"G1",10,0,0)

	# portrait
	Button = GemRB.GetControl (Window, 84)
	GemRB.SetButtonPicture (Window, Button, GemRB.GetPlayerPortrait (pc,0))

	# encumbrance
	Label = GemRB.GetControl (Window, 0x10000042)
	# Loading tables of modifications
	Table = GemRB.LoadTable ("strmod")
	TableEx = GemRB.LoadTable ("strmodex")
	# Getting the character's strength
	sstr = GemRB.GetPlayerStat (pc, IE_STR)
	ext_str = GemRB.GetPlayerStat (pc, IE_STREXTRA)

	max_encumb = GemRB.GetTableValue(Table, sstr, 3) + GemRB.GetTableValue(TableEx, ext_str, 3)
	encumbrance = GemRB.GetPlayerStat (pc, IE_ENCUMBRANCE)
        GemRB.SetText (Window, Label, str(encumbrance)+"/"+str(max_encumb)+GemRB.GetString(39537) )

        ratio = (0.0 + encumbrance) / max_encumb
        if ratio > 1.0:
                GemRB.SetLabelTextColor (Window, Label, 255, 0, 0)
        elif ratio > 0.8:
                GemRB.SetLabelTextColor (Window, Label, 255, 255, 0)
        else:
                GemRB.SetLabelTextColor (Window, Label, 0, 255, 0)

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

	Button = GemRB.GetControl (Window, 62)
	Color = GemRB.GetPlayerStat (pc, IE_MAJOR_COLOR)
	GemRB.SetButtonBAM(Window, Button, "COLGRAD", 0, 0, Color)

	Button = GemRB.GetControl (Window, 63)
	Color = GemRB.GetPlayerStat (pc, IE_MINOR_COLOR)
	GemRB.SetButtonBAM(Window, Button, "COLGRAD", 0, 0, Color)

	Button = GemRB.GetControl (Window, 82)
	Color = GemRB.GetPlayerStat (pc, IE_HAIR_COLOR)
	GemRB.SetButtonBAM(Window, Button, "COLGRAD", 0, 0, Color)

	Button = GemRB.GetControl (Window, 83)
	Color = GemRB.GetPlayerStat (pc, IE_SKIN_COLOR)
	GemRB.SetButtonBAM(Window, Button, "COLGRAD", 0, 0, Color)

	# populate inventory slot controls
	for i in range (50):
		UpdateSlot (pc, i)

	GemRB.UnhideGUI()
	return

def UpdateSlot (pc, slot):

	Window = InventoryWindow
	SlotType = GemRB.GetSlotType(slot)
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

def OnDragItem ():
	pc = GemRB.GameGetSelectedPCSingle ()

	slot = GemRB.GetVar ("ItemButton")
	if not GemRB.IsDraggingItem ():
		slot_item = GemRB.GetSlotItem (pc, slot)
		item = GemRB.GetItem (slot_item["ItemResRef"])
		GemRB.DragItem (pc, slot, item["ItemIcon"], 0, 0, 0)
	else:
		GemRB.DropDraggedItem (pc, slot)

	UpdateInventoryWindow ()
	return

def OnDropItemToPC ():
	pc = GemRB.GetVar ("PressedPortrait") + 1
	GemRB.DropDraggedItem (pc, -1)
	UpdateInventoryWindow ()
	return

###################################################
# End of file GUIINV.py
