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
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#


# GUISTORE.py - script to open the store/inn/temple windows

###################################################

import GemRB
import GUICommon
import GUICommonWindows
from GUIDefines import *
from ie_stats import *
from ie_slots import *
from ie_sounds import *
from ie_feats import FEAT_MERCANTILE_BACKGROUND

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

ITEM_PC    = 0
ITEM_STORE = 1

Inventory = None
RentIndex = -1
Store = None
Buttons = [-1,-1,-1,-1]
inventory_slots = ()
total_price = 0
total_income = 0
if GUICommon.GameIsIWD2():
	ItemButtonCount = 6
else:
	ItemButtonCount = 4
RepModTable = None
SpellTable = None
PreviousPC = 0
BarteringPC = 0

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

if GUICommon.GameIsIWD1():
	# no bam for bags
	storebams = ("STORSTOR","STORTVRN","STORINN","STORTMPL","STORSTOR","STORSTOR")
else:
	storebams = ("STORSTOR","STORTVRN","STORINN","STORTMPL","STORBAG","STORBAG")
storetips = (14288,14292,14291,12138,15013,14289,14287)
roomtypes = (17389,17517,17521,17519)
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
	if not GUICommon.GameIsBG1():
		if PortraitWindow:
			PortraitWindow.Unload ()
	StoreWindow = None
	GemRB.LeaveStore ()
	if not GUICommon.GameIsBG1():
		GUICommonWindows.PortraitWindow = OldPortraitWindow
	if Inventory:
		GUIINV.OpenInventoryWindow ()
	else:
		GemRB.GamePause (0, 3)
		GUICommon.GameWindow.SetVisible(WINDOW_VISIBLE) #enabling the game control screen
		GemRB.UnhideGUI () #enabling the other windows
		GUICommonWindows.SetSelectionChangeHandler( None )
	return

def OpenStoreWindow ():
	global Store
	global StoreWindow, ActionWindow, PortraitWindow
	global OldPortraitWindow
	global store_funcs
	global SpellTable, RepModTable
	global Inventory, BarteringPC

	#these are function pointers, not strings
	#can't put this in global init, doh!
	store_funcs = (OpenStoreShoppingWindow,
	OpenStoreIdentifyWindow,OpenStoreStealWindow,
	OpenStoreHealWindow, OpenStoreDonateWindow,
	OpenStoreRumourWindow,OpenStoreRentWindow )

	RepModTable = GemRB.LoadTable ("repmodst")
	SpellTable = GemRB.LoadTable ("storespl", 1)

	GemRB.HideGUI ()
	GUICommon.GameWindow.SetVisible(WINDOW_INVISIBLE) #removing the game control screen

	if GemRB.GetVar ("Inventory"):
		Inventory = 1
	else:
		Inventory = None
		# pause the game, so we don't get interrupted
		GemRB.GamePause (1, 3)

	GemRB.SetVar ("Action", 0)
	if GUICommon.GameIsIWD2():
		GemRB.LoadWindowPack ("GUISTORE", 800, 600)
	else:
		GemRB.LoadWindowPack ("GUISTORE", 640, 480)
	StoreWindow = Window = GemRB.LoadWindow (3)
	#saving the original portrait window
	OldPortraitWindow = GUICommonWindows.PortraitWindow
	if GUICommon.GameIsIWD2() or GUICommon.GameIsBG1():
		#PortraitWindow = GUICommonWindows.OpenPortraitWindow ()
		pass
	else:
		PortraitWindow = GUICommonWindows.OpenPortraitWindow (0)
	ActionWindow = GemRB.LoadWindow (0)
	#this window is static and grey, but good to stick the frame onto
	ActionWindow.SetFrame ()

	Store = GemRB.GetStore ()
	BarteringPC = GemRB.GameGetFirstSelectedPC ()

	# Done
	Button = Window.GetControl (0)
	Button.SetText (11973)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, CloseStoreWindow)

	#Store type icon
	if not GUICommon.GameIsIWD2():
		Button = Window.GetControl (5)
		Button.SetSprites (storebams[Store['StoreType']],0,0,0,0,0)

	#based on shop type, these buttons will change
	store_type = Store['StoreType']
	store_buttons = Store['StoreButtons']
	for i in range (4):
		Buttons[i] = Button = Window.GetControl (i+1)
		Action = store_buttons[i]
		Button.SetVarAssoc ("Action", i)
		if Action>=0:
			Button.SetFlags (IE_GUI_BUTTON_RADIOBUTTON, OP_OR)
			if GUICommon.GameIsIWD1() or GUICommon.GameIsIWD2():
				Button.SetSprites ("GUISTBBC", Action, 1,2,0,0)
			else:
				Button.SetSprites ("GUISTBBC", Action, 0,1,2,0)
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
	if not GUICommon.GameIsIWD2():
		if GUICommon.GameIsBG1():
			GUICommonWindows.PortraitWindow.SetVisible (WINDOW_VISIBLE)
		else:
			PortraitWindow.SetVisible (WINDOW_VISIBLE)
	return

