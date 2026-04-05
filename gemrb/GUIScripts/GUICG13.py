# SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

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
	stats = PaperDoll.ColorStatsFromPortrait (PortraitName)
	pc = GemRB.GetVar("Slot")
	ColorWindow = PaperDoll.OpenPaperDollWindow (pc, "GUICG", stats)
	if GameCheck.IsBG2 ():
		CharGenCommon.PositionCharGenWin (ColorWindow, -6)

	def BackPress():
		if GameCheck.IsBG1 ():
			return CharGenCommon.back (ColorWindow)
		else:
			ColorWindow.Close ()
		GemRB.SetNextScript ("CharGen7")
		return

	BackButton = ColorWindow.GetControl(13)
	BackButton.SetText(15416)
	BackButton.OnPress (BackPress)
	BackButton.MakeEscape()

	def NextPress():
		ColorWindow.Close()

		PaperDoll.SaveStats (stats, pc)

		if GameCheck.IsBG1 ():
			CharGenCommon.next ()
		else:
			GemRB.SetNextScript ("GUICG19") # sounds
		return

	DoneButton = ColorWindow.GetControl (0)
	DoneButton.SetText (11973)
	DoneButton.OnPress (NextPress)
