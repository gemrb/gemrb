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
# $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/GUIScripts/iwd2/GUISTORE.py,v 1.2 2005/03/03 22:33:11 avenger_teambg Exp $


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
# 3 - heal
# 4 - donate
# 5 - drink
# 6 - rent

storebams = ("STORSTOR","STORTVRN","STORINN","STORTMPL","STORBAG","STORBAG")
roomtypes=(17389,17517,17521,17519)

store_funcs = ( "OpenStoreShoppingWindow", "OpenStoreIdentifyWindow",
"OpenStoreStealWindow", "OpenStoreHealWindow", "OpenStoreDonateWindow", 
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
		Buttons[i]=Button=GemRB.GetControl (Window, i+1)
		Action = store_buttons[i]
		GemRB.SetButtonSprites(Window, Button, "GUISTBBC", Action, 0,1,2,0)
		GemRB.SetVarAssoc (Window, Button, "Action", i)
		GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_RADIOBUTTON, OP_OR)
		if Action>=0:
			GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, store_funcs[Action])
			GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_ENABLED)
		else:
			GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "")
			GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_DISABLED)

	GemRB.UnhideGUI()
	store_update_funcs[store_buttons[0]] ()

def OpenStoreShoppingWindow ():
	global StoreShoppingWindow

	GemRB.HideGUI()
	if StoreShoppingWindow != None:
		Window = StoreShoppingWindow
		GemRB.SetVar ("TopWindow", Window)
		UpdateStoreShoppingWindow ()
		GemRB.UnhideGUI()
		return

	StoreShoppingWindow = Window = GemRB.LoadWindow (2)
	GemRB.SetVar ("TopWindow", Window)

	# buy price ...
	Label = GemRB.GetControl (Window, 0x1000002b)
	GemRB.SetText (Window, Label, "0")

	# sell price ...
	Label = GemRB.GetControl (Window, 0x1000002c)
	GemRB.SetText (Window, Label, "0")

	# Buy
	Button = GemRB.GetControl (Window, 2)
	GemRB.SetText (Window, Button, 13703)

	# Sell
	Button = GemRB.GetControl (Window, 3)
	GemRB.SetText (Window, Button, 13704)

	Button = GemRB.GetControl (Window, 50)
	GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_DISABLED)
	GemRB.SetText (Window, Button, 13707)
	# 45374

	# 7 scrollbar
	# 8-11, @11 -@14 - slots and their labels
	# 16 scrollbar
	# 17-20, @20-@23 - slots and labels
	# 25 encumbrance button

	# encumbrance
	Label = GemRB.CreateLabel (Window, 0x10000043, 15,325,60,15,"NUMBER","0:",IE_FONT_ALIGN_LEFT|IE_FONT_ALIGN_TOP)
	Label = GemRB.CreateLabel (Window, 0x10000044, 15,365,80,15,"NUMBER","0:",IE_FONT_ALIGN_RIGHT|IE_FONT_ALIGN_TOP)

	SetSelectionChangeHandler( UpdateStoreShoppingWindow )
	UpdateStoreShoppingWindow ()
	GemRB.UnhideGUI()


def OpenStoreIdentifyWindow ():
	global StoreIdentifyWindow
	
	GemRB.HideGUI()
	if StoreIdentifyWindow != None:
		Window = StoreIdentifyWindow
		GemRB.SetVar ("TopWindow", Window)
		UpdateStoreIdentifyWindow ()
		GemRB.UnhideGUI()
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
	UpdateStoreIdentifyWindow ()
	GemRB.UnhideGUI()


def OpenStoreStealWindow ():
	global StoreStealWindow
	
	GemRB.HideGUI()
	if StoreStealWindow != None:
		Window = StoreStealWindow
		GemRB.SetVar ("TopWindow", Window)
		UpdateStoreStealWindow ()
		GemRB.UnhideGUI()
		return
	
	StoreStealWindow = Window = GemRB.LoadWindow (6)
	GemRB.SetVar ("TopWindow", Window)

	# Steal
	Button = GemRB.GetControl (Window, 1)
	GemRB.SetText (Window, Button, 14179)

	# encumbrance
	Label = GemRB.CreateLabel (Window, 0x10000043, 15,325,60,15,"NUMBER","0:",IE_FONT_ALIGN_LEFT|IE_FONT_ALIGN_TOP)
	Label = GemRB.CreateLabel (Window, 0x10000044, 15,365,80,15,"NUMBER","0:",IE_FONT_ALIGN_RIGHT|IE_FONT_ALIGN_TOP)

	SetSelectionChangeHandler( UpdateStoreStealWindow )
	UpdateStoreStealWindow ()
	GemRB.UnhideGUI()


