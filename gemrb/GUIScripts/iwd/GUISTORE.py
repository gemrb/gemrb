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
# $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/GUIScripts/iwd/GUISTORE.py,v 1.8 2005/03/18 20:01:17 avenger_teambg Exp $


# GUISTORE.py - script to open store/inn/temple windows from GUISTORE winpack

###################################################

import string
import GemRB
from GUIDefines import *
from ie_stats import *
from GUICommonWindows import SetSelectionChangeHandler
from GUICommon import CloseOtherWindow

StoreWindow = None
StoreShoppingWindow = None
StoreIdentifyWindow = None
StoreStealWindow = None
StoreDonateWindow = None
StoreHealWindow = None
StoreRumourWindow = None
StoreRentWindow = None

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

def OpenStoreWindow ():
	global Store
	global StoreWindow
	global store_update_funcs, inventory_slots

	#these are function pointers, not strings
	#can't put this in global init, doh!
	store_update_funcs = (OpenStoreShoppingWindow,
	OpenStoreIdentifyWindow,OpenStoreStealWindow,
	OpenStoreHealWindow, OpenStoreDonateWindow,
	OpenStoreRumourWindow,OpenStoreRentWindow )

	if CloseOtherWindow( OpenStoreWindow ):
		GemRB.HideGUI ()
		CloseStoreShoppingWindow ()
		CloseStoreIdentifyWindow ()
		CloseStoreStealWindow ()
		CloseStoreHealWindow ()
		CloseStoreDonateWindow ()
		CloseStoreRumourWindow ()
		CloseStoreRentWindow ()

		GemRB.UnloadWindow (StoreWindow)
		StoreWindow = None
		GemRB.SetVar ("OtherWindow", -1)
		GemRB.SetVar ("TopWindow", -1)
		SetSelectionChangeHandler( None )
		GemRB.UnhideGUI ()
		GemRB.LeaveStore ()
		return

	GemRB.HideGUI ()
	GemRB.SetVar ("Action", 0)
	GemRB.LoadWindowPack ("GUISTORE")
	StoreWindow = Window = GemRB.LoadWindow (3)
	GemRB.SetVar ("OtherWindow", StoreWindow) #this is the store button row

	Store = GemRB.GetStore()
	# Done
	Button = GemRB.GetControl (Window, 0)
	GemRB.SetText (Window, Button, 11973)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenStoreWindow")

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

	GemRB.UnhideGUI ()
	store_update_funcs[store_buttons[0]] ()

def OpenStoreShoppingWindow ():
	global StoreShoppingWindow

	GemRB.HideGUI ()
	if StoreShoppingWindow != None:
		Window = StoreShoppingWindow
		GemRB.SetVar ("TopWindow", Window)
		UpdateStoreShoppingWindow ()
		GemRB.UnhideGUI ()
		return

	StoreShoppingWindow = Window = GemRB.LoadWindow (2)
	GemRB.SetVar ("TopWindow", Window)

	# buy price ...
	Label = GemRB.GetControl (Window, 0x1000002b)
	GemRB.SetText (Window, Label, "0")

	# sell price ...
	Label = GemRB.GetControl (Window, 0x1000002c)
	GemRB.SetText (Window, Label, "0")

	j = 1
	for i in range(4):
		Button = GemRB.GetControl (Window, i+5)
		GemRB.SetVarAssoc (Window, Button, "LeftIndex", j)
		GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_CHECKBOX, OP_OR)

		Button = GemRB.GetControl (Window, i+13)
		GemRB.SetVarAssoc (Window, Button, "RightIndex", j)
		GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_CHECKBOX, OP_OR)
		j <<= 1

	# Buy
	Button = GemRB.GetControl (Window, 2)
	GemRB.SetText (Window, Button, 13703)

	# Sell
	Button = GemRB.GetControl (Window, 3)
	GemRB.SetText (Window, Button, 13704)

	Button = GemRB.GetControl (Window, 50)
	GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_LOCKED)
	GemRB.SetText (Window, Button, 13707)


	Button = GemRB.GetControl (Window, 44)
	GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_DISABLED)

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
	GemRB.UnhideGUI ()


