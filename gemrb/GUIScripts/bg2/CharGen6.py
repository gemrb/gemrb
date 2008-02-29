#character generation (GUICG 0)
import GemRB
from CharGenCommon import *

def OnLoad():
	for i in range(0,6):
		GemRB.SetVar("Skill "+str(i),0) #thieving skills
	for i in range(0,32):
		GemRB.SetVar("Prof "+str(i),0)  #proficiencies

	DisplayOverview (6)

	return
