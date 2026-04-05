# SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later


# GUIJRNL.py - scripts to control journal/diary windows from GUIJRNL winpack

###################################################
import GemRB
import GUICommon
import GUICommonWindows
from GUIDefines import *

###################################################

Chapter = 0
StartTime = 0
StartYear = 0

###################################################
def InitJournalWindow (JournalWindow):
	global StartTime, StartYear, Chapter

	JournalWindow.AddAlias("WIN_JRNL")

	Table = GemRB.LoadTable("YEARS")
	#StartTime is the time offset for ingame time, beginning from the startyear
	StartTime = Table.GetValue("STARTTIME", "VALUE") // 4500
	#StartYear is the year of the lowest ingame date to be printed
	StartYear = Table.GetValue("STARTYEAR", "VALUE")
	
	TextArea = JournalWindow.GetControl (1)
	JournalWindow.SetEventProxy (TextArea)

	Button = JournalWindow.GetControl (3)
	Button = JournalWindow.ReparentSubview (Button, TextArea)
	Button.OnPress (JournalPrevSectionPress)
	Button.SetHotKey (GEM_LEFT, 0, True)

	Button = JournalWindow.GetControl (4)
	Button = JournalWindow.ReparentSubview (Button, TextArea)
	Button.OnPress (JournalNextSectionPress)
	Button.SetHotKey (GEM_RIGHT, 0, True)

	Chapter = GemRB.GetGameVar("chapter")
	return

###################################################
def UpdateJournalWindow (JournalWindow):
	if JournalWindow == None:
		JournalWindow = GemRB.GetView("WIN_JRNL")

	# Title
	Title = JournalWindow.GetControl (5)
	Title.SetColor (ColorWhitish, TA_COLOR_INITIALS)
	Title.SetText (16202 + Chapter)

	# text area
	Text = JournalWindow.GetControl (1)
	Text.Clear ()
	
	for i in range (GemRB.GetJournalSize (Chapter)):
		je = GemRB.GetJournalEntry (Chapter, i)

		if je == None:
			continue

		hours = je['GameTime'] // 4500
		days = int(hours/24)
		year = str (StartYear + int(days/365))
		dayandmonth = int(StartTime + days % 365)
		GemRB.SetToken("GAMEDAY", str(days) )
		GemRB.SetToken("HOUR",str(hours%24 ) )
		GemRB.SetVar("DAYANDMONTH",dayandmonth)
		GemRB.SetToken("YEAR",year)
		Text.Append (GemRB.GetString(15980) + "\n")

		Text.Append (GemRB.GetString(je['Text']) + "\n\n")

ToggleJournalWindow = GUICommonWindows.CreateTopWinLoader(2, "GUIJRNL", GUICommonWindows.ToggleWindow, InitJournalWindow, UpdateJournalWindow)
OpenJournalWindow = GUICommonWindows.CreateTopWinLoader(2, "GUIJRNL", GUICommonWindows.OpenWindowOnce, InitJournalWindow, UpdateJournalWindow)

###################################################
def JournalPrevSectionPress ():
	global Chapter

	if Chapter > 0:
		Chapter = Chapter - 1
		UpdateJournalWindow (None)


###################################################
def JournalNextSectionPress ():
	global Chapter

	#if GemRB.GetJournalSize (Chapter + 1) > 0:
	if Chapter < GemRB.GetGameVar("chapter"):
		Chapter = Chapter + 1
		UpdateJournalWindow (None)


###################################################
# End of file GUIJRNL.py
