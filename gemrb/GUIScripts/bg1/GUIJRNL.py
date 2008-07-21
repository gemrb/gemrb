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
# $Id$


# GUIJRNL.py - scripts to control journal/diary windows from GUIJRNL winpack

###################################################
import GemRB
import GUICommonWindows
from GUIDefines import *
from GUICommon import CloseOtherWindow
from GUICommonWindows import *

###################################################
JournalWindow = None
OldOptionsWindow = None
OverSlot = None
Chapter = 0
StartTime = 0
StartYear = 0

###################################################
def OpenJournalWindow ():
	global StartTime, StartYear
	global JournalWindow, OptionsWindow, PortraitWindow
	global OldOptionsWindow

	Table = GemRB.LoadTable("YEARS")
	#StartTime is the time offset for ingame time, beginning from the startyear
	StartTime = GemRB.GetTableValue(Table, "STARTTIME", "VALUE") / 4500
	#StartYear is the year of the lowest ingame date to be printed
	StartYear = GemRB.GetTableValue(Table, "STARTYEAR", "VALUE")
	GemRB.UnloadTable(Table)

	if CloseOtherWindow (OpenJournalWindow):
		GemRB.UnloadWindow (JournalWindow)
		GemRB.UnloadWindow (OptionsWindow)
		
		JournalWindow = None
		GemRB.SetVar ("OtherWindow", -1)
		GemRB.SetVisible (0,1)
		GemRB.UnhideGUI ()
		OptionsWindow = OldOptionsWindow
		OldOptionsWindow = None
		return
		
	GemRB.HideGUI ()
	GemRB.SetVisible (0,0)
	
	GemRB.LoadWindowPack ("GUIJRNL", 640, 480)
	JournalWindow = Window = GemRB.LoadWindow (2)
	GemRB.SetVar("OtherWindow", JournalWindow)
	OldOptionsWindow = OptionsWindow
	OptionsWindow = GemRB.LoadWindow (0)
	SetupMenuWindowControls (OptionsWindow, 0, "OpenJournalWindow")
	#saving the original portrait window
	GemRB.SetWindowFrame (OptionsWindow)

	
	Button = GemRB.GetControl (Window, 3)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "JournalPrevSectionPress")

	Button = GemRB.GetControl (Window, 4)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "JournalNextSectionPress")

	Chapter = GemRB.GetGameVar("chapter")
	GemRB.SetVar("TopIndex", 0)
	SetSelectionChangeHandler (UpdateJournalWindow)
	UpdateJournalWindow ()
	
	GemRB.SetVisible (OptionsWindow, 1)
	GemRB.SetVisible (Window, 1)
	GemRB.SetVisible (PortraitWindow, 1)


###################################################
def UpdateJournalWindow ():
	Window = JournalWindow

	# Title
	Title = GemRB.GetControl (Window, 5)
	GemRB.SetText (Window, Title, 16202 + Chapter)

	# text area
	Text = GemRB.GetControl (Window, 1)
	GemRB.TextAreaClear (Window, Text)
	
	for i in range (GemRB.GetJournalSize (Chapter)):
		je = GemRB.GetJournalEntry (Chapter, i)

		if je == None:
			continue

		hours = je['GameTime'] / 4500
		days = int(hours/24)
		year = str (StartYear + int(days/365))
		dayandmonth = StartTime + days%365
		GemRB.SetToken("GAMEDAY", str(days) )
		GemRB.SetToken("HOUR",str(hours%24 ) )
		GemRB.SetVar("DAYANDMONTH",dayandmonth)
		GemRB.SetToken("YEAR",year)
		#GemRB.TextAreaAppend (Window, Text, "[color=FFFF00]"+GemRB.GetString(15980)+"[/color]", 3*i)
		GemRB.TextAreaAppend (Window, Text, GemRB.GetString(15980), 3*i)
		GemRB.TextAreaAppend (Window, Text, je['Text'], 3*i + 1)
		GemRB.TextAreaAppend (Window, Text, "", 3*i + 2)


###################################################
def JournalPrevSectionPress ():
	global Chapter

	if Chapter > 0:
		Chapter = Chapter - 1
		UpdateJournalWindow ()


###################################################
def JournalNextSectionPress ():
	global Chapter

	#if GemRB.GetJournalSize (Chapter + 1) > 0:
	if Chapter < GemRB.GetGameVar("chapter"):
		Chapter = Chapter + 1
		UpdateJournalWindow ()


###################################################
# End of file GUIJRNL.py
