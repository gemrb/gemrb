# GemRB - Infinity Engine Emulator
# Copyright (C) 2003 The GemRB Project
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
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
#
#
#Single Player Party Select
import GemRB
import IDLUCommon
from GameCheck import MAX_PARTY_SIZE

def OnLoad():
	LoadPartyCharacters()
	GemRB.SetNextScript("SPPartyFormation")
	return	
	
#loading characters from party.ini
def LoadPartyCharacters():
	i = GemRB.GetVar ("PartyIdx") + GemRB.GetVar ("TopIndex")
	Tag = "Party " + str(i)
	for j in range(1, min(6, MAX_PARTY_SIZE)+1):
		Key = "Char"+str(j)
		CharName = GemRB.GetINIPartyKey(Tag, Key, "")
		if CharName !="":
			GemRB.CreatePlayer(CharName, j, 1)
			# ensure they get all their feat spells
			# eg. Mordakai of Thay doesn't get his spell focus until
			# the next level up with feats otherwise
			IDLUCommon.LearnFeatInnates (j, j, True)
	return
