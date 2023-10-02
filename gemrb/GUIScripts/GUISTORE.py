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
import GameCheck
import GUICommon
import GUICommonWindows
from CommonWindow import AddScrollbarProxy
from GUIDefines import *
from ie_stats import *
from ie_slots import *
from ie_sounds import *
from ie_feats import FEAT_MERCANTILE_BACKGROUND
import CommonTables

StoreWindow = None

RentConfirmWindow = None
LeftButton = None
RightButton = None
CureTable = None
RaiseDeadTable = None

ITEM_PC    = 0
ITEM_STORE = 1
ITEM_BAG   = 2

STORE_MAIN = 0
STORE_BAG  = 1

Inventory = None
Store = None
Bag = None
inventory_slots = ()
total_price = 0
total_income = 0
if GameCheck.IsIWD2():
	ItemButtonCount = 6
else:
	ItemButtonCount = 4
RepModTable = None
SpellTable = None
PreviousPC = 0
MaxAmount = 0
BuySum = 0
SellSum = 0

UnusableColor = {'r' : 255, 'g' : 128, 'b' : 128, 'a' : 64}

if GameCheck.IsBG2():
	IDColor = {'r' : 0, 'g' : 0, 'b' : 128, 'a' : 160}
elif GameCheck.IsPST():
	IDColor = {'r' : 128, 'g' : 0, 'b' : 0, 'a' : 100}
else:
	IDColor = {'r' : 32, 'g' : 32, 'b' : 192, 'a' : 128}

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

storetips = (14288,14292,14291,12138,15013,14289,14287)
roomdesc = (17389,17517,17521,17519)
roomnames = (14294, 14295, 14296, 14297)
windowIDs = { 'shop' : 2, 'ident' : 4, 'steal' : 6, 'donate' : 9, 'heal' : 5, 'rumour' : 8, 'rent' : 7, 'iteminfo' : 12,  'cureinfo' : 14, 'rentconfirm' : 11, 'error' : 10 }
strrefs = { 'done' : 11973, 'cancel' : 13727, 'buy' : 13703, 'sell' : 13704, 'identify' : 14133, 'steal' : 14179, 'donate' : 15101, 'heal' : 8786, 'rumors' : 14144, 'rent' : 14293, 'itemnamecost' : 10162, 'restedfor' : 16476, 'rest' : 17199, 'confirmrest' : 15358, 'itemstoocostly' : 11047, 'spelltoocostly' : 11048, 'drinktoocostly' : 11049, 'toodrunk' : 10832, 'idtoocostly' : 11050, 'roomtoocostly' : 11051, 'donorgood' : 10468, 'donorfail' : 10469 }
store_funcs = None
StoreButtonCount = 4
if GameCheck.IsIWD1():
	# no bam for bags
	storebams = ("STORSTOR","STORTVRN","STORINN","STORTMPL","STORSTOR","STORSTOR")
elif GameCheck.IsPST():
	#             Buy/Sell Identify   Steal      Aid        Donate    Drink     Rent
	storebams = ("SSBBS", "SSBIDNT", "SSBSTEL", "SSBHEAL", "SSBDON", "SSBRMR", "SSBRENT")
	storetips =  (44970,   44971,     44972,     67294,     45121,    45119,    45120)
	roomdesc = (66865, 66866, 66867, 66868)
	roomnames = (45308, 45310, 45313, 45316)
	windowIDs = { 'shop' : 4, 'ident' : 5, 'steal' : 7, 'donate' : 10, 'heal' : 6, 'rumour' : 9, 'rent' : 8, 'iteminfo' : 13,  'cureinfo' : 13, 'rentconfirm' : 12, 'error' : 11 }
	strrefs = { 'done' : 1403, 'cancel' : 4196, 'identify' : 44971, 'buy' : 45303, 'sell' : 45304, 'steal' : 45305, 'rent' : 45306, 'donate' : 45307, 'heal' : 8836, 'rumors' : 0, 'itemnamecost' : 45374, 'restedfor' : 19262, 'rest' : 4242, 'confirmrest' : 4241, 'itemstoocostly' : 50081, 'spelltoocostly' : 50082, 'drinktoocostly' : 50083, 'toodrunk' : 0, 'idtoocostly' : 50084, 'roomtoocostly' : 50085, 'donorgood' : 26914, 'donorfail' : 26914}
	StoreButtonCount = 7
else:
	storebams = ("STORSTOR","STORTVRN","STORINN","STORTMPL","STORBAG","STORBAG")
	if GameCheck.IsBG1 ():
		strrefs["heal"] = strrefs["buy"]
Buttons = [-1] * StoreButtonCount

# center the windows according to game needs and available space
StoreWindowPlacement = WINDOW_HCENTER | WINDOW_VCENTER
ResolutionH = GemRB.GetSystemVariable (SV_HEIGHT)
# iwd2 keeps everything displayed, so we only vertically center when
# there's enough room to fit bottom windows (934 = (600-433)*2 + 600)
if ResolutionH < 600 or (GameCheck.IsIWD2 () and ResolutionH < 934):
	StoreWindowPlacement = WINDOW_HCENTER | WINDOW_TOP

def SetupScrollBar(ScrollBar, VarName, Count, Proxy, Callback):
	ScrollBar.SetVarAssoc(VarName, GemRB.GetVar(VarName) or 0, 0, max(0, Count))
	ScrollBar.OnChange(Callback) # must come after SetVarAssoc
	if Proxy:
		AddScrollbarProxy(ScrollBar.Window, ScrollBar, Proxy)
	return ScrollBar

def CloseStoreWindow ():
	global StoreWindow, CureTable
	
	if StoreWindow:
		StoreWindow.Close ()

	global Bag
	if Bag:
		UnselectBag ()
		GemRB.CloseRighthandStore ()
		Bag = None

	GemRB.LeaveStore ()

	CureTable = None

	if Inventory:
		# reopen the inventory
		import GUIINV
		GUIINV.OpenInventoryWindow ()
	else:
		GemRB.GamePause (0, 3)
		GUICommonWindows.SetSelectionChangeHandler( None )
		GUICommonWindows.CloseTopWindow()
	return

def PositionStoreWinRelativeTo(win):
	storewin = GemRB.GetView("WIN_STORE")
	if not storewin:
		return
	
	# ignore the esc key, normally it would be fine, but we have multiple windows that join together.
	EscButton = win.CreateButton (99, 0, 0, 0, 0);
	EscButton.MakeEscape ()

	winframe = win.GetFrame()
	storeframe = storewin.GetFrame()

	if GameCheck.IsIWD2():
		win.SetFlags(WF_ALPHA_CHANNEL, OP_OR)
		storeframe['x'] = winframe['x']
		storeframe['y'] = winframe['y']
	else:
		storeframe['x'] = winframe['x']
		storeframe['y'] = winframe['y'] + winframe['h']

	storewin.SetFrame(storeframe)

def OpenStoreWindow ():
	global Store
	global StoreWindow
	global store_funcs
	global SpellTable, RepModTable
	global Inventory
	global CureTable

	Inventory = GemRB.GetView ("WIN_INV") is not None

	def ChangeStoreView(func):
		# WIN_TOP needs to be focused for us to change it (see CreateTopWinLoader)
		top = GemRB.GetView("WIN_TOP")
		if top:
			top.Focus()
		return func()
	
	store_funcs = (lambda: ChangeStoreView(OpenStoreShoppingWindow),
					lambda: ChangeStoreView(OpenStoreIdentifyWindow),
					lambda: ChangeStoreView(OpenStoreStealWindow),
					lambda: ChangeStoreView(OpenStoreHealWindow),
					lambda: ChangeStoreView(OpenStoreDonateWindow),
					lambda: ChangeStoreView(OpenStoreRumourWindow),
					lambda: ChangeStoreView(OpenStoreRentWindow) )
	
	Store = GemRB.GetStore ()
	#based on shop type, these buttons will change
	store_buttons = Store['StoreButtons']
	
	# we have to load the "top win" first
	# the code doesn't permit a "normal win" to sit between 2 "top win"
	# normal windows are considered children of the "top win" below them
	topwin = store_funcs[store_buttons[0]] ()

	if GameCheck.IsPST():
		CureTable = GemRB.LoadTable("speldesc") #additional info not supported by core
	else:
		if not GameCheck.IsIWD2(): # present from before, resulting in a 10x price increase
			RepModTable = GemRB.LoadTable ("repmodst")
		SpellTable = GemRB.LoadTable ("storespl", 1)

	if not Inventory:
		# pause the game, so we don't get interrupted
		GemRB.GamePause (1, 3)

	GemRB.SetVar ("Action", 0)

	StoreWindow = Window = GemRB.LoadWindow (3, "GUISTORE", StoreWindowPlacement)
	StoreWindow.SetFlags (IE_GUI_VIEW_IGNORE_EVENTS, OP_OR)
	if GameCheck.IsIWD2():
		# IWD2 has weird overlay windows
		StoreWindow.SetFlags(WF_ALPHA_CHANNEL, OP_OR)
	Window.AddAlias("WIN_STORE")

	PositionStoreWinRelativeTo(topwin)

	# Done
	Button = Window.GetControl (0)
	Button.SetText (strrefs["done"])
	Button.OnPress (CloseStoreWindow)
	Button.MakeEscape()

	#Store type icon
	if not GameCheck.IsIWD2():
		Button = Window.GetControl (5)
		Button.SetSprites (storebams[Store['StoreType']],0,0,0,0,0)

	for i in range (StoreButtonCount):
		Buttons[i] = Button = Window.GetControl (i+1)
		Action = store_buttons[i]
		Button.SetVarAssoc ("Action", i)
		# iwd2 has no steal window
		if GameCheck.IsIWD2() and Action == 2:
			Action = None
		if Action is not None:
			Button.SetFlags (IE_GUI_BUTTON_RADIOBUTTON, OP_OR)
			if GameCheck.IsIWD1() or GameCheck.IsIWD2():
				Button.SetSprites ("GUISTBBC", Action, 1,2,0,0)
			elif GameCheck.IsPST():
				Button.SetSprites (storebams[Action], 0, 0,1,2,0)
			else:
				Button.SetSprites ("GUISTBBC", Action, 0,1,2,0)
			Button.SetTooltip (storetips[Action])
			Button.OnPress (store_funcs[Action])
			Button.SetState (IE_GUI_BUTTON_ENABLED)
		else:
			Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_OR)
			Button.SetTooltip ("")
			Button.OnPress (None)
			Button.SetState (IE_GUI_BUTTON_DISABLED)

	return

