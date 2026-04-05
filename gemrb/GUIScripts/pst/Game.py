# SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

###################################################

import GemRB
import MessageWindow
from GameCheck import MAX_PARTY_SIZE
from ie_stats import IE_SPECIFIC, IE_MORALE, IE_LEVELFIGHTER

def EnterGame():
	GemRB.GameSetPartySize(MAX_PARTY_SIZE)
	GemRB.GameSetProtagonistMode(0)

	MessageWindow.OnLoad()

# used for Dak'kon's blade upgrades
def CheckKarachUpgrade(pc, moraleDiff, levelDiff):
	if GemRB.GetPlayerStat (pc, IE_SPECIFIC) != 7:
		return False

	prefix = None
	suffix = None
	# detect if any of the level or morale thresholds have been crossed
	oldLevel = GemRB.GetPlayerStat (pc, IE_LEVELFIGHTER)
	level = oldLevel + levelDiff
	if levelDiff == 0:
		oldLevel = 0
	# descending order for the unlikely case that he gains enough levels at once to jump two thresholds
	if oldLevel < 10 and level >= 10:
		suffix = "3"
	elif oldLevel < 7 and level >= 7:
		suffix = "2"
	elif oldLevel < 4 and level >= 4:
		suffix = ""
	else:
		# too low level
		return False

	morale = GemRB.GetPlayerStat (pc, IE_MORALE)
	oldMorale = morale - moraleDiff
	if moraleDiff == 0:
		oldMorale = 0
	if oldMorale < 15 and morale >= 15:
		prefix = "g"
	elif oldMorale < 6 and morale >= 6:
		prefix = "n"
	elif oldMorale >= 6 and morale < 6:
		prefix = "b"
	elif oldMorale >= 15 and morale < 15:
		prefix = "n"
	else: # just for the one unhandled case if the morale didn't change
		prefix = "b"

	if not prefix and not suffix:
		return False

	blade = prefix + "karach" + suffix
	GemRB.CreateItem (pc, blade, 10) # fist slot, karach is his default weapon

	if GemRB.Roll (1, 2, 0) == 1:
		GemRB.PlaySound ("dak082")
	else:
		GemRB.PlaySound ("dak083")
	return True
