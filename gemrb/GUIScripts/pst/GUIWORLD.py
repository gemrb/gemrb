# -*-python-*-

import GemRB
from GUIDefines import *

FormationWindow = None
ReformPartyWindow = None

def OpenReformPartyWindow ():
	global ReformPartyWindow
        GemRB.HideGUI ()

        if ReformPartyWindow:
		GemRB.UnloadWindow (ReformPartyWindow)
		ReformPartyWindow = None

		GemRB.SetVar ("OtherWindow", -1)
		GemRB.LoadWindowPack ("GUIREC")
		GemRB.UnhideGUI ()
		return

	GemRB.LoadWindowPack ("GUIWORLD")
	ReformPartyWindow = Window = GemRB.LoadWindow (24)
	GemRB.SetVar ("OtherWindow", Window)

	# Remove
	Button = GemRB.GetControl (Window, 15)
	GemRB.SetText (Window, Button, 42514)
	GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_DISABLED)

	# Done
	Button = GemRB.GetControl (Window, 8)
	GemRB.SetText (Window, Button, 1403)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenReformPartyWindow")

	GemRB.UnhideGUI ()


last_formation = None

def OpenFormationWindow ():
	global FormationWindow
        GemRB.HideGUI ()

        if FormationWindow:
		GemRB.UnloadWindow (FormationWindow)
		FormationWindow = None

		GemRB.GameSetFormation (last_formation)
		GemRB.SetVar ("OtherWindow", -1)
		GemRB.UnhideGUI ()
		return

	FormationWindow = Window = GemRB.LoadWindow (27)
	GemRB.SetVar ("OtherWindow", Window)

	# Done
	Button = GemRB.GetControl (Window, 13)
	GemRB.SetText (Window, Button, 1403)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenFormationWindow")

	tooltips = (
		44957,  # Follow
		44958,  # T
		44959,  # Gather
		44960,  # 4 and 2
		44961,  # 3 by 2
		44962,  # Protect
		48152,  # 2 by 3
		44964,  # Rank
		44965,  # V
		44966,  # Wedge
		44967,  # S
		44968,  # Line
		44969,  # None
	)

	for i in range (13):
		Button = GemRB.GetControl (Window, i)
		GemRB.SetVarAssoc (Window, Button, "SelectedFormation", i)
		GemRB.SetTooltip (Window, Button, tooltips[i])
		GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "SelectFormation")

	GemRB.SetVar ("SelectedFormation", GemRB.GameGetFormation ())
	SelectFormation ()

	GemRB.UnhideGUI ()

def SelectFormation ():
	global last_formation
	Window = FormationWindow
	
	formation = GemRB.GetVar ("SelectedFormation")
	print "FORMATION:", formation
	if last_formation != None and last_formation != formation:
		Button = GemRB.GetControl (Window, last_formation)
		GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_UNPRESSED)

	Button = GemRB.GetControl (Window, formation)
	GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_SELECTED)

	last_formation = formation
