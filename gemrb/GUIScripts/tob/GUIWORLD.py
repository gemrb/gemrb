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
# $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/GUIScripts/tob/GUIWORLD.py,v 1.13 2005/06/20 17:15:24 avenger_teambg Exp $


# GUIW.py - scripts to control some windows from GUIWORLD winpack
#    except of Actions, Portrait, Options and Dialog windows

###################################################

import GemRB
from GUIDefines import *
from GUICommon import CloseOtherWindow
from GUICommonWindows import SetEncumbranceLabels

ContainerWindow = None
ContinueWindow = None
FormationWindow = None
ReformPartyWindow = None
OldActionsWindow = None
OldMessageWindow = None
Container = None

def CloseContinueWindow ():
	global ContinueWindow, OldActionsWindow

	if ContinueWindow == None:
		return
	GemRB.UnloadWindow (ContinueWindow)
	GemRB.SetVar ("ActionsWindow", OldActionsWindow)
	ContinueWindow = None
	OldActionsWindow = None
	GemRB.UnhideGUI ()


def OpenEndMessageWindow ():
	global ContinueWindow, OldActionsWindow

	GemRB.HideGUI ()

	if ContinueWindow:
		return

	GemRB.LoadWindowPack (GetWindowPack())
	ContinueWindow = Window = GemRB.LoadWindow (9)
	OldActionsWindow = GemRB.GetVar ("ActionsWindow")
	GemRB.SetVar ("ActionsWindow", Window)

	#end dialog
	Button = GemRB.GetControl (Window, 0)
	GemRB.SetText (Window, Button, 9371)
	GemRB.SetVarAssoc (Window, Button, "DialogChoose", DIALOGCHOOSE_END)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "CloseContinueWindow")
	
	GemRB.UnhideGUI ()

def OpenContinueMessageWindow ():
	global ContinueWindow, OldActionsWindow

	GemRB.HideGUI ()

	if ContinueWindow:
		return

	GemRB.LoadWindowPack (GetWindowPack())
	ContinueWindow = Window = GemRB.LoadWindow (9)
	OldActionsWindow = GemRB.GetVar ("ActionsWindow")
	GemRB.SetVar ("ActionsWindow", Window)

	#continue
	Button = GemRB.GetControl (Window, 0)
	GemRB.SetText (Window, Button, 9372)
	GemRB.SetVarAssoc (Window, Button, "DialogChoose", DIALOGCHOOSE_CONTINUE)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "CloseContinueWindow")
	
	GemRB.UnhideGUI ()


def CloseContainerWindow ():
	global OldActionsWindow, OldMessageWindow, ContainerWindow

	if ContainerWindow == None:
		return

	hideflag = GemRB.HideGUI ()

	GemRB.UnloadWindow (ContainerWindow)
	ContainerWindow = None
	GemRB.SetVar ("ActionsWindow", OldActionsWindow)
	GemRB.SetVar ("MessageWindow", OldMessageWindow)
	Table = GemRB.LoadTable ("containr")
	row = Container['Type']
	tmp = GemRB.GetTableValue (Table, row, 2)
	#play closing sound if applicable
	if tmp!='*':
		GemRB.PlaySound (tmp)

	#it is enough to close here
	GemRB.UnloadTable (Table)

	if hideflag:
		GemRB.UnhideGUI ()


