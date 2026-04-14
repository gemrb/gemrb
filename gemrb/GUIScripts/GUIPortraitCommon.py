# SPDX-FileCopyrightText: 2011 Contributors to the GemRB project <https://gemrb.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

# code shared between the common GUICG12, CGPortrait
import GemRB
import GameCheck
from GUIDefines import *
from ie_restype import RES_BMP

CustomWindow = 0
AppearanceWindow = 0
PortraitButton = 0
PortraitsTable = 0
LastPortrait = 0
Gender = 0

def SetupAppearanceWindow(GetGenderFunc, BackHandler = None, ExtraSetupFunc = None, CustomDoneFunc = None, NextPressFunc = None):
	global AppearanceWindow, PortraitButton, PortraitsTable, LastPortrait
	global Gender

	# Window
	AppearanceWindow = GemRB.LoadWindow(11, "GUICG")
	
	# Load the Gender
	Gender = GetGenderFunc()
	
	# Optional extra setup
	if ExtraSetupFunc:
		ExtraSetupFunc(AppearanceWindow)

	# Load the Portraits Table
	PortraitsTable = GemRB.LoadTable("PICTURES")
	if not GameCheck.IsIWD1():
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
		LastPortrait = 0

	# Controls
	PortraitButton = AppearanceWindow.GetControl(1)
	PortraitButton.SetFlags(IE_GUI_BUTTON_PICTURE | IE_GUI_BUTTON_NO_IMAGE, OP_SET)
	PortraitButton.SetState(IE_GUI_BUTTON_LOCKED)

	LeftButton = AppearanceWindow.GetControl(2)
	if GameCheck.IsIWD1():
		LeftButton.SetState(IE_GUI_BUTTON_ENABLED)
		LeftButton.SetFlags(IE_GUI_BUTTON_RADIOBUTTON, OP_OR)
		
	RightButton = AppearanceWindow.GetControl(3)
	if GameCheck.IsIWD1():
		RightButton.SetState(IE_GUI_BUTTON_ENABLED)
		RightButton.SetFlags(IE_GUI_BUTTON_RADIOBUTTON, OP_OR)
		
	BackButton = AppearanceWindow.GetControl(5)
	if GameCheck.IsIWD1():
		BackButton.SetText(13727)
	else:
		BackButton.SetText(15416)
	
	if GameCheck.IsIWD2() or GameCheck.IsIWD1():
		BackButton.MakeEscape()
		
	CustomButton = AppearanceWindow.GetControl(6)
	CustomButton.SetText(17545)
	if GameCheck.IsIWD1():
		CustomButton.SetState(IE_GUI_BUTTON_ENABLED)

	DoneButton = AppearanceWindow.GetControl(0)
	if GameCheck.IsIWD2():
		DoneButton.SetText(36789)
	else:
		DoneButton.SetText(11973)
		if GameCheck.IsIWD1():
			DoneButton.SetState(IE_GUI_BUTTON_ENABLED)
	DoneButton.MakeDefault()

	# Events
	RightButton.OnPress(PortraitButtonRightPress)
	LeftButton.OnPress(PortraitButtonLeftPress)
	if BackHandler:
		BackButton.OnPress(BackHandler)
	else:
		BackButton.OnPress(lambda: PortraitBackPress(AppearanceWindow))
	CustomButton.OnPress(lambda: PortraitButtonCustomPress(CustomDoneFunc))
	DoneButton.OnPress(lambda: PortraitButtonNextPress(NextPressFunc))
	
	if GameCheck.IsIWD2():
		PortraitSetPicture(PortraitButton, PortraitsTable, LastPortrait)
	else:
		if GameCheck.IsIWD1():
			while PortraitsTable.GetValue(LastPortrait, 0) != Gender:
				LastPortrait = LastPortrait + 1
			PortraitSetPicture(PortraitButton, PortraitsTable, LastPortrait)
		else:
			# Portrait selection loop
			flag = False
			while True:
				if PortraitsTable.GetValue(LastPortrait, 0) == Gender:
					PortraitSetPicture(PortraitButton, PortraitsTable, LastPortrait)
					break
				LastPortrait += 1
				if LastPortrait >= PortraitsTable.GetRowCount():
					LastPortrait = 0
					if flag:
						PortraitSetPicture(PortraitButton, PortraitsTable, LastPortrait)
						break
					flag = True

	if GameCheck.IsIWD2() or GameCheck.IsBG2OrEE() or GameCheck.IsBG2Demo():
		AppearanceWindow.Focus()
	if GameCheck.IsBG1OrEE():
		AppearanceWindow.ShowModal(MODAL_SHADOW_GRAY)
	if GameCheck.IsIWD1():
		AppearanceWindow.ShowModal(MODAL_SHADOW_NONE)

