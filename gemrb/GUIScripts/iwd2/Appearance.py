# SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

#character generation, color (GUICG13)
import GemRB

import CharOverview
import PaperDoll
import Portrait
from GUIDefines import *
from ie_stats import IE_SEX

ColorWindow = None
stats = None
PaperDoll.StanceAnim = "G11"

def OnLoad():
	global ColorWindow, stats

	pc = GemRB.GetVar ("Slot")

	#set these colors to some default
	Gender = GemRB.GetPlayerStat (pc, IE_SEX)
	Portrait.Init (Gender)
	Portrait.Set (GemRB.GetPlayerPortrait (pc)["ResRef"])
	PortraitName = Portrait.Name () # strips the last char like the table needs

	stats = PaperDoll.ColorStatsFromPortrait (PortraitName)
	ColorWindow = PaperDoll.OpenPaperDollWindow (pc, "GUICG", stats)
	CharOverview.PositionCharGenWin (ColorWindow)

	BackButton = ColorWindow.GetControl(13)
	BackButton.OnPress (BackPress)
	DoneButton = ColorWindow.GetControl(0)
	DoneButton.OnPress (NextPress)

def BackPress():
	ColorWindow.Close ()
	GemRB.SetNextScript("CharGen7")
	return

def NextPress():
	ColorWindow.Close ()
	pc = GemRB.GetVar ("Slot")
	PaperDoll.SaveStats (stats, pc)
	GemRB.SetNextScript("CSound") #character sound
	return
