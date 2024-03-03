# GemRB - Infinity Engine Emulator
# Copyright (C) 2010 The GemRB Project
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
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#
# a library of any functions that are common to bg1 and bg2, but not useful for others

import GemRB
import CommonTables
import GUICommon
from ie_stats import IE_ALIGNMENT, IE_RACE, IE_REPUTATION, IE_SEX

def RefreshPDoll(button, MinorColor, MajorColor, SkinColor, HairColor):
	MyChar = GemRB.GetVar ("Slot")
	AnimID = 0x6000
	table = GemRB.LoadTable ("avprefr")
	Race = GemRB.GetPlayerStat (MyChar, IE_RACE)
	AnimID = AnimID + table.GetValue(Race, 0)
	table = GemRB.LoadTable ("avprefc")
	ClassName = GUICommon.GetClassRowName (MyChar)
	AnimID = AnimID + table.GetValue (ClassName, "CLASS")
	table = GemRB.LoadTable ("avprefg")
	Gender = GemRB.GetPlayerStat (MyChar, IE_SEX)
	AnimID = AnimID + table.GetValue (Gender, 0)
	ResRef = CommonTables.Pdolls.GetValue (hex(AnimID), "LEVEL1")

	button.SetPLT (ResRef, 0, MinorColor, MajorColor, SkinColor, 0, 0, HairColor, 0)
	return

def SetReputation ():
	MyChar = GemRB.GetVar ("Slot")

	# If Rep > 0 then MyChar is an imported character with a saved reputation,
	# in which case the reputation shouldn't be overwritten.
	Rep = GemRB.GetPlayerStat (MyChar, IE_REPUTATION)

	if Rep <= 0:
		# use the alignment to apply starting reputation
		Alignment = GemRB.GetPlayerStat (MyChar, IE_ALIGNMENT)
		AlignmentAbbrev = CommonTables.Aligns.FindValue (3, Alignment)
		RepTable = GemRB.LoadTable ("repstart")
		Rep = RepTable.GetValue (AlignmentAbbrev, 0) * 10
		GemRB.SetPlayerStat (MyChar, IE_REPUTATION, Rep)

	# set the party rep if this is the main char
	if MyChar == 1:
		GemRB.GameSetReputation (Rep)
