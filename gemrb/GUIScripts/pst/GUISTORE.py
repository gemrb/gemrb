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
# $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/GUIScripts/pst/GUISTORE.py,v 1.1 2004/02/19 01:08:11 edheldil Exp $


# GUISTORE.py - script to open store/inn/temple windows from GUISTORE winpack

###################################################

import string
import GemRB
from GUIDefines import *
#from GUICommonWindows import OpenCommonWindows, CloseCommonWindows
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

store_name = ''
store_room_prices = (0, 0, 0, 0)

def OpenStoreWindow ():
	global StoreWindow, store_name, store_room_prices
	
	GemRB.HideGUI ()
	
	if StoreWindow != None:
		CloseStoreShoppingWindow ()
		CloseStoreIdentifyWindow ()

		GemRB.UnloadWindow (StoreWindow)
		StoreWindow = None
		GemRB.SetVar ("OtherWindow", -1)
		
		GemRB.UnhideGUI ()
		return

	# FIXME: temporary
	#GemRB.EnterStore ("FELL")
	GemRB.EnterStore ("BROKAH")
	store_name = string.upper (GemRB.GetStoreName ())
	store_room_prices = GemRB.GetStoreRoomPrices ()
	
	GemRB.LoadWindowPack ("GUISTORE")
	StoreWindow = Window = GemRB.LoadWindow (3)
        #GemRB.SetVar ("OtherWindow", StoreWindow)
	GemRB.SetVisible (Window, 1)


	Button = GemRB.GetControl (Window, 0);
	GemRB.SetText (Window, Button, 1403)   # FIXME: find proper StrRef
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenStoreWindow")


	Button = GemRB.GetControl (Window, 1);
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenStoreShoppingWindow")

	Button = GemRB.GetControl (Window, 2);
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenStoreIdentifyWindow")
	
	Button = GemRB.GetControl (Window, 3);
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenStoreStealWindow")

	Button = GemRB.GetControl (Window, 4);
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenStoreDonateWindow")
	
	Button = GemRB.GetControl (Window, 5);
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenStoreHealWindow")

	Button = GemRB.GetControl (Window, 6);
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenStoreRumoursWindow")

	Button = GemRB.GetControl (Window, 7);
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenStoreRentWindow")


	OpenStoreShoppingWindow ()
	
	GemRB.UnhideGUI()


def OpenStoreShoppingWindow ():
	global StoreShoppingWindow
	
	GemRB.HideGUI ()

	if StoreShoppingWindow != None:
		Window = StoreShoppingWindow
		GemRB.SetVar ("OtherWindow", Window)
		GemRB.UnhideGUI ()
		return
	
	StoreShoppingWindow = Window = GemRB.LoadWindow (4)
	GemRB.SetVar ("OtherWindow", Window)

	# title ...
	Label = GemRB.GetControl (Window, 0x10000001)
	GemRB.SetText (Window, Label, store_name)

	# 45303 buy
	# 45304 sell
	
	GemRB.UnhideGUI ()


def OpenStoreIdentifyWindow ():
	global StoreIdentifyWindow
	
	GemRB.HideGUI ()

	if StoreIdentifyWindow != None:
		Window = StoreIdentifyWindow
		GemRB.SetVar ("OtherWindow", Window)
		GemRB.UnhideGUI ()
		return
	
	StoreIdentifyWindow = Window = GemRB.LoadWindow (5)
	GemRB.SetVar ("OtherWindow", Window)

	# title ...
	Label = GemRB.GetControl (Window, 0x0fffffff)
	GemRB.SetText (Window, Label, store_name)

	GemRB.UnhideGUI ()


def OpenStoreStealWindow ():
	global StoreStealWindow
	
	GemRB.HideGUI ()

	if StoreStealWindow != None:
		Window = StoreStealWindow
		GemRB.SetVar ("OtherWindow", Window)
		GemRB.UnhideGUI ()
		return
	
	StoreStealWindow = Window = GemRB.LoadWindow (7)
        GemRB.SetVar ("OtherWindow", Window)

	# title ...
	Label = GemRB.GetControl (Window, 0x10000000)
	GemRB.SetText (Window, Label, store_name)

	# 45305 Steal
	GemRB.UnhideGUI ()


def OpenStoreDonateWindow ():
	global StoreDonateWindow
	
	GemRB.HideGUI ()

	if StoreDonateWindow != None:
		Window = StoreDonateWindow
		GemRB.SetVar ("OtherWindow", Window)
		GemRB.UnhideGUI ()
		return
	
	StoreDonateWindow = Window = GemRB.LoadWindow (10)
        GemRB.SetVar ("OtherWindow", Window)

	# title ...
	Label = GemRB.GetControl (Window, 0x10000005)
	GemRB.SetText (Window, Label, store_name)
	# 45307 Donate
	
	GemRB.UnhideGUI ()


def OpenStoreHealWindow ():
	global StoreHealWindow
	
	GemRB.HideGUI ()

	if StoreHealWindow != None:
		Window = StoreHealWindow
		GemRB.SetVar ("OtherWindow", Window)
		GemRB.UnhideGUI ()
		return
	
	StoreHealWindow = Window = GemRB.LoadWindow (7)
	GemRB.SetVar ("OtherWindow", Window)

	# title ...
	Label = GemRB.GetControl (Window, 0x10000000)
	GemRB.SetText (Window, Label, store_name)
	
	GemRB.UnhideGUI ()


def OpenStoreRumourWindow ():
	global StoreRumourWindow
	
	GemRB.HideGUI ()

	if StoreRumourWindow != None:
		Window = StoreRumourWindow
		GemRB.SetVar ("OtherWindow", Window)
		GemRB.UnhideGUI ()
		return
	
	StoreRumourWindow = Window = GemRB.LoadWindow (9)
		
        GemRB.SetVar ("OtherWindow", Window)
	GemRB.UnhideGUI ()


def OpenStoreRentWindow ():
	global StoreRentWindow, HelpStoreRent
	
	GemRB.HideGUI ()

	if StoreRentWindow != None:
		Window = StoreRentWindow
		GemRB.SetVar ("OtherWindow", Window)
		GemRB.UnhideGUI ()
		return
	
	StoreRentWindow = Window = GemRB.LoadWindow (8)
	GemRB.SetVar ("OtherWindow", Window)

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


	# title ...
	Label = GemRB.GetControl (Window, 0x1000000a)
	GemRB.SetText (Window, Label, store_name)

	# party gold ...
	Label = GemRB.GetControl (Window, 0x1000000b)
	GemRB.SetText (Window, Label, "$1,000")

	# price ...
	Label = GemRB.GetControl (Window, 0x1000000c)
	GemRB.SetText (Window, Label, "$5")

	HelpStoreRent = Text = GemRB.GetControl (Window, 9)


	GemRB.UnhideGUI ()


last_room = None
def SelectStoreRent (room):
	global last_room

	Window = StoreRentWindow
	
	GemRB.SetText (Window, HelpStoreRent, 66865 + room)

	print last_room, room

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
	GemRB.SetText (Window, Label, str (store_room_prices[room]))

	last_room  = room

def SelectStoreRentPeasant ():
	SelectStoreRent (0)

def SelectStoreRentMerchant ():
	SelectStoreRent (1)

def SelectStoreRentNoble ():
	SelectStoreRent (2)

def SelectStoreRentRoyal ():
	SelectStoreRent (3)



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
