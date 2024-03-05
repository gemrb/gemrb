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
#character generation, color (GUICG13)
import GemRB
import BGCommon
import CharGenCommon
import GameCheck
import GUICommon
from GUIDefines import *
from ie_stats import *

ColorTable = 0
ColorWindow = 0
ColorPicker = 0
DoneButton = 0
ColorIndex = 0
PickedColor = 0
HairButton = 0
SkinButton = 0
MajorButton = 0
MinorButton = 0
HairColor = 0
SkinColor = 0
MinorColor = 0
MajorColor = 0
PDollButton = 0
ModalShadow = MODAL_SHADOW_NONE

def OnLoad():
	global ColorWindow, DoneButton, PDollButton, ColorTable, ModalShadow
	global HairButton, SkinButton, MajorButton, MinorButton
	global HairColor, SkinColor, MinorColor, MajorColor
	
	ColorWindow=GemRB.LoadWindow(13, "GUICG")
	ColorWindow.SetFlags (WF_ALPHA_CHANNEL, OP_OR)
	if GameCheck.IsBG2 ():
		CharGenCommon.PositionCharGenWin (ColorWindow, -6)

	ColorTable = GemRB.LoadTable("clowncol")
	#set these colors to some default
	PortraitTable = GemRB.LoadTable("pictures")
	PortraitName = GemRB.GetToken("LargePortrait")
	PortraitName = PortraitName[0:len(PortraitName)-1]
	PortraitIndex = PortraitTable.GetRowIndex(PortraitName)
	if PortraitIndex == None:
		PortraitIndex = 0
	HairColor = PortraitTable.GetValue (PortraitIndex, 1)
	SkinColor = PortraitTable.GetValue (PortraitIndex, 2)
	MinorColor = PortraitTable.GetValue (PortraitIndex, 3)
	MajorColor = PortraitTable.GetValue (PortraitIndex, 4)

	PDollButton = ColorWindow.GetControl(1)
	PDollButton.SetState (IE_GUI_BUTTON_LOCKED)
	PDollButton.SetFlags(IE_GUI_BUTTON_PICTURE,OP_OR)

	HairButton = ColorWindow.GetControl(2)
	HairButton.SetFlags(IE_GUI_BUTTON_PICTURE,OP_OR)
	HairButton.OnPress (HairPress)
	HairButton.SetBAM ("COLGRAD", GameCheck.IsBG2(), 0, HairColor)

	SkinButton = ColorWindow.GetControl(3)
	SkinButton.SetFlags(IE_GUI_BUTTON_PICTURE,OP_OR)
	SkinButton.OnPress (SkinPress)
	SkinButton.SetBAM ("COLGRAD", GameCheck.IsBG2(), 0, SkinColor)

	MajorButton = ColorWindow.GetControl(5)
	MajorButton.SetFlags(IE_GUI_BUTTON_PICTURE,OP_OR)
	MajorButton.OnPress (MajorPress)
	MajorButton.SetBAM ("COLGRAD", GameCheck.IsBG2(), 0, MinorColor)

	MinorButton = ColorWindow.GetControl(4)
	MinorButton.SetFlags(IE_GUI_BUTTON_PICTURE,OP_OR)
	MinorButton.OnPress (MinorPress)
	MinorButton.SetBAM ("COLGRAD", GameCheck.IsBG2(), 0, MajorColor)

	BackButton = ColorWindow.GetControl(13)
	BackButton.SetText(15416)
	BackButton.MakeEscape()
	DoneButton = ColorWindow.GetControl(0)
	DoneButton.SetText(11973)
	DoneButton.MakeDefault()

	DoneButton.OnPress (NextPress)
	BackButton.OnPress (BackPress)
	BGCommon.RefreshPDoll (PDollButton, MinorColor, MajorColor, SkinColor, HairColor)

	if GameCheck.IsBG1 ():
		ModalShadow = MODAL_SHADOW_GRAY

	ColorWindow.ShowModal (ModalShadow)

	return