# Button right press function	
def PortraitButtonRightPress():
	global LastPortrait
	LastPortrait = PortraitNext(PortraitsTable, LastPortrait, Gender)
	PortraitSetPicture(PortraitButton, PortraitsTable, LastPortrait)

# Button left press function	
def PortraitButtonLeftPress():
	global LastPortrait
	LastPortrait = PortraitPrev(PortraitsTable, LastPortrait, Gender)
	PortraitSetPicture(PortraitButton, PortraitsTable, LastPortrait)

# Button custom done, must pass function because of BG1
def PortraitButtonCustomDone(next_func = None):
	global AppearanceWindow
	PortraitPictureButtonName = PortraitCustomDone(AppearanceWindow)
	if next_func:
		if GameCheck.IsIWD1():
			next_func(PortraitPictureButtonName)
		else:
			next_func()

# Button abort function
def PortraitButtonCustomAbort():
	PortraitCustomAbort(AppearanceWindow)

# Button custom press, must pass function because of BG1
def PortraitButtonCustomPress(custom_done_func = None):
	PortraitCustomPress(PortraitsTable, LastPortrait, lambda: PortraitButtonCustomDone(custom_done_func), PortraitButtonCustomAbort)

# Button next press, must pass function because of BG1
def PortraitButtonNextPress(next_func = None):
	PortraitPictureButtonName = PortraitApplySelection(AppearanceWindow, LastPortrait)
	if next_func:
		if GameCheck.IsIWD1():
			next_func(PortraitPictureButtonName)
		else:
			next_func()

	
# This if for moving to next portrait
def PortraitNext(PortraitsTable, LastPortrait, gender):
	while True:
		LastPortrait += 1
		if LastPortrait >= PortraitsTable.GetRowCount():
			LastPortrait = 0

		if PortraitsTable.GetValue(LastPortrait, 0) == gender:
			return LastPortrait

# This is for moving to previous portrait
def PortraitPrev(PortraitsTable, LastPortrait, gender):
	while True:
		LastPortrait -= 1
		if LastPortrait < 0:
			LastPortrait = PortraitsTable.GetRowCount() - 1

		if PortraitsTable.GetValue(LastPortrait, 0) == gender:
			return LastPortrait

# This is for when is done
def PortraitCustomDone(appearance_window):

	global PortraitList1, PortraitList2
	global RowCount1
	global CustomWindow

	portrait_large = PortraitList1.QueryText()
	GemRB.SetToken("LargePortrait", portrait_large)

	portrait_small = PortraitList2.QueryText()
	GemRB.SetToken("SmallPortrait", portrait_small)

	if CustomWindow:
		CustomWindow.Close()

	if appearance_window:
		appearance_window.Close()
	if GameCheck.IsIWD2() or GameCheck.IsBG2OrEE() or GameCheck.IsBG2Demo():
		GemRB.SetNextScript ("CharGen2")
	return portrait_large

# This is for abort
def PortraitCustomAbort(AppearanceWindow=None):
	if CustomWindow:
		CustomWindow.Close()
	if AppearanceWindow:
		if GameCheck.IsBG1OrEE():
			AppearanceWindow.ShowModal (MODAL_SHADOW_GRAY) # narrower than CustomWindow, so borders will remain
		elif GameCheck.IsBG2OrEE() or GameCheck.IsBG2Demo():
			AppearanceWindow.ShowModal (MODAL_SHADOW_NONE) # narrower than CustomWindow, so borders will remain

# This is for applying
def PortraitApplySelection(AppearanceWindow, LastPortrait):
	if AppearanceWindow:
		AppearanceWindow.Close ()
	PortraitTable = GemRB.LoadTable("pictures")
	PortraitName = PortraitTable.GetRowName(LastPortrait)
	large_suffix = "L"
	if GameCheck.IsBG2OrEE() or GameCheck.IsBG2Demo():
		large_suffix = "M"
	GemRB.SetToken("SmallPortrait", PortraitName + "S")
	GemRB.SetToken("LargePortrait", PortraitName + large_suffix)
	if GameCheck.IsIWD2() or GameCheck.IsBG2OrEE() or GameCheck.IsBG2Demo():
		GemRB.SetNextScript ("CharGen2") #Before race
	return PortraitName + large_suffix

