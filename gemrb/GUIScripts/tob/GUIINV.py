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
# $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/GUIScripts/tob/GUIINV.py,v 1.4 2004/08/28 21:11:48 avenger_teambg Exp $


# GUIINV.py - scripts to control inventory windows from GUIINV winpack

###################################################

from GUIDefines import *
from ie_stats import *
import GemRB
from GUICommonWindows import GetActorPaperDoll, SetSelectionChangeHandler
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
	InventoryWindow = Window = GemRB.LoadWindow(2)
	GemRB.SetVar("OtherWindow", InventoryWindow)

#11997 Armor
#11998 Gauntlets
#11999 Helmet
#12000 Amulet
#12001 Belt
#12002 Left Ring
#12003 Righ Ring
#12004 Cloak
#12005 Boots
#12006 Sield
#12007  major
#12008  minor
	# Quick Weapon
	for i in range (1, 4):
		Button = GemRB.GetControl (Window, i);
		GemRB.SetTooltip (Window, Button, 12010)

	# Quick Item
	for i in range (5, 7):
		Button = GemRB.GetControl (Window, i);
		GemRB.SetTooltip (Window, Button, 12012)

	for i in range (11, 14):
		Button = GemRB.GetControl (Window, i);
		GemRB.SetTooltip (Window, Button, 4274)

	# Quiver
	for i in range (15, 17):
		Button = GemRB.GetControl (Window, i);
		GemRB.SetTooltip (Window, Button, 12009)

	# Personal Item
	for i in range (30, 45):
		Button = GemRB.GetControl (Window, i);
		GemRB.SetTooltip (Window, Button, 12013)

	# Ground Item
	for i in range (68, 72):
		Button = GemRB.GetControl (Window, i);
		GemRB.SetTooltip (Window, Button, 12011)
	
	Button = GemRB.GetControl (Window, 30);
	GemRB.SetItemIcon (Window, Button, "SW1H02")
	
	Button = GemRB.GetControl (Window, 68);
	GemRB.SetItemIcon (Window, Button, "MISC07")

	#major & minor clothing color
	Button = GemRB.GetControl (Window, 62);
	GemRB.SetButtonFlags(Window, Button, IE_GUI_BUTTON_PICTURE,OP_OR)
	GemRB.SetEvent(Window, Button, IE_GUI_BUTTON_ON_PRESS,"MajorPress")

	Button = GemRB.GetControl (Window, 63);
	GemRB.SetButtonFlags(Window, Button, IE_GUI_BUTTON_PICTURE,OP_OR)
	GemRB.SetEvent(Window, Button, IE_GUI_BUTTON_ON_PRESS,"MinorPress")

	# portrait
	Button = GemRB.GetControl (Window, 50);
	GemRB.SetButtonState(Window, Button, IE_GUI_BUTTON_LOCKED);
	GemRB.SetButtonFlags(Window, Button, IE_GUI_BUTTON_NO_IMAGE | IE_GUI_BUTTON_PICTURE, OP_SET)

	# encumbrance
	#Button = GemRB.GetControl (Window, 46);
	#GemRB.SetButtonFont (Window, Button, "NUMBER")
	#GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_NO_IMAGE, OP_SET)

	# armor class
	Label = GemRB.GetControl (Window, 0x10000038)
	GemRB.SetTooltip (Window, Label, 4197)

	# hp current
	Label = GemRB.GetControl (Window, 0x10000039)
	GemRB.SetTooltip (Window, Label, 4198)

	# hp max
	Label = GemRB.GetControl (Window, 0x1000003a)
	GemRB.SetTooltip (Window, Label, 4199)

	#ground icons scrollbar
	ScrollBar = GemRB.GetControl (Window, 66)
	
	#info label, game paused, etc
	Label = GemRB.GetControl (Window, 0x1000003f)
	GemRB.SetText (Window, Label, "")

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
	return

def ColorDonePress():
	pc = GemRB.GameGetSelectedPCSingle ()

	GemRB.UnloadWindow(ColorPicker)
	GemRB.SetVisible(InventoryWindow,1)
	ColorTable = GemRB.LoadTable("clowncol")
	PickedColor=GemRB.GetTableValue(ColorTable, ColorIndex, GemRB.GetVar("Selected"))
	GemRB.UnloadTable(ColorTable)
	if ColorIndex==2:
		GemRB.SetPlayerStat (pc, IE_MAJOR_COLOR, PickedColor)
		UpdateInventoryWindow()
		return
	GemRB.SetPlayerStat (pc, IE_MINOR_COLOR, PickedColor)
	UpdateInventoryWindow()
	return

