import GemRB
from GUIDefines import GS_PARTYAI

def OnLoad():
	# skipping chargen
	GemRB.CreatePlayer ("protagon", 1|0x8000) # exportable
	GemRB.SetPlayerName (1, "Seeker", 0)

	GemRB.GameSetScreenFlags (GS_PARTYAI, OP_OR)
	GemRB.EnterGame()
