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
IsGUIRECCommon = False

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

def OnLoad(GUIRECCommon = False, UseGender = None, PortraitName = None):
	global AppearanceWindow, PortraitButton, PortraitsTable, LastPortrait
	global Gender, IsGUIRECCommon
	
	IsGUIRECCommon = GUIRECCommon
	# Window
	if GUIRECCommon:
		AppearanceWindow = GemRB.LoadWindow(18, "GUIREC")
		AppearanceWindow.AddAlias("SUB_WIN", 0)
	else:
		AppearanceWindow = GemRB.LoadWindow(11, "GUICG")
	
	# Load the Gender
	if GUIRECCommon:
		Gender = UseGender
	else:
		Gender = GetGender()
	if not GUIRECCommon:
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
	if not GUIRECCommon:
		PortraitsStart = PortraitsTable.FindValue(0, 2)
		FemaleCountAdd = 1
		LastPortraitOffset = 0
		if GameCheck.IsIWD2():
			FemaleCountAdd = 0
			LastPortraitOffset = -1
		FemaleCount = PortraitsTable.GetRowCount() - PortraitsStart + FemaleCountAdd
		if Gender == 2:
			LastPortrait = GemRB.Roll(1, FemaleCount, PortraitsStart - 1)
		else:
			LastPortrait = GemRB.Roll(1, PortraitsTable.GetRowCount() - FemaleCount, LastPortraitOffset)
	else:
		if GameCheck.IsBG2OrEE():
			PortraitName = PortraitName.rstrip("[ms]")
		else:
			PortraitName = PortraitName.rstrip("[ls]")

		# capitalize PortraitName
		PortraitName = PortraitName.upper()

		# search table
		for i in range(0, PortraitsTable.GetRowCount()):
			if PortraitName == PortraitsTable.GetRowName(i).upper():
				LastPortrait = i
				break

	# Controls
	PortraitButtonNumber = 1
	if GUIRECCommon:
		PortraitButtonNumber = 0
	PortraitButton = AppearanceWindow.GetControl(PortraitButtonNumber)
	PortraitButton.SetFlags(IE_GUI_BUTTON_PICTURE | IE_GUI_BUTTON_NO_IMAGE, OP_SET)
	PortraitButton.SetState(IE_GUI_BUTTON_LOCKED)

	LeftButtonNumber = 2
	if GUIRECCommon:
		LeftButtonNumber = 1
	LeftButton = AppearanceWindow.GetControl(LeftButtonNumber)
	LeftButton.SetState(IE_GUI_BUTTON_ENABLED)
	LeftButton.SetFlags(IE_GUI_BUTTON_RADIOBUTTON, OP_OR)
	LeftButton.OnPress(PortraitButtonLeftPress)
	
	RightButtonNumber = 3
	if GUIRECCommon:
		RightButtonNumber = 2
	RightButton = AppearanceWindow.GetControl(RightButtonNumber)
	RightButton.SetState(IE_GUI_BUTTON_ENABLED)
	RightButton.SetFlags(IE_GUI_BUTTON_RADIOBUTTON, OP_OR)
	RightButton.OnPress(PortraitButtonRightPress)
	
	if GUIRECCommon:
		CancelButton = AppearanceWindow.GetControl(4)
		CancelButton.SetState(IE_GUI_BUTTON_ENABLED)
		CancelButton.OnPress(AppearanceWindow.Close)
		CancelButton.SetText(13727)
		CancelButton.MakeEscape()
	else:
		BackButton = AppearanceWindow.GetControl(5)
		if GameCheck.IsIWD1():
			BackButton.SetText(13727)
		else:
			BackButton.SetText(15416)
		
		BackButton.MakeEscape()
		if GameCheck.IsBG1OrEE():
			import CharGenCommon
			BackButton.OnPress(lambda: CharGenCommon.back(AppearanceWindow))
		else:
			BackButton.OnPress(PortraitBackPress)
	
	CustomButtonNumber = 6
	if GUIRECCommon:
		CustomButtonNumber = 5
	CustomButton = AppearanceWindow.GetControl(CustomButtonNumber)
	CustomButton.SetText(17545)
	CustomButton.SetState(IE_GUI_BUTTON_ENABLED)
	CustomButton.OnPress(PortraitCustomPress)
	
	DoneButtonNumber = 0
	if GUIRECCommon:
		DoneButtonNumber = 3
	DoneButton = AppearanceWindow.GetControl(DoneButtonNumber)
	if not GUIRECCommon and GameCheck.IsIWD2():
		DoneButton.SetText(36789)
	else:
		DoneButton.SetText(11973)
		DoneButton.SetState(IE_GUI_BUTTON_ENABLED)
	DoneButton.MakeDefault()
	DoneButton.OnPress(PortraitButtonNextPress)
	PortraitSetPicture()
	AppearanceWindow.Focus()
	if GUIRECCommon or GameCheck.IsBG1OrEE():
		AppearanceWindow.ShowModal(MODAL_SHADOW_GRAY)
	elif GameCheck.IsIWD1():
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
	if not IsGUIRECCommon:
		PortraitPictureButtonName = PortraitCustomDone()
		if GameCheck.IsBG1OrEE():
			import CharGenCommon
			CharGenCommon.next()
		elif GameCheck.IsIWD1():
			CGPortraitChangeToRace(PortraitPictureButtonName)
	else:
		pc = GemRB.GameGetSelectedPCSingle()
		GemRB.FillPlayerInfo(pc, PortraitList1.QueryText(), PortraitList2.QueryText())
		if CustomWindow:
			CustomWindow.Close()
		if AppearanceWindow:
			AppearanceWindow.Close()

