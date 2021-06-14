# -*-python-*-
# GemRB - Infinity Engine Emulator
# Copyright (C) 2021 The GemRB Project
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
import GameCheck

def UpdateClock ():
	OptionsWindow = GemRB.GetView("OPTWIN")
	ActionsWindow = GemRB.GetView("ACTWIN")
	
	Clock = None
	if OptionsWindow:
		if GameCheck.IsIWD2():
			Clock = OptionsWindow.GetControl (10)
		elif OptionsWindow.GetControl(9):
			Clock = OptionsWindow.GetControl (9)
			
		if Clock.IsVisible() == False:
			Clock = None
	
	if Clock is None and ActionsWindow:
		Clock = ActionsWindow.GetControl (62)
	
	if Clock and (Clock.HasAnimation("CGEAR") or GameCheck.IsIWD2()):
		Hours = (GemRB.GetGameTime () % 7200) // 300
		GUICommon.SetGamedaysAndHourToken ()
		Clock.SetBAM ("CDIAL", 0, int((Hours + 12) % 24))
		Clock.SetTooltip (GemRB.GetString (16041)) # refetch the string, since the tokens changed
