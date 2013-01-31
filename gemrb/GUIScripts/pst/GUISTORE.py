# -*-python-*-
# GemRB - Infinity Engine Emulator
# Copyright (C) 2003 The GemRB Project
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
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#


# GUISTORE.py - script to open store/inn/temple windows

###################################################

import GemRB
import GUICommon
import GUICommonWindows
from GUIDefines import *
from ie_stats import *
from ie_slots import *

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
RentConfirmWindow = None
LeftButton = None
RightButton = None
CureTable = None

ITEM_PC    = 0
ITEM_STORE = 1

Inventory = None
RentIndex = -1
Store = None
Buttons = [-1,-1,-1,-1]
inventory_slots = ()
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
storetips = (44970, 44971, 44972, 45118, 45121, 45119, 45120)
roomtypes = (66865, 66866, 66867, 66868)
store_funcs = None

def CloseWindows ():
	global StoreShoppingWindow, StoreIdentifyWindow, StoreStealWindow
	global StoreHealWindow, StoreDonateWindow, StoreRumourWindow, StoreRentWindow

	for win in StoreShoppingWindow, StoreIdentifyWindow, StoreStealWindow, StoreHealWindow, StoreDonateWindow, StoreRumourWindow, StoreRentWindow:
		if win:
			win.Unload ()

	StoreShoppingWindow = StoreIdentifyWindow = StoreStealWindow = StoreHealWindow = StoreDonateWindow = StoreRumourWindow = StoreRentWindow = None
	return

def CloseStoreWindow ():
	import GUIINV
	global StoreWindow, ActionWindow, PortraitWindow
	global OldPortraitWindow

	GemRB.SetVar ("Inventory", 0)
	CloseWindows ()
	if StoreWindow:
		StoreWindow.Unload ()
	if ActionWindow:
		ActionWindow.Unload ()
	if PortraitWindow:
		PortraitWindow.Unload ()
	StoreWindow = None
	GemRB.LeaveStore ()
	GUICommonWindows.PortraitWindow = OldPortraitWindow
	if Inventory: # broken if available
		GUIINV.OpenInventoryWindow ()
	else:
		GUICommon.GameWindow.SetVisible(WINDOW_VISIBLE) #enabling the game control screen
		GemRB.UnhideGUI () #enabling the other windows
		GUICommonWindows.SetSelectionChangeHandler( None )

	CureTable = None
	return

def OpenStoreWindow ():
	global Store
	global StoreWindow, ActionWindow, PortraitWindow
	global OldPortraitWindow
	global store_funcs
	global Inventory
	global CureTable

	CureTable = GemRB.LoadTable("speldesc") #additional info not supported by core

	#these are function pointers, not strings
	#can't put this in global init, doh!
	store_funcs = (OpenStoreShoppingWindow,
	OpenStoreIdentifyWindow,OpenStoreStealWindow,
	OpenStoreHealWindow, OpenStoreDonateWindow,
	OpenStoreRumourWindow,OpenStoreRentWindow )

	GemRB.HideGUI ()
	GUICommon.GameWindow.SetVisible(WINDOW_INVISIBLE) #removing the game control screen

	if GemRB.GetVar ("Inventory"):
		Inventory = 1
	else:
		Inventory = None

	GemRB.SetVar ("Action", 0)
	GemRB.LoadWindowPack ("GUISTORE", 640, 480)
	StoreWindow = Window = GemRB.LoadWindow (3)
	#saving the original portrait window
	OldPortraitWindow = GUICommonWindows.PortraitWindow
	PortraitWindow = GUICommonWindows.OpenPortraitWindow (0)
	ActionWindow = GemRB.LoadWindow (0)
	#this window is static and grey, but good to stick the frame onto
	ActionWindow.SetFrame ()

	Store = GemRB.GetStore ()

	# Done
	Button = Window.GetControl (0)
	Button.SetText (1403)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, CloseStoreWindow)

	#Store type icon
	Button = Window.GetControl (5)
	#Button.SetSprites (storebams[Store['StoreType']],0,0,0,0,0)

	#based on shop type, these buttons will change
	store_type = Store['StoreType']
	store_buttons = Store['StoreButtons']
	for i in range (4):
		Buttons[i] = Button = Window.GetControl (i+1)
		Action = store_buttons[i]
		Button.SetVarAssoc ("Action", i)
		if Action>=0:
			Button.SetFlags (IE_GUI_BUTTON_RADIOBUTTON, OP_OR)
			#this is different from IWD???
			#Button.SetSprites ("SSBBS", Action, 0,1,2,0)
			Button.SetTooltip (storetips[Action])
			Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, store_funcs[Action])
			Button.SetState (IE_GUI_BUTTON_ENABLED)
		else:
			Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_OR)
			Button.SetTooltip ("")
			Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, None)
			Button.SetState (IE_GUI_BUTTON_DISABLED)

	ActionWindow.SetVisible (WINDOW_VISIBLE)
	Window.SetVisible (WINDOW_VISIBLE)
	store_funcs[store_buttons[0]] ()
	PortraitWindow.SetVisible (WINDOW_VISIBLE)
	return

