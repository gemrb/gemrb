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
import GameCheck
import GUICommon
import GUICommonWindows
from GUIDefines import *
from ie_stats import *
from ie_slots import *
from ie_sounds import *
from ie_feats import FEAT_MERCANTILE_BACKGROUND
import CommonTables

StoreWindow = None

ItemAmountWindow = None
RentConfirmWindow = None
LeftButton = None
RightButton = None
CureTable = None

ITEM_PC    = 0
ITEM_STORE = 1

Inventory = None
Store = None
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
BarteringPC = 0
MaxAmount = 0

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
Buttons = [-1] * StoreButtonCount

def CloseStoreWindow ():
	import GUIINV
	global StoreWindow

	GemRB.SetVar ("Inventory", 0)
	if StoreWindow:
		StoreWindow.Close ()

	GemRB.LeaveStore ()

	if Inventory: # broken if available (huh? pst-related huh?)
		GUIINV.OpenInventoryWindow ()
	else:
		GemRB.GamePause (0, 3)
		GUICommonWindows.SetSelectionChangeHandler( None )

	CureTable = None
	GUICommonWindows.CloseTopWindow()
	return

def OpenStoreWindow ():
	global Store
	global StoreWindow, MenuWindow
	global store_funcs
	global SpellTable, RepModTable
	global Inventory, BarteringPC
	global CureTable

	if GameCheck.IsPST():
		CureTable = GemRB.LoadTable("speldesc") #additional info not supported by core
	else:
		if not GameCheck.IsIWD2(): # present from before, resulting in a 10x price increase
			RepModTable = GemRB.LoadTable ("repmodst")
		SpellTable = GemRB.LoadTable ("storespl", 1)

	store_funcs = (OpenStoreShoppingWindow,
	OpenStoreIdentifyWindow,OpenStoreStealWindow,
	OpenStoreHealWindow, OpenStoreDonateWindow,
	OpenStoreRumourWindow,OpenStoreRentWindow )

	if GemRB.GetVar ("Inventory"):
		Inventory = 1
	else:
		Inventory = None
		# pause the game, so we don't get interrupted
		GemRB.GamePause (1, 3)

	GemRB.SetVar ("Action", 0)

	StoreWindow = Window = GemRB.LoadWindow (3, "GUISTORE", WINDOW_HCENTER|WINDOW_BOTTOM)
	#this window is static and grey, but good to stick the frame onto

	if GameCheck.IsPST():
		MenuWindow = GemRB.LoadWindow (2)

	Store = GemRB.GetStore ()
	BarteringPC = GemRB.GameGetFirstSelectedPC ()

	# Done
	Button = Window.GetControl (0)
	Button.SetText (strrefs["done"])
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, CloseStoreWindow)

	#Store type icon
	if not GameCheck.IsIWD2():
		Button = Window.GetControl (5)
		Button.SetSprites (storebams[Store['StoreType']],0,0,0,0,0)

	#based on shop type, these buttons will change
	store_type = Store['StoreType']
	store_buttons = Store['StoreButtons']
	for i in range (StoreButtonCount):
		Buttons[i] = Button = Window.GetControl (i+1)
		Action = store_buttons[i]
		Button.SetVarAssoc ("Action", i)
		# iwd2 has no steal window
		if GameCheck.IsIWD2() and Action == 2:
			Action = -1
		if Action>=0:
			Button.SetFlags (IE_GUI_BUTTON_RADIOBUTTON, OP_OR)
			if GameCheck.IsIWD1() or GameCheck.IsIWD2():
				Button.SetSprites ("GUISTBBC", Action, 1,2,0,0)
			elif GameCheck.IsPST():
				Button.SetSprites (storebams[Action], 0, 0,1,2,0)
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

	store_funcs[store_buttons[0]] ()

	return

