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
# $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/GUIScripts/pst/FloatMenuWindow.py,v 1.3 2004/06/27 19:30:07 edheldil Exp $

# FloatMenuWindow.py - display PST's floating menu window from GUIWORLD winpack

###################################################

import GemRB
from GUIDefines import *
from GUICommonWindows import GetActorPortrait, SetSelectionChangeHandler

FloatMenuWindow = None

def OpenFloatMenuWindow ():
	global FloatMenuWindow
        GemRB.HideGUI ()

        if FloatMenuWindow:
		GemRB.UnloadWindow (FloatMenuWindow)
		FloatMenuWindow = None

		GemRB.SetVar ("FloatWindow", -1)
		SetSelectionChangeHandler (None)
		GemRB.UnhideGUI ()
		return

	GemRB.LoadWindowPack ("GUIWORLD")
	FloatMenuWindow = Window = GemRB.LoadWindow (3)
	GemRB.SetVar ("FloatWindow", Window)

	x, y = GemRB.GetVar ("MenuX"), GemRB.GetVar ("MenuY")

	# FIXME: keep the menu inside the viewport!!!
	GemRB.SetWindowPos (Window, x, y)

	# portrait button
	Button = GemRB.GetControl (Window, 0)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "FloatMenuSelectNextPC")

	# Initiate Dialogue
	Button = GemRB.GetControl (Window, 1)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "FloatMenuForceDialog")
	GemRB.SetTooltip (Window, Button, 8191)

	# Attack/Select Weapon
	Button = GemRB.GetControl (Window, 2)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "FloatMenuForceAttack")
	GemRB.SetTooltip (Window, Button, 8192)

	# Cast spell
	Button = GemRB.GetControl (Window, 3)
	GemRB.SetTooltip (Window, Button, 8193)

	# Use Item
	Button = GemRB.GetControl (Window, 4)
	GemRB.SetTooltip (Window, Button, 8194)

	# Use Special Ability
	Button = GemRB.GetControl (Window, 5)
	GemRB.SetTooltip (Window, Button, 8195)

	# Menu Anchors
	Button = GemRB.GetControl (Window, 11)
	GemRB.SetTooltip (Window, Button, 8199)
	Button = GemRB.GetControl (Window, 12)
	GemRB.SetTooltip (Window, Button, 8199)

	# Rotate Items left & right
	Button = GemRB.GetControl (Window, 13)
	GemRB.SetTooltip (Window, Button, 8197)
	Button = GemRB.GetControl (Window, 14)
	GemRB.SetTooltip (Window, Button, 8198)

	# 6 - 10 - items/spells/other
	# 15 - 19 - empty item slot

	# BAMs:
	# AMALLSTP - 41655
	# AMATTCK - 41654
	#AMGENB1
	#AMGENS
	#AMGUARD - 31657, 32431, 41652, 48887
	#AMHILITE - highlight frame
	#AMINVNT - 41601, 41709
	#AMJRNL - 41623, 41714
	#AMMAP - 41625, 41710
	#AMSPLL - 4709
	#AMSTAT - 4707
	#AMTLK - 41653
	
	#AMPANN
	#AMPDKK
	#AMPFFG
	#AMPIGY
	#AMPMRT
	#AMPNDM
	#AMPNM1
	#AMPVHA

	SetSelectionChangeHandler (UpdateFloatMenuWindow)
	UpdateFloatMenuWindow ()
	
	GemRB.UnhideGUI ()


def UpdateFloatMenuWindow ():
	Window = FloatMenuWindow
	
	pc = GemRB.GameGetSelectedPCSingle ()
	pc = pc + 1; 

	Button = GemRB.GetControl (Window, 0)
	#GemRB.SetButtonFlags(Window, Button, IE_GUI_BUTTON_NO_IMAGE | IE_GUI_BUTTON_PICTURE, OP_SET)
	GemRB.SetButtonSprites (Window, Button, GetActorPortrait (pc, 'FMENU'), 0, 0, 1, 2, 3)


def FloatMenuSelectNextPC ():
	sel = (GemRB.GameGetSelectedPCSingle () + 1) % GemRB.GetPartySize ()
	GemRB.GameSelectPCSingle (sel)
	UpdateFloatMenuWindow ()
