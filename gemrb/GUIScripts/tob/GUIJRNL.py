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
# $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/GUIScripts/tob/GUIJRNL.py,v 1.1 2004/08/26 19:05:41 avenger_teambg Exp $


# GUIJRNL.py - scripts to control journal/diary windows from GUIJRNL winpack

# GUIJRNL:
# 0 - main journal window
# 1 - quests window
# 2 - beasts window
# 3 - log/diary window

###################################################
import GemRB
from GUIDefines import *

#from GUICommonWindows import CloseCommonWindows

###################################################
JournalWindow = None
LogWindow = None
BeastsWindow = None
QuestsWindow = None

# list of all assigned (0) or completed (1) quests
quests = [ [], [] ]
global quests

# whether user has chosen assigned (0) or completed (1) quests
selected_quest_class = 0
global selected_quest_class

###################################################
def OpenJournalWindow ():
	global JournalWindow

	print "Open journal"
	return

	GemRB.HideGUI()

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

	#GemRB.SetVisible (JournalWindow, 1)
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
	

def OnJournalQuestSelect ():
	row = GemRB.GetVar ('SelectedQuest')
	q = quests[selected_quest_class][row]
	GemRB.SetText (QuestsWindow, QuestDesc, int (q[1])) 
	
def OnJournalAssignedPress ():
	global selected_quest_class

	# Assigned Quests
	Label = GemRB.GetControl (QuestsWindow, 0x10000005)
	GemRB.SetText (QuestsWindow, Label, 38585)

	selected_quest_class = 0
	PopulateQuestsList ()
	
def OnJournalCompletedPress ():
	global selected_quest_class

	# Completed Quests
	Label = GemRB.GetControl (QuestsWindow, 0x10000005)
	GemRB.SetText (QuestsWindow, Label, 39527)

	selected_quest_class = 1
	PopulateQuestsList ()


def PopulateQuestsList ():
	GemRB.SetVar ('SelectedQuest', -1)
	GemRB.TextAreaClear (QuestsWindow, QuestsList)
	GemRB.TextAreaClear (QuestsWindow, QuestDesc)

	j = 0
	for q in quests[selected_quest_class]:
		title = GemRB.GetINIQuestsKey (str (q[0]), 'title', '0')
		GemRB.TextAreaAppend (QuestsWindow, QuestsList, '- ', j)
		GemRB.TextAreaAppend (QuestsWindow, QuestsList, int (title), j)
		j = j + 1
	
def EvaluateCondition (var, value, condition):
	cur_value = int (GemRB.GetGameVar (var))

	#print cur_value, condition, value
	if condition == 'EQ':
		return cur_value == int (value)
	elif condition == 'GT':
		return cur_value > int (value)
	elif condition == 'LT':
		return cur_value < int (value)
	else:
		print 'Unknown condition in quests.ini:', condition
		return None

def EvaluateQuest (index):
	tag = str (index)

	endings = int (GemRB.GetINIQuestsKey (tag, 'possibleEndings', '1'))

	for e in range (endings):
		if e == 0:
			suff = ''
		else:
			suff = chr (ord ('A') + e)

		completed = 1
		cc = int (GemRB.GetINIQuestsKey (tag, 'completeChecks' + suff, '0'))
		for i in range (1, cc + 1):
			var = GemRB.GetINIQuestsKey (tag, 'cVar' + suff + str (i), '')
			value = GemRB.GetINIQuestsKey (tag, 'cValue' + suff + str (i), '0')
			condition = GemRB.GetINIQuestsKey (tag, 'cCondition' + suff + str (i), 'EQ')

			completed = completed and EvaluateCondition (var, value, condition)

			#print 'CC:', var, int (GemRB.GetGameVar (var)), condition, value, ': ', completed
			if not completed: break

		if completed:
			#print "COMPLETED", suff
			desc = GemRB.GetINIQuestsKey (tag, 'descCompleted' + suff, '0')
			return (1, desc)
			break


	assigned = 1
	ac = int (GemRB.GetINIQuestsKey (tag, 'assignedChecks', '0'))
	for i in range (1, ac + 1):
		var = GemRB.GetINIQuestsKey (tag, 'aVar' + str (i), '')
		value = GemRB.GetINIQuestsKey (tag, 'aValue' + str (i), '0')
		condition = GemRB.GetINIQuestsKey (tag, 'aCondition' + str (i), 'EQ')

		assigned = assigned and EvaluateCondition (var, value, condition)

		#print 'AC:', var, condition, value, ': ', assigned
		if not assigned: break

	if assigned:
		#print "ASSIGNED"
		desc = GemRB.GetINIQuestsKey (tag, 'descAssigned', '0')
		return (0, desc)

	return None


def EvaluateAllQuests ():
	del quests[0][:]
	del quests[1][:]

	count = int (GemRB.GetINIQuestsKey ('init', 'questcount', '0'))
	for i in range (count):
		res = EvaluateQuest (i)
		if res:
			quests[res[0]].append ((i, res[1]))

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
