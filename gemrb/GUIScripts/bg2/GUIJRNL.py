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
import GUICommonWindows

###################################################

global Section
Section = 1
Chapter = 0
Order = 0
StartTime = 0
StartYear = 0

###################################################

def InitJournalWindow (JournalWindow):
	global StartTime, StartYear, Chapter
	
	JournalWindow.AddAlias("WIN_JRNL")
		
	Table = GemRB.LoadTable("YEARS")
	StartTime = Table.GetValue("STARTTIME", "VALUE") // 4500
	StartYear = Table.GetValue("STARTYEAR", "VALUE")

	# prev. chapter
	Button = JournalWindow.GetControl (3)
	Button.OnPress (PrevChapterPress)

	# next chapter
	Button = JournalWindow.GetControl (4)
	Button.OnPress (NextChapterPress)
	
	def Update():
		UpdateLogWindow(JournalWindow)

	GemRB.SetVar ("Section", Section)

	# Quests completed
	Button = JournalWindow.GetControl (7)
	Button.SetFlags (IE_GUI_BUTTON_RADIOBUTTON, OP_OR)
	Button.SetVarAssoc ("Section", 2)
	Button.SetText (45486)
	Button.OnPress (Update)

	# Journal
	Button = JournalWindow.GetControl (8)
	Button.SetFlags (IE_GUI_BUTTON_RADIOBUTTON, OP_OR)
	Button.SetVarAssoc ("Section", 4)
	Button.SetText (15333)
	Button.OnPress (Update)

	# User
	Button = JournalWindow.GetControl (9)
	Button.SetFlags (IE_GUI_BUTTON_RADIOBUTTON, OP_OR)
	Button.SetVarAssoc ("Section", 0)
	Button.SetText (45487)
	Button.OnPress (Update)

	# Quests
	Button = JournalWindow.GetControl (6)
	Button.SetFlags (IE_GUI_BUTTON_RADIOBUTTON, OP_OR)
	Button.SetVarAssoc ("Section", 1)
	Button.SetText (45485)
	Button.OnPress (Update)

	# Order
	Button = JournalWindow.GetControl (10)
	Button.SetText (4627)
	Button.OnPress (ToggleOrderWindow)
	Chapter = GemRB.GetGameVar("chapter")
	if Chapter > 65535:
		Chapter = 0

	return

def ToggleOrderWindow ():
	global Order
	JournalWindow = GemRB.GetView("WIN_JRNL")
	Button = JournalWindow.GetControl (10)
	if Order:
		Order = 0
		Button.SetText (4627)
	else:
		Order = 1
		Button.SetText (4628)
	UpdateLogWindow (None)
	return

def SortByName (entry):
	je2 = GemRB.GetString(entry['Text']).split("\n",1)
	return je2[0]

def SortByDate (entry):
	return entry["GameTime"]

def GetGameTime (time):
	hours = time // 4500
	days = int(hours / 24)
	year = str (StartYear + int(days / 365))
	dayandmonth = int(StartTime + days % 365)
	GemRB.SetToken ("GAMEDAYS", str(days)) # Other IE games use "GAMEDAY"
	GemRB.SetToken ("HOUR", str(hours % 24))
	GemRB.SetVar ("DAYANDMONTH", dayandmonth)
	GemRB.SetToken ("YEAR", year)
	return GemRB.GetString (15980)

def UpdateLogWindow (JournalWindow):
	global Order
	if JournalWindow == None:
		JournalWindow = GemRB.GetView("WIN_JRNL")

	Section = GemRB.GetVar("Section")
	GemRB.SetToken ("CurrentChapter", str(Chapter))
	# CurrentChapter
	Label = JournalWindow.GetControl (5)
	Label.SetText (15873)
	Label.SetFlags (IE_GUI_VIEW_IGNORE_EVENTS, OP_OR) # needed to prevent scrollbar appearing
	Label.SetColor (ColorWhitish, TA_COLOR_INITIALS)
	print ("Chapter", Chapter, "Section", Section)

	Text = JournalWindow.GetControl (1)
	Text.SetFlags (IE_GUI_TEXTAREA_AUTOSCROLL)

	Text.Clear ()
	EntryList = []
	for i in range (GemRB.GetJournalSize (Chapter, Section)):
		je = GemRB.GetJournalEntry (Chapter, i, Section)
		if je == None:
			continue
		EntryList.append(je)
	#sorting method
	Label = JournalWindow.GetControl (0x1000000a)
	if Order:
		Label.SetText (45228)
		EntryList.sort(key = SortByName)
	else:
		Label.SetText (45202)
		EntryList.sort(key = SortByDate)

	Color = "800000"
	# only the personal journal section used a different title color
	if Section == 4:
		Color = "003d00"
	for i in range(len(EntryList)):
		je = EntryList[i]
		entryTime = GetGameTime (je['GameTime'])

		# each journal entry consists of the title and description
		# but the game displays the entry date between the two
		je2 = GemRB.GetString(je['Text']).split("\n",1)
		JournalTitle = "\n[color=" + Color + "]" + je2[0] + "[/color]\n"
		if len(je2) == 1:
			# broken entry, bg1 style (no title)
			JournalText = ""
		else:
			JournalText = "\n" + je2[1] + "\n"

		Text.Append (JournalTitle + entryTime + JournalText)
	return

ToggleJournalWindow = GUICommonWindows.CreateTopWinLoader(2, "GUIJRNL", GUICommonWindows.ToggleWindow, InitJournalWindow, UpdateLogWindow, GUICommonWindows.DefaultWinPos, True)
OpenJournalWindow = GUICommonWindows.CreateTopWinLoader(2, "GUIJRNL", GUICommonWindows.OpenWindowOnce, InitJournalWindow, UpdateLogWindow, GUICommonWindows.DefaultWinPos, True)

###################################################
def PrevChapterPress ():
	global Chapter 
	if GameCheck.IsTOB():
		firstChapter = 0
	else:
		firstChapter = 1

	if Chapter > firstChapter:
		Chapter = Chapter - 1
		UpdateLogWindow (None)
	return

###################################################
def NextChapterPress ():
	global Chapter

	if Chapter < GemRB.GetGameVar("chapter"):
		Chapter = Chapter + 1
		UpdateLogWindow (None)
	return

###################################################
# End of file GUIJRNL.py
