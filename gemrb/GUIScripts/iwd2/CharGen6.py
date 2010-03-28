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
from CharOverview import *

def OnLoad():
	#setting the stats so the feat code will work
	MyChar = GemRB.GetVar("Slot")
	GemRB.SetPlayerStat (MyChar, IE_STR, GemRB.GetVar ("Ability 1"))
	GemRB.SetPlayerStat (MyChar, IE_DEX, GemRB.GetVar ("Ability 2"))
	GemRB.SetPlayerStat (MyChar, IE_CON, GemRB.GetVar ("Ability 3"))
	GemRB.SetPlayerStat (MyChar, IE_INT, GemRB.GetVar ("Ability 4"))
	GemRB.SetPlayerStat (MyChar, IE_WIS, GemRB.GetVar ("Ability 5"))
	GemRB.SetPlayerStat (MyChar, IE_CHR, GemRB.GetVar ("Ability 6"))
	UpdateOverview(6)
	return
