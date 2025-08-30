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
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#
# character generation - ability; next skills/profs/spells (CharGen6)
import GemRB
import GUICommon
import Spellbook
import CommonTables
import CharGenCommon
import LUSkillsSelection
import LUProfsSelection
from GUIDefines import IE_SPELL_TYPE_PRIEST, IE_SPELL_TYPE_WIZARD, GTV_STR
from ie_stats import *

def OnLoad():
	MyChar = GemRB.GetVar ("Slot")

	# nullify our thieving skills
	LUSkillsSelection.SkillsNullify (MyChar)

	# nullify our proficiencies
	LUProfsSelection.ProfsNullify ()

	# nully other variables
	GemRB.SetVar ("HatedRace", 0)

	#remove all known spells and nullify the memorizable counts
	Spellbook.RemoveKnownSpells (MyChar, IE_SPELL_TYPE_WIZARD, 1,9, 1)
	Spellbook.RemoveKnownSpells (MyChar, IE_SPELL_TYPE_PRIEST, 1,7, 1)

	# mage spells
	ClassName = GUICommon.GetClassRowName (MyChar)
	IsMulti = GUICommon.IsMultiClassed (MyChar, 1)
	Levels = [GemRB.GetPlayerStat (MyChar, IE_LEVEL), GemRB.GetPlayerStat (MyChar, IE_LEVEL2), \
		GemRB.GetPlayerStat (MyChar, IE_LEVEL3)]
	TableName = CommonTables.ClassSkills.GetValue (ClassName, "MAGESPELL", GTV_STR)
	if TableName != "*":
		levelIdx = 0
		if IsMulti[0] > 1:
			# find out which class gets mage spells
			for i in range (IsMulti[0]):
				TmpClassName = GUICommon.GetClassRowName (IsMulti[i + 1], "class")
				if CommonTables.ClassSkills.GetValue (TmpClassName, "MAGESPELL", GTV_STR) != "*":
					levelIdx = i
					break
		Spellbook.SetupSpellLevels(MyChar, TableName, IE_SPELL_TYPE_WIZARD, Levels[levelIdx])

	# learn divine spells if appropriate
	TableName = CommonTables.ClassSkills.GetValue (ClassName, "CLERICSPELL", GTV_STR) # cleric spells
	DruidTable = CommonTables.ClassSkills.GetValue (ClassName, "DRUIDSPELL", GTV_STR)

	if TableName == "*" and DruidTable == "*":
		CharGenCommon.DisplayOverview (6)
		return

	ClassFlag = Spellbook.GetClassFlag (TableName)
	if DruidTable != "*" and TableName == "*": # only druid spells
		ClassFlag = Spellbook.GetClassFlag (DruidTable)
		TableName = Spellbook.GetPriestSpellTable (DruidTable)
	elif DruidTable != "*" and TableName != "*": # cleric and druid spells
		ClassFlag = 0
	else: # only cleric spells
		pass

	if TableName != "*":
		#figure out which class casts spells and use the level of the class
		#to setup the priest spells
		index = 0
		IsMulti = GUICommon.IsMultiClassed (MyChar, 1)
		if IsMulti[0]>1:
			#loop through each multiclass until we come across the class that gives
			#divine spells; because clerics have a lower id than rangers, they should
			#be looked at first in Cleric/Ranger multi's, which is correct
			foundindex = 0
			for i in range (IsMulti[0]):
				ClassName = GUICommon.GetClassRowName (IsMulti[i+1], "class")
				for table in "CLERICSPELL", "DRUIDSPELL":
					if CommonTables.ClassSkills.GetValue (ClassName, table) != "*":
						index = i
						foundindex = 1
						break
				if foundindex:
					break

		#set our memorizable counts
		Spellbook.SetupSpellLevels (MyChar, TableName, IE_SPELL_TYPE_PRIEST, Levels[index])

		#learn all our priest spells up to the level we can learn
		for level in range (8):
			if GemRB.GetMemorizableSpellsCount (MyChar, IE_SPELL_TYPE_PRIEST, level, 0) <= 0:
				Spellbook.LearnPriestSpells (MyChar, level, ClassFlag)
				break
	CharGenCommon.DisplayOverview (6)
	return
