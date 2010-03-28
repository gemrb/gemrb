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
#character generation - alignment; next abilities (GUICG 0)
import GemRB
from CharGenCommon import *

def OnLoad():
	MyChar = GemRB.GetVar ("Slot")
	AbilityTable = GemRB.LoadTableObject ("ability")
	AbilityCount = AbilityTable.GetRowCount ()
	
	# set all our abilites to zero
	GemRB.SetVar ("Ability -1", 0)
	GemRB.SetVar ("StrExtra", 0)
	GemRB.SetPlayerStat (MyChar, IE_STREXTRA, 0)
	for i in range(AbilityCount):
		GemRB.SetVar ("Ability "+str(i), 0)
		StatID = AbilityTable.GetValue (i, 3)
		GemRB.SetPlayerStat (MyChar, StatID, 0)

	DisplayOverview (5)

	return