def OpenStoreShoppingWindow ():
	global StoreShoppingWindow
	global LeftButton, RightButton

	CloseWindows()

	StoreShoppingWindow = Window = GemRB.LoadWindow (2)

	# left scrollbar
	ScrollBarLeft = Window.GetControl (11)
	ScrollBarLeft.SetEvent (IE_GUI_SCROLLBAR_ON_CHANGE, RedrawStoreShoppingWindow)

	# right scrollbar
	ScrollBarRight = Window.GetControl (12)
	ScrollBarRight.SetEvent (IE_GUI_SCROLLBAR_ON_CHANGE, RedrawStoreShoppingWindow)

	if Inventory:
		# Title
		Label = Window.GetControl (0xfffffff)
		if GUICommon.GameIsIWD1() or GUICommon.GameIsIWD2():
			Label.SetText (26291)
		elif GUICommon.GameIsBG2():
			Label.SetText (51881)
		else:
			Label.SetText ("")
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
		Label = Window.GetControl (0x1000002b)
		Label.SetText ("0")

		# sell price ...
		Label = Window.GetControl (0x1000002c)
		Label.SetText ("0")

	for i in range (ItemButtonCount):
		Button = Window.GetControl (i+5)
		if GUICommon.GameIsBG2():
			Button.SetBorder (0,0,0,0,0,0,0,128,160,0,1)
		else:
			Button.SetBorder (0,0,0,0,0,32,32,192,128,0,1)
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, SelectBuy)
		Button.SetEvent (IE_GUI_BUTTON_ON_RIGHT_PRESS, InfoLeftWindow)
		Button.SetFont ("NUMBER")
		Button.SetFlags (IE_GUI_BUTTON_ALIGN_RIGHT|IE_GUI_BUTTON_ALIGN_TOP, OP_OR)
		Button.AttachScrollBar (ScrollBarLeft)

		Button = Window.GetControl (i+13)
		if GUICommon.GameIsBG2():
			Button.SetBorder (0,0,0,0,0,0,0,128,160,0,1)
			Button.SetSprites ("GUIBTBUT", 0, 0,1,2,5)
		else:
			Button.SetBorder (0,0,0,0,0,32,32,192,128,0,1)
		if Store['StoreType'] != 3: # can't sell to temples
			Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, SelectSell)
		Button.SetEvent (IE_GUI_BUTTON_ON_RIGHT_PRESS, InfoRightWindow)
		Button.SetFont ("NUMBER")
		Button.SetFlags (IE_GUI_BUTTON_ALIGN_RIGHT|IE_GUI_BUTTON_ALIGN_TOP, OP_OR)
		Button.AttachScrollBar (ScrollBarRight)

	# Buy
	LeftButton = Button = Window.GetControl (2)
	if Inventory:
		if GUICommon.GameIsIWD2():
			Button.SetText (26287)
		elif GUICommon.GameIsIWD1():
			Button.SetText (26288)
		elif GUICommon.GameIsBG2():
			Button.SetText (51882)
		else:
			Button.SetText ("")
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, ToBackpackPressed)
	else:
		Button.SetText (13703)
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, BuyPressed)

	# Sell
	RightButton = Button = Window.GetControl (3)
	if Inventory:
		if GUICommon.GameIsIWD1() or GUICommon.GameIsIWD2():
			Button.SetText (26288)
		elif GUICommon.GameIsBG2():
			Button.SetText (51883)
		else:
			Button.SetText ("")
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, ToBagPressed)
	else:
		Button.SetText (13704)
		if Store['StoreType'] != 3: # can't sell to temples
			Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, SellPressed)

	# inactive button
	if GUICommon.GameIsBG2():
		Button = Window.GetControl (50)
		Button.SetState (IE_GUI_BUTTON_LOCKED)
		Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_SET)

	#backpack
	Button = Window.GetControl (44)
	Button.SetState (IE_GUI_BUTTON_LOCKED)

	# encumbrance
	Label = Window.CreateLabel (0x10000043, 15,325,60,15,"NUMBER","0:",IE_FONT_ALIGN_LEFT|IE_FONT_ALIGN_TOP)
	Label = Window.CreateLabel (0x10000044, 15,365,80,15,"NUMBER","0:",IE_FONT_ALIGN_RIGHT|IE_FONT_ALIGN_TOP)

	GUICommonWindows.SetSelectionChangeHandler( UpdateStoreShoppingWindow )
	UpdateStoreShoppingWindow ()
	Window.SetVisible (WINDOW_VISIBLE)
	return

def OpenStoreIdentifyWindow ():
	global StoreIdentifyWindow
	global LeftButton

	GemRB.SetVar ("Index", -1)
	GemRB.SetVar ("TopIndex", 0)
	CloseWindows()

	StoreIdentifyWindow = Window = GemRB.LoadWindow (4)

	ScrollBar = Window.GetControl (7)
	ScrollBar.SetEvent (IE_GUI_SCROLLBAR_ON_CHANGE, RedrawStoreIdentifyWindow)

	TextArea = Window.GetControl (23)
	TextArea.SetFlags (IE_GUI_TEXTAREA_AUTOSCROLL)

	# Identify
	LeftButton = Button = Window.GetControl (5)
	Button.SetText (14133)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, IdentifyPressed)
	Button.SetEvent (IE_GUI_BUTTON_ON_RIGHT_PRESS, InfoIdentifyWindow)

	# price ...
	Label = Window.GetControl (0x10000003)
	Label.SetText ("0")

	# 8-11 item slots, 0x1000000c-f labels
	for i in range (ItemButtonCount):
		Button = Window.GetControl (i+8)
		Button.SetFlags (IE_GUI_BUTTON_RADIOBUTTON, OP_OR)
		if GUICommon.GameIsIWD1() or GUICommon.GameIsIWD2():
			Button.SetSprites ("GUISTMSC", 0, 1,2,0,3)
			Button.SetBorder (0,0,0,0,0,32,32,192,128,0,1)
		elif GUICommon.GameIsBG1():
			Button.SetBorder (0,0,0,0,0,32,32,192,128,0,1)
		else:
			Button.SetBorder (0,0,0,0,0,0,0,128,160,0,1)
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, SelectID)
		Button.SetEvent (IE_GUI_BUTTON_ON_RIGHT_PRESS, InfoIdentifyWindow)
		Button.SetFont ("NUMBER")
		Button.SetFlags (IE_GUI_BUTTON_ALIGN_RIGHT|IE_GUI_BUTTON_ALIGN_TOP, OP_OR)
		Button.AttachScrollBar (ScrollBar)

	GUICommonWindows.SetSelectionChangeHandler( UpdateStoreIdentifyWindow )
	UpdateStoreIdentifyWindow ()
	Window.SetVisible (WINDOW_VISIBLE)
	return

