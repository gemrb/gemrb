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
# $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/GUIScripts/pst/GUIJRNL.py,v 1.6 2004/03/15 19:55:41 avenger_teambg Exp $


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

###################################################
def OpenJournalWindow ():
	global JournalWindow

	GemRB.HideGUI()

	if JournalWindow:
		if LogWindow: OpenLogWindow()
		if BeastsWindow: OpenBeastsWindow()
		if QuestsWindow: OpenQuestsWindow()
		
		GemRB.UnloadWindow(JournalWindow)
		JournalWindow = None
		GemRB.SetVar("OtherWindow", -1)
		
		GemRB.UnhideGUI()
		return
		
	GemRB.LoadWindowPack ("GUIJRNL")
	JournalWindow = GemRB.LoadWindow (0)
	GemRB.SetVar("OtherWindow", JournalWindow)

	print "JournalWindow", JournalWindow
	
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
	global JournalWindow, QuestsWindow
	
	GemRB.HideGUI()
	
	if QuestsWindow:
		GemRB.UnloadWindow(QuestsWindow)
		QuestsWindow = None
		
		GemRB.SetVar("OtherWindow", JournalWindow)
		
		GemRB.UnhideGUI()
		return
	
	QuestsWindow = Window = GemRB.LoadWindow (1)
	GemRB.SetVar("OtherWindow", Window)
	
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
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenQuestsWindow")

	# Done
	Button = GemRB.GetControl (Window, 0)
	GemRB.SetText (Window, Button, 20636)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenJournalWindow")

	list = GemRB.GetControl (Window, 1)
	for i in range (GemRB.GetINIBeastsCount ()):
		if GemRB.GameIsBeastKnown (i):
			name = GemRB.GetINIBeastsKey (str (i), 'name', '0')
			GemRB.TextAreaAppend (Window, list, int (name), i)
		

	#GemRB.SetVisible (QuestsWindow, 1)
	GemRB.UnhideGUI()
	
	

###################################################

selected_beast_class = 0
global selected_beast_class

beasts_row2index = []
global beasts_row2index

BeastImage = None
global BeastImage

def OpenBeastsWindow ():
	global JournalWindow, BeastsWindow, BeastsList, BeastImage
	
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
	GemRB.SetEvent (BeastsWindow, Button, IE_GUI_BUTTON_ON_PRESS, "OnJournalPCPress")

	# NPC
	Button = GemRB.GetControl (BeastsWindow, 6)
	GemRB.SetText (BeastsWindow, Button, 20638)
	GemRB.SetEvent (BeastsWindow, Button, IE_GUI_BUTTON_ON_PRESS, "OnJournalNPCPress")

	# Back
	Button = GemRB.GetControl (BeastsWindow, 7)
	GemRB.SetText (BeastsWindow, Button, 46677)
	GemRB.SetEvent (BeastsWindow, Button, IE_GUI_BUTTON_ON_PRESS, "OpenBeastsWindow")

	# Done
	Button = GemRB.GetControl (BeastsWindow, 4)
	GemRB.SetText (BeastsWindow, Button, 20636)
	GemRB.SetEvent (BeastsWindow, Button, IE_GUI_BUTTON_ON_PRESS, "OpenJournalWindow")

	BeastsList = List = GemRB.GetControl (Window, 0)
	GemRB.SetTextAreaSelectable (Window, List, 1)
	GemRB.SetVarAssoc (Window, List, 'SelectedBeast', -1)
	GemRB.SetEvent(Window,List, IE_GUI_TEXTAREA_ON_CHANGE, "OnJournalBeastSelect")
	PopulateBeastsList ()

	GemRB.CreateButton (Window, 8, 20, 20, 281, 441)
	BeastImage = GemRB.GetControl (Window, 8)
	GemRB.SetButtonFlags (Window, BeastImage, IE_GUI_BUTTON_PICTURE | IE_GUI_BUTTON_NO_IMAGE, OP_SET)

	#GemRB.SetVisible (BeastsWindow, 1)
	GemRB.UnhideGUI()

def OnJournalBeastSelect ():
	#print "select"

	Window = BeastsWindow
	List = BeastsList
	
	index2 = GemRB.GetVar ('SelectedBeast')
	index = beasts_row2index[index2]
	
	desc = GemRB.GetINIBeastsKey (str (index), 'desc0', '0')
	image = GemRB.GetINIBeastsKey (str (index), 'imageKnown', '')
	#GemRB.TextAreaAppend (BeastsWindow, BeastsList, int (name), j)

	DescText = GemRB.GetControl (Window, 2)
	GemRB.SetText (Window, DescText, int (desc)) 

	print image
	GemRB.SetButtonPicture (Window, BeastImage, image)
	
def OnJournalPCPress ():
	global selected_beast_class
	# FIXME: it's a hack, because TextArea can't do select events
	if selected_beast_class == 0:
		OnJournalBeastSelect ()
		return

	selected_beast_class = 0
	PopulateBeastsList ()
	
def OnJournalNPCPress ():
	global selected_beast_class
	# FIXME: it's a hack, because TextArea can't do select events
	if selected_beast_class == 1:
		OnJournalBeastSelect ()
		return

	selected_beast_class = 1
	PopulateBeastsList ()


def PopulateBeastsList ():
	GemRB.TextAreaClear (BeastsWindow, BeastsList)
	del beasts_row2index[:]
	
	j = 0
	for i in range (GemRB.GetINIBeastsCount ()):
		if not GemRB.GameIsBeastKnown (i):
			continue
		
		if selected_beast_class != int (GemRB.GetINIBeastsKey (str (i), 'class', '0')):
			continue
		
		name = GemRB.GetINIBeastsKey (str (i), 'name', '0')
		#klass = GemRB.GetINIBeastsKey (str (i), 'class', '0')
		#desc = GemRB.GetINIBeastsKey (str (i), 'desc0', '0')
		#image = GemRB.GetINIBeastsKey (str (i), 'imageKnown', '')
		GemRB.TextAreaAppend (BeastsWindow, BeastsList, int (name), j)
		beasts_row2index.append (i)
		j = j + 1
	
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
		
		GemRB.TextAreaAppend (Window, Text, "[color=FFFF00]Time: " + str (je['Time']) + ":[/color]", 3*i)
		GemRB.TextAreaAppend (Window, Text, je['Text'], 3*i + 1)
		GemRB.TextAreaAppend (Window, Text, "", 3*i + 2)
			

	GemRB.SetVisible (Window, 1)
	
	GemRB.UnhideGUI()

###################################################
# End of file GUIJRNL.py
