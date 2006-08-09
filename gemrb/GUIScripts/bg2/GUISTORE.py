# -*-python-*-
# GemRB - Infinity Engine Emulator
# Copyright (C) 2003-2005 The GemRB Project
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
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
#
# $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/GUIScripts/bg2/GUISTORE.py,v 1.30 2006/08/09 19:05:32 avenger_teambg Exp $


# GUISTORE.py - script to open store/inn/temple windows from GUISTORE winpack

###################################################

import string
import GemRB
import GUICommonWindows
from GUIDefines import *
from GUICommonWindows import *
from ie_stats import *

StoreWindow = None
MessageWindow = None
ActionWindow = None
PortraitWindow = None
StoreShoppingWindow = None
StoreIdentifyWindow = None
StoreStealWindow = None
StoreDonateWindow = None
StoreHealWindow = None
StoreRumourWindow = None
StoreRentWindow = None
OldPortraitWindow = None

ITEM_PC    = 0
ITEM_STORE = 1

RentIndex = -1
Store = None
Buttons = [-1,-1,-1,-1]
total_price = 0
total_income = 0

# 0 - Store
# 1 - Tavern
# 2 - Inn
# 3 - Temple
# 4 - Container
# 5 - Container

# 0 - buy/sell
# 1 - identify
# 2 - steal
# 3 - heal
# 4 - donate
# 5 - drink
# 6 - rent

storebams = ("STORSTOR","STORTVRN","STORINN","STORTMPL","STORBAG","STORBAG")
storetips = (14288,14292,14291,12138,15013,14289,14287)
roomtypes = (17389,17517,17521,17519)
store_funcs = ( "OpenStoreShoppingWindow", "OpenStoreIdentifyWindow",
"OpenStoreStealWindow", "OpenStoreHealWindow", "OpenStoreDonateWindow",
"OpenStoreRumourWindow", "OpenStoreRentWindow" )
store_update_funcs = None

def CloseWindows ():
	CloseStoreShoppingWindow ()
	CloseStoreIdentifyWindow ()
	CloseStoreStealWindow ()
	CloseStoreHealWindow ()
	CloseStoreDonateWindow ()
	CloseStoreRumourWindow ()
	CloseStoreRentWindow ()


def CloseStoreWindow ():
	global StoreWindow, ActionWindow, PortraitWindow
	global OldPortraitWindow

	CloseWindows ()
	GemRB.UnloadWindow (StoreWindow)
	GemRB.UnloadWindow (ActionWindow)
	GemRB.UnloadWindow (PortraitWindow)
	StoreWindow = None
	GemRB.LeaveStore ()
	GemRB.SetVisible (0,1) #enabling the game control screen
	GemRB.UnhideGUI () #enabling the other windows
	GUICommonWindows.PortraitWindow = OldPortraitWindow
	SetSelectionChangeHandler( None )


def OpenStoreWindow ():
	global Store
	global StoreWindow, ActionWindow, PortraitWindow
	global OldPortraitWindow
	global store_update_funcs

	#these are function pointers, not strings
	#can't put this in global init, doh!
	store_update_funcs = (OpenStoreShoppingWindow,
	OpenStoreIdentifyWindow,OpenStoreStealWindow,
	OpenStoreHealWindow, OpenStoreDonateWindow,
	OpenStoreRumourWindow,OpenStoreRentWindow )

	GemRB.HideGUI ()
	GemRB.SetVisible (0,0) #removing the game control screen

	GemRB.SetVar ("Action", 0)
	GemRB.LoadWindowPack ("GUISTORE", 640, 480)
	StoreWindow = Window = GemRB.LoadWindow (3)
	#saving the original portrait window
	OldPortraitWindow = GUICommonWindows.PortraitWindow
	PortraitWindow = OpenPortraitWindow(0)
	ActionWindow = GemRB.LoadWindow(0)
	#this window is static and grey, but good to stick the frame onto
	GemRB.SetWindowFrame (ActionWindow)

	Store = GemRB.GetStore()
	# Done
	Button = GemRB.GetControl (Window, 0)
	GemRB.SetText (Window, Button, 11973)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "CloseStoreWindow")

	#Store type icon
	Button = GemRB.GetControl (Window, 5)
	GemRB.SetButtonSprites(Window, Button,storebams[Store['StoreType']],0,0,0,0,0)

	#based on shop type, these buttons will change
	store_type = Store['StoreType']
	store_buttons = Store['StoreButtons']
	for i in range(4):
		Buttons[i] = Button = GemRB.GetControl (Window, i+1)
		Action = store_buttons[i]
		GemRB.SetVarAssoc (Window, Button, "Action", i)
		if Action>=0:
			GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_RADIOBUTTON, OP_OR)
			GemRB.SetButtonSprites (Window, Button, "GUISTBBC", Action, 0,1,2,0)
			GemRB.SetTooltip (Window, Button, storetips[Action])
			GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, store_funcs[Action])
			GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_ENABLED)
		else:
			GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_NO_IMAGE, OP_OR)
			GemRB.SetTooltip (Window, Button, "")
			GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "")
			GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_DISABLED)

	GemRB.SetVisible (ActionWindow, 1)
	GemRB.SetVisible (Window, 1)
	store_update_funcs[store_buttons[0]] ()
	GemRB.SetVisible (PortraitWindow, 1)


