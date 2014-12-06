# GemRB - Infinity Engine Emulator
# Copyright (C) 2014 The GemRB Project
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
#character generation, spells (GUISPL2)
import GemRB
import CommonTables
import GUICommon
import LUSpellSelection
import IDLUCommon
import Spellbook
from ie_stats import IE_CLASS
from GUIDefines import IE_IWD2_SPELL_DOMAIN

def OnLoad ():
	MyChar = GemRB.GetVar ("Slot")
	Class = GemRB.GetPlayerStat (MyChar, IE_CLASS)
	ClassName = GUICommon.GetClassRowName (Class, "class")
	SpellTableName = CommonTables.ClassSkills.GetValue (ClassName, "MAGESPELL")
	# mxsplbon.2da is handled in core and does not affect learning

	# sorcerer types need this hack to not get too many spells
	# for them the castable and known count progress differently
	if SpellTableName == "MXSPLSOR":
		SpellTableName = "SPLSRCKN"
	elif SpellTableName == "MXSPLBRD":
		SpellTableName = "SPLBRDKN"

	# learn priest spells if any and setup spell levels
	# but first nullify any previous spells
	for row in range(CommonTables.ClassSkills.GetRowCount()):
		rowname = CommonTables.ClassSkills.GetRowName (row)
		SpellBookType = CommonTables.ClassSkills.GetValue (rowname, "SPLTYPE")
		if SpellBookType != "*":
			Spellbook.RemoveKnownSpells (MyChar, SpellBookType, 1,9, 0)
	Spellbook.RemoveKnownSpells (MyChar, IE_IWD2_SPELL_DOMAIN, 1,9, 0)
	IDLUCommon.LearnAnySpells (MyChar, ClassName)

	# make sure we have a correct table
	if SpellTableName == "*":
		GemRB.SetNextScript ("CharGen7")
		return

	Level = 1
	# NOTE: this way will only work for chargen, where there aren't any multikits
	KitValue = GemRB.GetPlayerStat (MyChar, IE_KIT)
	SpellBookType = CommonTables.ClassSkills.GetValue (ClassName, "SPLTYPE")

	LUSpellSelection.OpenSpellsWindow (MyChar, SpellTableName, Level, Level, KitValue, 1, True, SpellBookType)
