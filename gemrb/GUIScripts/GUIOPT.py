# -*-python-*-
# GemRB - Infinity Engine Emulator
# Copyright (C) 2012 The GemRB Project
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#

# GUIOPT.py - scripts to control options windows mostly from the GUIOPT winpack:
# 0 - Main options window (peacock tail)
# 1 - Video options window
# 2 - msg win with 1 button
# 3 - msg win with 2 buttons
# 4 - msg win with 3 buttons
# 5 - Audio options window
# 6 - Gameplay options window
# 8 - Feedback options window
# 9 - Autopause options window

###################################################
import GemRB
import GUICommon
import GUICommonWindows
import GUISAVE
import GUIOPTControls
from GUIDefines import *

###################################################
GameOptionsWindow = None # not in PST
PortraitWindow = None # not in BG1 or PST
OldPortraitWindow = None #not in BG1 or PST
OptionsWindow = None
OldOptionsWindow = None
HelpTextArea = None

LoadMsgWindow = None
QuitMsgWindow = None
MovieWindow = None

if GUICommon.GameIsBG1():
	SubOptionsWindow = None
	SubSubOptionsWindow = None
	HelpTextArea2 = None
else:
	# just an alias to keep our logic from being plagued by too many GUICommon.GameIsBG1() checks
	HelpTextArea2 = HelpTextArea

if GUICommon.GameIsIWD2():
	WIDTH = 800
	HEIGHT = 600
else:
	WIDTH = 640
	HEIGHT = 480

###################################################
def CloseOptionsWindow ():
	global GameOptionsWindow, OptionsWindow, PortraitWindow
	global OldPortraitWindow, OldOptionsWindow

	if GameOptionsWindow == None:
		return

	if GameOptionsWindow:
		GameOptionsWindow.Unload ()
	if OptionsWindow:
		OptionsWindow.Unload ()
	if PortraitWindow:
		PortraitWindow.Unload ()

	GameOptionsWindow = None
	GemRB.SetVar ("OtherWindow", -1)
	GUICommon.GameWindow.SetVisible(WINDOW_VISIBLE)
	GemRB.UnhideGUI ()
	GUICommonWindows.OptionsWindow = OldOptionsWindow
	OldOptionsWindow = None
	GUICommonWindows.PortraitWindow = OldPortraitWindow
	OldPortraitWindow = None
	return

###################################################
def OpenOptionsWindow ():
	"""Open main options window"""

	global GameOptionsWindow, OptionsWindow, PortraitWindow
	global OldPortraitWindow, OldOptionsWindow

	if GUICommon.CloseOtherWindow (OpenOptionsWindow):
		CloseOptionsWindow()
		return

	GUICommon.GameWindow.SetVisible(WINDOW_INVISIBLE)
	if GUICommon.GameIsBG1():
		GUICommonWindows.SetSelectionChangeHandler (None)

	GemRB.LoadWindowPack ("GUIOPT", WIDTH, HEIGHT)
	GameOptionsWindow = Window = GemRB.LoadWindow (2)
	GemRB.SetVar ("OtherWindow", GameOptionsWindow.ID)

	if OldOptionsWindow == None:
		OldOptionsWindow = GUICommonWindows.OptionsWindow
		OptionsWindow = GemRB.LoadWindow (0)
		if GUICommon.GameIsBG2():
			GUICommonWindows.MarkMenuButton (OptionsWindow)
		GUICommonWindows.SetupMenuWindowControls (OptionsWindow, 0, OpenOptionsWindow)
		OptionsWindow.SetFrame ()
		if not GUICommon.GameIsBG1(): #not in PST either, but it has its own OpenOptionsWindow()
			OptionsWindow.SetFrame ()
			#saving the original portrait window
			OldPortraitWindow = GUICommonWindows.PortraitWindow
			PortraitWindow = GUICommonWindows.OpenPortraitWindow (0)

	# Return to Game
	Button = Window.GetControl (11)
	Button.SetText (10308)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, OpenOptionsWindow)
	Button.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)

	# Quit Game
	Button = Window.GetControl (10)
	Button.SetText (13731)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, OpenQuitMsgWindow)

	# Load Game
	Button = Window.GetControl (5)
	Button.SetText (13729)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, OpenLoadMsgWindow)

	# Save Game
	Button = Window.GetControl (6)
	Button.SetText (13730)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, OpenSaveMsgWindow)

	# Video Options
	Button = Window.GetControl (7)
	Button.SetText (17162)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, OpenVideoOptionsWindow)

	# Audio Options
	Button = Window.GetControl (8)
	Button.SetText (17164)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, OpenAudioOptionsWindow)

	# Gameplay Options
	Button = Window.GetControl (9)
	Button.SetText (17165)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, OpenGameplayOptionsWindow)

	# game version, e.g. v1.1.0000
	VersionLabel = Window.GetControl (0x1000000b)
	VersionLabel.SetText (GEMRB_VERSION)

	if GUICommon.GameIsIWD2():
		# Keyboard shortcuts
		KeyboardButton = Window.GetControl (13)
		KeyboardButton.SetText (33468)
		KeyboardButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, None) #TODO: OpenKeyboardWindow

		# Movies
		MoviesButton = Window.GetControl (14)
		MoviesButton.SetText (15415)
		MoviesButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, OpenMovieWindow)

	OptionsWindow.SetVisible (WINDOW_VISIBLE)
	Window.SetVisible (WINDOW_VISIBLE)
	if not GUICommon.GameIsBG1():
		PortraitWindow.SetVisible (WINDOW_VISIBLE)
	return