def InitStoreShoppingWindow (Window):
	global LeftButton, RightButton

	Window.AddAlias('WINSHOP')

	GemRB.SetVar ("LeftIndex", 0) # reset the shopkeeps list
	GemRB.SetVar ("LeftTopIndex", 0)
	GemRB.SetVar ("RightTopIndex", 0)
	GemRB.SetVar ("LeftTopIndex", 0)

	if GameCheck.IsPST():
		# remap controls, so we can avoid too many ifdefs
		# strings must be able to fit into a resref (<= 8 char)
		aliases = { 'STOLBTN' : 0, 'STORBTN' : 1, 'STODONAT' : 7,
					'STOTEDIT' : 8, 'STOPLUS' : 9, 'STOMINUS' : 10,
					'???' : 11, 'STOSBARR' : 16, 'STOSBARL' : 110,

					'PRICEB' : 0x10000003, 'PRICES' : 0x10000004,
					'STOTITLE' : 0x10000001, 'STONAME' : 0x10000005, 'STOGOLD' : 0x10000002,
					}

		GUICommon.AliasControls (Window,  {'RBTN' + str(x) : x+17 for x in range(ItemButtonCount)} )
		GUICommon.AliasControls (Window,  {'RLBL' + str(x) : x+0x10000014 for x in range(ItemButtonCount)} )
		GUICommon.AliasControls (Window,  {'LLBL' + str(x) : x+0x1000000b for x in range(ItemButtonCount)} )

	else:
		aliases = { 'STOLBTN' : 2, 'STORBTN' : 3, 'STODONAT' : 110,
					'STOTEDIT' : 7, 'STOPLUS' : 5, 'STOMINUS' : 6,
					'???' : 8, 'STOSBARR' : 12, 'STOSBARL' : 11,

					'PRICEB' : 0x1000002b, 'PRICES' : 0x1000002c,
					'STOTITLE' : 0x10000003, 'STONAME' : 0x1000002e, 'STOGOLD' : 0x1000002a,
					}

		GUICommon.AliasControls (Window,  {'RBTN' + str(x) : x+13 for x in range(ItemButtonCount)} )
		GUICommon.AliasControls (Window,  {'RLBL' + str(x) : x+0x1000001e for x in range(ItemButtonCount)} )
		GUICommon.AliasControls (Window,  {'LLBL' + str(x) : x+0x10000012 for x in range(ItemButtonCount)} )


	GUICommon.AliasControls (Window, aliases)

	# left scrollbar
	ScrollBarLeft = GemRB.GetView ('STOSBARL')
	ScrollBarLeft.SetEvent (IE_GUI_SCROLLBAR_ON_CHANGE, lambda: RedrawStoreShoppingWindow(Window))

	# right scrollbar
	ScrollBarRight = GemRB.GetView ('STOSBARR')
	ScrollBarRight.SetEvent (IE_GUI_SCROLLBAR_ON_CHANGE, lambda: RedrawStoreShoppingWindow(Window))

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
		Label = GemRB.GetView ('PRICEB')
		Label.SetText ("")
		# sell price ...
		Label = GemRB.GetView ('PRICES')
		Label.SetText ("")
		# buy price ...
		Label = Window.GetControl (0x1000002f)
		Label.SetText ("")
		# sell price ...
		Label = Window.GetControl (0x10000030)
		Label.SetText ("")
	else:
		# buy price ...
		Label = GemRB.GetView ('PRICEB')
		Label.SetText ("0")

		# sell price ...
		Label = GemRB.GetView ('PRICES')
		Label.SetText ("0")

	for i in range (ItemButtonCount):
		if GameCheck.IsBG2():
			color = {'r' : 0, 'g' : 0, 'b' : 128, 'a' : 160}
		elif GameCheck.IsPST():
			color = {'r' : 128, 'g' : 0, 'b' : 0, 'a' : 100}
		else:
			color = {'r' : 32, 'g' : 32, 'b' : 192, 'a' : 128}

		Button = Window.GetControl (i+5)
		Button.SetBorder (0,color,0,1)
		color = {'r' : 255, 'g' : 128, 'b' : 128, 'a' : 64}
		Button.SetBorder (1, color, 0,1)
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, SelectBuy)
		Button.SetEvent (IE_GUI_BUTTON_ON_DOUBLE_PRESS, OpenItemAmountWindow)
		Button.SetEvent (IE_GUI_BUTTON_ON_RIGHT_PRESS, InfoLeftWindow)
		Button.SetFont ("NUMBER")
		Button.SetFlags (IE_GUI_BUTTON_ALIGN_RIGHT|IE_GUI_BUTTON_ALIGN_BOTTOM, OP_OR)

		Button = Window.GetControl (i+13)
		if GameCheck.IsBG2():
			Button.SetSprites ("GUIBTBUT", 0, 0,1,2,5)

		Button.SetBorder (0,color,0,1)
		if Store['StoreType'] != 3: # can't sell to temples
			Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, SelectSell)
		Button.SetEvent (IE_GUI_BUTTON_ON_RIGHT_PRESS, InfoRightWindow)
		Button.SetFont ("NUMBER")
		Button.SetFlags (IE_GUI_BUTTON_ALIGN_RIGHT|IE_GUI_BUTTON_ALIGN_BOTTOM, OP_OR)

	UnselectNoRedraw ()

	# Buy
	LeftButton = Button = GemRB.GetView ('STOLBTN')
	if Inventory:
		if GameCheck.IsIWD1() or GameCheck.IsIWD2():
			Button.SetText (26287)
		elif GameCheck.IsBG2():
			Button.SetText (51882)
		else:
			Button.SetText ("")
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, ToBackpackPressed)
	else:
		Button.SetText (strrefs["buy"])
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, BuyPressed)

	# Sell
	RightButton = Button = GemRB.GetView ('STORBTN')
	if Inventory:
		if GameCheck.IsIWD1() or GameCheck.IsIWD2():
			Button.SetText (26288)
		elif GameCheck.IsBG2():
			Button.SetText (51883)
		else:
			Button.SetText ("")
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, ToBagPressed)
	else:
		Button.SetText (strrefs["sell"])
		if Store['StoreType'] != 3: # can't sell to temples
			Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, SellPressed)

	# inactive button
	if GameCheck.IsBG2():
		Button = Window.GetControl (50)
		Button.SetState (IE_GUI_BUTTON_LOCKED)
		Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_SET)

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
		Label = BackpackButton.CreateLabel (0x10000043, "NUMBER", "0:",
			IE_FONT_ALIGN_LEFT|IE_FONT_ALIGN_TOP|IE_FONT_SINGLE_LINE)
		Label = BackpackButton.CreateLabel (0x10000044, "NUMBER", "0:",
			IE_FONT_ALIGN_RIGHT|IE_FONT_ALIGN_BOTTOM|IE_FONT_SINGLE_LINE)

	return

def UpdateStoreShoppingWindow (Window):
	global Store, inventory_slots

	#reget store in case of a change
	Store = GemRB.GetStore ()
	pc = GetPC()

	LeftCount = Store['StoreItemCount'] - ItemButtonCount
	if LeftCount<0:
		LeftCount=0
	ScrollBar = Window.GetControl (11)
	ScrollBar.SetVarAssoc ("LeftTopIndex", LeftCount)
	LeftTopIndex = GemRB.GetVar ("LeftTopIndex")
	if LeftTopIndex>LeftCount:
		GemRB.SetVar ("LeftTopIndex", LeftCount)

	inventory_slots = GemRB.GetSlots (pc, SLOT_INVENTORY)
	RightCount = len(inventory_slots) - ItemButtonCount
	if RightCount<0:
		RightCount=0
	ScrollBar = Window.GetControl (12)
	ScrollBar.SetVarAssoc ("RightTopIndex", RightCount)
	RightTopIndex = GemRB.GetVar ("RightTopIndex")
	if RightTopIndex>RightCount:
		GemRB.SetVar ("RightTopIndex", RightCount)

	RedrawStoreShoppingWindow (Window)
	return

ToggleStoreShoppingWindow = GUICommonWindows.CreateTopWinLoader(windowIDs["shop"], "GUISTORE", GUICommonWindows.ToggleWindow, InitStoreShoppingWindow, UpdateStoreShoppingWindow, WINDOW_HCENTER|WINDOW_TOP)
OpenStoreShoppingWindow = GUICommonWindows.CreateTopWinLoader(windowIDs["shop"], "GUISTORE", GUICommonWindows.OpenWindowOnce, InitStoreShoppingWindow, UpdateStoreShoppingWindow, WINDOW_HCENTER|WINDOW_TOP)

