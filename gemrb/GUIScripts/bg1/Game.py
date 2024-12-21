# GemRB - Infinity Engine Emulator
# Copyright (C) 2003-2005 The GemRB Project
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

###################################################

import GemRB
import GUICommon
import MessageWindow
from ie_restype import RES_2DA
from GameCheck import MAX_PARTY_SIZE

def EnterGame():
	GemRB.GameSetPartySize(MAX_PARTY_SIZE)
	GemRB.GameSetProtagonistMode(1)
	GUICommon.SetSaveDir ()

	MessageWindow.OnLoad()

	# reset gold, so a message is printed
	if GemRB.GetGameTime () <= 2103:
		GemRB.SetTimedEvent (lambda: GemRB.GameSetPartyGold (GemRB.GameGetPartyGold (), 1), 1)

#upgrade savegame to next version
#this is not necessary if TotSC was already installed before starting the game
def GameExpansion():
	if HasTOTSC():
		#reload world map if it doesn't have AR1000
		GemRB.UpdateWorldMap("WORLDMAP", "AR1000")

def HasTOTSC ():
	return GemRB.HasResource ("toscst", RES_2DA)
