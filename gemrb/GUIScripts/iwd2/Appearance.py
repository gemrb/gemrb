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

import CharOverview
import CommonTables
import GUICommon
import IDLUCommon
import Portrait
from GUIDefines import *
from ie_stats import IE_SEX, IE_CLASS

ColorTable = 0
HairTable = 0
SkinTable = 0
ColorWindow = 0
ColorPicker = 0
DoneButton = 0
ColorIndex = 0
PickedColor = 0
HairButton = 0
SkinButton = 0
MajorButton = 0
MinorButton = 0
Color1 = 0
Color2 = 0
Color3 = 0
Color4 = 0
PDollButton = 0
PDollResRef = 0

def RefreshPDoll():
	global ColorWindow, PDollButton
	global Color1, Color2, Color3, Color4, PDollResRef

	PDollButton.SetFlags(IE_GUI_BUTTON_PLAYALWAYS|IE_GUI_BUTTON_CENTER_PICTURES, OP_OR)
	PDollButton.SetBAM("", 0, 0, 0) # just hide or there is a tiny artifact
	PDollButton.SetAnimation (None) # force reset
	PDollButton.SetAnimation (PDollResRef, 1, 1, [0, Color4, Color3, Color2, 0, 0, Color1, 0])
	return

def OnLoad():
	global ColorWindow, DoneButton, PDollButton
	global HairTable, SkinTable, ColorTable
	global HairButton, SkinButton, MajorButton, MinorButton
	global Color1, Color2, Color3, Color4, PDollResRef
	
	ColorWindow=GemRB.LoadWindow(13, "GUICG")
	CharOverview.PositionCharGenWin(ColorWindow)

	pc = GemRB.GetVar ("Slot")
	Race = IDLUCommon.GetRace (pc)
	RaceName = CommonTables.Races.GetRowName (Race)
	HairTable = GemRB.LoadTable(CommonTables.Races.GetValue(RaceName, "HAIR"))
	SkinTable = GemRB.LoadTable(CommonTables.Races.GetValue(RaceName, "SKIN"))
	ColorTable = GemRB.LoadTable("clowncol")

	#set these colors to some default
	Gender = GemRB.GetPlayerStat (pc, IE_SEX)
	Portrait.Init (Gender)
	Portrait.Set (GemRB.GetPlayerPortrait (pc)["ResRef"])
	PortraitName = Portrait.Name () # strips the last char like the table needs

	PortraitTable = GemRB.LoadTable("pictures")
	Color1 = PortraitTable.GetValue(PortraitName, "HAIR", GTV_INT)
	Color2 = PortraitTable.GetValue(PortraitName, "SKIN", GTV_INT)
	Color3 = PortraitTable.GetValue(PortraitName, "MAJOR", GTV_INT)
	Color4 = PortraitTable.GetValue(PortraitName, "MINOR", GTV_INT)
	PDollButton = ColorWindow.GetControl(1)
	PDollButton.SetFlags(IE_GUI_BUTTON_PICTURE,OP_OR)
	PDollButton.SetState(IE_GUI_BUTTON_LOCKED)

	HairButton = ColorWindow.GetControl(2)
	HairButton.SetFlags(IE_GUI_BUTTON_PICTURE,OP_OR)
	HairButton.OnPress (HairPress)
	HairButton.SetBAM("COLGRAD", 1, 0, Color1)

	SkinButton = ColorWindow.GetControl(3)
	SkinButton.SetFlags(IE_GUI_BUTTON_PICTURE,OP_OR)
	SkinButton.OnPress (SkinPress)
	SkinButton.SetBAM("COLGRAD", 1, 0, Color2)

	MajorButton = ColorWindow.GetControl(5)
	MajorButton.SetFlags(IE_GUI_BUTTON_PICTURE,OP_OR)
	MajorButton.OnPress (MajorPress)
	MajorButton.SetBAM("COLGRAD", 1, 0, Color3)

	MinorButton = ColorWindow.GetControl(4)
	MinorButton.SetFlags(IE_GUI_BUTTON_PICTURE,OP_OR)
	MinorButton.OnPress (MinorPress)
	MinorButton.SetBAM("COLGRAD", 1, 0, Color4)

	BackButton = ColorWindow.GetControl(13)
	BackButton.SetText(15416)
	DoneButton = ColorWindow.GetControl(0)
	DoneButton.SetText(11973)
	DoneButton.MakeDefault()

	DoneButton.OnPress (NextPress)
	BackButton.OnPress (BackPress)

	# calculate the paperdoll animation id from the race, class and gender
	PDollTable = GemRB.LoadTable ("avatars")
	table = GemRB.LoadTable ("avprefr")
	RaceID = CommonTables.Races.GetValue (RaceName, "ID", GTV_INT)
	# look up base race if needed
	if RaceID > 1000:
		RaceID = RaceID >> 16
		Race = CommonTables.Races.FindValue ("ID", RaceID)
		RaceName = CommonTables.Races.GetRowName (Race)
	AnimID = 0x6000 + table.GetValue (RaceName, "RACE")

	table = GemRB.LoadTable ("avprefc")
	Class = GemRB.GetPlayerStat (pc, IE_CLASS)
	ClassName = GUICommon.GetClassRowName (Class - 1, "index")
	AnimID = AnimID + table.GetValue (ClassName, "PREFIX")

	table = GemRB.LoadTable ("avprefg")
	AnimID = AnimID + table.GetValue (Gender, 0)

	PDollResRef = PDollTable.GetValue (hex(AnimID), "AT_1", GTV_STR)
	if PDollResRef == "*":
		print("ERROR, couldn't find the paperdoll! AnimID is", hex(AnimID))
		print("Falling back to an elven paperdoll.")
		PDollResRef = "CEMB1"
	PDollResRef += "G11"

	RefreshPDoll()
	ColorWindow.Focus()
	return