def InitStoreIdentifyWindow (Window):
	global LeftButton

	GemRB.SetVar ("Index", -1)
	GemRB.SetVar ("TopIndex", 0)

	if GameCheck.IsPST():
		# remap controls, so we can avoid too many ifdefs
		GUICommon.AliasControls (Window,  { 'IDSBAR' : 5, 'IDLBTN' : 4, 'IDTA' : 14,
											'IDPRICE' : 0x10000001, 'STOGOLD' : 0x10000000,
											'STOTITLE' : 0x0fffffff, 'STONAME' : 0x10000002
										  } )
		GUICommon.AliasControls (Window,  {'IDBTN' + str(x) : 9-x for x in range(ItemButtonCount)} )
	else:
		GUICommon.AliasControls (Window,  { 'IDSBAR' : 7, 'IDLBTN' : 5, 'IDTA' : 23,
											'IDPRICE' : 0x10000003, 'STOGOLD' : 0x10000001,
											'STOTITLE' : 0x10000000, 'STONAME' : 0x10000005
										  } )
		GUICommon.AliasControls (Window,  {'IDBTN' + str(x) : 11-x for x in range(ItemButtonCount)} )

	ScrollBar = GemRB.GetView ('IDSBAR')
	ScrollBar.SetEvent (IE_GUI_SCROLLBAR_ON_CHANGE, lambda: RedrawStoreIdentifyWindow(Window))

	TextArea = GemRB.GetView ('IDTA')
	TextArea.SetFlags (IE_GUI_TEXTAREA_AUTOSCROLL)

	# Identify
	LeftButton = Button = GemRB.GetView ('IDLBTN')
	Button.SetText (strrefs["identify"])
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, IdentifyPressed)
	Button.SetEvent (IE_GUI_BUTTON_ON_RIGHT_PRESS, InfoIdentifyWindow)

	# price ...
	Label = GemRB.GetView ('IDPRICE')
	Label.SetText ("0")

	# 8-11 item slots, 0x1000000c-f labels
	for i in range (ItemButtonCount):
		if GameCheck.IsIWD1() or GameCheck.IsIWD2():
			Button.SetSprites ("GUISTMSC", 0, 1,2,0,3)
			color = {'r' : 32, 'g' : 32, 'b' : 192, 'a' : 128}
		elif GameCheck.IsBG1():
			color = {'r' : 32, 'g' : 32, 'b' : 192, 'a' : 128}
		elif GameCheck.IsPST():
			color = {'r' : 128, 'g' : 0, 'b' : 0, 'a' : 100}
		else:
			color = {'r' : 0, 'g' : 0, 'b' : 128, 'a' : 160}

		Button = Window.GetControl (i+8)
		Button.SetBorder (0, color, 0, 1)
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, SelectID)
		Button.SetEvent (IE_GUI_BUTTON_ON_RIGHT_PRESS, InfoIdentifyWindow)
		Button.SetFont ("NUMBER")
		Button.SetFlags (IE_GUI_BUTTON_ALIGN_RIGHT|IE_GUI_BUTTON_ALIGN_BOTTOM, OP_OR)

	UnselectNoRedraw ()
	return

def UpdateStoreIdentifyWindow (Window):
	global inventory_slots

	pc = GetPC()
	inventory_slots = GemRB.GetSlots (pc, SLOT_INVENTORY)
	Count = len(inventory_slots)
	ScrollBar = Window.GetControl (7)
	ScrollBar.SetVarAssoc ("TopIndex", Count-ItemButtonCount)
	GemRB.SetVar ("Index", -1)
	RedrawStoreIdentifyWindow (Window)
	return

ToggleStoreIdentifyWindow = GUICommonWindows.CreateTopWinLoader(windowIDs["ident"], "GUISTORE", GUICommonWindows.ToggleWindow, InitStoreIdentifyWindow, UpdateStoreIdentifyWindow, WINDOW_HCENTER|WINDOW_TOP)
OpenStoreIdentifyWindow = GUICommonWindows.CreateTopWinLoader(windowIDs["ident"], "GUISTORE", GUICommonWindows.OpenWindowOnce, InitStoreIdentifyWindow, UpdateStoreIdentifyWindow, WINDOW_HCENTER|WINDOW_TOP)

def InitStoreStealWindow (Window):
	global LeftButton

	GemRB.SetVar ("RightIndex", 0)
	GemRB.SetVar ("LeftIndex", 0)
	GemRB.SetVar ("RightTopIndex", 0)
	GemRB.SetVar ("LeftTopIndex", 0)

	if GameCheck.IsPST():
		# remap controls, so we can avoid too many ifdefs

		GUICommon.AliasControls (Window,  { 'STEAL' : 0, 'SWLSBAR' : 4, 'SWRSBAR' : 13,
								 			'STONAME' : 0x10000002, 'STOTITLE' : 0x10000000, 'STOGOLD' : 0x10000001
								 			} )

		GUICommon.AliasControls (Window,  {'SWLBTN' + str(x) : x+5 for x in range(ItemButtonCount)} )
		GUICommon.AliasControls (Window,  {'SWRBTN' + str(x) : x+14 for x in range(ItemButtonCount)} )
		GUICommon.AliasControls (Window,  {'SWRLBL' + str(x) : x+0x10000011 for x in range(ItemButtonCount)} )
		GUICommon.AliasControls (Window,  {'SWLLBL' + str(x) : x+0x10000008 for x in range(ItemButtonCount)} )
	else:
		GUICommon.AliasControls (Window,  { 'STEAL' : 1, 'SWLSBAR' : 9, 'SWRSBAR' : 10,
								 			'STONAME' : 0x10000027, 'STOTITLE' : 0x10000002, 'STOGOLD' : 0x10000023
										  } )

		GUICommon.AliasControls (Window,  {'SWLBTN' + str(x) : x+4 for x in range(ItemButtonCount)} )
		GUICommon.AliasControls (Window,  {'SWRBTN' + str(x) : x+11 for x in range(ItemButtonCount)} )
		GUICommon.AliasControls (Window,  {'SWRLBL' + str(x) : x+0x10000019 for x in range(ItemButtonCount)} )
		GUICommon.AliasControls (Window,  {'SWLLBL' + str(x) : x+0x1000000f for x in range(ItemButtonCount)} )

	# left scrollbar
	ScrollBarLeft = GemRB.GetView ('SWLSBAR')
	ScrollBarLeft.SetEvent (IE_GUI_SCROLLBAR_ON_CHANGE, lambda: RedrawStoreStealWindow(Window))

	# right scrollbar
	ScrollBarRight = GemRB.GetView ('SWRSBAR')
	ScrollBarRight.SetEvent (IE_GUI_SCROLLBAR_ON_CHANGE, lambda: RedrawStoreStealWindow(Window))

	for i in range (ItemButtonCount):
		if GameCheck.IsBG2():
			color = {'r' : 0, 'g' : 0, 'b' : 128, 'a' : 160}
		elif GameCheck.IsPST():
			color = {'r' : 128, 'g' : 0, 'b' : 0, 'a' : 100}
		else:
			color = {'r' : 32, 'g' : 32, 'b' : 192, 'a' : 128}

		Button = GemRB.GetView ('SWLBTN' + str(i))
		Button.SetBorder (0,color,0,1)
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, lambda: RedrawStoreStealWindow(Window))
		Button.SetFont ("NUMBER")
		Button.SetFlags (IE_GUI_BUTTON_ALIGN_RIGHT|IE_GUI_BUTTON_ALIGN_BOTTOM, OP_OR)

		Button = GemRB.GetView ('SWRBTN' + str(i))
		Button.SetBorder (0,color,0,1)
		Button.SetEvent (IE_GUI_BUTTON_ON_RIGHT_PRESS, InfoRightWindow)
		Button.SetFont ("NUMBER")
		Button.SetFlags (IE_GUI_BUTTON_ALIGN_RIGHT|IE_GUI_BUTTON_ALIGN_BOTTOM, OP_OR)

	UnselectNoRedraw ()

	# Steal
	LeftButton = Button = GemRB.GetView ('STEAL')
	Button.SetText (strrefs["steal"])
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, StealPressed)

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
		Label = Button.CreateLabel (0x10000043, "NUMBER", "0:",
			IE_FONT_ALIGN_LEFT|IE_FONT_ALIGN_TOP|IE_FONT_SINGLE_LINE)
		Label = Button.CreateLabel (0x10000044, "NUMBER", "0:",
			IE_FONT_ALIGN_RIGHT|IE_FONT_ALIGN_BOTTOM|IE_FONT_SINGLE_LINE)

	return

