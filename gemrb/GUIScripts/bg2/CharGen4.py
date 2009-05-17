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
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
#
# $Id$
# character generation - classes+kits; next alignment/reputation(CharGen4.py)
import GemRB
from CharGenCommon import *
from GUICommon import AddClassAbilities

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
	ClassTable = GemRB.LoadTableObject ("classes")
	ClassIndex = GemRB.GetVar ("Class") - 1
	Class = ClassTable.GetValue (ClassIndex, 5)
	ClassName = ClassTable.GetRowName (ClassIndex)
	GemRB.SetPlayerStat (MyChar, IE_CLASS, Class)
	
	# save the kit
	KitList = GemRB.LoadTableObject ("kitlist")
	KitIndex = GemRB.GetVar ("Class Kit")
	KitValue = (0x4000 + KitIndex)
	KitName = KitList.GetValue (KitIndex, 0)
	GemRB.SetPlayerStat (MyChar, IE_KIT, KitValue)

	# save the class/kit abilites
	ClassSkillsTable = GemRB.LoadTableObject ("clskills")

	# no table for innates, so we make it a high value
	# to guarantee innate, class ability and HLA memorization works
	GemRB.SetMemorizableSpellsCount (MyChar, 50, IE_SPELL_TYPE_INNATE, 0)

	# apply class/kit abilities
	if KitIndex:
		ABTable = KitList.GetValue (str(KitIndex), "ABILITIES")
	else:
		ABTable = ClassSkillsTable.GetValue (ClassName, "ABILITIES")
	if not KitIndex and "," in ABTable:
		# multiclass
		classes = ABTable.split(",")
		for j in classes:
			AddClassAbilities (MyChar, "CLAB"+j)
	else:
		if ABTable != "*" and ABTable[:6] != "CLABMA": # mage kits specify ability tables which don't exist
			AddClassAbilities (MyChar, ABTable)
	

	# diagnostic output
	print "CharGen4 output:"
	print "\tClass: ",Class
	print "\tClass Name: ",ClassName
	print "\tKitValue: ",KitValue
	print "\tKitName: ",KitName
	print "\tABTable: ",ABTable

	DisplayOverview (4)

	return
