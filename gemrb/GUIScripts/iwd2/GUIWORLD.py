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
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#


# GUIW.py - scripts to control some windows from GUIWORLD winpack
# except of Portrait, Options and Dialog windows

###################################################

import GemRB
from GUIDefines import *
from GUICommon import CloseOtherWindow, SetEncumbranceLabels
from GUICommonWindows import *
from GUIClasses import GWindow

FRAME_PC_SELECTED = 0
FRAME_PC_TARGET   = 1

ContainerWindow = None
ContinueWindow = None
ReformPartyWindow = None
OldActionsWindow = None
OldMessageWindow = None
Container = None

def CloseContinueWindow ():
	if ContinueWindow:
		# don't close the actual window now to avoid flickering: we might still want it open
		GemRB.SetVar ("DialogChoose", GemRB.GetVar ("DialogOption"))

def NextDialogState ():
	global ContinueWindow, OldActionsWindow

	if ContinueWindow == None:
		return

	hideflag = GemRB.HideGUI ()

	if ContinueWindow:
		ContinueWindow.Unload ()
	GemRB.SetVar ("PortraitWindow", OldActionsWindow.ID)
	ContinueWindow = None
	OldActionsWindow = None
	if hideflag:
		GemRB.UnhideGUI ()


def OpenEndMessageWindow ():
	global ContinueWindow, OldActionsWindow

	hideflag = GemRB.HideGUI ()

	if not ContinueWindow:
		GemRB.LoadWindowPack (GetWindowPack())
		ContinueWindow = Window = GemRB.LoadWindow (9)
		OldActionsWindow = GWindow( GemRB.GetVar ("PortraitWindow") )
		GemRB.SetVar ("PortraitWindow", Window.ID)

	#end dialog
	Button = ContinueWindow.GetControl (0)
	Button.SetText (9371)
	Button.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "CloseContinueWindow")

	if hideflag:
		GemRB.UnhideGUI ()

def OpenContinueMessageWindow ():
	global ContinueWindow, OldActionsWindow

	hideflag = GemRB.HideGUI ()

	if not ContinueWindow:
		GemRB.LoadWindowPack (GetWindowPack())
		ContinueWindow = Window = GemRB.LoadWindow (9)
		OldActionsWindow = GWindow( GemRB.GetVar ("PortraitWindow") )
		GemRB.SetVar ("PortraitWindow", Window.ID)

	#continue
	Button = ContinueWindow.GetControl (0)
	Button.SetText (9372)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "CloseContinueWindow")
	
	if hideflag:
		GemRB.UnhideGUI ()


def CloseContainerWindow ():
	global OldActionsWindow, OldMessageWindow, ContainerWindow

	if ContainerWindow == None:
		return

	hideflag = GemRB.HideGUI ()

	if ContainerWindow:
		ContainerWindow.Unload ()
	ContainerWindow = None
	GemRB.SetVar ("PortraitWindow", OldActionsWindow.ID)
	GemRB.SetVar ("MessageWindow", OldMessageWindow.ID)
	Table = GemRB.LoadTable ("containr")
	row = Container['Type']
	tmp = Table.GetValue (row, 2)
	#play closing sound if applicable
	if tmp!='*':
		GemRB.PlaySound (tmp)

	#it is enough to close here

	if hideflag:
		GemRB.UnhideGUI ()


def UpdateContainerWindow ():
	global Container

	Window = ContainerWindow

	pc = GemRB.GameGetFirstSelectedPC ()
	SetEncumbranceLabels( Window, 0x10000043, 0x10000044, pc)

	party_gold = GemRB.GameGetPartyGold ()
	Text = Window.GetControl (0x10000036)
	Text.SetText (str (party_gold))

	Container = GemRB.GetContainer(0) #will use first selected pc anyway
	LeftCount = Container['ItemCount']
	ScrollBar = Window.GetControl (52)
	Count = LeftCount / 5
	if Count<1:
		Count=1
	ScrollBar.SetVarAssoc ("LeftTopIndex", Count)
	
	inventory_slots = GemRB.GetSlots (pc, 0x8000)
	RightCount = len(inventory_slots)
	ScrollBar = Window.GetControl (53)
	Count = RightCount / 2
	if Count<1:
		Count=1
	ScrollBar.SetVarAssoc ("RightTopIndex", Count)
	RedrawContainerWindow ()