def UpdateStoreStealWindow (Window):
	global Store, inventory_slots

	#reget store in case of a change
	Store = GemRB.GetStore ()
	LeftCount = Store['StoreItemCount']
	ScrollBar = GemRB.GetView ('SWLSBAR')
	ScrollBar.SetVarAssoc ("LeftTopIndex", LeftCount-ItemButtonCount)

	pc = GetPC()
	inventory_slots = GemRB.GetSlots (pc, SLOT_INVENTORY)
	RightCount = len(inventory_slots)
	ScrollBar = GemRB.GetView ('SWRSBAR')
	ScrollBar.SetVarAssoc ("RightTopIndex", RightCount-ItemButtonCount)
	GemRB.SetVar ("LeftIndex", -1)
	LeftButton.SetState (IE_GUI_BUTTON_DISABLED)
	RedrawStoreStealWindow (Window)
	return

ToggleStoreStealWindow = GUICommonWindows.CreateTopWinLoader(windowIDs["steal"], "GUISTORE", GUICommonWindows.ToggleWindow, InitStoreStealWindow, UpdateStoreStealWindow, WINDOW_HCENTER|WINDOW_TOP)
OpenStoreStealWindow = GUICommonWindows.CreateTopWinLoader(windowIDs["steal"], "GUISTORE", GUICommonWindows.OpenWindowOnce, InitStoreStealWindow, UpdateStoreStealWindow, WINDOW_HCENTER|WINDOW_TOP)

def InitStoreDonateWindow (Window):
	Window.AddAlias('WINDONAT')

	if GameCheck.IsPST():
		# remap controls, so we can avoid too many ifdefs
		GUICommon.AliasControls (Window,  { 'DONATE' : 2, 'STOTEDIT' : 3, 'STOPLUS' : 4,
								 			'STOMINUS' : 5, 'STOTITLE' : 0x10000005, 'STOGOLD' : 0x10000006
										  } )
	else:
		GUICommon.AliasControls (Window,  { 'DONATE' : 3, 'STOTEDIT' : 5, 'STOPLUS' : 6,
								 			'STOMINUS' : 7, 'STOTITLE' : 0x10000007, 'STOGOLD' : 0x10000008
										  } )

	# graphics
	Button = Window.GetControl (10)
	if Button:
		Button.SetFlags (IE_GUI_BUTTON_PICTURE|IE_GUI_BUTTON_ANIMATED|IE_GUI_BUTTON_PLAYONCE, OP_OR)
		Button.SetState (IE_GUI_BUTTON_LOCKED)

	# Donate
	Button = GemRB.GetView ('DONATE')
	Button.SetText (strrefs["donate"])
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, DonateGold)
	Button.MakeDefault()

	# Entry
	Field = GemRB.GetView ('STOTEDIT')
	Field.SetText ("0")
	Field.SetEvent (IE_GUI_EDIT_ON_CHANGE, lambda: UpdateStoreDonateWindow(Window))
	Field.SetStatus (IE_GUI_EDIT_NUMBER)
	Field.Focus()

	# +
	Button = GemRB.GetView ('STOPLUS')
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, IncrementDonation)
	Button.SetActionInterval (50)
	# -
	Button = GemRB.GetView ('STOMINUS')
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, DecrementDonation)
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

	Button = GemRB.GetView ("DONATE")
	Button.SetDisabled(not donation)
	return

ToggleStoreDonateWindow = GUICommonWindows.CreateTopWinLoader(windowIDs["donate"], "GUISTORE", GUICommonWindows.ToggleWindow, InitStoreDonateWindow, UpdateStoreDonateWindow, WINDOW_HCENTER|WINDOW_TOP)
OpenStoreDonateWindow = GUICommonWindows.CreateTopWinLoader(windowIDs["donate"], "GUISTORE", GUICommonWindows.OpenWindowOnce, InitStoreDonateWindow, UpdateStoreDonateWindow, WINDOW_HCENTER|WINDOW_TOP)

