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
# $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/GUIScripts/pst/GUIINV.py,v 1.22 2005/02/11 22:25:02 avenger_teambg Exp $


# GUIINV.py - scripts to control inventory windows from GUIINV winpack

###################################################

import string

from GUIDefines import *
import ie_stats
import GemRB
from GUICommon import CloseOtherWindow
from GUICommonWindows import GetActorPortrait, SetSelectionChangeHandler


InventoryWindow = None
ItemInfoWindow = None
ItemAmountWindow = None
ItemIdentifyWindow = None

ItemHash = {}

# Control ID's:
# 0 - 9 eye, rings, earrings, tattoos, etc
# 10-13 Quick Weapon
# 14-18 Quick Item
# 19-23 Quiver
# 24-43 Personal Item
# 47-56 Ground Item

# 44      - portrait
# 45      - ground scrollbar?
# 46      - encumbrance
# 0x10000000 + 57 - name
# 0x10000000 + 58 - AC
# 0x10000000 + 59 - hp
# 0x10000000 + 60 - hp max
# 0x10000000 + 61 - msg
# 0x10000000 + 62 - party gold
# 0x10000000 + 63 - class


# Maps from ControlID to inventory Slot number
ControlToSlotMap = [6, 7, 5, 3, 19, 0, 1, 4, 8, 2,   9, 10, 11, 12,   20, 21, 22, 23, 24,   13, 14, 15, 16, 17,   25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44]

def OpenInventoryWindow ():
	global AvSlotsTable
	global InventoryWindow
	
	AvSlotsTable  = GemRB.LoadTable ('AVSLOTS')

	if CloseOtherWindow (OpenInventoryWindow):
		GemRB.HideGUI ()
		GemRB.UnloadWindow (InventoryWindow)
		InventoryWindow = None
		GemRB.SetVar ("OtherWindow", -1)
		SetSelectionChangeHandler (None)
		GemRB.UnhideGUI ()
		return
		
	GemRB.HideGUI ()
	GemRB.LoadWindowPack ("GUIINV")
	InventoryWindow = Window = GemRB.LoadWindow(3)
        GemRB.SetVar("OtherWindow", InventoryWindow)


	# inventory slots
	for i in range (44):
		Button = GemRB.GetControl (Window, i)
		GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_ALIGN_RIGHT | IE_GUI_BUTTON_ALIGN_BOTTOM, OP_OR)
		#GemRB.SetButtonFont (Window, Button, 'NUMBER')
		GemRB.SetVarAssoc (Window, Button, 'ItemButton', i)

	# Ground Item
	for i in range (47, 57):
		Button = GemRB.GetControl (Window, i)
		GemRB.SetTooltip (Window, Button, 4273)
		GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_ALIGN_RIGHT | IE_GUI_BUTTON_ALIGN_BOTTOM, OP_OR)
		#GemRB.SetButtonFont (Window, Button, 'NUMBER')
	
	for i in range (57, 64):
		Label = GemRB.GetControl (Window, 0x10000000 + i)
		GemRB.SetText (Window, Label, str (i))
		

	# portrait
	Button = GemRB.GetControl (Window, 44)
	GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_LOCKED)
	GemRB.SetButtonFlags(Window, Button, IE_GUI_BUTTON_NO_IMAGE | IE_GUI_BUTTON_PICTURE, OP_SET)

	# encumbrance
	Button = GemRB.GetControl (Window, 46)
	GemRB.SetButtonFont (Window, Button, "NUMBER")
	GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_LOCKED)
	GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_NO_IMAGE, OP_SET)

	# armor class
	Label = GemRB.GetControl (Window, 0x1000003a)
	GemRB.SetTooltip (Window, Label, 4197)

	# hp current
	Label = GemRB.GetControl (Window, 0x1000003b)
	GemRB.SetTooltip (Window, Label, 4198)

	# hp max
	Label = GemRB.GetControl (Window, 0x1000003c)
	GemRB.SetTooltip (Window, Label, 4199)

	SetSelectionChangeHandler (UpdateInventoryWindow)
	UpdateInventoryWindow ()

	GemRB.UnhideGUI()