# Button next press
def PortraitButtonNextPress():
	global PortraitsTable, LastPortrait
	PortraitPictureButtonName = PortraitApplySelection()
	print(PortraitPictureButtonName)
	if not IsGUIRECCommon:
		if GameCheck.IsBG1OrEE():
			import CharGenCommon
			CharGenCommon.next()
		elif GameCheck.IsIWD1():
			CGPortraitChangeToRace(PortraitPictureButtonName)
	else:
		PortraitName = PortraitPictureButtonName[:-1]
		pc = GemRB.GameGetSelectedPCSingle()
		print(PortraitName)
		# eh, different sizes
		if GameCheck.IsBG2OrEE():
			GemRB.FillPlayerInfo(pc, PortraitName + "M", PortraitName + "S")
		else:
			GemRB.FillPlayerInfo(pc, PortraitName + "L", PortraitName + "S")
		#GemRB.GetView("SUB_WIN", 0).Close()
		#AppearanceWindow.Close()
	
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
	if not IsGUIRECCommon and AppearanceWindow:
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

	Window = CustomWindow

	Portrait = PortraitList1.QueryText()

	# small hack
	if GemRB.GetVar("Row1") == RowCount1:
		return

	Label = Window.GetControl(0x10000007)
	Label.SetText(Portrait)

	ButtonNumber = 6
	if IsGUIRECCommon:
		ButtonNumber = 10
	Button = Window.GetControl(ButtonNumber)

	if Portrait == "":
		Portrait = EmptyPortrait["medium"]
		if IsGUIRECCommon:
			Button.SetState(IE_GUI_BUTTON_DISABLED)
		elif GameCheck.IsBG2OrEE() or GameCheck.IsBG2Demo():
			Button.SetDisabled(True)
		else:
			Button.SetState(IE_GUI_BUTTON_DISABLED)
	else:
		if PortraitList2.QueryText() != "":
			if IsGUIRECCommon:
				Button.SetState(IE_GUI_BUTTON_ENABLED)
			elif GameCheck.IsBG2OrEE() or GameCheck.IsBG2Demo():
				Button.SetDisabled(False)
			else:
				Button.SetState(IE_GUI_BUTTON_ENABLED)

	Preview = Window.GetControl(0)
	Preview.SetPicture(Portrait, EmptyPortrait["medium"])

# This is for the small custom portrait
def PortraitCommonSmallCustom():
	global PortraitList1, PortraitList2
	global RowCount2
	global CustomWindow

	Window = CustomWindow

	Portrait = PortraitList2.QueryText()

	# small hack
	if GemRB.GetVar("Row2") == RowCount2:
		return

	Label = Window.GetControl(0x10000008)
	Label.SetText(Portrait)

	ButtonNumber = 6
	if IsGUIRECCommon:
		ButtonNumber = 10
	Button = Window.GetControl(ButtonNumber)

	if Portrait == "":
		Portrait = EmptyPortrait["small"]
		if IsGUIRECCommon:
			Button.SetState(IE_GUI_BUTTON_DISABLED)
		elif GameCheck.IsBG2OrEE() or GameCheck.IsBG2Demo():
			Button.SetDisabled(True)
		else:
			Button.SetState(IE_GUI_BUTTON_DISABLED)
	else:
		if PortraitList1.QueryText() != "":
			if IsGUIRECCommon:
				Button.SetState(IE_GUI_BUTTON_ENABLED)
			elif GameCheck.IsBG2OrEE() or GameCheck.IsBG2Demo():
				Button.SetDisabled(False)
			else:
				Button.SetState(IE_GUI_BUTTON_ENABLED)

	Preview = Window.GetControl(1)
	Preview.SetPicture(Portrait, EmptyPortrait["small"])