def InitStoreHealWindow (Window):
	GemRB.SetVar ("Index", -1)
	GemRB.SetVar ("TopIndex", 0)

	Window.AddAlias('WINHEAL')

	if GameCheck.IsPST():
		# remap controls, so we can avoid too many ifdefs
		GUICommon.AliasControls (Window,  { 'HWSBAR' : 4, 'HEALBTN' : 3, 'HWTA' : 13,
								 'HWPRICE' : 0x10000001, 'STOGOLD' : 0x10000000, 'STOTITLE' : 0x0fffffff
								 } )
		GUICommon.AliasControls (Window,  {'HWLBTN' + str(x) : x+5 for x in range(ItemButtonCount)} )
	else:
		GUICommon.AliasControls (Window,  { 'HWSBAR' : 7, 'HEALBTN' : 5, 'HWTA' : 23,
								 'HWPRICE' : 0x10000003, 'STOGOLD' : 0x10000001, 'STOTITLE' : 0x10000000
								 } )
		GUICommon.AliasControls (Window,  {'HWLBTN' + str(x) : x+8 for x in range(ItemButtonCount)} )

	ScrollBar = GemRB.GetView ('HWSBAR')
	ScrollBar.SetEvent (IE_GUI_SCROLLBAR_ON_CHANGE, lambda: UpdateStoreHealWindow(Window))

	#spell buttons
	for i in range (ItemButtonCount):
		Button = GemRB.GetView ('HWLBTN' + str(i))
		Button.SetFlags (IE_GUI_BUTTON_RADIOBUTTON, OP_OR)
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, lambda: UpdateStoreHealWindow(Window))
		Button.SetEvent (IE_GUI_BUTTON_ON_RIGHT_PRESS, InfoHealWindow)

	UnselectNoRedraw ()

	# price tag
	Label = GemRB.GetView ('HWPRICE')
	Label.SetText ("0")

	# Heal
	Button = GemRB.GetView ('HEALBTN')
	Button.SetText (strrefs["heal"])
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, BuyHeal)
	Button.SetDisabled (True)

	Count = Store['StoreCureCount']
	if Count>4:
		Count = Count-4
	else:
		Count = 0
	ScrollBar.SetVarAssoc ("TopIndex", Count)
	return

def UpdateStoreHealWindow (Window):
	UpdateStoreCommon (Window, "STOTITLE", None, "STOGOLD")
	TopIndex = GemRB.GetVar ("TopIndex")
	Index = GemRB.GetVar ("Index")
	pc = GemRB.GameGetSelectedPCSingle ()
	labelOffset = 0x1000000c
	if GameCheck.IsIWD2():
		labelOffset += 1 # grrr
	elif GameCheck.IsPST():
		labelOffset = 0x10000008
	for i in range (ItemButtonCount):
		Cure = GemRB.GetStoreCure (TopIndex+i)

		Button = GemRB.GetView ('HWLBTN' + str(i))
		Label = Window.GetControl (labelOffset+i)
		Button.SetVarAssoc ("Index", TopIndex+i)
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
				Button.SetState (IE_GUI_BUTTON_ENABLED)
				Button.SetBorder (0, None, 0,0)

			GemRB.SetToken ("ITEMNAME", GemRB.GetString (Spell['SpellName']))
			GemRB.SetToken ("ITEMCOST", str(Cure['Price']) )
			Label.SetText (strrefs["itemnamecost"])
		else:
			Button.SetDisabled (True)
			Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_OR)
			Button.SetFlags (IE_GUI_BUTTON_PICTURE, OP_NAND)
			Button.SetBorder (0, None, 0,0)
			Label.SetText ("")

		if TopIndex+i==Index:
			TextArea = Window.GetControl (23)
			TextArea.SetText (Cure['Description'])
			Label = Window.GetControl (0x10000003)
			Label.SetText (str(Cure['Price']) )
			Button = Window.GetControl (5)
			Button.SetState (IE_GUI_BUTTON_ENABLED)
	return

ToggleStoreHealWindow = GUICommonWindows.CreateTopWinLoader(windowIDs["heal"], "GUISTORE", GUICommonWindows.ToggleWindow, InitStoreHealWindow, UpdateStoreHealWindow, WINDOW_HCENTER|WINDOW_TOP)
OpenStoreHealWindow = GUICommonWindows.CreateTopWinLoader(windowIDs["heal"], "GUISTORE", GUICommonWindows.OpenWindowOnce, InitStoreHealWindow, UpdateStoreHealWindow, WINDOW_HCENTER|WINDOW_TOP)

def InitStoreRumourWindow (Window):
	GemRB.SetVar ("TopIndex", 0)

	Window.AddAlias('WINRUMOR')

	if GameCheck.IsPST():
		# remap controls, so we can avoid too many ifdefs
		GUICommon.AliasControls (Window,  { 'STOTITLE' : 0x1000000a, 'STOGOLD' : 0x1000000b} )
	else:
		GUICommon.AliasControls (Window,  { 'STOTITLE' : 0x10000011, 'STOGOLD' : 0x10000012} )

	#removing those pesky labels
	if not GameCheck.IsIWD2():
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
	ScrollBar.SetEvent (IE_GUI_SCROLLBAR_ON_CHANGE, lambda: UpdateStoreRumourWindow(Window))
	Count = Store['StoreDrinkCount']
	if Count>5:
		Count = Count-5
	else:
		Count = 0
	ScrollBar.SetVarAssoc ("TopIndex", Count)
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
			Button.SetState (IE_GUI_BUTTON_ENABLED)
			Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, GulpDrink)
		else:
			Button.SetText ("")
			Button.SetDisabled (True)
			if GameCheck.IsIWD2():
				CostLabel = Window.GetControl (0x10000000+29+i)
				CostLabel.SetText ("")
	return

ToggleStoreRumourWindow = GUICommonWindows.CreateTopWinLoader(windowIDs["rumour"], "GUISTORE", GUICommonWindows.ToggleWindow, InitStoreRumourWindow, UpdateStoreRumourWindow, WINDOW_HCENTER|WINDOW_TOP)
OpenStoreRumourWindow = GUICommonWindows.CreateTopWinLoader(windowIDs["rumour"], "GUISTORE", GUICommonWindows.OpenWindowOnce, InitStoreRumourWindow, UpdateStoreRumourWindow, WINDOW_HCENTER|WINDOW_TOP)

def InitStoreRentWindow (Window):
	Window.AddAlias('WINRENT')

	if GameCheck.IsPST():
		# remap controls, so we can avoid too many ifdefs
		GUICommon.AliasControls (Window,  { 'RENTBTN' : 8, 'RENTTA' : 9,
								 'STOTITLE' : 0x1000000a, 'STOGOLD' : 0x1000000b, 'RENTLBL' : 0x1000000c
								 } )
	else:
		GUICommon.AliasControls (Window,  { 'RENTBTN' : 11, 'RENTTA' : 12,
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
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, lambda: UpdateStoreRentWindow(Window))
		if ok<0:
			Button.SetState (IE_GUI_BUTTON_DISABLED) #disabled room icons are selected, not disabled
		else:
			Button.SetVarAssoc ("RentIndex", i)

		Button = Window.GetControl (i+4)
		Button.SetText (roomnames[i])
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, lambda: UpdateStoreRentWindow(Window))
		Button.SetFlags (IE_GUI_BUTTON_RADIOBUTTON, OP_OR)
		Button.SetVarAssoc ("RentIndex", i)
		if GameCheck.IsBG1():
			#these bioware guys screw up everything possible
			#remove this line if you fixed guistore
			Button.SetSprites ("GUISTROC",0, 1,2,0,3)
		if ok<0:
			Button.SetState (IE_GUI_BUTTON_DISABLED)

	# Rent
	Button = GemRB.GetView ('RENTBTN')
	Button.SetText (strrefs["rent"])
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, RentRoom)
	return

