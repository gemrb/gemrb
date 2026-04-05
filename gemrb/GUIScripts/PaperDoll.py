# SPDX-FileCopyrightText: 2026 Contributors to the GemRB project <https://gemrb.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

import GemRB

import CommonTables
import GameCheck
import GUICommon

if GameCheck.IsIWD2 ():
	import IDLUCommon

from GUIDefines import *
from ie_restype import *
from ie_stats import *
from ie_slots import SLOT_WEAPON

ColorTable = GemRB.LoadTable ("clowncol", True, True)
StanceAnim = "" # set before use

def ColorStatsFromPortrait (PortraitName):
	if GameCheck.IsIWD1 ():
		PortraitTable = GemRB.LoadTable ("PORTCOLR")
		PortraitName = PortraitTable.GetRowName (PortraitName)
	elif GameCheck.IsAnyEE ():
		return ColorStatsFromEETables ()
	else:
		PortraitTable = GemRB.LoadTable ("pictures")

	def GetColorForStat (color):
		rgb = color & 0xFF
		rgb |= rgb << 8
		rgb |= rgb << 16
		return rgb

	return {
		IE_METAL_COLOR : GetColorForStat (0x17), # not editable in GUI, but part of the SetPLT payload
		IE_MINOR_COLOR : PortraitTable.GetValue (PortraitName, "MINOR", GTV_INT),
		IE_MAJOR_COLOR : PortraitTable.GetValue (PortraitName, "MAJOR", GTV_INT),
		IE_SKIN_COLOR : PortraitTable.GetValue (PortraitName, "SKIN", GTV_INT),
		IE_LEATHER_COLOR : GetColorForStat (0x1B),
		IE_ARMOR_COLOR : GetColorForStat (0x16),
		IE_HAIR_COLOR : PortraitTable.GetValue (PortraitName, "HAIR", GTV_INT)
	}

def ColorStatsFromEETables ():
	RaceColorTable = GemRB.LoadTable ("racecolr")
	ClassColorTable = GemRB.LoadTable ("clascolr")
	pc = GemRB.GetVar ("Slot")
	RaceName = GUICommon.GetRaceRowName (pc)
	# why be consistent?
	if RaceName == "HALF_ORC":
		RaceName = "HALFORC"
	# class or kit name
	ClassName = GUICommon.GetKitRowName (pc)

	return {
		IE_METAL_COLOR : ClassColorTable.GetValue ("METAL", ClassName, GTV_INT),
		IE_MINOR_COLOR : ClassColorTable.GetValue ("MINOR_CLOTH", ClassName, GTV_INT),
		IE_MAJOR_COLOR : ClassColorTable.GetValue ("MAIN_CLOTH", ClassName, GTV_INT),
		IE_SKIN_COLOR : RaceColorTable.GetValue ("SKIN", RaceName, GTV_INT),
		IE_LEATHER_COLOR : ClassColorTable.GetValue ("LEATHER", ClassName, GTV_INT),
		IE_ARMOR_COLOR : ClassColorTable.GetValue ("ARMOR", ClassName, GTV_INT),
		IE_HAIR_COLOR : RaceColorTable.GetValue ("HAIR", RaceName, GTV_INT)
	}

def ColorStatsFromPC (pc):
	return {
		IE_METAL_COLOR : GemRB.GetPlayerStat (pc, IE_METAL_COLOR), # not editable in GUI, but part of the SetPLT payload
		IE_MINOR_COLOR : GemRB.GetPlayerStat (pc, IE_MINOR_COLOR),
		IE_MAJOR_COLOR : GemRB.GetPlayerStat (pc, IE_MAJOR_COLOR),
		IE_SKIN_COLOR : GemRB.GetPlayerStat (pc, IE_SKIN_COLOR),
		IE_LEATHER_COLOR : GemRB.GetPlayerStat (pc, IE_LEATHER_COLOR),
		IE_ARMOR_COLOR : GemRB.GetPlayerStat (pc, IE_ARMOR_COLOR),
		IE_HAIR_COLOR : GemRB.GetPlayerStat (pc, IE_HAIR_COLOR)
	}

