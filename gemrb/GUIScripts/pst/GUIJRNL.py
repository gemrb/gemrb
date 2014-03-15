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
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#


# GUIJRNL.py - scripts to control journal/diary windows from GUIJRNL winpack

# GUIJRNL:
# 0 - main journal window
# 1 - quests window
# 2 - beasts window
# 3 - log/diary window

###################################################
import GemRB
import GUICommon
import GUICommonWindows
from GUIDefines import *

###################################################
JournalWindow = None
LogWindow = None
BeastsWindow = None
QuestsWindow = None

# list of all assigned (0) or completed (1) quests
global quests
quests = [ [], [] ]

# whether user has chosen assigned (0) or completed (1) quests
global selected_quest_class
selected_quest_class = 0

# list of all PC (0) or NPC (1) beasts/creatures
global beasts
beasts = [ [], [] ]

# whether user has chosen PC (0) or NPC (1) beasts
global selected_beast_class
selected_beast_class = 0

global BeastImage
BeastImage = None
StartTime = 0

###################################################
def OpenJournalWindow ():
	global JournalWindow, PortraitWindow, ActionsWindow
	global StartTime

	Table = GemRB.LoadTable("YEARS")
	StartTime = Table.GetValue("STARTTIME", "VALUE")
	
	if GUICommon.CloseOtherWindow (OpenJournalWindow):
		if LogWindow: OpenLogWindow()
		if BeastsWindow: OpenBeastsWindow()
		if QuestsWindow: OpenQuestsWindow()
		
		GemRB.HideGUI ()
		
		if JournalWindow:
			JournalWindow.Unload()
		#making the portraitwindow visible again
		GUICommonWindows.EnableAnimatedWindows ()
		GemRB.SetVar ("OtherWindow", -1)
		PortraitWindow = None
		ActionsWindow = None
		JournalWindow = None
		
		GemRB.UnhideGUI ()
		return
		
	GemRB.HideGUI ()
	GemRB.LoadWindowPack ("GUIJRNL")
	JournalWindow = GemRB.LoadWindow (0)
	GemRB.SetVar ("OtherWindow", JournalWindow.ID)
	GUICommonWindows.DisableAnimatedWindows ()

	# Quests
	Button = JournalWindow.GetControl (0)
	Button.SetText (20430)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, OpenQuestsWindow)

	# Beasts
	Button = JournalWindow.GetControl (1)
	Button.SetText (20634)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, OpenBeastsWindow)

	# Journal
	Button = JournalWindow.GetControl (2)
	Button.SetText (20635)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, OpenLogWindow)

	# Done
	Button = JournalWindow.GetControl (3)
	Button.SetText (20636)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, OpenJournalWindow)
	Button.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)
	Button.SetStatus (IE_GUI_CONTROL_FOCUSED)

	#JournalWindow.SetVisible (WINDOW_VISIBLE)
	GemRB.UnhideGUI()
	


###################################################
def OpenQuestsWindow ():
	global JournalWindow, QuestsWindow, QuestsList, QuestDesc
	
	GemRB.HideGUI()
	
	if QuestsWindow:
		if QuestsWindow:
			QuestsWindow.Unload()
		QuestsWindow = None
		
		GemRB.SetVar ("OtherWindow", JournalWindow.ID)
		
		GemRB.UnhideGUI()
		return
	
	QuestsWindow = Window = GemRB.LoadWindow (1)
	GemRB.SetVar ("OtherWindow", Window.ID)
	
	# Assigned
	Button = Window.GetControl (8)
	Button.SetText (39433)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, OnJournalAssignedPress)

	# Completed
	Button = Window.GetControl (9)
	Button.SetText (39434)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, OnJournalCompletedPress)

	# Back
	Button = Window.GetControl (5)
	Button.SetText (46677)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, OpenQuestsWindow)
	Button.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)
	Button.SetStatus (IE_GUI_CONTROL_FOCUSED)

	# Done
	Button = Window.GetControl (0)
	Button.SetText (20636)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, OpenJournalWindow)

	QuestsList = List = Window.GetControl (1)
	List.SetVarAssoc ('SelectedQuest', -1)
	List.SetEvent (IE_GUI_TEXTAREA_ON_CHANGE, OnJournalQuestSelect)

	QuestDesc = Window.GetControl (3)

	EvaluateAllQuests ()
	PopulateQuestsList ()

	#QuestsWindow.SetVisible (WINDOW_VISIBLE)
	GemRB.UnhideGUI()
	

def OnJournalQuestSelect ():
	row = GemRB.GetVar ('SelectedQuest')
	q = quests[selected_quest_class][row]
	QuestDesc.SetText (int (q[1])) 
	
def OnJournalAssignedPress ():
	global selected_quest_class

	# Assigned Quests
	Label = QuestsWindow.GetControl (0x10000005)
	Label.SetText (38585)

	selected_quest_class = 0
	PopulateQuestsList ()
	
def OnJournalCompletedPress ():
	global selected_quest_class

	# Completed Quests
	Label = QuestsWindow.GetControl (0x10000005)
	Label.SetText (39527)

	selected_quest_class = 1
	PopulateQuestsList ()


