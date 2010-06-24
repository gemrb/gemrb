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


# GUISTORE.py - script to open store/inn/temple windows from GUISTORE winpack

###################################################

import GemRB
import GUICommon
import GUICommonWindows
from GUIDefines import *

StoreWindow = None

StoreShoppingWindow = None
StoreIdentifyWindow = None
StoreStealWindow = None
StoreDonateWindow = None
StoreHealWindow = None
StoreRumourWindow = None
StoreRentWindow = None

HelpStoreRent = None

store_name = ""
pc = 0
character_name = ""
party_gold = 0
store_buttons = {}
store_update_functions = {}

def CloseStoreWindow ():
	global StoreWindow 

	CloseStoreShoppingWindow ()
	CloseStoreIdentifyWindow ()
	CloseStoreStealWindow ()
	CloseStoreDonateWindow ()
	CloseStoreHealWindow ()
	CloseStoreRumourWindow ()
	CloseStoreRentWindow ()

	GemRB.SetVar ("OtherWindow", -1)
	if StoreWindow:
		StoreWindow.Unload ()
	StoreWindow = None
	GemRB.LeaveStore ()
	GUICommon.GameWindow.SetVisible(WINDOW_VISIBLE) #enabling the game control screen
	GemRB.UnhideGUI () #enabling the other windows
	GUICommonWindows.SetSelectionChangeHandler (None)
	

def OpenStoreWindow ():
	global StoreWindow, party_gold, store_name, Store
	
	GemRB.HideGUI ()
	GUICommon.GameWindow.SetVisible(WINDOW_INVISIBLE) #removing the game control screen
	
	GemRB.LoadWindowPack ("GUISTORE")
	StoreWindow = Window = GemRB.LoadWindow (3)
	Window.SetVisible (WINDOW_VISIBLE)

	Store = GemRB.GetStore ()
	# font used for store name has only uppercase chars
	store_name = GemRB.GetString (Store['StoreName']).upper ()
	
	# Done
	Button = Window.GetControl (0)
	Button.SetText (1403)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, CloseStoreWindow)
	Button.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)

	# buy / sell
	Button = Window.GetControl (1)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, OpenStoreShoppingWindow)
	store_buttons['shopping'] = Button
	store_update_functions['shopping'] = UpdateStoreShoppingWindow

	# identify
	Button = Window.GetControl (2)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, OpenStoreIdentifyWindow)
	store_buttons['identify'] = Button
	store_update_functions['identify'] = UpdateStoreIdentifyWindow

	# steal
	Button = Window.GetControl (3)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, OpenStoreStealWindow)
	store_buttons['steal'] = Button
	store_update_functions['steal'] = UpdateStoreStealWindow

	# donate
	Button = Window.GetControl (4)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, OpenStoreDonateWindow)
	store_buttons['donate'] = Button
	store_update_functions['donate'] = UpdateStoreDonateWindow

	# heal / cure
	Button = Window.GetControl (5)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, OpenStoreHealWindow)
	store_buttons['heal'] = Button
	store_update_functions['heal'] = UpdateStoreHealWindow

	# rumour
	Button = Window.GetControl (6)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, OpenStoreRumourWindow)
	store_buttons['rumour'] = Button
	store_update_functions['rumour'] = UpdateStoreRumourWindow

	# rent room
	Button = Window.GetControl (7)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, OpenStoreRentWindow)
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
		Button.SetState (IE_GUI_BUTTON_UNPRESSED)
		
	Button = store_buttons[action]
	Button.SetState (IE_GUI_BUTTON_SELECTED)

	GUICommonWindows.SetSelectionChangeHandler (store_update_functions[action])
	
	last_store_action = action



def OpenStoreShoppingWindow ():
	global StoreShoppingWindow
	
	GemRB.HideGUI ()
	SelectStoreAction ('shopping')

	if StoreShoppingWindow != None:
		Window = StoreShoppingWindow
		GemRB.SetVar ("OtherWindow", Window.ID)
		UpdateStoreShoppingWindow ()
		GemRB.UnhideGUI ()
		return

	StoreShoppingWindow = Window = GemRB.LoadWindow (4)
	GemRB.SetVar ("OtherWindow", Window.ID)

	# title ...
	Label = Window.GetControl (0x10000001)
	Label.SetText (store_name)

	# buy price ...
	Label = Window.GetControl (0x10000003)
	Label.SetText (str (666))

	# sell price ...
	Label = Window.GetControl (0x10000004)
	Label.SetText (str (999))


	# Buy
	Button = Window.GetControl (0)
	Button.SetText (45303)

	# Sell
	Button = Window.GetControl (1)
	Button.SetText (45304)

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
		GemRB.SetVar ("OtherWindow", Window.ID)
		UpdateStoreIdentifyWindow ()
		GemRB.UnhideGUI ()
		return
	
	StoreIdentifyWindow = Window = GemRB.LoadWindow (5)
	GemRB.SetVar ("OtherWindow", Window.ID)

	# title ...
	Label = Window.GetControl (0x0fffffff)
	Label.SetText (store_name)

	# Identify
	Button = Window.GetControl (4)
	Button.SetText (44971)

	# 14 ta

	# price ...
	Label = Window.GetControl (0x10000001)
	Label.SetText (str (666))

	# 6-9 item slots, 0x10000009-c labels

	UpdateStoreIdentifyWindow ()
	GemRB.UnhideGUI ()


