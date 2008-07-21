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
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
#
# $Id$


# GUIW.py - scripts to control some windows from GUIWORLD winpack
# except of Actions, Portrait, Options and Dialog windows
#################################################################

import GemRB
from GUIDefines import *
from GUICommon import CloseOtherWindow
from GUICommonWindows import *

FRAME_PC_SELECTED = 0
FRAME_PC_TARGET   = 1

ContainerWindow = None
ContinueWindow = None
ReformPartyWindow = None
OldActionsWindow = None
OldMessageWindow = None
Container = None

def CloseContinueWindow ():
	global ContinueWindow, OldActionsWindow

	if ContinueWindow == None:
		return

	hideflag = GemRB.HideGUI ()

	GemRB.UnloadWindow (ContinueWindow)
	GemRB.SetVar ("ActionsWindow", OldActionsWindow)
	GemRB.SetVar ("DialogChoose", GemRB.GetVar ("DialogOption"))
	ContinueWindow = None
	OldActionsWindow = None
	if hideflag:
		GemRB.UnhideGUI ()


def OpenEndMessageWindow ():
	global ContinueWindow, OldActionsWindow

	if ContinueWindow:
		return

	hideflag = GemRB.HideGUI ()

	GemRB.LoadWindowPack (GetWindowPack())
	ContinueWindow = Window = GemRB.LoadWindow (9)
	OldActionsWindow = GemRB.GetVar ("ActionsWindow")
	GemRB.SetVar ("ActionsWindow", Window)

	#end dialog
	Button = GemRB.GetControl (Window, 0)
	GemRB.SetText (Window, Button, 9371)	
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "CloseContinueWindow")
	if hideflag:
		GemRB.UnhideGUI ()


def OpenContinueMessageWindow ():
	global ContinueWindow, OldActionsWindow

	if ContinueWindow:
		return

	hideflag = GemRB.HideGUI ()

	GemRB.LoadWindowPack (GetWindowPack())
	ContinueWindow = Window = GemRB.LoadWindow (9)
	OldActionsWindow = GemRB.GetVar ("ActionsWindow")
	GemRB.SetVar ("ActionsWindow", Window)

	#continue
	Button = GemRB.GetControl (Window, 0)
	GemRB.SetText (Window, Button, 9372)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "CloseContinueWindow")
	if hideflag:
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

	for i in range (6):
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


	for i in range (4):
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

	hideflag = GemRB.HideGUI ()

	GemRB.LoadWindowPack (GetWindowPack())
	ContainerWindow = Window = GemRB.LoadWindow (8)
	OldActionsWindow = GemRB.GetVar ("ActionsWindow")
	OldMessageWindow = GemRB.GetVar ("MessageWindow")
	GemRB.SetVar ("ActionsWindow", Window)
	GemRB.SetVar ("MessageWindow", -1)

	Container = GemRB.GetContainer(0)

	# 0 - 5 - Ground Item
	# 10 - 13 - Personal Item
	# 50 hand
	# 52, 53 scroller ground, scroller personal
	# 54 - encumbrance

	for i in range (6):
		Button = GemRB.GetControl (Window, i)
		GemRB.SetVarAssoc (Window, Button, "LeftIndex", i)
		#GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_CHECKBOX, OP_OR)
		GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "TakeItemContainer")

	for i in range (4):
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
	Label = GemRB.GetControl (Window, 0x1000000f)
	GemRB.SetText (Window, Label, str(need_to_drop) )

	#done
	Button = GemRB.GetControl (Window, 8)
	if need_to_drop:
		GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_DISABLED)
	else:
		GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_ENABLED)

	#remove
	Button = GemRB.GetControl (Window, 15)
	if select:
		GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_ENABLED)
	else:
		GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_DISABLED)

	for i in range (PARTY_SIZE+1):
		Button = GemRB.GetControl (Window, i)
		GemRB.EnableButtonBorder (Window, Button, FRAME_PC_SELECTED, select == i+2 )
		#+2 because protagonist is skipped
		pic = GemRB.GetPlayerPortrait (i+2,1)
		if not pic:
			GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_NO_IMAGE, OP_SET)
			GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_LOCKED)
			continue

		GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_ENABLED)
		GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_PICTURE|IE_GUI_BUTTON_ALIGN_BOTTOM|IE_GUI_BUTTON_ALIGN_LEFT, OP_SET)
		GemRB.SetButtonPicture (Window, Button, pic, "NOPORTSM")
	UpdatePortraitWindow ()
	return