def OpenStoreShoppingWindow ():
	global StoreShoppingWindow

	GemRB.SetVar ("RightIndex", 0)
	GemRB.SetVar ("LeftIndex", 0)
	GemRB.SetVar ("BuySum", 0)
	GemRB.SetVar ("SellSum", 0)
	CloseWindows()

	StoreShoppingWindow = Window = GemRB.LoadWindow (2)

	# buy price ...
	Label = GemRB.GetControl (Window, 0x1000002b)

	# sell price ...
	Label = GemRB.GetControl (Window, 0x1000002c)

	for i in range(4):
		Button = GemRB.GetControl (Window, i+5)
		GemRB.SetVarAssoc (Window, Button, "LeftIndex", i)
		GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "SelectBuy")
		GemRB.SetButtonBorder (Window, Button, 0,0,0,0,0,0,0,128,160,0,1)
		
		Button = GemRB.GetControl (Window, i+13)
		GemRB.SetVarAssoc (Window, Button, "RightIndex", i)
		GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "SelectSell")
		GemRB.SetButtonBorder (Window, Button, 0,0,0,0,0,0,0,128,160,0,1) 		

	# Buy
	Button = GemRB.GetControl (Window, 2)
	GemRB.SetText (Window, Button, 13703)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "BuyPressed")

	# Sell
	Button = GemRB.GetControl (Window, 3)
	GemRB.SetText (Window, Button, 13704)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "SellPressed")

	# inactive button
	Button = GemRB.GetControl (Window, 50)
	GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_LOCKED)
	GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_NO_IMAGE, OP_SET)

	#backpack
	Button = GemRB.GetControl (Window, 44)
	GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_LOCKED)

	# encumbrance
	Label = GemRB.CreateLabel (Window, 0x10000043, 15,325,60,15,"NUMBER","0:",IE_FONT_ALIGN_LEFT|IE_FONT_ALIGN_TOP)
	Label = GemRB.CreateLabel (Window, 0x10000044, 15,365,80,15,"NUMBER","0:",IE_FONT_ALIGN_RIGHT|IE_FONT_ALIGN_TOP)

	# left scrollbar
	ScrollBar = GemRB.GetControl (Window, 11)
	GemRB.SetEvent (Window, ScrollBar, IE_GUI_SCROLLBAR_ON_CHANGE, "RedrawStoreShoppingWindow")

	# right scrollbar
	ScrollBar = GemRB.GetControl (Window, 12)
	GemRB.SetEvent (Window, ScrollBar, IE_GUI_SCROLLBAR_ON_CHANGE, "RedrawStoreShoppingWindow")

	SetSelectionChangeHandler( UpdateStoreShoppingWindow )
	UpdateStoreShoppingWindow ()
	GemRB.SetVisible(Window, 1)


def OpenStoreIdentifyWindow ():
	global StoreIdentifyWindow
	
	GemRB.SetVar ("Index", -1)
	GemRB.SetVar ("TopIndex", 0)
	CloseWindows()
	
	StoreIdentifyWindow = Window = GemRB.LoadWindow (4)

	# Identify
	Button = GemRB.GetControl (Window, 5)
	GemRB.SetText (Window, Button, 14133)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "InfoIdentifyWindow")

	# 23 ta

	# price ...
	Label = GemRB.GetControl (Window, 0x10000003)
	GemRB.SetText (Window, Label, "0")

	# 8-11 item slots, 0x1000000c-f labels

	for i in range(4):
		Button = GemRB.GetControl (Window, i+8)
		GemRB.SetVarAssoc (Window, Button, "Index", i)
		GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_RADIOBUTTON, OP_OR)
		GemRB.SetButtonBorder (Window, Button, 0,0,0,0,0,0,0,128,160,0,1) 		
		GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "RedrawStoreIdentifyWindow")

	ScrollBar = GemRB.GetControl (Window, 7)
	GemRB.SetEvent (Window, ScrollBar, IE_GUI_SCROLLBAR_ON_CHANGE, "RedrawStoreIdentifyWindow")
	
	SetSelectionChangeHandler( UpdateStoreIdentifyWindow )
	UpdateStoreIdentifyWindow ()
	GemRB.SetVisible(Window, 1)


def OpenStoreStealWindow ():
	global StoreStealWindow
	
	GemRB.SetVar ("RightIndex", 0)
	GemRB.SetVar ("LeftIndex", 0)
	CloseWindows()
	
	StoreStealWindow = Window = GemRB.LoadWindow (6)

	for i in range(4):
		Button = GemRB.GetControl (Window, i+4)
		GemRB.SetVarAssoc (Window, Button, "LeftIndex", i)
		GemRB.SetButtonBorder (Window, Button, 0,0,0,0,0,0,0,128,160,0,1)
		GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "RedrawStoreStealWindow")

		Button = GemRB.GetControl (Window, i+11)
		GemRB.SetVarAssoc (Window, Button, "RightIndex", i)
		GemRB.SetButtonBorder (Window, Button, 0,0,0,0,0,0,0,128,160,0,1) 		
		GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_RIGHT_PRESS, "StealInfoWindow")

	# Steal
	Button = GemRB.GetControl (Window, 1)
	GemRB.SetText (Window, Button, 14179)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "StealPressed")

	Button = GemRB.GetControl (Window, 37)
	GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_LOCKED)

	# encumbrance
	Label = GemRB.CreateLabel (Window, 0x10000043, 15,325,60,15,"NUMBER","0:",IE_FONT_ALIGN_LEFT|IE_FONT_ALIGN_TOP)
	Label = GemRB.CreateLabel (Window, 0x10000044, 15,365,80,15,"NUMBER","0:",IE_FONT_ALIGN_RIGHT|IE_FONT_ALIGN_TOP)

	# left scrollbar
	ScrollBar = GemRB.GetControl (Window, 9)
	GemRB.SetEvent (Window, ScrollBar, IE_GUI_SCROLLBAR_ON_CHANGE, "RedrawStoreStealWindow")

	# right scrollbar
	ScrollBar = GemRB.GetControl (Window, 10)
	GemRB.SetEvent (Window, ScrollBar, IE_GUI_SCROLLBAR_ON_CHANGE, "RedrawStoreStealWindow")

	SetSelectionChangeHandler( UpdateStoreStealWindow )
	UpdateStoreStealWindow ()
	GemRB.SetVisible(Window, 1)


