# SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

# character generation, mage spells (GUICG7)

import GemRB
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

	# get our kit
	KitValue = GemRB.GetPlayerStat (Slot, IE_KIT)

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
	Spellbook.SetupSpellLevels(Slot, TableName, IE_SPELL_TYPE_WIZARD, 1)

	LUSpellSelection.OpenSpellsWindow (Slot, TableName, Level, Level, KitValue, 1,False)

	return
