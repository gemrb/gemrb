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
JournalWindow = None
Chapter = 0
StartTime = 0
StartYear = 0
###################################################

def InitJournalWindow (Window):
	global StartTime, StartYear
	global JournalWindow, Chapter

	Table = GemRB.LoadTable("YEARS")
	#StartTime is the time offset for ingame time, beginning from the startyear
	StartTime = Table.GetValue("STARTTIME", "VALUE") // 4500
	#StartYear is the year of the lowest ingame date to be printed
	StartYear = Table.GetValue("STARTYEAR", "VALUE")
		
	JournalWindow = Window

	TextArea = JournalWindow.GetControl (1)
	JournalWindow.SetEventProxy (TextArea)

	Button = Window.GetControl (3)
	Button.OnPress (JournalPrevSectionPress)
	Button.SetHotKey (GEM_LEFT, 0, True)

	Button = Window.GetControl (4)
	Button.OnPress (JournalNextSectionPress)
	Button.SetHotKey (GEM_RIGHT, 0, True)

	Chapter = GemRB.GetGameVar("chapter")
	UpdateJournalWindow (JournalWindow)
	return

###################################################
def UpdateJournalWindow (Window):
	# Title
	Title = Window.GetControl (5)
	Title.SetText ("[color=FFFF00]" + GemRB.GetString(16202+Chapter) + "[/color]")

	# text area
	Text = Window.GetControl (1)
	Text.SetFlags (IE_GUI_TEXTAREA_AUTOSCROLL)
	Text.Clear ()

	JournalText = ""
	for i in range (GemRB.GetJournalSize (Chapter)):
		je = GemRB.GetJournalEntry (Chapter, i)

		if je == None:
			continue

		hours = je['GameTime'] // 4500
		days = int(hours/24)
		year = str (StartYear + int(days/365))
		dayandmonth = int(StartTime) + days % 365
		GemRB.SetToken("GAMEDAY", str(days) )
		GemRB.SetToken("HOUR",str(hours%24 ) )
		GemRB.SetVar("DAYANDMONTH",dayandmonth)
		GemRB.SetToken("YEAR",year)
		JournalText = JournalText + "[color=FFFF00]" + GemRB.GetString(15980) + "[/color]\n"
		JournalText = JournalText + GemRB.GetString(je['Text']) + "\n\n"
	Text.SetText (JournalText)

ToggleJournalWindow = GUICommonWindows.CreateTopWinLoader(2, "GUIJRNL", GUICommonWindows.ToggleWindow, InitJournalWindow, None, True)
OpenJournalWindow = GUICommonWindows.CreateTopWinLoader(2, "GUIJRNL", GUICommonWindows.OpenWindowOnce, InitJournalWindow, None, True)


###################################################
def JournalPrevSectionPress ():
	global Chapter

	if Chapter > 0:
		Chapter = Chapter - 1
		UpdateJournalWindow (JournalWindow)


###################################################
def JournalNextSectionPress ():
	global Chapter

	#if GemRB.GetJournalSize (Chapter + 1) > 0:
	if Chapter < GemRB.GetGameVar("chapter"):
		Chapter = Chapter + 1
		UpdateJournalWindow (JournalWindow)


###################################################
# End of file GUIJRNL.py