def InitStoreShoppingWindow (Window):
	global LeftButton, RightButton, Inventory

	Window.AddAlias('WINSHOP')
	PositionStoreWinRelativeTo(Window)

	if GameCheck.IsPST():
		# remap controls, so we can avoid too many ifdefs
		# strings must be able to fit into a resref (<= 8 char)
		aliases = { 'STOLBTN' : 0, 'STORBTN' : 1,
					'STOSBARR' : 16, 'STOSBARL' : 7,

					'PRICEB' : 0x10000003, 'PRICES' : 0x10000004,
					'STOTITLE' : 0x10000001, 'STONAME' : 0x10000005, 'STOGOLD' : 0x10000002,
					}

		Window.AliasControls ({'LBTN' + str(x) : x+8 for x in range(ItemButtonCount)})
		Window.AliasControls ({'RBTN' + str(x) : x+17 for x in range(ItemButtonCount)} )
		Window.AliasControls ({'RLBL' + str(x) : x+0x10000014 for x in range(ItemButtonCount)} )
		Window.AliasControls ({'LLBL' + str(x) : x+0x1000000b for x in range(ItemButtonCount)} )

	else:
		aliases = { 'STOLBTN' : 2, 'STORBTN' : 3,
					'STOSBARR' : 12, 'STOSBARL' : 11,

					'PRICEB' : 0x1000002b, 'PRICES' : 0x1000002c,
					'STOTITLE' : 0x10000003, 'STONAME' : 0x1000002e, 'STOGOLD' : 0x1000002a,
					}

		Window.AliasControls ({'LBTN' + str(x) : x+5 for x in range(ItemButtonCount)})
		Window.AliasControls ({'RBTN' + str(x) : x+13 for x in range(ItemButtonCount)} )
		Window.AliasControls ({'RLBL' + str(x) : x+0x1000001e for x in range(ItemButtonCount)} )
		Window.AliasControls ({'LLBL' + str(x) : x+0x10000012 for x in range(ItemButtonCount)} )


	Window.AliasControls (aliases)

	if Inventory:
		# Title
		Label = Window.GetControl (0xfffffff)
		if GameCheck.IsIWD1() or GameCheck.IsIWD2():
			Label.SetText (26291)
		elif GameCheck.IsBG2():
			Label.SetText (51881)
		else:
			Label.SetText ("")

		# buy price ...
		Label = Window.GetControl (0x1000002f)
		Label.SetText ("")
		# sell price ...
		Label = Window.GetControl (0x10000030)
		Label.SetText ("")

	# buy price ...
	Label = Window.GetControlAlias ('PRICEB')
	Label.SetText ("")
	# sell price ...
	Label = Window.GetControlAlias ('PRICES')
	Label.SetText ("")

	for i in range (ItemButtonCount):
		Button = Window.GetControlAlias ('LBTN' + str(i))
		Button.SetBorder (0, IDColor, 0, 1)
		Button.SetBorder (1, UnusableColor, 0, 1)
		Button.SetVarAssoc ("LeftIndex", i)
		Button.OnPress (SelectBuy)
		Button.OnDoublePress (lambda: OpenItemAmountWindow(Window))
		Button.OnRightPress (InfoLeftWindow)
		Button.SetFont ("NUMBER")
		Button.SetFlags (IE_GUI_BUTTON_ALIGN_RIGHT | IE_GUI_BUTTON_ALIGN_BOTTOM | IE_GUI_BUTTON_PICTURE, OP_OR)

		Button = Window.GetControlAlias ('RBTN' + str(i))
		Button.SetVarAssoc ("RightIndex", i)
		if GameCheck.IsBG2():
			Button.SetSprites ("GUIBTBUT", 0, 0,1,2,5)

		Button.SetBorder (0, IDColor, 0, 1)
		Button.SetBorder (1, UnusableColor, 0, 1)
		if Store['StoreType'] != 3: # can't sell to temples
			Button.OnPress (SelectSell)
			Button.OnDoublePress (lambda: OpenBag(Window))
		Button.OnRightPress (InfoRightWindow)
		Button.SetFont ("NUMBER")
		Button.SetFlags (IE_GUI_BUTTON_ALIGN_RIGHT | IE_GUI_BUTTON_ALIGN_BOTTOM | IE_GUI_BUTTON_PICTURE, OP_OR)

	GemRB.SetVar ("RightIndex", 0)
	GemRB.SetVar ("LeftIndex", 0)
	UnselectNoRedraw ()

	# Buy
	LeftButton = Button = Window.GetControlAlias ('STOLBTN')
	if Inventory:
		if GameCheck.IsIWD1() or GameCheck.IsIWD2():
			Button.SetText (26287)
		elif GameCheck.IsBG2():
			Button.SetText (51882)
		else:
			Button.SetText ("")
		Button.OnPress (ToBackpackPressed)
	else:
		Button.SetText (strrefs["buy"])
		Button.OnPress (BuyPressed)

	# Sell
	RightButton = Button = Window.GetControlAlias ('STORBTN')
	if Inventory:
		if GameCheck.IsIWD1() or GameCheck.IsIWD2():
			Button.SetText (26288)
		elif GameCheck.IsBG2():
			Button.SetText (51883)
		else:
			Button.SetText ("")
		Button.OnPress (ToBagPressed)
	else:
		Button.SetText (strrefs["sell"])
		if Store['StoreType'] != 3: # can't sell to temples
			Button.OnPress (SellPressed)

	# inactive button (close container)
	if GameCheck.IsBG2():
		Button = Window.GetControl (50)
		Button.SetState (IE_GUI_BUTTON_LOCKED)
		Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_SET)
		Button.OnPress (lambda: CloseBag(Window))

	#backpack
	BackpackButton = Window.GetControl (44)
	if BackpackButton:
		BackpackButton.SetState (IE_GUI_BUTTON_LOCKED)

	# encumbrance
	if GameCheck.IsPST():
		Button = Window.GetControl (25)
		Button.SetState (IE_GUI_BUTTON_LOCKED)
		Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_SET)
		Button.SetFont ('NUMBER')
	else:
		BackpackButton.CreateLabel (0x10000043, "NUMBER", "0:",
			IE_FONT_ALIGN_LEFT|IE_FONT_ALIGN_TOP|IE_FONT_SINGLE_LINE)
		BackpackButton.CreateLabel (0x10000044, "NUMBER", "0:",
			IE_FONT_ALIGN_RIGHT|IE_FONT_ALIGN_BOTTOM|IE_FONT_SINGLE_LINE)

	return

def UpdateStoreShoppingWindow (Window):
	global Bag, Store, inventory_slots

	#reget store in case of a change
	Store = GemRB.GetStore ()
	pc = GetPC()
	Bag = GemRB.GetStore (STORE_BAG)

	callback = lambda: RedrawStoreShoppingWindow(Window)
	LeftCount = Store['StoreItemCount'] - ItemButtonCount
	# left scrollbar
	SetupScrollBar(Window.GetControlAlias('STOSBARL'), "LeftTopIndex", LeftCount, Window.GetControlAlias('LBTN0'), callback)

	if Bag:
		RightCount = Bag['StoreItemCount'] - ItemButtonCount
	else:
		inventory_slots = GemRB.GetSlots (pc, SLOT_INVENTORY)
		RightCount = len(inventory_slots) - ItemButtonCount

	# right scrollbar
	SetupScrollBar(Window.GetControlAlias('STOSBARR'), "RightTopIndex", RightCount, Window.GetControlAlias('RBTN0'), callback)

	RedrawStoreShoppingWindow (Window)
	return

ToggleStoreShoppingWindow = GUICommonWindows.CreateTopWinLoader(windowIDs["shop"], "GUISTORE", GUICommonWindows.ToggleWindow, InitStoreShoppingWindow, UpdateStoreShoppingWindow, False, StoreWindowPlacement)
OpenStoreShoppingWindow = GUICommonWindows.CreateTopWinLoader(windowIDs["shop"], "GUISTORE", GUICommonWindows.OpenWindowOnce, InitStoreShoppingWindow, UpdateStoreShoppingWindow, False, StoreWindowPlacement)

def InitStoreIdentifyWindow (Window):
	global LeftButton

	PositionStoreWinRelativeTo(Window)

	if GameCheck.IsPST():
		# remap controls, so we can avoid too many ifdefs
		Window.AliasControls ({ 'IDSBAR' : 5, 'IDLBTN' : 4, 'IDTA' : 14,
								'IDPRICE' : 0x10000001, 'STOGOLD' : 0x10000000,
								'STOTITLE' : 0x0fffffff, 'STONAME' : 0x10000002
							  } )
		Window.AliasControls ({'IDBTN' + str(x) : 6+x for x in range(ItemButtonCount)})
	else:
		Window.AliasControls ({ 'IDSBAR' : 7, 'IDLBTN' : 5, 'IDTA' : 23,
								'IDPRICE' : 0x10000003, 'STOGOLD' : 0x10000001,
								'STOTITLE' : 0x10000000, 'STONAME' : 0x10000005
							  } )
		Window.AliasControls ({'IDBTN' + str(x) : 8+x for x in range(ItemButtonCount)})

	TextArea = Window.GetControlAlias ('IDTA')
	TextArea.SetFlags (IE_GUI_TEXTAREA_AUTOSCROLL)

	# Identify
	LeftButton = Button = Window.GetControlAlias ('IDLBTN')
	Button.SetText (strrefs["identify"])
	Button.OnPress (lambda: IdentifyPressed (Window))
	Button.OnRightPress (InfoIdentifyWindow)

	# price ...
	Label = Window.GetControlAlias ('IDPRICE')
	Label.SetText ("0")

	# 8-11 item slots, 0x1000000c-f labels
	for i in range (ItemButtonCount):
		Button = Window.GetControlAlias ("IDBTN" + str(i))
		Button.SetVarAssoc ("Index", i)

		if GameCheck.IsIWD1() or GameCheck.IsIWD2():
			Button.SetSprites ("GUISTMSC", 0, 1,2,0,3)

		Button.SetBorder (0, IDColor, 0, 1)
		Button.OnPress (SelectID)
		Button.OnRightPress (InfoIdentifyWindow)
		Button.SetFont ("NUMBER")
		Button.SetFlags (IE_GUI_BUTTON_ALIGN_RIGHT|IE_GUI_BUTTON_ALIGN_BOTTOM, OP_OR)

	GemRB.SetVar ("Index", None)
	UnselectNoRedraw ()
	return

