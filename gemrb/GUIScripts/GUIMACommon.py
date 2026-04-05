# SPDX-FileCopyrightText: 2019 Contributors to the GemRB project <https://gemrb.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

# GUIMACommon.py - common functions for scripts controlling map windows from GUIMA and GUIWMAP winpacks

import GemRB
import GameCheck
import GUICommonWindows
from GUIDefines import STR_AREANAME, LOG_MESSAGE, ColorWhite

def MoveToNewArea ():
	import GUIMA
	travel = GUIMA.WorldMapControl.GetDestinationArea (not GameCheck.IsPST()) # no random encounters in pst, AFAIR
	if not travel:
		return

	hours = travel["Distance"]
	GUICommonWindows.CloseTopWindow ()
	if GemRB.GetView ("WIN_PSTWMAP"):
		GemRB.GetView ("WIN_PSTWMAP").Close ()
		GemRB.GamePause (0, 0)

	if travel["Destination"].lower() == GemRB.GetGameString(STR_AREANAME).lower():
		return
	elif hours == -1:
		print ("Invalid target", travel)
		return

	# pst drops you off in the center
	entrance = travel["Entrance"]
	if not entrance:
		entrance = "FROMMAP"
	GemRB.CreateMovement (travel["Destination"], entrance, travel["Direction"])

	if hours == 0:
		return

	# distance is stored in hours, but the action needs seconds
	GemRB.ExecuteString ("AdvanceTime(%d)"%(hours*300), 1)

	# ~The journey took <DURATION>.~ but pst has it without the token
	if GameCheck.IsPST():
		# GemRB.DisplayString can only deal with resrefs, so cheat until noticed
		if hours > 1:
			GemRB.Log (LOG_MESSAGE, "Actor", GemRB.GetString (19261) + str(hours) + " " + GemRB.GetString (19313))
		else:
			GemRB.Log (LOG_MESSAGE, "Actor", GemRB.GetString (19261) + str(hours) + " " + GemRB.GetString (19312))
	else:
		time = ""
		GemRB.SetToken ("HOUR", str(hours))
		if hours > 1:
			time =  GemRB.GetString (10700)
		else:
			time =  GemRB.GetString (10701)
		GemRB.SetToken ("DURATION", time)
		GemRB.DisplayString (10689, ColorWhite)
