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
# $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/GUIScripts/pst/GUIINV.py,v 1.10 2004/08/27 14:17:44 edheldil Exp $


# GUIINV.py - scripts to control inventory windows from GUIINV winpack

###################################################

from GUIDefines import *
import ie_stats
import GemRB
#from GUICommonWindows import OpenCommonWindows, CloseCommonWindows
from GUICommonWindows import GetActorPortrait, SetSelectionChangeHandler
#import GUICommonWindows

InventoryWindow = None

def OpenInventoryWindow ():
	global InventoryWindow
	
	GemRB.HideGUI()
	
	if InventoryWindow != None:
		
		GemRB.UnloadWindow(InventoryWindow)
		InventoryWindow = None
		GemRB.SetVar("OtherWindow", -1)
		SetSelectionChangeHandler (None)
		GemRB.UnhideGUI()
		return
		
	GemRB.LoadWindowPack ("GUIINV")
	InventoryWindow = Window = GemRB.LoadWindow(3)
        GemRB.SetVar("OtherWindow", InventoryWindow)

	# 0 - 9   - body slots
	# 10 - 13 - weapons
	# 14 - 18 - quick items
	# 19 - 23 - arrows/missiles
	# 24 - 43 - items
	# 44      - portrait
	# 45      - ground scrollbar?
	# 46      - encumbrance
	# 47 - 56 - ground items
	# 0x10000000 + 57 - name
	# 0x10000000 + 58 - AC
	# 0x10000000 + 59 - hp
	# 0x10000000 + 60 - hp max
	# 0x10000000 + 61 - msg
	# 0x10000000 + 62 - party gold
	# 0x10000000 + 63 - class


	for i in range (10):
		Button = GemRB.GetControl (Window, i);
		GemRB.SetText (Window, Button, str (i))

	# Quick Weapon
	for i in range (10, 14):
		Button = GemRB.GetControl (Window, i);
		GemRB.SetTooltip (Window, Button, 4261)

	# Quick Item
	for i in range (14, 19):
		Button = GemRB.GetControl (Window, i);
		GemRB.SetTooltip (Window, Button, 4274)

	# Quiver
	for i in range (19, 24):
		Button = GemRB.GetControl (Window, i);
		GemRB.SetTooltip (Window, Button, 4262)

	# Personal Item
	for i in range (24, 44):
		Button = GemRB.GetControl (Window, i);
		GemRB.SetTooltip (Window, Button, 4275)

	# Ground Item
	for i in range (47, 57):
		Button = GemRB.GetControl (Window, i);
		GemRB.SetTooltip (Window, Button, 4273)
	
	for i in range (57, 64):
		Label = GemRB.GetControl (Window, 0x10000000 + i);
		GemRB.SetText (Window, Label, str (i))
	
	Button = GemRB.GetControl (Window, 10);
	GemRB.SetItemIcon (Window, Button, "AEGIS")
	
	Button = GemRB.GetControl (Window, 11);
	GemRB.SetItemIcon (Window, Button, "COPEARC")


	# portrait
	Button = GemRB.GetControl (Window, 44);
	GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_LOCKED)
	GemRB.SetButtonFlags(Window, Button, IE_GUI_BUTTON_NO_IMAGE | IE_GUI_BUTTON_PICTURE, OP_SET)

	# encumbrance
	Button = GemRB.GetControl (Window, 46);
	GemRB.SetButtonFont (Window, Button, "NUMBER")
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

	# 4263 Armor
	# 4264 Gauntlets
	# 4265 Helmet
	# 4266 Amulet
	# 4267 Belt
	# 4268 Left Ring
	# 4269 Right Ring
	# 4270 Cloak
	# 4271 Boots
	# 4272 Shield
	# 31580 Left Earring
	# 32857 Right Earring

	# 32795 Magnifying Lens
	# 32796 Helm
	# 32797 Tattoo
	# 32798 Chest
	# 32799 Teeth
	# 32800 Finger
	# 32801 Hand
	# 32802 Wrist
	# 32803 Crossbow
	# 32804 Quarrel
	

	SetSelectionChangeHandler (UpdateInventoryWindow)
	UpdateInventoryWindow ()

	GemRB.UnhideGUI()



def UpdateInventoryWindow ():
	Window = InventoryWindow

	pc = GemRB.GameGetSelectedPCSingle ()
	pc = pc + 1; 
	
	# name
	Label = GemRB.GetControl (Window, 0x10000039)
	GemRB.SetText (Window, Label, GemRB.GetPlayerName (pc, 0))
	

	# portrait
	Button = GemRB.GetControl (Window, 44);
	GemRB.SetButtonPicture (Window, Button, GetActorPortrait (pc, 'INVENTORY'))

	# encumbrance
	Button = GemRB.GetControl (Window, 46);
	GemRB.SetText (Window, Button, "\001\n\n\n\n\007\007\007\013")

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
	ClassTable = GemRB.LoadTable ("CLASS")
	text = GemRB.GetTableValue (ClassTable, GemRB.GetPlayerStat (pc, ie_stats.IE_CLASS) - 1, 0)
	GemRB.UnloadTable (ClassTable)

	Label = GemRB.GetControl (Window, 0x1000003f)
	GemRB.SetText (Window, Label, text)


###################################################
# End of file GUIINV.py
