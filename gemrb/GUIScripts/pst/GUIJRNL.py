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
# $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/GUIScripts/pst/GUIJRNL.py,v 1.2 2004/01/17 14:51:34 balrog994 Exp $


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
JournalWindow = None
LogWindow = None
BeastsWindow = None
QuestsWindow = None

###################################################
def OpenJournalWindow ():
	global JournalWindow
	
	if JournalWindow:
		if LogWindow: OpenLogWindow()
		if BeastsWindow: OpenBeastsWindow()
		if QuestsWindow: OpenQuestsWindow()
		
		GemRB.HideGUI()
		
		GemRB.UnloadWindow(JournalWindow)
		JournalWindow = None
		GemRB.SetVar("OtherWindow", -1)
		
		GemRB.UnhideGUI()
		
		return
		
	GemRB.HideGUI()
	
	GemRB.LoadWindowPack ("GUIJRNL")
	JournalWindow = GemRB.LoadWindow (0)
	GemRB.SetVar("OtherWindow", JournalWindow)
	
	# Quests
	Button = GemRB.GetControl (JournalWindow, 0)
	GemRB.SetText (JournalWindow, Button, 20430)
	GemRB.SetEvent (JournalWindow, Button, IE_GUI_BUTTON_ON_PRESS, "OpenQuestsWindow")

	# Beasts
	Button = GemRB.GetControl (JournalWindow, 1)
	GemRB.SetText (JournalWindow, Button, 20634)
	GemRB.SetEvent (JournalWindow, Button, IE_GUI_BUTTON_ON_PRESS, "OpenBeastsWindow")

	# Journal
	Button = GemRB.GetControl (JournalWindow, 2)
	GemRB.SetText (JournalWindow, Button, 20635)
	GemRB.SetEvent (JournalWindow, Button, IE_GUI_BUTTON_ON_PRESS, "OpenLogWindow")

	# Done
	Button = GemRB.GetControl (JournalWindow, 3)
	GemRB.SetText (JournalWindow, Button, 20636)
	GemRB.SetEvent (JournalWindow, Button, IE_GUI_BUTTON_ON_PRESS, "OpenJournalWindow")

	GemRB.SetVisible (JournalWindow, 1)
	
	GemRB.UnhideGUI()


###################################################
def OpenQuestsWindow ():
	global JournalWindow, QuestsWindow
	
	GemRB.HideGUI()
	
	if QuestsWindow:
		GemRB.UnloadWindow(QuestsWindow)
		QuestsWindow = None
		
		GemRB.SetVar("OtherWindow", JournalWindow)
		
		GemRB.UnhideGUI()
		return
	
	QuestsWindow = GemRB.LoadWindow (1)
	GemRB.SetVar("OtherWindow", QuestsWindow)
	
	# 1 - Assigned quests list
	# 3 - quest description

	# Assigned
	Button = GemRB.GetControl (QuestsWindow, 8)
	GemRB.SetText (QuestsWindow, Button, 39433)

	# Completed
	Button = GemRB.GetControl (QuestsWindow, 9)
	GemRB.SetText (QuestsWindow, Button, 39434)

	# Back
	Button = GemRB.GetControl (QuestsWindow, 5)
	GemRB.SetText (QuestsWindow, Button, 46677)
	GemRB.SetEvent (QuestsWindow, Button, IE_GUI_BUTTON_ON_PRESS, "OpenQuestsWindow")

	# Done
	Button = GemRB.GetControl (QuestsWindow, 0)
	GemRB.SetText (QuestsWindow, Button, 20636)
	GemRB.SetEvent (QuestsWindow, Button, IE_GUI_BUTTON_ON_PRESS, "OpenJournalWindow")

	GemRB.SetVisible (QuestsWindow, 1)
	
	GemRB.UnhideGUI()
	

###################################################

def OpenBeastsWindow ():
	global JournalWindow, BeastsWindow
	
	GemRB.HideGUI()
	
	if BeastsWindow:
		GemRB.UnloadWindow(BeastsWindow)
		BeastsWindow = None
		
		GemRB.SetVar("OtherWindow", JournalWindow)
		
		GemRB.UnhideGUI()
		return
	
	BeastsWindow = Window = GemRB.LoadWindow (2)
	GemRB.SetVar("OtherWindow", BeastsWindow)

	# 0 - beasts list
	# 2 - beast description
	
	# PC
	Button = GemRB.GetControl (BeastsWindow, 5)
	GemRB.SetText (BeastsWindow, Button, 20637)

	# NPC
	Button = GemRB.GetControl (BeastsWindow, 6)
	GemRB.SetText (BeastsWindow, Button, 20638)

	# Back
	Button = GemRB.GetControl (BeastsWindow, 7)
	GemRB.SetText (BeastsWindow, Button, 46677)
	GemRB.SetEvent (BeastsWindow, Button, IE_GUI_BUTTON_ON_PRESS, "OpenBeastsWindow")

	# Done
	Button = GemRB.GetControl (BeastsWindow, 4)
	GemRB.SetText (BeastsWindow, Button, 20636)
	GemRB.SetEvent (BeastsWindow, Button, IE_GUI_BUTTON_ON_PRESS, "OpenJournalWindow")

	GemRB.SetVisible (BeastsWindow, 1)
	
	GemRB.UnhideGUI()

	
###################################################

def OpenLogWindow ():
	global JournalWindow, LogWindow
	
	GemRB.HideGUI()
	
	if LogWindow:
		GemRB.UnloadWindow(LogWindow)
		LogWindow = None
		
		GemRB.SetVar("OtherWindow", JournalWindow)
		
		GemRB.UnhideGUI()
		return
	
	LogWindow = GemRB.LoadWindow (3)

	# Back
	Button = GemRB.GetControl (LogWindow, 1)
	GemRB.SetText (LogWindow, Button, 46677)
	GemRB.SetEvent (LogWindow, Button, IE_GUI_BUTTON_ON_PRESS, "OpenLogWindow")

	# Done
	Button = GemRB.GetControl (LogWindow, 0)
	GemRB.SetText (LogWindow, Button, 20636)
	GemRB.SetEvent (LogWindow, Button, IE_GUI_BUTTON_ON_PRESS, "OpenJournalWindow")

	GemRB.SetVisible (LogWindow, 1)
	
	GemRB.UnhideGUI()

###################################################
# End of file GUIJRNL.py
