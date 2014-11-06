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
PortraitWindow = None
OldPortraitWindow = None
OptionsWindow = None
OldOptionsWindow = None
###################################################
def OpenJournalWindow ():
	global StartTime, StartYear
	global JournalWindow, Chapter
	global PortraitWindow, OptionsWindow
	global OldPortraitWindow, OldOptionsWindow

	Table = GemRB.LoadTable("YEARS")
	#StartTime is the time offset for ingame time, beginning from the startyear
	StartTime = Table.GetValue("STARTTIME", "VALUE") / 4500
	#StartYear is the year of the lowest ingame date to be printed
	StartYear = Table.GetValue("STARTYEAR", "VALUE")

	if GUICommon.CloseOtherWindow (OpenJournalWindow):
		GemRB.HideGUI ()
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
		
	GemRB.HideGUI ()
	GUICommon.GameWindow.SetVisible (WINDOW_INVISIBLE)
	GemRB.LoadWindowPack ("GUIJRNL", 800, 600)
	JournalWindow = Window = GemRB.LoadWindow (2)
	GemRB.SetVar("OtherWindow", JournalWindow.ID)

	#saving the original portrait window
	OldPortraitWindow = GUICommonWindows.PortraitWindow
	PortraitWindow = GUICommonWindows.OpenPortraitWindow ()
	OldOptionsWindow = GUICommonWindows.OptionsWindow
	OptionsWindow = GemRB.LoadWindow (0)
	GUICommonWindows.SetupMenuWindowControls (OptionsWindow, 0, OpenJournalWindow)
	Window.SetFrame ()
	
	Button = Window.GetControl (3)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, JournalPrevSectionPress)

	Button = Window.GetControl (4)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, JournalNextSectionPress)

	Chapter = GemRB.GetGameVar("chapter")
	Window.SetVisible (WINDOW_VISIBLE)
	OptionsWindow.SetVisible (WINDOW_VISIBLE)
	PortraitWindow.SetVisible (WINDOW_VISIBLE)
	# this is here just so we redraw the portrait borders properly in case of reselection
	GUICommonWindows.SetSelectionChangeHandler (UpdateJournalWindow)
	UpdateJournalWindow ()


###################################################
def UpdateJournalWindow ():
	Window = JournalWindow

	# Title
	Title = Window.GetControl (5)
	Title.SetText ("[color=FFFF00]" + GemRB.GetString(16202+Chapter) + "[/color]")

	# text area
	Text = Window.GetControl (1)
	Text.Clear ()
	
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
		Text.Append ("[color=FFFF00]"+GemRB.GetString(15980)+"[/color]", 3*i)
		Text.Append (je['Text'], 3*i + 1)
		Text.Append ("", 3*i + 2)


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
