# GemRB - Infinity Engine Emulator
# Copyright (C) 2010 The GemRB Project
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
#################### Interchapter text screen functions #################

import GemRB
from ie_restype import RES_2DA
from ie_sounds import CHAN_GUI, SND_SPEECH, GEM_SND_VOL_AMBIENTS
from GUIDefines import *
import GameCheck

TextScreen = None
TextArea = None
Row = 1
TableName = None

def FindTextRow (Table):
	global Row

	#this is still not the full implementation, but the original engine never used this stuff
	Row = Table.GetRowIndex("DEFAULT")
	if Table.GetValue (Row, 1)== -1:
		if GemRB.GameGetReputation() >= 100:
			Row = Table.GetRowIndex("GOOD_REPUTATION")
		else:
			Row = Table.GetRowIndex("BAD_REPUTATION")
	return

def StartTextScreen ():
	global TextScreen, TextArea, TableName, Row

	# for easier development
	if GameCheck.IsGemRBDemo () and GemRB.GetVar ("SkipIntroVideos") and not GemRB.GetGameVar ("completed"):
		GemRB.GamePause (0, 3)
		return

	GemRB.GamePause (1, 3)
	ToggleAmbients (0)

	ID = -1
	MusicName = "*"
	Message = 17556   # default: Paused for chapter text
	TableName = GemRB.GetGameString (STR_TEXTSCREEN)

	#iwd2/bg2 has no separate music
	if GameCheck.IsIWD1():
		if TableName == "":
			MusicName = "chap0"
		else:
			MusicName = "chap1"
		TableName = "chapters"
	elif GameCheck.IsIWD2():
		TableName = "chapters"
	elif GameCheck.IsGemRBDemo ():
		#TODO: set MusicName
		TableName = "chapters"
		Message = "*"

	if TableName == "":
		EndTextScreen ()
		return

	if GemRB.HasResource ("textscrn", RES_2DA, 1):
		TextTable = GemRB.LoadTable ("textscrn", 1)
		if TextTable != None:
			TxtRow = TextTable.GetRowIndex (TableName)
			if TxtRow is not None:
				ID = TextTable.GetValue (TxtRow, 0)
				MusicName = TextTable.GetValue (TxtRow, 1)
				Message = TextTable.GetValue (TxtRow, 2)

	if Message != "*":
		GemRB.DisplayString (Message, ColorRed)

	Table = GemRB.LoadTable (TableName)
	if GameCheck.IsBG2OrEE ():
		LoadPic = Table.GetValue (-1, -1)
		if LoadPic.startswith ("*"): # BG2 epilogues
			ID = 63
			LoadPic = LoadPic.replace ("*", "")
		else:
			ID = 62
	elif ID == -1:
		#default: try to determine ID from current chapter
		ID = GemRB.GetGameVar("CHAPTER") & 0x7fffffff
		Chapter = ID + 1

	# Textscreen should always stop currently playing music before starting
	# to play any chapter music.
	GemRB.HardEndPL ()

	if MusicName != "*":
		GemRB.LoadMusicPL (MusicName + ".mus")

	TextScreen = GemRB.LoadWindow (ID, "GUICHAP")
	TextArea = TextScreen.GetControl (2)
	TextArea.SetFlags (IE_GUI_VIEW_IGNORE_EVENTS, OP_OR)
	TextArea.SetColor (ColorWhitish, TA_COLOR_INITIALS)

	GameWin = GemRB.GetView("GAMEWIN")
	GameWin.SetDisabled(True)

	if GameCheck.IsBG1():
		#these suckers couldn't use a fix row
		FindTextRow (Table)
	elif GameCheck.IsBG2OrEE ():
		FindTextRow (Table)
		if LoadPic == "NONE": # (at least) tutu tries to load a nonexisting image
			LoadPic = None
		if LoadPic != "":
			if ID == 63:
				#only for BG2 epilogue windows
				PicButton = TextScreen.GetControl (4)
				PicButton.SetPicture (LoadPic)
				PicButton.SetState (IE_GUI_BUTTON_LOCKED)
			else:
				TextScreen.SetBackground (LoadPic)
	else:
		Row = Chapter

	#caption
	Value = Table.GetValue (Row, 0)
	#don't display the fake -1 string (No caption in toscst.2da)
	Label = TextScreen.GetControl (0x10000000)
	if Label and Value!="NONE" and Value>0:
		Label.SetText (Value)

	#done
	Button=TextScreen.GetControl (0)
	if GameCheck.IsGemRBDemo ():
		Button.SetText (84)
	else:
		Button.SetText (11973)
	Button.OnPress (EndTextScreen)
	Button.MakeDefault()

	#replay
	Button=TextScreen.GetControl (3)
	if GameCheck.IsGemRBDemo ():
		Button.SetText (85)
	else:
		Button.SetText (16510)
	Button.OnPress (ReplayTextScreen)

	#if this was opened from somewhere other than game control close that window
	import GUICommonWindows
	GUICommonWindows.CloseTopWindow ()

	TextScreen.ShowModal (MODAL_SHADOW_NONE)

	ReplayTextScreen()
	return

# FIXME: move to GUIScript as a PauseAmbients method that works directly with the AmbientMgr
AmbientVolume = 0
def ToggleAmbients (state):
	global AmbientVolume

	if state:
		GemRB.SetVar ("Volume Ambients", AmbientVolume)
	else:
		AmbientVolume = GemRB.GetVar ("Volume Ambients")
		GemRB.SetVar ("Volume Ambients", 0)

	GemRB.UpdateVolume (GEM_SND_VOL_AMBIENTS)

def EndTextScreen ():
	global TextScreen, TableName

	if TextScreen:
		TextScreen.Close ()
		GemRB.HardEndPL ()
		GemRB.PlaySound(None, CHAN_GUI, 0, 0, SND_SPEECH)

	GameWin = GemRB.GetView("GAMEWIN")
	GameWin.SetDisabled(False)
	ToggleAmbients (1)
	GemRB.GamePause (0, 3)

	if TableName == "25ecred":
		GemRB.SetNextScript("QuitGame")
	return

def ReplayTextScreen ():
	global TextArea, TableName, Row

	GemRB.PlaySound(None, CHAN_GUI, 0, 0, SND_SPEECH)

	Table = GemRB.LoadTable (TableName)
	Count = Table.GetColumnCount (Row)

	#hack for messy chapters.2da in IWD
	#note: iwd doesn't use TextScreen, only IncrementChapter
	if GameCheck.IsIWD1() or GameCheck.IsIWD2():
		Count = 2

	text = ""
	for i in range(1, Count):
		# flag value of 14 = IE_STR_SOUND|IE_STR_SPEECH/GEM_SND_SPEECH|GEM_SND_QUEUE
		text += "\n\n" if not GameCheck.IsGemRBDemo () else ""
		text += GemRB.GetString(Table.GetValue (Row, i), 14)

	TextArea.SetChapterText (text)

	return
