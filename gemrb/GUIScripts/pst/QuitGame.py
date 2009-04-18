# -*-python-*-
# GemRB - Infinity Engine Emulator
# Copyright (C) 2007 The GemRB Project
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
# $Id$

# QuitGame.py - display EndGame sequence

###################################################

import GemRB
from GUIDefines import *

movies = [None,"T1DEATH","T1ABSORB","FINALE"]

def OnLoad ():
	GemRB.HideGUI ()
	which = movies[GemRB.GetVar ("QuitGame1")]
	if which!=None:
		GemRB.PlayMovie (which,1)
	which = movies[GemRB.GetVar ("QuitGame2")]
	if which!=None:
		GemRB.PlayMovie (which,1)
	which = GemRB.GetVar ("QuitGame3")
	if which:
		DeathWindowEnd ()
	else:
		GemRB.QuitGame ()
		GemRB.SetNextScript("Start")

def DonePress ():
		GemRB.QuitGame ()
		GemRB.SetNextScript("Start")

def DeathWindowEnd ():
	GemRB.GamePause (1,1)

	GemRB.LoadWindowPack (GetWindowPack())
	Window = GemRB.LoadWindowObject (25)

	#reason for death
	Label = Window.GetControl (0x0fffffff)
	strref = GemRB.GetVar ("QuitGame3")
	Label.SetText (strref)

	#done
	Button = Window.GetControl (1)
	Button.SetText (17237)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "DonePress")
	Button.SetFlags (IE_GUI_BUTTON_DEFAULT|IE_GUI_BUTTON_CANCEL, OP_OR)

	GemRB.HideGUI ()
	GemRB.SetVar ("MessageWindow", -1)
	GemRB.SetVar ("PortraitWindow", Window.ID)
	GemRB.UnhideGUI ()
	#making the playing field gray
	GemRB.SetVisible (0,2)
	return

def GetWindowPack():
	width = GemRB.GetSystemVariable (SV_WIDTH)
	if width == 800:
		return "GUIWORLD"
	if width == 1024:
		return "GUIWORLD"
	if width == 1280:
		return "GUIWORLD"
	#default
	return "GUIWORLD"
