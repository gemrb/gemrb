# SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

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

	if GameCheck.HasTOB():
		Table = GemRB.LoadTable ("loadh25")
	else:
		Table = GemRB.LoadTable ("loadhint")
	tmp = Table.GetRowCount ()
	tmp = GemRB.Roll (1, tmp-1, 0)
	HintStr = Table.GetValue (tmp, 0)

	Bar = LoadScreen.GetControl (0)
	Bar.AddAlias("LOAD_PROG")
	Bar.SetVarAssoc ("Progress", 0)
	Bar.OnEndReached (lambda: EndLoadScreen(HintStr))

	# original has no load screen, just use own simple one instead
	if GameCheck.IsBGEE():
		return

	Middle = LoadScreen.GetControl (3)
	Progress = GemRB.GetVar ("Progress")

	if not GameCheck.IsBG2Demo () and not GameCheck.IsBG2EE ():
		LoadPic = GemRB.GetGameString (STR_LOADMOS)
		if LoadPic == "":
			#the addition of 1 is not an error, bg2 loadpic resrefs are GTRSK002-GTRSK006
			LoadPic = "GTRSK00"+str(GemRB.Roll(1,5,1) )
		Middle.SetPicture (LoadPic)
	elif not GameCheck.IsBG2EE ():
		# During loading, this fn is called at 0% and 70%, so take advantage of that
		#   and display the "quiet" frame first and the "flaming" frame the second time.
		#   It would be even better to display the phases inbetween as well; however,
		#   the bg2demo does not either, even though the frames are there.
		if Progress:
			Middle.SetBAM ("COADCNTR", 1, 0)
		else:
			Middle.SetBAM ("COADCNTR", 0, 0)

	Label = LoadScreen.GetControl (2)
	Label.SetText(HintStr)

	LoadScreen.ShowModal(MODAL_SHADOW_NONE)

def EndLoadScreen (HintStr):
	TMessageTA = GemRB.GetView("MsgSys", 0)
	TMessageTA.Append("[p][color=f1f28d]" + GemRB.GetString (HintStr) + "[/color][/p]\n")
	
	if GameCheck.IsBG2Demo():
		Middle = LoadScreen.GetControl (3)
		Middle.SetBAM ("COADCNTR", 1, 0)

	LoadScreen.OnClose (lambda win: GemRB.GamePause(0, 0))
	GemRB.SetTimer(LoadScreen.Close, 500, 0)
	return