def GetActorPaperDoll (pc):
	if GameCheck.IsIWD2 ():
		return GetActorPaperDollIWD2 (pc)
	elif GameCheck.IsGemRBDemo ():
		# sometimes we need a different table and animations
		level = GemRB.GetPlayerStat (pc, IE_ARMOR_TYPE)
		return GemRB.GetAvatarsValue (pc, level) + StanceAnim
	return GetActorPaperDoll0 (pc)

def GetActorPaperDollIWD2 (pc):
	# calculate the paperdoll animation id from the race, class and gender
	PDollTable = GemRB.LoadTable ("avatars")
	table = GemRB.LoadTable ("avprefr")

	RaceName = GUICommon.GetRaceRowName (pc)
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
	PDollResRef += StanceAnim

	return PDollResRef

def GetActorPaperDoll0 (pc):
	# calculate the paperdoll animation id from the race, class and gender
	anim_id = GemRB.GetPlayerStat (pc, IE_ANIMATION_ID)
	if anim_id == 0x2000: # still in chargen
		anim_id = 0x6000
		table = GemRB.LoadTable ("avprefr")
		Race = GemRB.GetPlayerStat (pc, IE_RACE)
		anim_id = anim_id + table.GetValue (Race, 0)
		table = GemRB.LoadTable ("avprefc")
		ClassName = GUICommon.GetClassRowName (pc)
		anim_id = anim_id + table.GetValue (ClassName, "CLASS")
		table = GemRB.LoadTable ("avprefg")
		Gender = GemRB.GetPlayerStat (pc, IE_SEX)
		anim_id = anim_id + table.GetValue (Gender, 0)

	level = GemRB.GetPlayerStat (pc, IE_ARMOR_TYPE) or 0
	which = "LEVEL%d" % (level + 1)
	anim_id = hex(anim_id)
	doll = CommonTables.Pdolls.GetValue (anim_id, which)
	if doll == "*":
		# guess a name
		doll = GemRB.GetAvatarsValue (pc, level) + "INV"
		if not GemRB.HasResource (doll, RES_BAM):
			print("GetActorPaperDoll: Missing paper doll for animation {} and {}".format(anim_id, which))
	return doll

def OpenPaperDollWindow(pc, pack, stats):
	if pack == "GUICG":
		winID = 13
		controlIDs = {"DOLL" : 1, "HAIR" : 2, "SKIN" : 3, "MAJOR" : 4, "MINOR" : 5, "DONE" : 0, "CANCEL" : 13}
	else:
		# only GUIREC remains valid so treat everything else as GUIREC
		pack = "GUIREC"
		winID = 21
		controlIDs = {"DOLL" : 0, "HAIR" : 3, "SKIN" : 4, "MAJOR" : 5, "MINOR" : 6, "DONE" : 12, "CANCEL" : 13}

	ColorWindow = GemRB.LoadWindow (winID, pack)
	ColorWindow.AliasControls (controlIDs)

	PDollButton = ColorWindow.GetControlAlias ("DOLL")
	pDollFlags = IE_GUI_BUTTON_CENTER_PICTURES if GameCheck.IsIWD2 () else 0
	PDollButton.SetFlags (IE_GUI_BUTTON_PICTURE | pDollFlags, OP_OR) # add/OP_SET IE_GUI_BUTTON_NO_IMAGE?
	PDollButton.SetState (IE_GUI_BUTTON_LOCKED)

	HairButton = ColorWindow.GetControlAlias ("HAIR")
	SkinButton = ColorWindow.GetControlAlias ("SKIN")
	MajorButton = ColorWindow.GetControlAlias ("MAJOR")
	MinorButton = ColorWindow.GetControlAlias ("MINOR")

	def SaveDoll ():
		SaveStats (stats, pc)
		ColorWindow.Close ()

	def UpdatePaperDoll ():
		pdoll = GetActorPaperDoll (pc)
		if GameCheck.IsIWD2 ():
			pal = [0, stats[IE_MINOR_COLOR], stats[IE_MAJOR_COLOR], stats[IE_SKIN_COLOR], 0, 0, stats[IE_HAIR_COLOR], 0]
			PDollButton.SetAnimation (None) # force reset
			PDollButton.SetAnimation (pdoll, 1, A_ANI_ACTIVE, pal) # add A_ANI_PLAYONCE?
			PDollButton.SetBAM ("", 0, 0, 0) # just hide or there is a tiny artifact
		else:
			PDollButton.SetPLT (pdoll, stats, 0)

		cycle = int(GameCheck.IsIWD2 ())
		MinorButton.SetBAM ("COLGRAD", cycle, 0, stats[IE_MINOR_COLOR])
		MajorButton.SetBAM ("COLGRAD", cycle, 0, stats[IE_MAJOR_COLOR])
		SkinButton.SetBAM ("COLGRAD", cycle, 0, stats[IE_SKIN_COLOR])
		HairButton.SetBAM ("COLGRAD", cycle, 0, stats[IE_HAIR_COLOR])

		return

	def SelectColor (stat):
		Picker = SelectColorForPC (stat, pc, pack)

		def SaveStat ():
			stats[stat] = GemRB.GetVar ("PickedColor")
			UpdatePaperDoll ()

		Picker.SetAction (SaveStat, ACTION_WINDOW_CLOSED)

	HairButton.OnPress (lambda: SelectColor (IE_HAIR_COLOR))
	SkinButton.OnPress (lambda: SelectColor (IE_SKIN_COLOR))
	MajorButton.OnPress (lambda: SelectColor (IE_MAJOR_COLOR))
	MinorButton.OnPress (lambda: SelectColor (IE_MINOR_COLOR))

	BackButton = ColorWindow.GetControlAlias ("CANCEL")
	BackButton.SetText (15416 if GameCheck.IsIWD2 () else 13727)
	BackButton.MakeEscape ()

	DoneButton = ColorWindow.GetControlAlias ("DONE")
	DoneButton.SetText (11973)
	DoneButton.MakeDefault ()
	if not GameCheck.IsIWD2 ():
		DoneButton.OnPress (SaveDoll)
		BackButton.OnPress (ColorWindow.Close)

	UpdatePaperDoll ()

	if not GameCheck.IsBG2 ():
		ColorWindow.ShowModal (MODAL_SHADOW_GRAY)
	return ColorWindow