def OpenStoreStealWindow ():
	global StoreStealWindow
	global LeftButton

	GemRB.SetVar ("RightIndex", 0)
	GemRB.SetVar ("LeftIndex", 0)
	GemRB.SetVar ("RightTopIndex", 0)
	GemRB.SetVar ("LeftTopIndex", 0)
	CloseWindows()

	StoreStealWindow = Window = GemRB.LoadWindow (6)

	# left scrollbar
	ScrollBarLeft = Window.GetControl (9)
	ScrollBarLeft.SetEvent (IE_GUI_SCROLLBAR_ON_CHANGE, RedrawStoreStealWindow)

	# right scrollbar
	ScrollBarRight = Window.GetControl (10)
	ScrollBarRight.SetEvent (IE_GUI_SCROLLBAR_ON_CHANGE, RedrawStoreStealWindow)

	for i in range (ItemButtonCount):
		Button = Window.GetControl (i+4)
		if GUICommon.GameIsBG2():
			Button.SetBorder (0,0,0,0,0,0,0,128,160,0,1)
		else:
			Button.SetBorder (0,0,0,0,0,32,32,192,128,0,1)
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, RedrawStoreStealWindow)
		Button.AttachScrollBar (ScrollBarLeft)

		Button = Window.GetControl (i+11)
		if GUICommon.GameIsBG2():
			Button.SetBorder (0,0,0,0,0,0,0,128,160,0,1)
		else:
			Button.SetBorder (0,0,0,0,0,32,32,192,128,0,1)
		Button.SetEvent (IE_GUI_BUTTON_ON_RIGHT_PRESS, InfoRightWindow)
		Button.SetFont ("NUMBER")
		Button.SetFlags (IE_GUI_BUTTON_ALIGN_RIGHT|IE_GUI_BUTTON_ALIGN_TOP, OP_OR)
		Button.AttachScrollBar (ScrollBarRight)

	# Steal
	LeftButton = Button = Window.GetControl (1)
	Button.SetText (14179)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, StealPressed)

	Button = Window.GetControl (37)
	Button.SetState (IE_GUI_BUTTON_LOCKED)

	# encumbrance
	Label = Window.CreateLabel (0x10000043, 15,325,60,15,"NUMBER","0:",IE_FONT_ALIGN_LEFT|IE_FONT_ALIGN_TOP)
	Label = Window.CreateLabel (0x10000044, 15,365,80,15,"NUMBER","0:",IE_FONT_ALIGN_RIGHT|IE_FONT_ALIGN_TOP)

	GUICommonWindows.SetSelectionChangeHandler( UpdateStoreStealWindow )
	UpdateStoreStealWindow ()
	Window.SetVisible (WINDOW_VISIBLE)
	return

def OpenStoreDonateWindow ():
	global StoreDonateWindow

	CloseWindows ()

	StoreDonateWindow = Window = GemRB.LoadWindow (9)

	# graphics
	Button = Window.GetControl (10)
	Button.SetFlags (IE_GUI_BUTTON_PICTURE|IE_GUI_BUTTON_ANIMATED|IE_GUI_BUTTON_PLAYONCE, OP_OR)
	Button.SetState (IE_GUI_BUTTON_LOCKED)

	# Donate
	Button = Window.GetControl (3)
	Button.SetText (15101)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, DonateGold)
	Button.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)

	# Entry
	Field = Window.GetControl (5)
	Field.SetText ("0")
	Field.SetEvent (IE_GUI_EDIT_ON_CHANGE, UpdateStoreDonateWindow)
	Field.SetStatus (IE_GUI_EDIT_NUMBER|IE_GUI_CONTROL_FOCUSED)

	# +
	Button = Window.GetControl (6)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, IncrementDonation)
	# -
	Button = Window.GetControl (7)
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

	StoreHealWindow = Window = GemRB.LoadWindow (5)

	ScrollBar = Window.GetControl (7)
	ScrollBar.SetEvent (IE_GUI_SCROLLBAR_ON_CHANGE, UpdateStoreHealWindow)

	#spell buttons
	for i in range (ItemButtonCount):
		Button = Window.GetControl (i+8)
		Button.SetFlags (IE_GUI_BUTTON_RADIOBUTTON, OP_OR)
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, UpdateStoreHealWindow)
		Button.SetEvent (IE_GUI_BUTTON_ON_RIGHT_PRESS, InfoHealWindow)
		#Button.AttachScrollBar (ScrollBar)

	# price tag
	Label = Window.GetControl (0x10000003)
	Label.SetText ("0")

	# Heal
	Button = Window.GetControl (5)
	Button.SetText (13703)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, BuyHeal)
	Button.SetState (IE_GUI_BUTTON_DISABLED)

	Count = Store['StoreCureCount']
	if Count>4:
		Count = Count-4
	else:
		Count = 0
	ScrollBar.SetVarAssoc ("TopIndex", Count+1)

	GUICommonWindows.SetSelectionChangeHandler( UpdateStoreHealWindow )
	UpdateStoreHealWindow ()
	Window.SetVisible (WINDOW_VISIBLE)
	return

def OpenStoreRumourWindow ():
	global StoreRumourWindow

	GemRB.SetVar ("TopIndex", 0)
	CloseWindows()

	StoreRumourWindow = Window = GemRB.LoadWindow (8)

	#removing those pesky labels
	for i in range (5):
		Window.DeleteControl (0x10000005+i)

	TextArea = Window.GetControl (11)
	TextArea.SetText (14144)

	#tavern quality image
	if GUICommon.GameIsBG1() or GUICommon.GameIsBG2():
		BAM = "TVRNQUL%d"% ((Store['StoreFlags']>>9)&3)
		Button = Window.GetControl (12)
		Button.SetSprites (BAM, 0, 0, 0, 0, 0)
		Button.SetState (IE_GUI_BUTTON_LOCKED)

	ScrollBar = Window.GetControl (5)
	ScrollBar.SetEvent (IE_GUI_SCROLLBAR_ON_CHANGE, UpdateStoreRumourWindow)
	Count = Store['StoreDrinkCount']
	if Count>4:
		Count = Count-4
	else:
		Count = 0
	ScrollBar.SetVarAssoc ("TopIndex", Count+1)

	GUICommonWindows.SetSelectionChangeHandler( UpdateStoreRumourWindow )
	UpdateStoreRumourWindow ()
	Window.SetVisible (WINDOW_VISIBLE)
	return

