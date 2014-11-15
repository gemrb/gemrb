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
import GUICommonWindows
import LUSpellSelection
import IDLUCommon
from ie_stats import IE_CLASS

def OnLoad ():
	MyChar = GemRB.GetVar ("Slot")
	Class = GemRB.GetPlayerStat (MyChar, IE_CLASS)
	ClassName = GUICommon.GetClassRowName (Class, "class")
	SpellTableName = CommonTables.ClassSkills.GetValue (ClassName, "MAGESPELL")
	# mxsplbon.2da is handled in core and does not affect learning

	# learn priest spells if any and setup spell levels
	IDLUCommon.LearnAnySpells (MyChar, ClassName)

	# make sure we have a correct table
	if SpellTableName == "*":
		GemRB.SetNextScript ("CharGen7")
		return

	Level = 1
	# NOTE: this way will only work for chargen, where there aren't any multikits
#	KitIndex = GUICommonWindows.GetKitIndex (MyChar, Class)
#	KitName = CommonTables.Classes.GetRowName (KitIndex)
#	KitValue = CommonTables.Classes.GetValue (KitName, "ID")
	KitValue = GemRB.GetPlayerStat (MyChar, IE_KIT)

	LUSpellSelection.OpenSpellsWindow (MyChar, SpellTableName, Level, Level, KitValue, 1)
