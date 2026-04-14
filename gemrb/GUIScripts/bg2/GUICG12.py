# SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

#character generation, appearance (GUICG12)
import GemRB

import CharGenCommon
from GUIPortraitCommon import *

def OnLoad():
	def get_gender():
		return GemRB.GetVar("Gender")

	def extra_setup(window):
		CharGenCommon.PositionCharGenWin(window, -6)

		TextAreaControl = window.GetControl(7)
		if TextAreaControl:
			TextAreaControl.SetText("")

	SetupAppearanceWindow(
		GetGenderFunc=get_gender,
		ExtraSetupFunc=extra_setup
	)

