# -*-python-*-

import GemRB
from GUIDefines import *

FloatMenuWindow = None

def OpenFloatMenuWindow ():
	global FloatMenuWindow
        GemRB.HideGUI ()

        if FloatMenuWindow:
		GemRB.UnloadWindow (FloatMenuWindow)
		FloatMenuWindow = None

		GemRB.SetVar ("FloatWindow", -1)
		GemRB.UnhideGUI ()
		return

	FloatMenuWindow = Window = GemRB.LoadWindow (3)
	GemRB.SetVar ("FloatWindow", Window)

	GemRB.UnhideGUI ()
