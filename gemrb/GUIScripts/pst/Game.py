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
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#

###################################################

import GemRB
import MessageWindow
from GameCheck import MAX_PARTY_SIZE

def EnterGame():
	GemRB.GameSetPartySize(MAX_PARTY_SIZE)
	GemRB.GameSetProtagonistMode(0)
	GemRB.SetInfoTextColor({'r' : 0, 'g' : 255, 'b' : 255, 'a' : 255})

	MessageWindow.OnLoad()
