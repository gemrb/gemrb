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
import CharGenCommon
import GameCheck
import PaperDoll

from GUIDefines import *
from ie_stats import *

def OnLoad():
	PortraitName = GemRB.GetToken("LargePortrait")
	PortraitName = PortraitName[0:len(PortraitName)-1]

	stats = PaperDoll.ColorStatsFromPortrait(PortraitName)
	pc = GemRB.GetVar("Slot")
	ColorWindow = PaperDoll.OpenPaperDollWindow(pc, "GUICG", stats)
	if GameCheck.IsBG2 ():
		CharGenCommon.PositionCharGenWin (ColorWindow, -6)

	def BackPress():
		if GameCheck.IsBG1():
			return CharGenCommon.back(ColorWindow)
		else:
			ColorWindow.Close()

		GemRB.SetNextScript("CharGen7")
		return
		
	BackButton = ColorWindow.GetControl(13)
	BackButton.SetText(15416)
	BackButton.OnPress(BackPress)

	def NextPress():
		ColorWindow.Close()
		
		PaperDoll.SaveStats(stats, pc)

		if GameCheck.IsBG1():
			CharGenCommon.next()
		else:
			GemRB.SetNextScript("GUICG19") #sounds
		return

	DoneButton = ColorWindow.GetControl(0)
	DoneButton.SetText(11973)
	DoneButton.OnPress(NextPress)
