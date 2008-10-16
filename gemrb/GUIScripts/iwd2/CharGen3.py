#character generation (GUICG 0)
import GemRB
from CharOverview import *

def OnLoad():
	MyChar = GemRB.GetVar("Slot")
	GemRB.SetPlayerStat (MyChar, IE_SEX, GemRB.GetVar ("Gender") )
	GemRB.SetPlayerStat (MyChar, IE_RACE, GemRB.GetVar ("BaseRace") )
	race = GemRB.GetVar ("Race")
	GemRB.SetPlayerStat (MyChar, IE_SUBRACE, race & 255 )
	UpdateOverview(3)
	return