###################################################

def CloseVideoOptionsWindow ():
	if GUICommon.GameIsBG1():
		global SubOptionsWindow
		if SubOptionsWindow:
			SubOptionsWindow.Unload()
		SubOptionsWindow = None
	else:
		# no idea why this is here twice
		OpenOptionsWindow ()
		OpenOptionsWindow ()

	return

def OpenVideoOptionsWindow ():
	"""Open video options window"""
	global HelpTextArea
	Window = None

	if GUICommon.GameIsBG1():
		global SubOptionsWindow
		Window = SubOptionsWindow
	else:
		global GameOptionsWindow
		Window = GameOptionsWindow

	if Window:
		Window.Unload ()

	Window = GemRB.LoadWindow (6)

	HelpTextArea = GUIOPTControls.OptHelpText ('VideoOptions', Window, 33, 18038)

	GUIOPTControls.OptDone (CloseVideoOptionsWindow, Window, 21)
	GUIOPTControls.OptCancel (CloseVideoOptionsWindow, Window, 32)

	GUIOPTControls.OptSlider (DisplayHelpBrightness, Window, 3, 'Brightness Correction', 4)
	GUIOPTControls.OptSlider (DisplayHelpContrast, Window, 22, 'Gamma Correction', 1)

	GUIOPTControls.OptRadio (DisplayHelpBPP, Window, 5, 37, 'BitsPerPixel', 16)
	GUIOPTControls.OptRadio (DisplayHelpBPP, Window, 6, 37, 'BitsPerPixel', 24)
	GUIOPTControls.OptRadio (DisplayHelpBPP, Window, 7, 37, 'BitsPerPixel', 32)

	GUIOPTControls.OptCheckbox (DisplayHelpFullScreen, Window, 9, 38, 'Full Screen', 1)

	GUIOPTControls.OptCheckboxNoCallback (20620, HelpTextArea, Window, 51, 50, 'Translucent Shadows')
	GUIOPTControls.OptCheckboxNoCallback (18004, HelpTextArea, Window, 40, 44, 'SoftMirrorBlt')
	GUIOPTControls.OptCheckboxNoCallback (18006, HelpTextArea, Window, 41, 46, 'SoftSrcKeyBlt')
	GUIOPTControls.OptCheckboxNoCallback (18007, HelpTextArea, Window, 42, 48, 'SoftBltFast')

	if GUICommon.GameIsBG1():
		SubOptionsWindow = Window
	else:
		GameOptionsWindow = Window

	Window.ShowModal (MODAL_SHADOW_GRAY)
	return

def DisplayHelpFullScreen ():
	HelpTextArea.SetText (18000)
	GemRB.SetFullScreen (GemRB.GetVar("Full Screen"))

def DisplayHelpBPP ():
	HelpTextArea.SetText (17205)

def DisplayHelpBrightness ():
	HelpTextArea.SetText (17203)
	SetGfxCorrection ()

def DisplayHelpContrast ():
	HelpTextArea.SetText (17204)
	SetGfxCorrection ()

