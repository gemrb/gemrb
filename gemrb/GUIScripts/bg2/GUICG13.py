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
import CommonTables
from GUIDefines import *
from ie_stats import *

global IE_ANIM_ID
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
MajorColor = 0
MinorColor = 0
PDollButton = 0
MyChar = 0
IE_ANIM_ID = 206

def RefreshPDoll():
	AnimID = 0x6000
	table = GemRB.LoadTable("avprefr")
	Race = GemRB.GetPlayerStat (MyChar, IE_RACE)
	AnimID = AnimID+table.GetValue(Race, 0)
	table = GemRB.LoadTable("avprefc")
	Class = GemRB.GetPlayerStat (MyChar, IE_CLASS)
	AnimID = AnimID+table.GetValue(Class, 0)
	table = GemRB.LoadTable("avprefg")
	Gender = GemRB.GetPlayerStat (MyChar, IE_SEX)
	AnimID = AnimID+table.GetValue(Gender,0)
	ResRef = CommonTables.Pdolls.GetValue(hex(AnimID), "LEVEL1")

	PDollButton.SetPLT(ResRef, 0, MinorColor, MajorColor, SkinColor, 0, 0, HairColor, 0)
	return

def OnLoad():
	global ColorWindow, DoneButton, PDollButton, ColorTable
	global HairButton, SkinButton, MajorButton, MinorButton
	global HairColor, SkinColor, MajorColor, MinorColor
	global MyChar
	
	GemRB.LoadWindowPack("GUICG", 640, 480)
	ColorWindow=GemRB.LoadWindow(13)

	MyChar = GemRB.GetVar ("Slot")
	ColorTable = GemRB.LoadTable("clowncol")
	#set these colors to some default
	PortraitTable = GemRB.LoadTable("pictures")
	PortraitName = GemRB.GetToken("LargePortrait")
	PortraitName = PortraitName[0:len(PortraitName)-1]
	PortraitIndex = PortraitTable.GetRowIndex(PortraitName)
	if PortraitIndex<0:
		HairColor=PortraitTable.GetValue(0,1)
		SkinColor=PortraitTable.GetValue(0,2)
		MinorColor=PortraitTable.GetValue(0,3)
		MajorColor=PortraitTable.GetValue(0,4)
	else:
		HairColor=PortraitTable.GetValue(PortraitIndex,1)
		SkinColor=PortraitTable.GetValue(PortraitIndex,2)
		MinorColor=PortraitTable.GetValue(PortraitIndex,3)
		MajorColor=PortraitTable.GetValue(PortraitIndex,4)

	PDollButton = ColorWindow.GetControl(1)
	PDollButton.SetState (IE_GUI_BUTTON_LOCKED)
	PDollButton.SetFlags(IE_GUI_BUTTON_PICTURE,OP_OR)

	HairButton = ColorWindow.GetControl(2)
	HairButton.SetFlags(IE_GUI_BUTTON_PICTURE,OP_OR)
	HairButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, HairPress)
	HairButton.SetBAM("COLGRAD", 1, 0, HairColor)

	SkinButton = ColorWindow.GetControl(3)
	SkinButton.SetFlags(IE_GUI_BUTTON_PICTURE,OP_OR)
	SkinButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, SkinPress)
	SkinButton.SetBAM("COLGRAD", 1, 0, SkinColor)

	MajorButton = ColorWindow.GetControl(5)
	MajorButton.SetFlags(IE_GUI_BUTTON_PICTURE,OP_OR)
	MajorButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, MajorPress)
	MajorButton.SetBAM("COLGRAD", 1, 0, MinorColor)

	MinorButton = ColorWindow.GetControl(4)
	MinorButton.SetFlags(IE_GUI_BUTTON_PICTURE,OP_OR)
	MinorButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, MinorPress)
	MinorButton.SetBAM("COLGRAD", 1, 0, MajorColor)

	BackButton = ColorWindow.GetControl(13)
	BackButton.SetText(15416)
	BackButton.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)
	DoneButton = ColorWindow.GetControl(0)
	DoneButton.SetText(11973)
	DoneButton.SetFlags(IE_GUI_BUTTON_DEFAULT,OP_OR)

	DoneButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, NextPress)
	BackButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, BackPress)
	RefreshPDoll()
	ColorWindow.SetVisible(WINDOW_VISIBLE)
	return

