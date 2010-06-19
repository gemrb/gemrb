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
# character generation - race; next class/kit (CharGen3)
import GemRB
import GUICommon
from CharGenCommon import *

def OnLoad():
	MyChar = GemRB.GetVar ("Slot")
	GemRB.SetVar("Class",0) #class
	GemRB.SetVar("Class Kit",0) #class kit
	GemRB.SetPlayerStat (MyChar, IE_CLASS, 0)
	GemRB.SetPlayerStat (MyChar, IE_KIT, 0)

	#reset all the levels (assigned in CharGen4)
	GemRB.SetPlayerStat (MyChar, IE_LEVEL, 0)
	GemRB.SetPlayerStat (MyChar, IE_LEVEL2, 0)
	GemRB.SetPlayerStat (MyChar, IE_LEVEL3, 0)

	DisplayOverview (3)

	return