def OpenStoreIdentifyWindow ():
	global StoreIdentifyWindow
	
	GemRB.HideGUI ()
	GemRB.SetVar ("Index", -1)
	GemRB.SetVar ("TopIndex", 0)
	if StoreIdentifyWindow != None:
		Window = StoreIdentifyWindow
		GemRB.SetVar ("TopWindow", Window)
		UpdateStoreIdentifyWindow ()
		GemRB.UnhideGUI ()
		return
	
	StoreIdentifyWindow = Window = GemRB.LoadWindow (4)
	GemRB.SetVar ("TopWindow", Window)

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
		GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "RedrawStoreIdentifyWindow")

	ScrollBar = GemRB.GetControl (Window, 7)
	GemRB.SetEvent (Window, ScrollBar, IE_GUI_SCROLLBAR_ON_CHANGE, "RedrawStoreIdentifyWindow")
	
	SetSelectionChangeHandler( UpdateStoreIdentifyWindow )
	UpdateStoreIdentifyWindow ()
	GemRB.UnhideGUI ()


def OpenStoreStealWindow ():
	global StoreStealWindow
	
	GemRB.HideGUI ()
	if StoreStealWindow != None:
		Window = StoreStealWindow
		GemRB.SetVar ("TopWindow", Window)
		UpdateStoreStealWindow ()
		GemRB.UnhideGUI ()
		return
	
	StoreStealWindow = Window = GemRB.LoadWindow (6)
	GemRB.SetVar ("TopWindow", Window)

	j = 1
	for i in range(4):
		Button = GemRB.GetControl (Window, i+4)
		GemRB.SetVarAssoc (Window, Button, "LeftIndex", j)
		GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_CHECKBOX, OP_OR)
		GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "RedrawStoreStealWindow")

		Button = GemRB.GetControl (Window, i+11)
		GemRB.SetVarAssoc (Window, Button, "RightIndex", j)
		GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_CHECKBOX, OP_OR)
		GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_RIGHT_PRESS, "StealInfoWindow")
		j <<= 1

	# Steal
	Button = GemRB.GetControl (Window, 1)
	GemRB.SetText (Window, Button, 14179)

	Button = GemRB.GetControl (Window, 37)
	GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_DISABLED)

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
	GemRB.UnhideGUI ()


def OpenStoreDonateWindow ():
	global StoreDonateWindow
	
	GemRB.HideGUI ()
	if StoreDonateWindow != None:
		Window = StoreDonateWindow
		GemRB.SetVar ("TopWindow", Window)
		UpdateStoreDonateWindow ()
		GemRB.UnhideGUI ()
		return
	
	StoreDonateWindow = Window = GemRB.LoadWindow (9)
	GemRB.SetVar ("TopWindow", Window)

	# graphics
	Button = GemRB.GetControl (Window, 10)
	GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_ANIMATED|IE_GUI_BUTTON_PLAYONCE, OP_OR)
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
	GemRB.UnhideGUI ()


def OpenStoreHealWindow ():
	global StoreHealWindow
	
	GemRB.HideGUI ()
	GemRB.SetVar ("Index", -1)
	GemRB.SetVar ("TopIndex", 0)
	if StoreHealWindow != None:
		Window = StoreHealWindow
		GemRB.SetVar ("TopWindow", Window)
		UpdateStoreHealWindow ()
		GemRB.UnhideGUI ()
		return
	
	StoreHealWindow = Window = GemRB.LoadWindow (5)
	GemRB.SetVar ("TopWindow", Window)

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
	GemRB.UnhideGUI ()


def OpenStoreRumourWindow ():
	global StoreRumourWindow
	
	GemRB.HideGUI ()
	GemRB.SetVar ("TopIndex", 0)
	if StoreRumourWindow != None:
		Window = StoreRumourWindow
		GemRB.SetVar ("TopWindow", Window)
		UpdateStoreRumourWindow ()
		GemRB.UnhideGUI ()
		return
	
	StoreRumourWindow = Window = GemRB.LoadWindow (8)
	GemRB.SetVar ("TopWindow", Window)
	
	#removing those pesky labels
	for i in range(5):
		GemRB.DeleteControl (Window, 0x10000005+i)

	TextArea = GemRB.GetControl (Window, 11)
	GemRB.SetText (Window, TextArea, 14144)

	BAM = "TVRNQUL%d"% ((Store['StoreFlags']>>9)&3)
	Button = GemRB.GetControl (Window, 12)
	GemRB.SetButtonSprites (Window, Button, BAM, 0, 0, 0, 0, 0)

	ScrollBar = GemRB.GetControl (Window, 5)
	GemRB.SetEvent (Window, ScrollBar, IE_GUI_SCROLLBAR_ON_CHANGE, "UpdateStoreRumourWindow")
	Count = Store['StoreDrinkCount']
	if Count>4:
		Count = Count-4
	else:
		Count = 0
	GemRB.SetVarAssoc (Window, ScrollBar, "TopIndex", Count+1)

	UpdateStoreRumourWindow ()
	GemRB.UnhideGUI ()


