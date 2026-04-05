# SPDX-FileCopyrightText: 2020 Contributors to the GemRB project <https://gemrb.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

# MessageWindow.py - scripts and GUI for main (walk) window

###################################################

import GemRB

def EnterGame():
	from GameCheck import MAX_PARTY_SIZE
	GemRB.GameSetPartySize(MAX_PARTY_SIZE)
	GemRB.GameSetProtagonistMode(1)

	import MessageWindow
	MessageWindow.OnLoad()
	GemRB.GamePause (0, 0)

def GameExpansion():
	pass
