# SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

# character generation, appearance (GUICG12)
import GemRB
import CharOverview
from GUIPortraitCommon import *

def OnLoad():
	def extra_setup(window):
		CharOverview.PositionCharGenWin(window)
		
	SetupAppearanceWindow(
		ExtraSetupFunc=extra_setup
	)
