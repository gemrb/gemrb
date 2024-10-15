# GemRB - Infinity Engine Emulator
# Copyright (C) 2021 The GemRB Project
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
import GUIClasses
import GUICommon
import InventoryCommon
from CommonWindow import SetGameGUIHidden, IsGameGUIHidden
from ie_stats import *
from GUIDefines import *

HideOnClose = False
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
		GUICommon.SetEncumbranceLabels (Window, 54, None, pc)
	else:
		GUICommon.SetEncumbranceLabels (Window, 0x10000045, 0x10000046, pc)

	party_gold = GemRB.GameGetPartyGold ()
	Text = Window.GetControl (0x10000036)
	Text.SetText (str (party_gold))

	Container = GemRB.GetContainer (0) #will use first selected pc anyway
	LeftCount = Container['ItemCount']
	ScrollBar = Window.GetControl (52)
	Count = max (0, (LeftCount - ground_size + leftdiv - 1) // leftdiv)
	ScrollBar.SetVarAssoc ("LeftTopIndex", GemRB.GetVar ("LeftTopIndex"), 0, Count)

	inventory_slots = GemRB.GetSlots (pc, 0x8000)
	RightCount = len(inventory_slots)
	ScrollBar = Window.GetControl (53)
	Count = max (0, (RightCount - 4 + 1) // 2)
	ScrollBar.SetVarAssoc ("RightTopIndex", GemRB.GetVar ("RightTopIndex"), 0, Count)

	RedrawContainerWindow ()

def RedrawContainerWindow ():
	Window = ContainerWindow

	# scroll in multiples of the number of columns
	LeftTopIndex = GemRB.GetVar ("LeftTopIndex") * leftdiv
	RightTopIndex = GemRB.GetVar ("RightTopIndex") * 2
	pc = GemRB.GameGetFirstSelectedPC ()
	inventory_slots = GemRB.GetSlots (pc, 0x8000)
	RightCount = len(inventory_slots)

	for i in range (ground_size):
		#this is an autoselected container, but we could use PC too
		Slot = GemRB.GetContainerItem (0, i+LeftTopIndex)
		Button = Window.GetControl (i)
		if Slot:
			Button.SetVarAssoc ("LeftIndex", LeftTopIndex+i)
			callback = TakeItemContainer
		else:
			Button.SetVarAssoc ("LeftIndex", None)
			callback = None
		if GameCheck.IsPST():
			GUICommonWindows.SetItemButton (Window, Button, Slot, callback, None)
		else:
			InventoryCommon.UpdateInventorySlot (pc, Button, Slot, "container")

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
			callback = DropItemContainer
		else:
			Button.SetVarAssoc ("RightIndex", None)
			callback = None
		if GameCheck.IsPST():
			GUICommonWindows.SetItemButton (Window, Button, Slot, callback, None)
		else:
			InventoryCommon.UpdateInventorySlot (pc, Button, Slot, "inventory")

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

	global HideOnClose
	HideOnClose = IsGameGUIHidden()

	if HideOnClose:
		SetGameGUIHidden(False)
		# must use a timed event because SetGameGUIHidden(False) sets a flag to unhide the gui next frame
		# AFIK not needed, commented out in case it turns out there are conditions we will need it
		# GemRB.SetTimedEvent (lambda: GemRB.GetView ("MSGWIN").SetVisible(False), 1)
	else:
		GemRB.GetView ("MSGWIN").SetVisible (False)
		ActWin = GemRB.GetView ("ACTWIN")
		if ActWin:
			ActWin.SetVisible (False)

	ContainerWindow = Window = GemRB.LoadWindow (8, GUICommon.GetWindowPack(), WINDOW_BOTTOM|WINDOW_HCENTER)
	ContainerWindow.SetEventProxy(GUICommon.GameControl)

	# fix wrong height in the guiw10.chu and reposition
	# that chu is also used as a base for some arbitrary resolutions
	if GameCheck.IsBG2 () and GemRB.GetSystemVariable (SV_HEIGHT) >= 768:
		Size = Window.GetSize ()
		Pos = Window.GetPos ()
		Window.SetSize (Size[0], 90)
		Window.SetPos (Pos[0], GemRB.GetSystemVariable (SV_HEIGHT) - 90)

	if not GameCheck.IsIWD2 () and not GameCheck.IsGemRBDemo () and not GameCheck.IsPST ():
		# container window shouldnt be in front
		GemRB.GetView("OPTWIN").Focus()
		GemRB.GetView("PORTWIN").Focus()

	Container = GemRB.GetContainer(0)

	# Gears (time) when options pane is down
	if GameCheck.IsBG2():
		import Clock
		Clock.CreateClockButton(Window.GetControl (62))

	# 0-5 - Ground Item
	for i in range (ground_size):
		Button = Window.GetControl (i)
		Button.SetVarAssoc ("LeftIndex", i)
		Button.OnPress (TakeItemContainer)
		if GameCheck.IsPST():
			Button.SetFont ("NUMBER")
			Button.SetFlags (IE_GUI_BUTTON_ALIGN_RIGHT | IE_GUI_BUTTON_ALIGN_BOTTOM, OP_OR)

	# 10-13 - Personal Item
	for i in range (4):
		Button = Window.GetControl (i+10)
		Button.SetVarAssoc ("RightIndex", i)
		Button.OnPress (DropItemContainer)
		if GameCheck.IsPST():
			Button.SetFont ("NUMBER")
			Button.SetFlags (IE_GUI_BUTTON_ALIGN_RIGHT | IE_GUI_BUTTON_ALIGN_BOTTOM, OP_OR)

	# left scrollbar (container)
	ScrollBar = Window.GetControl (52)
	ScrollBar.SetVisible(True) # unhide because in PST it is linked to a TextArea
	ScrollBar.OnChange (RedrawContainerWindow)

	# right scrollbar (inventory)
	ScrollBar = Window.GetControl (53)
	ScrollBar.SetVisible(True) # unhide because in PST it is linked to a TextArea
	ScrollBar.OnChange (RedrawContainerWindow)

	# encumbrance and inventory icon
	# iwd has a handy button
	Button = Window.GetControl (54)
	if Button:
		if GameCheck.IsPST():
			Button.SetFont ("NUMBER")
			Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_SET)
		Button.CreateLabel (0x10000045, "NUMBER", "", IE_FONT_ALIGN_LEFT|IE_FONT_ALIGN_TOP|IE_FONT_SINGLE_LINE)
		Button.CreateLabel (0x10000046, "NUMBER", "", IE_FONT_ALIGN_RIGHT|IE_FONT_ALIGN_BOTTOM|IE_FONT_SINGLE_LINE)
	else:
		Window.CreateLabel (0x10000045, 323, 14, 60, 15, "NUMBER", "0:", IE_FONT_ALIGN_LEFT|IE_FONT_ALIGN_TOP|IE_FONT_SINGLE_LINE)
		Window.CreateLabel (0x10000046, 323, 20, 80, 15, "NUMBER", "0:", IE_FONT_ALIGN_RIGHT|IE_FONT_ALIGN_TOP|IE_FONT_SINGLE_LINE)

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
	else:
		Button.SetText ("")

	Button.MakeEscape()
	Button.OnPress (LeaveContainer)

	GemRB.SetVar ("LeftTopIndex", 0)
	GemRB.SetVar ("RightTopIndex", 0)
	UpdateContainerWindow ()

def CloseContainerWindow ():
	global ContainerWindow

	if not ContainerWindow:
		return

	ContainerWindow.Close ()
	ContainerWindow = None
	GemRB.GetView ("MSGWIN").SetVisible (True)
	if GemRB.GetView ("ACTWIN"):
		GemRB.GetView ("ACTWIN").SetVisible (True)
	SetGameGUIHidden(HideOnClose)

	Table = GemRB.LoadTable ("containr")
	row = Container['Type']
	tmp = Table.GetValue (row, 2)
	#play closing sound if applicable
	if tmp!='*':
		GemRB.PlaySound (tmp)

#doing this way it will inform the core system too, which in turn will call
#CloseContainerWindow ()
def LeaveContainer ():
	GemRB.LeaveContainer()

def DropItemContainer ():
	RightIndex = GemRB.GetVar ("RightIndex")
	if RightIndex is None:
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
	if LeftIndex is None:
		return

	if LeftIndex >= Container['ItemCount']:
		return

	GemRB.ChangeContainerItem (0, LeftIndex, 1)
	UpdateContainerWindow ()
