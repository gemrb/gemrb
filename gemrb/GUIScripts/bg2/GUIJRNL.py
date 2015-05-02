# -*-python-*-
# GemRB - Infinity Engine Emulator
# Copyright (C) 2003-2004 The GemRB Project
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

import GemRB
from GUIDefines import *
import GameCheck
import GUICommon

###################################################
JournalWindow = None
PortraitWindow = None
OldPortraitWindow = None
OldOptionsWindow = None

global Section
Section = 1
Chapter = 0
Order = 0
StartTime = 0
StartYear = 0

###################################################
def OpenJournalWindow ():
	import GUICommonWindows
	global JournalWindow, OptionsWindow, PortraitWindow
	global OldPortraitWindow, OldOptionsWindow
	global StartTime, StartYear
	global Chapter

	if GUICommon.CloseOtherWindow (OpenJournalWindow):
		
		if JournalWindow:
			JournalWindow.Unload ()
		if OptionsWindow:
			OptionsWindow.Unload ()
		if PortraitWindow:
			PortraitWindow.Unload ()

		JournalWindow = None
		GemRB.SetVar ("OtherWindow", -1)
		GUICommon.GameWindow.SetVisible(WINDOW_VISIBLE)
		GemRB.UnhideGUI ()
		GUICommonWindows.PortraitWindow = OldPortraitWindow
		OldPortraitWindow = None
		GUICommonWindows.OptionsWindow = OldOptionsWindow
		OldOptionsWindow = None
		GUICommonWindows.SetSelectionChangeHandler (None)
		return
		
	Table = GemRB.LoadTable("YEARS")
	StartTime = Table.GetValue("STARTTIME", "VALUE") / 4500
	StartYear = Table.GetValue("STARTYEAR", "VALUE")

	GemRB.HideGUI ()
	GUICommon.GameWindow.SetVisible(WINDOW_INVISIBLE)

	GemRB.LoadWindowPack ("GUIJRNL", 640, 480)
	JournalWindow = Window = GemRB.LoadWindow (2)
	GemRB.SetVar ("OtherWindow", JournalWindow.ID)
	#saving the original portrait window
	OldOptionsWindow = GUICommonWindows.OptionsWindow
	OptionsWindow = GemRB.LoadWindow (0)
	GUICommonWindows.MarkMenuButton (OptionsWindow)
	GUICommonWindows.SetupMenuWindowControls (OptionsWindow, 0, OpenJournalWindow)
	OldPortraitWindow = GUICommonWindows.PortraitWindow
	PortraitWindow = GUICommonWindows.OpenPortraitWindow (0)

	# prev. chapter
	Button = JournalWindow.GetControl (3)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, PrevChapterPress)

	# next chapter
	Button = JournalWindow.GetControl (4)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, NextChapterPress)

	GemRB.SetVar ("Section", Section)
	# Quests
	Button = JournalWindow.GetControl (6)
	Button.SetFlags (IE_GUI_BUTTON_RADIOBUTTON, OP_OR)
	Button.SetVarAssoc ("Section", 1)
	Button.SetText (45485)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, UpdateLogWindow)

	# Quests completed
	Button = JournalWindow.GetControl (7)
	Button.SetFlags (IE_GUI_BUTTON_RADIOBUTTON, OP_OR)
	Button.SetVarAssoc ("Section", 2)
	Button.SetText (45486)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, UpdateLogWindow)

	# Journal
	Button = JournalWindow.GetControl (8)
	Button.SetFlags (IE_GUI_BUTTON_RADIOBUTTON, OP_OR)
	Button.SetVarAssoc ("Section", 4)
	Button.SetText (15333)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, UpdateLogWindow)

	# User
	Button = JournalWindow.GetControl (9)
	Button.SetFlags (IE_GUI_BUTTON_RADIOBUTTON, OP_OR)
	Button.SetVarAssoc ("Section", 0)
	Button.SetText (45487)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, UpdateLogWindow)

	# Order
	Button = JournalWindow.GetControl (10)
	Button.SetText (4627)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, ToggleOrderWindow)

	# Done
	#Button = JournalWindow.GetControl (3)
	#Button.SetText (20636)
	#Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, OpenJournalWindow)

	Chapter = GemRB.GetGameVar("chapter")
	if Chapter>65535:
		Chapter=0

	GUICommonWindows.SetSelectionChangeHandler (UpdateLogWindow)
	UpdateLogWindow ()
	OptionsWindow.SetVisible (WINDOW_VISIBLE)
	Window.SetVisible (WINDOW_FRONT)
	PortraitWindow.SetVisible (WINDOW_VISIBLE)
	return

def ToggleOrderWindow ():
	global Order

	if Order:
		Order = 0
	else:
		Order = 1
	UpdateLogWindow ()
	return

def UpdateLogWindow ():

	# text area
	Window = JournalWindow

	Section = GemRB.GetVar("Section")
	GemRB.SetToken ("CurrentChapter", str(Chapter) )
	#sorting method
	# Label = JournalWindow.GetControl (0x1000000a)

	# CurrentChapter
	Label = JournalWindow.GetControl (5)
	Label.SetText (15873)
	print "Chapter ", Chapter, "Section ", Section

	Text = Window.GetControl (1)

	Text.Clear ()
	for i in range (GemRB.GetJournalSize (Chapter, Section)):
		je = GemRB.GetJournalEntry (Chapter, i, Section)

		if je == None:
			continue
		hours = je['GameTime'] / 4500
		days = int(hours/24)
		year = str (StartYear + int(days/365))
		dayandmonth = StartTime + days%365
		GemRB.SetToken ("GAMEDAYS", str(days) ) #Other IE games use "GAMEDAY"
		GemRB.SetToken ("HOUR",str(hours%24 ) )
		GemRB.SetVar ("DAYANDMONTH",dayandmonth)
		GemRB.SetToken ("YEAR",year)

		# each journal entry consists of the title and description
		# but the game displays the entry date between the two
		je2 = GemRB.GetString(je['Text']).split("\n",1)
		JournalTitle = "\n[color=d00000]" + je2[0] + "[/color]\n"
		JournalText = "\n" + je2[1] + "\n"

		Text.Append (JournalTitle + GemRB.GetString(15980))
		Text.Append (JournalText)

	Window.SetVisible (WINDOW_VISIBLE)
	return

###################################################
def PrevChapterPress ():
	global Chapter 
	if GameCheck.IsTOB():
		firstChapter = 0
	else:
		firstChapter = 1

	if Chapter > firstChapter:
		Chapter = Chapter - 1
		GemRB.SetToken ("CurrentChapter", str(Chapter) )
		UpdateLogWindow ()
	return

###################################################
def NextChapterPress ():
	global Chapter

	if Chapter < GemRB.GetGameVar("chapter"):
		Chapter = Chapter + 1
		GemRB.SetToken ("CurrentChapter", str(Chapter) )
		UpdateLogWindow ()
	return

###################################################
# End of file GUIJRNL.py
