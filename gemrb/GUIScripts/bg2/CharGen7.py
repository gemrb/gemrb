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

	# grant ranger and bard skills
	ClassSkillsTable = GemRB.LoadTableObject ("clskills")
	ClassTable = GemRB.LoadTableObject ("classes")
	Class = GemRB.GetPlayerStat (MyChar, IE_CLASS)
	ClassName = ClassTable.GetRowName (ClassTable.FindValue (5, Class))
	RangerSkills = ClassSkillsTable.GetValue (ClassName, "RANGERSKILL")
	BardSkills = ClassSkillsTable.GetValue (ClassName, "BARDSKILL")

	Level = 1
	for skills in RangerSkills, BardSkills:
		if skills == "*":
			continue
		SpecialSkillsTable = GemRB.LoadTableObject (skills)
		for skill in range(SpecialSkillsTable.GetColumnCount ()):
			skillname = SpecialSkillsTable.GetColumnName(skill)
			value = SpecialSkillsTable.GetValue (str(Level), skillname)
			StatID = SkillTable.GetValue (skillname, "ID")
			# setting it to value is enough, but this is more modder friendly
			GemRB.SetPlayerStat (MyChar, StatID, value + GemRB.GetPlayerStat (MyChar, StatID, 1))

	DisplayOverview (7)

	return
