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
# $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/GUIScripts/iwd/GUIJRNL.py,v 1.2 2004/10/23 13:01:35 avenger_teambg Exp $


# GUIJRNL.py - scripts to control journal/diary windows from GUIJRNL winpack

###################################################
import GemRB
from GUIDefines import *
from GUICommon import CloseOtherWindow

###################################################
JournalWindow = None
JournalSection = 0

###################################################
def OpenJournalWindow ():
	global JournalWindow

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
	GemRB.SetText (Window, Title, 16202 + JournalSection)

	# text area
	Text = GemRB.GetControl (Window, 1)
	GemRB.TextAreaClear (Window, Text)
	
	for i in range (GemRB.GetJournalSize (JournalSection)):
		je = GemRB.GetJournalEntry (JournalSection, i)

		if je == None:
			continue

		# FIXME: the date computed here is wrong by approx. time
		#   of the first journal entry compared to journal in
		#   orig. game. So it's probably computed since "awakening"
		#   there instead of start of the day.

		#date = str (1 + int (je['GameTime'] / 86400))
		#time = str (je['GameTime'])
		
		#GemRB.TextAreaAppend (Window, Text, "[color=FFFF00]Day " + date + '  (' + time + "):[/color]", 3*i)
		GemRB.TextAreaAppend (Window, Text, je['Text'], 2*i)
		GemRB.TextAreaAppend (Window, Text, "", 2*i + 1)


###################################################
def JournalPrevSectionPress ():
	global JournalSection

	if JournalSection > 0:
		JournalSection = JournalSection - 1
		UpdateJournalWindow ()


###################################################
def JournalNextSectionPress ():
	global JournalSection

	#if GemRB.GetJournalSize (JournalSection + 1) > 0:
	if JournalSection < GemRB.GetVar("chapter"):
		JournalSection = JournalSection + 1
		UpdateJournalWindow ()


###################################################
# End of file GUIJRNL.py
