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
# $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/GUIScripts/pst/GUIREC.py,v 1.1 2004/01/11 16:49:09 edheldil Exp $


# GUIREC.py - scripts to control stats/records windows from GUIREC winpack

# GUIREC:
# 0,1,2 - sommon windows (time, message, menu)
# 3 - main statistics window
# 4 - level up win
# 5 - info - kills, weapons ...
# 6 - dual class list ???
# 7 - class panel
# 8 - skills panel
# 9 - choose mage spells panel
# 10 - some small win, 1 button
# 11 - some small win, 2 buttons
# 12 - biography?
# 13 - specialist mage panel
# 14 - proficiencies
# 15 - some 2 panel window
# 16 - some 2 panel window
# 17 - some 2 panel window


# MainWindow:
# 0 - main textarea
# 1 - its scrollbar
# 2 - WMCHRP character portrait
# 5 - STALIGN alignment
# 6 - STFCTION faction
# 7,8,9 - STCSTM (info, reform party, level up)
# 0x1000000a - name
#0x1000000b - ac
#0x1000000c, 0x1000000d hp now, hp max
#0x1000000e str
#0x1000000f int
#0x10000010 wis
#0x10000011 dex
#0x10000012 con
#0x10000013 chr
#x10000014 race
#x10000015 sex
#0x10000016 class

#31-36 stat buts
#37 ac but
#38 hp but?


###################################################
import GemRB
from GUIDefines import *

from GUICommonWindows import OpenCommonWindows, CloseCommonWindows
import GUICommonWindows

###################################################
MainWindow = 0

###################################################
def OpenStatsWindow ():
	global MainWindow

	#GemRB.SetVisible (3, 0)
	#GemRB.UnloadWindow (3)

	CloseCommonWindows ()
	GemRB.LoadWindowPack ("GUIREC")
        OpenCommonWindows ()

	#MainWindow = Window = GemRB.LoadWindow (3)
	MainWindow = Window = GUICommonWindows.MainWindow

	# Information
	Button = GemRB.GetControl (Window, 7)
	GemRB.SetText (Window, Button, 4245)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenInformationWindow")
	
	# Reform Party
	Button = GemRB.GetControl (Window, 8)
	GemRB.SetText (Window, Button, 4244)

	# Level Up
	Button = GemRB.GetControl (Window, 9)
	GemRB.SetText (Window, Button, 4246)

	# help, info textarea
	Text = GemRB.GetControl (Window, 0)
	GemRB.SetText (Window, Text, "Pokus ....")

	# name
	Label = GemRB.GetControl (Window, 0x1000000a)
	GemRB.SetText (Window, Label, "Some name ...")

	# class
	Label = GemRB.GetControl (Window, 0x1000000b)
	GemRB.SetText (Window, Label, "AC")

	# hp now
	Label = GemRB.GetControl (Window, 0x1000000c)
	GemRB.SetText (Window, Label, "hp now")

	# hp max
	Label = GemRB.GetControl (Window, 0x1000000d)
	GemRB.SetText (Window, Label, "hp max")

	# stats
	for i in range (6):
		Label = GemRB.GetControl (Window, 0x1000000e + i)
		GemRB.SetText (Window, Label, "%d" %i)
		

	# race
	Label = GemRB.GetControl (Window, 0x10000014)
	GemRB.SetText (Window, Label, "race")

	#print 'stat:', GemRB.GetPlayerStat (0, 36)

	# sex
	Label = GemRB.GetControl (Window, 0x10000015)
	GemRB.SetText (Window, Label, "sex")

	# class
	Label = GemRB.GetControl (Window, 0x10000016)
	GemRB.SetText (Window, Label, "class")


	# stat buttons
	for i in range (6):
		Button = GemRB.GetControl (Window, 31 + i)
		GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_NO_IMAGE,OP_SET)

	# AC button
	Button = GemRB.GetControl (Window, 37)
	GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_NO_IMAGE,OP_SET)

	# HP button
	Button = GemRB.GetControl (Window, 38)
	GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_NO_IMAGE,OP_SET)

	#GemRB.SetVisible (Window, 1)

def OpenInformationWindow ():
	global InformationWindow
	
	GemRB.SetVisible (MainWindow, 0)
	#GemRB.UnloadWindow (MainWindow)
	
	InformationWindow = Window = GemRB.LoadWindow (5)

	# Biography
	Button = GemRB.GetControl (Window, 1)
	GemRB.SetText (Window, Button, 4247)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenBiographyWindow")

	# Done
	Button = GemRB.GetControl (Window, 0)
	GemRB.SetText (Window, Button, 1403)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "CloseInformationWindow")


	GemRB.SetVisible (Window, 1)

def CloseInformationWindow ():
	GemRB.SetVisible (InformationWindow, 0)
	GemRB.UnloadWindow (InformationWindow)

	GemRB.SetVisible (MainWindow, 1)
	

def OpenBiographyWindow ():
	global BiographyWindow
	
	GemRB.SetVisible (InformationWindow, 0)
	
	BiographyWindow = Window = GemRB.LoadWindow (12)

	# Done
	Button = GemRB.GetControl (Window, 2)
	GemRB.SetText (Window, Button, 1403)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "CloseBiographyWindow")

	GemRB.SetVisible (Window, 1)
	
def CloseBiographyWindow ():
	GemRB.SetVisible (BiographyWindow, 0)
	GemRB.UnloadWindow (BiographyWindow)

	GemRB.SetVisible (InformationWindow, 1)
	


#FillPlayerInfo, 
#GetPlayerStat
#SetButtonBAM
#SetButtonPLT
#SetButtonPicture
#SetButtonSprites
#SetLabelTextColor
#SetSaveGamePortrait
#SetSaveGamePreview
#SetTextAreaSelectable
#ShowModal
#StatComment(...)
#        Replaces values into an strref.

###################################################
# End of file GUIREC.py
