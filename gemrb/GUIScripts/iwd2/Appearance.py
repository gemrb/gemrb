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
from GUIDefines import *
import CommonTables

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

	PDollButton.SetFlags(IE_GUI_BUTTON_ANIMATED|IE_GUI_BUTTON_PLAYALWAYS|IE_GUI_BUTTON_CENTER_PICTURES, OP_OR)
	PDollButton.SetPLT(PDollResRef, 0, Color4, Color3, Color2, 0, 0, Color1, 0)
	PDollButton.SetBAM("", 0, 0, 0) # just hide or there is a tiny artifact
	PDollButton.SetAnimation(PDollResRef, 2)
	return

def OnLoad():
	global ColorWindow, DoneButton, PDollButton
	global HairTable, SkinTable, ColorTable
	global HairButton, SkinButton, MajorButton, MinorButton
	global Color1, Color2, Color3, Color4, PDollResRef
	
	GemRB.LoadWindowPack("GUICG", 800, 600)
	ColorWindow=GemRB.LoadWindow(13)

	Race = CommonTables.Races.FindValue (3, GemRB.GetVar ("Race") )
	HairTable = GemRB.LoadTable(CommonTables.Races.GetValue(Race, 5))
	SkinTable = GemRB.LoadTable(CommonTables.Races.GetValue(Race, 6))
	ColorTable = GemRB.LoadTable("clowncol")

	#set these colors to some default
	PortraitTable = GemRB.LoadTable("pictures")
	PortraitIndex = GemRB.GetVar("PortraitIndex")
	Color1=PortraitTable.GetValue(PortraitIndex,1)
	Color2=PortraitTable.GetValue(PortraitIndex,2)
	Color3=PortraitTable.GetValue(PortraitIndex,3)
	Color4=PortraitTable.GetValue(PortraitIndex,4)
	PDollButton = ColorWindow.GetControl(1)
	PDollButton.SetFlags(IE_GUI_BUTTON_PICTURE,OP_OR)
	PDollButton.SetState(IE_GUI_BUTTON_LOCKED)

	HairButton = ColorWindow.GetControl(2)
	HairButton.SetFlags(IE_GUI_BUTTON_PICTURE,OP_OR)
	HairButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, HairPress)
	HairButton.SetBAM("COLGRAD", 1, 0, Color1)

	SkinButton = ColorWindow.GetControl(3)
	SkinButton.SetFlags(IE_GUI_BUTTON_PICTURE,OP_OR)
	SkinButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, SkinPress)
	SkinButton.SetBAM("COLGRAD", 1, 0, Color2)

	MajorButton = ColorWindow.GetControl(5)
	MajorButton.SetFlags(IE_GUI_BUTTON_PICTURE,OP_OR)
	MajorButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, MajorPress)
	MajorButton.SetBAM("COLGRAD", 1, 0, Color3)

	MinorButton = ColorWindow.GetControl(4)
	MinorButton.SetFlags(IE_GUI_BUTTON_PICTURE,OP_OR)
	MinorButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, MinorPress)
	MinorButton.SetBAM("COLGRAD", 1, 0, Color4)

	BackButton = ColorWindow.GetControl(13)
	BackButton.SetText(15416)
	DoneButton = ColorWindow.GetControl(0)
	DoneButton.SetText(11973)
	DoneButton.SetFlags(IE_GUI_BUTTON_DEFAULT,OP_OR)

	DoneButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, NextPress)
	BackButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, BackPress)

	# calculate the paperdoll animation id from the race, class and gender
	PDollTable = GemRB.LoadTable ("avatars")
	table = GemRB.LoadTable ("avprefr")
	AnimID = 0x6000 + table.GetValue (GemRB.GetVar("BaseRace"), 0)

	table = GemRB.LoadTable ("avprefc")
	AnimID = AnimID + table.GetValue (GemRB.GetVar("BaseClass"), 0)

	table = GemRB.LoadTable ("avprefg")
	AnimID = AnimID + table.GetValue (GemRB.GetVar("Gender"), 0)

	PDollResRef = PDollTable.GetValue (hex(AnimID), "AT_1") + "G11"
	if PDollResRef == "*G11":
		print "ERROR, couldn't find the paperdoll! AnimID is", hex(AnimID)
		print "Falling back to an elven paperdoll."
		PDollResRef = "CEMB1G11"

	RefreshPDoll()
	ColorWindow.SetVisible(WINDOW_VISIBLE)
	return

def RandomDonePress():
	#should be better
	GemRB.SetVar("Selected", GemRB.Roll(1,5,0) )
	DonePress()
	
def DonePress():
	global Color1, Color2, Color3, Color4, ColorWindow, ColorIndex, PickedColor, ColorPicker
	if ColorPicker:
		ColorPicker.Unload()
	ColorWindow.SetVisible(WINDOW_VISIBLE)
	
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
		ColorPicker.Unload ()
	ColorWindow.SetVisible (WINDOW_VISIBLE)

def GetColor():
	global ColorPicker, ColorIndex, PickedColor

	ColorPicker=GemRB.LoadWindow(14)
	GemRB.SetVar("Selected",-1)
	for i in range(33):
		Button = ColorPicker.GetControl(i)
		Button.SetState(IE_GUI_BUTTON_DISABLED)
		Button.SetFlags(IE_GUI_BUTTON_PICTURE|IE_GUI_BUTTON_RADIOBUTTON,OP_OR)

	Selected = -1
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
			Selected = i
		Button.SetState(IE_GUI_BUTTON_ENABLED)
		Button.SetVarAssoc("Selected",i)
		Button.SetEvent(IE_GUI_BUTTON_ON_PRESS, DonePress)
	
	Button = ColorPicker.GetControl(33)
	#default button
	Button.SetVarAssoc("Selected", 0)
	Button.SetEvent(IE_GUI_BUTTON_ON_PRESS, RandomDonePress)
	Button.SetText("RND")

	CancelButton = ColorPicker.GetControl(35)
	CancelButton.SetText(13727)
	CancelButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, CancelPress)
	CancelButton.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)

	ColorPicker.SetVisible(WINDOW_VISIBLE)
	return

def HairPress():
	global ColorIndex, PickedColor

#	ColorWindow.Unload()
	ColorWindow.SetVisible(WINDOW_INVISIBLE)
	ColorIndex = 0
	PickedColor = Color1
	GetColor()
	return

def SkinPress():
	global ColorIndex, PickedColor

#	ColorWindow.Unload()
	ColorWindow.SetVisible(WINDOW_INVISIBLE)
	ColorIndex = 1
	PickedColor = Color2
	GetColor()
	return

def MajorPress():
	global ColorIndex, PickedColor

#	ColorWindow.Unload()
	ColorWindow.SetVisible(WINDOW_INVISIBLE)
	ColorIndex = 2
	PickedColor = Color3
	GetColor()
	return

def MinorPress():
	global ColorIndex, PickedColor

#	ColorWindow.Unload()
	ColorWindow.SetVisible(WINDOW_INVISIBLE)
	ColorIndex = 3
	PickedColor = Color4
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
	GemRB.SetVar("Color1",Color1)
	GemRB.SetVar("Color2",Color2)
	GemRB.SetVar("Color3",Color3)
	GemRB.SetVar("Color4",Color4)
	GemRB.SetNextScript("CSound") #character sound
	return
