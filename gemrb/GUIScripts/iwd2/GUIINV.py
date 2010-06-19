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
import GUICommonWindows
from GUIDefines import *
from ie_stats import *
from GUICommon import CloseOtherWindow, SetColorStat
from GUICommon import GameWindow
from GUICommonWindows import *

InventoryWindow = None
ItemInfoWindow = None
ItemAmountWindow = None
ItemIdentifyWindow = None
PortraitWindow = None
OldPortraitWindow = None
OptionsWindow = None
OldOptionsWindow = None

def OpenInventoryWindow ():
	global InventoryWindow, OptionsWindow, PortraitWindow
	global OldPortraitWindow, OldOptionsWindow

	if CloseOtherWindow (OpenInventoryWindow):
		if InventoryWindow:
			InventoryWindow.Unload ()
		if OptionsWindow:
			OptionsWindow.Unload ()
		if PortraitWindow:
			PortraitWindow.Unload ()

		InventoryWindow = None
		GemRB.SetVar ("OtherWindow", -1)
		GameWindow.SetVisible(WINDOW_VISIBLE)
		GemRB.UnhideGUI ()
		GUICommonWindows.PortraitWindow = OldPortraitWindow
		OldPortraitWindow = None
		GUICommonWindows.OptionsWindow = OldOptionsWindow
		OldOptionsWindow = None
		SetSelectionChangeHandler (None)
		return

	GemRB.HideGUI ()
	GameWindow.SetVisible(WINDOW_INVISIBLE)

	GemRB.LoadWindowPack ("GUIINV", 800, 600)
	InventoryWindow = Window = GemRB.LoadWindow (2)
	GemRB.SetVar ("OtherWindow", InventoryWindow.ID)
	#saving the original portrait window
	OldPortraitWindow = GUICommonWindows.PortraitWindow
	PortraitWindow = OpenPortraitWindow ()
	OldOptionsWindow = GUICommonWindows.OptionsWindow
	OptionsWindow = GemRB.LoadWindow (0)
	SetupMenuWindowControls (OptionsWindow, 0, "OpenInventoryWindow")
	Window.SetFrame ()

	#ground items scrollbar
	ScrollBar = Window.GetControl (66)
	ScrollBar.SetEventByName (IE_GUI_SCROLLBAR_ON_CHANGE, "RefreshInventoryWindow")

	# Ground Items (6)
	for i in range (5):
		Button = Window.GetControl (i+68)
		Button.SetTooltip (12011)
		Button.SetEventByName (IE_GUI_MOUSE_ENTER_BUTTON, "MouseEnterGround")
		Button.SetEventByName (IE_GUI_MOUSE_LEAVE_BUTTON, "MouseLeaveGround")
		Button.SetVarAssoc ("GroundItemButton", i)
		Button.SetFont ("NUMFONT")
		Button.SetFlags (IE_GUI_BUTTON_ALIGN_RIGHT | IE_GUI_BUTTON_ALIGN_BOTTOM | IE_GUI_BUTTON_PICTURE, OP_OR)
	Button = Window.GetControl (81)
	Button.SetTooltip (12011)
	Button.SetVarAssoc ("GroundItemButton", 6)
	Button.SetFont ("NUMFONT")
	Button.SetFlags (IE_GUI_BUTTON_ALIGN_RIGHT | IE_GUI_BUTTON_ALIGN_BOTTOM | IE_GUI_BUTTON_PICTURE, OP_OR)
	
	#major & minor clothing color
	Button = Window.GetControl (62)
	Button.SetFlags (IE_GUI_BUTTON_PICTURE,OP_OR)
	Button.SetEventByName (IE_GUI_BUTTON_ON_PRESS,"MajorPress")
	Button.SetTooltip (12007)

	Button = Window.GetControl (63)
	Button.SetFlags (IE_GUI_BUTTON_PICTURE,OP_OR)
	Button.SetEventByName (IE_GUI_BUTTON_ON_PRESS,"MinorPress")
	Button.SetTooltip (12008)

	#hair & skin color
	Button = Window.GetControl (82)
	Button.SetFlags (IE_GUI_BUTTON_PICTURE,OP_OR)
	Button.SetEventByName (IE_GUI_BUTTON_ON_PRESS,"HairPress")
	Button.SetTooltip (37560)

	Button = Window.GetControl (83)
	Button.SetFlags (IE_GUI_BUTTON_PICTURE,OP_OR)
	Button.SetEventByName (IE_GUI_BUTTON_ON_PRESS,"SkinPress")
	Button.SetTooltip (37559)

	# paperdoll
	Button = Window.GetControl (50)
	Button.SetState (IE_GUI_BUTTON_LOCKED)
	Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE | IE_GUI_BUTTON_PICTURE | IE_GUI_BUTTON_ANIMATED, OP_SET)
	Button.SetEventByName (IE_GUI_BUTTON_ON_DRAG_DROP, "OnAutoEquip")

	# portrait
	Button = Window.GetControl (84)
	Button.SetState (IE_GUI_BUTTON_LOCKED)
	Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE | IE_GUI_BUTTON_PICTURE, OP_SET)

	# armor class
	Label = Window.GetControl (0x10000038)
	Label.SetTooltip (4197)

	# hp current
	Label = Window.GetControl (0x10000039)
	Label.SetTooltip (4198)

	# hp max
	Label = Window.GetControl (0x1000003a)
	Label.SetTooltip (4199)

	#info label, game paused, etc
	Label = Window.GetControl (0x1000003f)
	Label.SetText ("")

	SlotCount = GemRB.GetSlotType (-1)["Count"]
	for slot in range (SlotCount):
		SlotType = GemRB.GetSlotType (slot+1)
		if SlotType["ID"]:
			Button = Window.GetControl (SlotType["ID"])
			Button.SetEventByName (IE_GUI_MOUSE_ENTER_BUTTON, "MouseEnterSlot")
			Button.SetEventByName (IE_GUI_MOUSE_LEAVE_BUTTON, "MouseLeaveSlot")
			Button.SetVarAssoc ("ItemButton", slot+1)
			Button.SetFont ("NUMFONT")
			Button.SetFlags (IE_GUI_BUTTON_ALIGN_RIGHT | IE_GUI_BUTTON_ALIGN_BOTTOM | IE_GUI_BUTTON_PICTURE, OP_OR)

	GemRB.SetVar ("TopIndex", 0)

	SetSelectionChangeHandler (UpdateInventoryWindow)

	UpdateInventoryWindow ()
	return