def OpenStoreRentWindow ():
	global StoreRentWindow, RentIndex

	CloseWindows()

	StoreRentWindow = Window = GemRB.LoadWindow (7)

	# room types
	RentIndex = -1
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
		Button.SetText (14294+i)
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, UpdateStoreRentWindow)
		Button.SetFlags (IE_GUI_BUTTON_RADIOBUTTON, OP_OR)
		Button.SetVarAssoc ("RentIndex", i)
		if GUICommon.GameIsBG1():
			#these bioware guys screw up everything possible
			#remove this line if you fixed guistore
			Button.SetSprites ("GUISTROC",0, 1,2,0,3)
		if ok<0:
			Button.SetState (IE_GUI_BUTTON_DISABLED)

	# Rent
	Button = Window.GetControl (11)
	Button.SetText (14293)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, RentRoom)

	GemRB.SetVar ("RentIndex", RentIndex)

	GUICommonWindows.SetSelectionChangeHandler( UpdateStoreRentWindow )
	UpdateStoreRentWindow ()
	Window.SetVisible (WINDOW_VISIBLE)
	return

def UpdateStoreCommon (Window, title, name, gold):

	Label = Window.GetControl (title)
	Label.SetText (Store['StoreName'])

	if name:
		pc = GemRB.GameGetSelectedPCSingle ()
		Label = Window.GetControl (name)
		Label.SetText (GemRB.GetPlayerName (pc, 0) )

	Label = Window.GetControl (gold)
	Label.SetText (str(GemRB.GameGetPartyGold ()))
	return

def GetPC():
	global PreviousPC

	if PreviousPC:
		pc = GemRB.GameGetSelectedPCSingle ()
		if PreviousPC != pc:
			PreviousPC = pc
			# reset the store indices, to prevent overscrolling
			GemRB.SetVar ("RightIndex", 0)
			GemRB.SetVar ("LeftIndex", 0)
			GemRB.SetVar ("RightTopIndex", 0)
			GemRB.SetVar ("LeftTopIndex", 0)
			GemRB.SetVar ("Index", 0)
			GemRB.SetVar ("TopIndex", 0)
	else:
		PreviousPC = GemRB.GameGetSelectedPCSingle ()
		pc = PreviousPC

	return pc

def UpdateStoreShoppingWindow ():
	global Store, inventory_slots

	Window = StoreShoppingWindow
	#reget store in case of a change
	Store = GemRB.GetStore ()
	pc = GetPC()

	LeftCount = Store['StoreItemCount'] - ItemButtonCount + 1
	if LeftCount<0:
		LeftCount=0
	ScrollBar = Window.GetControl (11)
	ScrollBar.SetVarAssoc ("LeftTopIndex", LeftCount)
	LeftTopIndex = GemRB.GetVar ("LeftTopIndex")
	if LeftTopIndex>LeftCount:
		GemRB.SetVar ("LeftTopIndex", LeftCount)

	pc = GemRB.GameGetSelectedPCSingle ()
	inventory_slots = GemRB.GetSlots (pc, SLOT_INVENTORY)
	RightCount = len(inventory_slots) - ItemButtonCount + 1
	if RightCount<0:
		RightCount=0
	ScrollBar = Window.GetControl (12)
	ScrollBar.SetVarAssoc ("RightTopIndex", RightCount)
	RightTopIndex = GemRB.GetVar ("RightTopIndex")
	if RightTopIndex>RightCount:
		GemRB.SetVar ("RightTopIndex", RightCount)

	RedrawStoreShoppingWindow ()
	return

def SelectID ():
	pc = GemRB.GameGetSelectedPCSingle ()
	Index = GemRB.GetVar ("Index")
	GemRB.ChangeStoreItem (pc, inventory_slots[Index], SHOP_ID|SHOP_SELECT)
	RedrawStoreIdentifyWindow ()
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
		ErrorWindow (11047)
		return

	pc = GemRB.GameGetSelectedPCSingle ()
	LeftCount = Store['StoreItemCount']
	#going backwards because removed items shift the slots
	for i in range (LeftCount, 0, -1):
		Flags = GemRB.IsValidStoreItem (pc, i-1, ITEM_STORE)&SHOP_SELECT
		if Flags:
			Slot = GemRB.GetStoreItem (i-1)
			Item = GemRB.GetItem (Slot['ItemResRef'])
			Price = GetRealPrice (pc, "sell", Item, Slot)
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
	GemRB.PlaySound(DEF_SOLD)
	UpdateStoreShoppingWindow ()
	return

def RedrawStoreShoppingWindow ():
	global BuySum, SellSum

	Window = StoreShoppingWindow

	UpdateStoreCommon (Window, 0x10000003, 0x1000002e, 0x1000002a)
	pc = GemRB.GameGetSelectedPCSingle ()

	LeftTopIndex = GemRB.GetVar ("LeftTopIndex")
	LeftIndex = GemRB.GetVar ("LeftIndex")
	RightTopIndex = GemRB.GetVar ("RightTopIndex")
	RightIndex = GemRB.GetVar ("RightIndex")
	idx = [ LeftTopIndex, RightTopIndex, LeftIndex, RightIndex ]
	LeftCount = Store['StoreItemCount']
	BuySum = 0
	selected_count = 0
	for i in range (LeftCount):
		if GemRB.IsValidStoreItem (pc, i, ITEM_STORE) & SHOP_SELECT:
			Slot = GemRB.GetStoreItem (i)
			Item = GemRB.GetItem (Slot['ItemResRef'])
			selected_count += 1
			if Inventory:
				Price = 1
			else:
				Price = GetRealPrice (pc, "sell", Item, Slot)
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
				Price = GetRealPrice (pc, "buy", Item, Slot)
			if Flags & SHOP_ID:
				Price = 1
			SellSum = SellSum + Price

	Label = Window.GetControl (0x1000002b)
	if Inventory:
		Label.SetText ("")
	else:
		Label.SetText (str(BuySum) )
	# also disable the button if the inventory is full
	if BuySum and selected_count <= len(GemRB.GetSlots (pc, SLOT_INVENTORY, -1)):
		LeftButton.SetState (IE_GUI_BUTTON_ENABLED)
	else:
		LeftButton.SetState (IE_GUI_BUTTON_DISABLED)

	Label = Window.GetControl (0x1000002c)
	if Inventory:
		Label.SetText ("")
	else:
		Label.SetText (str(SellSum) )
	if SellSum:
		RightButton.SetState (IE_GUI_BUTTON_ENABLED)
	else:
		RightButton.SetState (IE_GUI_BUTTON_DISABLED)

	for i in range (ItemButtonCount):
		if i+LeftTopIndex<LeftCount:
			Slot = GemRB.GetStoreItem (i+LeftTopIndex)
		else:
			Slot = None
		Button = Window.GetControl (i+5)
		Label = Window.GetControl (0x10000012+i)
		Button.SetVarAssoc ("LeftIndex", LeftTopIndex+i)
		SetupItems (pc, Slot, Button, Label, i, ITEM_STORE, idx)

		if i+RightTopIndex<RightCount:
			Slot = GemRB.GetSlotItem (pc, inventory_slots[i+RightTopIndex])
		else:
			Slot = None
		Button = Window.GetControl (i+13)
		Label = Window.GetControl (0x1000001e+i)
		Button.SetVarAssoc ("RightIndex", RightTopIndex+i)
		SetupItems (pc, Slot, Button, Label, i, ITEM_PC, idx)

	return

