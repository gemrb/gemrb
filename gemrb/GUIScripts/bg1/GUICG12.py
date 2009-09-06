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
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
#
# $Id$
#
#character generation, appearance (GUICG12)
import GemRB

from CharGenCommon import * 
from GUICommon import CloseOtherWindow


AppearanceWindow = 0
CustomWindow = 0
PortraitButton = 0
PortraitsTable = 0
LastPortrait = 0
Gender = 0

def SetPicture ():
	global PortraitsTable, LastPortrait

	PortraitName = PortraitsTable.GetRowName (LastPortrait)+"G"
	PortraitButton.SetPicture (PortraitName)
	return

def OnLoad():
	global AppearanceWindow, PortraitButton, PortraitsTable, LastPortrait
	global Gender

	if CloseOtherWindow (OnLoad):
		if(AppearanceWindow):AppearanceWindow.Unload()
		AppearanceWindow = None
		return

	GemRB.LoadWindowPack("GUICG")
	AppearanceWindow = GemRB.LoadWindowObject (11)
	
	#Load the Gender
	Gender=GemRB.GetVar ("Gender")

	#Load the Portraits Table
	PortraitsTable = GemRB.LoadTableObject ("PICTURES")
	LastPortrait = 0

	PortraitButton = AppearanceWindow.GetControl (1)
	PortraitButton.SetFlags (IE_GUI_BUTTON_PICTURE|IE_GUI_BUTTON_NO_IMAGE,OP_SET)

	LeftButton = AppearanceWindow.GetControl (2)
	RightButton = AppearanceWindow.GetControl (3)

	BackButton = AppearanceWindow.GetControl (5)
	BackButton.SetText (15416)

	CustomButton = AppearanceWindow.GetControl (6)
	CustomButton.SetText (17545)

	DoneButton = AppearanceWindow.GetControl (0)
	DoneButton.SetText (11973)
	DoneButton.SetFlags (IE_GUI_BUTTON_DEFAULT,OP_OR)

	RightButton.SetEvent (IE_GUI_BUTTON_ON_PRESS,"RightPress")
	LeftButton.SetEvent (IE_GUI_BUTTON_ON_PRESS,"LeftPress")
	BackButton.SetEvent (IE_GUI_BUTTON_ON_PRESS,"BackPress")
	CustomButton.SetEvent (IE_GUI_BUTTON_ON_PRESS,"CustomPress")
	DoneButton.SetEvent (IE_GUI_BUTTON_ON_PRESS,"NextPress")
	
	while True:
		if PortraitsTable.GetValue (LastPortrait, 0) == Gender:
			SetPicture ()
			break
		LastPortrait = LastPortrait + 1
		if LastPortrait > 100:
			LastPortrait = 0
	AppearanceWindow.ShowModal(MODAL_SHADOW_NONE)
	return

def RightPress():
	global LastPortrait
	while True:
		LastPortrait = LastPortrait + 1
		if LastPortrait >= PortraitsTable.GetRowCount ():
			LastPortrait = 0
		if PortraitsTable.GetValue (LastPortrait, 0) == Gender:
			SetPicture ()
			return

def LeftPress():
	global LastPortrait
	while True:
		LastPortrait = LastPortrait - 1
		if LastPortrait < 0:
			LastPortrait = PortraitsTable.GetRowCount ()-1
		if PortraitsTable.GetValue (LastPortrait, 0) == Gender:
			SetPicture ()
			return

def CustomDone():
	Window = CustomWindow

	Portrait = PortraitList1.QueryText ()
	GemRB.SetToken ("LargePortrait", Portrait)
	Portrait = PortraitList2.QueryText ()
	GemRB.SetToken ("SmallPortrait", Portrait)
	if Window:
		Window.Unload ()
	if AppearanceWindow:
		AppearanceWindow.Unload ()
	next()
	return

def CustomAbort():
	if CustomWindow:
		CustomWindow.Unload ()
	return