def PortraitCustomPress():
	global PortraitsTable, LastPortrait
	global PortraitList1, PortraitList2
	global RowCount1, RowCount2
	global CustomWindow

	if IsGUIRECCommon:
		CustomWindow = Window = GemRB.LoadWindow(19, "GUIREC")
	else:
		CustomWindow = Window = GemRB.LoadWindow(18, "GUICG")

	ButtonDoneNumber = 6
	if IsGUIRECCommon:
		ButtonDoneNumber = 10
	ButtonDone = Window.GetControl(ButtonDoneNumber)
	ButtonDone.SetText(11973)
	ButtonDone.MakeDefault()

	ButtonDone.OnPress(PortraitButtonCustomDone)
	if IsGUIRECCommon:
		ButtonDone.SetState(IE_GUI_BUTTON_DISABLED)
	elif GameCheck.IsBG2OrEE() or GameCheck.IsBG2Demo():
		ButtonDone.SetDisabled(True)
	else:
		ButtonDone.SetState(IE_GUI_BUTTON_DISABLED)
	
	ButtonCancelNumber = 6
	if IsGUIRECCommon:
		ButtonCancelNumber = 11
	ButtonCancel = Window.GetControl(ButtonCancelNumber)
	if IsGUIRECCommon:
		ButtonCancel.SetText(13727)
		ButtonCancel.MakeEscape()
		ButtonCancel.SetState(IE_GUI_BUTTON_ENABLED)
	else:
		ButtonCancel.SetText(15416)
	if GameCheck.IsIWD2():
		ButtonCancel.MakeEscape()
	ButtonCancel.OnPress(PortraitCustomAbort)

	if IsGUIRECCommon:
		if not GameCheck.IsIWD1():
			SmallPortraitButton = Window.GetControl(1)
			SmallPortraitButton.SetFlags(IE_GUI_BUTTON_PICTURE | IE_GUI_BUTTON_NO_IMAGE, OP_SET)
			LargePortraitButton = Window.GetControl(0)
			LargePortraitButton.SetFlags(IE_GUI_BUTTON_PICTURE | IE_GUI_BUTTON_NO_IMAGE, OP_SET)
	else:
		ButtonPortraitLarge = Window.GetControl(0)
		PortraitName = PortraitsTable.GetRowName(LastPortrait) + PortraitSuffix["large"]
		ButtonPortraitLarge.SetPicture(PortraitName, EmptyPortrait["medium"])
		ButtonPortraitLarge.SetState(IE_GUI_BUTTON_LOCKED)

		ButtonPortraitSmall = Window.GetControl(1)
		PortraitName = PortraitsTable.GetRowName(LastPortrait) + PortraitSuffix["small"]
		ButtonPortraitSmall.SetPicture(PortraitName, EmptyPortrait["small"])
		ButtonPortraitSmall.SetState(IE_GUI_BUTTON_LOCKED)

	ListMode1 = 1
	if GameCheck.IsIWD2():
		ListMode1 = 2
	PortraitList1 = Window.GetControl(2)
	RowCount1 = len(PortraitList1.ListResources(CHR_PORTRAITS, ListMode1))
	PortraitList1.OnSelect(PortraitCommonLargeCustom)
	PortraitList1.SetVarAssoc("Row1", RowCount1)

	PortraitList2Number = 4
	if IsGUIRECCommon:
		PortraitList2Number = 3
	PortraitList2 = Window.GetControl(PortraitList2Number)
	RowCount2 = len(PortraitList2.ListResources(CHR_PORTRAITS, 0))
	PortraitList2.OnSelect(PortraitCommonSmallCustom)
	PortraitList2.SetVarAssoc("Row2", RowCount2)
	
	ModalShadow = MODAL_SHADOW_NONE
	if IsGUIRECCommon or GameCheck.IsBG1OrEE():
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