def OpenStoreRentWindow ():
	global StoreRentWindow, RentIndex

	GemRB.HideGUI ()
	if StoreRentWindow != None:
		Window = StoreRentWindow
		GemRB.SetVar ("RentIndex", RentIndex)
		GemRB.SetVar ("TopWindow", Window)
		UpdateStoreRentWindow ()
		GemRB.UnhideGUI ()
		return
	
	StoreRentWindow = Window = GemRB.LoadWindow (7)
	GemRB.SetVar ("TopWindow", Window)

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
	GemRB.UnhideGUI ()


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


def RedrawStoreShoppingWindow ():
	Window = StoreShoppingWindow

	UpdateStoreCommon (Window, 0x10000003, 0x1000002e, 0x1000002a)
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
		Button = GemRB.GetControl (Window, i+5)
		Label = GemRB.GetControl (Window, 0x10000012+i)
		if Slot != None:
			Flags = GemRB.IsValidStoreItem (pc, i+LeftTopIndex, 1)
			Item = GemRB.GetItem (Slot['ItemResRef'])
			GemRB.SetVarAssoc (Window, Button, "LeftIndex", LeftTopIndex+i)
			GemRB.SetItemIcon (Window,Button, Slot['ItemResRef'],0)
			GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_NO_IMAGE, OP_NAND)
			GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_PICTURE, OP_OR)
			if Flags & 1:
				if i==LeftIndex:
					GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_SELECTED)
				else:
					GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_ENABLED)
			else:
				GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_DISABLED)

			GemRB.SetToken ("ITEMNAME", GemRB.GetString (Item['ItemName']))
			GemRB.SetToken ("ITEMCOST", str(Slot['Price']) )
			GemRB.SetText (Window, Label, 10162)
		else:
			GemRB.SetVarAssoc (Window, Button, "LeftIndex", -1)
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
			Flags = GemRB.IsValidStoreItem (pc, inventory_slots[i+RightTopIndex], 0)
			Item = GemRB.GetItem (Slot['ItemResRef'])
			GemRB.SetVarAssoc (Window, Button, "RightIndex", RightTopIndex+i)
			GemRB.SetItemIcon (Window,Button, Slot['ItemResRef'],0)
			GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_NO_IMAGE, OP_NAND)
			GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_PICTURE, OP_OR)
			Price = Item['Price'] * Store['BuyMarkup'] / 100;
			if Flags & 2:
				if i==RightIndex:
					GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_SELECTED)
				else:
					GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_ENABLED)
			else:
				GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_DISABLED)

			GemRB.SetToken ("ITEMNAME", GemRB.GetString (Item['ItemName']))
			GemRB.SetToken ("ITEMCOST", str(Price) )
			GemRB.SetText (Window, Label, 10162)
		else:
			GemRB.SetVarAssoc (Window, Button, "RightIndex", -1)
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
			Flags = GemRB.IsValidStoreItem (pc, inventory_slots[i+TopIndex], 0)
			Item = GemRB.GetItem (Slot['ItemResRef'])
			GemRB.SetVarAssoc (Window, Button, "Index", TopIndex+i)
			GemRB.SetItemIcon (Window,Button, Slot['ItemResRef'],0)
			GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_NO_IMAGE, OP_NAND)
			GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_PICTURE, OP_OR)
			if Flags & 4:
				if i==Index:
		        		Label = GemRB.GetControl (Window, 0x10000003)
				        GemRB.SetText (Window, Label, str(IDPrice))
					GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_SELECTED)
				else:
					GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_ENABLED)
			else:
				GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_DISABLED)

			GemRB.SetToken ("ITEMNAME", GemRB.GetString (Item['ItemName']))
			GemRB.SetToken ("ITEMCOST", str(IDPrice) )
			GemRB.SetText (Window, Label, 10162)
		else:
			GemRB.SetVarAssoc (Window, Button, "Index", -1)
			GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_DISABLED)
			GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_NO_IMAGE, OP_OR)
			GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_PICTURE, OP_NAND)
			GemRB.SetText (Window, Label, "")