def UpdateStoreIdentifyWindow ():
	global inventory_slots

	Window = StoreIdentifyWindow

	pc = GetPC()
	inventory_slots = GemRB.GetSlots (pc, SLOT_INVENTORY)
	Count = len(inventory_slots)
	ScrollBar = Window.GetControl (7)
	ScrollBar.SetVarAssoc ("TopIndex", Count-ItemButtonCount+1)
	GemRB.SetVar ("Index", -1)
	RedrawStoreIdentifyWindow ()
	return

def RedrawStoreIdentifyWindow ():
	Window = StoreIdentifyWindow

	UpdateStoreCommon (Window, 0x10000000, 0x10000005, 0x10000001)
	TopIndex = GemRB.GetVar ("TopIndex")
	Index = GemRB.GetVar ("Index")
	pc = GemRB.GameGetSelectedPCSingle ()
	Count = len(inventory_slots)
	IDPrice = Store['IDPrice']

	Selected = 0
	for Slot in range (0, Count):
		flags = GemRB.IsValidStoreItem (pc, inventory_slots[Slot], ITEM_PC)
		if flags & SHOP_ID and flags & SHOP_SELECT:
			Selected += 1

	for i in range (ItemButtonCount):
		if TopIndex+i<Count:
			Slot = GemRB.GetSlotItem (pc, inventory_slots[TopIndex+i])
		else:
			Slot = None
		Button = Window.GetControl (i+8)
		# TODO: recheck they really differ
		if GUICommon.GameIsIWD2():
			Label = Window.GetControl (0x1000000d+i)
		else:
			Label = Window.GetControl (0x1000000c+i)
		Button.SetVarAssoc ("Index", TopIndex+i)
		if Slot:
			Flags = GemRB.IsValidStoreItem (pc, inventory_slots[TopIndex+i], ITEM_PC)
			Item = GemRB.GetItem (Slot['ItemResRef'])
			Button.SetItemIcon (Slot['ItemResRef'], 0)
			if Item['MaxStackAmount'] > 1:
				Button.SetText (str(Slot['Usages0']))
			else:
				Button.SetText ("")
			Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_NAND)
			Button.SetFlags (IE_GUI_BUTTON_PICTURE, OP_OR)
			if Flags & SHOP_ID:
				if Flags & SHOP_SELECT:
					Button.SetState (IE_GUI_BUTTON_SELECTED)
				else:
					Button.SetState (IE_GUI_BUTTON_ENABLED)

				GemRB.SetToken ("ITEMNAME", GemRB.GetString (Item['ItemName']))
				GemRB.SetToken ("ITEMCOST", str(IDPrice) )
				Button.EnableBorder (0, 1)
			else:
				Button.SetState (IE_GUI_BUTTON_DISABLED)
				GemRB.SetToken ("ITEMNAME", GemRB.GetString (Item['ItemNameIdentified']))
				GemRB.SetToken ("ITEMCOST", str(0) )
				Button.EnableBorder (0, 0)

			Label.SetText (10162)
		else:
			Button.SetState (IE_GUI_BUTTON_DISABLED)
			Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_OR)
			Button.SetFlags (IE_GUI_BUTTON_PICTURE, OP_NAND)
			Label.SetText ("")

	Button = Window.GetControl (5)
	Label = Window.GetControl (0x10000003)
	if Selected:
		Button.SetState (IE_GUI_BUTTON_ENABLED)
		Label.SetText (str(IDPrice * Selected) )
	else:
		Button.SetState (IE_GUI_BUTTON_DISABLED)
		Label.SetText (str(0) )
	return

