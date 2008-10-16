#character generation (GUICG 0)
import GemRB
from CharOverview import *

def OnLoad():
	#setting the stats so the feat code will work
	MyChar = GemRB.GetVar("Slot")
	GemRB.SetPlayerStat (MyChar, IE_STR, GemRB.GetVar ("Ability 1"))
	GemRB.SetPlayerStat (MyChar, IE_DEX, GemRB.GetVar ("Ability 2"))
	GemRB.SetPlayerStat (MyChar, IE_CON, GemRB.GetVar ("Ability 3"))
	GemRB.SetPlayerStat (MyChar, IE_INT, GemRB.GetVar ("Ability 4"))
	GemRB.SetPlayerStat (MyChar, IE_WIS, GemRB.GetVar ("Ability 5"))
	GemRB.SetPlayerStat (MyChar, IE_CHR, GemRB.GetVar ("Ability 6"))
	UpdateOverview(6)
	return
