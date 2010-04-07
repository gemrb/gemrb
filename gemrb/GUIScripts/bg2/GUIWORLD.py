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


# GUIW.py - scripts to control some windows from the GUIWORLD winpack
# except of Actions, Portrait, Options and Dialog windows
#################################################################

import GemRB
from GUIDefines import *
from GUICommon import CloseOtherWindow, SetGamedaysAndHourToken, UpdateInventorySlot
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
	GemRB.SetVar ("ActionsWindow", OldActionsWindow.ID)
	ContinueWindow = None
	OldActionsWindow = None
	if hideflag:
		GemRB.UnhideGUI ()


def OpenEndMessageWindow ():
	global ContinueWindow, OldActionsWindow

	hideflag = GemRB.HideGUI ()

	if not ContinueWindow:
		GemRB.LoadWindowPack (GetWindowPack())
		ContinueWindow = Window = GemRB.LoadWindowObject (9)
		OldActionsWindow = GWindow( GemRB.GetVar ("ActionsWindow") )
		GemRB.SetVar ("ActionsWindow", Window.ID)

	#end dialog
	Button = ContinueWindow.GetControl (0)
	Button.SetText (9371)	
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "CloseContinueWindow")
	if PortraitWindow:
		UpdatePortraitWindow ()
	if hideflag:
		GemRB.UnhideGUI ()


def OpenContinueMessageWindow ():
	global ContinueWindow, OldActionsWindow

	hideflag = GemRB.HideGUI ()

	if not ContinueWindow:
		GemRB.LoadWindowPack (GetWindowPack())
		ContinueWindow = Window = GemRB.LoadWindowObject (9)
		OldActionsWindow = GWindow( GemRB.GetVar ("ActionsWindow") )
		GemRB.SetVar ("ActionsWindow", Window.ID)

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
	GemRB.SetVar ("ActionsWindow", OldActionsWindow.ID)
	GemRB.SetVar ("MessageWindow", OldMessageWindow.ID)
	Table = GemRB.LoadTableObject ("containr")
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
	Count = LeftCount/3
	if Count<1:
		Count=1
	ScrollBar.SetVarAssoc ("LeftTopIndex", Count)
	
	inventory_slots = GemRB.GetSlots (pc, 0x8000)
	RightCount = len(inventory_slots)
	ScrollBar = Window.GetControl (53)
	Count = RightCount/2
	if Count<1:
		Count=1
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
	RightCount = len(inventory_slots)

	for i in range (6):
		#this is an autoselected container, but we could use PC too
		Slot = GemRB.GetContainerItem (0, i+LeftTopIndex)
		Button = Window.GetControl (i)
		if Slot != None:
			Button.SetVarAssoc ("LeftIndex", LeftTopIndex+i)
		else:
			Button.SetVarAssoc ("LeftIndex", -1)
		UpdateInventorySlot (pc, Button, Slot, "container")

	for i in range (4):
		if i+RightTopIndex<RightCount:
			Slot = GemRB.GetSlotItem (pc, inventory_slots[i+RightTopIndex])
		else:
			Slot = None
		Button = Window.GetControl (i+10)
		if Slot!=None:
			Button.SetVarAssoc ("RightIndex", RightTopIndex+i)
		else:
			Button.SetVarAssoc ("RightIndex", -1)
		UpdateInventorySlot (pc, Button, Slot, "inventory")


def OpenContainerWindow ():
	global OldActionsWindow, OldMessageWindow
	global ContainerWindow, Container

	if ContainerWindow:
		return

	hideflag = GemRB.HideGUI ()

	GemRB.LoadWindowPack (GetWindowPack())
	ContainerWindow = Window = GemRB.LoadWindowObject (8)
	OldActionsWindow = GWindow( GemRB.GetVar ("ActionsWindow") )
	OldMessageWindow = GWindow( GemRB.GetVar ("MessageWindow") )
	GemRB.SetVar ("ActionsWindow", Window.ID)
	GemRB.SetVar ("MessageWindow", -1)

	Container = GemRB.GetContainer(0)

	# Gears (time) when options pane is down
	Button = Window.GetControl (62)
	Label = Button.CreateLabelOnButton (0x1000003e, "NORMAL", 0)

	Label.SetAnimation ("CPEN")
	Button.SetAnimation ("CGEAR")
	Button.SetBAM ("CDIAL", 0, 0)
	Button.SetState (IE_GUI_BUTTON_ENABLED)
	Button.SetFlags (IE_GUI_BUTTON_PICTURE|IE_GUI_BUTTON_ANIMATED|IE_GUI_BUTTON_NORMAL, OP_SET)
	Button.SetEvent(IE_GUI_BUTTON_ON_PRESS, "GearsClicked")
	SetGamedaysAndHourToken()
	Button.SetTooltip(16041)

	# 0 - 5 - Ground Item
	# 10 - 13 - Personal Item
	# 50 hand
	# 52, 53 scroller ground, scroller personal
	# 54 - encumbrance

	for i in range (6):
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
	Table = GemRB.LoadTableObject ("containr")
	row = Container['Type']
	tmp = Table.GetValue (row, 0)
	if tmp!='*':
		GemRB.PlaySound (tmp)
	tmp = Table.GetValue (row, 1)
	if tmp!='*':
		Button.SetSprites (tmp, 0, 0, 0, 0, 0 )

	# Done
	Button = Window.GetControl (51)
	Button.SetText (1403)
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