def InfoIdentifyWindow ():
	UpdateStoreIdentifyWindow ()
	Index = GemRB.GetVar ("Index")
	pc = GemRB.GameGetSelectedPCSingle ()
	inventory_slots = GemRB.GetSlots (pc, -1)
	Slot = GemRB.GetSlotItem (inventory_slots[Index])
	Item = GemRB.GetItem (Slot['ItemResRef'])
	#set the identify flag to 1
	#GemRB.SetSlotItem (inventory_slots[Index], {"Flags":4})
	#deduce gold from player

	Window = GemRB.LoadWindow (12)

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

	GemRB.HideGUI ()
	GemRB.SetVar ("FloatWindow", Window)
	GemRB.UnhideGUI ()
	GemRB.ShowModal (Window, MODAL_SHADOW_GRAY)


def UpdateStoreStealWindow ():
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
			Flags = GemRB.IsValidStoreItem (pc, i+LeftTopIndex, 1)
			Item = GemRB.GetItem (Slot['ItemResRef'])
			GemRB.SetVarAssoc (Window, Button, "LeftIndex", LeftTopIndex+i)
			GemRB.SetItemIcon (Window,Button, Slot['ItemResRef'],0)
			GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_NO_IMAGE, OP_NAND)
			GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_PICTURE, OP_OR)
			if Flags & 8:
				if i==LeftIndex:
					GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_SELECTED)
				else:
					GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_ENABLED)
			else:
				GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_DISABLED)

			GemRB.SetToken ("ITEMNAME", GemRB.GetString (Item['ItemName']))
			GemRB.SetToken ("ITEMCOST", str(Slot['Price']) )
			GemRB.SetText (Window, Label, 10162)
		else:
			GemRB.SetVarAssoc (Window, Button, "LeftIndex", -1)
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
			Flags = GemRB.IsValidStoreItem (pc, inventory_slots[i+RightTopIndex], 0)
			Item = GemRB.GetItem (Slot['ItemResRef'])
			GemRB.SetVarAssoc (Window, Button, "RightIndex", RightTopIndex+i)
			GemRB.SetItemIcon (Window,Button, Slot['ItemResRef'],0)
			GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_NO_IMAGE, OP_NAND)
			GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_PICTURE, OP_OR)
			Price = Item['Price'] * Store['BuyMarkup'] / 100;
			GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_DISABLED)
			GemRB.SetToken ("ITEMNAME", GemRB.GetString (Item['ItemName']))
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
	GemRB.SetTAAutoScroll (Window, TextArea, 1)

	Button = GemRB.GetControl (Window, 10)
	#GemRB.SetButtonAnimation (Window, Button, 0, 0)

	Field = GemRB.GetControl (Window, 5)
	donation = int("0"+GemRB.QueryText (Window, Field))
	GemRB.GameSetPartyGold (GemRB.GameGetPartyGold ()-donation)
	#reputation = GemRB.GameGetReputation ()
	#if reputation<180:
	if GemRB.IncreaseReputation( donation ):
		#limit = donationlimit[reputation/10]
		#if donation>=limit:
		#GemRB.GameSetReputation (reputation+10)
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
	UpdateStoreHealWindow ()
	Index = GemRB.GetVar ("Index")
	Cure = GemRB.GetStoreCure (Index)
	Spell = GemRB.GetSpell (Cure['CureResRef'])

	Window = GemRB.LoadWindow (14)

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

	GemRB.HideGUI ()
	GemRB.SetVar ("FloatWindow", Window)
	GemRB.UnhideGUI ()
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
	#GemRB.ApplySpell (pc, Cure['CureResRef'])
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
	GemRB.SetTAAutoScroll (Window, TextArea, 1)
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
		#GemRB.TextAreaAppend (Window, TextArea, 11049, -1)
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
		#GemRB.SetText (Window, TextArea, 11051)
		return
	GemRB.GameSetPartyGold (Gold-price)
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
	Window = GemRB.LoadWindow (10)

	TextArea = GemRB.GetControl (Window, 3)
	GemRB.SetText (Window, TextArea, strref)

	#done
	Button = GemRB.GetControl (Window, 0)
	GemRB.SetText (Window, Button, 11973)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "ErrorDone")

	GemRB.HideGUI ()
	GemRB.SetVar ("FloatWindow", Window)
	GemRB.UnhideGUI ()
	GemRB.ShowModal (Window, MODAL_SHADOW_GRAY)


def ErrorDone ():
	Window = GemRB.GetVar ("FloatWindow")
	GemRB.HideGUI ()
	GemRB.UnloadWindow (Window)
	GemRB.SetVar ("FloatWindow", -1)
	GemRB.UnhideGUI ()


###################################################
# End of file GUISTORE.py