# different games have different slider ranges, but the engine wants:
# gamma: 0-5
# brightness: 0-40
def SetGfxCorrection ():
	Brightness = GemRB.GetVar("Brightness Correction")
	Gamma = GemRB.GetVar("Gamma Correction")
	if GUICommon.GameIsIWD2(): # 10/11 ticks
		Gamma /= 2
	# TODO: check if pst really has them quintupled

	GemRB.SetGamma (Brightness, Gamma)

###################################################

def CloseAudioOptionsWindow ():
	if GUICommon.GameIsBG1():
		global SubOptionsWindow

		if SubOptionsWindow:
			SubOptionsWindow.Unload()
		SubOptionsWindow = None
	else:
		# no idea why this is here twice
		OpenOptionsWindow ()
		OpenOptionsWindow ()
	return

###################################################

def OpenAudioOptionsWindow ():
	"""Open audio options window"""
	global HelpTextArea

	Window = None
	if GUICommon.GameIsBG1():
		global SubOptionsWindow
		Window = SubOptionsWindow
	else:
		global GameOptionsWindow
		Window = GameOptionsWindow

	if Window:
		Window.Unload ()

	Window = GemRB.LoadWindow (7)
	HelpTextArea = GUIOPTControls.OptHelpText ('AudioOptions', Window, 14, 18040)

	GUIOPTControls.OptDone (CloseAudioOptionsWindow, Window, 24)
	GUIOPTControls.OptCancel (CloseAudioOptionsWindow, Window, 25)
	GUIOPTControls.OptButton (OpenCharacterSoundsWindow, Window, 13, 17778)

	GUIOPTControls.OptSlider (DisplayHelpAmbientVolume, Window, 1, 'Volume Ambients', 10)
	GUIOPTControls.OptSliderNoCallback (18009, HelpTextArea, Window, 2, 'Volume SFX', 10)
	GUIOPTControls.OptSliderNoCallback (18010, HelpTextArea, Window, 3, 'Volume Voices', 10)
	GUIOPTControls.OptSlider (DisplayHelpMusicVolume, Window, 4, 'Volume Music', 10)
	GUIOPTControls.OptSliderNoCallback (18012, HelpTextArea, Window, 22, 'Volume Movie', 10)

	GUIOPTControls.OptCheckboxNoCallback (18022, HelpTextArea, Window, 26, 28, 'Environmental Audio')

	if GUICommon.GameIsBG1():
		SubOptionsWindow = Window
	else:
		GameOptionsWindow = Window

	Window.ShowModal (MODAL_SHADOW_GRAY)
	return

def DisplayHelpAmbientVolume ():
	HelpTextArea.SetText (18008)
	GemRB.UpdateAmbientsVolume ()

def DisplayHelpMusicVolume ():
	HelpTextArea.SetText (18011)
	GemRB.UpdateMusicVolume ()

###################################################

def CloseCharacterSoundsWindow ():
	if GUICommon.GameIsBG1():
		global SubSubOptionsWindow
		if SubSubOptionsWindow:
			SubSubOptionsWindow.Unload()
		SubSubOptionsWindow = None
	else:
		global GameOptionsWindow
		if GameOptionsWindow:
			GameOptionsWindow.Unload()
			GameOptionsWindow = None
		OpenAudioOptionsWindow ()

	return

def OpenCharacterSoundsWindow ():
	"""Open character sounds window"""

	global HelpTextArea2 # same as HelpTextArea if not BG1
	Window = None

	if GUICommon.GameIsBG1():
		global SubSubOptionsWindow
		Window = SubSubOptionsWindow
	else:
		global GameOptionsWindow
		Window = GameOptionsWindow

	if Window:
		Window.Unload ()

	Window = GemRB.LoadWindow (12)
	HelpTextArea2 = GUIOPTControls.OptHelpText ('CharacterSounds', Window, 16, 18041)

	GUIOPTControls.OptDone (CloseCharacterSoundsWindow, Window, 24)
	GUIOPTControls.OptCancel (CloseCharacterSoundsWindow, Window, 25)

	GUIOPTControls.OptCheckboxNoCallback (18015, HelpTextArea2, Window, 5, 20, 'Subtitles')
	GUIOPTControls.OptCheckboxNoCallback (18013, HelpTextArea2, Window, 6, 18, 'Attack Sound')
	GUIOPTControls.OptCheckboxNoCallback (18014, HelpTextArea2, Window, 7, 19, 'Footsteps')
	GUIOPTControls.OptRadio (DisplayHelpCommandSounds, Window, 8, 21, 'Command Sounds Frequency', 2)
	GUIOPTControls.OptRadio (DisplayHelpCommandSounds, Window, 9, 21, 'Command Sounds Frequency', 1)
	GUIOPTControls.OptRadio (DisplayHelpCommandSounds, Window, 10, 21, 'Command Sounds Frequency', 0)
	GUIOPTControls.OptRadio (DisplayHelpSelectionSounds, Window, 58, 57, 'Selection Sounds Frequency', 2)
	GUIOPTControls.OptRadio (DisplayHelpSelectionSounds, Window, 59, 57, 'Selection Sounds Frequency', 1)
	GUIOPTControls.OptRadio (DisplayHelpSelectionSounds, Window, 60, 57, 'Selection Sounds Frequency', 0)

	Window.ShowModal (MODAL_SHADOW_GRAY)

	if GUICommon.GameIsBG1():
		SubSubOptionsWindow = Window
	else:
		GameOptionsWindow = Window

