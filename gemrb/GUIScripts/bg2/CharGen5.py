#character generation (GUICG 0)
import GemRB
from CharGenCommon import *

def OnLoad():
	for i in range(-1,6):
		GemRB.SetVar("Ability "+str(i),0) #abilities

	DisplayOverview (5)

	return
