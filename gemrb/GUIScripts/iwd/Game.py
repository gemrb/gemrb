# -*-python-*-
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
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#

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