def OpenStoreShoppingWindow ():
	global StoreShoppingWindow
	global LeftButton, RightButton

	CloseWindows()

	StoreShoppingWindow = Window = GemRB.LoadWindow (4)

	if Inventory:
		# Title
		Label = Window.GetControl (0xfffffff)
		Label.SetText (51881)
		# buy price ...
		Label = Window.GetControl (0x1000002b)
		Label.SetText ("")
		# sell price ...
		Label = Window.GetControl (0x1000002c)
		Label.SetText ("")
		# buy price ...
		Label = Window.GetControl (0x1000002f)
		Label.SetText ("")
		# sell price ...
		Label = Window.GetControl (0x10000030)
		Label.SetText ("")
	else:
		# buy price ...
		Label = Window.GetControl (0x10000003)
		Label.SetText ("0")

		# sell price ...
		Label = Window.GetControl (0x10000004)
		Label.SetText ("0")

	for i in range (4):
		Button = Window.GetControl (i+8)
		Button.SetBorder (0,0,0,0,0,0,0,128,160,0,1)
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, SelectBuy)
		Button.SetEvent (IE_GUI_BUTTON_ON_RIGHT_PRESS, InfoLeftWindow)

		Button = Window.GetControl (i+17)
		Button.SetBorder (0,0,0,0,0,0,0,128,160,0,1)
		if Store['StoreType'] != 3: # can't sell to temples
			Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, SelectSell)
		Button.SetEvent (IE_GUI_BUTTON_ON_RIGHT_PRESS, InfoRightWindow)

	# Buy
	LeftButton = Button = Window.GetControl (0)
	if Inventory:
		Button.SetText (51882)
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, ToBackpackPressed)
	else:
		Button.SetText (45303)
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, BuyPressed)

	# Sell
	RightButton = Button = Window.GetControl (1)
	if Inventory:
		Button.SetText (51883)
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, ToBagPressed)
	else:
		Button.SetText (45304)
		if Store['StoreType'] != 3: # can't sell to temples
			Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, SellPressed)

	## inactive button
	#Button = Window.GetControl (50)
	#Button.SetState (IE_GUI_BUTTON_LOCKED)
	#Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_SET)

	##backpack
	#Button = Window.GetControl (44)
	#Button.SetState (IE_GUI_BUTTON_LOCKED)

	## encumbrance
	Button = Window.GetControl (25)
	GUICommon.SetEncumbranceLabels (Window, 25, None, GemRB.GameGetSelectedPCSingle ())
	Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_OR)
	Button.SetState (IE_GUI_BUTTON_LOCKED)
	##Label = Window.CreateLabel (0x10000019, 15,325,60,15,"NUMBER","0:",IE_FONT_ALIGN_LEFT|IE_FONT_ALIGN_TOP)
	##Label = Window.CreateLabel (0x10000044, 15,365,80,15,"NUMBER","0:",IE_FONT_ALIGN_RIGHT|IE_FONT_ALIGN_TOP)

	# left scrollbar
	ScrollBar = Window.GetControl (7)
	ScrollBar.SetEvent (IE_GUI_SCROLLBAR_ON_CHANGE, RedrawStoreShoppingWindow)

	# right scrollbar
	ScrollBar = Window.GetControl (16)
	ScrollBar.SetEvent (IE_GUI_SCROLLBAR_ON_CHANGE, RedrawStoreShoppingWindow)

	GUICommonWindows.SetSelectionChangeHandler( UpdateStoreShoppingWindow )
	UpdateStoreShoppingWindow ()
	Window.SetVisible (WINDOW_VISIBLE)
	return

def OpenStoreIdentifyWindow ():
	global StoreIdentifyWindow

	GemRB.SetVar ("Index", -1)
	GemRB.SetVar ("TopIndex", 0)
	CloseWindows()

	StoreIdentifyWindow = Window = GemRB.LoadWindow (5)

	# Identify
	Button = Window.GetControl (4)
	Button.SetText (44971)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, IdentifyPressed)
	Button.SetEvent (IE_GUI_BUTTON_ON_RIGHT_PRESS, InfoIdentifyWindow)

	# price ...
	Label = Window.GetControl (0x10000001)
	Label.SetText ("0")

	# 8-11 item slots, 0x1000000c-f labels
	for i in range (4):
		Button = Window.GetControl (i+6)
		Button.SetFlags (IE_GUI_BUTTON_RADIOBUTTON, OP_OR)
		Button.SetBorder (0,0,0,0,0,0,0,128,160,0,1)
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, RedrawStoreIdentifyWindow)
		Button.SetEvent (IE_GUI_BUTTON_ON_RIGHT_PRESS, InfoIdentifyWindow)

	ScrollBar = Window.GetControl (5)
	ScrollBar.SetEvent (IE_GUI_SCROLLBAR_ON_CHANGE, RedrawStoreIdentifyWindow)

	GUICommonWindows.SetSelectionChangeHandler( UpdateStoreIdentifyWindow )
	UpdateStoreIdentifyWindow ()
	Window.SetVisible (WINDOW_VISIBLE)
	return

def OpenStoreStealWindow ():
	global StoreStealWindow
	global LeftButton

	GemRB.SetVar ("RightIndex", 0)
	GemRB.SetVar ("LeftIndex", 0)
	CloseWindows()

	StoreStealWindow = Window = GemRB.LoadWindow (7)

	for i in range (4):
		Button = Window.GetControl (i+5)
		Button.SetBorder (0,0,0,0,0,0,0,128,160,0,1)
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, RedrawStoreStealWindow)

		Button = Window.GetControl (i+14)
		Button.SetBorder (0,0,0,0,0,0,0,128,160,0,1)
		Button.SetEvent (IE_GUI_BUTTON_ON_RIGHT_PRESS, InfoRightWindow)

	# Steal
	LeftButton = Button = Window.GetControl (0)
	Button.SetText (45305)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, StealPressed)

	# encumbrance
	Button = Window.GetControl (22)
	Button.SetState (IE_GUI_BUTTON_LOCKED)
	GUICommon.SetEncumbranceLabels (Window, 22, None, GemRB.GameGetSelectedPCSingle ())
	Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_OR)
	#Label = Window.CreateLabel (0x10000043, 15,325,60,15,"NUMBER","0:",IE_FONT_ALIGN_LEFT|IE_FONT_ALIGN_TOP)
	#Label = Window.CreateLabel (0x10000044, 15,365,80,15,"NUMBER","0:",IE_FONT_ALIGN_RIGHT|IE_FONT_ALIGN_TOP)

	# left scrollbar
	ScrollBar = Window.GetControl (4)
	ScrollBar.SetEvent (IE_GUI_SCROLLBAR_ON_CHANGE, RedrawStoreStealWindow)

	# right scrollbar
	ScrollBar = Window.GetControl (13)
	ScrollBar.SetEvent (IE_GUI_SCROLLBAR_ON_CHANGE, RedrawStoreStealWindow)

	GUICommonWindows.SetSelectionChangeHandler( UpdateStoreStealWindow )
	UpdateStoreStealWindow ()
	Window.SetVisible (WINDOW_VISIBLE)
	return