def UpdateStoreRentWindow (Window):
	UpdateStoreCommon (Window, "STOTITLE", None, "STOGOLD")
	RentIndex = GemRB.GetVar ("RentIndex")
	Button = GemRB.GetView ('RENTBTN')
	Label = GemRB.GetView ('RENTLBL')
	if RentIndex>=0:
		TextArea = GemRB.GetView ('RENTTA')
		TextArea.SetText (roomdesc[RentIndex])
		price = Store['StoreRoomPrices'][RentIndex]
		Label.SetText (str(price) )
		Button.SetState (IE_GUI_BUTTON_ENABLED)
	else:
		Label.SetText ("0" )
		Button.SetState (IE_GUI_BUTTON_DISABLED)
	return

ToggleStoreRentWindow = GUICommonWindows.CreateTopWinLoader(windowIDs["rent"], "GUISTORE", GUICommonWindows.ToggleWindow, InitStoreRentWindow, UpdateStoreRentWindow, WINDOW_HCENTER|WINDOW_TOP)
OpenStoreRentWindow = GUICommonWindows.CreateTopWinLoader(windowIDs["rent"], "GUISTORE", GUICommonWindows.OpenWindowOnce, InitStoreRentWindow, UpdateStoreRentWindow, WINDOW_HCENTER|WINDOW_TOP)

def UpdateStoreCommon (Window, title, name, gold):
	if Store['StoreName'] != -1:
		Label = GemRB.GetView (title)
		if GameCheck.IsIWD2():
			# targos store is a good test - wouldn't fit as uppercase either
			Label.SetText (Store['StoreName'])
		else:
			Label.SetText (GemRB.GetString (Store['StoreName']).upper ())

	if name:
		pc = GemRB.GameGetSelectedPCSingle ()
		Label = GemRB.GetView (name)
		Label.SetText (GemRB.GetPlayerName (pc, 0) )

	Label = GemRB.GetView (gold)
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

def SelectID ():
	pc = GemRB.GameGetSelectedPCSingle ()
	Index = GemRB.GetVar ("Index")
	GemRB.ChangeStoreItem (pc, inventory_slots[Index], SHOP_ID|SHOP_SELECT)
	RedrawStoreIdentifyWindow (Window)
	return

def SelectBuy ():
	pc = GemRB.GameGetSelectedPCSingle ()
	LeftIndex = GemRB.GetVar ("LeftIndex")
	GemRB.ChangeStoreItem (pc, LeftIndex, SHOP_BUY|SHOP_SELECT)
	RedrawStoreShoppingWindow (GemRB.GetView('WINSHOP'))
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
				Price = 1

			if GemRB.ChangeStoreItem (pc, i-1, SHOP_BUY):
				GemRB.GameSetPartyGold (GemRB.GameGetPartyGold ()-Price)

	UpdateStoreShoppingWindow (GemRB.GetView('WINSHOP'))
	return

def SelectSell ():
	pc = GemRB.GameGetSelectedPCSingle ()
	RightIndex = GemRB.GetVar ("RightIndex")
	GemRB.ChangeStoreItem (pc, inventory_slots[RightIndex], SHOP_SELL|SHOP_SELECT)
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

def RedrawStoreShoppingWindow (Window):
	global BuySum, SellSum

	UpdateStoreCommon (Window, "STOTITLE", "STONAME", "STOGOLD")
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
				Price = GetRealPrice (pc, "sell", Item, Slot) * Slot['Purchased']
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

	if GameCheck.IsPST():
		GUICommon.SetEncumbranceLabels (Window, 25, None, pc, True)
	else:
		GUICommon.SetEncumbranceLabels (Window, 0x10000043, 0x10000044, pc)
	return

def OpenItemAmountWindow ():
	global ItemAmountWindow
	global MaxAmount

	if ItemAmountWindow != None:
		return

	wid = 16
	if GameCheck.IsIWD2():
		wid = 20
	elif GameCheck.IsBG2():
		pass
	else:
		return

	ItemAmountWindow = Window = GemRB.LoadWindow (wid)
	Index = GemRB.GetVar ("LeftIndex")
	Slot = GemRB.GetStoreItem (Index)
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
	Text.SetStatus (IE_GUI_EDIT_NUMBER)
	Text.Focus()

	# Decrease
	Button = Window.GetControl (4)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, DecreaseItemAmount)
	Button.SetActionInterval (200)

	# Increase
	Button = Window.GetControl (3)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, IncreaseItemAmount)
	Button.SetActionInterval (200)

	# Done
	Button = Window.GetControl (2)
	Button.SetText (11973)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, ConfirmItemAmount)
	Button.MakeDefault ()

	# Cancel
	Button = Window.GetControl (1)
	Button.SetText (13727)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, CancelItemAmount)

	Window.ShowModal (MODAL_SHADOW_GRAY)
	return

def DecreaseItemAmount ():
	global ItemAmountWindow

	Text = ItemAmountWindow.GetControl (6)
	Amount = Text.QueryText ()
	Number = int ("0"+Amount)-1
	if Number < 0:
		Number = 0
	Text.SetText (str (Number))
	return

def IncreaseItemAmount ():
	global ItemAmountWindow
	global MaxAmount

	Text = ItemAmountWindow.GetControl (6)
	Amount = Text.QueryText ()
	Number = int ("0"+Amount)+1
	if Number > MaxAmount:
		Number = MaxAmount
	Text.SetText (str (Number))
	return

def ConfirmItemAmount ():
	global ItemAmountWindow
	global MaxAmount

	Text = ItemAmountWindow.GetControl (6)
	Amount = Text.QueryText ()
	Number = int ("0"+Amount)
	if Number > MaxAmount:
		Number = MaxAmount
	elif Number < 0:
		Number = 0
	Index = GemRB.GetVar ("LeftIndex")
	GemRB.SetPurchasedAmount (Index, Number)

	ItemAmountWindow.Unload ()
	ItemAmountWindow = None
	UpdateStoreShoppingWindow ()
	return