def OpenStoreStealWindow ():
	global StoreStealWindow
	
	GemRB.HideGUI ()
	SelectStoreAction ('steal')

	if StoreStealWindow != None:
		Window = StoreStealWindow
		GemRB.SetVar ("OtherWindow", Window.ID)
		UpdateStoreStealWindow ()
		GemRB.UnhideGUI ()
		return
	
	StoreStealWindow = Window = GemRB.LoadWindow (7)
	GemRB.SetVar ("OtherWindow", Window.ID)

	# title ...
	Label = Window.GetControl (0x10000000)
	Label.SetText (store_name)

	# Steal
	Button = Window.GetControl (0)
	Button.SetText (45305)

	UpdateStoreStealWindow ()
	GemRB.UnhideGUI ()


def OpenStoreDonateWindow ():
	global StoreDonateWindow
	
	GemRB.HideGUI ()
	SelectStoreAction ('donate')

	if StoreDonateWindow != None:
		Window = StoreDonateWindow
		GemRB.SetVar ("OtherWindow", Window.ID)
		UpdateStoreDonateWindow ()
		GemRB.UnhideGUI ()
		return
	
	StoreDonateWindow = Window = GemRB.LoadWindow (10)
	GemRB.SetVar ("OtherWindow", Window.ID)

	# title ...
	Label = Window.GetControl (0x10000005)
	Label.SetText (store_name)

	# Donate
	Button = Window.GetControl (2)
	Button.SetText (45307)

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
		GemRB.SetVar ("OtherWindow", Window.ID)
		UpdateStoreHealWindow ()
		GemRB.UnhideGUI ()
		return
	
	StoreHealWindow = Window = GemRB.LoadWindow (6)
	GemRB.SetVar ("OtherWindow", Window.ID)

	# title ...
	Label = Window.GetControl (0x0fffffff)
	Label.SetText (store_name)

	# price ...
	Label = Window.GetControl (0x10000001)
	Label.SetText (str (666))

	# price ...
	Label = Window.GetControl (0x1000000e)
	Label.SetText (character_name)

	# Heal
	Button = Window.GetControl (3)
	Button.SetText (8836) # FIXME: better strref


	# 13 ta

	UpdateStoreHealWindow ()
	GemRB.UnhideGUI ()


def OpenStoreRumourWindow ():
	global StoreRumourWindow
	
	GemRB.HideGUI ()
	SelectStoreAction ('rumour')

	if StoreRumourWindow != None:
		Window = StoreRumourWindow
		GemRB.SetVar ("OtherWindow", Window.ID)
		UpdateStoreRumourWindow ()
		GemRB.UnhideGUI ()
		return
	
	StoreRumourWindow = Window = GemRB.LoadWindow (9)
	GemRB.SetVar ("OtherWindow", Window.ID)
	
	# title ...
	Label = Window.GetControl (0x1000000a)
	Label.SetText (store_name)
	
	# 13 ta, 15 ta

	UpdateStoreRumourWindow ()
	GemRB.UnhideGUI ()


def OpenStoreRentWindow ():
	global StoreRentWindow, HelpStoreRent
	
	GemRB.HideGUI ()
	SelectStoreAction ('rent')

	if StoreRentWindow != None:
		Window = StoreRentWindow
		GemRB.SetVar ("OtherWindow", Window.ID)
		UpdateStoreRentWindow ()
		GemRB.UnhideGUI ()
		return
	
	StoreRentWindow = Window = GemRB.LoadWindow (8)
	GemRB.SetVar ("OtherWindow", Window.ID)

	# title ...
	Label = Window.GetControl (0x1000000a)
	Label.SetText (store_name)

	# Peasant
	Button = Window.GetControl (0)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, SelectStoreRentPeasant)
	Button = Window.GetControl (4)
	Button.SetText (45308)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, SelectStoreRentPeasant)

	# Merchant
	Button = Window.GetControl (1)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, SelectStoreRentMerchant)
	Button = Window.GetControl (5)
	Button.SetText (45310)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, SelectStoreRentMerchant)

	# Noble
	Button = Window.GetControl (2)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, SelectStoreRentNoble)
	Button = Window.GetControl (6)
	Button.SetText (45313)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, SelectStoreRentNoble)

	# Royal
	Button = Window.GetControl (3)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, SelectStoreRentRoyal)
	Button = Window.GetControl (7)
	Button.SetText (45316)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, SelectStoreRentRoyal)

	
	# Rent
	Button = Window.GetControl (8)
	Button.SetText (45306)
	Button.SetState (IE_GUI_BUTTON_DISABLED)


	# price ...
	Label = Window.GetControl (0x1000000c)
	Label.SetText ("0")

	HelpStoreRent = Text = Window.GetControl (9)


	GemRB.UnhideGUI ()