def OpenStoreDonateWindow ():
	global StoreDonateWindow

	CloseWindows ()

	StoreDonateWindow = Window = GemRB.LoadWindow (10)

	## graphics
	#Button = Window.GetControl (10)
	#Button.SetFlags (IE_GUI_BUTTON_PICTURE|IE_GUI_BUTTON_ANIMATED|IE_GUI_BUTTON_PLAYONCE, OP_OR)
	#Button.SetState (IE_GUI_BUTTON_LOCKED)

	# Donate
	Button = Window.GetControl (2)
	Button.SetText (45307)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, DonateGold)
	Button.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)

	# Entry
	Field = Window.GetControl (3)
	Field.SetText ("0")
	Field.SetEvent (IE_GUI_EDIT_ON_CHANGE, UpdateStoreDonateWindow)
	Field.SetStatus (IE_GUI_EDIT_NUMBER|IE_GUI_CONTROL_FOCUSED)

	# +
	Button = Window.GetControl (4)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, IncrementDonation)
	# -
	Button = Window.GetControl (5)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, DecrementDonation)

	GUICommonWindows.SetSelectionChangeHandler( UpdateStoreDonateWindow )
	UpdateStoreDonateWindow ()
	Window.SetVisible (WINDOW_VISIBLE)
	return

def OpenStoreHealWindow ():
	global StoreHealWindow

	GemRB.SetVar ("Index", -1)
	GemRB.SetVar ("TopIndex", 0)
	CloseWindows()

	StoreHealWindow = Window = GemRB.LoadWindow (6)

	#spell buttons
	for i in range (4):
		Button = Window.GetControl (i+5)
		Button.SetFlags (IE_GUI_BUTTON_RADIOBUTTON, OP_OR)
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, UpdateStoreHealWindow)
		Button.SetEvent (IE_GUI_BUTTON_ON_RIGHT_PRESS, InfoHealWindow)

	# price tag
	Label = Window.GetControl (0x10000001)
	Label.SetText ("0")

	# Heal
	Button = Window.GetControl (3)
	Button.SetText (8836)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, BuyHeal)
	Button.SetState (IE_GUI_BUTTON_DISABLED)

	ScrollBar = Window.GetControl (4)
	ScrollBar.SetEvent (IE_GUI_SCROLLBAR_ON_CHANGE, UpdateStoreHealWindow)
	Count = Store['StoreCureCount']
	if Count>4:
		Count = Count-4
	else:
		Count = 0
	ScrollBar.SetVarAssoc ("TopIndex", Count+1)

	UpdateStoreHealWindow ()
	Window.SetVisible (WINDOW_VISIBLE)
	return

def OpenStoreRumourWindow ():
	global StoreRumourWindow

	GemRB.SetVar ("TopIndex", 0)
	CloseWindows()

	StoreRumourWindow = Window = GemRB.LoadWindow (9)

	#removing those pesky labels
	for i in range (5):
		Window.DeleteControl (0x10000005+i)

	TextArea = Window.GetControl (13)
	TextArea.SetText (0)

	##tavern quality image
	#BAM = "TVRNQUL%d"% ((Store['StoreFlags']>>9)&3)
	#Button = Window.GetControl (12)
	#Button.SetSprites (BAM, 0, 0, 0, 0, 0)
	#Button.SetState (IE_GUI_BUTTON_LOCKED)

	ScrollBar = Window.GetControl (5)
	ScrollBar.SetEvent (IE_GUI_SCROLLBAR_ON_CHANGE, UpdateStoreRumourWindow)
	Count = Store['StoreDrinkCount']
	if Count>4:
		Count = Count-4
	else:
		Count = 0
	ScrollBar.SetVarAssoc ("TopIndex", Count+1)

	UpdateStoreRumourWindow ()
	Window.SetVisible (WINDOW_VISIBLE)
	return

def OpenStoreRentWindow ():
	global StoreRentWindow, RentIndex

	CloseWindows()

	StoreRentWindow = Window = GemRB.LoadWindow (8)

	# room types
	RentIndex = -1
	room_type = (45308, 45310, 45313, 45316)
	for i in range (4):
		ok = Store['StoreRoomPrices'][i]
		Button = Window.GetControl (i)
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, UpdateStoreRentWindow)
		if ok<0:
			Button.SetState (IE_GUI_BUTTON_DISABLED) #disabled room icons are selected, not disabled
		else:
			Button.SetVarAssoc ("RentIndex", i)
			if RentIndex==-1:
				RentIndex = i

		Button = Window.GetControl (i+4)
		Button.SetText (room_type[i])
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, UpdateStoreRentWindow)
		Button.SetFlags (IE_GUI_BUTTON_RADIOBUTTON, OP_OR)
		Button.SetVarAssoc ("RentIndex", i)
		if ok<0:
			Button.SetState (IE_GUI_BUTTON_DISABLED)

	# Rent
	Button = Window.GetControl (8)
	Button.SetText (45306)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, RentRoom)

	GemRB.SetVar ("RentIndex", RentIndex)

	UpdateStoreRentWindow ()
	Window.SetVisible (WINDOW_VISIBLE)
	return

def UpdateStoreCommon (Window, title, name, gold):

	Label = Window.GetControl (title)
	Label.SetText (GemRB.GetString (Store['StoreName']).upper ())

	if name:
		pc = GemRB.GameGetSelectedPCSingle ()
		Label = Window.GetControl (name)
		Label.SetText (GemRB.GetPlayerName (pc, 0) )

	Label = Window.GetControl (gold)
	Label.SetText (str(GemRB.GameGetPartyGold ()))
	return

def UpdateStoreShoppingWindow ():
	global Store, inventory_slots

	Window = StoreShoppingWindow
	#reget store in case of a change
	Store = GemRB.GetStore ()
	LeftCount = Store['StoreItemCount']-3
	if LeftCount<0:
		LeftCount=0
	ScrollBar = Window.GetControl (7)
	ScrollBar.SetVarAssoc ("LeftTopIndex", LeftCount)
	LeftTopIndex = GemRB.GetVar ("LeftTopIndex")
	if LeftTopIndex>LeftCount:
		GemRB.SetVar ("LeftTopIndex", LeftCount)

	pc = GemRB.GameGetSelectedPCSingle ()
	inventory_slots = GemRB.GetSlots (pc, SLOT_INVENTORY)
	RightCount = len(inventory_slots)-3
	if RightCount<0:
		RightCount=0
	ScrollBar = Window.GetControl (16)
	ScrollBar.SetVarAssoc ("RightTopIndex", RightCount)
	RightTopIndex = GemRB.GetVar ("RightTopIndex")
	if RightTopIndex>RightCount:
		GemRB.SetVar ("RightTopIndex", RightCount)

	RedrawStoreShoppingWindow ()
	return

