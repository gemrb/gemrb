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
# $Id$


# GUIWORLD.py - scripts to control some windows from GUIWORLD winpack
#    except of Actions, Portrait, Options and Dialog windows

###################################################

import GemRB
from GUIDefines import *
from GUICommon import CloseOtherWindow
from GUICommonWindows import EnableAnimatedWindows, DisableAnimatedWindows, SetEncumbranceButton, SetItemButton, OpenWaitForDiscWindow

ContainerWindow = None
FormationWindow = None
ReformPartyWindow = None

Container = None

def CloseContinueWindow ():
	GemRB.SetVar ("DialogChoose", GemRB.GetVar ("DialogOption"))
	Window = GemRB.GetVar ("MessageWindow")
	Button = GemRB.GetControl (Window, 0)
	GemRB.SetText(Window, Button, 28082)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OnDecreaseSize")


def OpenEndMessageWindow ():
	Window = GemRB.GetVar ("MessageWindow")
	Button = GemRB.GetControl (Window, 0)
	GemRB.SetText (Window, Button, 34602)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "CloseContinueWindow")


def OpenContinueMessageWindow ():
	Window = GemRB.GetVar ("MessageWindow")
	Button = GemRB.GetControl (Window, 0)
	GemRB.SetText (Window, Button, 34603)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "CloseContinueWindow")


def OpenContainerWindow ():
	global ContainerWindow, Container

	if ContainerWindow:
		return

	hideflag = GemRB.HideGUI ()

	GemRB.LoadWindowPack (GetWindowPack())
	ContainerWindow = Window = GemRB.LoadWindow (8)
	DisableAnimatedWindows ()
	GemRB.SetVar ("ActionsWindow", Window)

	Container = GemRB.GetContainer(0)

	# 0 - 5 - Ground Item
	# 10 - 13 - Personal Item
	# 50 hand
	# 52, 53 scroller ground, scroller personal
	# 54 - encumbrance
	# 0x10000036 - label gold

	for i in range (6):
		Button = GemRB.GetControl (Window, i)
		GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_ALIGN_RIGHT | IE_GUI_BUTTON_ALIGN_BOTTOM, OP_OR)
		#GemRB.SetButtonFont (Window, Button, 'NUMBER')

	for i in range (4):
		Button = GemRB.GetControl (Window, 10 + i)
		GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_ALIGN_RIGHT | IE_GUI_BUTTON_ALIGN_BOTTOM, OP_OR)
		#GemRB.SetButtonFont (Window, Button, 'NUMBER')
		

	Button = GemRB.GetControl (Window, 50)
	GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_NO_IMAGE, OP_SET)

	Button = GemRB.GetControl (Window, 54)
	GemRB.SetButtonFont (Window, Button, "NUMBER")
	GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_NO_IMAGE, OP_SET)

	party_gold = GemRB.GameGetPartyGold ()
	Text = GemRB.GetControl (Window, 0x10000036)
	GemRB.SetText (Window, Text, str (party_gold))


	Count = 1

	# Ground items scrollbar
	ScrollBar = GemRB.GetControl (Window, 52)
	GemRB.SetEvent(Window, ScrollBar, IE_GUI_SCROLLBAR_ON_CHANGE, "RedrawContainerWindow")
	GemRB.SetVar ("LeftTopIndex", 0)
	GemRB.SetVarAssoc (Window, ScrollBar, "LeftTopIndex", Count)

	# Personal items scrollbar
	ScrollBar = GemRB.GetControl (Window, 53)
	GemRB.SetEvent(Window, ScrollBar, IE_GUI_SCROLLBAR_ON_CHANGE, "RedrawContainerWindow")
	GemRB.SetVar ("RightTopIndex", 0)
	GemRB.SetVarAssoc (Window, ScrollBar, "RightTopIndex", Count)


	# Done
	Button = GemRB.GetControl (Window, 51)
	GemRB.SetText (Window, Button, 1403)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "LeaveContainer")

	UpdateContainerWindow ()

	if hideflag:
		GemRB.UnhideGUI ()


def UpdateContainerWindow ():
	global Container

	Window = ContainerWindow

	pc = GemRB.GameGetFirstSelectedPC ()
	SetEncumbranceButton (Window, 54, pc)

	party_gold = GemRB.GameGetPartyGold ()
	Text = GemRB.GetControl (Window, 0x10000036)
	GemRB.SetText (Window, Text, str (party_gold))

	Container = GemRB.GetContainer (0) #will use first selected pc anyway
	LeftCount = Container['ItemCount']
	ScrollBar = GemRB.GetControl (Window, 52)
	Count = LeftCount / 3
	if Count < 1:
		Count = 1
	GemRB.SetVarAssoc (Window, ScrollBar, "LeftTopIndex", Count)
	
	inventory_slots = GemRB.GetSlots (pc, 0x8000)
	RightCount = len (inventory_slots)
	ScrollBar = GemRB.GetControl (Window, 53)
	Count = RightCount / 2
	if Count < 1:
		Count = 1
	GemRB.SetVarAssoc (Window, ScrollBar, "RightTopIndex", Count)

	RedrawContainerWindow ()


