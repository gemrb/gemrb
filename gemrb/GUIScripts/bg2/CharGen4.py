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
# $Id$
# character generation - classes+kits; next alignment/reputation(CharGen4.py)
import GemRB
from CharGenCommon import *
from GUICommonWindows import *
from LevelUp import GetNextLevelFromExp

def OnLoad():
	MyChar = GemRB.GetVar ("Slot")
	GemRB.SetVar ("Alignment", -1) #alignment
	GemRB.SetPlayerStat (MyChar, IE_ALIGNMENT, 0)
	GemRB.SetPlayerStat (MyChar, IE_REPUTATION, 0)

	# save new stats
	#	class
	#	kit
	#	class/kit abilites

	# A little explanation for the different handling of mage kit values:
	# Originally, the IE had only mage schools, and the kit field
	# was simply an unusability field (with a single bit set)
	# then BG2 crammed in a lot more kits, and 32 bits were not enough.
	# They solved this by making the generalist value 0x4000 to hold
	# the kit index in the lower portions.
	# When you see 0x4000 in a kit value, you need to translate
	# the kit index to unusability value, using the kitlist
	# So, for mages, the kit equals to the unusability value
	# but for others there is an additional mapping by kitlist.2da

	# find the class from the class table
	ClassIndex = GemRB.GetVar ("Class") - 1
	Class = ClassTable.GetValue (ClassIndex, 5)
	#protect against barbarians
	ClassName = ClassTable.GetRowName (ClassTable.FindValue (5, Class) )
	GemRB.SetPlayerStat (MyChar, IE_CLASS, Class)
	
	# save the kit
	KitIndex = GemRB.GetVar ("Class Kit")
	KitValue = (0x4000 + KitIndex)
	KitName = KitListTable.GetValue (KitIndex, 0)
	GemRB.SetPlayerStat (MyChar, IE_KIT, KitValue)

	# no table for innates, so we make it a high value
	# to guarantee innate, class ability and HLA memorization works
	GemRB.SetMemorizableSpellsCount (MyChar, 50, IE_SPELL_TYPE_INNATE, 0)

	#assign the correct XP
	if GameIsTOB():
		GemRB.SetPlayerStat (MyChar, IE_XP, ClassSkillsTable.GetValue (ClassName, "STARTXP2"))
	else:
		GemRB.SetPlayerStat (MyChar, IE_XP, ClassSkillsTable.GetValue (ClassName, "STARTXP"))

	#create an array to get all the classes from
	NumClasses = 1
	IsMulti = IsMultiClassed (MyChar, 1)
	if IsMulti[0] > 1:
		NumClasses = IsMulti[0]
		Classes = [IsMulti[1], IsMulti[2], IsMulti[3]]
	else:
		Classes = [GemRB.GetPlayerStat (MyChar, IE_CLASS)]

	#loop through each class and update it's level
	xp = GemRB.GetPlayerStat (MyChar, IE_XP)/NumClasses
	for i in range (NumClasses):
		CurrentLevel = GetNextLevelFromExp (xp, Classes[i])
		if i == 0:
			GemRB.SetPlayerStat (MyChar, IE_LEVEL, CurrentLevel)
		elif i <= 2:
			GemRB.SetPlayerStat (MyChar, IE_LEVEL2+i-1, CurrentLevel)

	# diagnostic output
	print "CharGen4 output:"
	print "\tClass: ",Class
	print "\tClass Name: ",ClassName
	print "\tKitValue: ",KitValue
	print "\tKitName: ",KitName

	DisplayOverview (4)

	return