def DonePress():
	global HairColor, SkinColor, MajorColor, MinorColor

	if ColorPicker:
		ColorPicker.Unload()
	ColorWindow.SetVisible(WINDOW_VISIBLE)
	PickedColor=ColorTable.GetValue(ColorIndex, GemRB.GetVar("Selected"))
	if ColorIndex==0:
		HairColor=PickedColor
		HairButton.SetBAM("COLGRAD", 1, 0, HairColor)
		RefreshPDoll()
		return
	if ColorIndex==1:
		SkinColor=PickedColor
		SkinButton.SetBAM("COLGRAD", 1, 0, SkinColor)
		RefreshPDoll()
		return
	if ColorIndex==2:
		MinorColor=PickedColor
		MajorButton.SetBAM("COLGRAD", 1, 0, MinorColor)
		RefreshPDoll()
		return

	MajorColor=PickedColor
	MinorButton.SetBAM("COLGRAD", 1, 0, MajorColor)
	RefreshPDoll()
	return

def GetColor():
	global ColorPicker

	ColorPicker=GemRB.LoadWindow(14)
	GemRB.SetVar("Selected",-1)
	for i in range(34):
		Button = ColorPicker.GetControl(i)
		Button.SetState(IE_GUI_BUTTON_DISABLED)
		Button.SetFlags(IE_GUI_BUTTON_PICTURE|IE_GUI_BUTTON_RADIOBUTTON,OP_OR)

	Selected = -1
	for i in range(34):
		MyColor = ColorTable.GetValue(ColorIndex, i)
		if MyColor == "*":
			break
		Button = ColorPicker.GetControl(i)
		Button.SetBAM("COLGRAD", 2, 0, MyColor)
		if PickedColor == MyColor:
			GemRB.SetVar("Selected",i)
			Selected = i
		Button.SetState(IE_GUI_BUTTON_ENABLED)
		Button.SetVarAssoc("Selected",i)
		Button.SetEvent(IE_GUI_BUTTON_ON_PRESS, DonePress)
	
	ColorPicker.SetVisible(WINDOW_VISIBLE)
	return

def HairPress():
	global ColorIndex, PickedColor

	ColorWindow.SetVisible(WINDOW_INVISIBLE)
	ColorIndex = 0
	PickedColor = HairColor
	GetColor()
	return

def SkinPress():
	global ColorIndex, PickedColor

	ColorWindow.SetVisible(WINDOW_INVISIBLE)
	ColorIndex = 1
	PickedColor = SkinColor
	GetColor()
	return

def MajorPress():
	global ColorIndex, PickedColor

	ColorWindow.SetVisible(WINDOW_INVISIBLE)
	ColorIndex = 2
	PickedColor = MinorColor
	GetColor()
	return

def MinorPress():
	global ColorIndex, PickedColor

	ColorWindow.SetVisible(WINDOW_INVISIBLE)
	ColorIndex = 3
	PickedColor = MajorColor
	GetColor()
	return

def BackPress():
	if ColorWindow:
		ColorWindow.Unload()
	GemRB.SetNextScript("CharGen7")
	return

def NextPress():
	if ColorWindow:
		ColorWindow.Unload()
	GemRB.SetVar("HairColor",HairColor)
	GemRB.SetVar("SkinColor",SkinColor)
	GemRB.SetVar("MinorColor",MinorColor)
	GemRB.SetVar("MajorColor",MajorColor)
	GemRB.SetNextScript("GUICG19") #sounds
	return
