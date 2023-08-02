# GemRB - Infinity Engine Emulator
# Copyright (C) 2003-2017 The GemRB Project
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#

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
	Button.SetTooltip (GemRB.GetString(65027))
	
	UpdateClock()

def SetPSTGamedaysAndHourToken ():
	currentTime = GemRB.GetGameTime()
	hours = (currentTime % 7200) / 300
	if hours < 12:
		ampm = "AM"
	else:
		ampm = "PM"
		hours -= 12
	minutes = (currentTime % 300) / 60

	GemRB.SetToken ('CLOCK_HOUR', str (hours))
	GemRB.SetToken ('CLOCK_MINUTE', '%02d' %minutes)
	GemRB.SetToken ('CLOCK_AMPM', ampm)

def UpdateClock ():
	ActionsWindow = GemRB.GetView("ACTWIN")

	SetPSTGamedaysAndHourToken ()
	twin = GemRB.GetView("WIN_TOP")
	Button = ActionsWindow.GetControl (2)

	if twin:
		Button.SetState (IE_GUI_BUTTON_ENABLED)
	else:
		Button.SetState (IE_GUI_BUTTON_DISABLED)