def UpdateContainerWindow ():
	global Container

	Window = ContainerWindow

	pc = GemRB.GameGetFirstSelectedPC ()
	SetEncumbranceLabels( Window, 0x10000043, 0x10000044, pc)

	party_gold = GemRB.GameGetPartyGold ()
	Text = GemRB.GetControl (Window, 0x10000036)
	GemRB.SetText (Window, Text, str (party_gold))

	Container = GemRB.GetContainer(0) #will use first selected pc anyway
	LeftCount = Container['ItemCount']
	ScrollBar = GemRB.GetControl (Window, 52)
	Count = LeftCount/3
	if Count<1:
		Count=1
	GemRB.SetVarAssoc (Window, ScrollBar, "LeftTopIndex", Count)
	
	inventory_slots = GemRB.GetSlots (pc, 0x8000)
	RightCount = len(inventory_slots)
	ScrollBar = GemRB.GetControl (Window, 53)
	Count = RightCount/2
	if Count<1:
		Count=1
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
	RightCount = len(inventory_slots)

	for i in range(6):
		#this is an autoselected container, but we could use PC too
		Slot = GemRB.GetContainerItem (0, i+LeftTopIndex)
		Button = GemRB.GetControl (Window, i)
		if Slot != None:
			Item = GemRB.GetItem (Slot['ItemResRef'])
			GemRB.SetVarAssoc (Window, Button, "LeftIndex", LeftTopIndex+i)
			GemRB.SetItemIcon (Window, Button, Slot['ItemResRef'],0)
			GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_PICTURE, OP_OR)
			GemRB.SetTooltip (Window, Button, Slot['ItemName'])
		else:
			GemRB.SetVarAssoc (Window, Button, "LeftIndex", -1)
			GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_PICTURE, OP_NAND)
			GemRB.SetTooltip (Window, Button, "")


	for i in range(4):
		if i+RightTopIndex<RightCount:
			Slot = GemRB.GetSlotItem (pc, inventory_slots[i+RightTopIndex])
		else:
			Slot = None
		Button = GemRB.GetControl (Window, i+10)
		if Slot!=None:
			Item = GemRB.GetItem (Slot['ItemResRef'])
			GemRB.SetVarAssoc (Window, Button, "RightIndex", RightTopIndex+i)
			GemRB.SetItemIcon (Window,Button, Slot['ItemResRef'],0)
			GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_PICTURE, OP_OR)
			#is this needed?
			#Slot = GemRB.GetItem(Slot['ItemResRef'])
			#GemRB.SetTooltip (Window, Button, Slot['ItemName'])
		else:
			GemRB.SetVarAssoc (Window, Button, "RightIndex", -1)
			GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_PICTURE, OP_NAND)
			GemRB.SetTooltip (Window, Button, "")


def OpenContainerWindow ():
	global OldActionsWindow, OldMessageWindow
	global ContainerWindow, Container

	if ContainerWindow:
		return

	GemRB.HideGUI ()

	GemRB.LoadWindowPack (GetWindowPack())
	ContainerWindow = Window = GemRB.LoadWindow (8)
	OldActionsWindow = GemRB.GetVar ("ActionsWindow")
	OldMessageWindow = GemRB.GetVar ("MessageWindow")
	GemRB.SetVar ("ActionsWindow", Window)
	GemRB.SetVar ("MessageWindow", -1)

	pc = GemRB.GameGetFirstSelectedPC()
	Container = GemRB.GetContainer(0)

	# Gears (time) when options pane is down
	Button = GemRB.GetControl (Window, 62)
	GemRB.SetAnimation (Window, Button, "CGEAR")
	GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_PICTURE | IE_GUI_BUTTON_ANIMATED, OP_SET)
	GemRB.SetButtonState(Window, Button, IE_GUI_BUTTON_LOCKED)

	# 0 - 5 - Ground Item
	# 10 - 13 - Personal Item
	# 50 hand
	# 52, 53 scroller ground, scroller personal
	# 54 - encumbrance

	for i in range(6):
		Button = GemRB.GetControl (Window, i)
		GemRB.SetVarAssoc (Window, Button, "LeftIndex", i)
		#GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_CHECKBOX, OP_OR)
		GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "TakeItemContainer")

	for i in range(4):
		Button = GemRB.GetControl (Window, i+10)
		GemRB.SetVarAssoc (Window, Button, "RightIndex", i)
		#GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_CHECKBOX, OP_OR)
		GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "DropItemContainer")

	# left scrollbar
	ScrollBar = GemRB.GetControl (Window, 52)
	GemRB.SetEvent (Window, ScrollBar, IE_GUI_SCROLLBAR_ON_CHANGE, "RedrawContainerWindow")

	# right scrollbar
	ScrollBar = GemRB.GetControl (Window, 53)
	GemRB.SetEvent (Window, ScrollBar, IE_GUI_SCROLLBAR_ON_CHANGE, "RedrawContainerWindow")

	Label = GemRB.CreateLabel (Window, 0x10000043, 323,14,60,15,"NUMBER","0:",IE_FONT_ALIGN_LEFT|IE_FONT_ALIGN_TOP)
	Label = GemRB.CreateLabel (Window, 0x10000044, 323,20,80,15,"NUMBER","0:",IE_FONT_ALIGN_RIGHT|IE_FONT_ALIGN_TOP)

	Button = GemRB.GetControl (Window, 50)
	GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_LOCKED)
	Table = GemRB.LoadTable ("containr")
	row = Container['Type']
	tmp = GemRB.GetTableValue (Table, row, 0)
	if tmp!='*':
		GemRB.PlaySound (tmp)
	tmp = GemRB.GetTableValue (Table, row, 1)
	if tmp!='*':
		GemRB.SetButtonSprites (Window, Button, tmp, 0, 0, 0, 0, 0 )

	# Done
	Button = GemRB.GetControl (Window, 51)
	GemRB.SetText (Window, Button, 1403)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "LeaveContainer")

	GemRB.SetVar ("LeftTopIndex", 0)
	GemRB.SetVar ("RightTopIndex", 0)
	UpdateContainerWindow ()
	GemRB.UnhideGUI ()