def RandomDonePress():
	#should be better
	GemRB.SetVar("Selected", GemRB.Roll(1,5,0) )
	DonePress()
	
def DonePress():
	global Color1, Color2, Color3, Color4, ColorWindow, ColorIndex, PickedColor, ColorPicker
	if ColorPicker:
		ColorPicker.Close ()
	ColorWindow.Focus()
	
	if ColorIndex==0:
		PickedColor=HairTable.GetValue(GemRB.GetVar("Selected"),0)
		Color1=PickedColor
		HairButton.SetBAM("COLGRAD", 1, 0, Color1)
		RefreshPDoll()
		return
	if ColorIndex==1:
		PickedColor=SkinTable.GetValue(GemRB.GetVar("Selected"),0)
		Color2=PickedColor
		SkinButton.SetBAM("COLGRAD", 1, 0, Color2)
		RefreshPDoll()
		return
	if ColorIndex==2:
		PickedColor=ColorTable.GetValue(0, GemRB.GetVar("Selected"))
		Color3=PickedColor
		MajorButton.SetBAM("COLGRAD", 1, 0, Color3)
		RefreshPDoll()
		return

	PickedColor=ColorTable.GetValue(1, GemRB.GetVar("Selected"))
	Color4=PickedColor
	MinorButton.SetBAM("COLGRAD", 1, 0, Color4)
	RefreshPDoll()
	return

def CancelPress():
	global ColorPicker, ColorWindow
	if ColorPicker:
		ColorPicker.Close ()
	ColorWindow.Focus()

def GetColor():
	global ColorPicker, ColorIndex, PickedColor

	ColorPicker=GemRB.LoadWindow(14)
	GemRB.SetVar("Selected", None)
	for i in range(33):
		Button = ColorPicker.GetControl(i)
		Button.SetState(IE_GUI_BUTTON_DISABLED)
		Button.SetFlags(IE_GUI_BUTTON_PICTURE|IE_GUI_BUTTON_RADIOBUTTON,OP_OR)

	m = 33
	if ColorIndex==0:
		m=HairTable.GetRowCount()
		t=HairTable
	if ColorIndex==1:
		m=SkinTable.GetRowCount()
		t=SkinTable
	for i in range(m):
		if ColorIndex<2:
			MyColor=t.GetValue(i,0)
		else:
			MyColor=ColorTable.GetValue(ColorIndex-2, i)
		if MyColor == "*":
			break
		Button = ColorPicker.GetControl(i)
		Button.SetBAM("COLGRAD", 2, 0, MyColor)
		if PickedColor == MyColor:
			GemRB.SetVar("Selected",i)
		Button.SetState(IE_GUI_BUTTON_ENABLED)
		Button.SetVarAssoc("Selected",i)
		Button.OnPress (DonePress)
	
	Button = ColorPicker.GetControl(33)
	#default button
	Button.SetVarAssoc("Selected", 0)
	Button.OnPress (RandomDonePress)
	Button.SetText("RND")

	CancelButton = ColorPicker.GetControl(35)
	CancelButton.SetText(13727)
	CancelButton.OnPress (CancelPress)
	CancelButton.MakeEscape()

	ColorPicker.ShowModal (MODAL_SHADOW_GRAY)
	ColorPicker.Focus()
	return

def HairPress():
	global ColorIndex, PickedColor

	ColorIndex = 0
	PickedColor = Color1
	GetColor()
	return

def SkinPress():
	global ColorIndex, PickedColor

	ColorIndex = 1
	PickedColor = Color2
	GetColor()
	return

def MajorPress():
	global ColorIndex, PickedColor

	ColorIndex = 2
	PickedColor = Color3
	GetColor()
	return

def MinorPress():
	global ColorIndex, PickedColor

	ColorIndex = 3
	PickedColor = Color4
	GetColor()
	return

def BackPress():
	if ColorWindow:
		ColorWindow.Close ()
	GemRB.SetNextScript("CharGen7")
	return

def NextPress():
	if ColorWindow:
		ColorWindow.Close ()
	GemRB.SetVar("Color1",Color1)
	GemRB.SetVar("Color2",Color2)
	GemRB.SetVar("Color3",Color3)
	GemRB.SetVar("Color4",Color4)
	GemRB.SetNextScript("CSound") #character sound
	return