def OpenStoreDonateWindow ():
	global StoreDonateWindow
	
	CloseWindows ()
	
	StoreDonateWindow = Window = GemRB.LoadWindow (9)

	# graphics
	Button = GemRB.GetControl (Window, 10)
	GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_PICTURE|IE_GUI_BUTTON_ANIMATED|IE_GUI_BUTTON_PLAYONCE, OP_OR)
	GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_LOCKED)
	
	# Donate
	Button = GemRB.GetControl (Window, 3)
	GemRB.SetText (Window, Button, 15101)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "DonateGold")
	GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_DEFAULT, OP_OR)

	# Entry
	Field = GemRB.GetControl (Window, 5)
	GemRB.SetText (Window, Field, "0")
	GemRB.SetEvent (Window, Field, IE_GUI_EDIT_ON_CHANGE, "UpdateStoreDonateWindow")
	GemRB.SetControlStatus (Window, Field, IE_GUI_EDIT_NUMBER)

	# +
	Button = GemRB.GetControl (Window, 6)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "IncrementDonation")
	# -
	Button = GemRB.GetControl (Window, 7)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "DecrementDonation")	

	SetSelectionChangeHandler( UpdateStoreDonateWindow )
	UpdateStoreDonateWindow ()
	GemRB.SetVisible(Window, 1)


def OpenStoreHealWindow ():
	global StoreHealWindow
	
	GemRB.SetVar ("Index", -1)
	GemRB.SetVar ("TopIndex", 0)
	CloseWindows()
	
	StoreHealWindow = Window = GemRB.LoadWindow (5)

	#spell buttons
	for i in range(4):
		Button = GemRB.GetControl (Window, i+8)
		GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_RADIOBUTTON, OP_OR)
		GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "UpdateStoreHealWindow")
		GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_RIGHT_PRESS, "InfoHealWindow")

	# price tag
	Label = GemRB.GetControl (Window, 0x10000003)
	GemRB.SetText (Window, Label, "0")

	# Heal
	Button = GemRB.GetControl (Window, 5)
	GemRB.SetText (Window, Button, 13703)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "BuyHeal")
	GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_DISABLED)

	ScrollBar = GemRB.GetControl (Window, 7)
	GemRB.SetEvent (Window, ScrollBar, IE_GUI_SCROLLBAR_ON_CHANGE, "UpdateStoreHealWindow")
	Count = Store['StoreCureCount']
	if Count>4:
		Count = Count-4
	else:
		Count = 0
	GemRB.SetVarAssoc (Window, ScrollBar, "TopIndex", Count+1)

	UpdateStoreHealWindow ()
	GemRB.SetVisible(Window, 1)


def OpenStoreRumourWindow ():
	global StoreRumourWindow
	
	GemRB.SetVar ("TopIndex", 0)
	CloseWindows()
	
	StoreRumourWindow = Window = GemRB.LoadWindow (8)
	
	#removing those pesky labels
	for i in range(5):
		GemRB.DeleteControl (Window, 0x10000005+i)

	TextArea = GemRB.GetControl (Window, 11)
	GemRB.SetText (Window, TextArea, 14144)

	#tavern quality image
	BAM = "TVRNQUL%d"% ((Store['StoreFlags']>>9)&3)
	Button = GemRB.GetControl (Window, 12)
	GemRB.SetButtonSprites (Window, Button, BAM, 0, 0, 0, 0, 0)
	GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_LOCKED)

	ScrollBar = GemRB.GetControl (Window, 5)
	GemRB.SetEvent (Window, ScrollBar, IE_GUI_SCROLLBAR_ON_CHANGE, "UpdateStoreRumourWindow")
	Count = Store['StoreDrinkCount']
	if Count>4:
		Count = Count-4
	else:
		Count = 0
	GemRB.SetVarAssoc (Window, ScrollBar, "TopIndex", Count+1)

	UpdateStoreRumourWindow ()
	GemRB.SetVisible(Window, 1)


def OpenStoreRentWindow ():
	global StoreRentWindow, RentIndex

	CloseWindows()
	
	StoreRentWindow = Window = GemRB.LoadWindow (7)

	# room types
	RentIndex = -1
	for i in range(4):
		ok = Store['StoreRoomPrices'][i]
		Button = GemRB.GetControl (Window, i)
		GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "UpdateStoreRentWindow")
		if ok<0:
			GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_DISABLED) #disabled room icons are selected, not disabled
		else:
			GemRB.SetVarAssoc (Window, Button, "RentIndex", i)
			if RentIndex==-1:
				RentIndex = i

		Button = GemRB.GetControl (Window, i+4)
		GemRB.SetText (Window, Button, 14294+i)
		GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "UpdateStoreRentWindow")
		GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_RADIOBUTTON, OP_OR)
		GemRB.SetVarAssoc (Window, Button, "RentIndex", i)
		if ok<0:
			GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_DISABLED)

	# Rent
	Button = GemRB.GetControl (Window, 11)
	GemRB.SetText (Window, Button, 14293)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "RentRoom")

	GemRB.SetVar ("RentIndex", RentIndex)

	UpdateStoreRentWindow ()
	GemRB.SetVisible(Window, 1)


