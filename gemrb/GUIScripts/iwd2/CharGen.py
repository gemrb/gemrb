#character generation (GUICG 0)
import GemRB
from CharOverview import *

def OnLoad():
	MyChar = GemRB.GetVar ("Slot")
	GemRB.CreatePlayer ("charbase", MyChar | 0x8000 )

	UpdateOverview(1)
	return