def IdentifyPressed ():
	pc = GemRB.GameGetSelectedPCSingle ()
	Count = len(inventory_slots)

	# get all the selected items
	toID = []
	for Slot in range (0, Count):
		Flags = GemRB.IsValidStoreItem (pc, inventory_slots[Slot], ITEM_PC)
		if Flags & SHOP_SELECT and Flags & SHOP_ID:
			toID.append(Slot)

	# enough gold?
	EndGold = GemRB.GameGetPartyGold () - Store['IDPrice'] * len(toID)
	if EndGold < 0:
		return

	# identify
	Window = StoreIdentifyWindow
	TextArea = Window.GetControl (23)
	for i in toID:
		GemRB.ChangeStoreItem (pc, inventory_slots[i], SHOP_ID)
		Slot = GemRB.GetSlotItem (pc, inventory_slots[i])
		Item = GemRB.GetItem (Slot['ItemResRef'])
		# FIXME: some items have the title, some don't - figure it out
		TextArea.Append(Item['ItemNameIdentified'])
		TextArea.Append("\n\n")
		TextArea.Append(Item['ItemDescIdentified'])
		TextArea.Append("\n\n\n")
	GemRB.GameSetPartyGold (EndGold)

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

	MessageWindow = Window = GemRB.LoadWindow (12)

	# TODO: check non-bg2 games to see which label is which
	if GUICommon.GameIsBG2():
		NameLabel = Window.GetControl (0x10000000)
		FakeLabel = Window.GetControl (0x10000007)
	else:
		NameLabel = Window.GetControl (0x10000007)
		FakeLabel = Window.GetControl (0x10000000)

	#fake label
	FakeLabel.SetText ("")

	#description bam
	if GUICommon.GameIsBG1() or GUICommon.GameIsBG2():
		Button = Window.GetControl (7)
		Button.SetFlags (IE_GUI_BUTTON_PICTURE | IE_GUI_BUTTON_CENTER_PICTURES | IE_GUI_BUTTON_NO_IMAGE, OP_OR)
		Button.SetItemIcon (Slot['ItemResRef'], 2)

	#slot bam
	Button = Window.GetControl (2)
	Button.SetItemIcon (Slot['ItemResRef'], 0)

	TextArea = Window.GetControl (5)
	if Identify:
		NameLabel.SetText (Item['ItemNameIdentified'])
		TextArea.SetText (Item['ItemDescIdentified'])
	else:
		NameLabel.SetText (Item['ItemName'])
		TextArea.SetText (Item['ItemDesc'])

	#Done
	Button = Window.GetControl (4)
	Button.SetText (11973)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, ErrorDone)

	# hide the empty button
	if GUICommon.GameIsBG2() or GUICommon.GameIsIWD2():
		Window.DeleteControl (9)

	Window.ShowModal (MODAL_SHADOW_GRAY)
	return

def UpdateStoreStealWindow ():
	global Store, inventory_slots

	Window = StoreStealWindow

	#reget store in case of a change
	Store = GemRB.GetStore ()
	LeftCount = Store['StoreItemCount']
	ScrollBar = Window.GetControl (9)
	ScrollBar.SetVarAssoc ("LeftTopIndex", LeftCount-ItemButtonCount+1)

	pc = GetPC()
	inventory_slots = GemRB.GetSlots (pc, SLOT_INVENTORY)
	RightCount = len(inventory_slots)
	ScrollBar = Window.GetControl (10)
	ScrollBar.SetVarAssoc ("RightTopIndex", RightCount-ItemButtonCount+1)
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
		GemRB.PlaySound(DEF_STOLEN)
		UpdateStoreStealWindow ()
	else:
		GemRB.StealFailed ()
		CloseStoreWindow ()
	return

def RedrawStoreStealWindow ():
	Window = StoreStealWindow

	UpdateStoreCommon (Window, 0x10000002, 0x10000027, 0x10000023)
	LeftTopIndex = GemRB.GetVar ("LeftTopIndex")
	LeftIndex = GemRB.GetVar ("LeftIndex")
	RightTopIndex = GemRB.GetVar ("RightTopIndex")
	RightIndex = GemRB.GetVar ("RightIndex")
	idx = [ LeftTopIndex, RightTopIndex, LeftIndex, RightIndex ]
	LeftCount = Store['StoreItemCount']
	pc = GemRB.GameGetSelectedPCSingle ()
	RightCount = len(inventory_slots)
	for i in range (ItemButtonCount):
		Slot = GemRB.GetStoreItem (i+LeftTopIndex)
		Button = Window.GetControl (i+4)
		Label = Window.GetControl (0x1000000f+i)
		Button.SetVarAssoc ("LeftIndex", LeftTopIndex+i)
		SetupItems (pc, Slot, Button, Label, i, ITEM_STORE, idx, 1)

		if i+RightTopIndex<RightCount:
			Slot = GemRB.GetSlotItem (pc, inventory_slots[i+RightTopIndex])
		else:
			Slot = None
		Button = Window.GetControl (i+11)
		Label = Window.GetControl (0x10000019+i)
		Button.SetVarAssoc ("RightIndex", RightTopIndex+i)
		SetupItems (pc, Slot, Button, Label, i, ITEM_PC, idx, 1)

	selected_count = 0
	for i in range (LeftCount):
		Flags = GemRB.IsValidStoreItem (pc, i, ITEM_STORE)
		if Flags & SHOP_SELECT:
			selected_count += 1

	# also disable the button if the inventory is full
	if LeftIndex>=0 and selected_count <= len(GemRB.GetSlots (pc, SLOT_INVENTORY, -1)):
		LeftButton.SetState (IE_GUI_BUTTON_ENABLED)
	else:
		LeftButton.SetState (IE_GUI_BUTTON_DISABLED)
	return

