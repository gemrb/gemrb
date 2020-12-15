import GemRB
from GUIDefines import GS_PARTYAI

def OnLoad():
	# skipping chargen
	GemRB.CreatePlayer ("protagon", 1|0x8000) # exportable
	GemRB.SetPlayerName (1, "Seeker", 0)

	# setup character colors (IE_MAJOR_COLOR and so on)
	statcolors = [ (211, 202116108), (214, 0), (210, 791621423), (209, 960051513), (208, 454761243), (212, 370546198),(213, 387389207) ]
	for (stat, color) in statcolors:
		GemRB.SetPlayerStat (1, stat, color)

	GemRB.GameSetScreenFlags (GS_PARTYAI, OP_OR)
	GemRB.EnterGame()
