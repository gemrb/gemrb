# -*-python-*-
# GemRB - Infinity Engine Emulator
# Copyright (C) 2003 The GemRB Project
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
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
#
# $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/GUIScripts/pst/MessageWindow.py,v 1.31 2005/08/13 19:27:27 avenger_teambg Exp $


# MessageWindow.py - scripts and GUI for main (walk) window

###################################################

import GemRB

from GUICommonWindows import *
import GUICommonWindows

from FloatMenuWindow import *

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
ActionsWindow = 0
PortraitWindow = 0
OptionsWindow = 0

def OnLoad():
	global MessageWindow, ActionsWindow, PortraitWindow, OptionsWindow

	GemRB.LoadWindowPack ("GUIWORLD")
	GemRB.SetInfoTextColor(0,255,0,255)
	ActionsWindow = GemRB.LoadWindow(0)
	OptionsWindow = GemRB.LoadWindow(2)
	MessageWindow = GemRB.LoadWindow(7)
	PortraitWindow = OpenPortraitWindow (1)

	MessageTA = GemRB.GetControl (MessageWindow, 1)
	GemRB.SetTextAreaFlags (MessageWindow, MessageTA, IE_GUI_TEXTAREA_AUTOSCROLL)
	GemRB.SetVar ("MessageTextArea", MessageTA)
	GemRB.SetVar ("ActionsWindow", ActionsWindow)
	GemRB.SetVar ("OptionsWindow", OptionsWindow)
	GemRB.SetVar ("MessageWindow", -1)
	GemRB.SetVar ("OtherWindow", -1)
	GemRB.SetVar ("ActionsPosition", 1) #Bottom
	GemRB.SetVar ("OptionsPosition", 1) #Bottom
	GemRB.SetVar ("MessagePosition", 1) #Bottom
	GemRB.SetVar ("OtherPosition", 0) #Left
	
	GemRB.SetVar ("MessageTextArea", MessageTA)
	GemRB.GameSetScreenFlags (0, OP_SET)
	
	CloseButton= GemRB.GetControl (MessageWindow, 0)
	GemRB.SetText(MessageWindow, CloseButton, 28082)
	GemRB.SetEvent (MessageWindow, CloseButton, IE_GUI_BUTTON_ON_PRESS, "OnDecreaseSize")
	
	OpenButton = GemRB.GetControl (OptionsWindow, 10)
	GemRB.SetEvent (OptionsWindow, OpenButton, IE_GUI_BUTTON_ON_PRESS, "OnIncreaseSize")

	# Select all
	Button = GemRB.GetControl (ActionsWindow, 1)
	GemRB.SetEvent (ActionsWindow, Button, IE_GUI_BUTTON_ON_PRESS, "SelectAllOnPress")

	FormationButton = GemRB.GetControl (ActionsWindow, 4)
	GemRB.SetEvent (ActionsWindow, FormationButton, IE_GUI_BUTTON_ON_PRESS, "OpenFormationWindow")

	SetupActionsWindowControls (ActionsWindow)
	SetupMenuWindowControls (OptionsWindow)

	UpdateControlStatus ()

	
def OnIncreaseSize():
	GemRB.GameSetScreenFlags (GS_LARGEDIALOG, OP_OR)


def OnDecreaseSize():
	GemRB.GameSetScreenFlags (GS_LARGEDIALOG, OP_NAND)


def UpdateControlStatus ():
	global MessageWindow, PortraitWindow, ActionsWindow, OptionsWindow

	Expand = GemRB.GetVar ("MessageWindowSize") & (GS_DIALOGMASK|GS_DIALOG)

	hideflags = GemRB.HideGUI ()
	if Expand:
		GemRB.SetVar ("MessageWindow", MessageWindow)
		GemRB.SetVar ("PortraitWindow", -1)
		GemRB.SetVar ("ActionsWindow", -1)
		GemRB.SetVar ("OptionsWindow", -1)
		MessageTA = GemRB.GetVar ("MessageTextArea")
		GemRB.SetControlStatus (MessageWindow,MessageTA,IE_GUI_CONTROL_FOCUSED)
	else:
		GemRB.SetVar ("MessageWindow", -1)
		GemRB.SetVar ("PortraitWindow", PortraitWindow)
		GemRB.SetVar ("ActionsWindow", ActionsWindow)
		GemRB.SetVar ("OptionsWindow", OptionsWindow)
		GemRB.SetControlStatus (0,0,IE_GUI_CONTROL_FOCUSED)

	if hideflags:
		GemRB.UnhideGUI ()