def SelectBuy ():
	Window = StoreShoppingWindow

	pc = GemRB.GameGetSelectedPCSingle ()
	LeftIndex = GemRB.GetVar ("LeftIndex")
	GemRB.ChangeStoreItem (pc, LeftIndex, SHOP_BUY|SHOP_SELECT)
	RedrawStoreShoppingWindow ()
	return

def ToBackpackPressed ():
	Window = StoreShoppingWindow

	pc = GemRB.GameGetSelectedPCSingle ()
	LeftCount = Store['StoreItemCount']
	#going backwards because removed items shift the slots
	for i in range (LeftCount, 0, -1):
		Flags = GemRB.IsValidStoreItem (pc, i-1, ITEM_STORE)&SHOP_SELECT
		if Flags:
			GemRB.ChangeStoreItem (pc, i-1, SHOP_BUY)

	UpdateStoreShoppingWindow ()
	return

def BuyPressed ():
	Window = StoreShoppingWindow

	if (BuySum>GemRB.GameGetPartyGold ()):
		ErrorWindow (50081)
		return

	pc = GemRB.GameGetSelectedPCSingle ()
	LeftCount = Store['StoreItemCount']
	#going backwards because removed items shift the slots
	for i in range (LeftCount, 0, -1):
		Flags = GemRB.IsValidStoreItem (pc, i-1, ITEM_STORE)&SHOP_SELECT
		if Flags:
			Slot = GemRB.GetStoreItem (i-1)
			Item = GemRB.GetItem (Slot['ItemResRef'])
			Price = Item['Price'] * Store['SellMarkup'] / 100
			if Price <= 0:
				Price = 1

			if GemRB.ChangeStoreItem (pc, i-1, SHOP_BUY):
				GemRB.GameSetPartyGold (GemRB.GameGetPartyGold ()-Price)
	UpdateStoreShoppingWindow ()
	return

def SelectSell ():
	Window = StoreShoppingWindow

	pc = GemRB.GameGetSelectedPCSingle ()
	RightIndex = GemRB.GetVar ("RightIndex")
	GemRB.ChangeStoreItem (pc, inventory_slots[RightIndex], SHOP_SELL|SHOP_SELECT)
	RedrawStoreShoppingWindow ()
	return

def ToBagPressed ():
	Window = StoreShoppingWindow

	pc = GemRB.GameGetSelectedPCSingle ()
	RightCount = len (inventory_slots)
	#no need to go reverse
	for Slot in range (RightCount):
		Flags = GemRB.IsValidStoreItem (pc, inventory_slots[Slot], ITEM_PC)
		if Flags & SHOP_SELECT:
			GemRB.ChangeStoreItem (pc, inventory_slots[Slot], SHOP_SELL)
	UpdateStoreShoppingWindow ()
	return

def SellPressed ():
	Window = StoreShoppingWindow

	pc = GemRB.GameGetSelectedPCSingle ()
	RightCount = len (inventory_slots)
	#no need to go reverse
	for Slot in range (RightCount):
		Flags = GemRB.IsValidStoreItem (pc, inventory_slots[Slot], ITEM_PC) & SHOP_SELECT
		if Flags:
			GemRB.ChangeStoreItem (pc, inventory_slots[Slot], SHOP_SELL)

	GemRB.GameSetPartyGold (GemRB.GameGetPartyGold ()+SellSum)
	UpdateStoreShoppingWindow ()
	return