def CancelItemAmount ():
	global ItemAmountWindow

	ItemAmountWindow.Unload ()
	ItemAmountWindow = None
	UpdateStoreShoppingWindow ()
	return

def RedrawStoreIdentifyWindow (Window):
	UpdateStoreCommon (Window, "STOTITLE", "STONAME", "STOGOLD")
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
		# TODO: recheck iwd2 vs non-pst really differ
		if GameCheck.IsIWD2():
			Label = Window.GetControl (0x1000000d+i)
		elif GameCheck.IsPST():
			Label = Window.GetControl (0x10000009+i)
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

			Label.SetText (strrefs["itemnamecost"])
		else:
			Button.SetState (IE_GUI_BUTTON_DISABLED)
			Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_OR)
			Button.SetFlags (IE_GUI_BUTTON_PICTURE, OP_NAND)
			Button.SetText ("")
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
		ErrorWindow (strrefs["idtoocostly"])
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
	Identify = Slot['Flags'] & IE_INV_ITEM_IDENTIFIED

	Window = GemRB.LoadWindow (windowIDs["iteminfo"], "GUISTORE", WINDOW_HCENTER|WINDOW_TOP)

	# TODO: check if we can simplify bg2 vs non-pst games to see which label is which
	if GameCheck.IsBG2():
		NameLabel = Window.GetControl (0x10000000)
		FakeLabel = Window.GetControl (0x10000007)
	elif GameCheck.IsPST():
		Window.ReassignControls ((4,3,6), (5,4,7))
		NameLabel = Window.GetControl (0x0fffffff)
		FakeLabel = Window.GetControl (0x10000000)
	else:
		NameLabel = Window.GetControl (0x10000007)
		FakeLabel = Window.GetControl (0x10000000)

	#fake label
	FakeLabel.SetText ("")

	#description bam
	if GameCheck.IsBG1() or GameCheck.IsBG2() or GameCheck.IsPST():
		Button = Window.GetControl (7)
		Button.SetFlags (IE_GUI_BUTTON_PICTURE | IE_GUI_BUTTON_CENTER_PICTURES | IE_GUI_BUTTON_NO_IMAGE, OP_OR)
		Button.SetState (IE_GUI_BUTTON_LOCKED)
		if GameCheck.IsPST():
			Button.SetItemIcon (Slot['ItemResRef'], 1)
		else:
			Button.SetItemIcon (Slot['ItemResRef'], 2)

	#slot bam
	Button = GemRB.GetView ("STOLBTN")
	Button.SetState (IE_GUI_BUTTON_LOCKED)
	Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_OR)
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
	Button.SetText (strrefs["done"])
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, lambda: Window.Close())

	# hide the empty button
	if GameCheck.IsBG2() or GameCheck.IsIWD2():
		Window.DeleteControl (9)

	Window.ShowModal (MODAL_SHADOW_GRAY)
	return

def StealPressed ():
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

def RedrawStoreStealWindow (Window):
	UpdateStoreCommon (Window, "STOTITLE", "STONAME", "STOGOLD")
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
		Button = GemRB.GetView ('SWLBTN' + str(i))
		Label = GemRB.GetView ('SWLLBL' + str(i))
		Button.SetVarAssoc ("LeftIndex", LeftTopIndex+i)
		SetupItems (pc, Slot, Button, Label, i, ITEM_STORE, idx, 1)

		if i+RightTopIndex<RightCount:
			Slot = GemRB.GetSlotItem (pc, inventory_slots[i+RightTopIndex])
		else:
			Slot = None
		Button = GemRB.GetView ('SWRBTN' + str(i))
		Label = GemRB.GetView ('SWRLBL' + str(i))
		Button.SetVarAssoc ("RightIndex", RightTopIndex+i)
		SetupItems (pc, Slot, Button, Label, i, ITEM_PC, idx, 1)

	selected_count = 0
	for i in range (LeftCount):
		Flags = GemRB.IsValidStoreItem (pc, i, ITEM_STORE)
		if Flags & SHOP_SELECT:
			selected_count += 1

	# shade the inventory icon if it is full
	free_slots = len(GemRB.GetSlots (pc, SLOT_INVENTORY, -1))
	Button = Window.GetControl (37)
	if Button:
		if free_slots == 0:
			Button.SetState (IE_GUI_BUTTON_PRESSED)
		else:
			Button.SetState (IE_GUI_BUTTON_LOCKED)

	# also disable the button if the inventory is full
	if LeftIndex>=0 and selected_count <= free_slots:
		LeftButton.SetState (IE_GUI_BUTTON_ENABLED)
	else:
		LeftButton.SetState (IE_GUI_BUTTON_DISABLED)

	if GameCheck.IsPST():
		GUICommon.SetEncumbranceLabels (Window, 22, None, pc, True)
	else:
		GUICommon.SetEncumbranceLabels (Window, 0x10000043, 0x10000044, pc)
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

			if GemRB.CanUseItemType (SLOT_ANY, Slot['ItemResRef'], pc):
				Button.EnableBorder (1, 0)
			else:
				Button.EnableBorder (1, 1)

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
			if GameCheck.IsIWD1() or GameCheck.IsIWD2():
				LabelText = GemRB.GetString (24890)
			elif GameCheck.IsBG2():
				LabelText = GemRB.GetString (28337)
			else:
				LabelText = ""
		else:
			GemRB.SetToken ("ITEMCOST", str(Price) )
			LabelText = GemRB.GetString (strrefs["itemnamecost"])
		if GameCheck.IsPST():
			LabelText = GemRB.GetString (strrefs["itemnamecost"])
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

	if Item['MaxStackAmount']>1:
		price = price * Slot['Usages0']
	elif Item['MaxCharge']>0:
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
			# jewelry doesn't suffer from deprecation, at least in BG2
			if Item['Type'] in [CommonTables.ItemType.GetRowIndex ("GEM"),
                                            CommonTables.ItemType.GetRowIndex ("RING"),
                                            CommonTables.ItemType.GetRowIndex ("AMULET")]:
                                count = 0
			if count > 2:
				count = 2
			mod -= count * Store['Depreciation']
	else:
		# charisma modifier (in percent)
		mod += GemRB.GetAbilityBonus (IE_CHR, GemRB.GetPlayerStat (BarteringPC, IE_CHR) - 1, 0)

		# reputation modifier (in percent, but absolute)
		if RepModTable:
			mod = mod * RepModTable.GetValue (0, GemRB.GameGetReputation()/10 - 1) / 100

	return price * mod / 100