def UpdateStoreCommon (Window, title, name, gold):

	Label = GemRB.GetControl (Window, title)
	GemRB.SetText (Window, Label, Store['StoreName'])

	if name:
		pc = GemRB.GameGetSelectedPCSingle ()
		Label = GemRB.GetControl (Window, name)
		GemRB.SetText (Window, Label, GemRB.GetPlayerName (pc, 0) )

	Label = GemRB.GetControl (Window, gold)
	GemRB.SetText (Window, Label, str(GemRB.GameGetPartyGold ()))
	

def UpdateStoreShoppingWindow ():
	global Store
 
	Window = StoreShoppingWindow
	#reget store in case of a change
	Store = GemRB.GetStore()
	LeftCount = Store['StoreItemCount']
	ScrollBar = GemRB.GetControl (Window, 11)
	GemRB.SetVarAssoc (Window, ScrollBar, "LeftTopIndex", LeftCount-3)

	pc = GemRB.GameGetSelectedPCSingle ()
	inventory_slots = GemRB.GetSlots (pc, -1)
	RightCount = len(inventory_slots)
	ScrollBar = GemRB.GetControl (Window, 12)
	GemRB.SetVarAssoc (Window, ScrollBar, "RightTopIndex", RightCount-3)
	RedrawStoreShoppingWindow ()


def SelectBuy ():
	Window = StoreShoppingWindow

	LeftIndex = GemRB.GetVar ("LeftTopIndex")+GemRB.GetVar ("LeftIndex")
	pc = GemRB.GameGetSelectedPCSingle ()
	GemRB.ChangeStoreItem (pc, LeftIndex, SHOP_BUY|SHOP_SELECT)
	RedrawStoreShoppingWindow ()


def BuyPressed ():
	Window = StoreShoppingWindow

	if (BuySum>GemRB.GameGetPartyGold()):
		#not enough money!
		return
		
	pc = GemRB.GameGetSelectedPCSingle ()
	LeftCount = Store['StoreItemCount']
	#going backwards because removed items shift the slots
	for Slot in range(LeftCount, 0, -1):
		Flags = GemRB.IsValidStoreItem (pc, Slot-1, ITEM_STORE) & SHOP_SELECT
		if Flags:
			GemRB.ChangeStoreItem (pc, Slot-1, SHOP_BUY)
	GemRB.GameSetPartyGold (GemRB.GameGetPartyGold()-BuySum)
	UpdateStoreShoppingWindow ()


def SelectSell ():
	Window = StoreShoppingWindow

	GemRB.SetVar ("SellSum",0)
	RightIndex = GemRB.GetVar ("RightTopIndex")+GemRB.GetVar ("RightIndex")
	pc = GemRB.GameGetSelectedPCSingle ()
	inventory_slots = GemRB.GetSlots (pc, -1)
	GemRB.ChangeStoreItem (pc, inventory_slots[RightIndex], SHOP_SELL|SHOP_SELECT)
	RedrawStoreShoppingWindow ()


def SellPressed ():
	global BuySum
	
	Window = StoreShoppingWindow

	pc = GemRB.GameGetSelectedPCSingle ()
	inventory_slots = GemRB.GetSlots (pc, -1)
	RightCount = len (inventory_slots)
	#no need to go reverse
	for Slot in range(RightCount):
		Flags = GemRB.IsValidStoreItem (pc, inventory_slots[Slot], ITEM_PC) & SHOP_SELECT
		if Flags:
			GemRB.ChangeStoreItem (pc, inventory_slots[Slot], SHOP_SELL)

	GemRB.GameSetPartyGold (GemRB.GameGetPartyGold()+SellSum)
	UpdateStoreShoppingWindow ()


