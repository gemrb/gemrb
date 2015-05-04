# -*-python-*-
# GemRB - Infinity Engine Emulator
# Copyright (C) 2010 The GemRB Project
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
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#
import GemRB
import GameCheck
from GUIDefines import GS_DIALOGMASK, OP_SET

# message window expansion
def OnIncreaseSize():
	GSFlags = GemRB.GetMessageWindowSize()
	Expand = GSFlags&GS_DIALOGMASK
	GSFlags = GSFlags-Expand
	if Expand>2:
		return
	Expand = (Expand + 1)*2
	GemRB.GameSetScreenFlags(Expand + GSFlags, OP_SET)

# message window contraction
def OnDecreaseSize():
	GSFlags = GemRB.GetMessageWindowSize()
	Expand = GSFlags&GS_DIALOGMASK
	GSFlags = GSFlags-Expand
	if Expand<2:
		return
	Expand = Expand/2 - 1 # 6->2, 2->0
	GemRB.GameSetScreenFlags(Expand + GSFlags, OP_SET)


##################################################################
# functions dealing with containers
##################################################################

import GUICommon
import GUIClasses
import GUIWORLD
from ie_stats import *
from GUIDefines import *

ContainerWindow = None
Container = None
if GameCheck.IsIWD2():
	leftdiv = 5
	ground_size = 10
else:
	leftdiv = 3
	ground_size = 6

if GameCheck.IsPST():
	import GUICommonWindows

def UpdateContainerWindow ():
	global Container

	Window = ContainerWindow

	pc = GemRB.GameGetFirstSelectedPC ()
	if GameCheck.IsPST():
		GUICommon.SetEncumbranceLabels (Window, 54, None, pc, True)
	else:
		GUICommon.SetEncumbranceLabels (Window, 0x10000043, 0x10000044, pc)

	party_gold = GemRB.GameGetPartyGold ()
	Text = Window.GetControl (0x10000036)
	Text.SetText (str (party_gold))

	Container = GemRB.GetContainer (0) #will use first selected pc anyway
	LeftCount = Container['ItemCount']
	ScrollBar = Window.GetControl (52)
	Count = max (0, (LeftCount - ground_size + leftdiv - 1) / leftdiv)
	ScrollBar.SetVarAssoc ("LeftTopIndex", Count)

	inventory_slots = GemRB.GetSlots (pc, 0x8000)
	RightCount = len(inventory_slots)
	ScrollBar = Window.GetControl (53)
	Count = max (0, (RightCount - 4 + 1) / 2)
	ScrollBar.SetVarAssoc ("RightTopIndex", Count)

	RedrawContainerWindow ()

def RedrawContainerWindow ():
	Window = ContainerWindow

	# scroll in multiples of the number of columns
	LeftTopIndex = GemRB.GetVar ("LeftTopIndex") * leftdiv
	LeftIndex = GemRB.GetVar ("LeftIndex")
	RightTopIndex = GemRB.GetVar ("RightTopIndex") * 2
	RightIndex = GemRB.GetVar ("RightIndex")
	LeftCount = Container['ItemCount']
	pc = GemRB.GameGetFirstSelectedPC ()
	inventory_slots = GemRB.GetSlots (pc, 0x8000)
	RightCount = len(inventory_slots)

	for i in range (ground_size):
		#this is an autoselected container, but we could use PC too
		Slot = GemRB.GetContainerItem (0, i+LeftTopIndex)
		Button = Window.GetControl (i)
		if Slot:
			Button.SetVarAssoc ("LeftIndex", LeftTopIndex+i)
			function = TakeItemContainer
		else:
			Button.SetVarAssoc ("LeftIndex", -1)
			function = None
		if GameCheck.IsPST():
			GUICommonWindows.SetItemButton (Window, Button, Slot, function, None)
		else:
			GUICommon.UpdateInventorySlot (pc, Button, Slot, "container")

	for i in range (4):
		if i+RightTopIndex < RightCount:
			Slot = GemRB.GetSlotItem (pc, inventory_slots[i+RightTopIndex])
		else:
			Slot = None
		Button = Window.GetControl (i+10)

		#pst had a redundant call here, reenable if it turns out it isn't redundant:
		#GUICommonWindows.SetItemButton (Window, Button, Slot, None, None)

		if Slot:
			Button.SetVarAssoc ("RightIndex", RightTopIndex+i)
			function = DropItemContainer
		else:
			Button.SetVarAssoc ("RightIndex", -1)
			function = None
		if GameCheck.IsPST():
			GUICommonWindows.SetItemButton (Window, Button, Slot, function, None)
		else:
			GUICommon.UpdateInventorySlot (pc, Button, Slot, "inventory")

	# shade the inventory icon if it is full
	Button = Window.GetControl (54)
	if Button:
		free_slots = GemRB.GetSlots (pc, 0x8000, -1)
		if free_slots == ():
			Button.SetState (IE_GUI_BUTTON_PRESSED)
		else:
			Button.SetState (IE_GUI_BUTTON_LOCKED)