def RedrawContainerWindow ():
	Window = ContainerWindow

	LeftTopIndex = GemRB.GetVar ("LeftTopIndex") * 5
	LeftIndex = GemRB.GetVar ("LeftIndex")
	RightTopIndex = GemRB.GetVar ("RightTopIndex") * 2
	RightIndex = GemRB.GetVar ("RightIndex")
	LeftCount = Container['ItemCount']
	pc = GemRB.GameGetFirstSelectedPC ()
	inventory_slots = GemRB.GetSlots (pc, 0x8000)
	RightCount = len(inventory_slots)

	for i in range (6):
		#this is an autoselected container, but we could use PC too
		Slot = GemRB.GetContainerItem (0, i+LeftTopIndex)
		Button = Window.GetControl (i)
		if Slot != None:
			Item = GemRB.GetItem (Slot['ItemResRef'])
			Button.SetVarAssoc ("LeftIndex", LeftTopIndex+i)
			Button.SetItemIcon (Slot['ItemResRef'],0)
			Button.SetFlags (IE_GUI_BUTTON_PICTURE, OP_OR)
			Button.SetTooltip (Slot['ItemName'])
		else:
			Button.SetVarAssoc ("LeftIndex", -1)
			Button.SetFlags (IE_GUI_BUTTON_PICTURE, OP_NAND)
			Button.SetTooltip ("")


	for i in range (4):
		if i+RightTopIndex<RightCount:
			Slot = GemRB.GetSlotItem (pc, inventory_slots[i+RightTopIndex])
		else:
			Slot = None
		Button = Window.GetControl (i+10)
		if Slot!=None:
			Item = GemRB.GetItem (Slot['ItemResRef'])
			Button.SetVarAssoc ("RightIndex", RightTopIndex+i)
			Button.SetItemIcon (Slot['ItemResRef'],0)
			Button.SetFlags (IE_GUI_BUTTON_PICTURE, OP_OR)
			#is this needed?
			#Slot = GemRB.GetItem(Slot['ItemResRef'])
			#Button.SetTooltip (Slot['ItemName'])
		else:
			Button.SetVarAssoc ("RightIndex", -1)
			Button.SetFlags (IE_GUI_BUTTON_PICTURE, OP_NAND)
			Button.SetTooltip ("")


def OpenContainerWindow ():
	global OldActionsWindow, OldMessageWindow
	global ContainerWindow, Container

	if ContainerWindow:
		return

	hideflag = GemRB.HideGUI ()

	GemRB.LoadWindowPack (GetWindowPack())
	ContainerWindow = Window = GemRB.LoadWindow (8)
	OldActionsWindow = GWindow( GemRB.GetVar ("PortraitWindow") )
	OldMessageWindow = GWindow( GemRB.GetVar ("MessageWindow") )
	GemRB.SetVar ("PortraitWindow", Window.ID)
	GemRB.SetVar ("MessageWindow", -1)

	pc = GemRB.GameGetFirstSelectedPC()
	Container = GemRB.GetContainer(0)

	# 0 - 9 - Ground Item
	# 10 - 13 - Personal Item
	# 50 container icon
	# 52, 53 scroller ground, scroller personal
	# 54 - encumbrance

	for i in range (10):
		Button = Window.GetControl (i)
		Button.SetVarAssoc ("LeftIndex", i)
		#Button.SetFlags (IE_GUI_BUTTON_CHECKBOX, OP_OR)
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "TakeItemContainer")

	for i in range (4):
		Button = Window.GetControl (i+10)
		Button.SetVarAssoc ("RightIndex", i)
		#Button.SetFlags (IE_GUI_BUTTON_CHECKBOX, OP_OR)
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "DropItemContainer")

	# left scrollbar
	ScrollBar = Window.GetControl (52)
	ScrollBar.SetEvent (IE_GUI_SCROLLBAR_ON_CHANGE, "RedrawContainerWindow")

	# right scrollbar
	ScrollBar = Window.GetControl (53)
	ScrollBar.SetEvent (IE_GUI_SCROLLBAR_ON_CHANGE, "RedrawContainerWindow")

	Label = Window.CreateLabel (0x10000043, 323,14,60,15,"NUMBER","0:",IE_FONT_ALIGN_LEFT|IE_FONT_ALIGN_TOP)
	Label = Window.CreateLabel (0x10000044, 323,20,80,15,"NUMBER","0:",IE_FONT_ALIGN_RIGHT|IE_FONT_ALIGN_TOP)

	Button = Window.GetControl (50)
	Button.SetState (IE_GUI_BUTTON_LOCKED)
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
	#no caption on this button
	Button.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "LeaveContainer")

	GemRB.SetVar ("LeftTopIndex", 0)
	GemRB.SetVar ("RightTopIndex", 0)
	UpdateContainerWindow ()
	if hideflag:
		GemRB.UnhideGUI ()


