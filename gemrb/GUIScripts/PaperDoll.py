# GemRB - Infinity Engine Emulator
# Copyright (C) 2022 The GemRB Project
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

import GemRB
import GameCheck

import CommonTables
import GUICommon

from ie_restype import *
from ie_stats import *
from GUIDefines import *

ColorTable = GemRB.LoadTable ("clowncol")

def ColorStatsFromPortrait(PortraitName):
	if GameCheck.IsIWD1():
		PortraitTable = GemRB.LoadTable("PORTCOLR")
		PortraitName = PortraitTable.GetRowName(PortraitName) + "L"
	else:
		PortraitTable = GemRB.LoadTable("pictures")

	return {
		IE_MINOR_COLOR : PortraitTable.GetValue (PortraitName, "MINOR", GTV_INT),
		IE_MAJOR_COLOR : PortraitTable.GetValue (PortraitName, "MAJOR", GTV_INT),
		IE_SKIN_COLOR : PortraitTable.GetValue (PortraitName, "SKIN", GTV_INT),
		IE_HAIR_COLOR : PortraitTable.GetValue (PortraitName, "HAIR", GTV_INT),
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
			
def GetActorPaperDoll (actor):
	anim_id = GemRB.GetPlayerStat (actor, IE_ANIMATION_ID)
	if anim_id == 0x2000: # still in chargen
		anim_id = 0x6000
		table = GemRB.LoadTable ("avprefr")
		Race = GemRB.GetPlayerStat (actor, IE_RACE)
		anim_id = anim_id + table.GetValue(Race, 0)
		table = GemRB.LoadTable ("avprefc")
		ClassName = GUICommon.GetClassRowName (actor)
		anim_id = anim_id + table.GetValue (ClassName, "CLASS")
		table = GemRB.LoadTable ("avprefg")
		Gender = GemRB.GetPlayerStat (actor, IE_SEX)
		anim_id = anim_id + table.GetValue (Gender, 0)
	
	level = GemRB.GetPlayerStat (actor, IE_ARMOR_TYPE) or 0
	which = "LEVEL%d" % (level + 1)
	anim_id = hex(anim_id)
	doll = CommonTables.Pdolls.GetValue(anim_id, which)
	if doll == "*":
		# guess a name
		doll = GemRB.GetAvatarsValue (actor, level) + "INV"
		if not GemRB.HasResource (doll, RES_BAM):
			print("GetActorPaperDoll: Missing paper doll for animation {} and {}".format(anim_id, which))
	return doll

def OpenColorPicker(row, PickedColor, pack = "GUIREC"):
	if pack == "GUICG":
		winid = 14
		btnids = range(0, 34)
		offset = 0
	elif pack == "GUIINV":
		winid = 3
		btnids = range(0, 34)
		offset = 0
	else:
		# only GUIREC remains valid so treat everything else as GUIREC
		pack = "GUIREC"
		winid = 22
		btnids = range(1, 35)
		offset = 1

	Window = GemRB.LoadWindow (winid, pack)
	Window.SetVar("PickedColor", PickedColor)

	for i in btnids:
		Button = Window.GetControl(i)
		Button.SetFlags(IE_GUI_BUTTON_PICTURE, OP_OR)

		MyColor = ColorTable.GetValue(row, i - offset)
		if MyColor == "*":
			Button.SetState(IE_GUI_BUTTON_LOCKED)
			continue

		Button.SetBAM("COLGRAD", 2, 0, MyColor)
		Button.SetVarAssoc("PickedColor", MyColor)
		Button.OnPress(Window.Close)

	# restore value after SetVarAssoc
	Window.SetVar("PickedColor", PickedColor)
		
	if pack == "GUICG" and GameCheck.IsBG2 ():
		import CharGenCommon
		CharGenCommon.PositionCharGenWin(Window, -2)

	return Window
	
def SaveStats(stats, pc):
	for stat, color in stats.items():
		GemRB.SetPlayerStat(pc, stat, color)
		
def SelectColorForPC(stat, pc, pack = "GUIREC", stats = None):
	if stats is None:
		stats = ColorStatsFromPC(pc)

	row = 0 # hair
	if stat == IE_SKIN_COLOR:
		row = 1
	elif stat == IE_MAJOR_COLOR:
		row = 2
	elif stat == IE_MINOR_COLOR:
		row = 3
		
	Picker = OpenColorPicker(row, stats[stat], pack)
	
	if not GameCheck.IsBG2():
		Picker.ShowModal (MODAL_SHADOW_GRAY)

	return Picker

def OpenPaperDollWindow(pc, pack = "GUIREC", stats = None):
	if stats is None:
		stats = ColorStatsFromPC(pc)
		
	if pack == "GUICG":
		winid = 13
		controlids = {"DOLL" : 1, "HAIR" : 2, "SKIN" : 3, "MAJOR" : 4, "MINOR" : 5, "DONE" : 0, "CANCEL" : 13}
	else:
		# only GUIREC remains valid so treat everything else as GUIREC
		pack = "GUIREC"
		winid = 21
		controlids = {"DOLL" : 0, "HAIR" : 3, "SKIN" : 4, "MAJOR" : 5, "MINOR" : 6, "DONE" : 12, "CANCEL" : 13}

	Window = GemRB.LoadWindow (winid, pack)
	Window.AliasControls (controlids)

	PaperdollButton = Window.GetControlAlias ("DOLL")
	PaperdollButton.SetFlags (IE_GUI_BUTTON_PICTURE|IE_GUI_BUTTON_NO_IMAGE,OP_SET)
	PaperdollButton.SetState (IE_GUI_BUTTON_LOCKED)
	
	HairButton = Window.GetControlAlias ("HAIR")
	SkinButton = Window.GetControlAlias ("SKIN")
	MajorButton = Window.GetControlAlias ("MAJOR")
	MinorButton = Window.GetControlAlias ("MINOR")
	
	def SaveDoll():
		SaveStats(stats, pc)
		Window.Close()
	
	def UpdatePaperDoll ():
		PaperdollButton.SetPLT(GetActorPaperDoll (pc), stats, 0)
		MinorButton.SetBAM ("COLGRAD", 0, 0, stats[IE_MINOR_COLOR])
		MajorButton.SetBAM ("COLGRAD", 0, 0, stats[IE_MAJOR_COLOR])
		SkinButton.SetBAM ("COLGRAD", 0, 0, stats[IE_SKIN_COLOR])
		HairButton.SetBAM ("COLGRAD", 0, 0, stats[IE_HAIR_COLOR])

		return
		
	def SelectColor(stat):
		Picker = SelectColorForPC(stat, pc, pack, stats)
		
		def SaveStat():
			stats[stat] = Picker.GetVar("PickedColor")
			UpdatePaperDoll()

		Picker.SetAction (SaveStat, ACTION_WINDOW_CLOSED)

	HairButton.OnPress (lambda: SelectColor(IE_HAIR_COLOR))
	SkinButton.OnPress (lambda: SelectColor(IE_SKIN_COLOR))
	MajorButton.OnPress (lambda: SelectColor(IE_MAJOR_COLOR))
	MinorButton.OnPress (lambda: SelectColor(IE_MINOR_COLOR))
	
	CancelButton = Window.GetControlAlias("CANCEL")
	CancelButton.OnPress(Window.Close)
	CancelButton.SetText (13727)
	CancelButton.MakeEscape()
	
	DoneButton = Window.GetControlAlias("DONE")
	DoneButton.SetText (11973)
	DoneButton.MakeDefault()
	DoneButton.OnPress(SaveDoll)

	UpdatePaperDoll ()
	
	if not GameCheck.IsBG2():
		Window.ShowModal (MODAL_SHADOW_GRAY)
	return Window
