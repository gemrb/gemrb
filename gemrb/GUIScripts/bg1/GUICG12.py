# SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

#character generation, portrait (GUICG12)
import GemRB
from GUIDefines import *
from ie_stats import *
import CharGenCommon
import GUICommon
from GUIPortraitCommon import *

AppearanceWindow = 0
PortraitButton = 0
PortraitsTable = 0
LastPortrait = 0
Gender = 0

def SetPicture ():
	portrait_set_picture(PortraitButton, PortraitsTable, LastPortrait)
	return

def OnLoad():
	global AppearanceWindow, PortraitButton, PortraitsTable, LastPortrait
	global Gender

	AppearanceWindow = GemRB.LoadWindow (11, "GUICG")

	#Load the Gender
	MyChar = GemRB.GetVar ("Slot")
	Gender = GemRB.GetPlayerStat (MyChar, IE_SEX)

	#Load the Portraits Table
	PortraitsTable = GemRB.LoadTable ("PICTURES")
	PortraitsStart = PortraitsTable.FindValue (0, 2)
	FemaleCount = PortraitsTable.GetRowCount () - PortraitsStart + 1
	if Gender == 2:
		LastPortrait = GemRB.Roll (1, FemaleCount, PortraitsStart-1)
	else:
		LastPortrait = GemRB.Roll (1, PortraitsTable.GetRowCount()-FemaleCount, 0)

	PortraitButton = AppearanceWindow.GetControl (1)
	PortraitButton.SetFlags (IE_GUI_BUTTON_PICTURE|IE_GUI_BUTTON_NO_IMAGE,OP_SET)
	PortraitButton.SetState (IE_GUI_BUTTON_LOCKED)

	LeftButton = AppearanceWindow.GetControl (2)
	RightButton = AppearanceWindow.GetControl (3)

	BackButton = AppearanceWindow.GetControl (5)
	BackButton.SetText (15416)

	CustomButton = AppearanceWindow.GetControl (6)
	CustomButton.SetText (17545)

	DoneButton = AppearanceWindow.GetControl (0)
	DoneButton.SetText (11973)
	DoneButton.MakeDefault()

	RightButton.OnPress (RightPress)
	LeftButton.OnPress (LeftPress)
	BackButton.OnPress (lambda: CharGenCommon.back(AppearanceWindow))
	CustomButton.OnPress (CustomPress)
	DoneButton.OnPress (NextPress)

	flag = False
	while True:
		if PortraitsTable.GetValue (LastPortrait, 0) == Gender:
			SetPicture ()
			break
		LastPortrait = LastPortrait + 1
		if LastPortrait >= PortraitsTable.GetRowCount ():
			LastPortrait = 0
			if flag:
				SetPicture ()
				break
			flag = True

	AppearanceWindow.ShowModal(MODAL_SHADOW_GRAY)
	return

def RightPress():
	global LastPortrait
	LastPortrait = portrait_next(PortraitsTable, LastPortrait, Gender)
	SetPicture()

def LeftPress():
	global LastPortrait
	LastPortrait = portrait_prev(PortraitsTable, LastPortrait, Gender)
	SetPicture()

def CustomDone():
	global AppearanceWindow
	portrait_custom_done(AppearanceWindow)
	CharGenCommon.next()
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
    CharGenCommon.next()