def UpdateStoreIdentifyWindow (Window):
	global inventory_slots

	pc = GetPC()
	inventory_slots = GemRB.GetSlots (pc, SLOT_INVENTORY)

	callback = lambda: RedrawStoreIdentifyWindow(Window)
	Count = len(inventory_slots) - ItemButtonCount
	ScrollBar = Window.GetControlAlias('IDSBAR')
	SetupScrollBar(Window.GetControlAlias('IDSBAR'), "TopIndex", Count, Window.GetControlAlias('IDBTN0'), callback)

	RedrawStoreIdentifyWindow (Window)
	return

ToggleStoreIdentifyWindow = GUICommonWindows.CreateTopWinLoader(windowIDs["ident"], "GUISTORE", GUICommonWindows.ToggleWindow, InitStoreIdentifyWindow, UpdateStoreIdentifyWindow, False, StoreWindowPlacement)
OpenStoreIdentifyWindow = GUICommonWindows.CreateTopWinLoader(windowIDs["ident"], "GUISTORE", GUICommonWindows.OpenWindowOnce, InitStoreIdentifyWindow, UpdateStoreIdentifyWindow, False, StoreWindowPlacement)

def InitStoreStealWindow (Window):
	global LeftButton

	PositionStoreWinRelativeTo(Window)

	if GameCheck.IsPST():
		# remap controls, so we can avoid too many ifdefs

		Window.AliasControls ({ 'STEAL' : 0, 'SWLSBAR' : 4, 'SWRSBAR' : 13,
								'STONAME' : 0x10000002, 'STOTITLE' : 0x10000000, 'STOGOLD' : 0x10000001
								} )

		Window.AliasControls ({'SWLBTN' + str(x) : x+5 for x in range(ItemButtonCount)} )
		Window.AliasControls ({'SWRBTN' + str(x) : x+14 for x in range(ItemButtonCount)} )
		Window.AliasControls ({'SWRLBL' + str(x) : x+0x10000011 for x in range(ItemButtonCount)} )
		Window.AliasControls ({'SWLLBL' + str(x) : x+0x10000008 for x in range(ItemButtonCount)} )
	else:
		Window.AliasControls ({ 'STEAL' : 1, 'SWLSBAR' : 9, 'SWRSBAR' : 10,
								'STONAME' : 0x10000027, 'STOTITLE' : 0x10000002, 'STOGOLD' : 0x10000023
							  } )

		Window.AliasControls ({'SWLBTN' + str(x) : x+4 for x in range(ItemButtonCount)} )
		Window.AliasControls ({'SWRBTN' + str(x) : x+11 for x in range(ItemButtonCount)} )
		Window.AliasControls ({'SWRLBL' + str(x) : x+0x10000019 for x in range(ItemButtonCount)} )
		Window.AliasControls ({'SWLLBL' + str(x) : x+0x1000000f for x in range(ItemButtonCount)} )

	for i in range (ItemButtonCount):
		Button = Window.GetControlAlias ('SWLBTN' + str(i))
		Button.SetVarAssoc ("LeftIndex", i)
		Button.SetBorder (0, IDColor, 0, 1)
		Button.SetBorder (1, UnusableColor, 0, 1)
		Button.OnRightPress (InfoLeftWindow)
		Button.OnPress (SelectSteal)
		Button.SetFont ("NUMBER")
		Button.SetFlags (IE_GUI_BUTTON_ALIGN_RIGHT | IE_GUI_BUTTON_ALIGN_BOTTOM | IE_GUI_BUTTON_PICTURE, OP_OR)

		Button = Window.GetControlAlias ('SWRBTN' + str(i))
		Button.SetVarAssoc ("RightIndex", i)
		Button.SetBorder (0, IDColor, 0, 1)
		Button.SetBorder (1, UnusableColor, 0, 1)
		Button.OnRightPress (InfoRightWindow)
		Button.SetFont ("NUMBER")
		Button.SetFlags (IE_GUI_BUTTON_ALIGN_RIGHT | IE_GUI_BUTTON_ALIGN_BOTTOM | IE_GUI_BUTTON_PICTURE, OP_OR)

	GemRB.SetVar ("RightIndex", None)
	GemRB.SetVar ("LeftIndex", None)
	UnselectNoRedraw ()

	# Steal
	LeftButton = Button = Window.GetControlAlias ('STEAL')
	Button.SetText (strrefs["steal"])
	Button.OnPress (StealPressed)

	Button = Window.GetControl (37)
	if Button:
		Button.SetState (IE_GUI_BUTTON_LOCKED)

	# encumbrance
	if GameCheck.IsPST():
		Button = Window.GetControl (22)
		Button.SetState (IE_GUI_BUTTON_LOCKED)
		Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_SET)
		Button.SetFont ('NUMBER')
	else:
		Button.CreateLabel (0x10000043, "NUMBER", "0:",
			IE_FONT_ALIGN_LEFT|IE_FONT_ALIGN_TOP|IE_FONT_SINGLE_LINE)
		Button.CreateLabel (0x10000044, "NUMBER", "0:",
			IE_FONT_ALIGN_RIGHT|IE_FONT_ALIGN_BOTTOM|IE_FONT_SINGLE_LINE)

	return

def UpdateStoreStealWindow (Window):
	global Store, inventory_slots

	#reget store in case of a change
	Store = GemRB.GetStore ()

	pc = GetPC()
	inventory_slots = GemRB.GetSlots (pc, SLOT_INVENTORY)

	# left scrollbar
	callback = lambda: RedrawStoreStealWindow(Window)
	LeftCount = Store['StoreItemCount'] - ItemButtonCount
	SetupScrollBar(Window.GetControlAlias('SWLSBAR'), "LeftTopIndex", LeftCount, Window.GetControlAlias('SWLBTN0'), callback)

	# right scrollbar
	RightCount = len(inventory_slots) - ItemButtonCount
	SetupScrollBar(Window.GetControlAlias('SWRSBAR'), "RightTopIndex", RightCount, Window.GetControlAlias('SWRBTN0'), callback)

	LeftButton.SetState (IE_GUI_BUTTON_DISABLED)
	RedrawStoreStealWindow (Window)
	return

ToggleStoreStealWindow = GUICommonWindows.CreateTopWinLoader(windowIDs["steal"], "GUISTORE", GUICommonWindows.ToggleWindow, InitStoreStealWindow, UpdateStoreStealWindow, False, StoreWindowPlacement)
OpenStoreStealWindow = GUICommonWindows.CreateTopWinLoader(windowIDs["steal"], "GUISTORE", GUICommonWindows.OpenWindowOnce, InitStoreStealWindow, UpdateStoreStealWindow, False, StoreWindowPlacement)

def InitStoreDonateWindow (Window):
	Window.AddAlias('WINDONAT')

	PositionStoreWinRelativeTo(Window)

	if GameCheck.IsPST():
		# remap controls, so we can avoid too many ifdefs
		Window.AliasControls ({ 'DONATE' : 2, 'STOTEDIT' : 3, 'STOPLUS' : 4,
								'STOMINUS' : 5, 'STOTITLE' : 0x10000005, 'STOGOLD' : 0x10000006
							  } )
	else:
		Window.AliasControls ({ 'DONATE' : 3, 'STOTEDIT' : 5, 'STOPLUS' : 6,
								'STOMINUS' : 7, 'STOTITLE' : 0x10000007, 'STOGOLD' : 0x10000008
							  } )

	# graphics
	Button = Window.GetControl (10)
	if Button:
		Button.SetFlags (IE_GUI_BUTTON_PICTURE, OP_OR)
		Button.SetState (IE_GUI_BUTTON_LOCKED)

	# Donate
	Button = Window.GetControlAlias ('DONATE')
	Button.SetText (strrefs["donate"])
	Button.OnPress (DonateGold)
	Button.MakeDefault()

	# Entry
	Field = Window.GetControlAlias ('STOTEDIT')
	Field.SetText ("0")
	Field.OnChange (lambda: UpdateStoreDonateWindow(Window))
	Field.SetFlags (IE_GUI_TEXTEDIT_ALPHACHARS, OP_NAND)
	Field.Focus()

	def SetItemAmount (Number):
		MaxAmount = GemRB.GameGetPartyGold()
		if Number < 0:
			Number = 0
		elif Number > MaxAmount:
			Number = MaxAmount
		Field.SetText (str(Number))
		Button = Window.GetControlAlias ("DONATE")
		Button.SetDisabled (not Number)

	# +
	Button = Window.GetControlAlias ('STOPLUS')
	Button.OnPress (lambda: SetItemAmount(Field.QueryInteger() + 1))
	Button.SetActionInterval (50)
	# -
	Button = Window.GetControlAlias ('STOMINUS')
	Button.OnPress (lambda: SetItemAmount(Field.QueryInteger() - 1))
	Button.SetActionInterval (50)
	return

def UpdateStoreDonateWindow (Window):
	UpdateStoreCommon (Window, "STOTITLE", None, "STOGOLD")
	Field = Window.GetControl (5)
	donation = int("0"+Field.QueryText ())
	gold = GemRB.GameGetPartyGold ()
	if donation>gold:
		donation = gold
		Field.SetText (str(gold) )

	Button = Window.GetControlAlias ("DONATE")
	Button.SetDisabled(not donation)
	return

ToggleStoreDonateWindow = GUICommonWindows.CreateTopWinLoader(windowIDs["donate"], "GUISTORE", GUICommonWindows.ToggleWindow, InitStoreDonateWindow, UpdateStoreDonateWindow, False, StoreWindowPlacement)
OpenStoreDonateWindow = GUICommonWindows.CreateTopWinLoader(windowIDs["donate"], "GUISTORE", GUICommonWindows.OpenWindowOnce, InitStoreDonateWindow, UpdateStoreDonateWindow, False, StoreWindowPlacement)