def UpdateInventoryWindow ():
	global ItemHash
	global slot_list

	Window = InventoryWindow

	pc = GemRB.GameGetSelectedPCSingle ()

	# name
	Label = GemRB.GetControl (Window, 0x10000039)
	GemRB.SetText (Window, Label, GemRB.GetPlayerName (pc, 1))
	

	# portrait
	Button = GemRB.GetControl (Window, 44)
	GemRB.SetButtonPicture (Window, Button, GetActorPortrait (pc, 'INVENTORY'))

	# encumbrance
	Button = GemRB.GetControl (Window, 46)

	# Current encumbrance
	encumbrance = GemRB.GetPlayerStat (pc, ie_stats.IE_ENCUMBRANCE)

	# Loading tables of modifications
	Table = GemRB.LoadTable("strmod")
	TableEx = GemRB.LoadTable("strmodex")
	# Getting the character's strength
	sstr = GemRB.GetPlayerStat (pc, ie_stats.IE_STR)
	ext_str = GemRB.GetPlayerStat (pc, ie_stats.IE_STREXTRA)

	max_encumb = GemRB.GetTableValue(Table, sstr, 3) + GemRB.GetTableValue(TableEx, ext_str, 3)
	# FIXME: there should be a space before LB symbol
	GemRB.SetText (Window, Button, str (encumbrance) + ":\n\n\n\n" + str (max_encumb) + ":")

 	ratio = (0.0 + encumbrance) / max_encumb
 	if ratio > 1.0:
 		GemRB.SetButtonTextColor (Window, Button, 255, 0, 0, True)
 	elif ratio > 0.8:
 		GemRB.SetButtonTextColor (Window, Button, 255, 255, 0, True)
 	else:
 		GemRB.SetButtonTextColor (Window, Button, 255, 255, 255, True)

		
	# FIXME: Current encumbrance is hardcoded
	# Unloading tables is not necessary, i think (they will stay cached)
	#GemRB.UnloadTable (Table)
	#GemRB.UnloadTable (TableEx)

	# armor class
	ac = GemRB.GetPlayerStat (pc, ie_stats.IE_ARMORCLASS)
	Label = GemRB.GetControl (Window, 0x1000003a)
	GemRB.SetText (Window, Label, str (ac))

	# hp current
	hp = GemRB.GetPlayerStat (pc, ie_stats.IE_HITPOINTS)
	Label = GemRB.GetControl (Window, 0x1000003b)
	GemRB.SetText (Window, Label, str (hp))

	# hp max
	hpmax = GemRB.GetPlayerStat (pc, ie_stats.IE_MAXHITPOINTS)
	Label = GemRB.GetControl (Window, 0x1000003c)
	GemRB.SetText (Window, Label, str (hpmax))

	# party gold
	Label = GemRB.GetControl (Window, 0x1000003e)
	GemRB.SetText (Window, Label, str (GemRB.GameGetPartyGold ()))

	# class
	ClassTable = GemRB.LoadTable ("classes")
	text = GemRB.GetTableValue (ClassTable, GemRB.GetPlayerStat (pc, ie_stats.IE_CLASS) - 1, 0)
	GemRB.UnloadTable (ClassTable)

	Label = GemRB.GetControl (Window, 0x1000003f)
	GemRB.SetText (Window, Label, text)


	# And now for the items ....

	ItemHash = {}

	# get a list which maps slot number to slot type/icon/toolti
	anim_id = GemRB.GetPlayerStat (pc, ie_stats.IE_ANIMATION_ID)
	#anim_id = GemRB.GetPlayerStat (pc, ie_stats.IE_ANIMATION_ID) & 0x7fff
	row = "0x%04X" %anim_id
	slot_list = map (int, string.split (GemRB.GetTableValue (AvSlotsTable, row, 'SLOTS'), ',')[1:])
	
	# populate inventory slot controls
	for i in range (44):
		UpdateSlot (pc, i)


