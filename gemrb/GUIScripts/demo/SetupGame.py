import GemRB
from GUIDefines import GS_PARTYAI
from ie_stats import IE_STR

def OnLoad():
	# skipping chargen
	GemRB.CreatePlayer ("protagon", 1|0x8000) # exportable
	GemRB.SetPlayerName (1, "Seeker", 0)
	# set strength, so items can be picked up (currently needed for encumbrance labels)
	GemRB.SetPlayerStat (1, IE_STR, 14)

	GemRB.GameSetScreenFlags (GS_PARTYAI, OP_OR)
	GemRB.EnterGame()
