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
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
#
# $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/GUIScripts/tob/GUISTORE.py,v 1.8 2005/02/28 22:49:48 avenger_teambg Exp $


# GUISTORE.py - script to open store/inn/temple windows from GUISTORE winpack

###################################################

import string
import GemRB
from GUIDefines import *
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

pc = None
character_name = None

Store = None
Buttons = [-1,-1,-1,-1]

# 0 - Store
# 1 - Tavern
# 2 - Inn
# 3 - Temple
# 4 - Container
# 5 - Container

# 0 - buy/sell
# 1 - identify 
# 2 - steal
# 3 - donate
# 4 - heal
# 5 - drink
# 6 - rent

storebams = ("STORSTOR","STORTVRN","STORINN","STORTMPL","STORBAG","STORBAG")
roomtypes=(17389,17517,17521,17519)

store_funcs = ( "OpenStoreShoppingWindow", "OpenStoreIdentifyWindow",
"OpenStoreStealWindow", "OpenStoreDonateWindow", "OpenStoreHealWindow",
"OpenStoreRumourWindow", "OpenStoreRentWindow" )
store_update_funcs = None

def OpenStoreWindow ():
	global Store
	global StoreWindow
	global store_update_funcs

	#these are function pointers, not strings
	#can't put this in global init, doh!
	store_update_funcs = (OpenStoreShoppingWindow,
	OpenStoreIdentifyWindow,OpenStoreStealWindow,
	OpenStoreDonateWindow,OpenStoreHealWindow,
	OpenStoreRumourWindow,OpenStoreRentWindow )

	if CloseOtherWindow( StoreWindow ):
		GemRB.HideGUI ()
		CloseStoreShoppingWindow ()
		CloseStoreIdentifyWindow ()
		CloseStoreStealWindow ()
		CloseStoreDonateWindow ()
		CloseStoreHealWindow ()
		CloseStoreRumourWindow ()
		CloseStoreRentWindow ()

		GemRB.UnloadWindow (StoreWindow)
		StoreWindow = None
		GemRB.SetVar ("OtherWindow", -1)
		SetSelectionChangeHandler( None )
		GemRB.UnhideGUI ()
		return

	GemRB.HideGUI ()
	GemRB.LoadWindowPack ("GUISTORE")
	StoreWindow = Window = GemRB.LoadWindow (3)
	GemRB.SetVar ("OtherWindow", StoreWindow) #this is the store button row

	Store = GemRB.GetStore()

	# Done
	Button = GemRB.GetControl (Window, 0)
	GemRB.SetText (Window, Button, 1403)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenStoreWindow")

	#Store type icon
	Button = GemRB.GetControl (Window, 5)
	print storebams[Store['StoreType']]
	GemRB.SetButtonSprites(Window, Button,storebams[Store['StoreType']],0,0,0,0,0)

	#based on shop type, these buttons will change
	store_type = Store['StoreType']
	store_buttons = Store['StoreButtons']
	for i in range(4):
		Buttons[i]=Button=GemRB.GetControl (Window, i+1)
		Action = store_buttons[i]
		GemRB.SetButtonSprites(Window, Button, "GUISTBBC", Action, 0,1,0,0)
		GemRB.SetVarAssoc (Window, Button, "Action", i)
		GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_RADIOBUTTON, OP_OR)
		if Action>=0:
			GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, store_funcs[Action])
			GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_ENABLED)
		else:
			GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "")
			GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_DISABLED)

	SetSelectionChangeHandler( store_update_funcs[store_buttons[0]] )
	GemRB.SetVar ("StoreRent",0)
	GemRB.UnhideGUI()
	#initializing selected variables
	store_update_funcs[store_buttons[0]] ()

def OpenStoreShoppingWindow ():
	global StoreShoppingWindow

	if StoreShoppingWindow != None:
		Window = StoreShoppingWindow
		GemRB.SetVar ("TopWindow", Window)
		UpdateStoreShoppingWindow ()
		return

	StoreShoppingWindow = Window = GemRB.LoadWindow (2)
	GemRB.SetVar ("TopWindow", Window)

	# buy price ...
	Label = GemRB.GetControl (Window, 0x1000002b)
	GemRB.SetText (Window, Label, 'cost')

	# sell price ...
	Label = GemRB.GetControl (Window, 0x1000002c)
	GemRB.SetText (Window, Label, 'sell')

	# Buy
	Button = GemRB.GetControl (Window, 2)
	GemRB.SetText (Window, Button, 45303)

	# Sell
	Button = GemRB.GetControl (Window, 3)
	GemRB.SetText (Window, Button, 45304)

	# 45374

	# 7 scrollbar
	# 8-11, @11 -@14 - slots and their labels
	# 16 scrollbar
	# 17-20, @20-@23 - slots and labels
	# 25 encumbrance button

	SetSelectionChangeHandler( UpdateStoreShoppingWindow )
	UpdateStoreShoppingWindow ()