def PopulateQuestsList ():
	GemRB.SetVar ('SelectedQuest', -1)
	QuestDesc.Clear ()

	opts = []
	j = 0
	for q in quests[selected_quest_class]:
		title = GemRB.GetINIQuestsKey (str (q[0]), 'title', '0')
		opts.append('- ' + GemRB.GetString(int(title))
		j = j + 1
	QuestsList.SetOptions(opts)
	
def EvaluateCondition (var, value, condition):
	cur_value = int (GemRB.GetGameVar (var))

	#print cur_value, condition, value
	if condition == 'EQ':
		return cur_value == int (value)
	if condition == 'NE':
		return cur_value != int (value)
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

def OpenBeastsWindow ():
	global JournalWindow, BeastsWindow, BeastsList, BeastImage, BeastDesc
	
	GemRB.HideGUI()
	
	if BeastsWindow:
		if BeastsWindow:
			BeastsWindow.Unload()
		BeastsWindow = None
		
		GemRB.SetVar ("OtherWindow", JournalWindow.ID)
		
		GemRB.UnhideGUI()
		return
	
	BeastsWindow = Window = GemRB.LoadWindow (2)
	GemRB.SetVar ("OtherWindow", BeastsWindow.ID)

	# PC
	Button = BeastsWindow.GetControl (5)
	Button.SetText (20637)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, OnJournalPCPress)

	# NPC
	Button = BeastsWindow.GetControl (6)
	Button.SetText (20638)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, OnJournalNPCPress)

	# Back
	Button = BeastsWindow.GetControl (7)
	Button.SetText (46677)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, OpenBeastsWindow)
	Button.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)
	Button.SetStatus (IE_GUI_CONTROL_FOCUSED)

	# Done
	Button = BeastsWindow.GetControl (4)
	Button.SetText (20636)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, OpenJournalWindow)

	BeastsList = List = Window.GetControl (0)
	List.SetVarAssoc ('SelectedBeast', -1)
	List.SetEvent(IE_GUI_TEXTAREA_ON_CHANGE, OnJournalBeastSelect)

	Window.CreateButton (8, 19, 19, 281, 441)
	BeastImage = Window.GetControl (8)
	BeastImage.SetFlags (IE_GUI_BUTTON_PICTURE | IE_GUI_BUTTON_NO_IMAGE, OP_SET)

	BeastDesc = Window.GetControl (2)
	
	EvaluateAllBeasts ()
	PopulateBeastsList ()
	
	GemRB.UnhideGUI()
	return

def OnJournalBeastSelect ():
	row = GemRB.GetVar ('SelectedBeast')
	b = beasts[selected_beast_class][row]
	
	desc = GemRB.GetINIBeastsKey (str (b), 'desc0', '0')
	BeastDesc.SetText (int (desc)) 

	image = GemRB.GetINIBeastsKey (str (b), 'imageKnown', '')
	BeastImage.SetPicture (image)
	
def OnJournalPCPress ():
	global selected_beast_class

	selected_beast_class = 0
	PopulateBeastsList ()
	
def OnJournalNPCPress ():
	global selected_beast_class

	selected_beast_class = 1
	PopulateBeastsList ()


def PopulateBeastsList ():
	GemRB.SetVar ('SelectedBeast', -1)
	BeastDesc.Clear ()
	BeastImage.SetPicture ('default')

	opts = []
	j = 0
	for b in beasts[selected_beast_class]:
		name = GemRB.GetINIBeastsKey (str (b), 'name', '0')
		opts.append(GemRB.GetString(int (name)))
		j = j + 1
	BeastsList.SetOptions(opts)

def EvaluateAllBeasts ():
	del beasts[0][:]
	del beasts[1][:]

	count = int (GemRB.GetINIBeastsKey ('init', 'beastcount', '0'))
	
	j = 0
	for i in range (count):
		if not GemRB.GameIsBeastKnown (i):
			continue

		klass = int (GemRB.GetINIBeastsKey (str (i), 'class', '0'))
		beasts[klass].append (i)
		
		j = j + 1


###################################################

def OpenLogWindow ():
	global JournalWindow, LogWindow
	
	GemRB.HideGUI()
	
	if LogWindow:
		if LogWindow:
			LogWindow.Unload()
		LogWindow = None
		
		GemRB.SetVar ("OtherWindow", JournalWindow.ID)
		
		GemRB.UnhideGUI()
		return
	
	LogWindow = Window = GemRB.LoadWindow (3)
	GemRB.SetVar ("OtherWindow", Window.ID)

	# Back
	Button = Window.GetControl (1)
	Button.SetText (46677)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, OpenLogWindow)
	Button.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)
	Button.SetStatus (IE_GUI_CONTROL_FOCUSED)

	# Done
	Button = Window.GetControl (0)
	Button.SetText (20636)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, OpenJournalWindow)

	# text area
	Text = Window.GetControl (2)

	for i in range (GemRB.GetJournalSize (0)):
		je = GemRB.GetJournalEntry (0, i)

		if je == None:
			continue

		# FIXME: the date computed here is wrong by approx. time
		#   of the first journal entry compared to journal in
		#   orig. game. So it's probably computed since "awakening"
		#   there instead of start of the day.
		# FIXME: use strref 19310 or 64192

		gt = StartTime + je["GameTime"]
		dt = int (gt/86400)
		date = str (1 + dt)
		#time = str (gt - dt*86400)
		
		Text.Append ("[color=FFFF00]"+GemRB.GetString(19310)+" "+date+"[/color]", 3*i)
		Text.Append (je['Text'], 3*i + 1)
		Text.Append ("", 3*i + 2)
			
	Window.SetVisible (WINDOW_VISIBLE)
	
	GemRB.UnhideGUI()

###################################################
# End of file GUIJRNL.py
