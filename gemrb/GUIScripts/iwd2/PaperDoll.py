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
import GUICommon
import IDLUCommon
import Portrait

from GUIDefines import *
from ie_stats import *

ColorTable = GemRB.LoadTable("clowncol")

def GetActorPaperDoll(pc):
	# calculate the paperdoll animation id from the race, class and gender
	PDollTable = GemRB.LoadTable ("avatars")
	table = GemRB.LoadTable ("avprefr")
	
	Race = IDLUCommon.GetRace (pc)
	RaceName = CommonTables.Races.GetRowName (Race)
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
	Gender = GemRB.GetPlayerStat (pc, IE_SEX)
	AnimID = AnimID + table.GetValue (Gender, 0)

	PDollResRef = PDollTable.GetValue (hex(AnimID), "AT_1", GTV_STR)
	if PDollResRef == "*":
		print("ERROR, couldn't find the paperdoll! AnimID is", hex(AnimID))
		print("Falling back to an elven paperdoll.")
		PDollResRef = "CEMB1"
	PDollResRef += "G11"
		
	return PDollResRef

def ColorStatsFromPortrait(PortraitName):
	PortraitTable = GemRB.LoadTable("pictures")

	return {
		IE_MINOR_COLOR : PortraitTable.GetValue(PortraitName, "MINOR", GTV_INT),
		IE_MAJOR_COLOR : PortraitTable.GetValue(PortraitName, "MAJOR", GTV_INT),
		IE_SKIN_COLOR : PortraitTable.GetValue(PortraitName, "SKIN", GTV_INT),
		IE_HAIR_COLOR : PortraitTable.GetValue(PortraitName, "HAIR", GTV_INT),
		# not editable in GUI, but part of the SetPLT payload
		IE_LEATHER_COLOR : 0,
		IE_ARMOR_COLOR : 0,
		IE_METAL_COLOR : 0
		}
		
def ColorStatsFromPC(pc):
	return {
			IE_MINOR_COLOR : GemRB.GetPlayerStat(pc, IE_MINOR_COLOR),
			IE_MAJOR_COLOR : GemRB.GetPlayerStat(pc, IE_MAJOR_COLOR),
			IE_SKIN_COLOR : GemRB.GetPlayerStat(pc, IE_SKIN_COLOR),
			IE_HAIR_COLOR : GemRB.GetPlayerStat(pc, IE_HAIR_COLOR),
			# not editable in GUI, but part of the SetPLT payload
			IE_LEATHER_COLOR : GemRB.GetPlayerStat(pc, IE_LEATHER_COLOR),
			IE_ARMOR_COLOR : GemRB.GetPlayerStat(pc, IE_ARMOR_COLOR),
			IE_METAL_COLOR : GemRB.GetPlayerStat(pc, IE_METAL_COLOR)
			}

def OpenPaperDollWindow(pc, pack, stats):
	ColorWindow = GemRB.LoadWindow(13, "GUICG")
	
	PDollButton = ColorWindow.GetControl(1)
	PDollButton.SetFlags(IE_GUI_BUTTON_PICTURE | IE_GUI_BUTTON_PLAYALWAYS | IE_GUI_BUTTON_CENTER_PICTURES, OP_OR)
	PDollButton.SetState(IE_GUI_BUTTON_LOCKED)

	HairButton = ColorWindow.GetControl(2)
	SkinButton = ColorWindow.GetControl(3)
	MinorButton = ColorWindow.GetControl(4)
	MajorButton = ColorWindow.GetControl(5)
	
	def UpdatePaperDoll ():
		pdoll = GetActorPaperDoll(pc)
		pal = [0, stats[IE_MINOR_COLOR], stats[IE_MAJOR_COLOR], stats[IE_SKIN_COLOR], 0, 0, stats[IE_HAIR_COLOR], 0]
		PDollButton.SetAnimation(pdoll, 1, 8, pal)
		PDollButton.SetBAM("", 0, 0, 0) # just hide or there is a tiny artifact
		PDollButton.SetAnimation (None) # force reset
		MinorButton.SetBAM ("COLGRAD", 1, 0, stats[IE_MINOR_COLOR])
		MajorButton.SetBAM ("COLGRAD", 1, 0, stats[IE_MAJOR_COLOR])
		SkinButton.SetBAM ("COLGRAD", 1, 0, stats[IE_SKIN_COLOR])
		HairButton.SetBAM ("COLGRAD", 1, 0, stats[IE_HAIR_COLOR])

		return
		
	def SelectColor(stat):
		Picker = SelectColorForPC(stat, pc, pack)
		
		def SaveStat():
			stats[stat] = Picker.GetVar("PickedColor")
			UpdatePaperDoll()

		Picker.SetAction (SaveStat, ACTION_WINDOW_CLOSED)

	HairButton.OnPress (lambda: SelectColor(IE_HAIR_COLOR))
	SkinButton.OnPress (lambda: SelectColor(IE_SKIN_COLOR))
	MajorButton.OnPress (lambda: SelectColor(IE_MAJOR_COLOR))
	MinorButton.OnPress (lambda: SelectColor(IE_MINOR_COLOR))

	BackButton = ColorWindow.GetControl(13)
	BackButton.SetText(15416)
	DoneButton = ColorWindow.GetControl(0)
	DoneButton.SetText(11973)
	DoneButton.MakeDefault()

	UpdatePaperDoll()
	return ColorWindow
	
