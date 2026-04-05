# SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

###################################################

import GemRB
import GameCheck
import GUICommon
import MessageWindow
from ie_restype import RES_2DA

def EnterGame():
	GemRB.GameSetPartySize (GameCheck.MAX_PARTY_SIZE)
	GemRB.GameSetProtagonistMode(1)
	GUICommon.SetSaveDir ()

	MessageWindow.OnLoad()

	# reset gold, so a message is printed
	if GemRB.GetGameTime () <= 2103:
		GemRB.SetTimedEvent (lambda: GemRB.GameSetPartyGold (GemRB.GameGetPartyGold (), 1), 1)

#upgrade savegame to next version
#this is not necessary if TotSC was already installed before starting the game
def GameExpansion():
	if GameCheck.HasTOTSC ():
		#reload world map if it doesn't have AR1000
		GemRB.UpdateWorldMap("WORLDMAP", "AR1000")
