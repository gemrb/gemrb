# SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

#character generation, appearance (GUICG12)
import GemRB
import CharOverview
from GUIDefines import *
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

def OnLoad ():
	global AppearanceWindow, PortraitButton, PortraitsTable, LastPortrait
	global Gender

	Gender=GemRB.GetVar ("Gender")

	AppearanceWindow = GemRB.LoadWindow (11, "GUICG")
	CharOverview.PositionCharGenWin(AppearanceWindow)

	# Load the Portraits Table, sorted by gender col (1: m, 2: f)
	PortraitsTable = GemRB.LoadTable ("PICTURES")
	FemalePortraitsStart = PortraitsTable.FindValue (0, 2)
	FemaleCount = PortraitsTable.GetRowCount () - FemalePortraitsStart

	if Gender == 2:
		LastPortrait = GemRB.Roll (1, FemaleCount, FemalePortraitsStart - 1)
	else:
		LastPortrait = GemRB.Roll (1, PortraitsTable.GetRowCount() - FemaleCount, -1)

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
	DoneButton.SetText (36789)
	DoneButton.MakeDefault()

	RightButton.OnPress (RightPress)
	LeftButton.OnPress (LeftPress)
	BackButton.OnPress (BackPress)
	CustomButton.OnPress (CustomPress)
	DoneButton.OnPress (NextPress)

	SetPicture ()
	AppearanceWindow.Focus()

	return

def RightPress ():
	global LastPortrait
	LastPortrait = portrait_next(PortraitsTable, LastPortrait, Gender)
	SetPicture()

def LeftPress ():
	global LastPortrait
	LastPortrait = portrait_prev(PortraitsTable, LastPortrait, Gender)
	SetPicture()

def BackPress ():
	portrait_back_press(AppearanceWindow)
	return

def CustomDone ():
	portrait_custom_done(AppearanceWindow)
	return

def CustomAbort ():
	portrait_custom_abort(CustomWindow, AppearanceWindow)
	return

def LargeCustomPortrait():
    portrait_common_large_custom()

def SmallCustomPortrait ():
	portrait_common_small_custom()

def CustomPress ():
    portrait_custom_press(
        PortraitsTable,
        LastPortrait,
        LargeCustomPortrait,
        SmallCustomPortrait,
        CustomDone,
        CustomAbort
    )

def NextPress ():
	portrait_apply_selection(AppearanceWindow, LastPortrait)
	return