def IncrementDonation ():
	Window = GemRB.GetView('WINDONAT')

	Field = Window.GetControl (5)
	donation = int("0"+Field.QueryText ())
	if donation<GemRB.GameGetPartyGold ():
		Field.SetText (str(donation+1) )
	else:
		Field.SetText (str(GemRB.GameGetPartyGold ()) )
	UpdateStoreDonateWindow (Window)
	return

def DecrementDonation ():
	Window = GemRB.GetView('WINDONAT')

	Field = Window.GetControl (5)
	donation = int("0"+Field.QueryText ())
	if donation>0:
		Field.SetText (str(donation-1) )
	else:
		Field.SetText (str(0) )
	UpdateStoreDonateWindow (Window)
	return

def DonateGold ():
	Window = GemRB.GetView('WINDONAT')

	TextArea = Window.GetControl (0)
	TextArea.SetFlags (IE_GUI_TEXTAREA_AUTOSCROLL)

	Button = Window.GetControl (10)
	if Button:
		Button.SetAnimation ("DONATE")

	Field = Window.GetControl (5)
	donation = int("0"+Field.QueryText ())
	GemRB.GameSetPartyGold (GemRB.GameGetPartyGold ()-donation)
	if GemRB.IncreaseReputation (donation):
		TextArea.Append (strrefs['donorgood'])
		GemRB.PlaySound (DEF_DONATE1)
		UpdateStoreDonateWindow (Window)
		return

	TextArea.Append (strrefs['donorfail'])
	GemRB.PlaySound (DEF_DONATE2)
	UpdateStoreDonateWindow (Window)
	return

def InfoHealWindow ():
	Window = GemRB.GetView('WINHEAL')

	UpdateStoreHealWindow (Window)
	Index = GemRB.GetVar ("Index")
	Cure = GemRB.GetStoreCure (Index)
	Spell = GemRB.GetSpell (Cure['CureResRef'])

	Window = GemRB.LoadWindow (windowIDs["cureinfo"], "GUISTORE", WINDOW_HCENTER|WINDOW_TOP)

	Label = Window.GetControl (0x10000000)
	Label.SetText (Spell['SpellName'])

	Button = GemRB.GetView ("STOLBTN")
	Button.SetSpellIcon (Cure['CureResRef'], 1)
	if GameCheck.IsPST():
		Window.ReassignControls ((3,4), (5,3))
		Button = Window.GetControl (6)
		Button.SetSpellIcon (Cure['CureResRef'], 2)
	else:
		pass

	TextArea = Window.GetControl (3)
	TextArea.SetText (Spell['SpellDesc'])

	#Done
	Button = Window.GetControl (5)
	Button.SetText (strrefs["done"])
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, lambda: Window.Close())

	Window.ShowModal (MODAL_SHADOW_GRAY)
	return

def BuyHeal ():
	Window = GemRB.GetView('WINHEAL')

	Index = GemRB.GetVar ("Index")
	Cure = GemRB.GetStoreCure (Index)
	gold = GemRB.GameGetPartyGold ()
	if gold < Cure['Price']:
		ErrorWindow (strrefs["spelltoocostly"])
		return

	GemRB.GameSetPartyGold (gold-Cure['Price'])
	pc = GemRB.GameGetSelectedPCSingle ()
	Spell = GemRB.GetSpell (Cure['CureResRef'])
	# for anything but raise/resurrect, the talker should be the caster, so
	# self-targetting spells work properly. Raise dead is an exception as
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

def GulpDrink ():
	Window = GemRB.GetView('WINRUMOR')

	TextArea = Window.GetControl (13)
	TextArea.SetFlags (IE_GUI_TEXTAREA_AUTOSCROLL)
	pc = GemRB.GameGetSelectedPCSingle ()
	intox = GemRB.GetPlayerStat (pc, IE_INTOXICATION)
	if intox > 80:
		TextArea.Append (strrefs["toodrunk"])
		return

	gold = GemRB.GameGetPartyGold ()
	Index = GemRB.GetVar ("TopIndex")+GemRB.GetVar ("Index")
	Drink = GemRB.GetStoreDrink (Index)
	if gold < Drink['Price']:
		ErrorWindow (strrefs["drinktoocostly"])
		return

	GemRB.GameSetPartyGold (gold-Drink['Price'])
	GemRB.SetPlayerStat (pc, IE_INTOXICATION, intox+Drink['Strength'])
	text = GemRB.GetRumour (Drink['Strength'], Store['TavernRumour'])
	TextArea.Append (text)
	if text > -1: TextArea.Append ("\n\n")
	GemRB.PlaySound (DEF_DRUNK)
	UpdateStoreRumourWindow (Window)
	return

def RentConfirm (Window):
	RentIndex = GemRB.GetVar ("RentIndex")
	price = Store['StoreRoomPrices'][RentIndex]
	Gold = GemRB.GameGetPartyGold ()
	GemRB.GameSetPartyGold (Gold-price)
	# TODO: run GemRB.RunRestScripts ()
	cutscene = GemRB.RestParty (13, 1, RentIndex+1)
	if RentConfirmWindow:
		RentConfirmWindow.Unload ()
	Window = GemRB.GetView('WINRENT')
	if cutscene:
		CloseStoreWindow ()
	else:
		TextArea = Window.GetControl (12)
		#is there any way to change this???
		GemRB.SetToken ("HP", "%d"%(RentIndex+1))
		TextArea.SetText (strrefs["restedfor"])
		GemRB.SetVar ("RentIndex", -1)
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
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, RentConfirm)
	Button.MakeDefault()

	#deny
	Button = Window.GetControl (1)
	Button.SetText (strrefs["cancel"])
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, lambda: Window.Close())
	Button.MakeEscape()

	#textarea
	TextArea = Window.GetControl (3)
	TextArea.SetText (strrefs["confirmrest"])

	Window.ShowModal (MODAL_SHADOW_GRAY)
	return

def ErrorWindow (strref):
	Window = GemRB.LoadWindow (windowIDs["error"])

	TextArea = Window.GetControl (3)
	TextArea.SetText (strref)

	#done
	Button = Window.GetControl (0)
	Button.SetText (strrefs["done"])
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, lambda: Window.Close())

	Window.ShowModal (MODAL_SHADOW_GRAY)
	return

###################################################
# End of file GUISTORE.py
