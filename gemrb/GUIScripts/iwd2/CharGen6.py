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

def OnLoad():
	#setting the stats so the feat code will work
	MyChar = GemRB.GetVar("Slot")
	TmpTable = GemRB.LoadTable ("ability")
	AbilityCount = TmpTable.GetRowCount ()
	for i in range (AbilityCount):
		StatID=TmpTable.GetValue (i,3,2)
		GemRB.SetPlayerStat (MyChar, StatID, GemRB.GetVar ("Ability "+str(i) ) )
		print "StatID:", StatID, " ", GemRB.GetPlayerStat(MyChar, StatID)

	CharOverview.UpdateOverview(6)
	return
