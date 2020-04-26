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
from ie_stats import IE_CLASS, IE_CLASSLEVELSUM, IE_KIT
from GUIDefines import IE_IWD2_SPELL_DOMAIN

def OnLoad ():
	SetupSpellsWindow (1)

def SetupSpellsWindow(chargen=0):
	if chargen:
		MyChar = GemRB.GetVar ("Slot")
		Class = GemRB.GetPlayerStat (MyChar, IE_CLASS)
		ClassName = GUICommon.GetClassRowName (Class, "class")
		Level = 0
		LevelDiff = 1
		KitValue = GemRB.GetPlayerStat (MyChar, IE_KIT)
	else:
		MyChar = GemRB.GameGetSelectedPCSingle ()
		ClassIndex = GemRB.GetVar ("LUClass")
		ClassName = GUICommon.GetClassRowName (ClassIndex, "index")
		LevelDiff = GemRB.GetVar ("LevelDiff")
		Level = GemRB.GetPlayerStat (MyChar, IE_CLASSLEVELSUM)
		# this is only used for detecting specialists!
		KitValue = GemRB.GetVar ("LUKit")

	SpellTableName = CommonTables.ClassSkills.GetValue (ClassName, "MAGESPELL")

	# charbase has domain slots reserved, so nuke them
	if chargen:
		Spellbook.UnsetupSpellLevels (MyChar, "MXSPLCLR", IE_IWD2_SPELL_DOMAIN, 1)

	# learn priest spells if any and setup spell levels
	# but first nullify any previous spells
	if chargen:
		for row in range(CommonTables.ClassSkills.GetRowCount()):
			rowname = CommonTables.ClassSkills.GetRowName (row)
			SpellBookType = CommonTables.ClassSkills.GetValue (rowname, "SPLTYPE")
			if SpellBookType != "*":
				Spellbook.RemoveKnownSpells (MyChar, SpellBookType, 1,9, 0)
		Spellbook.RemoveKnownSpells (MyChar, IE_IWD2_SPELL_DOMAIN, 1,9, 0)
	IDLUCommon.LearnAnySpells (MyChar, ClassName, chargen)

	# make sure we have a correct table and that we're eligible
	BookType = CommonTables.ClassSkills.GetValue (ClassName, "BOOKTYPE")
	canLearn = chargen or Spellbook.IsSorcererBook (BookType) # bard / sorcerer
	if SpellTableName == "*" or not canLearn:
		if chargen:
			GemRB.SetNextScript ("CharGen7")
		else:
			import GUIREC
			GUIREC.FinishLevelUp ()
		return

	SpellBookType = CommonTables.ClassSkills.GetValue (ClassName, "SPLTYPE")
	LUSpellSelection.OpenSpellsWindow (MyChar, SpellTableName, Level+LevelDiff, LevelDiff, KitValue, chargen, True, SpellBookType)