def OpenStoreDonateWindow ():
	global StoreDonateWindow
	
	GemRB.HideGUI()
	if StoreDonateWindow != None:
		Window = StoreDonateWindow
		GemRB.SetVar ("TopWindow", Window)
		UpdateStoreDonateWindow ()
		GemRB.UnhideGUI()
		return
	
	StoreDonateWindow = Window = GemRB.LoadWindow (9)
	GemRB.SetVar ("TopWindow", Window)

	# Donate
	Button = GemRB.GetControl (Window, 3)
	GemRB.SetText (Window, Button, 15101)

	# 0 ta
	# 5 donation entry
	# 6 7 +-
	Button = GemRB.GetControl (Window, 6)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "IncrementDonation")
	Button = GemRB.GetControl (Window, 7)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "DecrementDonation")	

	SetSelectionChangeHandler( UpdateStoreDonateWindow )
	UpdateStoreDonateWindow ()
	GemRB.UnhideGUI()


def OpenStoreHealWindow ():
	global StoreHealWindow
	
	GemRB.HideGUI()
	if StoreHealWindow != None:
		Window = StoreHealWindow
		GemRB.SetVar ("TopWindow", Window)
		UpdateStoreHealWindow ()
		GemRB.UnhideGUI()
		return
	
	StoreHealWindow = Window = GemRB.LoadWindow (5)
	GemRB.SetVar ("TopWindow", Window)

	#spell buttons
	for i in range(4):
		Button = GemRB.GetControl (Window, i+8)
		GemRB.SetVarAssoc (Window, Button, "Index", i)
		GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_RADIOBUTTON, OP_OR)

	# Heal
	Button = GemRB.GetControl (Window, 5)
	GemRB.SetText (Window, Button, 13703) 

	# 23 ta

	UpdateStoreHealWindow ()
	GemRB.UnhideGUI()


def OpenStoreRumourWindow ():
	global StoreRumourWindow
	
	GemRB.HideGUI()
	GemRB.SetVar ("TopIndex", 0)
	if StoreRumourWindow != None:
		Window = StoreRumourWindow
		GemRB.SetVar ("TopWindow", Window)
		UpdateStoreRumourWindow ()
		GemRB.UnhideGUI()
		return
	
	StoreRumourWindow = Window = GemRB.LoadWindow (8)
	GemRB.SetVar ("TopWindow", Window)
	
	#removing those pesky labels
	for i in range(5):
		GemRB.DeleteControl (Window, 0x10000005+i)

	ScrollBar = GemRB.GetControl (Window, 5)
	GemRB.SetEvent (Window, ScrollBar, IE_GUI_SCROLLBAR_ON_CHANGE, "UpdateStoreRumourWindow")
	Count=Store['StoreDrinkCount']
	if Count<5:
		Count=1
	else:
		Count=Count-4
	GemRB.SetVarAssoc(Window, ScrollBar, "TopIndex", Count)

	UpdateStoreRumourWindow ()
	GemRB.UnhideGUI()


def OpenStoreRentWindow ():
	global StoreRentWindow

	GemRB.HideGUI()
	if StoreRentWindow != None:
		Window = StoreRentWindow
		GemRB.SetVar ("TopWindow", Window)
		UpdateStoreRentWindow ()
		GemRB.UnhideGUI()
		return
	
	StoreRentWindow = Window = GemRB.LoadWindow (7)
	GemRB.SetVar ("TopWindow", Window)

	# room types
	Index = -1
	for i in range(4):
		ok = Store['StoreRoomPrices'][i]
		Button = GemRB.GetControl (Window, i)
		GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "SelectStoreRent")
		#GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_RADIOBUTTON, OP_OR)
		GemRB.SetVarAssoc (Window, Button, "Index", i)
		if ok<0:
			GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_DISABLED)
		else:
			if Index==-1:
				Index = i

		Button = GemRB.GetControl (Window, i+4)
		GemRB.SetText (Window, Button, 14294+i)
		GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "SelectStoreRent")
		GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_RADIOBUTTON, OP_OR)
		GemRB.SetVarAssoc (Window, Button, "Index", i)
		if ok<0:
			GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_DISABLED)

	# Rent
	Button = GemRB.GetControl (Window, 11)
	GemRB.SetText (Window, Button, 14293)
	GemRB.SetVar ("Index",Index)

	UpdateStoreRentWindow()
	GemRB.UnhideGUI()


def SelectStoreRent ():
	Window = StoreRentWindow
	
	Index = GemRB.GetVar ("Index")
	Text = GemRB.GetControl (Window, 12)
	GemRB.SetText (Window, Text, roomtypes[Index] )
	Label = GemRB.GetControl (Window, 0x1000000d)
	Rent = GemRB.GetVar ("StoreRent")
	price = Store['StoreRoomPrices'][Rent]
	GemRB.SetText (Window, Label, str(price) )

