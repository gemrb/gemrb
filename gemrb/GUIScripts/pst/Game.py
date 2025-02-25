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
from ie_stats import IE_SPECIFIC, IE_MORALE, IE_LEVELFIGHTER

def EnterGame():
	GemRB.GameSetPartySize(MAX_PARTY_SIZE)
	GemRB.GameSetProtagonistMode(0)

	MessageWindow.OnLoad()

# used for Dak'kon's blade upgrades
def CheckKarachUpgrade(pc, moraleDiff, levelDiff):
	if GemRB.GetPlayerStat (pc, IE_SPECIFIC) != 7:
		return

	prefix = None
	suffix = None
	# detect if any of the level or morale thresholds have been crossed
	level = GemRB.GetPlayerStat (pc, IE_LEVELFIGHTER)
	oldLevel = level - levelDiff
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
		return

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
		return

	blade = prefix + "karach" + suffix
	GemRB.CreateItem (pc, blade, 10) # fist slot, karach is his default weapon

	if GemRB.Roll (1, 2, 0) == 1:
		GemRB.PlaySound ("dak082")
	else:
		GemRB.PlaySound ("dak083")
	return