def RemovePlayer ():
	global ReformPartyWindow

	hideflag = GemRB.HideGUI ()

	GemRB.LoadWindowPack (GetWindowPack())
	GemRB.UnloadWindow (ReformPartyWindow)
	ReformPartyWindow = Window = GemRB.LoadWindow (25)
	GemRB.SetVar ("OtherWindow", Window)

	#are you sure
	Label = GemRB.GetControl (Window, 0x0fffffff)
	GemRB.SetText (Window, Label, 17518)

	#confirm
	Button = GemRB.GetControl (Window, 1)
	GemRB.SetText (Window, Button, 17507)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "RemovePlayerConfirm")

	#cancel
	Button = GemRB.GetControl (Window, 2)
	GemRB.SetText (Window, Button, 13727)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "RemovePlayerCancel")

def RemovePlayerConfirm ():
	global ReformPartyWindow

	hideflag = GemRB.HideGUI ()
	GemRB.UnloadWindow (ReformPartyWindow)
	GemRB.SetVar ("OtherWindow", -1)
	#removing selected player
	ReformPartyWindow = None
	if hideflag:
		GemRB.UnhideGUI ()
	GemRB.LeaveParty (GemRB.GetVar("Selected") )
	OpenReformPartyWindow ()
	return

def RemovePlayerCancel ():
	global ReformPartyWindow

	hideflag = GemRB.HideGUI ()
	GemRB.UnloadWindow (ReformPartyWindow)
	GemRB.SetVar ("OtherWindow", -1)
	ReformPartyWindow = None
	if hideflag:
		GemRB.UnhideGUI ()
	OpenReformPartyWindow ()
	return

def OpenReformPartyWindow ():
	global ReformPartyWindow, OldActionsWindow, OldMessageWindow

	GemRB.SetVar ("Selected", 0)
	hideflag = GemRB.HideGUI ()

	if ReformPartyWindow:
		GemRB.UnloadWindow (ReformPartyWindow)
		GemRB.SetVar ("ActionsWindow", OldActionsWindow)
		GemRB.SetVar ("MessageWindow", OldMessageWindow)
		GemRB.SetVar ("OtherWindow", -1)

		OldActionsWindow = None
		OldMessageWindow = None
		ReformPartyWindow = None

		#GemRB.LoadWindowPack ("GUIREC")
		if hideflag:
			GemRB.UnhideGUI ()
		#re-enabling party size control
		GemRB.GameSetPartySize (PARTY_SIZE)
		return

	GemRB.LoadWindowPack (GetWindowPack())
	ReformPartyWindow = Window = GemRB.LoadWindow (24)
	GemRB.SetVar ("OtherWindow", Window)

	#PC portraits
	for j in range (PARTY_SIZE+1):
		Button = GemRB.GetControl (Window,j)
		GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_LOCKED)
		GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_RADIOBUTTON|IE_GUI_BUTTON_NO_IMAGE|IE_GUI_BUTTON_PICTURE,OP_SET)
		GemRB.SetButtonBorder (Window, Button, FRAME_PC_SELECTED, 1, 1, 2, 2, 0, 255, 0, 255)
		#protagonist is skipped
		index = j + 2
		GemRB.SetVarAssoc (Window, Button, "Selected", index)
		GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "UpdateReformWindow")

	# Remove
	Button = GemRB.GetControl (Window, 15)
	GemRB.SetText (Window, Button, 17507)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "RemovePlayer")

	# Done
	Button = GemRB.GetControl (Window, 8)
	GemRB.SetText (Window, Button, 11973)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenReformPartyWindow")

	OldActionsWindow = GemRB.GetVar ("ActionsWindow")
	OldMessageWindow = GemRB.GetVar ("MessageWindow")
	GemRB.SetVar ("ActionsWindow", -1)
	GemRB.SetVar ("MessageWindow", -1)
	UpdateReformWindow ()
	if hideflag:
		GemRB.UnhideGUI ()
	GemRB.ShowModal (Window, MODAL_SHADOW_GRAY)
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
	Window = GemRB.LoadWindow (17)

	#reason for death
	Label = GemRB.GetControl (Window, 0x0fffffff)
	GemRB.SetText (Window, Label, 16498)

	#load
	Button = GemRB.GetControl (Window, 1)
	GemRB.SetText (Window, Button, 15590)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "LoadPress")

	#quit
	Button = GemRB.GetControl (Window, 2)
	GemRB.SetText (Window, Button, 15417)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "QuitPress")

	GemRB.HideGUI ()
	GemRB.SetVar ("MessageWindow", -1)
	GemRB.SetVar ("ActionsWindow", Window)
	GemRB.UnhideGUI ()
	#making the playing field gray
	GemRB.SetVisible (0,2)
	Window = GemRB.GetVar ("PortraitWindow")
	GemRB.SetVisible (Window,2)
	Window = GemRB.GetVar ("OptionsWindow")
	GemRB.SetVisible (Window,2)
	return

def QuitPress():
	GemRB.QuitGame ()
	GemRB.SetNextScript ("Start")
	return

def LoadPress():
	GemRB.QuitGame ()
	GemRB.SetNextScript ("GUILOAD")
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