def RedrawStoreShoppingWindow ():
	global BuySum
	
	Window = StoreShoppingWindow

	UpdateStoreCommon (Window, 0x10000003, 0x1000002e, 0x1000002a)
	pc = GemRB.GameGetSelectedPCSingle ()

	BuySum = 0
	LeftTopIndex = GemRB.GetVar ("LeftTopIndex")
	RightTopIndex = GemRB.GetVar ("RightTopIndex")
	LeftCount = Store['StoreItemCount']
	for i in range(LeftCount):
		if GemRB.IsValidStoreItem (pc, i, ITEM_STORE) & SHOP_SELECT:
			Slot = GemRB.GetStoreItem (i)
			Item = GemRB.GetItem (Slot['ItemResRef'])
			Price = Item['Price'] * Store['SellMarkup'] / 100
			if Price <= 0:
				Price = 1
			BuySum = BuySum + Price

	SellSum = 0
	inventory_slots = GemRB.GetSlots (pc, -1)
	RightCount = len(inventory_slots)
	for i in range(RightCount):
		if GemRB.IsValidStoreItem (pc, inventory_slots[i], ITEM_PC) & SHOP_SELECT:
			Slot = GemRB.GetSlotItem (pc, inventory_slots[i])
			Item = GemRB.GetItem (Slot['ItemResRef'])
			Price = Item['Price'] * Store['BuyMarkup'] / 100
			SellSum = SellSum + Price

	Label = GemRB.GetControl (Window, 0x1000002b)
	GemRB.SetText (Window, Label, str(BuySum) )

	Label = GemRB.GetControl (Window, 0x1000002c)
	GemRB.SetText (Window, Label, str(SellSum) )

	for i in range(4):
		Slot = GemRB.GetStoreItem (i+LeftTopIndex)
		Button = GemRB.GetControl (Window, i+5)
		Label = GemRB.GetControl (Window, 0x10000012+i)
		if Slot != None:
			Flags = GemRB.IsValidStoreItem (pc, i+LeftTopIndex, ITEM_STORE)
			Item = GemRB.GetItem (Slot['ItemResRef'])
			GemRB.SetItemIcon (Window,Button, Slot['ItemResRef'],0)
			GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_NO_IMAGE, OP_NAND)
			GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_PICTURE, OP_OR)
			if Flags & SHOP_BUY:
				if Flags & SHOP_SELECT:
					GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_SELECTED)
				else:
					GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_ENABLED)
			else:
				GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_DISABLED)

			if Flags & SHOP_ID:
				GemRB.SetToken ("ITEMNAME", GemRB.GetString (Item['ItemName']))
				GemRB.EnableButtonBorder (Window, Button, 0, 1)
			else:
				GemRB.SetToken ("ITEMNAME", GemRB.GetString (Item['ItemNameIdentified']))
				GemRB.EnableButtonBorder (Window, Button, 0, 0)
				
			Price = Item['Price'] * Store['SellMarkup'] / 100
			if Price <= 0:
				Price = 1
			GemRB.SetToken ("ITEMCOST", str(Price) )
			GemRB.SetText (Window, Label, 10162)
		else:
			GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_DISABLED)
			GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_NO_IMAGE, OP_OR)
			GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_PICTURE, OP_NAND)
			GemRB.SetText (Window, Label, "")

		if i+RightTopIndex<RightCount:
			Slot = GemRB.GetSlotItem (pc, inventory_slots[i+RightTopIndex])
		else:
			Slot = None
		Button = GemRB.GetControl (Window, i+13)
		Label = GemRB.GetControl (Window, 0x1000001e+i)
		if Slot != None:
			Flags = GemRB.IsValidStoreItem (pc, inventory_slots[i+RightTopIndex], ITEM_PC)
			Item = GemRB.GetItem (Slot['ItemResRef'])
			GemRB.SetItemIcon (Window,Button, Slot['ItemResRef'],0)
			GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_NO_IMAGE, OP_NAND)
			GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_PICTURE, OP_OR)
			Price = Item['Price'] * Store['BuyMarkup'] / 100
			if (Price>0) and (Flags & SHOP_SELL):
				if Flags & SHOP_SELECT:
					GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_SELECTED)
				else:
					GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_ENABLED)
			else:
				GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_DISABLED)

			if Flags & SHOP_ID:
				GemRB.SetToken ("ITEMNAME", GemRB.GetString (Item['ItemName']))
				GemRB.EnableButtonBorder (Window, Button, 0, 1)
			else:
				GemRB.SetToken ("ITEMNAME", GemRB.GetString (Item['ItemNameIdentified']))
				GemRB.EnableButtonBorder (Window, Button, 0, 0)
				
			GemRB.SetToken ("ITEMCOST", str(Price) )
			GemRB.SetText (Window, Label, 10162)
		else:
			GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_DISABLED)
			GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_NO_IMAGE, OP_OR)
			GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_PICTURE, OP_NAND)
			GemRB.SetText (Window, Label, "")


def UpdateStoreIdentifyWindow ():
	Window = StoreIdentifyWindow

	pc = GemRB.GameGetSelectedPCSingle ()
	inventory_slots = GemRB.GetSlots (pc, -1)
	Count = len(inventory_slots)
	ScrollBar = GemRB.GetControl (Window, 7)
	GemRB.SetVarAssoc (Window, ScrollBar, "TopIndex", Count-3)
	RedrawStoreIdentifyWindow ()


def RedrawStoreIdentifyWindow ():
	Window = StoreIdentifyWindow

	UpdateStoreCommon (Window, 0x10000000, 0x10000005, 0x10000001)
	TopIndex = GemRB.GetVar ("TopIndex")
	Index = GemRB.GetVar ("Index")
	pc = GemRB.GameGetSelectedPCSingle ()
	inventory_slots = GemRB.GetSlots (pc, -1)
	Count = len(inventory_slots)
	IDPrice = Store['IDPrice']
	for i in range(4):
		if i+TopIndex<Count:
			Slot = GemRB.GetSlotItem (pc, inventory_slots[i+TopIndex])
		else:
			Slot = None
		Button = GemRB.GetControl (Window, i+8)
		Label = GemRB.GetControl (Window, 0x1000000c+i)
		if Slot != None:
			Flags = GemRB.IsValidStoreItem (pc, inventory_slots[i+TopIndex], ITEM_PC)
			Item = GemRB.GetItem (Slot['ItemResRef'])
			GemRB.SetVarAssoc (Window, Button, "Index", TopIndex+i)
			GemRB.SetItemIcon (Window,Button, Slot['ItemResRef'],0)
			GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_NO_IMAGE, OP_NAND)
			GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_PICTURE, OP_OR)
			Label = GemRB.GetControl (Window, 0x10000003)
			GemRB.SetText (Window, Label, str(IDPrice))
			if Flags & SHOP_ID:
				GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_ENABLED)
				GemRB.SetToken ("ITEMNAME", GemRB.GetString (Item['ItemName']))
				GemRB.EnableButtonBorder (Window, Button, 0, 1)
			else:
				GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_DISABLED)
				GemRB.SetToken ("ITEMNAME", GemRB.GetString (Item['ItemNameIdentified']))
				GemRB.EnableButtonBorder (Window, Button, 0, 0)
				
			GemRB.SetToken ("ITEMCOST", str(IDPrice) )
			GemRB.SetText (Window, Label, 10162)
		else:
			GemRB.SetVarAssoc (Window, Button, "Index", -1)
			GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_DISABLED)
			GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_NO_IMAGE, OP_OR)
			GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_PICTURE, OP_NAND)
			GemRB.SetText (Window, Label, "")

	Button = GemRB.GetControl (Window, 5)
	if Index >= 0 and Count > Index:
		GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_ENABLED)
	else:
		GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_DISABLED)


