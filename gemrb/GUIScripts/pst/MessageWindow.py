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
# $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/GUIScripts/pst/MessageWindow.py,v 1.19 2004/05/15 21:13:57 avenger_teambg Exp $


# MessageWindow.py - scripts and GUI for main (walk) window

import GemRB

from GUICommonWindows import *

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
CloseButton = 0
OpenButton = 0
MaxExpand = 1

def OnLoad():
	global MessageWindow, ActionsWindow, PortraitWindow, OptionsWindow, CloseButton
	GemRB.LoadWindowPack("GUIWORLD")
	GemRB.SetInfoTextColor(0,255,0,255)
	ActionsWindow = GemRB.LoadWindow(0)
	PortraitWindow = GemRB.LoadWindow(1)
	PopulatePortraitWindow ()
	OptionsWindow = GemRB.LoadWindow(2)
	MessageWindow = GemRB.LoadWindow(7)
	MessageTA = GemRB.GetControl(MessageWindow, 1)
	GemRB.SetTAAutoScroll(MessageWindow, MessageTA, 1)
	GemRB.SetVar("PortraitWindow", PortraitWindow)
	GemRB.SetVar("ActionsWindow", ActionsWindow)
	GemRB.SetVar("OptionsWindow", OptionsWindow)
	GemRB.SetVar("MessageWindow", -1)
	GemRB.SetVar("OtherWindow", -1)
	GemRB.SetVar("PortraitPosition", 1) #Bottom
	GemRB.SetVar("ActionsPosition", 1) #Bottom
	GemRB.SetVar("OptionsPosition", 1) #Bottom
	GemRB.SetVar("MessagePosition", 1) #Bottom
	GemRB.SetVar("OtherPosition", 0) #Left
	
	GemRB.SetVar("MessageTextArea", MessageTA)
	GemRB.SetVar("MessageWindowSize", 0)
	
	CloseButton= GemRB.GetControl(MessageWindow, 0)
	GemRB.SetText(MessageWindow, CloseButton, 28082)
	
	OpenButton = GemRB.GetControl(OptionsWindow, 10)
	GemRB.SetEvent(OptionsWindow, OpenButton, IE_GUI_BUTTON_ON_PRESS, "OnIncreaseSize")

	FormationButton = GemRB.GetControl(ActionsWindow, 4)
	GemRB.SetEvent(ActionsWindow, FormationButton, IE_GUI_BUTTON_ON_PRESS, "OpenFormationWindow")


	SetupActionsWindowControls (ActionsWindow)
	SetupMenuWindowControls (OptionsWindow)

	UpdateResizeButtons()
	
	GemRB.SetVisible(ActionsWindow, 1)
	GemRB.SetVisible(PortraitWindow, 1)
	GemRB.SetVisible(OptionsWindow, 1)
	GemRB.SetVisible(MessageWindow, 0)
	return
	
def OnIncreaseSize():
	global MessageWindow, PortraitWindow, ActionsWindow, OptionsWindow
	
	MessageTA = GemRB.GetVar("MessageTextArea")
	Expand = GemRB.GetVar("MessageWindowSize")
	
	if Expand == 1:
		return
		
	GemRB.HideGUI()
	
	if Expand == 0:
		GemRB.SetVar("MessageWindow", MessageWindow)
		GemRB.SetVar("PortraitWindow", -1)
		GemRB.SetVar("ActionsWindow", -1)
		GemRB.SetVar("OptionsWindow", -1)
		GemRB.SetVisible(MessageWindow, 1)
		GemRB.SetVisible(PortraitWindow, 0)
		GemRB.SetVisible(ActionsWindow, 0)
		GemRB.SetVisible(OptionsWindow, 0)
			
	Expand = 1
	
	GemRB.SetVar("MessageWindowSize", Expand)
	UpdateResizeButtons()
	GemRB.UnhideGUI()
	GemRB.SetControlStatus(MessageWindow,MessageTA,IE_GUI_CONTROL_FOCUSED);
	return
	
def OnDecreaseSize():
	global MessageWindow, PortraitWindow, ActionsWindow, OptionsWindow
	
	MessageTA = GemRB.GetVar("MessageTextArea")
	Expand = GemRB.GetVar("MessageWindowSize")
	
	if Expand == 0:
		return
		
	GemRB.HideGUI()
	
	if Expand == 1:
		GemRB.SetVar("MessageWindow", -1)
		GemRB.SetVar("PortraitWindow", PortraitWindow)
		GemRB.SetVar("ActionsWindow", ActionsWindow)
		GemRB.SetVar("OptionsWindow", OptionsWindow)
		GemRB.SetVisible(MessageWindow, 0)
		GemRB.SetVisible(PortraitWindow, 1)
		GemRB.SetVisible(ActionsWindow, 1)
		GemRB.SetVisible(OptionsWindow, 1)
			
	Expand = 0
	
	GemRB.SetVar("MessageWindowSize", Expand)
	UpdateResizeButtons()
	GemRB.UnhideGUI()
	if Expand:
		GemRB.SetControlStatus(MessageWindow,MessageTA,IE_GUI_CONTROL_FOCUSED);
	else:
		GemRB.SetControlStatus(0,0,IE_GUI_CONTROL_FOCUSED);
	return
	
def UpdateResizeButtons():
	global MessageWindow, CloseButton
	
	GemRB.SetEvent(MessageWindow, CloseButton, IE_GUI_BUTTON_ON_PRESS, "OnDecreaseSize")
	return

def PopulatePortraitWindow ():
	Window = PortraitWindow

	for i in range (0,6):
		Button = GemRB.GetControl (Window, i)
		GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "PortraitButtonOnPress")

		pic = GemRB.GetPlayerPortrait (i+1,0)
		sel = GemRB.IsPlayerSelected (i+1)
		GemRB.SetButtonBAM (Window, Button, pic, 0, 0, 0)
		GemRB.SetButtonFlags(Window, Button, IE_GUI_BUTTON_PICTURE | IE_GUI_BUTTON_ANIMATED, OP_SET)
		GemRB.SetVarAssoc (Window, Button, 'SelectedSingle', i)
		if sel:
			GemRB.SetButtonState(Window, Button, IE_GUI_BUTTON_SELECTED)
		else:
			GemRB.SetButtonState(Window, Button, IE_GUI_BUTTON_UNPRESSED)

def PortraitButtonOnPress ():
	var = GemRB.GetVar ("SelectedSingle")
	#print "SELECTED:", var

	if var < GemRB.GetPartySize ():
		GemRB.GameSelectPCSingle (var)

		RunSelectionChangeHandler ()