def InitStoreHealWindow (Window):
	Window.AddAlias('WINHEAL')

	PositionStoreWinRelativeTo(Window)

	if GameCheck.IsPST():
		# remap controls, so we can avoid too many ifdefs
		Window.AliasControls ({ 'HWSBAR' : 4, 'HEALBTN' : 3, 'HWTA' : 13,
								 'HWPRICE' : 0x10000001, 'STOGOLD' : 0x10000000, 'STOTITLE' : 0x0fffffff
								 } )
		Window.AliasControls ({'HWLBTN' + str(x) : x+5 for x in range(ItemButtonCount)} )
	else:
		Window.AliasControls ({ 'HWSBAR' : 7, 'HEALBTN' : 5, 'HWTA' : 23,
								 'HWPRICE' : 0x10000003, 'STOGOLD' : 0x10000001, 'STOTITLE' : 0x10000000
								 } )
		Window.AliasControls ({'HWLBTN' + str(x) : x+8 for x in range(ItemButtonCount)})

	#spell buttons
	for i in range (ItemButtonCount):
		Button = Window.GetControlAlias ('HWLBTN' + str(i))
		Button.SetFlags (IE_GUI_BUTTON_RADIOBUTTON, OP_OR)
		Button.OnPress (lambda: UpdateStoreHealWindow(Window))
		Button.OnRightPress (InfoHealWindow)
		Button.SetVarAssoc ("Index", i)

	UnselectNoRedraw ()

	# price tag
	Label = Window.GetControlAlias ('HWPRICE')
	Label.SetText ("0")

	# Heal
	Button = Window.GetControlAlias ('HEALBTN')
	Button.SetText (strrefs["heal"])
	Button.OnPress (BuyHeal)
	Button.SetDisabled (True)

	callback = lambda: UpdateStoreHealWindow(Window)
	Count = Store['StoreCureCount'] - 4
	SetupScrollBar(Window.GetControlAlias('HWSBAR'), "TopIndex", Count, Window.GetControlAlias('HWLBTN0'), callback)

	return

def UpdateStoreHealWindow (Window):
	UpdateStoreCommon (Window, "STOTITLE", None, "STOGOLD")
	TopIndex = GemRB.GetVar ("TopIndex")
	Index = GemRB.GetVar ("Index") + TopIndex
	pc = GemRB.GameGetSelectedPCSingle ()
	labelOffset = 0x1000000c
	if GameCheck.IsIWD2():
		labelOffset += 1 # grrr
	elif GameCheck.IsPST():
		labelOffset = 0x10000008

	for i in range (ItemButtonCount):
		Cure = GemRB.GetStoreCure (TopIndex+i)

		Button = Window.GetControlAlias ('HWLBTN' + str(i))
		Label = Window.GetControl (labelOffset+i)
		if Cure:
			Spell = GemRB.GetSpell (Cure['CureResRef'])
			Button.SetSpellIcon (Cure['CureResRef'], 1)
			Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_NAND)
			Button.SetFlags (IE_GUI_BUTTON_PICTURE, OP_OR)
			dead = GemRB.GetPlayerStat (pc, IE_STATE_ID) & STATE_DEAD
			# toggle raise dead/resurrect based on state
			# unfortunately the flags are not set properly in iwd
			if not GameCheck.IsIWD1() and not GameCheck.IsPST() and (  # 3 - non-living
							 (dead and Spell["SpellTargetType"] != 3) or \
							 (not dead and Spell["SpellTargetType"] == 3)):
				# locked and shaded
				Button.SetDisabled (True)
				color = {'r' : 200, 'g' : 0, 'b' : 0, 'a' : 100}
				Button.SetBorder (0, color, 1,1)
			else:
				Button.SetDisabled (False)
				Button.SetBorder (0, None, 0,0)

			CurePrice = str(GetRealCurePrice (Cure))
			GemRB.SetToken ("ITEMNAME", GemRB.GetString (Spell['SpellName']))
			GemRB.SetToken ("ITEMCOST", CurePrice)
			Label.SetText (strrefs["itemnamecost"])

			if TopIndex + i == Index:
				TextArea = Window.GetControlAlias ("HWTA")
				TextArea.SetText (Cure['Description'])
				Label = Window.GetControlAlias ("HWPRICE")
				Label.SetText (CurePrice)
				Button = Window.GetControlAlias ("HEALBTN")
				Button.SetDisabled (False)
		else:
			Button.SetDisabled (True)
			Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_OR)
			Button.SetFlags (IE_GUI_BUTTON_PICTURE, OP_NAND)
			Button.SetBorder (0, None, 0,0)
			Label.SetText ("")

	return

ToggleStoreHealWindow = GUICommonWindows.CreateTopWinLoader(windowIDs["heal"], "GUISTORE", GUICommonWindows.ToggleWindow, InitStoreHealWindow, UpdateStoreHealWindow, False, StoreWindowPlacement)
OpenStoreHealWindow = GUICommonWindows.CreateTopWinLoader(windowIDs["heal"], "GUISTORE", GUICommonWindows.OpenWindowOnce, InitStoreHealWindow, UpdateStoreHealWindow, False, StoreWindowPlacement)

def InitStoreRumourWindow (Window):
	Window.AddAlias('WINRUMOR')

	PositionStoreWinRelativeTo(Window)

	if GameCheck.IsPST():
		# remap controls, so we can avoid too many ifdefs
		# ... in case any mod adds rumours, since there are no such stores by default
		Window.AliasControls ({ 'STOTITLE' : 0x1000000a, 'STOGOLD' : 0x1000000b})
		Window.DeleteControl (16)
	else:
		Window.AliasControls ({ 'STOTITLE' : 0x10000011, 'STOGOLD' : 0x10000012})

	#removing those pesky labels
	if not (GameCheck.IsIWD2 () or GameCheck.IsBG1 ()):
		for i in range (5):
			Window.DeleteControl (0x10000005+i)

	TextArea = None
	if GameCheck.IsIWD2() or GameCheck.IsPST():
		TextArea = Window.GetControl (13)
	else:
		TextArea = Window.GetControl (11)
	TextArea.SetText (strrefs["rumors"])

	#tavern quality image
	if GameCheck.IsBG1() or GameCheck.IsBG2():
		BAM = "TVRNQUL%d"% ((Store['StoreFlags']>>9)&3)
		Button = Window.GetControl (12)
		Button.SetSprites (BAM, 0, 0, 0, 0, 0)
		Button.SetState (IE_GUI_BUTTON_LOCKED)

	# this scrollbar must be unhidden because it is falsely linked to a TextArea
	ScrollBar = Window.GetControl (5)
	ScrollBar.SetVisible(True)

	callback = lambda: UpdateStoreRumourWindow(Window)
	Count = Store['StoreDrinkCount'] - 5
	SetupScrollBar(ScrollBar, "TopIndex", Count, None, callback)

	UpdateStoreRumourWindow(Window)
	return

def UpdateStoreRumourWindow (Window):
	UpdateStoreCommon (Window, "STOTITLE", None, "STOGOLD")
	TopIndex = GemRB.GetVar ("TopIndex")
	DrinkButtonCount = ItemButtonCount + 1
	offset = 0
	if GameCheck.IsIWD2():
		offset = 40
		DrinkButtonCount += 1 # shows even more than with inventories
	for i in range (DrinkButtonCount):
		Drink = GemRB.GetStoreDrink (TopIndex+i)
		Button = Window.GetControl (i+offset)
		Button.SetVarAssoc ("Index", i)
		if Drink:
			if GameCheck.IsIWD2():
				Button.SetText (GemRB.GetString (Drink['DrinkName']))
				CostLabel = Window.GetControl (0x10000000+29+i)
				CostLabel.SetText (str(Drink['Price']))
			else:
				GemRB.SetToken ("ITEMNAME", GemRB.GetString (Drink['DrinkName']))
				GemRB.SetToken ("ITEMCOST", str(Drink['Price']) )
				Button.SetText (strrefs["itemnamecost"])
				if GameCheck.IsBG1 ():
					CostLabel = Window.GetControl (0x10000005 + i)
					CostLabel.SetText (str(Drink['Price']))
					Button.SetText (Drink['DrinkName'])
			Button.SetState (IE_GUI_BUTTON_ENABLED)
			Button.OnPress (GulpDrink)
		else:
			Button.SetText ("")
			Button.SetDisabled (True)
			if GameCheck.IsIWD2():
				CostLabel = Window.GetControl (0x10000000+29+i)
				CostLabel.SetText ("")
			elif GameCheck.IsBG1 ():
				CostLabel = Window.GetControl (0x100000005 + i)
				CostLabel.SetText ("")
	return

ToggleStoreRumourWindow = GUICommonWindows.CreateTopWinLoader(windowIDs["rumour"], "GUISTORE", GUICommonWindows.ToggleWindow, InitStoreRumourWindow, UpdateStoreRumourWindow, False, StoreWindowPlacement)
OpenStoreRumourWindow = GUICommonWindows.CreateTopWinLoader(windowIDs["rumour"], "GUISTORE", GUICommonWindows.OpenWindowOnce, InitStoreRumourWindow, UpdateStoreRumourWindow, False, StoreWindowPlacement)

def InitStoreRentWindow (Window):
	Window.AddAlias('WINRENT')

	PositionStoreWinRelativeTo(Window)

	if GameCheck.IsPST():
		# remap controls, so we can avoid too many ifdefs
		Window.AliasControls ({ 'RENTBTN' : 8, 'RENTTA' : 9,
								 'STOTITLE' : 0x1000000a, 'STOGOLD' : 0x1000000b, 'RENTLBL' : 0x1000000c
								 } )
	else:
		Window.AliasControls ({ 'RENTBTN' : 11, 'RENTTA' : 12,
								 'STOTITLE' : 0x10000008, 'STOGOLD' : 0x10000009, 'RENTLBL' : 0x1000000d
								 } )

	# room types
	RentIndex = -1
	for i in range (4):
		ok = Store['StoreRoomPrices'][i]
		if ok >= 0:
			RentIndex = i
			break

	# RentIndex needs to be set before SetVarAssoc
	GemRB.SetVar ("RentIndex", RentIndex)

	for i in range (4):
		ok = Store['StoreRoomPrices'][i]
		Button = Window.GetControl (i)
		Button.OnPress (lambda: UpdateStoreRentWindow(Window))
		if not ok:
			Button.SetState (IE_GUI_BUTTON_DISABLED) #disabled room icons are selected, not disabled
		else:
			Button.SetVarAssoc ("RentIndex", i)

		Button = Window.GetControl (i+4)
		Button.SetText (roomnames[i])
		Button.OnPress (lambda: UpdateStoreRentWindow(Window))
		Button.SetFlags (IE_GUI_BUTTON_RADIOBUTTON, OP_OR)
		
		if GameCheck.IsBG1():
			#these bioware guys screw up everything possible
			#remove this line if you fixed guistore
			Button.SetSprites ("GUISTROC",0, 1,2,0,3)
		
		if not ok:
			Button.SetState (IE_GUI_BUTTON_DISABLED)
		else:
			Button.SetVarAssoc ("RentIndex", i)

	# Rent
	Button = Window.GetControlAlias ('RENTBTN')
	Button.SetText (strrefs["rent"])
	Button.OnPress (RentRoom)
	return