def SaveStats (stats, pc):
	for stat, color in stats.items():
		GemRB.SetPlayerStat (pc, stat, color)

def ResetStats (pc):
	nullStats = {
		IE_MINOR_COLOR : 0, IE_MAJOR_COLOR : 0, IE_SKIN_COLOR : 0,
		IE_HAIR_COLOR : 0, IE_LEATHER_COLOR : 0, IE_ARMOR_COLOR : 0, IE_METAL_COLOR : 0
	}
	SaveStats (nullStats, pc)
	return

def SelectColorForPC (stat, pc, pack = "GUICG"):
	stats = ColorStatsFromPC (pc)

	row = 0 # hair
	if stat == IE_SKIN_COLOR:
		row = 1
	elif stat == IE_MAJOR_COLOR:
		row = 2
	elif stat == IE_MINOR_COLOR:
		row = 3

	Picker = OpenColorPicker (row, pc, stats[stat], pack)
	if not GameCheck.IsBG2():
		Picker.ShowModal (MODAL_SHADOW_GRAY)

	return Picker

def GetMyColor (row, i, offset, table):
	if not GameCheck.IsIWD2 ():
		return ColorTable.GetValue (row, i - offset)

	if row < 2:
		return table.GetValue (i, 0)
	else:
		return ColorTable.GetValue (row - 2, i)

