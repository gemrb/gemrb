# GemRB - Infinity Engine Emulator
# Copyright (C) 2023 The GemRB Project
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

def OnLoad():
	skip_videos = GemRB.GetVar("SkipIntroVideos")
	if not skip_videos and not GemRB.GetVar("SeenIntroVideos"):
		GemRB.PlayMovie("logo", 1)
		GemRB.PlayMovie("intro", 1)
		GemRB.SetVar("SeenIntroVideos", 1)

	StartWindow = GemRB.LoadWindow(11, "START")
	Label = StartWindow.CreateLabel(0x0fff0000, 0, 0, 1024, 30, "REALMS", "", IE_FONT_SINGLE_LINE | IE_FONT_ALIGN_CENTER)
	Label.SetText(GemRB.Version)

	MusicTable = GemRB.LoadTable("songlist")
	theme = MusicTable.GetValue("33", "RESOURCE")
	GemRB.LoadMusicPL(theme, 1)

	GemRB.SetToken ("SaveDir", "save")

	tobButton = StartWindow.GetControl (2)
	tobButton.OnPress (LoadSingle)

def LoadSingle():
	GemRB.SetVar ("PlayMode", 2)
	GemRB.SetMasterScript ("BALDUR25", "WORLDM25")
	GemRB.SetNextScript ("GUILOAD")
