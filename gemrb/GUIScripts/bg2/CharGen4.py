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
# character generation - classes+kits; next alignment/reputation(CharGen4.py)
import GemRB
import CharGenCommon
from ie_stats import IE_ALIGNMENT, IE_REPUTATION

def OnLoad():
	MyChar = GemRB.GetVar ("Slot")
	GemRB.SetVar ("Alignment", -1) #alignment
	GemRB.SetPlayerStat (MyChar, IE_ALIGNMENT, 0)
	GemRB.SetPlayerStat (MyChar, IE_REPUTATION, 0)

	CharGenCommon.DisplayOverview (4)

	return