def RedrawStoreShoppingWindow ():
	global BuySum, SellSum

	Window = StoreShoppingWindow

	UpdateStoreCommon (Window, 0x10000001, 0x10000005, 0x10000002)
	pc = GemRB.GameGetSelectedPCSingle ()

	LeftTopIndex = GemRB.GetVar ("LeftTopIndex")
	LeftIndex = GemRB.GetVar ("LeftIndex")
	RightTopIndex = GemRB.GetVar ("RightTopIndex")
	RightIndex = GemRB.GetVar ("RightIndex")
	LeftCount = Store['StoreItemCount']
	BuySum = 0
	for i in range (LeftCount):
		if GemRB.IsValidStoreItem (pc, i, ITEM_STORE) & SHOP_SELECT:
			Slot = GemRB.GetStoreItem (i)
			Item = GemRB.GetItem (Slot['ItemResRef'])
			if Inventory:
				Price = 1
			else:
				Price = Item['Price'] * Store['SellMarkup'] / 100
			if Price <= 0:
				Price = 1
			BuySum = BuySum + Price

	RightCount = len(inventory_slots)
	SellSum = 0
	for i in range (RightCount):
		Flags = GemRB.IsValidStoreItem (pc, inventory_slots[i], ITEM_PC)
		if Flags & SHOP_SELECT:
			Slot = GemRB.GetSlotItem (pc, inventory_slots[i])
			Item = GemRB.GetItem (Slot['ItemResRef'])
			if Inventory:
				Price = 1
			else:
				Price = Item['Price'] * Store['BuyMarkup'] / 100
			if Flags & SHOP_ID:
				Price = 1
			SellSum = SellSum + Price

	Label = Window.GetControl (0x10000003)
	if Inventory:
		Label.SetText ("")
	else:
		Label.SetText (str(BuySum) )
	if BuySum:
		LeftButton.SetState (IE_GUI_BUTTON_ENABLED)
	else:
		LeftButton.SetState (IE_GUI_BUTTON_DISABLED)

	Label = Window.GetControl (0x10000004)
	if Inventory:
		Label.SetText ("")
	else:
		Label.SetText (str(SellSum) )
	if SellSum:
		RightButton.SetState (IE_GUI_BUTTON_ENABLED)
	else:
		RightButton.SetState (IE_GUI_BUTTON_DISABLED)

	for i in range (4):
		if i+LeftTopIndex<LeftCount:
			Slot = GemRB.GetStoreItem (i+LeftTopIndex)
		else:
			Slot = None
		Button = Window.GetControl (i+8)
		Label = Window.GetControl (0x1000000b+i)
		Button.SetVarAssoc ("LeftIndex", LeftTopIndex+i)
		if Slot != None:
			Flags = GemRB.IsValidStoreItem (pc, i+LeftTopIndex, ITEM_STORE)
			Item = GemRB.GetItem (Slot['ItemResRef'])
			Button.SetItemIcon (Slot['ItemResRef'], 0)
			Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_NAND)
			Button.SetFlags (IE_GUI_BUTTON_PICTURE, OP_OR)
			if Flags & SHOP_BUY:
				if Flags & SHOP_SELECT:
					Button.SetState (IE_GUI_BUTTON_SELECTED)
				else:
					Button.SetState (IE_GUI_BUTTON_ENABLED)
			else:
				Button.SetState (IE_GUI_BUTTON_DISABLED)

			if Flags & SHOP_ID:
				GemRB.SetToken ("ITEMNAME", GemRB.GetString (Item['ItemName']))
				Button.EnableBorder (0, 1)
			else:
				GemRB.SetToken ("ITEMNAME", GemRB.GetString (Item['ItemNameIdentified']))
				Button.EnableBorder (0, 0)

			if Inventory:
				Label.SetText (28337)
			else:
				Price = Item['Price'] * Store['SellMarkup'] / 100
				if Price <= 0:
					Price = 1
				GemRB.SetToken ("ITEMCOST", str(Price) )
				Label.SetText (45374)
		else:
			Button.SetState (IE_GUI_BUTTON_DISABLED)
			Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_OR)
			Button.SetFlags (IE_GUI_BUTTON_PICTURE, OP_NAND)
			Label.SetText ("")

		if i+RightTopIndex<RightCount:
			Slot = GemRB.GetSlotItem (pc, inventory_slots[i+RightTopIndex])
		else:
			Slot = None
		Button = Window.GetControl (i+17)
		Label = Window.GetControl (0x10000014+i)
		Button.SetVarAssoc ("RightIndex", RightTopIndex+i)
		if Slot != None:
			Flags = GemRB.IsValidStoreItem (pc, inventory_slots[i+RightTopIndex], ITEM_PC)
			Item = GemRB.GetItem (Slot['ItemResRef'])
			Button.SetItemIcon (Slot['ItemResRef'], 0)
			Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_NAND)
			Button.SetFlags (IE_GUI_BUTTON_PICTURE, OP_OR)
			if Inventory:
				Price = 1
			else:
				Price = Item['Price'] * Store['BuyMarkup'] / 100

			if (Price>0) and (Flags & SHOP_SELL):
				if Flags & SHOP_SELECT:
					Button.SetState (IE_GUI_BUTTON_SELECTED)
				else:
					Button.SetState (IE_GUI_BUTTON_ENABLED)
			else:
				Button.SetState (IE_GUI_BUTTON_DISABLED)

			if Flags & SHOP_ID:
				GemRB.SetToken ("ITEMNAME", GemRB.GetString (Item['ItemName']))
				Price = 1
				Button.EnableBorder (0, 1)
			else:
				GemRB.SetToken ("ITEMNAME", GemRB.GetString (Item['ItemNameIdentified']))
				Button.EnableBorder (0, 0)

			if Inventory:
				Label.SetText (28337)
			else:
				GemRB.SetToken ("ITEMCOST", str(Price) )
				Label.SetText (45374)
		else:
			Button.SetState (IE_GUI_BUTTON_DISABLED)
			Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_OR)
			Button.SetFlags (IE_GUI_BUTTON_PICTURE, OP_NAND)
			Label.SetText ("")
	return

def UpdateStoreIdentifyWindow ():
	global inventory_slots

	Window = StoreIdentifyWindow

	pc = GemRB.GameGetSelectedPCSingle ()
	inventory_slots = GemRB.GetSlots (pc, SLOT_INVENTORY)
	Count = len(inventory_slots)
	ScrollBar = Window.GetControl (5)
	ScrollBar.SetVarAssoc ("TopIndex", Count-3)
	GemRB.SetVar ("Index", -1)
	RedrawStoreIdentifyWindow ()
	return

def RedrawStoreIdentifyWindow ():
	Window = StoreIdentifyWindow

	UpdateStoreCommon (Window, 0x0fffffff, 0x10000002, 0x10000000)
	TopIndex = GemRB.GetVar ("TopIndex")
	Index = GemRB.GetVar ("Index")
	pc = GemRB.GameGetSelectedPCSingle ()
	Count = len(inventory_slots)
	IDPrice = Store['IDPrice']

	TextArea = Window.GetControl (14)
	TextArea.SetText ("")
	Selected = 0
	for i in range (4):
		if TopIndex+i<Count:
			Slot = GemRB.GetSlotItem (pc, inventory_slots[TopIndex+i])
		else:
			Slot = None
		Button = Window.GetControl (i+6)
		Label = Window.GetControl (0x10000009+i)
		Button.SetVarAssoc ("Index", TopIndex+i)
		if Slot != None:
			Flags = GemRB.IsValidStoreItem (pc, inventory_slots[TopIndex+i], ITEM_PC)
			Item = GemRB.GetItem (Slot['ItemResRef'])
			Button.SetItemIcon (Slot['ItemResRef'], 0)
			Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_NAND)
			Button.SetFlags (IE_GUI_BUTTON_PICTURE, OP_OR)
			if Flags & SHOP_ID:
				if Index == TopIndex+i:
					Button.SetState (IE_GUI_BUTTON_SELECTED)
					Text = Item['ItemDesc']
					TextArea.SetText (Text)
					Selected = 1
				else:
					Button.SetState (IE_GUI_BUTTON_ENABLED)

				GemRB.SetToken ("ITEMNAME", GemRB.GetString (Item['ItemName']))
				GemRB.SetToken ("ITEMCOST", str(IDPrice) )
				Button.EnableBorder (0, 1)
			else:
				if Index == TopIndex+i:
					Text = Item['ItemDescIdentified']
					TextArea.SetText (Text)
				Button.SetState (IE_GUI_BUTTON_DISABLED)
				GemRB.SetToken ("ITEMNAME", GemRB.GetString (Item['ItemNameIdentified']))
				GemRB.SetToken ("ITEMCOST", str(0) )
				Button.EnableBorder (0, 0)

			Label.SetText (45374)
		else:
			Button.SetState (IE_GUI_BUTTON_DISABLED)
			Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_OR)
			Button.SetFlags (IE_GUI_BUTTON_PICTURE, OP_NAND)
			Label.SetText ("")

	Button = Window.GetControl (4)
	Label = Window.GetControl (0x10000001)
	if Selected:
		Button.SetState (IE_GUI_BUTTON_ENABLED)
		Label.SetText (str(IDPrice) )
	else:
		Button.SetState (IE_GUI_BUTTON_DISABLED)
		Label.SetText (str(0) )
	return