def MajorPress():
	global ColorIndex, PickedColor

	pc = GemRB.GameGetSelectedPCSingle ()
	ColorIndex = 2
	PickedColor = GemRB.GetPlayerStat (pc, IE_MAJOR_COLOR)
	GetColor()
	return

def MinorPress():
	global ColorIndex, PickedColor

	pc = GemRB.GameGetSelectedPCSingle ()
	ColorIndex = 3
	PickedColor = GemRB.GetPlayerStat (pc, IE_MINOR_COLOR)
	GetColor()
	return

def GetColor():
	global ColorPicker

	ColorTable = GemRB.LoadTable("clowncol")
	GemRB.SetVisible(InventoryWindow,2) #darken it
	ColorPicker=GemRB.LoadWindow(3)
	GemRB.SetVar("Selected",-1)
	for i in range(0,34):
		Button = GemRB.GetControl(ColorPicker, i)
		GemRB.SetButtonState(ColorPicker, Button, IE_GUI_BUTTON_DISABLED)
		GemRB.SetButtonFlags(ColorPicker, Button, IE_GUI_BUTTON_PICTURE|IE_GUI_BUTTON_RADIOBUTTON,OP_OR)

	Selected = -1
	for i in range(0,34):
		MyColor = GemRB.GetTableValue(ColorTable, ColorIndex, i)
		if MyColor == "*":
			break
		Button = GemRB.GetControl(ColorPicker, i)
		GemRB.SetButtonBAM(ColorPicker, Button, "COLGRAD", 2, 0, MyColor)
		if PickedColor == MyColor:
			GemRB.SetVar("Selected",i)
			Selected = i
		GemRB.SetButtonState(ColorPicker, Button, IE_GUI_BUTTON_ENABLED)
		GemRB.SetVarAssoc(ColorPicker, Button, "Selected",i)
		GemRB.SetEvent(ColorPicker, Button, IE_GUI_BUTTON_ON_PRESS, "ColorDonePress")
	GemRB.UnloadTable(ColorTable)
	GemRB.SetVisible(ColorPicker,1)
	return


def UpdateInventoryWindow ():
	Window = InventoryWindow

	pc = GemRB.GameGetSelectedPCSingle ()

	# name
	Label = GemRB.GetControl (Window, 0x10000039)
	GemRB.SetText (Window, Label, GemRB.GetPlayerName (pc, 0))

	# portrait
	Button = GemRB.GetControl (Window, 50);
	Color1 = GemRB.GetPlayerStat (pc, IE_HAIR_COLOR)
	Color2 = GemRB.GetPlayerStat (pc, IE_SKIN_COLOR)
	Color3 = GemRB.GetPlayerStat (pc, IE_MAJOR_COLOR)
	Color4 = GemRB.GetPlayerStat (pc, IE_MINOR_COLOR)
	GemRB.SetButtonPLT (Window, Button, GetActorPaperDoll (pc),
		Color2, Color1, Color3, Color4, 0, 0, 0, 0)

	# encumbrance
	#Button = GemRB.GetControl (Window, 46);
	#GemRB.SetText (Window, Button, "\001\n\n\n\n\007\007\007\013")

	# armor class
	ac = GemRB.GetPlayerStat (pc, IE_ARMORCLASS)
	Label = GemRB.GetControl (Window, 0x10000038)
	GemRB.SetText (Window, Label, str (ac))

	# hp current
	hp = GemRB.GetPlayerStat (pc, IE_HITPOINTS)
	Label = GemRB.GetControl (Window, 0x10000039)
	GemRB.SetText (Window, Label, str (hp))

	# hp max
	hpmax = GemRB.GetPlayerStat (pc, IE_MAXHITPOINTS)
	Label = GemRB.GetControl (Window, 0x1000003a)
	GemRB.SetText (Window, Label, str (hpmax))

	# party gold
	Label = GemRB.GetControl (Window, 0x10000040)
	GemRB.SetText (Window, Label, str (GemRB.GameGetPartyGold ()))

	# class (but this is set by a stat)
	text = GemRB.GetPlayerStat (pc, IE_TITLE1)
	Label = GemRB.GetControl (Window, 0x10000042)
	GemRB.SetText (Window, Label, text)

	Button = GemRB.GetControl (Window, 62);
	Color = GemRB.GetPlayerStat (pc, IE_MAJOR_COLOR)
	GemRB.SetButtonBAM(Window, Button, "COLGRAD", 1, 0, Color)

	Button = GemRB.GetControl (Window, 63);
	Color = GemRB.GetPlayerStat (pc, IE_MINOR_COLOR)
	GemRB.SetButtonBAM(Window, Button, "COLGRAD", 1, 0, Color)
	return

###################################################
# End of file GUIINV.py