#doing this way it will inform the core system too, which in turn will call
#CloseContainerWindow ()
def LeaveContainer ():
	GemRB.LeaveContainer()

def DropItemContainer ():
	RightIndex = GemRB.GetVar ("RightIndex")

	print RightIndex
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
	print LeftIndex

	if LeftIndex<0:
		return
	
	if LeftIndex >= Container['ItemCount']:
		return
	GemRB.ChangeContainerItem (0, LeftIndex, 1)
	UpdateContainerWindow ()


def OpenReformPartyWindow ():
	global ReformPartyWindow

	hideflag = GemRB.HideGUI ()

	if ReformPartyWindow:
		if ReformPartyWindow:
			ReformPartyWindow.Unload ()
		ReformPartyWindow = None

		GemRB.SetVar ("OtherWindow", -1)
		GemRB.LoadWindowPack ("GUIREC", 800, 600)
		if hideflag:
			GemRB.UnhideGUI ()
		return

	GemRB.LoadWindowPack (GetWindowPack())
	ReformPartyWindow = Window = GemRB.LoadWindow (24)
	GemRB.SetVar ("OtherWindow", Window.ID)

	# Remove
	Button = Window.GetControl (15)
	Button.SetText (42514)
	Button.SetState (IE_GUI_BUTTON_DISABLED)

	# Done
	Button = Window.GetControl (8)
	Button.SetText (1403)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "OpenReformPartyWindow")
	if hideflag:
		GemRB.UnhideGUI ()

def DeathWindowPlot():
	#no death movie, but music is changed
	GemRB.LoadMusicPL ("Theme.mus",1)
	GemRB.HideGUI ()
	GemRB.SetVar("QuitGame1", 32848)
	GemRB.SetTimedEvent ("DeathWindowEnd", 10)
	return

def DeathWindow():
	#no death movie, but music is changed
	GemRB.LoadMusicPL ("Theme.mus",1)
	GemRB.HideGUI ()
	GemRB.SetVar("QuitGame1", 16498)
	GemRB.SetTimedEvent ("DeathWindowEnd", 10)
	return

def DeathWindowEnd ():
	GemRB.GamePause (1,1)

	GemRB.LoadWindowPack (GetWindowPack())
	Window = GemRB.LoadWindow (17)

	#reason for death
	Label = Window.GetControl (0x0fffffff)
	strref = GemRB.GetVar ("QuitGame1")
	Label.SetText (strref)

	#load
	Button = Window.GetControl (1)
	Button.SetText (15590)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "LoadPress")

	#quit
	Button = Window.GetControl (2)
	Button.SetText (15417)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "QuitPress")

	GemRB.HideGUI ()
	GemRB.SetVar ("MessageWindow", -1)
	GemRB.UnhideGUI ()
	Window.ShowModal (MODAL_SHADOW_GRAY)
	return

def QuitPress():
	GemRB.QuitGame()
	GemRB.SetNextScript("Start")
	return

def LoadPress():
	GemRB.QuitGame()
	GemRB.SetNextScript("GUILOAD")
	return
