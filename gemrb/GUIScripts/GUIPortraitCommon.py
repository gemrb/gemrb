# SPDX-FileCopyrightText: 2026 Contributors to the GemRB project <https://gemrb.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

# code shared between the common GUICG12, CGPortrait
import GemRB
import GameCheck
from GUIDefines import *
from ie_restype import RES_BMP
from ie_stats import *

CustomWindow = 0
AppearanceWindow = 0
PortraitButton = 0
PortraitsTable = 0
LastPortrait = 0
Gender = 0
EmptyPortrait = {
	"big": "NOPORTLG",
	"medium": "NOPORTLG" if GameCheck.IsBG1OrEE() else "NOPORTMD",
	"small": "NOPORTSM",
}

PortraitSuffix = {
	"medium": "M",
	"large": "M" if (GameCheck.IsBG2OrEE() or GameCheck.IsBG2Demo()) else "L",
	"small": "S",
	"set": "G" if (GameCheck.IsBG1OrEE() or GameCheck.IsIWD1()) else "L",
}

def OnLoad():
	global AppearanceWindow, PortraitButton, PortraitsTable, LastPortrait
	global Gender

	# Window
	AppearanceWindow = GemRB.LoadWindow(11, "GUICG")
	
	# Load the Gender
	Gender = GetGender()
	
	# Optional extra setup
	if GameCheck.IsBG2OrEE() or GameCheck.IsBG2Demo():
		import CharGenCommon
		CharGenCommon.PositionCharGenWin(AppearanceWindow, -6)

		TextAreaControl = AppearanceWindow.GetControl(7)
		if TextAreaControl:
			TextAreaControl.SetText("")
	elif GameCheck.IsIWD2():
		import CharOverview
		CharOverview.PositionCharGenWin(AppearanceWindow)
		
	# Load the Portraits table
	PortraitsTable = GemRB.LoadTable("PICTURES")
	PortraitsStart = PortraitsTable.FindValue(0, 2)
	FemaleCountAdd = 1
	LastPortraitOffset = 0
	FemaleCount = PortraitsTable.GetRowCount() - PortraitsStart + FemaleCountAdd
	if Gender == 2:
		LastPortrait = GemRB.Roll(1, FemaleCount, PortraitsStart - 1)
	else:
		LastPortrait = GemRB.Roll(1, PortraitsTable.GetRowCount() - FemaleCount, LastPortraitOffset)

	# Controls
	PortraitButton = AppearanceWindow.GetControl(1)
	PortraitButton.SetFlags(IE_GUI_BUTTON_PICTURE | IE_GUI_BUTTON_NO_IMAGE, OP_SET)
	PortraitButton.SetState(IE_GUI_BUTTON_LOCKED)

	LeftButton = AppearanceWindow.GetControl(2)

	LeftButton.SetState(IE_GUI_BUTTON_ENABLED)
	LeftButton.SetFlags(IE_GUI_BUTTON_RADIOBUTTON, OP_OR)
		
	RightButton = AppearanceWindow.GetControl(3)

	RightButton.SetState(IE_GUI_BUTTON_ENABLED)
	RightButton.SetFlags(IE_GUI_BUTTON_RADIOBUTTON, OP_OR)
		
	BackButton = AppearanceWindow.GetControl(5)
	if GameCheck.IsIWD1():
		BackButton.SetText(13727)
	else:
		BackButton.SetText(15416)
	
	BackButton.MakeEscape()
		
	CustomButton = AppearanceWindow.GetControl(6)
	CustomButton.SetText(17545)
	CustomButton.SetState(IE_GUI_BUTTON_ENABLED)

	DoneButton = AppearanceWindow.GetControl(0)
	if GameCheck.IsIWD2():
		DoneButton.SetText(36789)
	else:
		DoneButton.SetText(11973)
		DoneButton.SetState(IE_GUI_BUTTON_ENABLED)
	DoneButton.MakeDefault()

	# Events
	RightButton.OnPress(PortraitButtonRightPress)
	LeftButton.OnPress(PortraitButtonLeftPress)
	if GameCheck.IsBG1OrEE():
		import CharGenCommon
		BackButton.OnPress(lambda: CharGenCommon.back(AppearanceWindow))
	else:
		BackButton.OnPress(PortraitBackPress)

	CustomButton.OnPress(PortraitButtonCustomDone)
	DoneButton.OnPress(PortraitButtonNextPress)
	PortraitSetPicture()
	AppearanceWindow.Focus()
	if GameCheck.IsBG1OrEE():
		AppearanceWindow.ShowModal(MODAL_SHADOW_GRAY)
	if GameCheck.IsIWD1():
		AppearanceWindow.ShowModal(MODAL_SHADOW_NONE)