def IdentifyPressed ():
	IDPrice = Store['IDPrice']
	if (GemRB.GameGetPartyGold ()<IDPrice):
		return

	Index = GemRB.GetVar ("Index")
	if (Index<0):
		return

	pc = GemRB.GameGetSelectedPCSingle ()
	Count = len(inventory_slots)
	if Index >= Count:
		return

	GemRB.ChangeStoreItem (pc, inventory_slots[Index], SHOP_ID)
	GemRB.GameSetPartyGold (GemRB.GameGetPartyGold ()-IDPrice)
	UpdateStoreIdentifyWindow ()
	return

def InfoIdentifyWindow ():
	Index = GemRB.GetVar ("Index")
	pc = GemRB.GameGetSelectedPCSingle ()
	Count = len(inventory_slots)
	if Index >= Count:
		return
	Slot = GemRB.GetSlotItem (pc, inventory_slots[Index])
	Item = GemRB.GetItem (Slot['ItemResRef'])
	InfoWindow (Slot, Item)
	return

def InfoLeftWindow ():
	Index = GemRB.GetVar ("LeftIndex")
	Slot = GemRB.GetStoreItem (Index)
	Item = GemRB.GetItem (Slot['ItemResRef'])
	InfoWindow (Slot, Item)
	return

def InfoRightWindow ():
	Index = GemRB.GetVar ("RightIndex")
	pc = GemRB.GameGetSelectedPCSingle ()
	Count = len(inventory_slots)
	if Index >= Count:
		return
	Slot = GemRB.GetSlotItem (pc, inventory_slots[Index])
	Item = GemRB.GetItem (Slot['ItemResRef'])
	InfoWindow (Slot, Item)
	return

def InfoWindow (Slot, Item):
	global MessageWindow

	Identify = Slot['Flags'] & IE_INV_ITEM_IDENTIFIED

	MessageWindow = Window = GemRB.LoadWindow (13)

	#fake label
	Label = Window.GetControl (0x10000000)
	Label.SetText ("")

	#description bam
	Button = Window.GetControl (6)
	Button.SetState (IE_GUI_BUTTON_LOCKED)
	Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_OR)
	Button.SetItemIcon (Slot['ItemResRef'], 2)

	#slot bam
	Button = Window.GetControl (2)
	Button.SetState (IE_GUI_BUTTON_LOCKED)
	Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_OR)
	Button.SetItemIcon (Slot['ItemResRef'], 0)

	Label = Window.GetControl (0x0fffffff)
	TextArea = Window.GetControl (4)
	if Identify:
		Label.SetText (Item['ItemNameIdentified'])
		TextArea.SetText (Item['ItemDescIdentified'])
	else:
		Label.SetText (Item['ItemName'])
		TextArea.SetText (Item['ItemDesc'])

	#Done
	Button = Window.GetControl (3)
	Button.SetText (1403)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, ErrorDone)

	# hide the empty button
	#Window.DeleteControl (9)

	Window.ShowModal (MODAL_SHADOW_GRAY)
	return

def UpdateStoreStealWindow ():
	global Store, inventory_slots

	Window = StoreStealWindow

	#reget store in case of a change
	Store = GemRB.GetStore ()
	LeftCount = Store['StoreItemCount']
	ScrollBar = Window.GetControl (4)
	ScrollBar.SetVarAssoc ("LeftTopIndex", LeftCount-3)

	pc = GemRB.GameGetSelectedPCSingle ()
	inventory_slots = GemRB.GetSlots (pc, SLOT_INVENTORY)
	RightCount = len(inventory_slots)
	ScrollBar = Window.GetControl (13)
	ScrollBar.SetVarAssoc ("RightTopIndex", RightCount-3)
	GemRB.SetVar ("LeftIndex", -1)
	LeftButton.SetState (IE_GUI_BUTTON_DISABLED)
	RedrawStoreStealWindow ()
	return

def StealPressed ():
	Window = StoreShoppingWindow

	LeftIndex = GemRB.GetVar ("LeftIndex")
	pc = GemRB.GameGetSelectedPCSingle ()
	#percentage skill check, if fails, trigger StealFailed
	#if difficulty = 0 and skill=100, automatic success
	#if difficulty = 0 and skill=50, 50% success
	#if difficulty = 50 and skill=50, 0% success
	#if skill>random(100)+difficulty - success
	if GUICommon.CheckStat100 (pc, IE_PICKPOCKET, Store['StealFailure']):
		GemRB.ChangeStoreItem (pc, LeftIndex, SHOP_STEAL)
		UpdateStoreStealWindow ()
	else:
		GemRB.StealFailed ()
		CloseStoreWindow ()
	return

