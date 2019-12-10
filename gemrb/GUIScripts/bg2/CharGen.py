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
import CharGenCommon
import GUICommonWindows

def OnLoad():
	GemRB.SetVar("Gender",0) #gender
	GemRB.SetVar("Race",0) #race
	GemRB.SetVar("Class",0) #class
	GemRB.SetVar("Class Kit",0) #class
	GemRB.SetVar("Alignment",-1) #alignment

	GUICommonWindows.PortraitWindow = None

	MyChar = GemRB.GetVar ("Slot")
	GemRB.CreatePlayer ("charbase", MyChar | 0x8000, 0, 11 ) # 11 = force bg2
	GemRB.SetVar ("ImportedChar", 0)
	CharGenCommon.DisplayOverview (1)

	return