def UpdateStoreRentWindow (Window):
	UpdateStoreCommon (Window, "STOTITLE", None, "STOGOLD")
	RentIndex = GemRB.GetVar ("RentIndex")
	Button = Window.GetControlAlias ('RENTBTN')
	Label = Window.GetControlAlias ('RENTLBL')
	if RentIndex is not None:
		TextArea = Window.GetControlAlias ('RENTTA')
		TextArea.SetText (roomdesc[RentIndex])
		price = Store['StoreRoomPrices'][RentIndex]
		Label.SetText (str(price) )
		Button.SetState (IE_GUI_BUTTON_ENABLED)
	else:
		Label.SetText ("0" )
		Button.SetState (IE_GUI_BUTTON_DISABLED)
	return

ToggleStoreRentWindow = GUICommonWindows.CreateTopWinLoader(windowIDs["rent"], "GUISTORE", GUICommonWindows.ToggleWindow, InitStoreRentWindow, UpdateStoreRentWindow, False, StoreWindowPlacement)
OpenStoreRentWindow = GUICommonWindows.CreateTopWinLoader(windowIDs["rent"], "GUISTORE", GUICommonWindows.OpenWindowOnce, InitStoreRentWindow, UpdateStoreRentWindow, False, StoreWindowPlacement)

def UpdateStoreCommon (Window, title, name, gold):
	if Store['StoreName'] != -1:
		Label = Window.GetControlAlias (title)
		if GameCheck.IsIWD2():
			# targos store is a good test - wouldn't fit as uppercase either
			Label.SetText (Store['StoreName'])
		else:
			Label.SetText (GemRB.GetString (Store['StoreName']).upper ())

	if name:
		Label = Window.GetControlAlias (name)
		if Bag:
			Label.SetText (Bag['StoreName'])
		else:
			pc = GemRB.GameGetSelectedPCSingle ()
			Label.SetText (GemRB.GetPlayerName (pc, 0))

	Label = Window.GetControlAlias (gold)
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
			GemRB.SetVar ("RightTopIndex", 0)
			GemRB.SetVar ("Index", 0)
			GemRB.SetVar ("TopIndex", 0)
			UnselectBag ()
			GemRB.CloseRighthandStore ()
			UnselectNoRedraw ()
	else:
		PreviousPC = GemRB.GameGetSelectedPCSingle ()
		pc = PreviousPC

	return pc

# Unselects all the selected buttons, so they are not preselected in other windows
def UnselectNoRedraw ():
	pc = GemRB.GameGetSelectedPCSingle ()
	LeftCount = Store['StoreItemCount']
	for i in range (LeftCount, 0, -1):
		Flags = GemRB.IsValidStoreItem (pc, i-1, ITEM_STORE)&SHOP_SELECT
		if Flags:
			GemRB.ChangeStoreItem (pc, i-1, SHOP_BUY|SHOP_SELECT)

	RightCount = len (inventory_slots)
	for Slot in range (RightCount):
		Flags = GemRB.IsValidStoreItem (pc, inventory_slots[Slot], ITEM_PC) & SHOP_SELECT
		if Flags:
			GemRB.ChangeStoreItem (pc, inventory_slots[Slot], SHOP_SELL|SHOP_SELECT)
			# same code for ID, so no repeat needed

def SelectID (btn):
	pc = GemRB.GameGetSelectedPCSingle ()
	Index = btn.Value + GemRB.GetVar ("TopIndex")
	GemRB.ChangeStoreItem (pc, inventory_slots[Index], SHOP_ID|SHOP_SELECT)
	RedrawStoreIdentifyWindow (btn.Window)
	return

def SelectBuy (btn):
	pc = GemRB.GameGetSelectedPCSingle ()
	LeftIndex = btn.Value + GemRB.GetVar("LeftTopIndex")
	GemRB.ChangeStoreItem (pc, LeftIndex, SHOP_BUY|SHOP_SELECT)
	RedrawStoreShoppingWindow (GemRB.GetView('WINSHOP'))
	return
	
def SelectSteal(btn):
	pc = GemRB.GameGetSelectedPCSingle ()
	LeftIndex = btn.Value + GemRB.GetVar("LeftTopIndex")
	isSelected = GemRB.IsValidStoreItem (pc, LeftIndex, ITEM_STORE) & SHOP_SELECT
	UnselectNoRedraw()

	if not isSelected:
		GemRB.ChangeStoreItem (pc, LeftIndex, SHOP_STEAL | SHOP_SELECT)
	else:
		GemRB.SetVar("LeftIndex", None)
	RedrawStoreStealWindow (btn.Window)
	return

def ToBackpackPressed ():
	pc = GemRB.GameGetSelectedPCSingle ()
	LeftCount = Store['StoreItemCount']
	#going backwards because removed items shift the slots
	for i in range (LeftCount, 0, -1):
		Flags = GemRB.IsValidStoreItem (pc, i-1, ITEM_STORE)&SHOP_SELECT
		if Flags:
			GemRB.ChangeStoreItem (pc, i-1, SHOP_BUY)

	UpdateStoreShoppingWindow (GemRB.GetView('WINSHOP'))
	return

def BuyPressed ():
	if (BuySum>GemRB.GameGetPartyGold ()):
		ErrorWindow (strrefs["itemstoocostly"])
		return

	pc = GemRB.GameGetSelectedPCSingle ()
	LeftCount = Store['StoreItemCount']
	#going backwards because removed items shift the slots
	for i in range (LeftCount, 0, -1):
		Flags = GemRB.IsValidStoreItem (pc, i-1, ITEM_STORE)&SHOP_SELECT
		if Flags:
			Slot = GemRB.GetStoreItem (i-1)
			Item = GemRB.GetItem (Slot['ItemResRef'])
			Price = GetRealPrice (pc, "sell", Item, Slot) * Slot['Purchased']
			if Price <= 0:
				Price = Slot['Purchased']

			if GemRB.ChangeStoreItem (pc, i-1, SHOP_BUY):
				GemRB.GameSetPartyGold (GemRB.GameGetPartyGold ()-Price)

	UpdateStoreShoppingWindow (GemRB.GetView('WINSHOP'))
	return

def SelectSell (btn):
	pc = GemRB.GameGetSelectedPCSingle ()
	RightIndex = btn.Value + GemRB.GetVar("RightTopIndex")
	if not Bag:
		RightIndex = inventory_slots[RightIndex]
	#bags may be clickable despite not being sellable
	Flags = GemRB.IsValidStoreItem (pc, RightIndex, ITEM_BAG if Bag else ITEM_PC)
	if Flags & SHOP_SELL:
		GemRB.ChangeStoreItem (pc, RightIndex, SHOP_SELL|SHOP_SELECT)
		RedrawStoreShoppingWindow (GemRB.GetView('WINSHOP'))
	return

def ToBagPressed ():
	pc = GemRB.GameGetSelectedPCSingle ()
	RightCount = len (inventory_slots)
	#no need to go reverse
	for Slot in range (RightCount):
		Flags = GemRB.IsValidStoreItem (pc, inventory_slots[Slot], ITEM_PC)
		if Flags & SHOP_SELECT:
			GemRB.ChangeStoreItem (pc, inventory_slots[Slot], SHOP_SELL)

	UpdateStoreShoppingWindow (GemRB.GetView('WINSHOP'))
	return

def SellPressed ():
	pc = GemRB.GameGetSelectedPCSingle ()
	if Bag:
		RightCount = Bag['StoreItemCount']
		#going backwards because removed items shift the slots
		for Slot in range (RightCount-1, -1, -1):
			Flags = GemRB.IsValidStoreItem (pc, Slot, ITEM_BAG)
			if Flags & SHOP_SELECT:
				#NOTE: what if the transaction fails?
				GemRB.ChangeStoreItem (pc, Slot, SHOP_SELL)
	else:
		RightCount = len (inventory_slots)
		#no need to go reverse
		for Slot in range (RightCount):
			Flags = GemRB.IsValidStoreItem (pc, inventory_slots[Slot], ITEM_PC) & SHOP_SELECT
			if Flags:
				GemRB.ChangeStoreItem (pc, inventory_slots[Slot], SHOP_SELL)

	GemRB.GameSetPartyGold (GemRB.GameGetPartyGold ()+SellSum)
	GemRB.PlaySound(DEF_SOLD)
	UpdateStoreShoppingWindow (GemRB.GetView('WINSHOP'))
	return

def OpenBag (Window):
	if Bag:
		#nested containers are not openable in bg2,
		#but double-click has another function here
		OpenItemAmountWindow (Window, STORE_BAG)
		return
	if Inventory:
		return
	pc = GemRB.GameGetSelectedPCSingle ()
	RightIndex = GemRB.GetVar ("RightIndex")
	Slot = GemRB.GetSlotItem (pc, inventory_slots[RightIndex])
	Item = GemRB.GetItem (Slot['ItemResRef'])
	if Item['Function'] & ITM_F_CONTAINER:
		GemRB.SetVar ("RightIndex", 0)
		GemRB.SetVar ("RightTopIndex", 0)
		GemRB.SetVar ("Index", 0)
		GemRB.SetVar ("TopIndex", 0)
		UnselectNoRedraw ()
		GemRB.LoadRighthandStore (Slot['ItemResRef'])
		UpdateStoreShoppingWindow (Window)
	return