def CancelColor():
	if ColorPicker:
		ColorPicker.Unload ()
	InventoryWindow.SetVisible (WINDOW_VISIBLE)
	return

def ColorDonePress():
	pc = GemRB.GameGetSelectedPCSingle ()

	if ColorPicker:
		ColorPicker.Unload ()
	ColorTable = GemRB.LoadTable ("clowncol")
	PickedColor=ColorTable.GetValue (ColorIndex, GemRB.GetVar ("Selected"))
	if ColorIndex==0:
		SetColorStat (pc, IE_HAIR_COLOR, PickedColor)
	elif ColorIndex==1:
		SetColorStat (pc, IE_SKIN_COLOR, PickedColor)
	elif ColorIndex==2:
		SetColorStat (pc, IE_MAJOR_COLOR, PickedColor)
	else:
		SetColorStat (pc, IE_MINOR_COLOR, PickedColor)
	UpdateInventoryWindow ()
	return

def HairPress():
	global ColorIndex, PickedColor

	pc = GemRB.GameGetSelectedPCSingle ()
	ColorIndex = 0
	PickedColor = GemRB.GetPlayerStat (pc, IE_HAIR_COLOR, 1) & 0xFF
	GetColor()
	return

def SkinPress():
	global ColorIndex, PickedColor

	pc = GemRB.GameGetSelectedPCSingle ()
	ColorIndex = 1
	PickedColor = GemRB.GetPlayerStat (pc, IE_SKIN_COLOR, 1) & 0xFF
	GetColor()
	return

def MajorPress():
	global ColorIndex, PickedColor

	pc = GemRB.GameGetSelectedPCSingle ()
	ColorIndex = 2
	PickedColor = GemRB.GetPlayerStat (pc, IE_MAJOR_COLOR, 1) & 0xFF
	GetColor()
	return