def InfoIdentifyWindow ():
	global MessageWindow

	UpdateStoreIdentifyWindow ()
	Index = GemRB.GetVar ("Index")
	pc = GemRB.GameGetSelectedPCSingle ()
	inventory_slots = GemRB.GetSlots (pc, -1)
	Count = len(inventory_slots)
	if Index >= Count:
		return
	Slot = GemRB.GetSlotItem (inventory_slots[Index])
	Item = GemRB.GetItem (Slot['ItemResRef'])
	#identify the item
	GemRB.ChangeStoreItem (pc, inventory_slots[Index], SHOP_ID)
	#deduce gold from player
	IDPrice = Store['IDPrice']
	GemRB.GameSetPartyGold (GemRB.GameGetPartyGold()-IDPrice)

	MessageWindow = Window = GemRB.LoadWindow (12)

	#description bam
	Button = GemRB.GetControl (Window, 7)
	GemRB.SetItemIcon (Window, Button, Slot['ItemResRef'],1)

	Label = GemRB.GetControl (Window, 0x10000007)
	GemRB.SetText (Window, Label, Item['ItemName'])

	#slot bam
	Button = GemRB.GetControl (Window, 2)
	GemRB.SetItemIcon (Window, Button, Slot['ItemResRef'],0)

	TextArea = GemRB.GetControl (Window, 5)
	GemRB.SetText (Window, TextArea, Item['ItemDescIdentified'])

	#Done
	Button = GemRB.GetControl (Window, 4)
	GemRB.SetText (Window, Button, 11973)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "ErrorDone")

	GemRB.ShowModal (Window, MODAL_SHADOW_GRAY)


def UpdateStoreStealWindow ():
	global Store
 
	Window = StoreStealWindow

	#reget store in case of a change
	Store = GemRB.GetStore()
	LeftCount = Store['StoreItemCount']
	ScrollBar = GemRB.GetControl (Window, 9)
	GemRB.SetVarAssoc (Window, ScrollBar, "LeftTopIndex", LeftCount-3)

	pc = GemRB.GameGetSelectedPCSingle ()
	inventory_slots = GemRB.GetSlots (pc, -1)
	RightCount = len(inventory_slots)
	ScrollBar = GemRB.GetControl (Window, 10)
	GemRB.SetVarAssoc (Window, ScrollBar, "RightTopIndex", RightCount-3)
	RedrawStoreStealWindow ()


def RedrawStoreStealWindow ():
	Window = StoreStealWindow

	UpdateStoreCommon (Window, 0x10000002, 0x10000027, 0x10000023)
	LeftTopIndex = GemRB.GetVar ("LeftTopIndex")
	LeftIndex = GemRB.GetVar ("LeftIndex")
	RightTopIndex = GemRB.GetVar ("RightTopIndex")
	RightIndex = GemRB.GetVar ("RightIndex")
	LeftCount = Store['StoreItemCount']
	pc = GemRB.GameGetSelectedPCSingle ()
	inventory_slots = GemRB.GetSlots (pc, -1)
	RightCount = len(inventory_slots)
	for i in range(4):
		Slot = GemRB.GetStoreItem (i+LeftTopIndex)
		Button = GemRB.GetControl (Window, i+4)
		Label = GemRB.GetControl (Window, 0x1000000f+i)
		if Slot != None:
			Flags = GemRB.IsValidStoreItem (pc, i+LeftTopIndex, ITEM_STORE)
			Item = GemRB.GetItem (Slot['ItemResRef'])
			GemRB.SetItemIcon (Window,Button, Slot['ItemResRef'],0)
			GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_NO_IMAGE, OP_NAND)
			GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_PICTURE, OP_OR)
			if Flags & SHOP_STEAL:
				GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_ENABLED)
			else:
				GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_DISABLED)

			if Flags & SHOP_ID:
				GemRB.SetToken ("ITEMNAME", GemRB.GetString (Item['ItemName']))
				GemRB.EnableButtonBorder (Window, Button, 0, 1)
			else:
				GemRB.SetToken ("ITEMNAME", GemRB.GetString (Item['ItemNameIdentified']))
				GemRB.EnableButtonBorder (Window, Button, 0, 0)
				
			GemRB.SetToken ("ITEMCOST", str(Slot['Price']) )
			GemRB.SetText (Window, Label, 10162)
		else:
			GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_DISABLED)
			GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_NO_IMAGE, OP_OR)
			GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_PICTURE, OP_NAND)
			GemRB.SetText (Window, Label, "")

		if i+RightTopIndex<RightCount:
			Slot = GemRB.GetSlotItem (pc, inventory_slots[i+RightTopIndex])
		else:
			Slot = None
		Button = GemRB.GetControl (Window, i+11)
		Label = GemRB.GetControl (Window, 0x10000019+i)
		if Slot != None:
			Flags = GemRB.IsValidStoreItem (pc, inventory_slots[i+RightTopIndex], ITEM_PC)
			Item = GemRB.GetItem (Slot['ItemResRef'])
			GemRB.SetVarAssoc (Window, Button, "RightIndex", RightTopIndex+i)
			GemRB.SetItemIcon (Window,Button, Slot['ItemResRef'],0)
			GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_NO_IMAGE, OP_NAND)
			GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_PICTURE, OP_OR)
			Price = Item['Price'] * Store['BuyMarkup'] / 100
			GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_DISABLED)
			if Flags & SHOP_ID:
				GemRB.SetToken ("ITEMNAME", GemRB.GetString (Item['ItemName']))
				GemRB.EnableButtonBorder (Window, Button, 0, 1)
			else:
				GemRB.SetToken ("ITEMNAME", GemRB.GetString (Item['ItemNameIdentified']))
				GemRB.EnableButtonBorder (Window, Button, 0, 0)

			GemRB.SetToken ("ITEMCOST", str(Price) )
			GemRB.SetText (Window, Label, 10162)
		else:
			GemRB.SetVarAssoc (Window, Button, "RightIndex", -1)
			GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_DISABLED)
			GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_NO_IMAGE, OP_OR)
			GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_PICTURE, OP_NAND)
			GemRB.SetText (Window, Label, "")
			