def OpenStoreIdentifyWindow ():
	global StoreIdentifyWindow
	
	GemRB.HideGUI ()

	if StoreIdentifyWindow != None:
		Window = StoreIdentifyWindow
		GemRB.SetVar ("TopWindow", Window)
		GemRB.UnhideGUI ()
		UpdateStoreIdentifyWindow ()
		return
	
	StoreIdentifyWindow = Window = GemRB.LoadWindow (4)
	GemRB.SetVar ("TopWindow", Window)

	# Identify
	Button = GemRB.GetControl (Window, 5)
	GemRB.SetText (Window, Button, 44971)

	# 23 ta

	# price ...
	Label = GemRB.GetControl (Window, 0x10000003)
	GemRB.SetText (Window, Label, str (666))

	# 6-9 item slots, 0x10000009-c labels

	SetSelectionChangeHandler( UpdateStoreIdentifyWindow )
	GemRB.UnhideGUI ()
	UpdateStoreIdentifyWindow ()


def OpenStoreStealWindow ():
	global StoreStealWindow
	
	GemRB.HideGUI ()

	if StoreStealWindow != None:
		Window = StoreStealWindow
		GemRB.SetVar ("TopWindow", Window)
		GemRB.UnhideGUI ()
		UpdateStoreStealWindow ()
		return
	
	StoreStealWindow = Window = GemRB.LoadWindow (6)
	GemRB.SetVar ("TopWindow", Window)

	# Steal
	Button = GemRB.GetControl (Window, 1)
	GemRB.SetText (Window, Button, 45305)

	SetSelectionChangeHandler( UpdateStoreStealWindow )
	GemRB.UnhideGUI ()
	UpdateStoreStealWindow ()


def OpenStoreDonateWindow ():
	global StoreDonateWindow
	
	GemRB.HideGUI ()

	if StoreDonateWindow != None:
		Window = StoreDonateWindow
		GemRB.SetVar ("TopWindow", Window)
		GemRB.UnhideGUI ()
		UpdateStoreDonateWindow ()
		return
	
	StoreDonateWindow = Window = GemRB.LoadWindow (9)
	GemRB.SetVar ("TopWindow", Window)

	# Donate
	Button = GemRB.GetControl (Window, 2)
	GemRB.SetText (Window, Button, 45307)

	# 0 ta
	# 3 donation entry
	# 4 5 +-

	SetSelectionChangeHandler( UpdateStoreDonateWindow )
	GemRB.UnhideGUI ()
	UpdateStoreDonateWindow ()


def OpenStoreHealWindow ():
	global StoreHealWindow
	
	GemRB.HideGUI ()

	if StoreHealWindow != None:
		Window = StoreHealWindow
		GemRB.SetVar ("TopWindow", Window)
		GemRB.UnhideGUI ()
		UpdateStoreHealWindow ()
		return
	
	StoreHealWindow = Window = GemRB.LoadWindow (5)
	GemRB.SetVar ("TopWindow", Window)

	# price ...
	Label = GemRB.GetControl (Window, 0x10000001)
	GemRB.SetText (Window, Label, str (666))

	# price ...
	Label = GemRB.GetControl (Window, 0x1000000e)
	GemRB.SetText (Window, Label, character_name)

	# Heal
	Button = GemRB.GetControl (Window, 3)
	GemRB.SetText (Window, Button, 8836) # FIXME: better strref


	# 13 ta

	GemRB.UnhideGUI ()
	UpdateStoreHealWindow ()


def OpenStoreRumourWindow ():
	global StoreRumourWindow
	
	GemRB.HideGUI ()

	if StoreRumourWindow != None:
		Window = StoreRumourWindow
		GemRB.SetVar ("TopWindow", Window)
		GemRB.UnhideGUI ()
		UpdateStoreRumourWindow ()
		return
	
	StoreRumourWindow = Window = GemRB.LoadWindow (8)
	GemRB.SetVar ("TopWindow", Window)
	
	Scrollbar = GemRB.GetControl (Window, 15)
	GemRB.SetEvent(Window, ScrollBar, IE_GUI_SCROLLBAR_ON_CHANGE, "RumourScrollBarPress")
	GemRB.SetVar("TopIndex", 0)
        Count=Store['RumourCount']
        GemRB.SetVarAssoc(Window, ScrollBar, "TopIndex", Count)
	RumourScrollBarPress()

	GemRB.UnhideGUI ()
	UpdateStoreRumourWindow ()