def DisplayHelpCommandSounds ():
	# same as HelpTextArea if not BG1
	HelpTextArea2.SetText (18016)

def DisplayHelpSelectionSounds ():
	# same as HelpTextArea if not BG1
	HelpTextArea2.SetText (11352)

###################################################

def CloseGameplayOptionsWindow ():
	if GUICommon.GameIsBG1():
		global SubOptionsWindow

		if SubOptionsWindow:
			SubOptionsWindow.Unload()
		SubOptionsWindow = None
	else:
		# no idea why this is here twice
		OpenOptionsWindow ()
		OpenOptionsWindow ()

def OpenGameplayOptionsWindow ():
	"""Open gameplay options window"""
	global HelpTextArea
	Window = None
	if GUICommon.GameIsBG1():
		global SubOptionsWindow
		Window = SubOptionsWindow
	else:
		global GameOptionsWindow
		Window = GameOptionsWindow

	if GameOptionsWindow:
		GameOptionsWindow.Unload ()
		GameOptionsWindow = None

	#gameplayoptions
	Window = GemRB.LoadWindow (8)

	HelpTextArea = GUIOPTControls.OptHelpText ('GameplayOptions', Window, 40, 18042)

	GUIOPTControls.OptDone (CloseGameplayOptionsWindow, Window, 7)
	GUIOPTControls.OptCancel (CloseGameplayOptionsWindow, Window, 20)

	GUIOPTControls.OptSlider (DisplayHelpTooltipDelay, Window, 1, 'Tooltips', TOOLTIP_DELAY_FACTOR)
	GUIOPTControls.OptSlider (DisplayHelpMouseScrollingSpeed, Window, 2, 'Mouse Scroll Speed', 5)
	GUIOPTControls.OptSliderNoCallback (18019, HelpTextArea, Window, 3, 'Keyboard Scroll Speed', 5)
	GUIOPTControls.OptSliderNoCallback (18020, HelpTextArea, Window, 12, 'Difficulty Level', 0)

	GUIOPTControls.OptCheckboxNoCallback (18021, HelpTextArea, Window, 14, 25, 'Always Dither')
	GUIOPTControls.OptCheckboxNoCallback (18023, HelpTextArea, Window, 19, 27, 'Gore')
	GUIOPTControls.OptCheckboxNoCallback (11797, HelpTextArea, Window, 42, 44, 'Infravision')
	GUIOPTControls.OptCheckboxNoCallback (20619, HelpTextArea, Window, 47, 46, 'Weather')
	if GUICommon.GameIsBG2():
		GUIOPTControls.OptCheckboxNoCallback (2242, HelpTextArea, Window, 50, 48, 'Heal Party on Rest')
	elif GUICommon.GameIsIWD2():
		GUIOPTControls.OptCheckboxNoCallback (15136, HelpTextArea, Window, 50, 49, 'Maximum HP')

	GUIOPTControls.OptButton (OpenFeedbackOptionsWindow, Window, 5, 17163)
	GUIOPTControls.OptButton (OpenAutopauseOptionsWindow, Window, 6, 17166)
	if GUICommon.GameIsBG2():
		GUIOPTControls.OptButton (OpenHotkeyOptionsWindow, Window, 51, 816)

	if GUICommon.GameIsBG1():
		SubOptionsWindow = Window
	else:
		GameOptionsWindow = Window

	Window.ShowModal (MODAL_SHADOW_GRAY)
	return