def MinorPress():
	global ColorIndex, PickedColor

	pc = GemRB.GameGetSelectedPCSingle ()
	ColorIndex = 3
	PickedColor = GemRB.GetPlayerStat (pc, IE_MINOR_COLOR, 1) & 0xFF
	GetColor()
	return

def GetColor():
	global ColorPicker

	ColorTable = GemRB.LoadTable ("clowncol")
	InventoryWindow.SetVisible (WINDOW_GRAYED) #darken it
	ColorPicker=GemRB.LoadWindow (3)
	GemRB.SetVar ("Selected",-1)
	Button = ColorPicker.GetControl (35)
	Button.SetEventByName (IE_GUI_BUTTON_ON_PRESS, "CancelColor")
	Button.SetText(13727)

	for i in range (34):
		Button = ColorPicker.GetControl (i)
		Button.SetState (IE_GUI_BUTTON_DISABLED)
		Button.SetFlags (IE_GUI_BUTTON_PICTURE|IE_GUI_BUTTON_RADIOBUTTON,OP_OR)

	Selected = -1
	for i in range (34):
		MyColor = ColorTable.GetValue (ColorIndex, i)
		if MyColor == "*":
			break
		Button = ColorPicker.GetControl (i)
		Button.SetBAM ("COLGRAD", 2, 0, MyColor)
		if PickedColor == MyColor:
			GemRB.SetVar ("Selected",i)
			Selected = i
		Button.SetState (IE_GUI_BUTTON_ENABLED)
		Button.SetVarAssoc ("Selected",i)
		Button.SetEventByName (IE_GUI_BUTTON_ON_PRESS, "ColorDonePress")
	ColorPicker.SetVisible (WINDOW_VISIBLE)
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
	RefreshInventoryWindow ()
	# populate inventory slot controls
	SlotCount = GemRB.GetSlotType (-1)["Count"]
	for i in range (SlotCount):
		UpdateSlot (pc, i)
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
	Button.SetAnimation (GetActorPaperDoll (pc)+"G11")
	Button.SetAnimationPalette (Color1, Color2, Color3, Color4, Color5, Color6, Color7, 0)

	# portrait
	Button = Window.GetControl (84)
	Button.SetPicture (GemRB.GetPlayerPortrait (pc,0))

	# encumbrance
	SetEncumbranceLabels (Window, 0x10000042, None, pc)

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

		Button.SetEventByName (IE_GUI_BUTTON_ON_DRAG_DROP, "OnDragItemGround")
		Slot = GemRB.GetContainerItem (pc, i+TopIndex)
		if Slot != None:
			Item = GemRB.GetItem (Slot['ItemResRef'])
			Button.SetItemIcon (Slot['ItemResRef'],0)
			Button.SetFlags (IE_GUI_BUTTON_PICTURE, OP_OR)
			Button.SetEventByName (IE_GUI_BUTTON_ON_PRESS, "OnDragItemGround")
			Button.SetEventByName (IE_GUI_BUTTON_ON_RIGHT_PRESS, "OpenItemInfoGroundWindow")
			Button.SetEventByName (IE_GUI_BUTTON_ON_SHIFT_PRESS, "OpenItemAmountGroundWindow")
		else:
			Button.SetFlags (IE_GUI_BUTTON_PICTURE, OP_NAND)
			Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, None)
			Button.SetEvent (IE_GUI_BUTTON_ON_RIGHT_PRESS, None)
			Button.SetEvent (IE_GUI_BUTTON_ON_SHIFT_PRESS, None)

	#if actor is uncontrollable, make this grayed
	Window.SetVisible (WINDOW_VISIBLE)
	PortraitWindow.SetVisible (WINDOW_VISIBLE)
	OptionsWindow.SetVisible (WINDOW_VISIBLE)
	return