def OpenStoreRentWindow ():
	global StoreRentWindow
	
	GemRB.HideGUI ()

	if StoreRentWindow != None:
		Window = StoreRentWindow
		GemRB.SetVar ("TopWindow", Window)
		GemRB.UnhideGUI ()
		UpdateStoreRentWindow ()
		return
	
	StoreRentWindow = Window = GemRB.LoadWindow (7)
	GemRB.SetVar ("TopWindow", Window)

	# room types
	StoreRent = -1
	for i in range(4):
		ok = Store['StoreRoomPrices'][i]
		Button = GemRB.GetControl (Window, i)
		GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "SelectStoreRent")
		GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_RADIOBUTTON, OP_OR)
		GemRB.SetVarAssoc (Window, Button, "StoreRent", i)
		if ok<0:
			GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_DISABLED)
		else:
			if StoreRent==-1:
				StoreRent = i

		Button = GemRB.GetControl (Window, i+4)
		GemRB.SetText (Window, Button, 14294+i)
		GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "SelectStoreRent")
		GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_RADIOBUTTON, OP_OR)
		GemRB.SetVarAssoc (Window, Button, "StoreRent", i)
		if ok<0:
			GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_DISABLED)

	# Rent
	Button = GemRB.GetControl (Window, 11)
	GemRB.SetText (Window, Button, 14293)
	GemRB.SetVar ("StoreRent",StoreRent)
	GemRB.UnhideGUI ()


def SelectStoreRent ():
	Window = StoreRentWindow
	
	room = GemRB.GetVar ("StoreRent")
	Text = GemRB.GetControl (Window, 12)
	GemRB.SetText (Window, Text, roomtypes[room] )
	Label = GemRB.GetControl (Window, 0x1000000d)
	Rent = GemRB.GetVar ("StoreRent")
	price = Store['StoreRoomPrices'][Rent]
	GemRB.SetText (Window, Label, str(price) )

def UpdateStoreCommon (Window, title, name, gold):
	GemRB.HideGUI()

	Label = GemRB.GetControl (Window, title)
	GemRB.SetText (Window, Label, Store['StoreName'])

	if name:
		pc = GemRB.GameGetSelectedPCSingle ()
		Label = GemRB.GetControl (Window, name)
		GemRB.SetText (Window, Label, GemRB.GetPlayerName (pc, 0) )

	Label = GemRB.GetControl (Window, gold)
	GemRB.SetText (Window, Label, str( GemRB.GameGetPartyGold () ) )
	GemRB.UnhideGUI()
	

def UpdateStoreShoppingWindow ():
	UpdateStoreCommon (StoreShoppingWindow, 0x10000003, 0x1000002e, 0x1000002a)

def UpdateStoreIdentifyWindow ():
	UpdateStoreCommon (StoreIdentifyWindow, 0x10000000, 0x10000005, 0x10000002)

def UpdateStoreStealWindow ():
	UpdateStoreCommon (StoreStealWindow, 0x10000002, 0x10000027, 0x10000023)

def UpdateStoreDonateWindow ():
	UpdateStoreCommon (StoreDonateWindow, 0x10000007, 0, 0x10000008)

def UpdateStoreHealWindow ():
	UpdateStoreCommon (StoreHealWindow, 0x10000005, 0, 0x10000001)
	
def UpdateStoreRumourWindow ():
	Window = StoreRumourWindow
	UpdateStoreCommon (Window, 0x10000011, 0, 0x10000012)
	TopIndex=GemRB.GetVar ("TopIndex")
	for i in range(5):
		Drink = GemRB.GetStoreDrink (i+TopIndex)
		Button = GemRB.GetControl (Window, i+4)
		GemRB.SetText (Window, Button, Drink['DrinkName'])
		GemRB.SetVarAssoc (Window, Button, "Intox", Drink['Strength'])
		GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "GulpDrink")

def GulpDrink ():
	PlaySound("gulp")
	print "Intox improvement: ", GemRB.GetVar("Intox")


def UpdateStoreRentWindow ():
	UpdateStoreCommon (StoreRentWindow, 0x10000008, 0, 0x10000009)
	# price ...
	SelectStoreRent ()


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

###################################################
# End of file GUISTORE.py