#doing this way it will inform the core system too, which in turn will call
#CloseContainerWindow ()
def LeaveContainer ():
	GemRB.LeaveContainer()

def DropItemContainer ():
	RightIndex = GemRB.GetVar ("RightIndex")
	if RightIndex<0:
		return
	
	#we need to get the right slot number
	pc = GemRB.GameGetFirstSelectedPC ()
	inventory_slots = GemRB.GetSlots (pc, 0x8000)
	if RightIndex >= len(inventory_slots):
		return
	GemRB.ChangeContainerItem (0, inventory_slots[RightIndex], 0)
	UpdateContainerWindow ()


def TakeItemContainer ():
	LeftIndex = GemRB.GetVar ("LeftIndex")
	if LeftIndex<0:
		return
	
	if LeftIndex >= Container['ItemCount']:
		return
	GemRB.ChangeContainerItem (0, LeftIndex, 1)
	UpdateContainerWindow ()


def OpenReformPartyWindow ():
	global ReformPartyWindow
	GemRB.HideGUI ()

	if ReformPartyWindow:
		GemRB.UnloadWindow (ReformPartyWindow)
		ReformPartyWindow = None

		GemRB.SetVar ("OtherWindow", -1)
		GemRB.LoadWindowPack ("GUIREC")
		GemRB.UnhideGUI ()
		return

	GemRB.LoadWindowPack (GetWindowPack())
	ReformPartyWindow = Window = GemRB.LoadWindow (24)
	GemRB.SetVar ("OtherWindow", Window)

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

	if CloseOtherWindow (OpenFormationWindow):
		GemRB.HideGUI ()
		GemRB.UnloadWindow (FormationWindow)
		FormationWindow = None

		GemRB.GameSetFormation (last_formation)
		GemRB.SetVar ("OtherWindow", -1)
		GemRB.UnhideGUI ()
		return

	GemRB.HideGUI ()
	GemRB.LoadWindowPack (GetWindowPack())
	FormationWindow = Window = GemRB.LoadWindow (27)
	GemRB.SetVar ("OtherWindow", Window)

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

	GemRB.SetVar ("SelectedFormation", GemRB.GameGetFormation ())
	SelectFormation ()

	GemRB.UnhideGUI ()

def SelectFormation ():
	global last_formation
	Window = FormationWindow
	
	formation = GemRB.GetVar ("SelectedFormation")
	if last_formation != None and last_formation != formation:
		Button = GemRB.GetControl (Window, last_formation)
		GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_UNPRESSED)

	Button = GemRB.GetControl (Window, formation)
	GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_SELECTED)

	last_formation = formation
	return

def GetWindowPack():
	width = GemRB.GetSystemVariable (SV_WIDTH)
	if width == 800:
		return "GUIW08"
	if width == 1024:
		return "GUIW10"
	if width == 1280:
		return "GUIW12"
	#default
	return "GUIW"