def UpdateStoreDonateWindow ():
	Window = StoreDonateWindow

	UpdateStoreCommon (Window, 0x10000007, 0, 0x10000008)
	Field = GemRB.GetControl (Window, 5)
	donation = int("0"+GemRB.QueryText (Window, Field))
	gold = GemRB.GameGetPartyGold ()
	if donation>gold:
		donation = gold
		GemRB.SetText (Window, Field, str(gold) )

	Button = GemRB.GetControl (Window, 3)
	if donation:
		GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_ENABLED)
	else:
		GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_DISABLED)


def IncrementDonation ():
	Window = StoreDonateWindow

	Field = GemRB.GetControl (Window, 5)
	donation = int("0"+GemRB.QueryText (Window, Field))
	if donation<GemRB.GameGetPartyGold ():
		GemRB.SetText (Window, Field, str(donation+1) )
	else:
		GemRB.SetText (Window, Field, str(GemRB.GameGetPartyGold ()) )
	UpdateStoreDonateWindow ()
		
def DecrementDonation ():
	Window = StoreDonateWindow

	Field = GemRB.GetControl (Window, 5)
	donation = int("0"+GemRB.QueryText (Window, Field))
	if donation>0:
		GemRB.SetText (Window, Field, str(donation-1) )
	else:
		GemRB.SetText (Window, Field, str(0) )
	UpdateStoreDonateWindow ()

def DonateGold ():
	Window = StoreDonateWindow
	
	TextArea = GemRB.GetControl (Window, 0)	
	GemRB.SetTextAreaFlags (Window, TextArea, IE_GUI_TEXTAREA_AUTOSCROLL)

	Button = GemRB.GetControl (Window, 10)
	GemRB.SetAnimation (Window, Button, "DONATE")

	Field = GemRB.GetControl (Window, 5)
	donation = int("0"+GemRB.QueryText (Window, Field))
	GemRB.GameSetPartyGold (GemRB.GameGetPartyGold ()-donation)
	if GemRB.IncreaseReputation( donation ):
		GemRB.TextAreaAppend (Window, TextArea, 10468, -1)
		GemRB.PlaySound ("act_03")
		UpdateStoreDonateWindow ()
		return

	GemRB.TextAreaAppend (Window, TextArea, 10469, -1)
	GemRB.PlaySound ("act_03e")
	UpdateStoreDonateWindow ()
	
	
def UpdateStoreHealWindow ():
	Window = StoreHealWindow

	UpdateStoreCommon (Window, 0x10000000, 0, 0x10000001)
	TopIndex = GemRB.GetVar ("TopIndex")
	Index = GemRB.GetVar ("Index")
	for i in range(4):
		Cure = GemRB.GetStoreCure (i+TopIndex)

		Button = GemRB.GetControl (Window, i+8)
		Label = GemRB.GetControl (Window, 0x1000000c+i)
		if Cure != None:
			GemRB.SetVarAssoc (Window, Button, "Index", TopIndex+i)
			Spell = GemRB.GetSpell (Cure['CureResRef'])
			GemRB.SetSpellIcon (Window,Button, Cure['CureResRef'],1)
			GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_NO_IMAGE, OP_NAND)
			GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_PICTURE, OP_OR)

			GemRB.SetToken ("ITEMNAME", GemRB.GetString (Spell['SpellName']))
			GemRB.SetToken ("ITEMCOST", str(Cure['Price']) )
			GemRB.SetText (Window, Label, 10162)

		else:
			GemRB.SetVarAssoc (Window, Button, "Index", -1)
			GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_DISABLED)
			GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_NO_IMAGE, OP_OR)
			GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_PICTURE, OP_NAND)
			GemRB.SetText (Window, Label, "")
		if i==Index:
			TextArea = GemRB.GetControl (Window, 23)
			GemRB.SetText (Window, TextArea, Cure['Description'])
			Label = GemRB.GetControl (Window, 0x10000003)
			GemRB.SetText (Window, Label, str(Cure['Price']) )
			Button = GemRB.GetControl (Window, 5)
			GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_ENABLED)


def InfoHealWindow ():
	global MessageWindow

	UpdateStoreHealWindow ()
	Index = GemRB.GetVar ("Index")
	Cure = GemRB.GetStoreCure (Index)
	Spell = GemRB.GetSpell (Cure['CureResRef'])

	MessageWindow = Window = GemRB.LoadWindow (14)

	Label = GemRB.GetControl (Window, 0x10000000)
	GemRB.SetText (Window, Label, Spell['SpellName'])

	Button = GemRB.GetControl (Window, 2)
	GemRB.SetSpellIcon (Window, Button, Cure['CureResRef'],1)

	TextArea = GemRB.GetControl (Window, 3)
	GemRB.SetText (Window, TextArea, Spell['SpellDesc'])

	#Done
	Button = GemRB.GetControl (Window, 5)
	GemRB.SetText (Window, Button, 11973)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "ErrorDone")

	GemRB.ShowModal (Window, MODAL_SHADOW_GRAY)