def SetupItems (pc, Slot, Button, Label, i, type, idx, steal=0):
	if Slot == None:
		Button.SetState (IE_GUI_BUTTON_DISABLED)
		Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_OR)
		Button.SetFlags (IE_GUI_BUTTON_PICTURE, OP_NAND)
		Label.SetText ("")
		Button.SetText ("")
	else:
		LeftTopIndex = idx[0]
		RightTopIndex = idx[1]
		LeftIndex = idx[2]

		Item = GemRB.GetItem (Slot['ItemResRef'])
		Button.SetItemIcon (Slot['ItemResRef'], 0)
		if Item['MaxStackAmount']>1:
			Button.SetText ( str(Slot['Usages0']) )
		else:
			Button.SetText ("")
		Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_NAND)
		Button.SetFlags (IE_GUI_BUTTON_PICTURE, OP_OR)

		if type == ITEM_STORE:
			Price = GetRealPrice (pc, "buy", Item, Slot)
			Flags = GemRB.IsValidStoreItem (pc, i+LeftTopIndex, type)
			if steal:
				Button.SetState (IE_GUI_BUTTON_ENABLED)
			else:
				if Flags & SHOP_BUY:
					if Flags & SHOP_SELECT:
						Button.SetState (IE_GUI_BUTTON_SELECTED)
					else:
						Button.SetState (IE_GUI_BUTTON_ENABLED)
				else:
					Button.SetState (IE_GUI_BUTTON_DISABLED)

				if not Inventory:
					Price = GetRealPrice (pc, "sell", Item, Slot)
					if Price <= 0:
						Price = 1
		else:
			Flags = GemRB.IsValidStoreItem (pc, inventory_slots[i+RightTopIndex], type)
			if Flags & SHOP_STEAL:
				if LeftIndex == LeftTopIndex + i:
					Button.SetState (IE_GUI_BUTTON_SELECTED)
				else:
					Button.SetState (IE_GUI_BUTTON_ENABLED)
			else:
				Button.SetState (IE_GUI_BUTTON_DISABLED)

			if Inventory:
				Price = 1
			else:
				Price = GetRealPrice (pc, "buy", Item, Slot)

			if (Price>0) and (Flags & SHOP_SELL):
				if Flags & SHOP_SELECT:
					Button.SetState (IE_GUI_BUTTON_SELECTED)
				else:
					Button.SetState (IE_GUI_BUTTON_ENABLED)
			else:
				Button.SetState (IE_GUI_BUTTON_DISABLED)

		if Flags & SHOP_ID:
			Name = GemRB.GetString (Item['ItemName'])
			Button.EnableBorder (0, 1)
			if not steal and type == ITEM_PC:
				Price = 1
		else:
			Name = GemRB.GetString (Item['ItemNameIdentified'])
			Button.EnableBorder (0, 0)

		GemRB.SetToken ("ITEMNAME", Name)
		if Inventory:
			if GUICommon.GameIsIWD1() or GUICommon.GameIsIWD2():
				LabelText = GemRB.GetString (24890)
			elif GUICommon.GameIsBG2():
				LabelText = GemRB.GetString (28337)
			else:
				LabelText = ""
		else:
			GemRB.SetToken ("ITEMCOST", str(Price) )
			LabelText = GemRB.GetString(10162)
		if type == ITEM_STORE:
			if steal:
				LabelText = Name
			elif Slot["Amount"] != -1:
				LabelText = LabelText + " (" + str(Slot["Amount"]) + ")"
		Label.SetText (LabelText)
	return

def GetRealPrice (pc, mode, Item, Slot):
	# get the base from the item
	price = Item['Price']

	if Item['MaxCharge']>0:
		price = price * Slot['Usages0'] / Item['MaxCharge']

	# modifier from store properties (in percent)
	if mode == "buy":
		mod = Store['BuyMarkup']
		if GemRB.HasFeat(pc, FEAT_MERCANTILE_BACKGROUND):
			mod -= 5
	else:
		mod = Store['SellMarkup']
		if GemRB.HasFeat(pc, FEAT_MERCANTILE_BACKGROUND):
			mod += 5

	# depreciation works like this:
	# - if you sell the item the first time, SellMarkup is used;
	# - if you sell the item the second time, SellMarkup-DepreciationRate is used;
	# - if you sell the item any more times, SellMarkup-2*DepreciationRate is used.
	# If the storekeep has an infinite amount of the item, only SellMarkup is used.
	# The amount of items sold at the same time doesn't matter! Selling three bows
	# separately will produce less gold then selling them at the same time.
	# We don't care who is the seller, so if the store already has 2 items, there'll be no gain
	if mode == "buy":
		count = GemRB.FindStoreItem (Slot["ItemResRef"])
		if count:
			oc = count
			if count > 2:
				count = 2
			mod -= count * Store['Depreciation']
	else:
		# charisma modifier (in percent)
		mod += GemRB.GetAbilityBonus (IE_CHR, GemRB.GetPlayerStat (BarteringPC, IE_CHR) - 1, 0)

		# reputation modifier (in percent, but absolute)
		mod = mod * RepModTable.GetValue (0, GemRB.GameGetReputation()/10 - 1) / 100

	return price * mod / 100

def UpdateStoreDonateWindow ():
	Window = StoreDonateWindow

	UpdateStoreCommon (Window, 0x10000007, 0, 0x10000008)
	Field = Window.GetControl (5)
	donation = int("0"+Field.QueryText ())
	gold = GemRB.GameGetPartyGold ()
	if donation>gold:
		donation = gold
		Field.SetText (str(gold) )

	Button = Window.GetControl (3)
	if donation:
		Button.SetState (IE_GUI_BUTTON_ENABLED)
	else:
		Button.SetState (IE_GUI_BUTTON_DISABLED)
	return

def IncrementDonation ():
	Window = StoreDonateWindow

	Field = Window.GetControl (5)
	donation = int("0"+Field.QueryText ())
	if donation<GemRB.GameGetPartyGold ():
		Field.SetText (str(donation+1) )
	else:
		Field.SetText (str(GemRB.GameGetPartyGold ()) )
	UpdateStoreDonateWindow ()
	return

def DecrementDonation ():
	Window = StoreDonateWindow

	Field = Window.GetControl (5)
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

	Button = Window.GetControl (10)
	Button.SetAnimation ("DONATE")

	Field = Window.GetControl (5)
	donation = int("0"+Field.QueryText ())
	GemRB.GameSetPartyGold (GemRB.GameGetPartyGold ()-donation)
	if GemRB.IncreaseReputation (donation):
		TextArea.Append (10468, -1)
		GemRB.PlaySound (DEF_DONATE1)
		UpdateStoreDonateWindow ()
		return

	TextArea.Append (10469, -1)
	GemRB.PlaySound (DEF_DONATE2)
	UpdateStoreDonateWindow ()
	return