# Function for getting the gender.
def GetGender():
	if GameCheck.IsIWD2() or GameCheck.IsBG2OrEE() or GameCheck.IsBG2Demo():
		return GemRB.GetVar("Gender")
	else:
		MyChar = GemRB.GetVar("Slot")
		return GemRB.GetPlayerStat(MyChar, IE_SEX)
	
# Button right press function	
def PortraitButtonRightPress():
	global LastPortrait
	LastPortrait = PortraitNext()
	PortraitSetPicture()

# Button left press function	
def PortraitButtonLeftPress():
	global LastPortrait
	LastPortrait = PortraitPrev()
	PortraitSetPicture()

# Button custom done
def PortraitButtonCustomDone():
	PortraitPictureButtonName = PortraitCustomDone()
	if GameCheck.IsBG1OrEE():
		import CharGenCommon
		CharGenCommon.next()
	elif GameCheck.IsIWD1():
		CGPortraitChangeToRace(PortraitPictureButtonName)

# Button next press
def PortraitButtonNextPress():
	global PortraitsTable, LastPortrait
	PortraitPictureButtonName = PortraitApplySelection()
	if GameCheck.IsBG1OrEE():
		import CharGenCommon
		CharGenCommon.next()
	elif GameCheck.IsIWD1():
		CGPortraitChangeToRace(PortraitPictureButtonName)
	
# This if for moving to next portrait
def PortraitNext():
	global LastPortrait
	while True:
		LastPortrait += 1
		if LastPortrait >= PortraitsTable.GetRowCount():
			LastPortrait = 0

		if PortraitsTable.GetValue(LastPortrait, 0) == Gender:
			return LastPortrait

# This is for moving to previous portrait
def PortraitPrev():
	global LastPortrait
	while True:
		LastPortrait -= 1
		if LastPortrait < 0:
			LastPortrait = PortraitsTable.GetRowCount() - 1

		if PortraitsTable.GetValue(LastPortrait, 0) == Gender:
			return LastPortrait

# This is for when is done
def PortraitCustomDone():
	global AppearanceWindow
	global PortraitList1, PortraitList2
	global RowCount1
	global CustomWindow

	PortraitLarge = PortraitList1.QueryText()
	GemRB.SetToken("LargePortrait", PortraitLarge)

	PortraitSmall = PortraitList2.QueryText()
	GemRB.SetToken("SmallPortrait", PortraitSmall)

	if CustomWindow:
		CustomWindow.Close()

	if AppearanceWindow:
		AppearanceWindow.Close()
	if GameCheck.IsIWD2() or GameCheck.IsBG2OrEE() or GameCheck.IsBG2Demo():
		GemRB.SetNextScript ("CharGen2")
	return PortraitLarge

# This is for abort
def PortraitCustomAbort():
	global CustomWindow, AppearanceWindow
	if CustomWindow:
		CustomWindow.Close()
	if AppearanceWindow:
		if GameCheck.IsBG1OrEE():
			AppearanceWindow.ShowModal (MODAL_SHADOW_GRAY) # narrower than CustomWindow, so borders will remain
		elif GameCheck.IsBG2OrEE() or GameCheck.IsBG2Demo():
			AppearanceWindow.ShowModal (MODAL_SHADOW_NONE) # narrower than CustomWindow, so borders will remain

# This is for applying
def PortraitApplySelection():
	global AppearanceWindow, LastPortrait
	if AppearanceWindow:
		AppearanceWindow.Close ()
	PortraitTable = GemRB.LoadTable("pictures")
	PortraitName = PortraitTable.GetRowName(LastPortrait)
	GemRB.SetToken("SmallPortrait", PortraitName + PortraitSuffix["small"])
	GemRB.SetToken("LargePortrait", PortraitName + PortraitSuffix["large"])
	if GameCheck.IsIWD2() or GameCheck.IsBG2OrEE() or GameCheck.IsBG2Demo():
		GemRB.SetNextScript ("CharGen2") #Before race
	return PortraitName + PortraitSuffix["large"]

# This is for the large custom portrait
def PortraitCommonLargeCustom():
	global PortraitList1, PortraitList2
	global RowCount1
	global CustomWindow

	window = CustomWindow

	portrait = PortraitList1.QueryText()

	# small hack
	if GemRB.GetVar("Row1") == RowCount1:
		return

	label = window.GetControl(0x10000007)
	label.SetText(portrait)

	button = window.GetControl(6)

	if portrait == "":
		portrait = EmptyPortrait["medium"]
		if GameCheck.IsBG2OrEE() or GameCheck.IsBG2Demo():
			button.SetDisabled(True)
		else:
			button.SetState(IE_GUI_BUTTON_DISABLED)
	else:
		if PortraitList2.QueryText() != "":
			if GameCheck.IsBG2OrEE() or GameCheck.IsBG2Demo():
				button.SetDisabled(False)
			else:
				button.SetState(IE_GUI_BUTTON_ENABLED)

	preview = window.GetControl(0)
	preview.SetPicture(portrait, EmptyPortrait["medium"])

