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
# character generation - race; next class/kit (CharGen3)
import GemRB
from CharGenCommon import *

def OnLoad():
	MyChar = GemRB.GetVar ("Slot")
	GemRB.SetVar("Class",0) #class
	GemRB.SetVar("Class Kit",0) #class kit
	GemRB.SetPlayerStat (MyChar, IE_CLASS, 0)
	GemRB.SetPlayerStat (MyChar, IE_KIT, 0)

	# set new stats:
	#	race
	RaceTable = GemRB.LoadTableObject ("races")
	Race = GemRB.GetVar ("Race") - 1
	GemRB.SetPlayerStat (MyChar, IE_RACE, RaceTable.GetValue (Race, 3))

	# diagnostic output
	print "CharGen3 output:"
	print "\tRace: ",Race
	print "\tRace Name: ",RaceTable.GetRowName (Race)

	DisplayOverview (3)

	return
