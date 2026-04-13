# SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

#character generation, appearance (GUICG12)
import GemRB
from ie_restype import RES_BMP

import CharGenCommon
from GUIPortraitCommon import *

AppearanceWindow = 0
PortraitButton = 0
PortraitsTable = 0
LastPortrait = 0
Gender = 0

def SetPicture ():
	global PortraitsTable, LastPortrait
	portrait_set_picture(PortraitButton, PortraitsTable, LastPortrait)
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
	LastPortrait = portrait_next(PortraitsTable, LastPortrait, Gender)
	SetPicture()

def LeftPress():
	global LastPortrait
	LastPortrait = portrait_prev(PortraitsTable, LastPortrait, Gender)
	SetPicture()

def BackPress():
	portrait_back_press(AppearanceWindow)
	return

def CustomDone():
	portrait_custom_done(AppearanceWindow)
	return

def CustomAbort():
	portrait_custom_abort(AppearanceWindow)
	return

def CustomPress():
    portrait_custom_press(
        PortraitsTable,
        LastPortrait,
        CustomDone,
        CustomAbort
    )

def NextPress():
	portrait_apply_selection(AppearanceWindow, LastPortrait)
	return

