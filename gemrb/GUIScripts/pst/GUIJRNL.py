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
# $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/GUIScripts/pst/GUIJRNL.py,v 1.1 2004/01/11 16:49:09 edheldil Exp $


# GUIJRNL.py - scripts to control journal/diary windows from GUIJRNL winpack

# GUIJRNL:
# 0 - main journal window
# 1 - quests window
# 2 - beasts window
# 3 - log/diary window

###################################################
import GemRB
from GUIDefines import *

from GUICommonWindows import CloseCommonWindows

###################################################
MainWindow = 0
LogWindow = 0
BeastsWindow = 0
QuestsWindow = 0

###################################################
def OpenJournalWindow ():
	global MainWindow
	
	CloseCommonWindows ()
	GemRB.LoadWindowPack ("GUIJRNL")

	MainWindow = Window = GemRB.LoadWindow (0)
	
	# Quests
	Button = GemRB.GetControl (Window, 0)
	GemRB.SetText (Window, Button, 20430)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenQuestsWindow")

	# Beasts
	Button = GemRB.GetControl (Window, 1)
	GemRB.SetText (Window, Button, 20634)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenBeastsWindow")

	# Journal
	Button = GemRB.GetControl (Window, 2)
	GemRB.SetText (Window, Button, 20635)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenLogWindow")

	# Done
	Button = GemRB.GetControl (Window, 3)
	GemRB.SetText (Window, Button, 20636)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "CloseJournalWindow")

	GemRB.SetVisible (Window, 1)


def CloseJournalWindow ():
	if LogWindow: CloseLogWindow ()
	if BeastsWindow: CloseBeastsWindow ()
	if QuestsWindow: CloseQuestsWindow ()
	
	GemRB.SetVisible (MainWindow, 0)
	GemRB.UnloadWindow (MainWindow)


###################################################
def OpenQuestsWindow ():
	global QuestsWindow
	QuestsWindow = Window = GemRB.LoadWindow (1)

	# 1 - Assigned quests list
	# 3 - quest description

	# Assigned
	Button = GemRB.GetControl (Window, 8)
	GemRB.SetText (Window, Button, 39433)

	# Completed
	Button = GemRB.GetControl (Window, 9)
	GemRB.SetText (Window, Button, 39434)

	# Back
	Button = GemRB.GetControl (Window, 5)
	GemRB.SetText (Window, Button, 46677)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "CloseQuestsWindow")

	# Done
	Button = GemRB.GetControl (Window, 0)
	GemRB.SetText (Window, Button, 20636)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "CloseJournalWindow")

	GemRB.SetVisible (Window, 1)


def CloseQuestsWindow ():
	global QuestsWindow
	GemRB.SetVisible (QuestsWindow, 0)
	GemRB.UnloadWindow (QuestsWindow)
	QuestsWindow = 0
	GemRB.SetVisible (MainWindow, 1)

###################################################

def OpenBeastsWindow ():
	global BeastsWindow
	BeastsWindow = Window = GemRB.LoadWindow (2)

	# 0 - beasts list
	# 2 - beast description
	
	# PC
	Button = GemRB.GetControl (Window, 5)
	GemRB.SetText (Window, Button, 20637)

	# NPC
	Button = GemRB.GetControl (Window, 6)
	GemRB.SetText (Window, Button, 20638)

	# Back
	Button = GemRB.GetControl (Window, 7)
	GemRB.SetText (Window, Button, 46677)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "CloseBeastsWindow")

	# Done
	Button = GemRB.GetControl (Window, 4)
	GemRB.SetText (Window, Button, 20636)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "CloseJournalWindow")

	GemRB.SetVisible (Window, 1)


def CloseBeastsWindow ():
	global BeastsWindow
	GemRB.SetVisible (BeastsWindow, 0)
	GemRB.UnloadWindow (BeastsWindow)
	BeastsWindow = 0
	GemRB.SetVisible (MainWindow, 1)
	
###################################################

def OpenLogWindow ():
	global LogWindow
	LogWindow = Window = GemRB.LoadWindow (3)

	# Back
	Button = GemRB.GetControl (Window, 1)
	GemRB.SetText (Window, Button, 46677)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "CloseLogWindow")

	# Done
	Button = GemRB.GetControl (Window, 0)
	GemRB.SetText (Window, Button, 20636)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "CloseJournalWindow")

	GemRB.SetVisible (Window, 1)

def CloseLogWindow ():
	global LogWindow
	GemRB.SetVisible (LogWindow, 0)
	GemRB.UnloadWindow (LogWindow)
	LogWindow = 0
	GemRB.SetVisible (MainWindow, 1)
	

###################################################
# End of file GUIJRNL.py
