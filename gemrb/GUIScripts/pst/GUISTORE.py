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
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
#
# $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/GUIScripts/pst/GUISTORE.py,v 1.8 2005/05/13 17:07:50 avenger_teambg Exp $


# GUISTORE.py - script to open store/inn/temple windows from GUISTORE winpack

###################################################

import string
import GemRB
from GUIDefines import *
#from GUICommonWindows import OpenCommonWindows, CloseCommonWindows
from GUICommonWindows import SetSelectionChangeHandler
#import GUICommonWindows

StoreWindow = None

StoreShoppingWindow = None
StoreIdentifyWindow = None
StoreStealWindow = None
StoreDonateWindow = None
StoreHealWindow = None
StoreRumourWindow = None
StoreRentWindow = None

HelpStoreRent = None

pc = 0
character_name = ""
party_gold = 0
store_buttons = {}
store_update_functions = {}

def OpenStoreWindow ():
	global StoreWindow, party_gold, Store
	
	GemRB.HideGUI ()
	
	if StoreWindow != None:
		CloseStoreShoppingWindow ()
		CloseStoreIdentifyWindow ()

		GemRB.UnloadWindow (StoreWindow)
		StoreWindow = None
		GemRB.SetVar ("OtherWindow", -1)
		SetSelectionChangeHandler (None)
		
		GemRB.UnhideGUI ()
		return

	Store = GemRB.GetStore ()
	
	GemRB.LoadWindowPack ("GUISTORE")
	StoreWindow = Window = GemRB.LoadWindow (3)
        #GemRB.SetVar ("OtherWindow", StoreWindow)
	GemRB.SetVisible (Window, 1)


	# Done
	Button = GemRB.GetControl (Window, 0)
	GemRB.SetText (Window, Button, 1403)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenStoreWindow")

	# buy / sell
	Button = GemRB.GetControl (Window, 1)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenStoreShoppingWindow")
	store_buttons['shopping'] = Button
	store_update_functions['shopping'] = UpdateStoreShoppingWindow

	# identify
	Button = GemRB.GetControl (Window, 2)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenStoreIdentifyWindow")
	store_buttons['identify'] = Button
	store_update_functions['identify'] = UpdateStoreIdentifyWindow

	# steal
	Button = GemRB.GetControl (Window, 3)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenStoreStealWindow")
	store_buttons['steal'] = Button
	store_update_functions['steal'] = UpdateStoreStealWindow

	# donate
	Button = GemRB.GetControl (Window, 4)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenStoreDonateWindow")
	store_buttons['donate'] = Button
	store_update_functions['donate'] = UpdateStoreDonateWindow

	# heal / cure
	Button = GemRB.GetControl (Window, 5)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenStoreHealWindow")
	store_buttons['heal'] = Button
	store_update_functions['heal'] = UpdateStoreHealWindow

	# rumour
	Button = GemRB.GetControl (Window, 6)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenStoreRumourWindow")
	store_buttons['rumour'] = Button
	store_update_functions['rumour'] = UpdateStoreRumourWindow

	# rent room
	Button = GemRB.GetControl (Window, 7)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenStoreRentWindow")
	store_buttons['rent'] = Button
	store_update_functions['rent'] = UpdateStoreRentWindow

	OpenStoreShoppingWindow ()
	
	GemRB.UnhideGUI()

last_store_action = None
def SelectStoreAction (action):
	global last_store_action

	Window = StoreWindow

	if last_store_action != None and action != last_store_action:
		Button = store_buttons[last_store_action]
		GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_UNPRESSED)
		
	Button = store_buttons[action]
	GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_SELECTED)

	SetSelectionChangeHandler (store_update_functions[action])
	
	last_store_action = action