def UpdateSlot (pc, slot):

	Window = InventoryWindow
	SlotType = GemRB.GetSlotType (slot+1)
	if not SlotType["ID"]:
		return

	Button = Window.GetControl (SlotType["ID"])
	slot_item = GemRB.GetSlotItem (pc, slot+1)

	Button.SetEventByName (IE_GUI_BUTTON_ON_DRAG_DROP, "OnDragItem")
	if slot_item:
		item = GemRB.GetItem (slot_item["ItemResRef"])
		identified = slot_item["Flags"] & IE_INV_ITEM_IDENTIFIED

		Button.SetItemIcon (slot_item["ItemResRef"])
		if item["StackAmount"] > 1:
			Button.SetText (str (slot_item["Usages0"]))
		else:
			Button.SetText ("")

		if not identified or item["ItemNameIdentified"] == -1:
			Button.SetTooltip (item["ItemName"])
		else:
			Button.SetTooltip (item["ItemNameIdentified"])

		Button.SetEventByName (IE_GUI_BUTTON_ON_PRESS, "OnDragItem")
		Button.SetEventByName (IE_GUI_BUTTON_ON_RIGHT_PRESS, "OpenItemInfoWindow")
		Button.SetEventByName (IE_GUI_BUTTON_ON_SHIFT_PRESS, "OpenItemAmountWindow")
	else:
		if SlotType["ResRef"]=="*":
			Button.SetBAM ("",0,0)
		else:
			Button.SetBAM (SlotType["ResRef"],0,0)
		Button.SetText ("")
		Button.SetTooltip (SlotType["Tip"])

		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, None)
		Button.SetEvent (IE_GUI_BUTTON_ON_RIGHT_PRESS, None)
		Button.SetEvent (IE_GUI_BUTTON_ON_SHIFT_PRESS, None)
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

	GemRB.DropDraggedItem (pc, -1)

	if GemRB.IsDraggingItem ():
		GemRB.PlaySound("GAM_47") #failed equip

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

	UpdateInventoryWindow ()
	return

def OnDropItemToPC ():
	pc = GemRB.GetVar ("PressedPortrait") + 1

	GemRB.DropDraggedItem (pc, -3)
	if GemRB.IsDraggingItem ():
		GemRB.DisplayString(17999,0xffffff)
	UpdateInventoryWindow ()
	return

def DragItemAmount ():
	Text = ItemAmountWindow.GetControl (6)
	Amount = Text.QueryText ()
	print Amount

	OpenItemAmountWindow()
	return

def OpenItemAmountWindow ():
	global ItemAmountWindow

	GemRB.HideGUI ()

	if ItemAmountWindow != None:
		if ItemAmountWindow:
			ItemAmountWindow.Unload ()
		ItemAmountWindow = None
		GemRB.SetVar ("FloatWindow", -1)

		GemRB.UnhideGUI ()
		return

	ItemAmountWindow = Window = GemRB.LoadWindow (4)
	GemRB.SetVar ("FloatWindow", ItemAmountWindow.ID)

	pc = GemRB.GameGetSelectedPCSingle ()
	slot = GemRB.GetVar ("ItemButton")
	slot_item = GemRB.GetSlotItem (pc, slot)
	ResRef = slot_item['ItemResRef']
	item = GemRB.GetItem (ResRef)

	print Window.ID
	# item icon
	Icon = Window.GetControl (0)
	Icon.SetFlags (IE_GUI_BUTTON_PICTURE | IE_GUI_BUTTON_NO_IMAGE, OP_SET)
	Icon.SetItemIcon (ResRef)

	# item amount
	Text = Window.GetControl (6)
	Text.SetSize (40, 40)
	Text.SetText ("1")
	Text.SetStatus (IE_GUI_EDIT_NUMBER|IE_GUI_CONTROL_FOCUSED)

	# Done
	Button = Window.GetControl (2)
	Button.SetText (11973)
	Button.SetEventByName (IE_GUI_BUTTON_ON_PRESS, "DragItemAmount")
	Button.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)

	# Cancel
	Button = Window.GetControl (1)
	Button.SetText (13727)
	Button.SetEventByName (IE_GUI_BUTTON_ON_PRESS, "OpenItemAmountWindow")
	Button.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)

	# 0 bmp
	# 1,2 done/cancel?
	# 3 +
	# 4 -
	# 6 text

	GemRB.UnhideGUI ()
	Window.ShowModal (MODAL_SHADOW_GRAY)
	return

def DrinkItemWindow ():
	pc = GemRB.GameGetSelectedPCSingle ()
	slot = GemRB.GetVar ("ItemButton")
	# the drink item header is always the first
	GemRB.UseItem (pc, slot, 0)
	CloseItemInfoWindow ()
	return