def UpdateSlot (pc, ControlID):
	Window = InventoryWindow
	Button = GemRB.GetControl (Window, ControlID)
	slot = ControlToSlotMap[ControlID]
	slot_item = GemRB.GetSlotItem (pc, slot)

	slottype = slot_list[slot]
	slotdict = GemRB.GetSlotType (slottype)

	# NOTE: there are invisible items (e.g. MORTEP) in inaccessible slots
	#   used to assign powers and protections
	if slot_item and slottype:
		item = GemRB.GetItem (slot_item['ItemResRef'])
		identified = slot_item['Flags'] & IE_INV_ITEM_IDENTIFIED
		ItemHash[ControlID] = [slot, slot_item, item]
			
		GemRB.SetButtonSprites (Window, Button, 'IVSLOT', 0,  0, 0, 0, 0)
		GemRB.SetItemIcon (Window, Button, slot_item['ItemResRef'])
		if item['StackAmount'] > 1:
			GemRB.SetText (Window, Button, str (slot_item['Usages0']))
		else:
			GemRB.SetText (Window, Button, '')
		if not identified or item['ItemNameIdentified'] == -1:
			GemRB.SetTooltip (Window, Button, item['ItemName'])
		else:
			GemRB.SetTooltip (Window, Button, item['ItemNameIdentified'])
		GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_NO_IMAGE, OP_NAND)
		GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OnDragItem")
		GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_RIGHT_PRESS, "OpenItemInfoWindow")
		GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_SHIFT_PRESS, "OpenItemAmountWindow")
		GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_DRAG_DROP, "OnDragItem")
	else:

		GemRB.SetItemIcon (Window, Button, '')
		GemRB.SetText (Window, Button, '')
		
		if slottype:
			GemRB.SetButtonSprites (Window, Button, slotdict['ResRef'], 0,  0, 0, 0, 0)
			GemRB.SetTooltip (Window, Button, slotdict['Tip'])
			GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_NO_IMAGE, OP_NAND)
			GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_DRAG_DROP, "OnDragItem")
		else:
			GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_NO_IMAGE, OP_OR)
			GemRB.SetTooltip (Window, Button, '')
			GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_DRAG_DROP, '')

		GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "")
		GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_RIGHT_PRESS, "")
		GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_SHIFT_PRESS, "")



def OnDragItem ():
	pc = GemRB.GameGetSelectedPCSingle ()

	if not GemRB.IsDraggingItem ():
		slot, slot_item, item = ItemHash[GemRB.GetVar ('ItemButton')]
		GemRB.DragItem (pc, slot, item['ItemIcon'], 0, 0, 0)
	else:
		slot = ControlToSlotMap[GemRB.GetVar ('ItemButton')]
		GemRB.DropDraggedItem (pc, slot)
		
	UpdateInventoryWindow ()

def OnDropItemToPC ():
	pc = GemRB.GetVar ("PressedPortrait") + 1
	GemRB.DropDraggedItem (pc, -1)
	UpdateInventoryWindow ()
	

def OpenItemInfoWindow ():
	global ItemInfoWindow

	GemRB.HideGUI ()
	
	if ItemInfoWindow != None:
		GemRB.UnloadWindow (ItemInfoWindow)
		ItemInfoWindow = None
		GemRB.SetVar ("FloatWindow", -1)
		
		GemRB.UnhideGUI ()
		return
		
	ItemInfoWindow = Window = GemRB.LoadWindow (5)
        GemRB.SetVar ("FloatWindow", ItemInfoWindow)

	slot, slot_item, item = ItemHash[GemRB.GetVar ('ItemButton')]
	ResRef = slot_item['ItemResRef']

	identified = slot_item['Flags'] & IE_INV_ITEM_IDENTIFIED

	item = GemRB.GetItem (ResRef)

	# Done
	Button = GemRB.GetControl (Window, 4)
	GemRB.SetText (Window, Button, 1403)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenItemInfoWindow")

	# Identify
	Button = GemRB.GetControl (Window, 8)
	GemRB.SetText (Window, Button, 4256)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenItemIdentifyWindow")
	# FIXME: the button should be hidden if not needed
	if identified:
		GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_DISABLED)
	else:
		GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_ENABLED)

	# Talk to Item/Use
	Button = GemRB.GetControl (Window, 9)
	if item['Dialog']:
		GemRB.SetText (Window, Button, 4254)
		GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_ENABLED)
	else:
		GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_DISABLED)

	Label = GemRB.GetControl (Window, 0x10000000)
	if not identified or item['ItemNameIdentified'] == -1:
		GemRB.SetText (Window, Label, string.upper (GemRB.GetString (item['ItemName'])))
	else:
		GemRB.SetText (Window, Label, string.upper (GemRB.GetString (item['ItemNameIdentified'])))

	Icon = GemRB.GetControl (Window, 2)
	GemRB.SetButtonFlags (Window, Icon, IE_GUI_BUTTON_PICTURE | IE_GUI_BUTTON_NO_IMAGE, OP_SET)
	GemRB.SetItemIcon (Window, Icon, ResRef)

	Text = GemRB.GetControl (Window, 5)
	if not identified or item['ItemDescIdentified'] == -1:
		GemRB.SetText (Window, Text, item['ItemDesc'])
	else:
		GemRB.SetText (Window, Text, item['ItemDescIdentified'])



	IdText = GemRB.GetControl (Window, 0x1000000b)
	if identified:
		GemRB.SetText (Window, IdText, '')
	else:
		# 4279 - This item has not been identified
		GemRB.SetText (Window, IdText, 4279)

	
	Icon = GemRB.GetControl (Window, 7)
	GemRB.SetButtonFlags (Window, Icon, IE_GUI_BUTTON_PICTURE | IE_GUI_BUTTON_NO_IMAGE, OP_SET)
	GemRB.SetItemIcon (Window, Icon, ResRef, 1)

	# 9, 8, 4


	GemRB.UnhideGUI ()
	GemRB.ShowModal (Window, MODAL_SHADOW_GRAY)

