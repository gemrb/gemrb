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

	GUIOPTControls.OptCheckbox (DisplayHelpTransShadow, Window, 51, 50, 'Translucent Shadows', 1)
	GUIOPTControls.OptCheckbox (DisplayHelpSoftMirrBlt, Window, 40, 44, 'SoftMirrorBlt' ,1)
	GUIOPTControls.OptCheckbox (DisplayHelpSoftTransBlt, Window, 41, 46, 'SoftSrcKeyBlt' ,1)
	GUIOPTControls.OptCheckbox (DisplayHelpSoftStandBlt, Window, 42, 48, 'SoftBltFast' ,1)

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
	GemRB.SetGamma (GemRB.GetVar("Brightness Correction"),GemRB.GetVar("Gamma Correction"))

def DisplayHelpContrast ():
	HelpTextArea.SetText (17204)
	GemRB.SetGamma (GemRB.GetVar("Brightness Correction"),GemRB.GetVar("Gamma Correction"))

def DisplayHelpSoftMirrBlt ():
	HelpTextArea.SetText (18004)

def DisplayHelpSoftTransBlt ():
	HelpTextArea.SetText (18006)

def DisplayHelpSoftStandBlt ():
	HelpTextArea.SetText (18007)

def DisplayHelpTransShadow ():
	HelpTextArea.SetText (20620)

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
	GUIOPTControls.OptSlider (DisplayHelpSoundFXVolume, Window, 2, 'Volume SFX', 10)
	GUIOPTControls.OptSlider (DisplayHelpVoiceVolume, Window, 3, 'Volume Voices', 10)
	GUIOPTControls.OptSlider (DisplayHelpMusicVolume, Window, 4, 'Volume Music', 10)
	GUIOPTControls.OptSlider (DisplayHelpMovieVolume, Window, 22, 'Volume Movie', 10)

	GUIOPTControls.OptCheckbox (DisplayHelpCreativeEAX, Window, 26, 28, 'Environmental Audio', 1)

	if GUICommon.GameIsBG1():
		SubOptionsWindow = Window
	else:
		GameOptionsWindow = Window

	Window.ShowModal (MODAL_SHADOW_GRAY)
	return

def DisplayHelpAmbientVolume ():
	HelpTextArea.SetText (18008)
	GemRB.UpdateAmbientsVolume ()

def DisplayHelpSoundFXVolume ():
	HelpTextArea.SetText (18009)

def DisplayHelpVoiceVolume ():
	HelpTextArea.SetText (18010)

def DisplayHelpMusicVolume ():
	HelpTextArea.SetText (18011)
	GemRB.UpdateMusicVolume ()

def DisplayHelpMovieVolume ():
	HelpTextArea.SetText (18012)

def DisplayHelpCreativeEAX ():
	HelpTextArea.SetText (18022)

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

	GUIOPTControls.OptCheckbox (DisplayHelpSubtitles, Window, 5, 20, 'Subtitles', 1)
	GUIOPTControls.OptCheckbox (DisplayHelpAttackSounds, Window, 6, 18, 'Attack Sounds', 1)
	GUIOPTControls.OptCheckbox (DisplayHelpFootsteps, Window, 7, 19, 'Footsteps', 1)
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

def DisplayHelpSubtitles ():
	# same as HelpTextArea if not BG1
	HelpTextArea2.SetText (18015)

def DisplayHelpAttackSounds ():
	# same as HelpTextArea if not BG1
	HelpTextArea2.SetText (18013)

