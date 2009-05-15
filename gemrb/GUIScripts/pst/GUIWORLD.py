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
from GUIClasses import GWindow

ContainerWindow = None
FormationWindow = None
ReformPartyWindow = None

Container = None

def CloseContinueWindow ():
	GemRB.SetVar ("DialogChoose", GemRB.GetVar ("DialogOption"))
	Window = GWindow(GemRB.GetVar ("MessageWindow"))
	Button = Window.GetControl (0)
	Button.SetText(28082)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "OnDecreaseSize")


def OpenEndMessageWindow ():
	Window = GWindow(GemRB.GetVar ("MessageWindow"))
	Button = Window.GetControl (0)
	Button.SetText (34602)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "CloseContinueWindow")


def OpenContinueMessageWindow ():
	Window = GWindow(GemRB.GetVar ("MessageWindow"))
	Button = Window.GetControl (0)
	Button.SetText (34603)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "CloseContinueWindow")


def OpenContainerWindow ():
	global ContainerWindow, Container

	if ContainerWindow:
		return

	hideflag = GemRB.HideGUI ()

	GemRB.LoadWindowPack (GetWindowPack())
	ContainerWindow = Window = GemRB.LoadWindowObject (8)
	DisableAnimatedWindows ()
	GemRB.SetVar ("ActionsWindow", Window.ID)

	Container = GemRB.GetContainer(0)

	# 0 - 5 - Ground Item
	# 10 - 13 - Personal Item
	# 50 hand
	# 52, 53 scroller ground, scroller personal
	# 54 - encumbrance
	# 0x10000036 - label gold

	for i in range (6):
		Button = Window.GetControl (i)
		Button.SetFlags (IE_GUI_BUTTON_ALIGN_RIGHT | IE_GUI_BUTTON_ALIGN_BOTTOM, OP_OR)
		#Button.SetFont ('NUMBER')

	for i in range (4):
		Button = Window.GetControl (10 + i)
		Button.SetFlags (IE_GUI_BUTTON_ALIGN_RIGHT | IE_GUI_BUTTON_ALIGN_BOTTOM, OP_OR)
		#Button.SetFont ('NUMBER')
		

	Button = Window.GetControl (50)
	Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_SET)

	Button = Window.GetControl (54)
	Button.SetFont ("NUMBER")
	Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_SET)

	party_gold = GemRB.GameGetPartyGold ()
	Text = Window.GetControl (0x10000036)
	Text.SetText (str (party_gold))


	Count = 1

	# Ground items scrollbar
	ScrollBar = Window.GetControl (52)
	ScrollBar.SetEvent(IE_GUI_SCROLLBAR_ON_CHANGE, "RedrawContainerWindow")
	GemRB.SetVar ("LeftTopIndex", 0)
	ScrollBar.SetVarAssoc ("LeftTopIndex", Count)

	# Personal items scrollbar
	ScrollBar = Window.GetControl (53)
	ScrollBar.SetEvent(IE_GUI_SCROLLBAR_ON_CHANGE, "RedrawContainerWindow")
	GemRB.SetVar ("RightTopIndex", 0)
	ScrollBar.SetVarAssoc ("RightTopIndex", Count)


	# Done
	Button = Window.GetControl (51)
	Button.SetText (1403)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "LeaveContainer")

	UpdateContainerWindow ()

	if hideflag:
		GemRB.UnhideGUI ()