def OpenContainerWindow ():
	global ContainerWindow, Container

	if ContainerWindow:
		return

	hideflag = GemRB.HideGUI ()

	GemRB.LoadWindowPack (GUICommon.GetWindowPack())
	ContainerWindow = Window = GemRB.LoadWindow (8)


	#stop gears from interfering
	if GameCheck.IsPST():
		GUIWORLD.OldPortraitWindow = GUIClasses.GWindow( GemRB.GetVar ("PortraitWindow") )
		GUICommonWindows.DisableAnimatedWindows ()

	if GameCheck.IsIWD2():
		GUIWORLD.OldMessageWindow = GUIClasses.GWindow( GemRB.GetVar ("MessageWindow") )
		GemRB.SetVar ("MessageWindow", Window.ID)
	else:
		GUIWORLD.OldActionsWindow = GUIClasses.GWindow( GemRB.GetVar ("ActionsWindow") )
		GUIWORLD.OldMessageWindow = GUIClasses.GWindow( GemRB.GetVar ("MessageWindow") )
		GemRB.SetVar ("MessageWindow", -1)
		GemRB.SetVar ("ActionsWindow", Window.ID)

	Container = GemRB.GetContainer(0)

	# Gears (time) when options pane is down
	if GameCheck.IsBG2():
		Button = Window.GetControl (62)
		Label = Button.CreateLabelOnButton (0x1000003e, "NORMAL", IE_FONT_SINGLE_LINE)

		Label.SetAnimation ("CPEN")
		Button.SetAnimation ("CGEAR")
		Button.SetBAM ("CDIAL", 0, 0)
		Button.SetState (IE_GUI_BUTTON_ENABLED)
		Button.SetFlags (IE_GUI_BUTTON_PICTURE|IE_GUI_BUTTON_ANIMATED|IE_GUI_BUTTON_NORMAL, OP_SET)
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, GUICommon.GearsClicked)
		GUICommon.SetGamedaysAndHourToken()
		Button.SetTooltip(16041)

	# 0-5 - Ground Item
	for i in range (ground_size):
		Button = Window.GetControl (i)
		Button.SetVarAssoc ("LeftIndex", i)
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, TakeItemContainer)
		if GameCheck.IsPST():
			Button.SetFont ("NUMBER")
			Button.SetFlags (IE_GUI_BUTTON_ALIGN_RIGHT | IE_GUI_BUTTON_ALIGN_BOTTOM, OP_OR)

	# 10-13 - Personal Item
	for i in range (4):
		Button = Window.GetControl (i+10)
		Button.SetVarAssoc ("RightIndex", i)
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, DropItemContainer)
		if GameCheck.IsPST():
			Button.SetFont ("NUMBER")
			Button.SetFlags (IE_GUI_BUTTON_ALIGN_RIGHT | IE_GUI_BUTTON_ALIGN_BOTTOM, OP_OR)

	# left scrollbar (container)
	ScrollBar = Window.GetControl (52)
	ScrollBar.SetEvent (IE_GUI_SCROLLBAR_ON_CHANGE, RedrawContainerWindow)

	# right scrollbar (inventory)
	ScrollBar = Window.GetControl (53)
	ScrollBar.SetEvent (IE_GUI_SCROLLBAR_ON_CHANGE, RedrawContainerWindow)

	# encumbrance and inventory icon
	# iwd has a handy button
	Button = Window.GetControl (54)
	if Button:
		if GameCheck.IsPST():
			Button.SetFont ("NUMBER")
			Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_SET)
		Button.CreateLabelOnButton (0x10000043, "NUMBER", IE_FONT_ALIGN_LEFT|IE_FONT_ALIGN_TOP|IE_FONT_SINGLE_LINE)
		Button.CreateLabelOnButton (0x10000044, "NUMBER", IE_FONT_ALIGN_RIGHT|IE_FONT_ALIGN_BOTTOM|IE_FONT_SINGLE_LINE)
	else:
		Label = Window.CreateLabel (0x10000043, 323,14,60,15,"NUMBER","0:",IE_FONT_ALIGN_LEFT|IE_FONT_ALIGN_TOP|IE_FONT_SINGLE_LINE)
		Label = Window.CreateLabel (0x10000044, 323,20,80,15,"NUMBER","0:",IE_FONT_ALIGN_RIGHT|IE_FONT_ALIGN_TOP|IE_FONT_SINGLE_LINE)

	# container icon
	Button = Window.GetControl (50)
	if GameCheck.IsPST():
		Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_SET)
	Button.SetState (IE_GUI_BUTTON_LOCKED)

	if not GameCheck.IsPST():
		Table = GemRB.LoadTable ("containr")
		row = Container['Type']
		tmp = Table.GetValue (row, 0)
		if tmp!='*':
			GemRB.PlaySound (tmp)
		tmp = Table.GetValue (row, 1)
		if tmp!='*':
			Button.SetSprites (tmp, 0, 0, 0, 0, 0 )

	# Done
	Button = Window.GetControl (51)
	if GameCheck.IsPST():
		Button.SetText (1403)
	elif GameCheck.IsIWD2():
		Button.SetText ("")
	else:
		Button.SetText (11973)
	Button.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, LeaveContainer)

	GemRB.SetVar ("LeftTopIndex", 0)
	GemRB.SetVar ("RightTopIndex", 0)
	UpdateContainerWindow ()
	if hideflag:
		GemRB.UnhideGUI ()