last_room = None
def SelectStoreRent (room):
	global last_room

	Window = StoreRentWindow
	
	HelpStoreRent.SetText (66865 + room)

	if last_room != None and room != last_room:
		Picture = Window.GetControl (0 + last_room)
		Picture.SetState (IE_GUI_BUTTON_UNPRESSED)
		Button = Window.GetControl (4 + last_room)
		Button.SetState (IE_GUI_BUTTON_UNPRESSED)
		
	Picture = Window.GetControl (0 + room)
	Picture.SetState (IE_GUI_BUTTON_SELECTED)
	Button = Window.GetControl (4 + room)
	Button.SetState (IE_GUI_BUTTON_PRESSED)

	Label = Window.GetControl (0x1000000c)
	Label.SetText (str (Store['StoreRoomPrices'][room]))

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
	Label = Window.GetControl (0x10000005)
	Label.SetText (character_name)

	# party gold ...
	Label = Window.GetControl (0x10000002)
	Label.SetText (str (party_gold))
	GemRB.UnhideGUI ()

def UpdateStoreIdentifyWindow ():
	Window = StoreIdentifyWindow
	
	GemRB.HideGUI ()
	UpdateStoreCommon ()

	# party gold ...
	Label = Window.GetControl (0x10000000)
	Label.SetText (str (party_gold))

	# character name ...
	Label = Window.GetControl (0x10000002)
	Label.SetText (character_name)

	GemRB.UnhideGUI ()

def UpdateStoreStealWindow ():
	Window = StoreStealWindow

	GemRB.HideGUI ()
	UpdateStoreCommon ()

	# party gold ...
	Label = Window.GetControl (0x10000001)
	Label.SetText (str (party_gold))

	# character name ...
	Label = Window.GetControl (0x10000002)
	Label.SetText (character_name)

	GemRB.UnhideGUI ()

def UpdateStoreDonateWindow ():
	Window = StoreDonateWindow

	GemRB.HideGUI ()
	UpdateStoreCommon ()

	# party gold ...
	Label = Window.GetControl (0x10000006)
	Label.SetText (str (party_gold))

	GemRB.UnhideGUI ()

def UpdateStoreHealWindow ():
	Window = StoreHealWindow
	
	GemRB.HideGUI ()
	UpdateStoreCommon ()

	# party gold ...
	Label = Window.GetControl (0x10000000)
	Label.SetText (str (party_gold))

	GemRB.UnhideGUI ()
	
def UpdateStoreRumourWindow ():
	Window = StoreRumourWindow
	
	GemRB.HideGUI ()
	UpdateStoreCommon ()

	# party gold ...
	Label = Window.GetControl (0x1000000b)
	Label.SetText (str (party_gold))

	GemRB.UnhideGUI ()

def UpdateStoreRentWindow ():
	Window = StoreRentWindow
	
	GemRB.HideGUI ()
	UpdateStoreCommon ()

	# party gold ...
	Label = Window.GetControl (0x1000000b)
	Label.SetText (str (party_gold))

	GemRB.UnhideGUI ()


def CloseStoreShoppingWindow ():
	global StoreShoppingWindow
	
	if StoreShoppingWindow != None:
		if StoreShoppingWindow:
			StoreShoppingWindow.Unload ()
		StoreShoppingWindow = None

	
def CloseStoreIdentifyWindow ():
	global StoreIdentifyWindow
	
	if StoreIdentifyWindow != None:
		if StoreIdentifyWindow:
			StoreIdentifyWindow.Unload ()
		StoreIdentifyWindow = None


def CloseStoreStealWindow ():
	global StoreStealWindow
	
	if StoreStealWindow != None:
		if StoreStealWindow:
			StoreStealWindow.Unload ()
		StoreStealWindow = None


def CloseStoreDonateWindow ():
	global StoreDonateWindow
	
	if StoreDonateWindow != None:
		if StoreDonateWindow:
			StoreDonateWindow.Unload ()
		StoreDonateWindow = None


def CloseStoreHealWindow ():
	global StoreHealWindow
	
	if StoreHealWindow != None:
		if StoreHealWindow:
			StoreHealWindow.Unload ()
		StoreHealWindow = None


def CloseStoreRumourWindow ():
	global StoreRumourWindow
	
	if StoreRumourWindow != None:
		if StoreRumourWindow:
			StoreRumourWindow.Unload ()
		StoreRumourWindow = None


def CloseStoreRentWindow ():
	global StoreRentWindow
	
	if StoreRentWindow != None:
		if StoreRentWindow:
			StoreRentWindow.Unload ()
		StoreRentWindow = None


###################################################
# End of file GUISTORE.py