def UpdateContainerWindow ():
	global Container

	Window = ContainerWindow

	pc = GemRB.GameGetFirstSelectedPC ()
	SetEncumbranceButton (Window, 54, pc)

	party_gold = GemRB.GameGetPartyGold ()
	Text = Window.GetControl (0x10000036)
	Text.SetText (str (party_gold))

	Container = GemRB.GetContainer (0) #will use first selected pc anyway
	LeftCount = Container['ItemCount']
	ScrollBar = Window.GetControl (52)
	Count = LeftCount / 3
	if Count < 1:
		Count = 1
	ScrollBar.SetVarAssoc ("LeftTopIndex", Count)
	
	inventory_slots = GemRB.GetSlots (pc, 0x8000)
	RightCount = len (inventory_slots)
	ScrollBar = Window.GetControl (53)
	Count = RightCount / 2
	if Count < 1:
		Count = 1
	ScrollBar.SetVarAssoc ("RightTopIndex", Count)

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
		Button = Window.GetControl (i)


		if Slot != None:
			SetItemButton (Window, Button, Slot, 'TakeItemContainer', '')
			Button.SetVarAssoc ("LeftIndex", LeftTopIndex+i)
		else:
			SetItemButton (Window, Button, Slot, '', '')
			Button.SetVarAssoc ("LeftIndex", -1)


	for i in range(4):
		if i + RightTopIndex < RightCount:
			Slot = GemRB.GetSlotItem (pc, inventory_slots[i+RightTopIndex])
		else:
			Slot = None
		Button = Window.GetControl (i + 10)
		
		SetItemButton (Window, Button, Slot, '', '')

		if Slot != None:
			SetItemButton (Window, Button, Slot, 'DropItemContainer', '')
			Button.SetVarAssoc ("RightIndex", RightTopIndex+i)
		else:
			SetItemButton (Window, Button, Slot, '', '')
			Button.SetVarAssoc ("RightIndex", -1)



def CloseContainerWindow ():
	global ContainerWindow

	if ContainerWindow == None:
		return

	hideflag = GemRB.HideGUI ()

	if ContainerWindow:
		ContainerWindow.Unload ()
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

	if CloseOtherWindow(OpenReformPartyWindow):
		GemRB.HideGUI ()
		if ReformPartyWindow:
			ReformPartyWindow.Unload ()
		ReformPartyWindow = None

		GemRB.SetVar ("OtherWindow", -1)
		EnableAnimatedWindows ()
		GemRB.LoadWindowPack ("GUIREC")
		GemRB.UnhideGUI ()
		return

	GemRB.HideGUI ()
	GemRB.LoadWindowPack (GetWindowPack())
	ReformPartyWindow = Window = GemRB.LoadWindowObject (24)
	GemRB.SetVar ("OtherWindow", Window.ID)
	DisableAnimatedWindows ()

	# Remove
	Button = Window.GetControl (15)
	Button.SetText (42514)
	Button.SetState (IE_GUI_BUTTON_DISABLED)

	# Done
	Button = Window.GetControl (8)
	Button.SetText (1403)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "OpenReformPartyWindow")

	GemRB.UnhideGUI ()


last_formation = None

def OpenFormationWindow ():
	global FormationWindow

	if CloseOtherWindow(OpenFormationWindow):
		GemRB.HideGUI ()
		if FormationWindow:
			FormationWindow.Unload ()
		FormationWindow = None

		GemRB.GameSetFormation (last_formation, 0)
		EnableAnimatedWindows ()
		GemRB.SetVar ("OtherWindow", -1)
		GemRB.UnhideGUI ()
		return

	GemRB.HideGUI ()
	GemRB.LoadWindowPack (GetWindowPack())
	FormationWindow = Window = GemRB.LoadWindowObject (27)
	GemRB.SetVar ("OtherWindow", Window.ID)
	DisableAnimatedWindows ()

	# Done
	Button = Window.GetControl (13)
	Button.SetText (1403)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "OpenFormationWindow")

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
		Button = Window.GetControl (i)
		Button.SetVarAssoc ("SelectedFormation", i)
		Button.SetTooltip (tooltips[i])
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "SelectFormation")

	GemRB.SetVar ("SelectedFormation", GemRB.GameGetFormation (0))
	SelectFormation ()

	GemRB.UnhideGUI ()

def SelectFormation ():
	global last_formation
	Window = FormationWindow
	
	formation = GemRB.GetVar ("SelectedFormation")
	print "FORMATION:", formation
	if last_formation != None and last_formation != formation:
		Button = Window.GetControl (last_formation)
		Button.SetState (IE_GUI_BUTTON_UNPRESSED)

	Button = Window.GetControl (formation)
	Button.SetState (IE_GUI_BUTTON_SELECTED)

	last_formation = formation


def GetWindowPack():
	width = GemRB.GetSystemVariable (SV_WIDTH)
	if width == 800:
		return "GUIW08"
	#default
	return "GUIWORLD"
