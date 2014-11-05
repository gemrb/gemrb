# GemRB - Infinity Engine Emulator
# Copyright (C) 2003 The GemRB Project
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
#character generation (GUICG 0)
import GemRB
from ie_stats import *
import CharOverview
import IDLUCommon

#this is the same list as in GUIREC
#barbarian, bard, cleric, druid, fighter, monk, paladin, ranger, rogue, sorcerer, wizard
Classes = IDLUCommon.Levels

def OnLoad():
	MyChar = GemRB.GetVar ("Slot")
	#base class
	Class=GemRB.GetVar ("BaseClass")
	GemRB.SetPlayerStat (MyChar, IE_CLASS, Class)
	#kit
	GemRB.SetPlayerStat (MyChar, IE_KIT, GemRB.GetVar ("Class") )

	#works only for the first level character generation
	#if this code ever needs to be more versatile, consider saving the
	#class values somewhere
	for i in range(len(Classes)):
		GemRB.SetPlayerStat (MyChar, Classes[i], 0)

	GemRB.SetPlayerStat (MyChar, Classes[Class-1], 1)
	print "Set class stat ",Classes[Class-1], " to 1"
	CharOverview.UpdateOverview(4)
	return