def DisplayHelpFootsteps ():
	# same as HelpTextArea if not BG1
	HelpTextArea2.SetText (18014)

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
	GUIOPTControls.OptSlider (DisplayHelpKeyboardScrollingSpeed, Window, 3, 'Keyboard Scroll Speed', 5)
	GUIOPTControls.OptSlider (DisplayHelpDifficulty, Window, 12, 'Difficulty Level', 0)

	GUIOPTControls.OptCheckbox (DisplayHelpDitherAlways, Window, 14, 25, 'Always Dither', 1)
	GUIOPTControls.OptCheckbox (DisplayHelpGore, Window, 19, 27, 'Gore', 1)
	GUIOPTControls.OptCheckbox (DisplayHelpInfravision, Window, 42, 44, 'Infravision', 1)
	GUIOPTControls.OptCheckbox (DisplayHelpWeather, Window, 47, 46, 'Weather', 1)
	if GUICommon.GameIsBG2():
		GUIOPTControls.OptCheckbox (DisplayHelpRestUntilHealed, Window, 50, 48, 'Heal Party on Rest', 1)
	elif GUICommon.GameIsIWD2():
		GUIOPTControls.OptCheckbox (DisplayHelpMaxHitpoints, Window, 50, 49, 'Maximum HP', 1)

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

def DisplayHelpKeyboardScrollingSpeed ():
	HelpTextArea.SetText (18019)

def DisplayHelpDifficulty ():
	HelpTextArea.SetText (18020)

def DisplayHelpDitherAlways ():
	HelpTextArea.SetText (18021)

def DisplayHelpGore ():
	HelpTextArea.SetText (18023)

def DisplayHelpInfravision ():
	HelpTextArea.SetText (11797)

def DisplayHelpWeather ():
	HelpTextArea.SetText (20619)

def DisplayHelpRestUntilHealed ():
	HelpTextArea.SetText (2242)

def DisplayHelpMaxHitpoints ():
	HelpTextArea.SetText (15136)

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
	GUIOPTControls.OptSlider (DisplayHelpLocatorFeedback, Window, 9, 'Locator Feedback Level', 1)

	GUIOPTControls.OptCheckbox (DisplayHelpToHitRolls, Window, 10, 32, 'Rolls', 1)
	GUIOPTControls.OptCheckbox (DisplayHelpCombatInfo, Window, 11, 33, 'Combat Info', 1)
	GUIOPTControls.OptCheckbox (DisplayHelpActions, Window, 12, 34, 'Actions', 1)
	GUIOPTControls.OptCheckbox (DisplayHelpStates, Window, 13, 35, 'State Changes', 1)
	GUIOPTControls.OptCheckbox (DisplayHelpSelection, Window, 14, 36, 'Selection Text', 1)
	GUIOPTControls.OptCheckbox (DisplayHelpMiscellaneous, Window, 15, 37, 'Miscellaneous Text', 1)

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

def DisplayHelpLocatorFeedback ():
	# same as HelpTextArea if not BG1
	HelpTextArea2.SetText (18025)

def DisplayHelpToHitRolls ():
	# same as HelpTextArea if not BG1
	HelpTextArea2.SetText (18026)

def DisplayHelpCombatInfo ():
	# same as HelpTextArea if not BG1
	HelpTextArea2.SetText (18027)

def DisplayHelpActions ():
	# same as HelpTextArea if not BG1
	HelpTextArea2.SetText (18028)

def DisplayHelpStates ():
	# same as HelpTextArea if not BG1
	HelpTextArea2.SetText (18029)

def DisplayHelpSelection ():
	# same as HelpTextArea if not BG1
	HelpTextArea2.SetText (18030)

