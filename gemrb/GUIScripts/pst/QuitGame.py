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
# $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/GUIScripts/pst/QuitGame.py,v 1.2 2007/02/10 14:29:19 avenger_teambg Exp $

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
	Window = GemRB.LoadWindow (25)

	#reason for death
	Label = GemRB.GetControl (Window, 0x0fffffff)
	strref = GemRB.GetVar ("QuitGame3")
	GemRB.SetText (Window, Label, strref)

	#done
	Button = GemRB.GetControl (Window, 1)
	GemRB.SetText (Window, Button, 17237)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "DonePress")

	GemRB.HideGUI ()
	GemRB.SetVar ("MessageWindow", -1)
	GemRB.SetVar ("PortraitWindow", Window)
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