def LargeCustomPortrait():
	Window = CustomWindow

	Portrait = PortraitList1.QueryText ()
	#small hack
	if GemRB.GetVar ("Row1") == RowCount1:
		return

	Label = Window.GetControl (0x10000007)
	Label.SetText (Portrait)

	Button = Window.GetControl (6)
	if Portrait=="":
		Portrait = "NOPORTLG"
		Button.SetState (IE_GUI_BUTTON_DISABLED)
	else:
		if PortraitList2.QueryText ()!="":
			Button.SetState (IE_GUI_BUTTON_ENABLED)

	Button = Window.GetControl (0)
	Button.SetPicture (Portrait, "NOPORTLG")
	return

def SmallCustomPortrait():
	Window = CustomWindow

	Portrait = PortraitList2.QueryText ()
	#small hack
	if GemRB.GetVar ("Row2") == RowCount2:
		return

	Label = Window.GetControl (0x10000008)
	Label.SetText (Portrait)

	Button = Window.GetControl (6)
	if Portrait=="":
		Portrait = "NOPORTSM"
		Button.SetState (IE_GUI_BUTTON_DISABLED)
	else:
		if PortraitList1.QueryText ()!="":
			Button.SetState (IE_GUI_BUTTON_ENABLED)

	Button = Window.GetControl (1)
	Button.SetPicture (Portrait, "NOPORTSM")
	return

def CustomPress():
	global PortraitList1, PortraitList2
	global RowCount1, RowCount2
	global CustomWindow

	CustomWindow = Window = GemRB.LoadWindowObject (18)
	PortraitList1 = Window.GetControl (2)
	RowCount1 = PortraitList1.GetPortraits (0)
	PortraitList1.SetEvent (IE_GUI_TEXTAREA_ON_CHANGE, "LargeCustomPortrait")
	GemRB.SetVar ("Row1", RowCount1)
	PortraitList1.SetVarAssoc ("Row1",RowCount1)

	PortraitList2 = Window.GetControl (4)
	RowCount2 = PortraitList2.GetPortraits (1)
	PortraitList2.SetEvent (IE_GUI_TEXTAREA_ON_CHANGE, "SmallCustomPortrait")
	GemRB.SetVar ("Row2", RowCount2)
	PortraitList2.SetVarAssoc ("Row2",RowCount2)

	Button = Window.GetControl (6)
	Button.SetText (11973)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "CustomDone")
	Button.SetState (IE_GUI_BUTTON_DISABLED)

	Button = Window.GetControl (7)
	Button.SetText (15416)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "CustomAbort")

	Button = Window.GetControl (0)
	PortraitName = PortraitsTable.GetRowName (LastPortrait)+"L"
	Button.SetPicture (PortraitName, "NOPORTLG")
	Button.SetState (IE_GUI_BUTTON_LOCKED)

	Button = Window.GetControl (1)
	PortraitName = PortraitsTable.GetRowName (LastPortrait)+"S"
	Button.SetPicture (PortraitName, "NOPORTSM")
	Button.SetState (IE_GUI_BUTTON_LOCKED)

	Window.ShowModal (MODAL_SHADOW_GRAY)
	return

def NextPress():
	PortraitTable = GemRB.LoadTableObject ("pictures")
	PortraitName = PortraitTable.GetRowName (LastPortrait )
	GemRB.SetToken ("SmallPortrait", PortraitName+"S")
	GemRB.SetToken ("LargePortrait", PortraitName+"L")
	next()
	#GemRB.SetVar("Step",1)
	#GemRB.SetNextScript("CharGen")
	#GemRB.SetNextScript ("CharGen2") #Before race
	#return

#def BackPress():
#	if AppearanceWindow:
#		AppearanceWindow.Unload ()
#	GemRB.SetNextScript ("GUICG1")
	#GemRB.SetVar ("Gender",0) #scrapping the gender value
#	return