def OpenColorPicker (row, pc, PickedColor, pack = "GUICG"):
	if pack == "GUICG":
		winID = 14
		btnIDs = range(0, 33 if GameCheck.IsIWD2 () else 34)
		offset = 0
	elif pack == "GUIINV":
		winID = 3
		btnIDs = range(0, 33 if GameCheck.IsIWD2 () else 34)
		offset = 0
	else:
		# only GUIREC remains valid so treat everything else as GUIREC
		pack = "GUIREC"
		winID = 22
		btnIDs = range(1, 35)
		offset = 1

	ColorPicker = GemRB.LoadWindow (winID, pack)
	# reset to single color (band) to match table values
	# needed for when the value comes from a saved stat
	PickedColor = PickedColor & 0xFF
	GemRB.SetVar ("PickedColor", PickedColor)

	for i in btnIDs:
		Button = ColorPicker.GetControl (i)
		Button.SetState (IE_GUI_BUTTON_LOCKED)
		Button.SetFlags (IE_GUI_BUTTON_PICTURE | IE_GUI_BUTTON_RADIOBUTTON, OP_OR)

	table = ColorTable
	if GameCheck.IsIWD2 ():
		SRTable = GemRB.LoadTable ("srtable", False, True)
		RaceName = GUICommon.GetRaceRowName (pc)
		HairTable = GemRB.LoadTable (SRTable.GetValue (RaceName, "HAIRCLRFILE"), False, True)
		SkinTable = GemRB.LoadTable (SRTable.GetValue (RaceName, "SKINCLRFILE"), False, True)
		if row == 0:
			btnIDs = range(HairTable.GetRowCount ())
			table = HairTable
		elif row == 1:
			btnIDs = range(SkinTable.GetRowCount ())
			table = SkinTable

	selectedBtn = None # radiobutton hack
	for i in btnIDs:
		MyColor = GetMyColor (row, i, offset, table)
		if MyColor == "*":
			break
		Button = ColorPicker.GetControl (i)
		Button.SetBAM ("COLGRAD", 2, 0, MyColor)

		Button.SetState (IE_GUI_BUTTON_ENABLED)
		Button.SetVarAssoc ("PickedColor", MyColor)
		if PickedColor == MyColor:
			selectedBtn = Button
		Button.OnPress (ColorPicker.Close)

	# restore value after SetVarAssoc
	GemRB.SetVar ("PickedColor", PickedColor)
	if selectedBtn:
		selectedBtn.SetState (IE_GUI_BUTTON_SELECTED)

	if pack == "GUICG" and GameCheck.IsBG2 ():
		import CharGenCommon
		CharGenCommon.PositionCharGenWin (ColorPicker, -2)

	def RandomSelect():
		color = "*"
		while color == "*":
			roll = GemRB.Roll (1, btnIDs[-1], 0)
			if row < 2:
				color = t.GetValue (roll, 0)
			else:
				color = ColorTable.GetValue (row - 2, roll)

		GemRB.SetVar ("PickedColor", color)
		ColorPicker.Close ()

	if GameCheck.IsIWD2 ():
		Button = ColorPicker.GetControl (33)
		Button.OnPress (RandomSelect)
		Button.SetText ("RND")

		CancelButton = ColorPicker.GetControl (35)
		CancelButton.SetText (13727)
		CancelButton.OnPress (ColorPicker.Close)
		CancelButton.MakeEscape ()

	return ColorPicker

def SelectPickerColor (stat):
	pc = GemRB.GameGetSelectedPCSingle ()
	Picker = SelectColorForPC (stat, pc, "GUIINV")
	Picker.SetAction (lambda: GemRB.SetPlayerStat (pc, stat, GemRB.GetVar ("PickedColor")), ACTION_WINDOW_CLOSED)
	return

# overlay more images for weapons, shields and helmets
# body armor already dictates the base paperdoll
def SetupEquipment (pc, button, size, stats):
	# Weapon
	slotItem = GemRB.GetSlotItem (pc, GemRB.GetEquippedQuickSlot (pc))
	if slotItem:
		item = GemRB.GetItem (slotItem["ItemResRef"])
		if item['AnimationType'] != '':
			button.SetPLT ("WP" + size + item['AnimationType'] + "INV", stats, 1)

	# Shield
	slotItem = GemRB.GetSlotItem (pc, 3)
	if slotItem:
		itemname = slotItem["ItemResRef"]
		item = GemRB.GetItem (itemname)
		if item['AnimationType'] != '':
			if (GemRB.CanUseItemType (SLOT_WEAPON, itemname)):
				# off-hand weapon
				button.SetPLT ("WP" + size + item['AnimationType'] + "OIN", stats, 2)
			else:
				# shield
				button.SetPLT ("WP" + size + item['AnimationType'] + "INV", stats, 2)

	# Helmet
	slotItem = GemRB.GetSlotItem (pc, 1)
	if slotItem:
		# halflings use gnome helmet files
		# only weapons exist with H, so this is fine for everyone
		if size == "H":
			size = "S"
		item = GemRB.GetItem (slotItem["ItemResRef"])
		if item['AnimationType'] != '':
			button.SetPLT ("WP" + size + item['AnimationType'] + "INV", stats, 3)