def UpdateStoreCommon (Window, title, name, gold):

	Label = GemRB.GetControl (Window, title)
	GemRB.SetText (Window, Label, Store['StoreName'])

	if name:
		pc = GemRB.GameGetSelectedPCSingle ()
		Label = GemRB.GetControl (Window, name)
		GemRB.SetText (Window, Label, GemRB.GetPlayerName (pc, 0) )

	Label = GemRB.GetControl (Window, gold)
	GemRB.SetText (Window, Label, str( GemRB.GameGetPartyGold () ) )
	

def UpdateStoreShoppingWindow ():
	UpdateStoreCommon (StoreShoppingWindow, 0x10000003, 0x1000002e, 0x1000002a)

def UpdateStoreIdentifyWindow ():
	UpdateStoreCommon (StoreIdentifyWindow, 0x10000000, 0x10000005, 0x10000002)

def UpdateStoreStealWindow ():
	UpdateStoreCommon (StoreStealWindow, 0x10000002, 0x10000027, 0x10000023)

def UpdateStoreDonateWindow ():
	UpdateStoreCommon (StoreDonateWindow, 0x10000007, 0, 0x10000008)
	
def IncrementDonation ():
	Window = StoreDonateWindow

	Field = GetControl (Window, 5)
	donation = val(GemRB.QueryText (Window, Field))
	if donation<GemRB.GetPartyGold():
		GemRB.SetText (Window, Field, str(donation+1) )
	else:
		GemRB.SetText (Window, Field, str(GemRB.GetPartyGold()) )
		
def DecrementDonation ():
	Window = StoreDonateWindow

	Field = GetControl (Window, 5)
	donation = val(GemRB.QueryText(Window, Field))
	if donation>0:
		GemRB.SetText (Window, Field, str(donation-1) )
	else:
		GemRB.SetText (Window, Field, str(0) )

def DonateGold ():
	Window = StoreDonateWindow
	
	TextArea = GemRB.GetControl (Window, 0)	
	GemRB.SetTAAutoScroll(Window, TextArea, 1)
	Button = GemRB.GetControl (Window, 10)
	GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_ANIMATE)
	Field = GetControl (Window, 5)
	donation = val(GemRB.QueryText(Window, Field))
	GemRB.SetPartyGold( GemRB.GetPartyGold()-donation)
	donation = donation / 1000
	if donation>0:
		#GemRB.IncreaseReputation (donation)
		GemRB.TextAreaAppend (Window, TextArea, 10468, -1)
		GemRB.PlaySound("act_03")
	else:
		GemRB.TextAreaAppend (Window, TextArea, 10469, -1)
		GemRB.PlaySound("act_03e")
	
	
def UpdateStoreHealWindow ():
	UpdateStoreCommon (StoreHealWindow, 0x10000000, 0, 0x10000001)
	
def UpdateStoreRumourWindow ():
	Window = StoreRumourWindow
	UpdateStoreCommon (Window, 0x10000011, 0, 0x10000012)
	TopIndex=GemRB.GetVar ("TopIndex")
	for i in range(5):
		Drink = GemRB.GetStoreDrink (i+TopIndex)

		Button = GemRB.GetControl (Window, i)
		if Drink != None:
			txt = "%s (%d)" % (GemRB.GetString(Drink['DrinkName']) , Drink['Price'])
			GemRB.SetText (Window, Button, txt)
			GemRB.SetVarAssoc (Window, Button, "Index", i)
			GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_ENABLED)
			GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "GulpDrink")
		else:
			GemRB.SetText (Window, Button, "")
			GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_DISABLED)

def GulpDrink ():
	Window = StoreRumourWindow

	TextArea = GemRB.GetControl (Window, 13)
	GemRB.SetTAAutoScroll(Window, TextArea, 1)
	pc = GemRB.GameGetSelectedPCSingle ()
	intox = GemRB.GetPlayerStat (pc, IE_INTOXICATION)
	if intox > 80:
		GemRB.TextAreaAppend (Window, TextArea, 10832, -1)
		return

	gold = GemRB.GameGetPartyGold ()
	Index = GemRB.GetVar ("TopIndex")+GemRB.GetVar ("Index")
	Drink = GemRB.GetStoreDrink (Index)
	if gold < Drink['Price']:
		print "No money"
		return

	GemRB.SetPlayerStat (pc, IE_INTOXICATION, intox+Drink['Strength'])
	#get some rumour
	GemRB.PlaySound("gam_07")


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