def SelectColorForPC(stat, pc, pack = "CUICG"):
	stats = ColorStatsFromPC(pc)

	row = 0 # hair
	if stat == IE_SKIN_COLOR:
		row = 1
	elif stat == IE_MAJOR_COLOR:
		row = 2
	elif stat == IE_MINOR_COLOR:
		row = 3
		
	Picker = OpenColorPicker(row, stats[stat], pack)
	Picker.ShowModal (MODAL_SHADOW_GRAY)

	return Picker
	
def OpenColorPicker(row, PickedColor, pack = "GUICG"):
	if pack == "GUICG":
		ColorPicker = GemRB.LoadWindow(14, pack)
	else:
		ColorPicker = GemRB.LoadWindow(3, pack)

	ColorPicker.SetVar("PickedColor", PickedColor)

	for i in range(33):
		Button = ColorPicker.GetControl(i)
		Button.SetState(IE_GUI_BUTTON_DISABLED)
		Button.SetFlags(IE_GUI_BUTTON_PICTURE|IE_GUI_BUTTON_RADIOBUTTON,OP_OR)
		
	pc = GemRB.GetVar("SELECTED_PC") or GemRB.GetVar("Slot")
	Race = IDLUCommon.GetRace (pc)
	RaceName = CommonTables.Races.GetRowName (Race)
	HairTable = GemRB.LoadTable(CommonTables.Races.GetValue(RaceName, "HAIR"))
	SkinTable = GemRB.LoadTable(CommonTables.Races.GetValue(RaceName, "SKIN"))

	m = 33
	if row == 0:
		m = HairTable.GetRowCount()
		t = HairTable
	if row == 1:
		m = SkinTable.GetRowCount()
		t = SkinTable
	for i in range(m):
		if row < 2:
			MyColor = t.GetValue(i, 0)
		else:
			MyColor = ColorTable.GetValue(row - 2, i)
		if MyColor == "*":
			break
		Button = ColorPicker.GetControl(i)
		Button.SetBAM("COLGRAD", 2, 0, MyColor)

		Button.SetState(IE_GUI_BUTTON_ENABLED)
		Button.SetVarAssoc("PickedColor", MyColor)
		Button.OnPress(ColorPicker.Close)
		
	def RandomSelect():
		color = "*"
		while color == "*":
			roll = GemRB.Roll(1, m, 0)
			if row < 2:
				color = t.GetValue(roll, 0)
			else:
				color = ColorTable.GetValue(row - 2, roll)

		print("Random Color: {}".format(color))
		ColorPicker.SetVar("PickedColor", color)
		ColorPicker.Close()
	
	Button = ColorPicker.GetControl(33)
	Button.OnPress (RandomSelect)
	Button.SetText("RND")

	CancelButton = ColorPicker.GetControl(35)
	CancelButton.SetText(13727)
	CancelButton.OnPress(ColorPicker.Close)
	CancelButton.MakeEscape()

	return ColorPicker