def RedrawStoreStealWindow ():
	Window = StoreStealWindow

	UpdateStoreCommon (Window, 0x10000000, 0x10000002, 0x10000001)
	LeftTopIndex = GemRB.GetVar ("LeftTopIndex")
	LeftIndex = GemRB.GetVar ("LeftIndex")
	RightTopIndex = GemRB.GetVar ("RightTopIndex")
	RightIndex = GemRB.GetVar ("RightIndex")
	LeftCount = Store['StoreItemCount']
	pc = GemRB.GameGetSelectedPCSingle ()
	RightCount = len(inventory_slots)
	for i in range (4):
		Slot = GemRB.GetStoreItem (i+LeftTopIndex)
		Button = Window.GetControl (i+5)
		Label = Window.GetControl (0x10000008+i)
		Button.SetVarAssoc ("LeftIndex", LeftTopIndex+i)
		if Slot != None:
			Flags = GemRB.IsValidStoreItem (pc, i+LeftTopIndex, ITEM_STORE)
			Item = GemRB.GetItem (Slot['ItemResRef'])
			Button.SetItemIcon (Slot['ItemResRef'], 0)
			Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_NAND)
			Button.SetFlags (IE_GUI_BUTTON_PICTURE, OP_OR)
			if Flags & SHOP_STEAL:
				if LeftIndex == LeftTopIndex + i:
					Button.SetState (IE_GUI_BUTTON_SELECTED)
				else:
					Button.SetState (IE_GUI_BUTTON_ENABLED)
			else:
				Button.SetState (IE_GUI_BUTTON_DISABLED)

			if Flags & SHOP_ID:
				GemRB.SetToken ("ITEMNAME", GemRB.GetString (Item['ItemName']))
				Button.EnableBorder (0, 1)
			else:
				GemRB.SetToken ("ITEMNAME", GemRB.GetString (Item['ItemNameIdentified']))
				Button.EnableBorder (0, 0)

			GemRB.SetToken ("ITEMCOST", str(Slot['Price']) )
			Label.SetText (45374)
		else:
			Button.SetState (IE_GUI_BUTTON_DISABLED)
			Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_OR)
			Button.SetFlags (IE_GUI_BUTTON_PICTURE, OP_NAND)
			Label.SetText ("")

		if i+RightTopIndex<RightCount:
			Slot = GemRB.GetSlotItem (pc, inventory_slots[i+RightTopIndex])
		else:
			Slot = None
		Button = Window.GetControl (i+14)
		Label = Window.GetControl (0x10000011+i)
		Button.SetVarAssoc ("RightIndex", RightTopIndex+i)
		if Slot != None:
			Flags = GemRB.IsValidStoreItem (pc, inventory_slots[i+RightTopIndex], ITEM_PC)
			Item = GemRB.GetItem (Slot['ItemResRef'])
			Button.SetItemIcon (Slot['ItemResRef'], 0)
			Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_NAND)
			Button.SetFlags (IE_GUI_BUTTON_PICTURE, OP_OR)
			Price = Item['Price'] * Store['BuyMarkup'] / 100
			Button.SetState (IE_GUI_BUTTON_ENABLED)
			if Flags & SHOP_ID:
				GemRB.SetToken ("ITEMNAME", GemRB.GetString (Item['ItemName']))
				Button.EnableBorder (0, 1)
			else:
				GemRB.SetToken ("ITEMNAME", GemRB.GetString (Item['ItemNameIdentified']))
				Button.EnableBorder (0, 0)

			GemRB.SetToken ("ITEMCOST", str(Price) )
			Label.SetText (45374)
		else:
			Button.SetState (IE_GUI_BUTTON_DISABLED)
			Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_OR)
			Button.SetFlags (IE_GUI_BUTTON_PICTURE, OP_NAND)
			Label.SetText ("")
	if LeftIndex>=0:
		LeftButton.SetState (IE_GUI_BUTTON_ENABLED)
	else:
		LeftButton.SetState (IE_GUI_BUTTON_DISABLED)
	return

def UpdateStoreDonateWindow ():
	Window = StoreDonateWindow

	UpdateStoreCommon (Window, 0x10000005, 0, 0x10000006)
	Field = Window.GetControl (3)
	donation = int("0"+Field.QueryText ())
	gold = GemRB.GameGetPartyGold ()
	if donation>gold:
		donation = gold
		Field.SetText (str(gold) )

	Button = Window.GetControl (2)
	if donation:
		Button.SetState (IE_GUI_BUTTON_ENABLED)
	else:
		Button.SetState (IE_GUI_BUTTON_DISABLED)
	return

def IncrementDonation ():
	Window = StoreDonateWindow

	Field = Window.GetControl (3)
	donation = int("0"+Field.QueryText ())
	if donation<GemRB.GameGetPartyGold ():
		Field.SetText (str(donation+1) )
	else:
		Field.SetText (str(GemRB.GameGetPartyGold ()) )
	UpdateStoreDonateWindow ()
	return

def DecrementDonation ():
	Window = StoreDonateWindow

	Field = Window.GetControl (3)
	donation = int("0"+Field.QueryText ())
	if donation>0:
		Field.SetText (str(donation-1) )
	else:
		Field.SetText (str(0) )
	UpdateStoreDonateWindow ()
	return

def DonateGold ():
	Window = StoreDonateWindow

	TextArea = Window.GetControl (0)
	TextArea.SetFlags (IE_GUI_TEXTAREA_AUTOSCROLL)

	#Button = Window.GetControl (10)
	#Button.SetAnimation ("DONATE")

	Field = Window.GetControl (3)
	donation = int("0"+Field.QueryText ())
	GemRB.GameSetPartyGold (GemRB.GameGetPartyGold ()-donation)
	if GemRB.IncreaseReputation (donation):
		TextArea.Append (26914, -1)
		GemRB.PlaySound ("act_03")
		UpdateStoreDonateWindow ()
		return

	TextArea.Append (26914, -1)
	GemRB.PlaySound ("act_03e")
	UpdateStoreDonateWindow ()
	return

def UpdateStoreHealWindow ():
	Window = StoreHealWindow

	UpdateStoreCommon (Window, 0x0fffffff, 0x1000000e, 0x10000000)
	TopIndex = GemRB.GetVar ("TopIndex")
	Index = GemRB.GetVar ("Index")
	for i in range (4):
		Cure = GemRB.GetStoreCure (TopIndex+i)

		Button = Window.GetControl (i+5)
		Label = Window.GetControl (0x10000008+i)
		Button.SetVarAssoc ("Index", TopIndex+i)
		if Cure != None:
			Spell = GemRB.GetSpell (Cure['CureResRef'])
			Button.SetSpellIcon (Cure['CureResRef'], 1)
			Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_NAND)
			Button.SetFlags (IE_GUI_BUTTON_PICTURE, OP_OR)

			GemRB.SetToken ("ITEMNAME", GemRB.GetString (Spell['SpellName']))
			GemRB.SetToken ("ITEMCOST", str(Cure['Price']) )
			Label.SetText (45374)

		else:
			Button.SetState (IE_GUI_BUTTON_DISABLED)
			Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_OR)
			Button.SetFlags (IE_GUI_BUTTON_PICTURE, OP_NAND)
			Label.SetText ("")
		if TopIndex+i==Index:
			TextArea = Window.GetControl (13)
			TextArea.SetText (Cure['Description'])
			Label = Window.GetControl (0x10000001)
			Label.SetText (str(Cure['Price']) )
			Button = Window.GetControl (3)
			Button.SetState (IE_GUI_BUTTON_ENABLED)
	return