def CloseBag (Window):
	if not Bag:
		return
	GemRB.SetVar ("RightIndex", 0)
	GemRB.SetVar ("RightTopIndex", 0)
	GemRB.SetVar ("Index", 0)
	GemRB.SetVar ("TopIndex", 0)
	UnselectBag ()
	GemRB.CloseRighthandStore ()
	UnselectNoRedraw ()
	UpdateStoreShoppingWindow (Window)
	return

def UnselectBag ():
	if not Bag:
		return
	pc = GemRB.GameGetSelectedPCSingle ()
	RightCount = Bag['StoreItemCount']
	for Slot in range (RightCount):
		Flags = GemRB.IsValidStoreItem (pc, Slot, ITEM_BAG)
		if Flags & SHOP_SELECT:
			GemRB.ChangeStoreItem (pc, Slot, SHOP_SELL|SHOP_SELECT)
	return

def RedrawStoreShoppingWindow (Window):
	global BuySum, SellSum

	UpdateStoreCommon (Window, "STOTITLE", "STONAME", "STOGOLD")
	pc = GemRB.GameGetSelectedPCSingle ()

	LeftTopIndex = GemRB.GetVar ("LeftTopIndex")
	RightTopIndex = GemRB.GetVar ("RightTopIndex")
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
				Price = GetRealPrice (pc, "sell", Item, Slot) * Slot['Purchased']
			if Price <= 0:
				Price = Slot['Purchased']
			BuySum = BuySum + Price

	if Bag:
		RightCount = Bag['StoreItemCount']
	else:
		RightCount = len(inventory_slots)
	SellSum = 0
	for i in range (RightCount):
		if Bag:
			Flags = GemRB.IsValidStoreItem (pc, i, ITEM_BAG)
		else:
			Flags = GemRB.IsValidStoreItem (pc, inventory_slots[i], ITEM_PC)
		if Flags & SHOP_SELECT:
			if Bag:
				Slot = GemRB.GetStoreItem (i, STORE_BAG)
			else:
				Slot = GemRB.GetSlotItem (pc, inventory_slots[i])
			Item = GemRB.GetItem (Slot['ItemResRef'])
			if Inventory:
				Price = 1
			else:
				Price = GetRealPrice (pc, "buy", Item, Slot)
				if Bag:
					Price *= Slot['Purchased']
			if Flags & SHOP_ID:
				Price = 1
			SellSum = SellSum + Price

	Label = Window.GetControlAlias ('PRICEB')
	if Inventory:
		Label.SetText ("")
	else:
		Label.SetText (str(BuySum) )

	# shade the inventory icon if it is full
	free_slots = len(GemRB.GetSlots (pc, SLOT_INVENTORY, -1))
	Button = Window.GetControl (44)
	if Button:
		if free_slots == 0:
			Button.SetState (IE_GUI_BUTTON_PRESSED)
		else:
			Button.SetState (IE_GUI_BUTTON_LOCKED)

	# also disable the button if the inventory is full
	if BuySum and selected_count <= free_slots:
		LeftButton.SetState (IE_GUI_BUTTON_ENABLED)
	else:
		LeftButton.SetState (IE_GUI_BUTTON_DISABLED)

	Label = Window.GetControlAlias ('PRICES')
	if Inventory:
		Label.SetText ("")
	else:
		Label.SetText (str(SellSum) )
	if SellSum:
		RightButton.SetState (IE_GUI_BUTTON_ENABLED)
	else:
		RightButton.SetState (IE_GUI_BUTTON_DISABLED)

	if GameCheck.IsBG2():
		CloseBagButton = Window.GetControl (50)
		if Bag:
			CloseBagButton.SetText (37452)
			CloseBagButton.SetState (IE_GUI_BUTTON_ENABLED)
			CloseBagButton.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_NAND)
		else:
			CloseBagButton.SetText (None)
			CloseBagButton.SetState (IE_GUI_BUTTON_LOCKED)
			CloseBagButton.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_OR)

	for i in range (ItemButtonCount):
		if i+LeftTopIndex<LeftCount:
			Slot = GemRB.GetStoreItem (i+LeftTopIndex)
		else:
			Slot = None
		Button = Window.GetControlAlias ('LBTN' + str(i))
		Label = Window.GetControlAlias ('LLBL' + str(i))
		SetupItems (pc, Slot, Button, Label, i, ITEM_STORE)

		if i+RightTopIndex<RightCount:
			if Bag:
				Slot = GemRB.GetStoreItem (i+RightTopIndex, STORE_BAG)
			else:
				Slot = GemRB.GetSlotItem (pc, inventory_slots[i+RightTopIndex])
		else:
			Slot = None
		Button = Window.GetControlAlias ('RBTN' + str(i))
		Label = Window.GetControlAlias ('RLBL' + str(i))
		SetupItems (pc, Slot, Button, Label, i, ITEM_BAG if Bag else ITEM_PC)

	if GameCheck.IsPST():
		GUICommon.SetEncumbranceLabels (Window, 25, None, pc)
	else:
		GUICommon.SetEncumbranceLabels (Window, 0x10000043, 0x10000044, pc)
	return

def OpenItemAmountWindow (ShopWin, store = STORE_MAIN):
	global MaxAmount

	wid = 16
	if GameCheck.IsIWD2() or GameCheck.IsIWD1():
		wid = 20
	elif GameCheck.IsBG2():
		pass
	else:
		return

	Window = GemRB.LoadWindow (wid, "GUISTORE")
	if store == STORE_MAIN:
		Index = GemRB.GetVar ("LeftIndex")
	else:
		Index = GemRB.GetVar ("RightIndex")
	Slot = GemRB.GetStoreItem (Index, store)
	Amount = Slot['Purchased']
	if Amount == 0:
		Amount = 1
	MaxAmount = Slot['Amount']
	if MaxAmount == -1:
		MaxAmount = 999

	# item icon
	Icon = Window.GetControl (0)
	Icon.SetFlags (IE_GUI_BUTTON_PICTURE | IE_GUI_BUTTON_NO_IMAGE, OP_SET)
	Icon.SetItemIcon (Slot['ItemResRef'])

	# item amount
	Text = Window.GetControl (6)
	Text.SetText (str (Amount))
	Text.SetFlags (IE_GUI_TEXTEDIT_ALPHACHARS, OP_NAND)
	Text.Focus()

	def SetItemAmount (Number):
		if Number < 0:
			Number = 0
		elif Number > MaxAmount:
			Number = MaxAmount
		Text.SetText (str (Number))

	# Decrease
	Button = Window.GetControl (4)
	Button.OnPress (lambda: SetItemAmount(Text.QueryInteger() - 1))
	Button.SetActionInterval (200)

	# Increase
	Button = Window.GetControl (3)
	Button.OnPress (lambda: SetItemAmount(Text.QueryInteger() + 1))
	Button.SetActionInterval (200)

	def CloseAmountWindow():
		Window.Close()
		UpdateStoreShoppingWindow(ShopWin)

	def Confirm():
		ConfirmItemAmount(Text.QueryInteger(), store)
		CloseAmountWindow()

	# Done
	Button = Window.GetControl (2)
	Button.SetText (11973)
	Button.OnPress (Confirm)
	Button.MakeDefault ()

	# Cancel
	Button = Window.GetControl (1)
	Button.SetText (13727)
	Button.OnPress (CloseAmountWindow)

	Window.ShowModal (MODAL_SHADOW_GRAY)
	return

def ConfirmItemAmount (Number, store = STORE_MAIN):
	global MaxAmount

	if Number > MaxAmount:
		Number = MaxAmount
	elif Number < 0:
		Number = 0
	if store == STORE_MAIN:
		Index = GemRB.GetVar ("LeftIndex")
	else:
		Index = GemRB.GetVar ("RightIndex")
	GemRB.SetPurchasedAmount (Index, Number, store)

	return

def RedrawStoreIdentifyWindow (Window):
	UpdateStoreCommon (Window, "STOTITLE", "STONAME", "STOGOLD")
	TopIndex = GemRB.GetVar ("TopIndex")
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
		Button = Window.GetControlAlias ("IDBTN" + str(i))
		# TODO: recheck iwd2 vs non-pst really differ
		if GameCheck.IsIWD2():
			Label = Window.GetControl (0x1000000d+i)
		elif GameCheck.IsPST():
			Label = Window.GetControl (0x10000009+i)
		else:
			Label = Window.GetControl (0x1000000c+i)

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
				Button.EnableBorder (0, True)
			else:
				Button.SetState (IE_GUI_BUTTON_DISABLED)
				GemRB.SetToken ("ITEMNAME", GemRB.GetString (Item['ItemNameIdentified']))
				GemRB.SetToken ("ITEMCOST", str(0) )
				Button.EnableBorder (0, False)

			Label.SetText (strrefs["itemnamecost"])
		else:
			Button.SetState (IE_GUI_BUTTON_DISABLED)
			Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_OR)
			Button.SetFlags (IE_GUI_BUTTON_PICTURE, OP_NAND)
			Button.SetText ("")
			Label.SetText ("")

	Button = Window.GetControlAlias ("IDLBTN")
	Label = Window.GetControlAlias ("IDPRICE")
	if Selected:
		Button.SetState (IE_GUI_BUTTON_ENABLED)
		Label.SetText (str(IDPrice * Selected) )
	else:
		Button.SetState (IE_GUI_BUTTON_DISABLED)
		Label.SetText (str(0) )
	return

def IdentifyPressed (Window):
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
		ErrorWindow (strrefs["idtoocostly"])
		return

	# identify
	TextArea = Window.GetControlAlias ("IDTA")
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

	UpdateStoreIdentifyWindow (Window)
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

def InfoLeftWindow (btn):
	Index = btn.Value + GemRB.GetVar("LeftTopIndex")
	Slot = GemRB.GetStoreItem (Index)
	Item = GemRB.GetItem (Slot['ItemResRef'])
	InfoWindow (Slot, Item)
	return

def InfoRightWindow (btn):
	Index = btn.Value + GemRB.GetVar("RightTopIndex")
	if Bag:
		Slot = GemRB.GetStoreItem (Index, STORE_BAG)
		Item = GemRB.GetItem (Slot['ItemResRef'])
	else:
		pc = GemRB.GameGetSelectedPCSingle ()
		Count = len(inventory_slots)
		if Index >= Count:
			return
		Slot = GemRB.GetSlotItem (pc, inventory_slots[Index])
		Item = GemRB.GetItem (Slot['ItemResRef'])
	InfoWindow (Slot, Item)
	return