def BuyHeal ():
	Index = GemRB.GetVar ("Index")
	Cure = GemRB.GetStoreCure (Index)
	gold = GemRB.GameGetPartyGold ()
	if gold < Cure['Price']:
		ErrorWindow (11048)
		return

	GemRB.GameSetPartyGold (gold-Cure['Price'])
	pc = GemRB.GameGetSelectedPCSingle ()
	UpdateStoreHealWindow ()


def UpdateStoreRumourWindow ():
	Window = StoreRumourWindow

	UpdateStoreCommon (Window, 0x10000011, 0, 0x10000012)
	TopIndex = GemRB.GetVar ("TopIndex")
	for i in range(5):
		Drink = GemRB.GetStoreDrink (i+TopIndex)
		Button = GemRB.GetControl (Window, i)
		if Drink != None:
			GemRB.SetToken ("ITEMNAME", GemRB.GetString (Drink['DrinkName']))
			GemRB.SetToken ("ITEMCOST", str(Drink['Price']) )
			GemRB.SetText (Window, Button, 10162)
			GemRB.SetVarAssoc (Window, Button, "Index", i)
			GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_ENABLED)
			GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "GulpDrink")
		else:
			GemRB.SetText (Window, Button, "")
			GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_DISABLED)

def GulpDrink ():
	Window = StoreRumourWindow

	TextArea = GemRB.GetControl (Window, 13)
	GemRB.SetTextAreaFlags (Window, TextArea, IE_GUI_TEXTAREA_AUTOSCROLL)
	pc = GemRB.GameGetSelectedPCSingle ()
	intox = GemRB.GetPlayerStat (pc, IE_INTOXICATION)
	intox = 0
	if intox > 80:
		GemRB.TextAreaAppend (Window, TextArea, 10832, -1)
		return

	gold = GemRB.GameGetPartyGold ()
	Index = GemRB.GetVar ("TopIndex")+GemRB.GetVar ("Index")
	Drink = GemRB.GetStoreDrink (Index)
	if gold < Drink['Price']:
		ErrorWindow (11049)
		return

	GemRB.GameSetPartyGold (gold-Drink['Price'])
	GemRB.SetPlayerStat (pc, IE_INTOXICATION, intox+Drink['Strength'])
	text = GemRB.GetRumour (Drink['Strength'], Store['TavernRumour'])
	GemRB.TextAreaAppend (Window, TextArea, text, -1)
	GemRB.PlaySound ("gam_07")
	UpdateStoreRumourWindow ()


def UpdateStoreRentWindow ():
	global RentIndex

	Window = StoreRentWindow
	UpdateStoreCommon (Window, 0x10000008, 0, 0x10000009)
	RentIndex = GemRB.GetVar ("RentIndex")
	TextArea = GemRB.GetControl (Window, 12)
	GemRB.SetText (Window, TextArea, roomtypes[RentIndex] )
	Label = GemRB.GetControl (Window, 0x1000000d)
	price = Store['StoreRoomPrices'][RentIndex]
	GemRB.SetText (Window, Label, str(price) )


def RentRoom ():
	global RentIndex

	Window = StoreRentWindow
	RentIndex = GemRB.GetVar ("RentIndex")
	price = Store['StoreRoomPrices'][RentIndex]
	Gold = GemRB.GameGetPartyGold ()
	TextArea = GemRB.GetControl (Window, 12)
	if Gold<price:
		ErrorWindow (11051)
		return

	GemRB.GameSetPartyGold (Gold-price)
	GemRB.RestParty (13, 1, 0)
	UpdateStoreRentWindow ()


def CloseStoreShoppingWindow ():
	global StoreShoppingWindow
	
	if StoreShoppingWindow != None:
		GemRB.UnloadWindow (StoreShoppingWindow)
		StoreShoppingWindow = None


def CloseStoreIdentifyWindow ():
	global StoreIdentifyWindow
	
	if StoreIdentifyWindow != None:
		GemRB.UnloadWindow (StoreIdentifyWindow)
		StoreIdentifyWindow = None


def CloseStoreStealWindow ():
	global StoreStealWindow
	
	if StoreStealWindow != None:
		GemRB.UnloadWindow (StoreStealWindow)
		StoreStealWindow = None


def CloseStoreDonateWindow ():
	global StoreDonateWindow
	
	if StoreDonateWindow != None:
		GemRB.UnloadWindow (StoreDonateWindow)
		StoreDonateWindow = None


def CloseStoreHealWindow ():
	global StoreHealWindow
	
	if StoreHealWindow != None:
		GemRB.UnloadWindow (StoreHealWindow)
		StoreHealWindow = None


def CloseStoreRumourWindow ():
	global StoreRumourWindow
	
	if StoreRumourWindow != None:
		GemRB.UnloadWindow (StoreRumourWindow)
		StoreRumourWindow = None


def CloseStoreRentWindow ():
	global StoreRentWindow
	
	if StoreRentWindow != None:
		GemRB.UnloadWindow (StoreRentWindow)
		StoreRentWindow = None


def ErrorWindow (strref):
	global MessageWindow

	MessageWindow = Window = GemRB.LoadWindow (10)

	TextArea = GemRB.GetControl (Window, 3)
	GemRB.SetText (Window, TextArea, strref)

	#done
	Button = GemRB.GetControl (Window, 0)
	GemRB.SetText (Window, Button, 11973)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "ErrorDone")

	GemRB.ShowModal (Window, MODAL_SHADOW_GRAY)


def ErrorDone ():
	GemRB.UnloadWindow (MessageWindow)


###################################################
# End of file GUISTORE.py
