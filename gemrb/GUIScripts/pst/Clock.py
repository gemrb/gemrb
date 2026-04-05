# SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

import GemRB
import GUICommon
from GUIDefines import *

def CreateClockButton(Button):
	Button.SetAnimation ("WMTIME", 0, A_ANI_GAMEANIM)
	Button.SetState (IE_GUI_BUTTON_LOCKED)
	Button.SetFlags (IE_GUI_BUTTON_PICTURE, OP_SET)
	Button.OnPress (lambda: GemRB.GamePause (2, 0))
	Button.OnMouseEnter (UpdateClock)
	SetPSTGamedaysAndHourToken ()
	
	UpdateClock()

def SetPSTGamedaysAndHourToken ():
	currentTime = GemRB.GetGameTime()
	hours = (currentTime % 7200) // 300
	if hours < 12:
		ampm = "AM"
	else:
		ampm = "PM"
		hours -= 12
	minutes = (currentTime % 300) // 5

	GemRB.SetToken ('CLOCK_HOUR', str (hours))
	GemRB.SetToken ('CLOCK_MINUTE', '%02d' % minutes)
	GemRB.SetToken ('CLOCK_AMPM', ampm)

def UpdateClock ():
	ActionsWindow = GemRB.GetView("ACTWIN")
	Clock = ActionsWindow.GetControl (0)
	Button = ActionsWindow.GetControl (2)

	SetPSTGamedaysAndHourToken ()
	Clock.SetTooltip (GemRB.GetString(65027))

	twin = GemRB.GetView("WIN_TOP")
	if twin:
		Button.SetState (IE_GUI_BUTTON_ENABLED)
	else:
		Button.SetState (IE_GUI_BUTTON_DISABLED)