# 4254 Talk to item/4251 Use   4256 Identify Done
# 4252 Copy Spell (also 4250, 4249)
# 4253 eat berries

# 4258 - How would you like to identify
# 4259 Spell/4260 Scroll//Cancel

# amnt dialog:
# Done/Cancel



def OpenItemAmountWindow ():
	global ItemAmountWindow

	GemRB.HideGUI ()
	
	if ItemAmountWindow != None:
		GemRB.UnloadWindow (ItemAmountWindow)
		ItemAmountWindow = None
		GemRB.SetVar ("FloatWindow", -1)
		
		GemRB.UnhideGUI ()
		return
		
	ItemAmountWindow = Window = GemRB.LoadWindow (4)
        GemRB.SetVar ("FloatWindow", ItemAmountWindow)

	ResRef = 'AEGIS'
	item = GemRB.GetItem (ResRef)

	# item icon
	Icon = GemRB.GetControl (Window, 0)
	GemRB.SetButtonFlags (Window, Icon, IE_GUI_BUTTON_PICTURE | IE_GUI_BUTTON_NO_IMAGE, OP_SET)
	GemRB.SetItemIcon (Window, Icon, ResRef)

	# item amount
	Text = GemRB.GetControl (Window, 6)
	GemRB.SetControlSize (Window, Text, 40, 40)
	GemRB.SetText (Window, Text, "1")

	# Done
	Button = GemRB.GetControl (Window, 2)
	GemRB.SetText (Window, Button, 1403)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenItemAmountWindow")

	# Cancel
	Button = GemRB.GetControl (Window, 1)
	GemRB.SetText (Window, Button, 4196)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenItemAmountWindow")


	# 0 bmp
	# 1,2 done/cancel?
	# 3 +
	# 4 -
	# 6 text

	GemRB.UnhideGUI ()
	GemRB.ShowModal (Window, MODAL_SHADOW_GRAY)



def OpenItemIdentifyWindow ():
	global ItemIdentifyWindow

	GemRB.HideGUI ()
	
	if ItemIdentifyWindow != None:
		GemRB.UnloadWindow (ItemIdentifyWindow)
		ItemIdentifyWindow = None
		GemRB.SetVar ("FloatWindow", ItemInfoWindow)
		
		GemRB.UnhideGUI ()
		GemRB.ShowModal (ItemInfoWindow, MODAL_SHADOW_GRAY)
		return
		
	ItemIdentifyWindow = Window = GemRB.LoadWindow (9)
        GemRB.SetVar ("FloatWindow", ItemIdentifyWindow)


	# how would you like to identify ....
	Text = GemRB.GetControl (Window, 3)
	GemRB.SetText (Window, Text, 4258)

	# Spell
	Button = GemRB.GetControl (Window, 0)
	GemRB.SetText (Window, Button, 4259)
	GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_DISABLED)
	#GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenItemAmountWindow")

	# Scroll
	Button = GemRB.GetControl (Window, 1)
	GemRB.SetText (Window, Button, 4260)
	GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_DISABLED)
	#GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenItemAmountWindow")

	# Cancel
	Button = GemRB.GetControl (Window, 2)
	GemRB.SetText (Window, Button, 4196)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenItemIdentifyWindow")


	GemRB.UnhideGUI ()
	GemRB.ShowModal (Window, MODAL_SHADOW_GRAY)
	


# IVIARW
#IVISWRD

#IVPIBOW
#IVPICHST
#IVPIEARL
#IVPIEARR
#IVPIEYER
#IVPIHNDL/R
#IVPILENS
#IVPIRING
#IVPITATT
#IVPITETH
#IVPIWRST
#IVQIARRW
#IVQIITM
#IVQIWEAP

# 6 eye
# 9-12 quick weapon
# 20-24 quick item
# 25-44 personal item

# default weapons:
# Annah's punch daggers
# Morte's bite
# Grace's touch
# Ignus? fireball?

###################################################
# End of file GUIINV.py
