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
# $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/GUIScripts/how/GUIJRNL.py,v 1.3 2004/12/04 11:09:11 avenger_teambg Exp $


# GUIJRNL.py - scripts to control journal/diary windows from GUIJRNL winpack

###################################################
import GemRB
from GUIDefines import *
from GUICommon import CloseOtherWindow

###################################################
JournalWindow = None
Chapter = 0
StartTime = 0
StartYear = 0

###################################################
def OpenJournalWindow ():
	global StartTime, StartYear
	global JournalWindow

	Table = GemRB.LoadTable("YEARS")
	#StartTime is the time offset for ingame time, beginning from the startyear
	StartTime = GemRB.GetTableValue(Table, "STARTTIME", "VALUE") / 4500
	#StartYear is the year of the lowest ingame date to be printed
	StartYear = GemRB.GetTableValue(Table, "STARTYEAR", "VALUE")
	GemRB.UnloadTable(Table)

	if CloseOtherWindow (OpenJournalWindow):
		GemRB.HideGUI ()
		GemRB.UnloadWindow (JournalWindow)
		JournalWindow = None
		GemRB.SetVar ("OtherWindow", -1)
		
		GemRB.UnhideGUI ()
		return
		
	GemRB.HideGUI ()
	GemRB.LoadWindowPack ("GUIJRNL")
	JournalWindow = Window = GemRB.LoadWindow (2)
	GemRB.SetVar("OtherWindow", JournalWindow)

	
	Button = GemRB.GetControl (Window, 3)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "JournalPrevSectionPress")

	Button = GemRB.GetControl (Window, 4)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "JournalNextSectionPress")

	UpdateJournalWindow ()
	GemRB.UnhideGUI ()


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
		GemRB.TextAreaAppend (Window, Text, "[color=FFFF00]"+GemRB.GetString(15980)+"[/color]", 3*i)

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
	if Chapter < GemRB.GetVar("chapter"):
		Chapter = Chapter + 1
		UpdateJournalWindow ()


###################################################
# End of file GUIJRNL.py
