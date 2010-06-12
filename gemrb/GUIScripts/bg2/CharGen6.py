# -*-python-*-
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
from CharGenCommon import *
from GUICommonWindows import *
from LUSkillsSelection import *
from LUProfsSelection import *
from GUICommon import LearnPriestSpells

def OnLoad():
	MyChar = GemRB.GetVar ("Slot")

	# nullify our thieving skills
	SkillsNullify ()
	SkillsSave (MyChar)

	# nullify our proficiencies
	ProfsNullify ()

	# nully other variables
	GemRB.SetVar ("HatedRace", 0)

	# save our previous stats:
	# 	abilities
	AbilityTable = GemRB.LoadTable ("ability")
	AbilityCount = AbilityTable.GetRowCount ()

	# print our diagnostic as we loop (so as not to duplicate)
	print "CharGen6 output:"

	#remove all known spells and nullify the memorizable counts
	RemoveKnownSpells (MyChar, IE_SPELL_TYPE_WIZARD, 1,9, 1)
	RemoveKnownSpells (MyChar, IE_SPELL_TYPE_PRIEST, 1,7, 1)

	# learn divine spells if appropriate
	Class = GemRB.GetPlayerStat (MyChar, IE_CLASS)
	TableName = ClassSkillsTable.GetValue (Class, 1, 0) # cleric spells

	if TableName == "*": # only druid spells or no spells at all
		TableName = ClassSkillsTable.GetValue (Class, 0, 0)
		ClassFlag = 0x8000
	elif ClassSkillsTable.GetValue (Class, 0, 0) != "*": # cleric and druid spells
		ClassFlag = 0
	else: # only cleric spells
		ClassFlag = 0x4000

	if TableName != "*":
		#figure out which class casts spells and use the level of the class
		#to setup the priest spells
		Levels = [GemRB.GetPlayerStat (MyChar, IE_LEVEL), \
				GemRB.GetPlayerStat (MyChar, IE_LEVEL2), \
				GemRB.GetPlayerStat (MyChar, IE_LEVEL3)]
		index = 0
		IsMulti = IsMultiClassed (MyChar, 1)
		if IsMulti[0]>1:
			#loop through each multiclass until we come across the class that gives
			#divine spells; because clerics have a lower id than rangers, they should
			#be looked at first in Cleric/Ranger multi's, which is correct
			foundindex = 0
			for i in range (IsMulti[0]):
				ClassName = ClassTable.GetRowName (ClassTable.FindValue (5, IsMulti[i+1]) )
				for table in "CLERICSPELL", "DRUIDSPELL":
					if ClassSkillsTable.GetValue (ClassName, table) != "*":
						index = i
						foundindex = 1
						break
				if foundindex:
					break

		#set our memorizable counts
		SetupSpellLevels (MyChar, TableName, IE_SPELL_TYPE_PRIEST, Levels[index])

		#learn all our priest spells up to the level we can learn
		for level in range (8):
			if GemRB.GetMemorizableSpellsCount (MyChar, IE_SPELL_TYPE_PRIEST, level, 1) <= 0:
				LearnPriestSpells (MyChar, level, ClassFlag)
				break
	DisplayOverview (6)
	return
