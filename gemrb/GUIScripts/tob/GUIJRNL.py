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
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
#
# $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/GUIScripts/tob/GUIJRNL.py,v 1.10 2005/05/18 15:37:10 avenger_teambg Exp $


# GUIJRNL.py - scripts to control journal/diary windows from GUIJRNL winpack

import GemRB
import GUICommonWindows
from GUIDefines import *
from GUICommon import CloseOtherWindow
from GUICommonWindows import *

###################################################
JournalWindow = None
LogWindow = None
QuestsWindow = None
PortraitWindow = None
OldPortraitWindow = None

global Section
Section = 1
Chapter = 0
Order = 0
StartTime = 0
StartYear = 0

###################################################
def OpenJournalWindow ():
        global JournalWindow, OptionsWindow, PortraitWindow
        global OldPortraitWindow
	global StartTime, StartYear
	global Chapter

	if CloseOtherWindow (OpenJournalWindow):
		if LogWindow: OpenLogWindow ()
		if QuestsWindow: OpenQuestsWindow ()
		
		GemRB.UnloadWindow (JournalWindow)
                GemRB.UnloadWindow (OptionsWindow)
                GemRB.UnloadWindow (PortraitWindow)

		JournalWindow = None
		GemRB.SetVar ("OtherWindow", -1)
                GemRB.SetVisible (0,1)
                GemRB.UnhideGUI ()
                GUICommonWindows.PortraitWindow = OldPortraitWindow
		OldPortraitWindow = None
		return
		
	Table = GemRB.LoadTable("YEARS")
	StartTime = GemRB.GetTableValue(Table, "STARTTIME", "VALUE") / 4500
	StartYear = GemRB.GetTableValue(Table, "STARTYEAR", "VALUE")
	GemRB.UnloadTable(Table)

	GemRB.HideGUI ()
        GemRB.SetVisible (0,0)

	GemRB.LoadWindowPack ("GUIJRNL", 640, 480)
	JournalWindow = Window = GemRB.LoadWindow (2)
	GemRB.SetVar ("OtherWindow", JournalWindow)
        #saving the original portrait window
        OldPortraitWindow = GUICommonWindows.PortraitWindow
        PortraitWindow = OpenPortraitWindow (0)
        OptionsWindow = GemRB.LoadWindow (0)
        SetupMenuWindowControls (OptionsWindow, 0)
        GemRB.SetWindowFrame (OptionsWindow)

	# prev. chapter
	Button = GemRB.GetControl (JournalWindow, 3)
	GemRB.SetEvent (JournalWindow, Button, IE_GUI_BUTTON_ON_PRESS, "PrevChapterPress")

	# next chapter
	Button = GemRB.GetControl (JournalWindow, 4)
	GemRB.SetEvent (JournalWindow, Button, IE_GUI_BUTTON_ON_PRESS, "NextChapterPress")

	GemRB.SetVar ("Section", Section)
	# Quests
	Button = GemRB.GetControl (JournalWindow, 6)
	GemRB.SetVarAssoc (JournalWindow, Button, "Section", 1)
	GemRB.SetText (JournalWindow, Button, 45485)
	GemRB.SetEvent (JournalWindow, Button, IE_GUI_BUTTON_ON_PRESS, "UpdateLogWindow")

	# Quests completed
	Button = GemRB.GetControl (JournalWindow, 7)
	GemRB.SetVarAssoc (JournalWindow, Button, "Section", 2)
	GemRB.SetText (JournalWindow, Button, 45486)
	GemRB.SetEvent (JournalWindow, Button, IE_GUI_BUTTON_ON_PRESS, "UpdateLogWindow")

	# Journal
	Button = GemRB.GetControl (JournalWindow, 8)
	GemRB.SetVarAssoc (JournalWindow, Button, "Section", 4)
	GemRB.SetText (JournalWindow, Button, 15333)
	GemRB.SetEvent (JournalWindow, Button, IE_GUI_BUTTON_ON_PRESS, "UpdateLogWindow")

	# User
	Button = GemRB.GetControl (JournalWindow, 9)
	GemRB.SetVarAssoc (JournalWindow, Button, "Section", 0)
	GemRB.SetText (JournalWindow, Button, 45487)
	GemRB.SetEvent (JournalWindow, Button, IE_GUI_BUTTON_ON_PRESS, "UpdateLogWindow")

	# Order
	Button = GemRB.GetControl (JournalWindow, 10)
	GemRB.SetText (JournalWindow, Button, 4627)
	GemRB.SetEvent (JournalWindow, Button, IE_GUI_BUTTON_ON_PRESS, "ToggleOrderWindow")

	# Done
	#Button = GemRB.GetControl (JournalWindow, 3)
	#GemRB.SetText (JournalWindow, Button, 20636)
	#GemRB.SetEvent (JournalWindow, Button, IE_GUI_BUTTON_ON_PRESS, "OpenJournalWindow")

	Chapter = GemRB.GetGameVar("chapter")
	UpdateLogWindow ()
        GemRB.SetVisible (OptionsWindow, 1)
        GemRB.SetVisible (Window, 1)
        GemRB.SetVisible (PortraitWindow, 1)
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
	# CurrentChapter
	Label = GemRB.GetControl (JournalWindow, 0x1000000a)
	GemRB.SetText (JournalWindow, Label, 15873)
	print "Chapter ", Chapter, "Section ", Section

	Text = GemRB.GetControl (Window, 1)

	GemRB.TextAreaClear (Window, Text )
	for i in range (GemRB.GetJournalSize (Chapter, Section)):
		je = GemRB.GetJournalEntry (Chapter, i, Section)

		if je == None:
			continue
		hours = je['GameTime'] / 4500
		days = int(hours/24)
		year = str (StartYear + int(days/365))
		dayandmonth = StartTime + days%365
		GemRB.SetToken ("GAMEDAY", str(days) )
		GemRB.SetToken ("HOUR",str(hours%24 ) )
		GemRB.SetVar ("DAYANDMONTH",dayandmonth)
		GemRB.SetToken ("YEAR",year)
		GemRB.TextAreaAppend (Window, Text, "[color=FFFF00]"+GemRB.GetString(15980)+"[/color]", 3*i)
		GemRB.TextAreaAppend (Window, Text, je['Text'], 3*i+1)
		GemRB.TextAreaAppend (Window, Text, "", 3*i + 2)

	return

###################################################
def PrevChapterPress ():
	global Chapter 

	if Chapter > 0:
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
