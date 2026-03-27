# GemRB - Infinity Engine Emulator
# Copyright (C) 2003-2004 The GemRB Project
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
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#
# character generation, mage spells (GUICG7)

import GemRB
import GameCheck
import GUICommon
import CommonTables
import LUSpellSelection
import Spellbook
from GUIDefines import *
from ie_stats import *

def OnLoad():
	Slot = GemRB.GetVar ("Slot")
	Class = GemRB.GetPlayerStat (Slot, IE_CLASS)
	ClassName = GUICommon.GetClassRowName (Class, "class")
	TableName = CommonTables.ClassSkills.GetValue(ClassName, "MAGESPELL")

	# make sure we have a correct table
	if TableName == "*":
		GemRB.SetNextScript("GUICG6")
		return

	# get our kit index
	KitTable = GemRB.LoadTable ("magesch")
	KitIndex = GUICommon.GetKitIndex (Slot)
	if KitIndex:
		if GameCheck.IsAnyEE ():
			KitRowName = CommonTables.KitList.GetRowName (KitIndex)
			KitValue = CommonTables.KitList.GetValue (KitRowName, "KITIDS")
		else:
			KitValue = KitTable.GetValue (KitIndex - 21, 3)

		# bards have kits too
		if KitValue == -1:
			KitValue = 0x4000 # we only need it for the spells, so this is ok
	else:
		KitValue = 0x4000

	# dragon disciples have their own table for the -1 spells per level penalty
	# howevere as kits they're treated as specialists, so get +1, therefore the table has -2
	# currently all such kits get treated the same way, since the table is not in kitlist.2da
	if Spellbook.IsSorcererKit (Slot):
		TableName = "MXSPLDD"

	# open up the spell selection window
	# remember, it is pc, table, level, diff, kit, chargen
	IsMulti = GUICommon.IsMultiClassed (Slot, 1)
	Level = GemRB.GetPlayerStat (Slot, IE_LEVEL)
	if IsMulti[0]>1:
		for i in range (1, IsMulti[0]):
			ClassName = GUICommon.GetClassRowName (IsMulti[i], "class")
			if CommonTables.ClassSkills.GetValue (ClassName, "MAGESPELL", GTV_STR) != "*":
				Level = GemRB.GetPlayerStat (Slot, IE_LEVEL2+i-1)
			break

	LUSpellSelection.OpenSpellsWindow (Slot, TableName, Level, Level, KitValue, 1)

	return
