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
# $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/GUIScripts/pst/GUIINV.py,v 1.5 2004/04/27 06:23:10 edheldil Exp $


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
	
	for i in range (57, 64):
		Label = GemRB.GetControl (Window, 0x10000000 + i);
		GemRB.SetText (Window, Label, str (i))
	
	Button = GemRB.GetControl (Window, 10);
	GemRB.SetItemIcon (Window, Button, "AEGIS")
	
	Button = GemRB.GetControl (Window, 11);
	GemRB.SetItemIcon (Window, Button, "COPEARC")


	# portrait
	Button = GemRB.GetControl (Window, 44);
	GemRB.SetButtonFlags(Window, Button, IE_GUI_BUTTON_NO_IMAGE | IE_GUI_BUTTON_PICTURE, OP_SET)

	# encumbrance
	Button = GemRB.GetControl (Window, 46);
	GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_NO_IMAGE, OP_SET)


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
	GemRB.SetButtonPicture (Window, Button, GetActorPortrait (pc, 0))

	# encumbrance
	Button = GemRB.GetControl (Window, 46);
	GemRB.SetText (Window, Button, "0\n66lb")

	# armor class
	ac = GemRB.GetPlayerStat (pc, ie_stats.IE_ARMORCLASS)
	Label = GemRB.GetControl (Window, 0x1000003a)
	GemRB.SetText (Window, Label, str (ac))

	# hp current
	hp = GemRB.GetPlayerStat (pc, ie_stats.IE_HITPOINTS)
	Label = GemRB.GetControl (Window, 0x1000003c)
	GemRB.SetText (Window, Label, str (hp))

	# hp max
	hpmax = GemRB.GetPlayerStat (pc, ie_stats.IE_MAXHITPOINTS)
	Label = GemRB.GetControl (Window, 0x1000003b)
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