# This is for the large custom portrait
def PortraitCommonLargeCustom():
	global PortraitList1, PortraitList2
	global RowCount1
	global CustomWindow

	if GameCheck.IsIWD2() or GameCheck.IsBG2OrEE() or GameCheck.IsBG2Demo() or GameCheck.IsIWD1():
		empty_portrait = "NOPORTMD"
	else:
		empty_portrait = "NOPORTLG"

	window = CustomWindow

	portrait = PortraitList1.QueryText()

	# small hack
	if GemRB.GetVar("Row1") == RowCount1:
		return

	label = window.GetControl(0x10000007)
	label.SetText(portrait)

	button = window.GetControl(6)

	if portrait == "":
		portrait = empty_portrait
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
	preview.SetPicture(portrait, empty_portrait)

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
		portrait = "NOPORTSM"

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
	preview.SetPicture(portrait, "NOPORTSM")


def PortraitCustomPress(
	portraits_table,
	last_portrait,
	custom_done,
	custom_abort
):
	global PortraitList1, PortraitList2
	global RowCount1, RowCount2
	global CustomWindow

	CustomWindow = Window = GemRB.LoadWindow(18, "GUICG")

	list_mode1 = 1
	if GameCheck.IsIWD2():
		list_mode1 = 2
	PortraitList1 = Window.GetControl(2)
	RowCount1 = len(PortraitList1.ListResources(CHR_PORTRAITS, list_mode1))
	PortraitList1.OnSelect(PortraitCommonLargeCustom)
	PortraitList1.SetVarAssoc("Row1", RowCount1)

	PortraitList2 = Window.GetControl(4)
	RowCount2 = len(PortraitList2.ListResources(CHR_PORTRAITS, 0))
	PortraitList2.OnSelect(PortraitCommonSmallCustom)
	PortraitList2.SetVarAssoc("Row2", RowCount2)

	Button = Window.GetControl(6)
	Button.SetText(11973)
	if GameCheck.IsIWD2():
		Button.MakeDefault()
	Button.OnPress(custom_done)
	if GameCheck.IsBG2OrEE() or GameCheck.IsBG2Demo():
		Button.SetDisabled(True)
	else:
		Button.SetState(IE_GUI_BUTTON_DISABLED)

	Button = Window.GetControl(7)
	Button.SetText(15416)
	if GameCheck.IsIWD2():
		Button.MakeEscape()
	Button.OnPress(custom_abort)

	large_suffix = "L"
	if GameCheck.IsBG2OrEE() or GameCheck.IsBG2Demo():
		large_suffix = "M"
	fallback_resource = "NOPORTMD"
	if GameCheck.IsBG1OrEE():
		fallback_resource = "NOPORTLG"
	Button = Window.GetControl(0)
	portrait_name = portraits_table.GetRowName(last_portrait) + large_suffix

	if GameCheck.IsBG2OrEE() or GameCheck.IsBG2Demo():
		if GemRB.HasResource(portrait_name, RES_BMP, 1) or GemRB.HasResource(fallback_resource, RES_BMP, 1):
			Button.SetPicture(portrait_name, fallback_resource)
	else:
		Button.SetPicture(portrait_name, fallback_resource)

	Button.SetState(IE_GUI_BUTTON_LOCKED)

	Button = Window.GetControl(1)
	portrait_name = portraits_table.GetRowName(last_portrait) + "S"
	Button.SetPicture(portrait_name, "NOPORTSM")
	Button.SetState(IE_GUI_BUTTON_LOCKED)

	modal_shadow = MODAL_SHADOW_NONE
	if GameCheck.IsBG1OrEE():
		modal_shadow = MODAL_SHADOW_GRAY
	Window.ShowModal(modal_shadow)

def PortraitBackPress(AppearanceWindow):
	if AppearanceWindow:
		AppearanceWindow.Close ()
	if GameCheck.IsIWD1():
		return
	next_script = "GUICG1"
	if GameCheck.IsIWD2():
		next_script = "CharGen"
	GemRB.SetNextScript (next_script)
	GemRB.SetVar ("Gender",0) #scrapping the gender value
	return

def PortraitSetPicture(PortraitButton, PortraitsTable, LastPortrait):
	picture_size = "L"
	if GameCheck.IsBG1OrEE() or GameCheck.IsIWD1():
		picture_size = "G"
	PortraitName = PortraitsTable.GetRowName (LastPortrait)+picture_size
	if GameCheck.IsBG2OrEE() or GameCheck.IsBG2Demo():
		PortraitButton.SetPicture (PortraitName, "NOPORTLG")
	else:
		PortraitButton.SetPicture (PortraitName)
	return
