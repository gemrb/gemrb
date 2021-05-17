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
import GameCheck
from GUIDefines import SV_SAVEPATH

skip_videos = False

def RunStart2(isTOB):
	global skip_videos
	
	if isTOB:
		GemRB.SetMasterScript("BALDUR25","WORLDM25")
		GemRB.SetVar("oldgame",0)
		if not skip_videos and not skip_videos&2:
			GemRB.PlayMovie ("INTRO", 1)
			skip_videos |= 2
	else:
		GemRB.SetMasterScript("BALDUR","WORLDMAP")
		GemRB.SetVar("oldgame",1)
		if not skip_videos and not skip_videos&4:
			GemRB.PlayMovie ("INTRO15F", 1)
			skip_videos |= 4
			
	if GameCheck.IsBG2Demo():
		GemRB.SetFeature (GF_ALL_STRINGS_TAGGED, True)

	GemRB.SetNextScript("Start2")
	MusicTable = GemRB.LoadTable ("songlist")
	# the table has useless rownames, so we can't search for BG2Theme
	theme = MusicTable.GetValue ("33", "RESOURCE")
	GemRB.LoadMusicPL (theme, 1)


def OnLoad():
	global skip_videos

	# migrate mpsave saves if possible and needed
	MigrateSaveDir ()

	skip_videos = GemRB.GetVar ("SkipIntroVideos")
	if not skip_videos and not GemRB.GetVar ("SeenIntroVideos"):
		GemRB.PlayMovie ("BISLOGO", 1)
		GemRB.PlayMovie ("BWDRAGON", 1)
		GemRB.PlayMovie ("WOTC", 1)
		# don't replay the intros on subsequent reentries
		GemRB.SetVar ("SeenIntroVideos", 1)

	#if not detected tob, we go right to the main menu
	if not GameCheck.HasTOB():
		RunStart2(False)
		return

	StartWindow = GemRB.LoadWindow(7, "START")
	Label = StartWindow.CreateLabel(0x0fff0000, 0,0,640,30, "REALMS", "", IE_FONT_SINGLE_LINE | IE_FONT_ALIGN_CENTER)
	Label.SetText(GemRB.Version)

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
	ExitButton.MakeEscape()
	SoAButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, lambda: RunStart2(False))
	ToBButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, lambda: RunStart2(True))
	ExitButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, lambda: GemRB.Quit())
	StartWindow.Focus()
	GemRB.LoadMusicPL("Cred.mus")
	return

def MigrateSaveDir():
	try:
		import os
		savePath = GemRB.GetSystemVariable (SV_SAVEPATH) # this is the parent dir already
		mpSaveDir = os.path.join (savePath, "mpsave")
		saveDir = os.path.join (savePath, "save")

		if not os.path.isdir (mpSaveDir) or not os.access (saveDir, os.W_OK):
			return

		saves = os.listdir (mpSaveDir)
		if len(saves) == 0:
			return

		print "Migrating saves from old location ...",
		if not os.path.isdir (saveDir):
			os.mkdir (saveDir)

		for save in saves:
			# make sure not to overwrite any saves, which is most likely with auto and quicksaves
			newSave = os.path.join (saveDir, save)
			if os.path.isdir (newSave):
				newSave = os.path.join (saveDir, save + "- moved from ToB")
			os.rename (os.path.join (mpSaveDir, save), newSave)

		print "done."
	except ImportError:
		print "No os module, cannot migrate save dir"
