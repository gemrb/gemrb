# SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

# character generation, portrait (GUICG12)
import GemRB
import CharGenCommon
from GUIPortraitCommon import *

def OnLoad():
	SetupAppearanceWindow(
		BackHandler= lambda: CharGenCommon.back(AppearanceWindow),
		NextPressFunc= CharGenCommon.next,
		CustomDoneFunc= CharGenCommon.next
	)