# This is for the small custom portrait
def PortraitCommonSmallCustom():
	global PortraitList1, PortraitList2
	global RowCount2
	global CustomWindow

	window = CustomWindow

	portrait = PortraitList2.QueryText()

	# small hack
	if GemRB.GetVar("Row2") == RowCount2:
		return

	label = window.GetControl(0x10000008)
	label.SetText(portrait)

	button = window.GetControl(6)

	if portrait == "":
		portrait = EmptyPortrait["small"]

		if GameCheck.IsBG2OrEE() or GameCheck.IsBG2Demo():
			button.SetDisabled(True)
		else:
			button.SetState(IE_GUI_BUTTON_DISABLED)
	else:
		if PortraitList1.QueryText() != "":
			if GameCheck.IsBG2OrEE() or GameCheck.IsBG2Demo():
				button.SetDisabled(False)
			else:
				button.SetState(IE_GUI_BUTTON_ENABLED)

	preview = window.GetControl(1)
	preview.SetPicture(portrait, EmptyPortrait["small"])


def PortraitCustomPress(CustomDone):
	global PortraitsTable, LastPortrait
	global PortraitList1, PortraitList2
	global RowCount1, RowCount2
	global CustomWindow

	CustomWindow = Window = GemRB.LoadWindow(18, "GUICG")

	ListMode1 = 1
	if GameCheck.IsIWD2():
		ListMode1 = 2
	PortraitList1 = Window.GetControl(2)
	RowCount1 = len(PortraitList1.ListResources(CHR_PORTRAITS, ListMode1))
	PortraitList1.OnSelect(PortraitCommonLargeCustom)
	PortraitList1.SetVarAssoc("Row1", RowCount1)

	PortraitList2 = Window.GetControl(4)
	RowCount2 = len(PortraitList2.ListResources(CHR_PORTRAITS, 0))
	PortraitList2.OnSelect(PortraitCommonSmallCustom)
	PortraitList2.SetVarAssoc("Row2", RowCount2)

	Button = Window.GetControl(6)
	Button.SetText(11973)
	Button.MakeDefault()
	
	Button.OnPress(CustomDone)
	if GameCheck.IsBG2OrEE() or GameCheck.IsBG2Demo():
		Button.SetDisabled(True)
	else:
		Button.SetState(IE_GUI_BUTTON_DISABLED)

	Button = Window.GetControl(7)
	Button.SetText(15416)
	if GameCheck.IsIWD2():
		Button.MakeEscape()
	Button.OnPress(PortraitCustomAbort)
	
	Button = Window.GetControl(0)
	PortraitName = PortraitsTable.GetRowName(LastPortrait) + PortraitSuffix["large"]
	Button.SetPicture(PortraitName, EmptyPortrait["medium"])

	Button.SetState(IE_GUI_BUTTON_LOCKED)

	Button = Window.GetControl(1)
	PortraitName = PortraitsTable.GetRowName(LastPortrait) + PortraitSuffix["small"]
	Button.SetPicture(PortraitName, EmptyPortrait["small"])
	Button.SetState(IE_GUI_BUTTON_LOCKED)

	ModalShadow = MODAL_SHADOW_NONE
	if GameCheck.IsBG1OrEE():
		ModalShadow = MODAL_SHADOW_GRAY
	Window.ShowModal(ModalShadow)

def PortraitBackPress():
	global AppearanceWindow
	if AppearanceWindow:
		AppearanceWindow.Close ()
	if GameCheck.IsIWD1():
		return
	NextScript = "GUICG1"
	if GameCheck.IsIWD2():
		NextScript = "CharGen"
	GemRB.SetNextScript (NextScript)
	GemRB.SetVar ("Gender",0) #scrapping the gender value
	return

def PortraitSetPicture():
	global PortraitButton
	PortraitName = PortraitsTable.GetRowName (LastPortrait)+PortraitSuffix["set"]
	PortraitButton.SetPicture(PortraitName, EmptyPortrait["big"])
	return

def CGPortraitChangeToRace(PortraitPictureButtonName):
	import CharGen
	CharGen.PortraitButton.SetPicture(PortraitPictureButtonName)
	CharGen.GenderButton.SetState(IE_GUI_BUTTON_DISABLED)
	CharGen.RaceButton.SetState(IE_GUI_BUTTON_ENABLED)
	CharGen.RaceButton.MakeDefault()
	CharGen.CharGenState = 1
	CharGen.SetCharacterDescription()
