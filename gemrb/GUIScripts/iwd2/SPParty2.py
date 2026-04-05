# SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

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