def UpdateReformWindow ():
	Window = ReformPartyWindow

	select = GemRB.GetVar ("Selected")

	need_to_drop = GemRB.GetPartySize ()-PARTY_SIZE
	if need_to_drop<0:
		need_to_drop = 0

	#excess player number
	Label = Window.GetControl (0x1000000f)
	Label.SetText (str(need_to_drop) )

	#done
	Button = Window.GetControl (8)
	if need_to_drop:
		Button.SetState (IE_GUI_BUTTON_DISABLED)
	else:
		Button.SetState (IE_GUI_BUTTON_ENABLED)

	#remove
	Button = Window.GetControl (15)
	if select:
		Button.SetState (IE_GUI_BUTTON_ENABLED)
	else:
		Button.SetState (IE_GUI_BUTTON_DISABLED)

	for i in range (PARTY_SIZE+1):
		Button = Window.GetControl (i)
		Button.EnableBorder (FRAME_PC_SELECTED, select == i+2 )
		#+2 because protagonist is skipped
		pic = GemRB.GetPlayerPortrait (i+2,1)
		if not pic:
			Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_SET)
			Button.SetState (IE_GUI_BUTTON_LOCKED)
			continue

		Button.SetState (IE_GUI_BUTTON_ENABLED)
		Button.SetFlags (IE_GUI_BUTTON_PICTURE|IE_GUI_BUTTON_ALIGN_BOTTOM|IE_GUI_BUTTON_ALIGN_LEFT, OP_SET)
		Button.SetPicture (pic, "NOPORTSM")
	UpdatePortraitWindow ()
	return

def RemovePlayer ():
	global ReformPartyWindow

	hideflag = GemRB.HideGUI ()

	GemRB.LoadWindowPack (GetWindowPack())
	if ReformPartyWindow:
		ReformPartyWindow.Unload ()
	ReformPartyWindow = Window = GemRB.LoadWindowObject (25)
	GemRB.SetVar ("OtherWindow", Window.ID)

	#are you sure
	Label = Window.GetControl (0x0fffffff)
	Label.SetText (17518)

	#confirm
	Button = Window.GetControl (1)
	Button.SetText (17507)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "RemovePlayerConfirm")
	Button.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)

	#cancel
	Button = Window.GetControl (2)
	Button.SetText (13727)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "RemovePlayerCancel")
	Button.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)

	GemRB.SetVar ("OtherWindow", Window.ID)
	GemRB.SetVar ("ActionsWindow", -1)
	if hideflag:
		GemRB.UnhideGUI ()
	Window.ShowModal (MODAL_SHADOW_GRAY)
	return

def RemovePlayerConfirm ():
	slot = GemRB.GetVar ("Selected")
	GemRB.LeaveParty (slot, 2)
	OpenReformPartyWindow ()
	return

def RemovePlayerCancel ():
	#Once for getting rid of the confirmation window
	OpenReformPartyWindow ()
	#and once for reopening the reform party window
	OpenReformPartyWindow ()
	return

def OpenReformPartyWindow ():
	global ReformPartyWindow, OldActionsWindow, OldMessageWindow

	GemRB.SetVar ("Selected", 0)
	hideflag = GemRB.HideGUI ()

	if ReformPartyWindow:
		if ReformPartyWindow:
			ReformPartyWindow.Unload ()

		GemRB.SetVar ("ActionsWindow", OldActionsWindow.ID)
		GemRB.SetVar ("MessageWindow", OldMessageWindow.ID)
		GemRB.SetVar ("OtherWindow", -1)

		OldActionsWindow = None
		OldMessageWindow = None
		ReformPartyWindow = None
		if hideflag:
			GemRB.UnhideGUI ()
		#re-enabling party size control
		GemRB.GameSetPartySize (PARTY_SIZE)
		UpdatePortraitWindow()
		return

	GemRB.LoadWindowPack (GetWindowPack())
	ReformPartyWindow = Window = GemRB.LoadWindowObject (24)
	GemRB.SetVar ("OtherWindow", Window.ID)

	#PC portraits
	for j in range (PARTY_SIZE+1):
		Button = Window.GetControl (j)
		Button.SetState (IE_GUI_BUTTON_LOCKED)
		Button.SetFlags (IE_GUI_BUTTON_RADIOBUTTON|IE_GUI_BUTTON_NO_IMAGE|IE_GUI_BUTTON_PICTURE,OP_SET)
		Button.SetBorder (FRAME_PC_SELECTED, 1, 1, 2, 2, 0, 255, 0, 255)
		#protagonist is skipped
		index = j + 2
		Button.SetVarAssoc ("Selected", index)
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "UpdateReformWindow")

	# Remove
	Button = Window.GetControl (15)
	Button.SetText (17507)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "RemovePlayer")

	# Done
	Button = Window.GetControl (8)
	Button.SetText (11973)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "OpenReformPartyWindow")

	OldActionsWindow = GWindow( GemRB.GetVar ("ActionsWindow") )
	OldMessageWindow = GWindow( GemRB.GetVar ("MessageWindow") )
	GemRB.SetVar ("ActionsWindow", -1)
	GemRB.SetVar ("MessageWindow", -1)
	UpdateReformWindow ()
	if hideflag:
		GemRB.UnhideGUI ()
	Window.ShowModal (MODAL_SHADOW_GRAY)
	return

def DeathWindow ():
	GemRB.HideGUI ()
	GemRB.SetTimedEvent ("DeathWindowEnd", 10)
	return

def DeathWindowEnd ():
	#playing death movie before continuing
	GemRB.PlayMovie ("deathand",1)
	GemRB.GamePause (1,1)

	GemRB.LoadWindowPack (GetWindowPack())
	Window = GemRB.LoadWindowObject (17)

	#reason for death
	Label = Window.GetControl (0x0fffffff)
	Label.SetText (16498)

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
	GemRB.QuitGame ()
	GemRB.SetNextScript ("Start")
	return

def LoadPress():
	GemRB.QuitGame ()
	GemRB.SetNextScript ("GUILOAD")
	return