def InfoHealWindow ():
	global MessageWindow

	UpdateStoreHealWindow ()
	Index = GemRB.GetVar ("Index")
	Cure = GemRB.GetStoreCure (Index)
	Spell = GemRB.GetSpell (Cure['CureResRef'])

	MessageWindow = Window = GemRB.LoadWindow (13)

	Label = Window.GetControl (0x10000000)
	Label.SetText (Spell['SpellName'])

	Button = Window.GetControl (2)
	Button.SetSpellIcon (Cure['CureResRef'], 1)
	Button = Window.GetControl (6)
	Button.SetSpellIcon (Cure['CureResRef'], 2)

	TextArea = Window.GetControl (5)
	TextArea.SetText (Spell['SpellDesc'])

	#Done
	Button = Window.GetControl (3)
	Button.SetText (1403)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, ErrorDone)

	Window.ShowModal (MODAL_SHADOW_GRAY)
	return

def PlayCureSoundEffect (spell):
	GemRB.PlaySound (CureTable.GetValue(spell, "SOUND_EFFECT") )
	return

def BuyHeal ():
	Index = GemRB.GetVar ("Index")
	Cure = GemRB.GetStoreCure (Index)
	gold = GemRB.GameGetPartyGold ()
	if gold < Cure['Price']:
		ErrorWindow (50082)
		return

	GemRB.GameSetPartyGold (gold-Cure['Price'])
	pc = GemRB.GameGetSelectedPCSingle ()
	GemRB.ApplySpell (pc, Cure['CureResRef'])
	PlayCureSoundEffect (Cure['CureResRef'])
	UpdateStoreHealWindow ()
	return

def UpdateStoreRumourWindow ():
	Window = StoreRumourWindow

	UpdateStoreCommon (Window, 0x1000000a, 0, 0x1000000b)
	TopIndex = GemRB.GetVar ("TopIndex")
	for i in range (5):
		Drink = GemRB.GetStoreDrink (TopIndex+i)
		Button = Window.GetControl (i)
		Button.SetVarAssoc ("Index", i)
		if Drink != None:
			GemRB.SetToken ("ITEMNAME", GemRB.GetString (Drink['DrinkName']))
			GemRB.SetToken ("ITEMCOST", str(Drink['Price']) )
			Button.SetText (45374)
			Button.SetState (IE_GUI_BUTTON_ENABLED)
			Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, GulpDrink)
		else:
			Button.SetText ("")
			Button.SetState (IE_GUI_BUTTON_DISABLED)
	return

def GulpDrink ():
	Window = StoreRumourWindow

	TextArea = Window.GetControl (13)
	TextArea.SetFlags (IE_GUI_TEXTAREA_AUTOSCROLL)
	pc = GemRB.GameGetSelectedPCSingle ()
	intox = GemRB.GetPlayerStat (pc, IE_INTOXICATION)
	intox = 0
	if intox > 80:
		TextArea.Append (0, -1)
		return

	gold = GemRB.GameGetPartyGold ()
	Index = GemRB.GetVar ("TopIndex")+GemRB.GetVar ("Index")
	Drink = GemRB.GetStoreDrink (Index)
	if gold < Drink['Price']:
		ErrorWindow (50083)
		return

	GemRB.GameSetPartyGold (gold-Drink['Price'])
	GemRB.SetPlayerStat (pc, IE_INTOXICATION, intox+Drink['Strength'])
	text = GemRB.GetRumour (Drink['Strength'], Store['TavernRumour'])
	TextArea.Append (text, -1)
	GemRB.PlaySound ("gam_07")
	UpdateStoreRumourWindow ()
	return

def UpdateStoreRentWindow ():
	global RentIndex

	Window = StoreRentWindow
	UpdateStoreCommon (Window, 0x1000000a, 0, 0x1000000b)
	RentIndex = GemRB.GetVar ("RentIndex")
	Button = Window.GetControl (8)
	Label = Window.GetControl (0x1000000c)
	if RentIndex>=0:
		TextArea = Window.GetControl (9)
		TextArea.SetText (roomtypes[RentIndex] )
		price = Store['StoreRoomPrices'][RentIndex]
		Label.SetText (str(price) )
		Button.SetState (IE_GUI_BUTTON_ENABLED)
	else:
		Label.SetText ("0" )
		Button.SetState (IE_GUI_BUTTON_DISABLED)
	return

def RentConfirm ():
	RentIndex = GemRB.GetVar ("RentIndex")
	price = Store['StoreRoomPrices'][RentIndex]
	Gold = GemRB.GameGetPartyGold ()
	GemRB.GameSetPartyGold (Gold-price)
	GemRB.RestParty (13, 1, (RentIndex+1)/2)
	if RentConfirmWindow:
		RentConfirmWindow.Unload ()
	Window = StoreRentWindow
	TextArea = Window.GetControl (9)
	#is there any way to change this???
	GemRB.SetToken ("HOUR", "8")
	GemRB.SetToken ("DURATION", "hours")
	GemRB.SetToken ("HP", "%d"%(RentIndex+1))
	TextArea.SetText (19262)
	GemRB.SetVar ("RentIndex", -1)
	Button = Window.GetControl (RentIndex+4)
	Button.SetState (IE_GUI_BUTTON_ENABLED)
	UpdateStoreRentWindow ()
	return

def RentDeny () :
	if RentConfirmWindow:
		RentConfirmWindow.Unload ()
	UpdateStoreRentWindow ()
	return

def RentRoom ():
	global RentIndex, RentConfirmWindow

	RentIndex = GemRB.GetVar ("RentIndex")
	price = Store['StoreRoomPrices'][RentIndex]
	Gold = GemRB.GameGetPartyGold ()
	if Gold<price:
		ErrorWindow (50085)
		return

	RentConfirmWindow = Window = GemRB.LoadWindow (12)
	#confirm
	Button = Window.GetControl (0)
	Button.SetText (4242)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, RentConfirm)
	Button.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)

	#deny
	Button = Window.GetControl (1)
	Button.SetText (4196)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, RentDeny)
	Button.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)

	#textarea
	TextArea = Window.GetControl (3)
	TextArea.SetText (4241)

	Window.ShowModal (MODAL_SHADOW_GRAY)
	return

def ErrorWindow (strref):
	global MessageWindow

	MessageWindow = Window = GemRB.LoadWindow (11)

	TextArea = Window.GetControl (3)
	TextArea.SetText (strref)

	#done
	Button = Window.GetControl (0)
	Button.SetText (1403)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, ErrorDone)

	Window.ShowModal (MODAL_SHADOW_GRAY)
	return

def ErrorDone ():
	if MessageWindow:
		MessageWindow.Unload ()
	return

###################################################
# End of file GUISTORE.py
