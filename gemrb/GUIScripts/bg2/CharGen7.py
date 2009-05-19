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
#character generation - skills/profs/spells; next apearance/sound (CharGen7)
import GemRB
from CharGenCommon import *

def OnLoad():
	MyChar = GemRB.GetVar ("Slot")

	# we're gonna output diagnostics as we go
	print "CharGen7 output:"

	# save the hated race
	GemRB.SetPlayerStat (MyChar, IE_HATEDRACE, GemRB.GetVar ("HatedRace"))
	print "\tHated Race: ",GemRB.GetVar ("HatedRace")

	# save all skills
	SkillTable = GemRB.LoadTableObject ("skills")
	SkillCount = SkillTable.GetRowCount () - 2
	for i in range(SkillCount):
		StatID = SkillTable.GetValue (i+2, 2)
		Value = GemRB.GetVar ("Skill "+str(i))
		GemRB.SetPlayerStat (MyChar, StatID, Value)
		print "\tSkill ",str(i),": ",Value

	# weapon proficiencies
	# set the base number of attacks; effects will add the proficiency bonus
	GemRB.SetPlayerStat (MyChar, IE_NUMBEROFATTACKS, 2)
	ProfsTable = GemRB.LoadTableObject ("weapprof")
	ProfsCount = ProfsTable.GetRowCount () - 8 # bg2 weapprof.2da contains the bg1 proficiencies too, skipping those
	for i in range(ProfsCount):
		StatID = ProfsTable.GetValue (i+8, 0)
		Value = GemRB.GetVar ("Prof "+str(i))
		if Value:
			GemRB.ApplyEffect (MyChar, "Proficiency", Value, StatID)
			print "\tProf ",str(i),": ",Value

	DisplayOverview (7)

	return
