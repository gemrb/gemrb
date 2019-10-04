# -*-python-*-
# GemRB - Infinity Engine Emulator
# Copyright (C) 2019 The GemRB Project
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#

# GUIMACommon.py - common functions for scripts controlling map windows from GUIMA and GUIWMAP winpacks

import GemRB
import GameCheck
from GUIDefines import STR_AREANAME, LOG_MESSAGE

def MoveToNewArea ():
	import GUIMA
	travel = GUIMA.WorldMapControl.GetDestinationArea (not GameCheck.IsPST()) # no random encounters in pst, AFAIR
	hours = travel["Distance"]
	GUIMA.OpenWorldMapWindow ()

	if travel["Destination"].lower() == GemRB.GetGameString(STR_AREANAME).lower():
		return
	elif hours == -1:
		print ("Invalid target", travel)
		return

	GemRB.CreateMovement (travel["Destination"], travel["Entrance"], travel["Direction"])

	if hours == 0:
		return

	# distance is stored in hours, but the action needs seconds
	GemRB.ExecuteString ("AdvanceTime(%d)"%(hours*300), 1)

	# ~The journey took <DURATION>.~ but pst has it without the token
	if GameCheck.IsPST():
		# GemRB.DisplayString can only deal with resrefs, so cheat until noticed
		if hours > 1:
			GemRB.Log (LOG_MESSAGE, "Actor", GemRB.GetString (19261) + str(hours) + GemRB.GetString (19313))
		else:
			GemRB.Log (LOG_MESSAGE, "Actor", GemRB.GetString (19261) + str(hours) + GemRB.GetString (19312))
	else:
		time = ""
		GemRB.SetToken ("HOUR", str(hours))
		if hours > 1:
			time =  GemRB.GetString (10700)
		else:
			time =  GemRB.GetString (10701)
		GemRB.SetToken ("DURATION", time)
		GemRB.DisplayString (10689, 0xffffff)
