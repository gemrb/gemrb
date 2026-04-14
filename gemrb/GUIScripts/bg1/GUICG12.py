# SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

# character generation, portrait (GUICG12)
import GemRB
from ie_stats import *
import CharGenCommon
from GUIPortraitCommon import *

def OnLoad():
	def get_gender():
		MyChar = GemRB.GetVar("Slot")
		return GemRB.GetPlayerStat(MyChar, IE_SEX)

	SetupAppearanceWindow(
		GetGenderFunc= get_gender,
		BackHandler= lambda: CharGenCommon.back(AppearanceWindow),
		NextPressFunc= CharGenCommon.next,
		CustomDoneFunc= CharGenCommon.next
	)