def InfoWindow (Slot, Item):
	Identify = Slot['Flags'] & IE_INV_ITEM_IDENTIFIED

	Window = GemRB.LoadWindow (windowIDs["iteminfo"], "GUISTORE", StoreWindowPlacement)

	# TODO: check if we can simplify bg2 vs non-pst games to see which label is which
	if GameCheck.IsBG2():
		NameLabel = Window.GetControl (0x10000000)
		FakeLabel = Window.GetControl (0x10000007)
	elif GameCheck.IsPST():
		NameLabel = Window.GetControl (0x0fffffff)
		FakeLabel = Window.GetControl (0x10000000)
	else:
		NameLabel = Window.GetControl (0x10000007)
		FakeLabel = Window.GetControl (0x10000000)

	if GameCheck.IsPST():
		aliases = { "INFTA": 4, "INFBTN": 3, "INFIMG": 6 }
	else:
		aliases = { "INFTA": 5, "INFBTN": 4, "INFIMG": 7 }
	Window.AliasControls (aliases)

	#fake label
	FakeLabel.SetText ("")

	#description bam
	if GameCheck.IsBG1() or GameCheck.IsBG2() or GameCheck.IsPST():
		Button = Window.GetControlAlias ("INFIMG")
		Button.SetFlags (IE_GUI_BUTTON_PICTURE | IE_GUI_BUTTON_CENTER_PICTURES | IE_GUI_BUTTON_NO_IMAGE, OP_OR)
		Button.SetState (IE_GUI_BUTTON_LOCKED)
		if GameCheck.IsPST():
			Button.SetItemIcon (Slot['ItemResRef'], 1)
		else:
			Button.SetItemIcon (Slot['ItemResRef'], 2)

	#slot bam
	Button = Window.GetControl (2)
	Button.SetState (IE_GUI_BUTTON_LOCKED)
	Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_OR)
	Button.SetItemIcon (Slot['ItemResRef'], 0)

	TextArea = Window.GetControlAlias ("INFTA")
	if Identify:
		NameLabel.SetText (Item['ItemNameIdentified'])
		TextArea.SetText (Item['ItemDescIdentified'])
	else:
		NameLabel.SetText (Item['ItemName'])
		TextArea.SetText (Item['ItemDesc'])

	#Done
	Button = Window.GetControlAlias ("INFBTN")
	Button.SetText (strrefs["done"])
	Button.OnPress (Window.Close)

	# hide the empty button
	if GameCheck.IsBG2() or GameCheck.IsIWD2():
		Window.DeleteControl (9)

	Window.ShowModal (MODAL_SHADOW_GRAY)
	return

def StealPressed (btn):
	LeftIndex = GemRB.GetVar("LeftIndex")
	pc = GemRB.GameGetSelectedPCSingle ()
	#percentage skill check, if fails, trigger StealFailed
	#if difficulty = 0 and skill=100, automatic success
	#if difficulty = 0 and skill=50, 50% success
	#if difficulty = 50 and skill=50, 0% success
	#if skill>random(100)+difficulty - success
	if GUICommon.CheckStat100 (pc, IE_PICKPOCKET, Store['StealFailure']):
		GemRB.ChangeStoreItem (pc, LeftIndex, SHOP_STEAL)
		GemRB.PlaySound(DEF_STOLEN)
		UpdateStoreStealWindow (btn.Window)
	else:
		GemRB.StealFailed ()
		CloseStoreWindow ()
	return

def RedrawStoreStealWindow (Window):
	UpdateStoreCommon (Window, "STOTITLE", "STONAME", "STOGOLD")
	LeftTopIndex = GemRB.GetVar ("LeftTopIndex")
	RightTopIndex = GemRB.GetVar ("RightTopIndex")
	LeftCount = Store['StoreItemCount']
	pc = GemRB.GameGetSelectedPCSingle ()
	RightCount = len(inventory_slots)
	for i in range (ItemButtonCount):
		Slot = GemRB.GetStoreItem (i+LeftTopIndex)
		Button = Window.GetControlAlias ('SWLBTN' + str(i))
		Label = Window.GetControlAlias ('SWLLBL' + str(i))
		SetupItems (pc, Slot, Button, Label, i, ITEM_STORE, True)

		if i+RightTopIndex<RightCount:
			Slot = GemRB.GetSlotItem (pc, inventory_slots[i+RightTopIndex])
		else:
			Slot = None
		Button = Window.GetControlAlias ('SWRBTN' + str(i))
		Label = Window.GetControlAlias ('SWRLBL' + str(i))
		SetupItems (pc, Slot, Button, Label, i, ITEM_PC, True)

	# shade the inventory icon if it is full
	free_slots = len(GemRB.GetSlots (pc, SLOT_INVENTORY, -1))
	Button = Window.GetControl (37)
	if Button:
		if free_slots == 0:
			Button.SetState (IE_GUI_BUTTON_PRESSED)
		else:
			Button.SetState (IE_GUI_BUTTON_LOCKED)

	# also disable the button if the inventory is full
	if GemRB.GetVar("LeftIndex") is not None and free_slots:
		LeftButton.SetState (IE_GUI_BUTTON_ENABLED)
	else:
		LeftButton.SetState (IE_GUI_BUTTON_DISABLED)

	if GameCheck.IsPST():
		GUICommon.SetEncumbranceLabels (Window, 22, None, pc)
	else:
		GUICommon.SetEncumbranceLabels (Window, 0x10000043, 0x10000044, pc)
	return

def SetupItems (pc, Slot, Button, Label, i, storetype, steal = False):
	if Slot == None:
		# BG1 doesnt have empty slots on the window bg
		# instead of GameChecking, just clear data for all instead of setting visibility
		Button.SetPicture(None)
		Button.SetState(IE_GUI_BUTTON_DISABLED)
		Button.SetText("")
		Button.EnableBorder (1, False)
		Label.SetVisible(False)
		return

	Item = GemRB.GetItem (Slot['ItemResRef'])
	Button.SetItemIcon (Slot['ItemResRef'], 0)
	Label.SetVisible(True)
	if Item['MaxStackAmount'] > 1:
		Button.SetText ( str(Slot['Usages0']) )
	else:
		Button.SetText ("")

	state = IE_GUI_BUTTON_DISABLED
	Price = None
	
	if GemRB.CanUseItemType (SLOT_ANY, Slot['ItemResRef'], pc):
		Button.EnableBorder (1, False)
	else:
		Button.EnableBorder (1, True)

	if storetype == ITEM_STORE:
		LeftTopIndex = GemRB.GetVar("LeftTopIndex")
		Price = max(1, GetRealPrice (pc, "buy", Item, Slot))
		Flags = GemRB.IsValidStoreItem (pc, i + LeftTopIndex, storetype)
		if Flags & SHOP_SELECT:
			state = IE_GUI_BUTTON_SELECTED
		elif (Flags & SHOP_STEAL) and steal:
			state = IE_GUI_BUTTON_ENABLED
		elif (Flags & SHOP_BUY) and not steal:
			state = IE_GUI_BUTTON_ENABLED

		if not Inventory:
			Price = max(1, GetRealPrice (pc, "sell", Item, Slot))
	else:
		RightTopIndex = GemRB.GetVar("RightTopIndex")
		index = RightTopIndex + i
		if not Bag:
			index = inventory_slots[index]

		Flags = GemRB.IsValidStoreItem (pc, index, storetype)
		if steal:
			Button.EnableBorder (1, False)
		else:
			Price = 1 if Inventory else GetRealPrice(pc, "buy", Item, Slot)

			if Item['Function'] & ITM_F_CONTAINER and not Bag and not Inventory:
				# containers are always clickable
				state = IE_GUI_BUTTON_ENABLED
			elif Flags & SHOP_SELECT:
				state = IE_GUI_BUTTON_SELECTED
			elif Flags & SHOP_SELL and Price > 0:
				state = IE_GUI_BUTTON_ENABLED
			else:
				color = {'r': 61, 'g': 47, 'b': 24, 'a': 100}
				Button.SetBorder (2, color, 1,1)
				state = IE_GUI_BUTTON_DISABLED

	Button.SetState(state)
	if Flags & SHOP_ID:
		Name = GemRB.GetString (Item['ItemName'])
		Button.EnableBorder (0, True)
	else:
		Name = GemRB.GetString (Item['ItemNameIdentified'])
		Button.EnableBorder (0, False)

	GemRB.SetToken ("ITEMNAME", Name)
	GemRB.SetToken ("ITEMCOST", str(Price))
	# only show the item name?
	if (Inventory or steal) and not GameCheck.IsPST():
		if GameCheck.IsIWD1() or GameCheck.IsIWD2():
			LabelText = GemRB.GetString (24890)
		elif GameCheck.IsBG2():
			LabelText = GemRB.GetString (28337)
		elif steal:
			LabelText = Name
		else:
			LabelText = ""
	else:
		LabelText = GemRB.GetString (strrefs["itemnamecost"])

	if (storetype == ITEM_STORE and not steal) or storetype == ITEM_BAG:
		if Slot["Amount"] != -1:
			LabelText = LabelText + " (" + str(Slot["Amount"]) + ")"
	Label.SetText (LabelText)
	return

# handle raisdead.2da price override for raising and resurrecting
def GetRealCurePrice (cure):
	global RaiseDeadTable

	if not RaiseDeadTable:
		RaiseDeadTable = GemRB.LoadTable ("raisdead")
	pc = GemRB.GameGetSelectedPCSingle ()
	raisingPrice = RaiseDeadTable.GetValue (str(GemRB.GetPlayerStat(pc, IE_CLASSLEVELSUM)), "VALUE")
	if cure['CureResRef'].lower() == "sppr504":
		return raisingPrice
	if cure['CureResRef'].lower() == "sppr712":
		return 150 * raisingPrice // 100
	return cure['Price']