def DisplayHelpTooltipDelay ():
	HelpTextArea.SetText (18017)
	GemRB.SetTooltipDelay (GemRB.GetVar ("Tooltips") )

def DisplayHelpMouseScrollingSpeed ():
	HelpTextArea.SetText (18018)
	GemRB.SetMouseScrollSpeed (GemRB.GetVar ("Mouse Scroll Speed") )

###################################################

def CloseFeedbackOptionsWindow ():
	if GUICommon.GameIsBG1():
		global SubSubOptionsWindow

		if SubSubOptionsWindow:
			SubSubOptionsWindow.Unload ()
		SubSubOptionsWindow = None
	else:
		global GameOptionsWindow

		if GameOptionsWindow:
			GameOptionsWindow.Unload ()
			GameOptionsWindow = None
		OpenGameplayOptionsWindow ()
	return

def OpenFeedbackOptionsWindow ():
	"""Open feedback options window"""

	global HelpTextArea2
	Window = None
	if GUICommon.GameIsBG1():
		global SubSubOptionsWindow
		Window = SubSubOptionsWindow
	else:
		global GameOptionsWindow
		Window = GameOptionsWindow

	if Window:
		Window.Unload ()

	Window = GemRB.LoadWindow (9)
	# same as HelpTextArea if not BG1
	HelpTextArea2 = GUIOPTControls.OptHelpText ('FeedbackOptions', Window, 28, 18043)

	GemRB.SetVar ("Circle Feedback", GemRB.GetVar ("GUI Feedback Level") - 1)

	GUIOPTControls.OptDone (CloseFeedbackOptionsWindow, Window, 26)
	GUIOPTControls.OptCancel (CloseFeedbackOptionsWindow, Window, 27)

	GUIOPTControls.OptSlider (DisplayHelpMarkerFeedback, Window, 8, 'Circle Feedback', 1)
	GUIOPTControls.OptSliderNoCallback (18025, HelpTextArea2, Window, 9, 'Locator Feedback Level', 1)

	GUIOPTControls.OptCheckboxNoCallback (18026, HelpTextArea2, Window, 10, 32, 'Rolls')
	GUIOPTControls.OptCheckboxNoCallback (18027, HelpTextArea2, Window, 11, 33, 'Combat Info')
	GUIOPTControls.OptCheckboxNoCallback (18028, HelpTextArea2, Window, 12, 34, 'Actions')
	GUIOPTControls.OptCheckboxNoCallback (18029, HelpTextArea2, Window, 13, 35, 'State Changes')
	GUIOPTControls.OptCheckboxNoCallback (18030, HelpTextArea2, Window, 14, 36, 'Selection Text')
	GUIOPTControls.OptCheckboxNoCallback (18031, HelpTextArea2, Window, 15, 37, 'Miscellaneous Text')

	Window.ShowModal (MODAL_SHADOW_GRAY)

	if GUICommon.GameIsBG1():
		SubSubOptionsWindow = Window
	else:
		GameOptionsWindow = Window

	return

def DisplayHelpMarkerFeedback ():
	# same as HelpTextArea if not BG1
	HelpTextArea2.SetText (18024)
	GemRB.SetVar ("GUI Feedback Level", GemRB.GetVar ("Circle Feedback") + 1)

###################################################

def CloseAutopauseOptionsWindow ():
	if GUICommon.GameIsBG1():
		global SubSubOptionsWindow

		if SubSubOptionsWindow:
			SubSubOptionsWindow.Unload ()
		SubSubOptionsWindow = None
	else:
		global GameOptionsWindow

		if GameOptionsWindow:
			GameOptionsWindow.Unload ()
			GameOptionsWindow = None
		OpenGameplayOptionsWindow ()

	return