def DonePress():
	global HairColor, SkinColor, MinorColor, MajorColor

	if ColorPicker:
		ColorPicker.Close ()

	ColorWindow.ShowModal (ModalShadow)

	PickedColor = ColorTable.GetValue(ColorIndex, GemRB.GetVar("Selected"))
	if ColorIndex == 0:
		HairColor = PickedColor
		HairButton.SetBAM ("COLGRAD", GameCheck.IsBG2(), 0, HairColor)
		BGCommon.RefreshPDoll (PDollButton, MinorColor, MajorColor, SkinColor, HairColor)
		return
	if ColorIndex == 1:
		SkinColor = PickedColor
		SkinButton.SetBAM ("COLGRAD", GameCheck.IsBG2(), 0, SkinColor)
		BGCommon.RefreshPDoll (PDollButton, MinorColor, MajorColor, SkinColor, HairColor)
		return
	if ColorIndex == 2:
		MinorColor = PickedColor
		MajorButton.SetBAM ("COLGRAD", GameCheck.IsBG2(), 0, MinorColor)
		BGCommon.RefreshPDoll (PDollButton, MinorColor, MajorColor, SkinColor, HairColor)
		return

	MajorColor = PickedColor
	MinorButton.SetBAM ("COLGRAD", GameCheck.IsBG2(), 0, MajorColor)
	BGCommon.RefreshPDoll (PDollButton, MinorColor, MajorColor, SkinColor, HairColor)
	return

def GetColor():
	global ColorPicker

	ColorPicker = GemRB.LoadWindow(14)
	GemRB.SetVar("Selected",-1)
	btnFlags = IE_GUI_BUTTON_PICTURE
	btnState = IE_GUI_BUTTON_LOCKED
	if GameCheck.IsBG2 ():
		btnFlags |= IE_GUI_BUTTON_RADIOBUTTON
		btnState = IE_GUI_BUTTON_DISABLED
	for i in range(34):
		Button = ColorPicker.GetControl(i)
		Button.SetState (btnState)
		Button.SetFlags (btnFlags, OP_OR)

	for i in range(34):
		MyColor = ColorTable.GetValue(ColorIndex, i)
		if MyColor == "*":
			break
		Button = ColorPicker.GetControl(i)
		Button.SetBAM("COLGRAD", 2, 0, MyColor)
		if PickedColor == MyColor:
			GemRB.SetVar("Selected",i)
		Button.SetState(IE_GUI_BUTTON_ENABLED)
		Button.SetVarAssoc("Selected",i)
		Button.OnPress (DonePress)

	ColorPicker.GetControl(0).MakeEscape ()
	ColorPicker.ShowModal (ModalShadow)
	return

def HairPress():
	global ColorIndex, PickedColor

	ColorIndex = 0
	PickedColor = HairColor
	GetColor()
	return

def SkinPress():
	global ColorIndex, PickedColor

	ColorIndex = 1
	PickedColor = SkinColor
	GetColor()
	return

def MajorPress():
	global ColorIndex, PickedColor

	ColorIndex = 2
	PickedColor = MinorColor
	GetColor()
	return

def MinorPress():
	global ColorIndex, PickedColor

	ColorIndex = 3
	PickedColor = MajorColor
	GetColor()
	return

def BackPress():
	if GameCheck.IsBG1 ():
		return CharGenCommon.back(ColorWindow)

	if ColorWindow:
		ColorWindow.Close ()
	GemRB.SetNextScript("CharGen7")
	return

def NextPress():
	if ColorWindow:
		ColorWindow.Close()

	MyChar = GemRB.GetVar ("Slot")
	GUICommon.SetColorStat (MyChar, IE_HAIR_COLOR, HairColor)
	GUICommon.SetColorStat (MyChar, IE_SKIN_COLOR, SkinColor)
	GUICommon.SetColorStat (MyChar, IE_MAJOR_COLOR, MajorColor)
	GUICommon.SetColorStat (MyChar, IE_MINOR_COLOR, MinorColor)
	# ignore IE_METAL_COLOR, IE_LEATHER_COLOR, IE_ARMOR_COLOR as they're set by equipment
	if GameCheck.IsBG1 ():
		CharGenCommon.next()
	else:
		GemRB.SetNextScript ("GUICG19") #sounds
	return
