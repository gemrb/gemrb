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
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
#
#
#ToB start window, precedes the SoA window
import GemRB
from GUICommon import HasTOB, LoadCommonTables

StartWindow = 0

def OnLoad():
	global StartWindow, skip_videos

	GemRB.EnableCheatKeys(1)
	skip_videos = GemRB.GetVar ("SkipIntroVideos")
	if not skip_videos and not GemRB.GetVar ("SeenIntroVideos"):
		GemRB.PlayMovie ("BISLOGO", 1)
		GemRB.PlayMovie ("BWDRAGON", 1)
		GemRB.PlayMovie ("WOTC", 1)
		# don't replay the intros on subsequent reentries
		GemRB.SetVar ("SeenIntroVideos", 1)

	# Find proper window border for higher resolutions
	screen_width = GemRB.GetSystemVariable (SV_WIDTH)
	screen_height = GemRB.GetSystemVariable (SV_HEIGHT)
	if screen_width == 800:
		GemRB.LoadWindowFrame("STON08L", "STON08R", "STON08T", "STON08B")
	elif screen_width == 1024:
		GemRB.LoadWindowFrame("STON10L", "STON10R", "STON10T", "STON10B")

	LoadCommonTables ()

	#if not detected tob, we go right to the main menu
	if not HasTOB():
		GemRB.SetMasterScript("BALDUR","WORLDMAP")
		GemRB.SetVar("oldgame",1)
		GemRB.SetNextScript("Start2")
		return

	GemRB.LoadWindowPack("START", 640, 480)
	StartWindow = GemRB.LoadWindowObject(7)
	StartWindow.SetFrame ()
	StartWindow.CreateLabel(0x0fff0000, 0,0,640,30, "REALMS", "", 1)
	Label=StartWindow.GetControl(0x0fff0000)
	Label.SetText(GEMRB_VERSION)

	TextArea = StartWindow.GetControl(0)
	TextArea.SetText(73245)
	TextArea = StartWindow.GetControl(1)
	TextArea.SetText(73246)
	SoAButton = StartWindow.GetControl(2)
	SoAButton.SetText(73247)
	ToBButton = StartWindow.GetControl(3)
	ToBButton.SetText(73248)
	ExitButton = StartWindow.GetControl(4)
	ExitButton.SetText(13731)
	ExitButton.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)
	SoAButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, "SoAPress")
	ToBButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, "ToBPress")
	ExitButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, "ExitPress")
	StartWindow.SetVisible(WINDOW_VISIBLE)
	GemRB.LoadMusicPL("Cred.mus")
	return
	
def SoAPress():
	if StartWindow:
		StartWindow.Unload()
	GemRB.SetMasterScript("BALDUR","WORLDMAP")
	GemRB.SetVar("oldgame",1)
	GemRB.SetNextScript("Start2")
	return

def ToBPress():
	GemRB.SetMasterScript("BALDUR25","WORLDM25")
	GemRB.SetVar("oldgame",0)
	if StartWindow:
		StartWindow.Unload()
	GemRB.SetNextScript("Start2")
	return

def ExitPress():
	GemRB.Quit()
	return