def ReadItemWindow ():
	pc = GemRB.GameGetSelectedPCSingle ()
	slot = GemRB.GetVar ("ItemButton")
	# the learn scroll header is always the second
	# 5 is TARGET_SELF, because some scrolls are buggy
	GemRB.UseItem (pc, slot, 1, 5)
	CloseItemInfoWindow ()
	return

def OpenItemWindow ():
	#close inventory
	GemRB.SetVar ("Inventory", 1)
	if ItemInfoWindow:
		ItemInfoWindow.Unload ()
	OpenInventoryWindow ()
	pc = GemRB.GameGetSelectedPCSingle ()
	slot = GemRB.GetVar ("ItemButton")
	slot_item = GemRB.GetSlotItem (pc, slot)
	ResRef = slot_item['ItemResRef']
	#the store will have to reopen the inventory
	GemRB.EnterStore (ResRef)
	return

def DialogItemWindow ():
	if ItemInfoWindow:
		ItemInfoWindow.Unload ()
	OpenInventoryWindow ()
	pc = GemRB.GameGetSelectedPCSingle ()
	slot = GemRB.GetVar ("ItemButton")
	slot_item = GemRB.GetSlotItem (pc, slot)
	ResRef = slot_item['ItemResRef']
	item = GemRB.GetItem (ResRef)
	dialog=item["Dialog"]
	GemRB.ExecuteString ("StartDialog(\""+dialog+"\",Myself)", pc)
	return

def IdentifyUseSpell ():
	global ItemIdentifyWindow

	pc = GemRB.GameGetSelectedPCSingle ()
	slot = GemRB.GetVar ("ItemButton")
	if ItemIdentifyWindow:
		ItemIdentifyWindow.Unload ()
	GemRB.HasSpecialSpell (pc, 1, 1)
	if ItemInfoWindow:
		ItemInfoWindow.Unload ()
	GemRB.ChangeItemFlag (pc, slot, IE_INV_ITEM_IDENTIFIED, OP_OR)
	OpenItemInfoWindow()
	return

def IdentifyUseScroll ():
	global ItemIdentifyWindow

	pc = GemRB.GameGetSelectedPCSingle ()
	slot = GemRB.GetVar ("ItemButton")
	if ItemIdentifyWindow:
		ItemIdentifyWindow.Unload ()
	if ItemInfoWindow:
		ItemInfoWindow.Unload ()
	if GemRB.HasSpecialItem (pc, 1, 1):
		GemRB.ChangeItemFlag (pc, slot, IE_INV_ITEM_IDENTIFIED, OP_OR)
	OpenItemInfoWindow()
	return

def CloseIdentifyItemWindow ():
	if ItemIdentifyWindow:
		ItemIdentifyWindow.Unload ()
	return

def IdentifyItemWindow ():
	global ItemIdentifyWindow

	pc = GemRB.GameGetSelectedPCSingle ()

	ItemIdentifyWindow = Window = GemRB.LoadWindow (9)
	Button = Window.GetControl (0)
	Button.SetText (17105)
	Button.SetEventByName (IE_GUI_BUTTON_ON_PRESS, "IdentifyUseSpell")
	Button = Window.GetControl (1)
	Button.SetText (17106)
	Button.SetEventByName (IE_GUI_BUTTON_ON_PRESS, "IdentifyUseSpell")
	Button = Window.GetControl (2)
	Button.SetText (13727)
	Button.SetEventByName (IE_GUI_BUTTON_ON_PRESS, "CloseIdentifyItemWindow")
	TextArea = Window.GetControl (3)
	TextArea.SetText (19394)
	Window.ShowModal (MODAL_SHADOW_GRAY)
	return

def CloseItemInfoWindow ():
	if ItemInfoWindow:
		ItemInfoWindow.Unload ()
	return