def OpenStoreShoppingWindow ():
	global StoreShoppingWindow
	
	GemRB.HideGUI ()
	SelectStoreAction ('shopping')

	if StoreShoppingWindow != None:
		Window = StoreShoppingWindow
		GemRB.SetVar ("OtherWindow", Window)
		UpdateStoreShoppingWindow ()
		GemRB.UnhideGUI ()
		return

	StoreShoppingWindow = Window = GemRB.LoadWindow (4)
	GemRB.SetVar ("OtherWindow", Window)

	# title ...
	Label = GemRB.GetControl (Window, 0x10000001)
	GemRB.SetText (Window, Label, Store['StoreName'])

	# buy price ...
	Label = GemRB.GetControl (Window, 0x10000003)
	GemRB.SetText (Window, Label, str (666))

	# sell price ...
	Label = GemRB.GetControl (Window, 0x10000004)
	GemRB.SetText (Window, Label, str (999))


	# Buy
	Button = GemRB.GetControl (Window, 0)
	GemRB.SetText (Window, Button, 45303)

	# Sell
	Button = GemRB.GetControl (Window, 1)
	GemRB.SetText (Window, Button, 45304)

	# 45374

	# 7 scrollbar
	# 8-11, @11 -@14 - slots and their labels
	# 16 scrollbar
	# 17-20, @20-@23 - slots and labels
	# 25 encumbrance button

	UpdateStoreShoppingWindow ()
	GemRB.UnhideGUI ()


def OpenStoreIdentifyWindow ():
	global StoreIdentifyWindow
	
	GemRB.HideGUI ()
	SelectStoreAction ('identify')

	if StoreIdentifyWindow != None:
		Window = StoreIdentifyWindow
		GemRB.SetVar ("OtherWindow", Window)
		UpdateStoreIdentifyWindow ()
		GemRB.UnhideGUI ()
		return
	
	StoreIdentifyWindow = Window = GemRB.LoadWindow (5)
	GemRB.SetVar ("OtherWindow", Window)

	# title ...
	Label = GemRB.GetControl (Window, 0x0fffffff)
	GemRB.SetText (Window, Label, Store['StoreName'])

	# Identify
	Button = GemRB.GetControl (Window, 4)
	GemRB.SetText (Window, Button, 44971)

	# 14 ta

	# price ...
	Label = GemRB.GetControl (Window, 0x10000001)
	GemRB.SetText (Window, Label, str (666))

	# 6-9 item slots, 0x10000009-c labels

	UpdateStoreIdentifyWindow ()
	GemRB.UnhideGUI ()


def OpenStoreStealWindow ():
	global StoreStealWindow
	
	GemRB.HideGUI ()
	SelectStoreAction ('steal')

	if StoreStealWindow != None:
		Window = StoreStealWindow
		GemRB.SetVar ("OtherWindow", Window)
		UpdateStoreStealWindow ()
		GemRB.UnhideGUI ()
		return
	
	StoreStealWindow = Window = GemRB.LoadWindow (7)
        GemRB.SetVar ("OtherWindow", Window)

	# title ...
	Label = GemRB.GetControl (Window, 0x10000000)
	GemRB.SetText (Window, Label, Store['StoreName'])

	# Steal
	Button = GemRB.GetControl (Window, 0)
	GemRB.SetText (Window, Button, 45305)

	UpdateStoreStealWindow ()
	GemRB.UnhideGUI ()


def OpenStoreDonateWindow ():
	global StoreDonateWindow
	
	GemRB.HideGUI ()
	SelectStoreAction ('donate')

	if StoreDonateWindow != None:
		Window = StoreDonateWindow
		GemRB.SetVar ("OtherWindow", Window)
		UpdateStoreDonateWindow ()
		GemRB.UnhideGUI ()
		return
	
	StoreDonateWindow = Window = GemRB.LoadWindow (10)
        GemRB.SetVar ("OtherWindow", Window)

	# title ...
	Label = GemRB.GetControl (Window, 0x10000005)
	GemRB.SetText (Window, Label, Store['StoreName'])

	# Donate
	Button = GemRB.GetControl (Window, 2)
	GemRB.SetText (Window, Button, 45307)

	# 0 ta
	# 3 donation entry
	# 4 5 +-

	UpdateStoreDonateWindow ()
	GemRB.UnhideGUI ()


