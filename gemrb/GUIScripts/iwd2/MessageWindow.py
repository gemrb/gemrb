# -*-python-*-
# GemRB - Infinity Engine Emulator
# Copyright (C) 2003-2005 The GemRB Project
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
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
#

import GemRB

from GUICommonWindows import *
import GUICommonWindows
from GUIJRNL import *
from GUIMA import *
from GUIMG import *
from GUIINV import *
from GUIOPT import *
from GUIPR import *
from GUIREC import *
from GUISTORE import *
from GUIWORLD import *

MessageWindow = 0
MessageTA = 0

def OnLoad():
	global MessageWindow, MessageTA
	GemRB.LoadWindowPack("GUIW08")
	OptionsWindow = MessageWindow = GemRB.LoadWindow(0)
	MessageTA = GemRB.GetControl(MessageWindow, 1)
	GemRB.SetTextAreaFlags(MessageWindow, MessageTA, IE_GUI_TEXTAREA_AUTOSCROLL)

	OpenPortraitWindow()
	ActionsWindow = PortraitWindow = GUICommonWindows.PortraitWindow

	#ActionsWindow = PortraitWindow = GemRB.LoadWindow(1)
	
	GemRB.SetVar("MessageWindow", MessageWindow)
	GemRB.SetVar("PortraitWindow", PortraitWindow)
	GemRB.SetVar("TopWindow", -1)
	GemRB.SetVar("OtherWindow", -1)
	GemRB.SetVar("FloatWindow", -1)
	GemRB.SetVar("ActionsPosition", -1) #already handled in portraitwindow
	GemRB.SetVar("OptionsPosition", -1) #already handled in messagewindow
	GemRB.SetVar("MessagePosition", 4)
	GemRB.SetVar("OtherPosition", 5) #Inactivating
	GemRB.SetVar("TopPosition", 5) #Inactivating

	GemRB.SetVar("MessageTextArea", MessageTA)
	
	SetupActionsWindowControls (ActionsWindow)
	SetupMenuWindowControls (OptionsWindow)

	GemRB.SetVisible(MessageWindow, 1)
	GemRB.SetVisible(PortraitWindow, 1)
	return