def OpenAutopauseOptionsWindow ():
	"""Open autopause options window"""

	global HelpTextArea2
	Window = None
	if GUICommon.GameIsBG1():
		global SubSubOptionsWindow
		Window = SubSubOptionsWindow
	else:
		global GameOptionsWindow
		Window = GameOptionsWindow

	if Window:
		Window.Unload ()

	Window = GemRB.LoadWindow (10)
	HelpTextArea2 = GUIOPTControls.OptHelpText ('AutopauseOptions', Window, 15, 18044)

	GUIOPTControls.OptDone (CloseAutopauseOptionsWindow, Window, 11)
	GUIOPTControls.OptCancel (CloseAutopauseOptionsWindow, Window, 14)

	GUIOPTControls.OptCheckboxNoCallback (18032, HelpTextArea2, Window, 1, 17, 'Auto Pause State', 1)
	GUIOPTControls.OptCheckboxNoCallback (18033, HelpTextArea2, Window, 2, 18, 'Auto Pause State', 2)
	GUIOPTControls.OptCheckboxNoCallback (18034, HelpTextArea2, Window, 3, 19, 'Auto Pause State', 4)
	GUIOPTControls.OptCheckboxNoCallback (18035, HelpTextArea2, Window, 4, 20, 'Auto Pause State', 8)
	GUIOPTControls.OptCheckboxNoCallback (18036, HelpTextArea2, Window, 5, 21, 'Auto Pause State', 16)
	GUIOPTControls.OptCheckboxNoCallback (18037, HelpTextArea2, Window, 13, 22, 'Auto Pause State', 32)
	GUIOPTControls.OptCheckboxNoCallback (10640, HelpTextArea2, Window, 25, 24, 'Auto Pause State', 64)
	if GUICommon.GameIsIWD2():
		GUIOPTControls.OptCheckboxNoCallback (23514, HelpTextArea2, Window, 30, 31, 'Auto Pause State', 128)
		GUIOPTControls.OptCheckboxNoCallback (58171, HelpTextArea2, Window, 34, 30, 'Auto Pause State', 256)
		GUIOPTControls.OptCheckboxNoCallback (10571, HelpTextArea2, Window, 33, 34, 'Auto Pause Center', 1)
		GUIOPTControls.OptCheckboxNoCallback (31872, HelpTextArea2, Window, 26, 28, 'Auto Pause State', 512)
	elif not GUICommon.GameIsIWD1():
		GUIOPTControls.OptCheckboxNoCallback (23514, HelpTextArea2, Window, 26, 27, 'Auto Pause State', 128)
	if GUICommon.GameIsBG2():
		# TODO: recheck if the first and third are correctly mapped
		GUIOPTControls.OptCheckboxNoCallback (58171, HelpTextArea2, Window, 31, 33, 'Auto Pause State', 256)
		GUIOPTControls.OptCheckboxNoCallback (10571, HelpTextArea2, Window, 37, 36, 'Auto Pause Center', 1)
		GUIOPTControls.OptCheckboxNoCallback (31872, HelpTextArea2, Window, 34, 30, 'Auto Pause State', 512)

	if GUICommon.GameIsBG1():
		SubSubOptionsWindow = Window
	else:
		GameOptionsWindow = Window

	Window.ShowModal (MODAL_SHADOW_GRAY)
	return

###################################################

def CloseMovieWindow ():
	if MovieWindow:
		MovieWindow.Unload ()
	return

def MoviePlayPress():
	s = GemRB.GetVar("MovieIndex")
	for i in range(0, MoviesTable.GetRowCount() ):
		t = MoviesTable.GetRowName(i)
		if GemRB.GetVar(t)==1:
			if s==0:
				s = MoviesTable.GetRowName(i)
				GemRB.PlayMovie(s, 1)
				MovieWindow.Invalidate()
				return
			s = s - 1
	return

def MovieCreditsPress():
	GemRB.PlayMovie("CREDITS")
	MovieWindow.Invalidate()
	return

def OpenMovieWindow ():
	global MovieWindow, TextAreaControl, MoviesTable

	GemRB.LoadWindowPack("GUIMOVIE", 800, 600)
	MovieWindow = Window = GemRB.LoadWindow(2)
	Window.SetFrame ()
	#reloading the guiopt windowpack
	GemRB.LoadWindowPack ("GUIOPT", 800, 600)
	TextAreaControl = Window.GetControl(0)
	TextAreaControl.SetFlags(IE_GUI_TEXTAREA_SELECTABLE)
	PlayButton = Window.GetControl(2)
	CreditsButton = Window.GetControl(3)
	DoneButton = Window.GetControl(4)
	MoviesTable = GemRB.LoadTable("MOVIDESC")
	for i in range(0, MoviesTable.GetRowCount() ):
		t = MoviesTable.GetRowName(i)
		if GemRB.GetVar(t)==1:
			s = MoviesTable.GetValue(i, 0)
			TextAreaControl.Append(s,-1)
	TextAreaControl.SetVarAssoc("MovieIndex",0)
	PlayButton.SetText(17318)
	CreditsButton.SetText(15591)
	DoneButton.SetText(11973)
	PlayButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, MoviePlayPress)
	CreditsButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, MovieCreditsPress)
	DoneButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, CloseMovieWindow)
	Window.ShowModal (MODAL_SHADOW_GRAY)
	return

