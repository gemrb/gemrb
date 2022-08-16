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
#character generation, color (GUICG13)
import GemRB

import CharOverview
import PaperDoll
import Portrait

from GUIDefines import *
from ie_stats import IE_SEX

def OnLoad():
	pc = GemRB.GetVar("Slot")
	#set these colors to some default
	Gender = GemRB.GetPlayerStat (pc, IE_SEX)
	Portrait.Init (Gender)
	Portrait.Set (GemRB.GetPlayerPortrait (pc)["ResRef"])
	PortraitName = Portrait.Name () # strips the last char like the table needs

	stats = PaperDoll.ColorStatsFromPortrait(PortraitName)
	ColorWindow = PaperDoll.OpenPaperDollWindow(pc, "GUICG", stats)
	CharOverview.PositionCharGenWin(ColorWindow)

	BackButton = ColorWindow.GetControl(13)
	DoneButton = ColorWindow.GetControl(0)
	
	def BackPress():
		ColorWindow.Close ()
		GemRB.SetNextScript("CharGen7")
		return

	def NextPress():
		ColorWindow.Close ()
		
		PaperDoll.SaveStats(stats, pc)

		GemRB.SetNextScript("CSound") #character sound
		return

	DoneButton.OnPress (NextPress)
	BackButton.OnPress (BackPress)

	return
