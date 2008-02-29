#character generation (GUICG 0)
import GemRB
from CharGenCommon import *

def OnLoad():
	GemRB.SetVar("Class",0) #class
	GemRB.SetVar("Class Kit",0) #class kit

	DisplayOverview (3)

	return
