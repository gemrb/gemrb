#Single Player Party Select
import GemRB

def OnLoad():
	LoadPartyCharacters()
	GemRB.SetNextScript("SPPartyFormation")
	return	
	
#loading characters from party.ini
def LoadPartyCharacters():
	i = GemRB.GetVar("PartyIdx")
	Tag = "Party " + str(i)
	for j in range(1, PARTY_SIZE+1):
		Key = "Char"+str(j)
		CharName = GemRB.GetINIPartyKey(Tag, Key, "")
		if CharName !="":
			GemRB.CreatePlayer(CharName, j, 1)
	return
