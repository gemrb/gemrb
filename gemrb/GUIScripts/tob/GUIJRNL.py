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
# $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/GUIScripts/tob/GUIJRNL.py,v 1.2 2004/08/28 20:36:39 avenger_teambg Exp $


# GUIJRNL.py - scripts to control journal/diary windows from GUIJRNL winpack

# GUIJRNL:
# 0 - main journal window
# 1 - quests window
# 2 - beasts window
# 3 - log/diary window

###################################################
import GemRB
from GUIDefines import *

###################################################
JournalWindow = None
LogWindow = None
QuestsWindow = None

###################################################
def OpenJournalWindow ():
	global JournalWindow

	GemRB.HideGUI()

	if JournalWindow:
		if LogWindow: OpenLogWindow()
		if QuestsWindow: OpenQuestsWindow()
		
		GemRB.HideGUI()
		
		GemRB.UnloadWindow(JournalWindow)
		JournalWindow = None
		GemRB.SetVar("OtherWindow", -1)
		
		GemRB.UnhideGUI()
		return
		
	GemRB.LoadWindowPack ("GUIJRNL")
	JournalWindow = GemRB.LoadWindow (2)
	GemRB.SetVar("OtherWindow", JournalWindow)

	# Quests
	#Button = GemRB.GetControl (JournalWindow, 0)
	#GemRB.SetText (JournalWindow, Button, 20430)
	#GemRB.SetEvent (JournalWindow, Button, IE_GUI_BUTTON_ON_PRESS, "OpenQuestsWindow")

	# Quests completed
	#Button = GemRB.GetControl (JournalWindow, 1)
	#GemRB.SetText (JournalWindow, Button, 20634)
	#GemRB.SetEvent (JournalWindow, Button, IE_GUI_BUTTON_ON_PRESS, "OpenCompletedWindow")

	# Journal
	#Button = GemRB.GetControl (JournalWindow, 2)
	#GemRB.SetText (JournalWindow, Button, 20635)
	#GemRB.SetEvent (JournalWindow, Button, IE_GUI_BUTTON_ON_PRESS, "OpenLogWindow")

	# Done
	#Button = GemRB.GetControl (JournalWindow, 3)
	#GemRB.SetText (JournalWindow, Button, 20636)
	#GemRB.SetEvent (JournalWindow, Button, IE_GUI_BUTTON_ON_PRESS, "OpenJournalWindow")

	GemRB.UnhideGUI()

###################################################
def OpenQuestsWindow ():
	global JournalWindow, QuestsWindow, QuestsList, QuestDesc
	
	GemRB.HideGUI()
	
	if QuestsWindow:
		GemRB.UnloadWindow(QuestsWindow)
		QuestsWindow = None
		
		GemRB.SetVar("OtherWindow", JournalWindow)
		
		GemRB.UnhideGUI()
		return
	
	QuestsWindow = Window = GemRB.LoadWindow (1)
	GemRB.SetVar("OtherWindow", Window)
	
	# Assigned
	Button = GemRB.GetControl (Window, 8)
	GemRB.SetText (Window, Button, 39433)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OnJournalAssignedPress")

	# Completed
	Button = GemRB.GetControl (Window, 9)
	GemRB.SetText (Window, Button, 39434)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OnJournalCompletedPress")

	# Back
	Button = GemRB.GetControl (Window, 5)
	GemRB.SetText (Window, Button, 46677)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenQuestsWindow")

	# Done
	Button = GemRB.GetControl (Window, 0)
	GemRB.SetText (Window, Button, 20636)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenJournalWindow")

	QuestsList = List = GemRB.GetControl (Window, 1)
	GemRB.SetTextAreaSelectable (Window, List, 1)
	GemRB.SetVarAssoc (Window, List, 'SelectedQuest', -1)
	GemRB.SetEvent (Window,List, IE_GUI_TEXTAREA_ON_CHANGE, "OnJournalQuestSelect")

	QuestDesc = GemRB.GetControl (Window, 3)

	EvaluateAllQuests ()
	PopulateQuestsList ()

	#GemRB.SetVisible (QuestsWindow, 1)
	GemRB.UnhideGUI()

def OnJournalIncompletePress ():
	print "Incomplete"
	
def OnJournalCompletedPress ():
	print "Complete"

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
	
	LogWindow = Window = GemRB.LoadWindow (3)
	GemRB.SetVar("OtherWindow", Window)

	# Back
	Button = GemRB.GetControl (Window, 1)
	GemRB.SetText (Window, Button, 46677)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenLogWindow")

	# Done
	Button = GemRB.GetControl (Window, 0)
	GemRB.SetText (Window, Button, 20636)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenJournalWindow")

	# text area
	Text = GemRB.GetControl (Window, 2)

	for i in range (GemRB.GetJournalSize (0)):
		je = GemRB.GetJournalEntry (0, i)

		if je == None:
			continue

		# FIXME: the date computed here is wrong by approx. time
		#   of the first journal entry compared to journal in
		#   orig. game. So it's probably computed since "awakening"
		#   there instead of start of the day.

		date = str (1 + int (je['GameTime'] / 86400))
		time = str (je['GameTime'])
		
		GemRB.TextAreaAppend (Window, Text, "[color=FFFF00]Day " + date + '  (' + time + "):[/color]", 3*i)
		GemRB.TextAreaAppend (Window, Text, je['Text'], 3*i + 1)
		GemRB.TextAreaAppend (Window, Text, "", 3*i + 2)
			

	GemRB.SetVisible (Window, 1)
	
	GemRB.UnhideGUI()

###################################################
# End of file GUIJRNL.py