###################################################

def OpenSaveMsgWindow ():
	GemRB.SetVar("QuitAfterSave",0)
	GUISAVE.OpenSaveWindow ()
	#save the game without quitting
	return

###################################################

def OpenLoadMsgWindow ():
	global LoadMsgWindow

	if LoadMsgWindow:
		return

	LoadMsgWindow = Window = GemRB.LoadWindow (4)

	# Load
	Button = Window.GetControl (0)
	Button.SetText (15590)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, LoadGamePress)
	Button.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)

	# Cancel
	Button = Window.GetControl (1)
	Button.SetText (GUIOPTControls.STR_OPT_CANCEL)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, CloseLoadMsgWindow)
	Button.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)

	# Loading a game will destroy ...
	Text = Window.GetControl (3)
	Text.SetText (19531)

	Window.ShowModal (MODAL_SHADOW_GRAY)
	return

def CloseLoadMsgWindow ():
	global LoadMsgWindow

	if LoadMsgWindow:
		LoadMsgWindow.Unload ()
	LoadMsgWindow = None

	if not GUICommon.GameIsBG1():
		OptionsWindow.SetVisible (WINDOW_VISIBLE)
		GameOptionsWindow.SetVisible (WINDOW_VISIBLE)
		PortraitWindow.SetVisible (WINDOW_VISIBLE)
	return

def LoadGamePress ():
	global LoadMsgWindow

	if LoadMsgWindow:
		LoadMsgWindow.Unload ()
	LoadMsgWindow = None
	GemRB.QuitGame ()
	OpenOptionsWindow()
	GemRB.SetNextScript ("GUILOAD")
	return

#save game AND quit
def SaveGamePress ():
	CloseQuitMsgWindow()
	#we need to set a state: quit after save
	GemRB.SetVar("QuitAfterSave",1)
	OpenOptionsWindow()
	GUISAVE.OpenSaveWindow ()
	return

def QuitGamePress ():
	if GemRB.GetVar("AskAndExit") == 1:
		GemRB.Quit()
		return

	CloseQuitMsgWindow()

	GemRB.QuitGame ()
	OpenOptionsWindow()
	GemRB.SetNextScript ("Start")
	return

###################################################

def OpenQuitMsgWindow ():
	global QuitMsgWindow

	if QuitMsgWindow:
		return

	QuitMsgWindow = Window = GemRB.LoadWindow (5)

	# Save
	Button = Window.GetControl (0)
	Button.SetText (15589)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, SaveGamePress)

	# Quit Game
	Button = Window.GetControl (1)
	Button.SetText (15417)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, QuitGamePress)
	Button.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)

	# Cancel
	Button = Window.GetControl (2)
	Button.SetText (GUIOPTControls.STR_OPT_CANCEL)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, CancelQuitMsgWindow)
	Button.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)

	# Do you wish to save the game ....
	Text = Window.GetControl (3)
	Text.SetText (16456)

	Window.ShowModal (MODAL_SHADOW_GRAY)
	return

def CancelQuitMsgWindow ():
	CloseQuitMsgWindow()

	OptionsWindow.SetVisible (WINDOW_VISIBLE)
	if not GUICommon.GameIsBG1():
		GameOptionsWindow.SetVisible (WINDOW_VISIBLE)
		PortraitWindow.SetVisible (WINDOW_VISIBLE)
	return

def CloseQuitMsgWindow ():
	global QuitMsgWindow

	if QuitMsgWindow:
		QuitMsgWindow.Unload ()
	QuitMsgWindow = None

	GemRB.SetVar("AskAndExit", 0)
	return

###################################################
#TODO
def OpenHotkeyOptionsWindow ():
	return

def CloseHotkeyOptionsWindow ():
	return

###################################################
# End of file GUIOPT.py