def OpenStoreHealWindow ():
	global StoreHealWindow
	
	GemRB.HideGUI ()
	SelectStoreAction ('heal')

	if StoreHealWindow != None:
		Window = StoreHealWindow
		GemRB.SetVar ("OtherWindow", Window)
		UpdateStoreHealWindow ()
		GemRB.UnhideGUI ()
		return
	
	StoreHealWindow = Window = GemRB.LoadWindow (6)
	GemRB.SetVar ("OtherWindow", Window)

	# title ...
	Label = GemRB.GetControl (Window, 0x0fffffff)
	GemRB.SetText (Window, Label, Store['StoreName'])

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

	UpdateStoreHealWindow ()
	GemRB.UnhideGUI ()


def OpenStoreRumourWindow ():
	global StoreRumourWindow
	
	GemRB.HideGUI ()
	SelectStoreAction ('rumour')

	if StoreRumourWindow != None:
		Window = StoreRumourWindow
		GemRB.SetVar ("OtherWindow", Window)
		UpdateStoreRumourWindow ()
		GemRB.UnhideGUI ()
		return
	
	StoreRumourWindow = Window = GemRB.LoadWindow (9)
	GemRB.SetVar ("OtherWindow", Window)
	
	# title ...
	Label = GemRB.GetControl (Window, 0x1000000a)
	GemRB.SetText (Window, Label, Store['StoreName'])
	
	# 13 ta, 15 ta

	UpdateStoreRumourWindow ()
	GemRB.UnhideGUI ()


def OpenStoreRentWindow ():
	global StoreRentWindow, HelpStoreRent
	
	GemRB.HideGUI ()
	SelectStoreAction ('rent')

	if StoreRentWindow != None:
		Window = StoreRentWindow
		GemRB.SetVar ("OtherWindow", Window)
		UpdateStoreRentWindow ()
		GemRB.UnhideGUI ()
		return
	
	StoreRentWindow = Window = GemRB.LoadWindow (8)
	GemRB.SetVar ("OtherWindow", Window)

	# title ...
	Label = GemRB.GetControl (Window, 0x1000000a)
	GemRB.SetText (Window, Label, Store['StoreName'])

	# Peasant
	Button = GemRB.GetControl (Window, 0)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "SelectStoreRentPeasant")
	Button = GemRB.GetControl (Window, 4)
	GemRB.SetText (Window, Button, 45308)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "SelectStoreRentPeasant")

	# Merchant
	Button = GemRB.GetControl (Window, 1)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "SelectStoreRentMerchant")
	Button = GemRB.GetControl (Window, 5)
	GemRB.SetText (Window, Button, 45310)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "SelectStoreRentMerchant")

	# Noble
	Button = GemRB.GetControl (Window, 2)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "SelectStoreRentNoble")
	Button = GemRB.GetControl (Window, 6)
	GemRB.SetText (Window, Button, 45313)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "SelectStoreRentNoble")

	# Royal
	Button = GemRB.GetControl (Window, 3)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "SelectStoreRentRoyal")
	Button = GemRB.GetControl (Window, 7)
	GemRB.SetText (Window, Button, 45316)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "SelectStoreRentRoyal")

	
	# Rent
	Button = GemRB.GetControl (Window, 8)
	GemRB.SetText (Window, Button, 45306)
	GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_DISABLED)


	# price ...
	Label = GemRB.GetControl (Window, 0x1000000c)
	GemRB.SetText (Window, Label, "0")

	HelpStoreRent = Text = GemRB.GetControl (Window, 9)


	GemRB.UnhideGUI ()


