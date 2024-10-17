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
#
#character generation, appearance (GUICG12)
import GemRB
from ie_restype import RES_BMP

import CharGenCommon

AppearanceWindow = 0
CustomWindow = 0
PortraitButton = 0
PortraitsTable = 0
LastPortrait = 0
Gender = 0

def SetPicture ():
	global PortraitsTable, LastPortrait

	PortraitName = PortraitsTable.GetRowName (LastPortrait)+"L"
	PortraitButton.SetPicture (PortraitName, "NOPORTLG")
	return

def OnLoad():
	global AppearanceWindow, PortraitButton, PortraitsTable, LastPortrait
	global Gender
	
	Gender=GemRB.GetVar ("Gender")

	AppearanceWindow = GemRB.LoadWindow (11, "GUICG")
	CharGenCommon.PositionCharGenWin(AppearanceWindow, -6)

	#Load the Portraits Table
	PortraitsTable = GemRB.LoadTable ("PICTURES")
	PortraitsStart = PortraitsTable.FindValue (0, 2)
	FemaleCount = PortraitsTable.GetRowCount () - PortraitsStart + 1
	if Gender == 2:
		LastPortrait = GemRB.Roll (1, FemaleCount, PortraitsStart-1)
	else:
		LastPortrait = GemRB.Roll (1, PortraitsTable.GetRowCount()-FemaleCount, 0)

	#this control doesn't exist in the demo version (it is unused, so lets just skip it)
	TextAreaControl = AppearanceWindow.GetControl (7)
	if TextAreaControl:
		TextAreaControl.SetText ("")

	PortraitButton = AppearanceWindow.GetControl (1)
	PortraitButton.SetFlags (IE_GUI_BUTTON_PICTURE|IE_GUI_BUTTON_NO_IMAGE,OP_SET)
	PortraitButton.SetState (IE_GUI_BUTTON_LOCKED)

	LeftButton = AppearanceWindow.GetControl (2)
	RightButton = AppearanceWindow.GetControl (3)

	BackButton = AppearanceWindow.GetControl (5)
	BackButton.SetText (15416)
	BackButton.MakeEscape()

	CustomButton = AppearanceWindow.GetControl (6)
	CustomButton.SetText (17545)

	DoneButton = AppearanceWindow.GetControl (0)
	DoneButton.SetText (11973)
	DoneButton.MakeDefault()

	RightButton.OnPress (RightPress)
	LeftButton.OnPress (LeftPress)
	BackButton.OnPress (BackPress)
	CustomButton.OnPress (CustomPress)
	DoneButton.OnPress (NextPress)
	
	Flag = False
	while True:
		if PortraitsTable.GetValue (LastPortrait, 0) == Gender:
			SetPicture ()
			break
		LastPortrait = LastPortrait + 1
		if LastPortrait >= PortraitsTable.GetRowCount ():
			LastPortrait = 0
			if Flag:
				SetPicture ()
				break
			Flag = True

	AppearanceWindow.Focus()
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

def BackPress():
	if AppearanceWindow:
		AppearanceWindow.Close ()
	GemRB.SetNextScript ("GUICG1")
	GemRB.SetVar ("Gender",0) #scrapping the gender value
	return

def CustomDone():
	Window = CustomWindow

	Portrait = PortraitList1.QueryText ()
	GemRB.SetToken ("LargePortrait", Portrait)
	Portrait = PortraitList2.QueryText ()
	GemRB.SetToken ("SmallPortrait", Portrait)
	if Window:
		Window.Close ()
	if AppearanceWindow:
		AppearanceWindow.Close ()
	GemRB.SetNextScript ("CharGen2")
	return

def CustomAbort():
	if CustomWindow:
		CustomWindow.Close ()
	AppearanceWindow.ShowModal (MODAL_SHADOW_NONE) # narrower than CustomWindow, so borders will remain
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
		Portrait = "NOPORTMD"
		Button.SetDisabled (True)
	else:
		if PortraitList2.QueryText ()!="":
			Button.SetDisabled (False)

	Button = Window.GetControl (0)
	Button.SetPicture (Portrait, "NOPORTMD")
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
		Button.SetDisabled (True)
	else:
		if PortraitList1.QueryText ()!="":
			Button.SetDisabled (False)

	Button = Window.GetControl (1)
	Button.SetPicture (Portrait, "NOPORTSM")
	return

def CustomPress():
	global PortraitList1, PortraitList2
	global RowCount1, RowCount2
	global CustomWindow

	CustomWindow = Window = GemRB.LoadWindow (18, "GUICG")
	PortraitList1 = Window.GetControl (2)
	RowCount1 = len(PortraitList1.ListResources (CHR_PORTRAITS, 1))
	PortraitList1.OnSelect (LargeCustomPortrait)
	PortraitList1.SetVarAssoc ("Row1",RowCount1)

	PortraitList2 = Window.GetControl (4)
	RowCount2 = len(PortraitList2.ListResources (CHR_PORTRAITS, 0))
	PortraitList2.OnSelect (SmallCustomPortrait)
	PortraitList2.SetVarAssoc ("Row2",RowCount2)

	Button = Window.GetControl (6)
	Button.SetText (11973)
	Button.OnPress (CustomDone)
	Button.SetDisabled (True)

	Button = Window.GetControl (7)
	Button.SetText (15416)
	Button.OnPress (CustomAbort)

	Button = Window.GetControl (0)
	PortraitName = PortraitsTable.GetRowName (LastPortrait)+"M"
	if GemRB.HasResource (PortraitName, RES_BMP, 1) or GemRB.HasResource ("NOPORTMD", RES_BMP, 1):
		Button.SetPicture (PortraitName, "NOPORTMD")
	Button.SetState (IE_GUI_BUTTON_LOCKED)

	Button = Window.GetControl (1)
	PortraitName = PortraitsTable.GetRowName (LastPortrait)+"S"
	Button.SetPicture (PortraitName, "NOPORTSM")
	Button.SetState (IE_GUI_BUTTON_LOCKED)

	Window.ShowModal (MODAL_SHADOW_NONE)
	return

def NextPress():
	if AppearanceWindow:
		AppearanceWindow.Close ()
	PortraitTable = GemRB.LoadTable ("pictures")
	PortraitName = PortraitTable.GetRowName (LastPortrait )
	GemRB.SetToken ("SmallPortrait", PortraitName+"S")
	GemRB.SetToken ("LargePortrait", PortraitName+"M")
	GemRB.SetNextScript ("CharGen2") #Before race
	return

