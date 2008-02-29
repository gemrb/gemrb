#character generation (GUICG 0)
import GemRB
from CharGenCommon import *

CharGenWindow = 0

def OnLoad():
	global CharGenWindow

	#this is tob specific code!
	if GemRB.GetVar("oldgame")==0:
		GemRB.GameSetExpansion(1)
		GemRB.SetVar ("PlayMode",2)
	else:
		GemRB.GameSetExpansion(0)
		GemRB.SetVar ("PlayMode",0)

	GemRB.SetVar("Gender",0) #gender
	GemRB.SetVar("Race",0) #race
	GemRB.SetVar("Class",0) #class
	GemRB.SetVar("Class Kit",0) #class
	GemRB.SetVar("Alignment",-1) #alignment

	DisplayOverview (1)	

	return
