# -*-python-*-
# GemRB - Infinity Engine Emulator
# Copyright (C) 2003-2004 The GemRB Project
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

# LoadScreen.py - display Loading screen

###################################################

import GemRB
import GameCheck
from GUIDefines import *

LoadScreen = None

def SetLoadScreen ():
	return

def StartLoadScreen ():
	global LoadScreen

	LoadScreen = GemRB.LoadWindow (0, "guils")
	LoadScreen.AddAlias("LOADWIN")
	
	Middle = LoadScreen.GetControl (3)
	Progress = GemRB.GetVar ("Progress")

	if not GameCheck.IsBG2Demo():
		LoadPic = GemRB.GetGameString (STR_LOADMOS)
		if LoadPic == "":
			#the addition of 1 is not an error, bg2 loadpic resrefs are GTRSK002-GTRSK006
			LoadPic = "GTRSK00"+str(GemRB.Roll(1,5,1) )
		Middle.SetMOS (LoadPic)
	else:
		# During loading, this fn is called at 0% and 70%, so take advantage of that
		#   and display the "quiet" frame first and the "flaming" frame the second time.
		#   It would be even better to display the phases inbetween as well; however,
		#   the bg2demo does not either, even though the frames are there.
		if Progress:
			Middle.SetBAM ("COADCNTR", 1, 0)
		else:
			Middle.SetBAM ("COADCNTR", 0, 0)

	if GameCheck.HasTOB():
		Table = GemRB.LoadTable ("loadh25")
	else:
		Table = GemRB.LoadTable ("loadhint")
	tmp = Table.GetRowCount ()
	tmp = GemRB.Roll (1, tmp-1, 0)
	HintStr = Table.GetValue (tmp, 0)

	Label = LoadScreen.GetControl (2)
	Label.SetText(HintStr)

	def EndLoadScreen ():
		TMessageTA = GemRB.GetView("MsgSys", 0)
		TMessageTA.Append("[p][color=f1f28d]" + GemRB.GetString (HintStr) + "[/color][/p]\n")
		
		if GameCheck.IsBG2Demo():
			Middle = LoadScreen.GetControl (3)
			Middle.SetBAM ("COADCNTR", 1, 0)
	
		LoadScreen.SetAction(lambda win: GemRB.GamePause(0, 0), ACTION_WINDOW_CLOSED)
		GemRB.SetTimer(LoadScreen.Close, 500, 0)
		return

	Bar = LoadScreen.GetControl (0)
	Bar.AddAlias("LOAD_PROG")
	Bar.SetVarAssoc ("Progress", 0)
	Bar.SetEvent (IE_GUI_PROGRESS_END_REACHED, EndLoadScreen)
	LoadScreen.ShowModal(MODAL_SHADOW_NONE)


