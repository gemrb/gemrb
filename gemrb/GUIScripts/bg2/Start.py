# SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

#ToB start window, precedes the SoA window
import GemRB
import GameCheck

skipVideos = False

def RunStart2(isTOB):
	global skipVideos

	skipIntro = GemRB.GetVar ("SeenIntroVideos")
	if isTOB:
		GemRB.SetMasterScript("BALDUR25", "WORLDM25")
		GemRB.SetVar("oldgame", 0)
		if not skipVideos and skipIntro & 2 == 0:
			GemRB.PlayMovie ("INTRO", 1)
			skipIntro |= 2
		GemRB.LoadMusicPL ("ThemeT.mus", 1)
	elif GameCheck.IsBGEE():
		GemRB.SetMasterScript("BALDUR", "WORLDMAP")
		GemRB.SetVar("oldgame", 1)
		if not skipVideos and skipIntro & 2 == 0:
			GemRB.PlayMovie ("INTRO", 1)
			skipIntro |= 2
		GemRB.LoadMusicPL ("oldthm.mus")
	else:
		GemRB.SetMasterScript("BALDUR", "WORLDMAP")
		GemRB.SetVar("oldgame", 1)
		if not skipVideos and skipIntro & 4 == 0:
			GemRB.PlayMovie ("INTRO15F", 1)
			skipIntro |= 4
		GemRB.LoadMusicPL ("Theme.mus", 1)

	GemRB.SetVar ("SeenIntroVideos", skipIntro)
	if GameCheck.IsBG2Demo():
		GemRB.SetFeature (GF_ALL_STRINGS_TAGGED, True)

	GemRB.SetNextScript("Start2")

def RunStartEE():
	StartWindow = GemRB.LoadWindow (11, "START")
	Label = StartWindow.CreateLabel (0x0fff0000, 0, 0, 1024, 30, "REALMS", "", IE_FONT_SINGLE_LINE | IE_FONT_ALIGN_CENTER)
	Label.SetText (GemRB.Version)

	MusicTable = GemRB.LoadTable("songlist")
	theme = MusicTable.GetValue("33", "RESOURCE")
	GemRB.LoadMusicPL(theme, 1)

	GemRB.SetToken ("SaveDir", "save")

	soaButton = StartWindow.GetControl (1)
	soaButton.OnPress (lambda: RunStart2(False))

	tobButton = StartWindow.GetControl (2)
	tobButton.OnPress (lambda: RunStart2(True))

	bp2Button = StartWindow.GetControl (3)

	for btn in [soaButton, tobButton, bp2Button]:
		btn.OnMouseEnter (lambda btn: btn.SetState (IE_GUI_BUTTON_FAKEPRESSED))
		btn.OnMouseLeave (lambda btn: btn.SetState (IE_GUI_BUTTON_ENABLED))

	# leftmost, Credits, button is missing from the chu
	# maybe just create it and reuse it for quick load, taking the most recent of the various quick saves?

	OptButton = StartWindow.GetControl (4)
	OptButton.SetText (13905)
	OptButton.OnPress (lambda: GemRB.SetNextScript ("StartOpt"))

	ExitButton = StartWindow.GetControl (5)
	ExitButton.SetText (13731)
	ExitButton.OnPress (lambda: GemRB.Quit())
	ExitButton.MakeEscape ()

def RunStartBG1EE():
	StartWindow = GemRB.LoadWindow(7, "START")

	Label = StartWindow.CreateLabel(0x0fff0000, 640, 35, 100, 25, "REALMS", "", IE_FONT_SINGLE_LINE | IE_FONT_ALIGN_CENTER)
	Label.SetText(GemRB.Version)

	# not implemented yet (tutorial, pit, credits, DLCs)
	for btnId in [3, 6, 9, 11]:
		Button = StartWindow.GetControl(btnId)
		Button.SetState(IE_GUI_BUTTON_LOCKED)

	TutorialButton = StartWindow.GetControl(6)
	TutorialButton.SetText(24338)
	CreditsButton = StartWindow.GetControl(3)
	CreditsButton.SetText(24410)

	ExitButton = StartWindow.GetControl(4)
	ExitButton.SetText(13731)
	ExitButton.OnPress(lambda: GemRB.Quit())
	ExitButton.MakeEscape ()

	MainMenuButton = StartWindow.GetControl(2)
	MainMenuButton.SetText(16509)
	MainMenuButton.OnPress(lambda: RunStart2(False))

	Help1 = StartWindow.GetControl(5)
	Help1.SetText(24630)
	Help2 = StartWindow.GetControl(1)
	Help2.SetText(24347)
	Help3 = StartWindow.GetControl(0)
	Help3.SetText(24346)

	DLC = StartWindow.GetControl(9)
	DLC.SetPicture("DLCDN")

	GemRB.LoadMusicPL("oldthm.mus")

def OnLoad():
	global skipVideos

	skipVideos = GemRB.GetVar ("SkipIntroVideos") or 0
	if not skipVideos and not GemRB.GetVar ("SeenIntroVideos"):
		# There may be differences between OS-native versions
		if GameCheck.IsAnyEE ():
			GemRB.PlayMovie ("logo", 1)
		else:
			GemRB.PlayMovie ("BISLOGO", 1)
			GemRB.PlayMovie ("BWDRAGON", 1)
			GemRB.PlayMovie ("WOTC", 1)
		# don't replay the intros on subsequent reentries
		GemRB.SetVar ("SeenIntroVideos", 1)

	# EE BG1 menu
	if GameCheck.IsBGEE ():
		RunStartBG1EE()
		return

	# if not detected ToB, we go right to the main SoA menu
	if not GameCheck.HasTOB():
		RunStart2(False)
		return

	# if EE, show the SoA / ToB / BP2 menu
	if GameCheck.IsBG2EE ():
		RunStartEE()
		return

	# SoA / ToB choice
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
	SoAButton.OnPress (lambda: RunStart2(False))
	ToBButton.OnPress (lambda: RunStart2(True))
	ExitButton.OnPress (lambda: GemRB.Quit())
	StartWindow.Focus()
	GemRB.LoadMusicPL("ThemeT.mus")
	return