def UpdateStoreHealWindow ():
	Window = StoreHealWindow

	UpdateStoreCommon (Window, 0x10000000, 0, 0x10000001)
	TopIndex = GemRB.GetVar ("TopIndex")
	Index = GemRB.GetVar ("Index")
	pc = GemRB.GameGetSelectedPCSingle ()
	for i in range (ItemButtonCount):
		Cure = GemRB.GetStoreCure (TopIndex+i)

		Button = Window.GetControl (i+8)
		Label = Window.GetControl (0x1000000c+i)
		Button.SetVarAssoc ("Index", TopIndex+i)
		if Cure:
			Spell = GemRB.GetSpell (Cure['CureResRef'])
			Button.SetSpellIcon (Cure['CureResRef'], 1)
			Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_NAND)
			Button.SetFlags (IE_GUI_BUTTON_PICTURE, OP_OR)
			dead = GemRB.GetPlayerStat (pc, IE_STATE_ID) & STATE_DEAD
			# toggle raise dead/resurrect based on state
			# unfortunately the flags are not set properly in iwd
			if (dead and Spell["SpellTargetType"] != 3) or \
			   (not dead and Spell["SpellTargetType"] == 3): # 3 - non-living
				# locked and shaded
				Button.SetState (IE_GUI_BUTTON_DISABLED)
				Button.SetBorder (0, 0,0, 0,0, 200,0,0,100, 1,1)
			else:
				Button.SetState (IE_GUI_BUTTON_ENABLED)
				Button.SetBorder (0, 0,0, 0,0, 0,0,0,0, 0,0)

			GemRB.SetToken ("ITEMNAME", GemRB.GetString (Spell['SpellName']))
			GemRB.SetToken ("ITEMCOST", str(Cure['Price']) )
			Label.SetText (10162)
		else:
			Button.SetState (IE_GUI_BUTTON_DISABLED)
			Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_OR)
			Button.SetFlags (IE_GUI_BUTTON_PICTURE, OP_NAND)
			Button.SetBorder (0, 0,0, 0,0, 0,0,0,0, 0,0)
			Label.SetText ("")

		if TopIndex+i==Index:
			TextArea = Window.GetControl (23)
			TextArea.SetText (Cure['Description'])
			Label = Window.GetControl (0x10000003)
			Label.SetText (str(Cure['Price']) )
			Button = Window.GetControl (5)
			Button.SetState (IE_GUI_BUTTON_ENABLED)
	return

def InfoHealWindow ():
	global MessageWindow

	UpdateStoreHealWindow ()
	Index = GemRB.GetVar ("Index")
	Cure = GemRB.GetStoreCure (Index)
	Spell = GemRB.GetSpell (Cure['CureResRef'])

	MessageWindow = Window = GemRB.LoadWindow (14)

	Label = Window.GetControl (0x10000000)
	Label.SetText (Spell['SpellName'])

	Button = Window.GetControl (2)
	Button.SetSpellIcon (Cure['CureResRef'], 1)

	TextArea = Window.GetControl (3)
	TextArea.SetText (Spell['SpellDesc'])

	#Done
	Button = Window.GetControl (5)
	Button.SetText (11973)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, ErrorDone)

	Window.ShowModal (MODAL_SHADOW_GRAY)
	return

def BuyHeal ():
	Index = GemRB.GetVar ("Index")
	Cure = GemRB.GetStoreCure (Index)
	gold = GemRB.GameGetPartyGold ()
	if gold < Cure['Price']:
		ErrorWindow (11048)
		return

	GemRB.GameSetPartyGold (gold-Cure['Price'])
	pc = GemRB.GameGetSelectedPCSingle ()
	GemRB.ApplySpell (pc, Cure['CureResRef'], pc)
	if SpellTable:
		sound = SpellTable.GetValue(Cure['CureResRef'], "SOUND")
	else:
		#if there is no table, simply use the spell's own completion sound
		Spell = GemRB.GetSpell(Cure['CureResRef'])
		sound = Spell['Completion']
	GemRB.PlaySound (sound)
	UpdateStoreHealWindow ()
	return

def UpdateStoreRumourWindow ():
	Window = StoreRumourWindow

	UpdateStoreCommon (Window, 0x10000011, 0, 0x10000012)
	TopIndex = GemRB.GetVar ("TopIndex")
	for i in range (5):
		Drink = GemRB.GetStoreDrink (TopIndex+i)
		Button = Window.GetControl (i)
		Button.SetVarAssoc ("Index", i)
		if Drink:
			GemRB.SetToken ("ITEMNAME", GemRB.GetString (Drink['DrinkName']))
			GemRB.SetToken ("ITEMCOST", str(Drink['Price']) )
			Button.SetText (10162)
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
		TextArea.Append (10832, -1)
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
	TextArea.Append (text, -1)
	GemRB.PlaySound (DEF_DRUNK)
	UpdateStoreRumourWindow ()
	return

def UpdateStoreRentWindow ():
	global RentIndex

	Window = StoreRentWindow
	UpdateStoreCommon (Window, 0x10000008, 0, 0x10000009)
	RentIndex = GemRB.GetVar ("RentIndex")
	Button = Window.GetControl (11)
	Label = Window.GetControl (0x1000000d)
	if RentIndex>=0:
		TextArea = Window.GetControl (12)
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
	# TODO: change dream back to 1 when we can handle restoring the gui
	GemRB.RestParty (13, -1, RentIndex+1)
	if RentConfirmWindow:
		RentConfirmWindow.Unload ()
	Window = StoreRentWindow
	TextArea = Window.GetControl (12)
	#is there any way to change this???
	GemRB.SetToken ("HOUR", "8")
	GemRB.SetToken ("HP", "%d"%(RentIndex+1))
	TextArea.SetText (16476)
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
		ErrorWindow (11051)
		return

	RentConfirmWindow = Window = GemRB.LoadWindow (11)
	#confirm
	Button = Window.GetControl (0)
	Button.SetText (17199)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, RentConfirm)
	Button.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)

	#deny
	Button = Window.GetControl (1)
	Button.SetText (13727)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, RentDeny)
	Button.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)

	#textarea
	TextArea = Window.GetControl (3)
	TextArea.SetText (15358)

	Window.ShowModal (MODAL_SHADOW_GRAY)
	return

def ErrorWindow (strref):
	global MessageWindow

	MessageWindow = Window = GemRB.LoadWindow (10)

	TextArea = Window.GetControl (3)
	TextArea.SetText (strref)

	#done
	Button = Window.GetControl (0)
	Button.SetText (11973)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, ErrorDone)

	Window.ShowModal (MODAL_SHADOW_GRAY)
	return

def ErrorDone ():
	if MessageWindow:
		MessageWindow.Unload ()
	return

###################################################
# End of file GUISTORE.py
