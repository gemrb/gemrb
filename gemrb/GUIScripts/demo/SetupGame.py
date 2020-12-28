import GemRB
import Start
from GUIDefines import GS_PARTYAI
from ie_stats import IE_STR

def OnLoad():
	# skipping chargen
	GemRB.CreatePlayer ("protagon", 1|0x8000) # exportable
	GemRB.SetPlayerName (1, Start.Name, 0)
	# set strength, so items can be picked up (currently needed for encumbrance labels)
	GemRB.SetPlayerStat (1, IE_STR, 14)

	GemRB.GameSetScreenFlags (GS_PARTYAI, OP_OR)
	GemRB.SetVar("CHAPTER", 1)
	GemRB.EnterGame()