last_room = None
def SelectStoreRent (room):
	global last_room

	Window = StoreRentWindow
	
	GemRB.SetText (Window, HelpStoreRent, 66865 + room)

	if last_room != None and room != last_room:
		Picture = GemRB.GetControl (Window, 0 + last_room)
		GemRB.SetButtonState (Window, Picture, IE_GUI_BUTTON_UNPRESSED)
		Button = GemRB.GetControl (Window, 4 + last_room)
		GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_UNPRESSED)
		
	Picture = GemRB.GetControl (Window, 0 + room)
	GemRB.SetButtonState (Window, Picture, IE_GUI_BUTTON_SELECTED)
	Button = GemRB.GetControl (Window, 4 + room)
	GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_PRESSED)

	Label = GemRB.GetControl (Window, 0x1000000c)
	GemRB.SetText (Window, Label, str (Store['StoreRoomPrices'][room]))

	last_room  = room

def SelectStoreRentPeasant ():
	SelectStoreRent (0)

def SelectStoreRentMerchant ():
	SelectStoreRent (1)

def SelectStoreRentNoble ():
	SelectStoreRent (2)

def SelectStoreRentRoyal ():
	SelectStoreRent (3)


def UpdateStoreCommon ():
	global party_gold, character_name, pc
	
	print "UpdateStoreWindow"
	pc = GemRB.GameGetSelectedPCSingle ()
	character_name = GemRB.GetPlayerName (pc, 1)

	party_gold = GemRB.GameGetPartyGold ()
	

def UpdateStoreShoppingWindow ():
	Window = StoreShoppingWindow

	GemRB.HideGUI ()
	UpdateStoreCommon ()

	# character name ...
	Label = GemRB.GetControl (Window, 0x10000005)
	GemRB.SetText (Window, Label, character_name)

	# party gold ...
	Label = GemRB.GetControl (Window, 0x10000002)
	GemRB.SetText (Window, Label, str (party_gold))
	GemRB.UnhideGUI ()

def UpdateStoreIdentifyWindow ():
	Window = StoreIdentifyWindow
	
	GemRB.HideGUI ()
	UpdateStoreCommon ()

	# party gold ...
	Label = GemRB.GetControl (Window, 0x10000000)
	GemRB.SetText (Window, Label, str (party_gold))

	# character name ...
	Label = GemRB.GetControl (Window, 0x10000002)
	GemRB.SetText (Window, Label, character_name)

	GemRB.UnhideGUI ()

def UpdateStoreStealWindow ():
	Window = StoreStealWindow

	GemRB.HideGUI ()
	UpdateStoreCommon ()

	# party gold ...
	Label = GemRB.GetControl (Window, 0x10000001)
	GemRB.SetText (Window, Label, str (party_gold))

	# character name ...
	Label = GemRB.GetControl (Window, 0x10000002)
	GemRB.SetText (Window, Label, character_name)

	GemRB.UnhideGUI ()

def UpdateStoreDonateWindow ():
	Window = StoreDonateWindow

	GemRB.HideGUI ()
	UpdateStoreCommon ()

	# party gold ...
	Label = GemRB.GetControl (Window, 0x10000006)
	GemRB.SetText (Window, Label, str (party_gold))

	GemRB.UnhideGUI ()

def UpdateStoreHealWindow ():
	Window = StoreHealWindow
	
	GemRB.HideGUI ()
	UpdateStoreCommon ()

	# party gold ...
	Label = GemRB.GetControl (Window, 0x10000000)
	GemRB.SetText (Window, Label, str (party_gold))

	GemRB.UnhideGUI ()
	
def UpdateStoreRumourWindow ():
	Window = StoreRumourWindow
	
	GemRB.HideGUI ()
	UpdateStoreCommon ()

	# party gold ...
	Label = GemRB.GetControl (Window, 0x1000000b)
	GemRB.SetText (Window, Label, str (party_gold))

	GemRB.UnhideGUI ()

def UpdateStoreRentWindow ():
	Window = StoreRentWindow
	
	GemRB.HideGUI ()
	UpdateStoreCommon ()

	# party gold ...
	Label = GemRB.GetControl (Window, 0x1000000b)
	GemRB.SetText (Window, Label, str (party_gold))

	GemRB.UnhideGUI ()


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


###################################################
# End of file GUISTORE.py