def CloseContainerWindow ():
	global ContainerWindow

	if not ContainerWindow:
		return

	hideflag = GemRB.HideGUI ()

	ContainerWindow.Unload ()

	if GameCheck.IsPST():
		GemRB.SetVar ("PortraitWindow", GUIWORLD.OldPortraitWindow.ID)
		GUICommonWindows.EnableAnimatedWindows ()
		 #PST needs a reminder to redraw the  clock for some reason
		if GUICommonWindows.ActionsWindow:
			GUICommonWindows.ActionsWindow.SetVisible(WINDOW_VISIBLE)
		

	# FIXME: iwd2 bug or just bad naming?
	if GameCheck.IsIWD2():
		GemRB.SetVar ("MessageWindow", GUIWORLD.OldMessageWindow.ID)
	else:
		GemRB.SetVar ("ActionsWindow", GUIWORLD.OldActionsWindow.ID)
		GemRB.SetVar ("MessageWindow", GUIWORLD.OldMessageWindow.ID)

	Table = GemRB.LoadTable ("containr")
	row = Container['Type']
	tmp = Table.GetValue (row, 2)
	#play closing sound if applicable
	if tmp!='*':
		GemRB.PlaySound (tmp)

	#it is enough to close here

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
	if RightIndex >= len(inventory_slots):
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