def RedrawContainerWindow ():
	Window = ContainerWindow

	LeftTopIndex = GemRB.GetVar ("LeftTopIndex") * 3
	LeftIndex = GemRB.GetVar ("LeftIndex")
	RightTopIndex = GemRB.GetVar ("RightTopIndex") * 2
	RightIndex = GemRB.GetVar ("RightIndex")
	LeftCount = Container['ItemCount']
	pc = GemRB.GameGetFirstSelectedPC ()
	inventory_slots = GemRB.GetSlots (pc, 0x8000)
	RightCount = len (inventory_slots)

	for i in range (6):
		#this is an autoselected container, but we could use PC too
		Slot = GemRB.GetContainerItem (0, i + LeftTopIndex)
		Button = GemRB.GetControl (Window, i)


		if Slot != None:
			SetItemButton (Window, Button, Slot, 'TakeItemContainer', '')
			GemRB.SetVarAssoc (Window, Button, "LeftIndex", LeftTopIndex+i)
		else:
			SetItemButton (Window, Button, Slot, '', '')
			GemRB.SetVarAssoc (Window, Button, "LeftIndex", -1)


	for i in range(4):
		if i + RightTopIndex < RightCount:
			Slot = GemRB.GetSlotItem (pc, inventory_slots[i+RightTopIndex])
		else:
			Slot = None
		Button = GemRB.GetControl (Window, i + 10)
		
		SetItemButton (Window, Button, Slot, '', '')

		if Slot != None:
			SetItemButton (Window, Button, Slot, 'DropItemContainer', '')
			GemRB.SetVarAssoc (Window, Button, "RightIndex", RightTopIndex+i)
		else:
			SetItemButton (Window, Button, Slot, '', '')
			GemRB.SetVarAssoc (Window, Button, "RightIndex", -1)



def CloseContainerWindow ():
	global ContainerWindow

	if ContainerWindow == None:
		return

	hideflag = GemRB.HideGUI ()

	GemRB.UnloadWindow (ContainerWindow)
	ContainerWindow = None
	EnableAnimatedWindows ()
	
	if hideflag:
		GemRB.UnhideGUI ()


#doing this way it will inform the core system too, which in turn will call
#CloseContainerWindow ()
def LeaveContainer ():
	GemRB.LeaveContainer()

def DropItemContainer ():
	RightIndex = GemRB.GetVar ("RightIndex")
	if RightIndex < 0:
		return
	
	#we need to get the right slot number
	pc = GemRB.GameGetFirstSelectedPC ()
	inventory_slots = GemRB.GetSlots (pc, 0x8000)
	if RightIndex >= len (inventory_slots):
		return
	
	GemRB.ChangeContainerItem (0, inventory_slots[RightIndex], 0)
	UpdateContainerWindow ()


def TakeItemContainer ():
	LeftIndex = GemRB.GetVar ("LeftIndex")
	if LeftIndex < 0:
		return
	
	if LeftIndex >= Container['ItemCount']:
		return
	
	GemRB.ChangeContainerItem (0, LeftIndex, 1)
	UpdateContainerWindow ()

	
def OpenReformPartyWindow ():
	global ReformPartyWindow

	if CloseOtherWindow(ReformPartyWindow):
		GemRB.HideGUI ()
		GemRB.UnloadWindow (ReformPartyWindow)
		ReformPartyWindow = None

		GemRB.SetVar ("OtherWindow", -1)
		EnableAnimatedWindows ()
		GemRB.LoadWindowPack ("GUIREC")
		GemRB.UnhideGUI ()
		return

	GemRB.HideGUI ()
	GemRB.LoadWindowPack (GetWindowPack())
	ReformPartyWindow = Window = GemRB.LoadWindow (24)
	GemRB.SetVar ("OtherWindow", Window)
	DisableAnimatedWindows ()

	# Remove
	Button = GemRB.GetControl (Window, 15)
	GemRB.SetText (Window, Button, 42514)
	GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_DISABLED)

	# Done
	Button = GemRB.GetControl (Window, 8)
	GemRB.SetText (Window, Button, 1403)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenReformPartyWindow")

	GemRB.UnhideGUI ()


last_formation = None

def OpenFormationWindow ():
	global FormationWindow

	if CloseOtherWindow(OpenFormationWindow):
		GemRB.HideGUI ()
		GemRB.UnloadWindow (FormationWindow)
		FormationWindow = None

		GemRB.GameSetFormation (last_formation, 0)
		EnableAnimatedWindows ()
		GemRB.SetVar ("OtherWindow", -1)
		GemRB.UnhideGUI ()
		return

	GemRB.HideGUI ()
	GemRB.LoadWindowPack (GetWindowPack())
	FormationWindow = Window = GemRB.LoadWindow (27)
	GemRB.SetVar ("OtherWindow", Window)
	DisableAnimatedWindows ()

	# Done
	Button = GemRB.GetControl (Window, 13)
	GemRB.SetText (Window, Button, 1403)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenFormationWindow")

	tooltips = (
		44957,  # Follow
		44958,  # T
		44959,  # Gather
		44960,  # 4 and 2
		44961,  # 3 by 2
		44962,  # Protect
		48152,  # 2 by 3
		44964,  # Rank
		44965,  # V
		44966,  # Wedge
		44967,  # S
		44968,  # Line
		44969,  # None
	)

	for i in range (13):
		Button = GemRB.GetControl (Window, i)
		GemRB.SetVarAssoc (Window, Button, "SelectedFormation", i)
		GemRB.SetTooltip (Window, Button, tooltips[i])
		GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "SelectFormation")

	GemRB.SetVar ("SelectedFormation", GemRB.GameGetFormation (0))
	SelectFormation ()

	GemRB.UnhideGUI ()

def SelectFormation ():
	global last_formation
	Window = FormationWindow
	
	formation = GemRB.GetVar ("SelectedFormation")
	print "FORMATION:", formation
	if last_formation != None and last_formation != formation:
		Button = GemRB.GetControl (Window, last_formation)
		GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_UNPRESSED)

	Button = GemRB.GetControl (Window, formation)
	GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_SELECTED)

	last_formation = formation


def GetWindowPack():
	width = GemRB.GetSystemVariable (SV_WIDTH)
	if width == 800:
		return "GUIW08"
	#default
	return "GUIWORLD"
