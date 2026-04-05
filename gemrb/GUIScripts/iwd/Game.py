# SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

# MessageWindow.py - scripts and GUI for main (walk) window

###################################################

import GemRB
import GUICommon
import MessageWindow
from GameCheck import MAX_PARTY_SIZE

def EnterGame():
	GemRB.GameSetPartySize(MAX_PARTY_SIZE)
	GemRB.GameSetProtagonistMode (2)
	GUICommon.SetSaveDir ()

	MessageWindow.OnLoad()
	GemRB.GamePause (0, 0)

#upgrade savegame to next version
def GameExpansion():
	#the original savegames got 0, but the engine upgrades all saves to 3
	#this is a good place to perform one-time adjustments if needed
	GemRB.GameSetExpansion(3)
	return