def DisplayItem (itemresref, type):
	global ItemInfoWindow

	item = GemRB.GetItem (itemresref)
	ItemInfoWindow = Window = GemRB.LoadWindow (5)

	#item icon
	Button = Window.GetControl (2)
	Button.SetFlags (IE_GUI_BUTTON_PICTURE, OP_OR)
	Button.SetItemIcon (itemresref,0)
	Button.SetState (IE_GUI_BUTTON_LOCKED)

	#middle button
	Button = Window.GetControl (4)
	Button.SetText (11973)
	Button.SetEventByName (IE_GUI_BUTTON_ON_PRESS, "CloseItemInfoWindow")

	#textarea
	Text = Window.GetControl (5)
	if (type&2):
		text = item["ItemDesc"]
	else:
		text = item["ItemDescIdentified"]
	Text.SetText (text)

	#left button
	Button = Window.GetControl(8)
	if type&2:
		Button.SetText (14133)
		Button.SetEventByName (IE_GUI_BUTTON_ON_PRESS, "IdentifyItemWindow")
	else:
		Button.SetState (IE_GUI_BUTTON_LOCKED)
		Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_SET)

	#description icon
	#Button = Window.GetControl (7)
	#Button.SetFlags (IE_GUI_BUTTON_PICTURE, OP_OR)
	#Button.SetItemIcon (itemresref,2)

	#right button
	Button = Window.GetControl(9)
	drink = (type&1) and (item["Function"]&1)
	read = (type&1) and (item["Function"]&2)
	container = (type&1) and (item["Function"]&4)
	dialog = (type&1) and (item["Dialog"]!="")
	if drink:
		Button.SetText (19392)
		Button.SetEventByName (IE_GUI_BUTTON_ON_PRESS, "DrinkItemWindow")
	elif read:
		Button.SetText (17104)
		Button.SetEventByName (IE_GUI_BUTTON_ON_PRESS, "ReadItemWindow")
	elif container:
		Button.SetText (14133)
		Button.SetEventByName (IE_GUI_BUTTON_ON_PRESS, "OpenItemWindow")
	elif dialog:
		Button.SetText (item["DialogName"])
		Button.SetEventByName (IE_GUI_BUTTON_ON_PRESS, "DialogItemWindow")
	else:
		Button.SetState (IE_GUI_BUTTON_LOCKED)
		Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_SET)

	Text = Window.GetControl (0x1000000b)
	if (type&2):
		text = item["ItemName"]
	else:
		text = item["ItemNameIdentified"]
	Text.SetText (text)

	ItemInfoWindow.ShowModal(MODAL_SHADOW_GRAY)
	return

def OpenItemInfoWindow ():
	pc = GemRB.GameGetSelectedPCSingle ()

	slot = GemRB.GetVar ("ItemButton")
	slot_item = GemRB.GetSlotItem (pc, slot)

	if slot_item["Flags"] & IE_INV_ITEM_IDENTIFIED:
		value = 1
	else:
		value = 3
	DisplayItem(slot_item["ItemResRef"], value)
	return

def OpenGroundItemInfoWindow ():
	global ItemInfoWindow

	pc = GemRB.GameGetSelectedPCSingle ()

	slot = GemRB.GetVar("TopIndex")+GemRB.GetVar("GroundItemButton")
	slot_item = GemRB.GetContainerItem (pc, slot)
	if item["Flags"] & IE_INV_ITEM_IDENTIFIED:
		value = 0
	else:
		value = 2
	DisplayItem(slot_item["ItemResRef"], value) #the ground items are only displayable
	return

def MouseEnterSlot ():
	global OverSlot

	pc = GemRB.GameGetSelectedPCSingle ()
	OverSlot = GemRB.GetVar ("ItemButton")
	if GemRB.IsDraggingItem ():
		UpdateSlot (pc, OverSlot-1)
	return

def MouseLeaveSlot ():
	global OverSlot

	pc = GemRB.GameGetSelectedPCSingle ()
	slot = GemRB.GetVar ("ItemButton")
	if slot == OverSlot or not GemRB.IsDraggingItem ():
		OverSlot = None
	UpdateSlot (pc, slot-1)
	return

def MouseEnterGround ():
	Window = InventoryWindow
	i = GemRB.GetVar ("GroundItemButton")
	Button = Window.GetControl (i+68)
	if GemRB.IsDraggingItem ():
		Button.SetState (IE_GUI_BUTTON_SELECTED)
	return

def MouseLeaveGround ():
	Window = InventoryWindow
	i = GemRB.GetVar ("GroundItemButton")
	Button = Window.GetControl (i+68)
	if GemRB.IsDraggingItem ():
		Button.SetState (IE_GUI_BUTTON_SECOND)
	return

###################################################
# End of file GUIINV.py