def GetRealPrice (pc, mode, Item, Slot):
	# get the base from the item
	price = Item['Price']

	if Item['MaxStackAmount']>1:
		price = price * Slot['Usages0']
	elif Item['MaxCharge']>0:
		price = price * Slot['Usages0'] // Item['MaxCharge']

	# modifier from store properties (in percent)
	if mode == "buy":
		mod = Store['BuyMarkup']
		if GemRB.HasFeat(pc, FEAT_MERCANTILE_BACKGROUND):
			mod += 5
	else:
		mod = Store['SellMarkup']
		if GemRB.HasFeat(pc, FEAT_MERCANTILE_BACKGROUND):
			mod -= 5

	# depreciation works like this:
	# - if you sell the item the first time, SellMarkup is used;
	# - if you sell the item the second time, SellMarkup-DepreciationRate is used;
	# - if you sell the item any more times (N), SellMarkup-N*DepreciationRate is used.
	# NB: there is a minimal price of 20 %, regardless of the amount of depreciation
	# If the storekeep has an infinite amount of the item, only SellMarkup is used.
	# The amount of items sold at the same time doesn't matter! Selling three bows
	# separately will produce less gold then selling them at the same time.
	# We don't care who is the seller, so if the store already has 2 items, there'll be no gain
	if mode == "buy":
		count = GemRB.FindStoreItem (Slot["ItemResRef"])
		if count:
			# jewelry doesn't suffer from deprecation, at least in BG2
			if Item['Type'] in [CommonTables.ItemType.GetRowIndex ("GEM"),
			                    CommonTables.ItemType.GetRowIndex ("RING"),
			                    CommonTables.ItemType.GetRowIndex ("AMULET")]:
				count = 0
			# give at least 20 %
			mod -= count * Store['Depreciation']
			mod = max(mod, 20)
	else:
		# charisma modifier (in percent)
		BarteringPC = GemRB.GetVar("BARTER_PC");
		mod += GemRB.GetAbilityBonus (IE_CHR, GemRB.GetPlayerStat (BarteringPC, IE_CHR) - 1, 0)

		# reputation modifier (in percent, but absolute)
		if RepModTable and not (Store['StoreFlags'] & SHOP_NOREPADJ):
			mod = mod * RepModTable.GetValue (0, GemRB.GameGetReputation() // 10 - 1) // 100

	effprice = price * mod // 100
	#in bg2 even 1gp items can be sold for at least 1gp
	if effprice == 0 and Item['Price'] > 0:
		effprice = 1
	return effprice

def DonateGold ():
	Window = GemRB.GetView('WINDONAT')

	TextArea = Window.GetControl (0)
	TextArea.SetFlags (IE_GUI_TEXTAREA_AUTOSCROLL)

	Button = Window.GetControl (10)
	if Button:
		Button.SetAnimation ("DONATE", 0, A_ANI_PLAYONCE)

	Field = Window.GetControl (5)
	donation = int("0"+Field.QueryText ())

	# for some reason temple donations get a boost to rumour generation,
	# even though comparatively they're higher than drink prices and with
	# the default factor of 3, very quickly guarantee a rumour
	RumourTable = GemRB.LoadTable ("donarumr")
	RumourFactor = RumourTable.GetValue ("RUMORRATE", "VALUE", GTV_INT)
	RumourChance = min(RumourFactor * donation, 100) # cap is from the original
	text = GemRB.GetRumour (RumourChance, Store['TempleRumour'])
	GemRB.GameSetPartyGold (GemRB.GameGetPartyGold ()-donation)
	feedbackSound = DEF_DONATE2
	feedbackString = strrefs['donorfail']
	if GemRB.IncreaseReputation (donation):
		feedbackSound = DEF_DONATE1
		feedbackString = strrefs['donorgood']

	TextArea.Append (feedbackString)
	TextArea.Append ("\n\n")
	if text > -1:
		TextArea.Append (text)
		TextArea.Append ("\n\n")
	GemRB.PlaySound (feedbackSound)
	UpdateStoreDonateWindow (Window)
	return

def InfoHealWindow ():
	Window = GemRB.GetView('WINHEAL')

	UpdateStoreHealWindow (Window)
	Index = GemRB.GetVar ("Index") + GemRB.GetVar ("TopIndex")
	Cure = GemRB.GetStoreCure (Index)
	Spell = GemRB.GetSpell (Cure['CureResRef'])

	Window = GemRB.LoadWindow (windowIDs["cureinfo"], "GUISTORE", StoreWindowPlacement)

	Label = Window.GetControl (0x10000000)
	Label.SetText (Spell['SpellName'])

	Button = Window.GetControl (2)
	Button.SetSpellIcon (Cure['CureResRef'], 1)
	if GameCheck.IsPST():
		Button = Window.GetControl (6)
		Button.SetSpellIcon (Cure['CureResRef'], 2)
		aliases = { "HEALBTN": 3, "HEALTA": 4 }
	else:
		aliases = { "HEALBTN": 5, "HEALTA": 3 }
	Window.AliasControls (aliases)

	TextArea = Window.GetControlAlias ("HEALTA")
	TextArea.SetText (Spell['SpellDesc'])

	#Done
	Button = Window.GetControlAlias ("HEALBTN")
	Button.SetText (strrefs["done"])
	Button.OnPress (Window.Close)

	Window.ShowModal (MODAL_SHADOW_GRAY)
	return

def BuyHeal ():
	Window = GemRB.GetView('WINHEAL')

	Index = GemRB.GetVar ("Index") + GemRB.GetVar ("TopIndex")
	Cure = GemRB.GetStoreCure (Index)
	gold = GemRB.GameGetPartyGold ()
	price = GetRealCurePrice (Cure)
	if gold < price:
		ErrorWindow (strrefs["spelltoocostly"])
		return

	GemRB.GameSetPartyGold (gold - price)
	pc = GemRB.GameGetSelectedPCSingle ()
	Spell = GemRB.GetSpell (Cure['CureResRef'])
	# for anything but raise/resurrect, the talker should be the caster, so
	# self-targeting spells work properly. Raise dead is an exception as
	# the teleporting to the temple would not work otherwise
	if Spell["SpellTargetType"] == 3: # non-living
		GemRB.ApplySpell (pc, Cure['CureResRef'], Store['StoreOwner'])
	else:
		GemRB.ApplySpell (pc, Cure['CureResRef'], pc)

	if SpellTable:
		sound = SpellTable.GetValue(Cure['CureResRef'], "SOUND")
	elif CureTable:
		sound = CureTable.GetValue(Cure['CureResRef'], "SOUND_EFFECT")
	else:
		#if there is no table, simply use the spell's own completion sound
		Spell = GemRB.GetSpell(Cure['CureResRef'])
		sound = Spell['Completion']
	GemRB.PlaySound (sound)
	UpdateStoreHealWindow (Window)
	return

def GulpDrink (btn):
	Window = GemRB.GetView('WINRUMOR')

	TextArea = Window.GetControl (13)
	TextArea.SetFlags (IE_GUI_TEXTAREA_AUTOSCROLL)
	pc = GemRB.GameGetSelectedPCSingle ()
	intox = GemRB.GetPlayerStat (pc, IE_INTOXICATION)
	if intox > 80:
		TextArea.Append (strrefs["toodrunk"])
		TextArea.Append ("\n\n")
		return

	gold = GemRB.GameGetPartyGold ()
	Index = btn.Value + GemRB.GetVar("TopIndex")
	Drink = GemRB.GetStoreDrink (Index)
	if gold < Drink['Price']:
		ErrorWindow (strrefs["drinktoocostly"])
		return

	GemRB.GameSetPartyGold (gold-Drink['Price'])
	GemRB.SetPlayerStat (pc, IE_INTOXICATION, intox+Drink['Strength'])
	text = GemRB.GetRumour (Drink['Strength'], Store['TavernRumour'])
	if text > -1:
		TextArea.Append (text)
		TextArea.Append ("\n\n")
	GemRB.PlaySound (DEF_DRUNK)
	UpdateStoreRumourWindow (Window)
	return

def RentConfirm (Window0):
	RentIndex = GemRB.GetVar ("RentIndex")
	price = Store['StoreRoomPrices'][RentIndex]
	Gold = GemRB.GameGetPartyGold ()
	GemRB.GameSetPartyGold (Gold-price)
	RestTable = GemRB.LoadTable ("restheal")
	healFor = RestTable.GetValue (RentIndex, 0, GTV_INT)
	# TODO: run GemRB.RunRestScripts ()
	info = GemRB.RestParty (2, 1, healFor) # 2 = REST_SCATTER, check that everyone is close by
	cutscene = info["Cutscene"]

	if RentConfirmWindow:
		RentConfirmWindow.Close ()
	if info["Error"]:
		# notify why resting isn't possible
		ErrorWindow (info["ErrorMsg"])

	Window = GemRB.GetView('WINRENT')
	if cutscene:
		CloseStoreWindow ()
	elif not info["Error"]:
		TextArea = Window.GetControlAlias('RENTTA')
		GemRB.SetToken ("HP", str(healFor))
		TextArea.SetText (strrefs["restedfor"])
		GemRB.SetVar ("RentIndex", None)
		Button = Window.GetControl (RentIndex+4)
		Button.SetState (IE_GUI_BUTTON_ENABLED)
	return

def RentRoom ():
	global RentConfirmWindow

	RentIndex = GemRB.GetVar ("RentIndex")
	price = Store['StoreRoomPrices'][RentIndex]
	Gold = GemRB.GameGetPartyGold ()
	if Gold<price:
		ErrorWindow (strrefs['roomtoocostly'])
		return

	RentConfirmWindow = Window = GemRB.LoadWindow (windowIDs["rentconfirm"], "GUISTORE")

	#confirm
	Button = Window.GetControl (0)
	Button.SetText (strrefs["rest"])
	Button.OnPress (RentConfirm)
	Button.MakeDefault()

	#deny
	Button = Window.GetControl (1)
	Button.SetText (strrefs["cancel"])
	Button.OnPress (Window.Close)
	Button.MakeEscape()

	#textarea
	TextArea = Window.GetControl (3)
	TextArea.SetText (strrefs["confirmrest"])

	Window.ShowModal (MODAL_SHADOW_GRAY)
	return

def ErrorWindow (strref):
	Window = GemRB.LoadWindow (windowIDs["error"], "GUISTORE")

	TextArea = Window.GetControl (3)
	TextArea.SetText (strref)

	#done
	Button = Window.GetControl (0)
	Button.SetText (strrefs["done"])
	Button.OnPress (Window.Close)

	Window.ShowModal (MODAL_SHADOW_GRAY)
	return

###################################################
# End of file GUISTORE.py