def DisplayHelpMiscellaneous ():
	# same as HelpTextArea if not BG1
	HelpTextArea2.SetText (18031)

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

	GUIOPTControls.OptCheckbox (DisplayHelpCharacterHit, Window, 1, 17, 'Auto Pause State', 1)
	GUIOPTControls.OptCheckbox (DisplayHelpCharacterInjured, Window, 2, 18, 'Auto Pause State', 2)
	GUIOPTControls.OptCheckbox (DisplayHelpCharacterDead, Window, 3, 19, 'Auto Pause State', 4)
	GUIOPTControls.OptCheckbox (DisplayHelpCharacterAttacked, Window, 4, 20, 'Auto Pause State', 8)
	GUIOPTControls.OptCheckbox (DisplayHelpWeaponUnusable, Window, 5, 21, 'Auto Pause State', 16)
	GUIOPTControls.OptCheckbox (DisplayHelpTargetGone, Window, 13, 22, 'Auto Pause State', 32)
	GUIOPTControls.OptCheckbox (DisplayHelpEndOfRound, Window, 25, 24, 'Auto Pause State', 64)
	if GUICommon.GameIsIWD2():
		GUIOPTControls.OptCheckbox (DisplayHelpEnemySighted, Window, 30, 28, 'Auto Pause State', 128)
	elif not GUICommon.GameIsIWD1():
		GUIOPTControls.OptCheckbox (DisplayHelpEnemySighted, Window, 26, 27, 'Auto Pause State', 128)
	if GUICommon.GameIsBG2() or GUICommon.GameIsIWD2():
		GUIOPTControls.OptCheckbox (DisplayHelpSpellCast, Window, 34, 30, 'Auto Pause State', 256)
		GUIOPTControls.OptCheckbox (DisplayHelpTrapFound, Window, 31, 33, 'Auto Pause State', 512)
		GUIOPTControls.OptCheckbox (DisplayHelpCenterOnActor, Window, 31, 33, 'Auto Pause Center', 1)
	if GUICommon.GameIsBG1():
		SubSubOptionsWindow = Window
	else:
		GameOptionsWindow = Window

	Window.ShowModal (MODAL_SHADOW_GRAY)
	return

# same as HelpTextArea if not BG1 in all the following functions
def DisplayHelpCharacterHit ():
	HelpTextArea2.SetText (18032)

def DisplayHelpCharacterInjured ():
	HelpTextArea2.SetText (18033)

def DisplayHelpCharacterDead ():
	HelpTextArea2.SetText (18034)

def DisplayHelpCharacterAttacked ():
	HelpTextArea2.SetText (18035)

def DisplayHelpWeaponUnusable ():
	HelpTextArea2.SetText (18036)

def DisplayHelpTargetGone ():
	HelpTextArea2.SetText (18037)

def DisplayHelpEndOfRound ():
	HelpTextArea2.SetText (10640)

def DisplayHelpEnemySighted ():
	HelpTextArea2.SetText (23514)

def DisplayHelpSpellCast ():
	HelpTextArea2.SetText (58171)

def DisplayHelpTrapFound ():
	HelpTextArea2.SetText (31872)

def DisplayHelpCenterOnActor ():
	HelpTextArea2.SetText (10571)

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
	Button.SetText (13727)
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
	global QuitMsgWindow

	if QuitMsgWindow:
		QuitMsgWindow.Unload ()
	QuitMsgWindow = None
	#we need to set a state: quit after save
	GemRB.SetVar("QuitAfterSave",1)
	OpenOptionsWindow()
	GUISAVE.OpenSaveWindow ()
	return

def QuitGamePress ():
	global QuitMsgWindow

	if QuitMsgWindow:
		QuitMsgWindow.Unload ()
	QuitMsgWindow = None
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
	Button.SetText (13727)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, CloseQuitMsgWindow)
	Button.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)

	# Do you wish to save the game ....
	Text = Window.GetControl (3)
	Text.SetText (16456)

	Window.ShowModal (MODAL_SHADOW_GRAY)
	return

def CloseQuitMsgWindow ():
	global QuitMsgWindow

	if QuitMsgWindow:
		QuitMsgWindow.Unload ()
	QuitMsgWindow = None
	OptionsWindow.SetVisible (WINDOW_VISIBLE)
	if not GUICommon.GameIsBG1():
		GameOptionsWindow.SetVisible (WINDOW_VISIBLE)
		PortraitWindow.SetVisible (WINDOW_VISIBLE)
	return

###################################################
#TODO
def OpenHotkeyOptionsWindow ():
	return

def CloseHotkeyOptionsWindow ():
	return

###################################################
# End of file GUIOPT.py
