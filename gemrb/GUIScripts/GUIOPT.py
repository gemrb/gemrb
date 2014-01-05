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
import CommonWindow
import GameCheck
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
SubOptionsWindow = None
SubSubOptionsWindow = None

if GameCheck.IsBG1():
	HelpTextArea2 = None
else:
	# just an alias to keep our logic from being plagued by too many GameCheck.IsBG1() checks
	HelpTextArea2 = HelpTextArea

if GameCheck.IsIWD2():
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
	if not GameCheck.IsBG1():
		GUICommonWindows.PortraitWindow = OldPortraitWindow
		OldPortraitWindow = None

	GemRB.GamePause (0, 3)
	return

###################################################
def OpenOptionsWindow ():
	"""Open main options window"""

	global GameOptionsWindow, OptionsWindow, PortraitWindow
	global OldPortraitWindow, OldOptionsWindow

	if GUICommon.CloseOtherWindow (OpenOptionsWindow):
		CloseOptionsWindow()
		return

	GemRB.GamePause (1, 3)

	CommonWindow.CloseContainerWindow ()
	GemRB.HideGUI ()
	GUICommon.GameWindow.SetVisible(WINDOW_INVISIBLE)
	if GameCheck.IsBG1():
		GUICommonWindows.SetSelectionChangeHandler (None)

	GemRB.LoadWindowPack ("GUIOPT", WIDTH, HEIGHT)
	GameOptionsWindow = Window = GemRB.LoadWindow (2)
	GemRB.SetVar ("OtherWindow", GameOptionsWindow.ID)

	if OldOptionsWindow == None:
		# OptionsWindow is the leftmost menu bar window present in most of the games
		OldOptionsWindow = GUICommonWindows.OptionsWindow
		OptionsWindow = GemRB.LoadWindow (0)
		GUICommonWindows.MarkMenuButton (OptionsWindow)
		GUICommonWindows.SetupMenuWindowControls (OptionsWindow, 0, OpenOptionsWindow)
		OptionsWindow.SetFrame ()
		if not GameCheck.IsBG1(): #not in PST either, but it has its own OpenOptionsWindow()
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

	if GameCheck.IsIWD2():
		# Keyboard shortcuts
		KeyboardButton = Window.GetControl (13)
		KeyboardButton.SetText (33468)
		KeyboardButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, OpenHotkeyOptionsWindow)

		# Movies
		MoviesButton = Window.GetControl (14)
		MoviesButton.SetText (15415)
		MoviesButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, OpenMovieWindow)

	RestoreWinVisibility ()

	return

def TrySavingConfiguration():
	if not GemRB.SaveConfig():
		print "ARGH, could not write config to disk!!"

###################################################

def CloseVideoOptionsWindow ():
	CloseSubOptionsWindow ()
	TrySavingConfiguration()

def OpenVideoOptionsWindow ():
	"""Open video options window"""
	global SubOptionsWindow, HelpTextArea

	Window = SubOptionsWindow
	CloseSubOptionsWindow ()

	Window = GemRB.LoadWindow (6)

	HelpTextArea = GUIOPTControls.OptHelpText ('VideoOptions', Window, 33, 18038)

	GUIOPTControls.OptDone (CloseVideoOptionsWindow, Window, 21)
	GUIOPTControls.OptCancel (CloseVideoOptionsWindow, Window, 32)

	GUIOPTControls.OptSlider (18038, 17203, HelpTextArea, Window, 3, 35, 17129, 'Brightness Correction', DisplayHelpBrightness, 4)
	GUIOPTControls.OptSlider (18038, 17204, HelpTextArea, Window, 22, 36, 17128, 'Gamma Correction', DisplayHelpContrast)

	GUIOPTControls.OptRadio (DisplayHelpBPP, Window, 5, 37, 'BitsPerPixel', 16)
	GUIOPTControls.OptRadio (DisplayHelpBPP, Window, 6, 37, 'BitsPerPixel', 24)
	GUIOPTControls.OptRadio (DisplayHelpBPP, Window, 7, 37, 'BitsPerPixel', 32)

	GUIOPTControls.OptCheckbox (18038, 18000, HelpTextArea, Window, 9, 38, 17131, 'Full Screen', DisplayHelpFullScreen)

	GUIOPTControls.OptCheckbox (18038, 20620, HelpTextArea, Window, 51, 50, 20617, 'Translucent Shadows')
	GUIOPTControls.OptCheckbox (18038, 18004, HelpTextArea, Window, 40, 44, 17134, 'SoftMirrorBlt')
	GUIOPTControls.OptCheckbox (18038, 18006, HelpTextArea, Window, 41, 46, 17136, 'SoftSrcKeyBlt') # software standard blit
	GUIOPTControls.OptCheckbox (18038, 18007, HelpTextArea, Window, 42, 48, 17135, 'SoftBltFast') # software transparent blit

	SubOptionsWindow = Window

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
	if GameCheck.IsIWD2(): # 10/11 ticks
		Gamma /= 2

	GemRB.SetGamma (Brightness, Gamma)

###################################################

def CloseAudioOptionsWindow ():
	CloseSubOptionsWindow ()
	TrySavingConfiguration()

def OpenAudioOptionsWindow ():
	"""Open audio options window"""
	global SubOptionsWindow, HelpTextArea

	Window = SubOptionsWindow
	CloseSubOptionsWindow ()

	Window = GemRB.LoadWindow (7)
	HelpTextArea = GUIOPTControls.OptHelpText ('AudioOptions', Window, 14, 18040)

	GUIOPTControls.OptDone (CloseAudioOptionsWindow, Window, 24)
	GUIOPTControls.OptCancel (CloseAudioOptionsWindow, Window, 25)
	GUIOPTControls.OptButton (OpenCharacterSoundsWindow, Window, 13, 17778)

	GUIOPTControls.OptSlider (18040, 18008, HelpTextArea, Window, 1, 16, 16514, 'Volume Ambients', DisplayHelpAmbientVolume, 10)
	GUIOPTControls.OptSlider (18040, 18009, HelpTextArea, Window, 2, 17, 16515, 'Volume SFX', None, 10)
	GUIOPTControls.OptSlider (18040, 18010, HelpTextArea, Window, 3, 18, 16512, 'Volume Voices', None, 10)
	GUIOPTControls.OptSlider (18040, 18011, HelpTextArea, Window, 4, 19, 16511, 'Volume Music', DisplayHelpMusicVolume, 10)
	GUIOPTControls.OptSlider (18040, 18012, HelpTextArea, Window, 22, 20, 16546, 'Volume Movie', None, 10)

	GUIOPTControls.OptCheckbox (18040, 18022, HelpTextArea, Window, 26, 28, 20689, 'Environmental Audio')

	SubOptionsWindow = Window

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
	CloseSubSubOptionsWindow ()

def OpenCharacterSoundsWindow ():
	"""Open character sounds window"""

	global SubSubOptionsWindow, HelpTextArea2

	Window = SubSubOptionsWindow
	CloseSubSubOptionsWindow ()

	Window = GemRB.LoadWindow (12)
	HelpTextArea2 = GUIOPTControls.OptHelpText ('CharacterSounds', Window, 16, 18041)

	GUIOPTControls.OptDone (CloseCharacterSoundsWindow, Window, 24)
	GUIOPTControls.OptCancel (CloseCharacterSoundsWindow, Window, 25)

	GUIOPTControls.OptCheckbox (18041, 18015, HelpTextArea2, Window, 5, 20, 17138, 'Subtitles')
	GUIOPTControls.OptCheckbox (18041, 18013, HelpTextArea2, Window, 6, 18, 17139, 'Attack Sound')
	GUIOPTControls.OptCheckbox (18041, 18014, HelpTextArea2, Window, 7, 19, 17140, 'Footsteps')
	GUIOPTControls.OptRadio (DisplayHelpCommandSounds, Window, 8, 21, 'Command Sounds Frequency', 2)
	GUIOPTControls.OptRadio (DisplayHelpCommandSounds, Window, 9, 21, 'Command Sounds Frequency', 1)
	GUIOPTControls.OptRadio (DisplayHelpCommandSounds, Window, 10, 21, 'Command Sounds Frequency', 0)
	GUIOPTControls.OptRadio (DisplayHelpSelectionSounds, Window, 58, 57, 'Selection Sounds Frequency', 2)
	GUIOPTControls.OptRadio (DisplayHelpSelectionSounds, Window, 59, 57, 'Selection Sounds Frequency', 1)
	GUIOPTControls.OptRadio (DisplayHelpSelectionSounds, Window, 60, 57, 'Selection Sounds Frequency', 0)

	Window.ShowModal (MODAL_SHADOW_GRAY)

	SubSubOptionsWindow = Window

def DisplayHelpCommandSounds ():
	# same as HelpTextArea if not BG1
	HelpTextArea2.SetText (18016)

def DisplayHelpSelectionSounds ():
	# same as HelpTextArea if not BG1
	HelpTextArea2.SetText (11352)

###################################################

def CloseGameplayOptionsWindow ():
	# FIXME: don't need to do anything, since we save stuff immediately
	# ergo canceling does not work
	CloseSubOptionsWindow ()
	TrySavingConfiguration()

def OpenGameplayOptionsWindow ():
	"""Open gameplay options window"""
	global SubOptionsWindow, HelpTextArea

	Window = SubOptionsWindow
	CloseSubOptionsWindow ()

	#gameplayoptions
	Window = GemRB.LoadWindow (8)

	HelpTextArea = GUIOPTControls.OptHelpText ('GameplayOptions', Window, 40, 18042)

	GUIOPTControls.OptDone (CloseGameplayOptionsWindow, Window, 7)
	GUIOPTControls.OptCancel (CloseGameplayOptionsWindow, Window, 20)

	GUIOPTControls.OptSlider (18042, 18017, HelpTextArea, Window, 1, 21, 17143, 'Tooltips', DisplayHelpTooltipDelay, TOOLTIP_DELAY_FACTOR)
	GUIOPTControls.OptSlider (18042, 18018, HelpTextArea, Window, 2, 22, 17144, 'Mouse Scroll Speed', DisplayHelpMouseScrollingSpeed, 5)
	GUIOPTControls.OptSlider (18042, 18019, HelpTextArea, Window, 3, 23, 17145, 'Keyboard Scroll Speed', None, 5)
	GUIOPTControls.OptSlider (18042, 18020, HelpTextArea, Window, 12, 24, 13911, 'Difficulty Level', None, 0)

	GUIOPTControls.OptCheckbox (18042, 18021, HelpTextArea, Window, 14, 25, 13697, 'Always Dither')
	GUIOPTControls.OptCheckbox (18042, 18023, HelpTextArea, Window, 19, 27, 17182, 'Gore')
	GUIOPTControls.OptCheckbox (18042, 11797, HelpTextArea, Window, 42, 44, 11795, 'Infravision')
	GUIOPTControls.OptCheckbox (18042, 20619, HelpTextArea, Window, 47, 46, 20618, 'Weather')
	if GameCheck.IsBG2():
		GUIOPTControls.OptCheckbox (18042, 2242, HelpTextArea, Window, 50, 48, 2241, 'Heal Party on Rest')
	elif GameCheck.IsIWD2():
		GUIOPTControls.OptCheckbox (18042, 15136, HelpTextArea, Window, 50, 49, 17378, 'Maximum HP')

	GUIOPTControls.OptButton (OpenFeedbackOptionsWindow, Window, 5, 17163)
	GUIOPTControls.OptButton (OpenAutopauseOptionsWindow, Window, 6, 17166)
	if GameCheck.IsBG2():
		GUIOPTControls.OptButton (OpenHotkeyOptionsWindow, Window, 51, 816)

	SubOptionsWindow = Window
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
	# FIXME: don't need to do anything, since we save stuff immediately
	# ergo canceling does not work
	CloseSubSubOptionsWindow ()

def OpenFeedbackOptionsWindow ():
	"""Open feedback options window"""

	global SubSubOptionsWindow, HelpTextArea2

	Window = SubSubOptionsWindow
	CloseSubSubOptionsWindow ()

	Window = GemRB.LoadWindow (9)
	# same as HelpTextArea if not BG1
	HelpTextArea2 = GUIOPTControls.OptHelpText ('FeedbackOptions', Window, 28, 18043)

	GemRB.SetVar ("Circle Feedback", GemRB.GetVar ("GUI Feedback Level") - 1)

	GUIOPTControls.OptDone (CloseFeedbackOptionsWindow, Window, 26)
	GUIOPTControls.OptCancel (CloseFeedbackOptionsWindow, Window, 27)

	GUIOPTControls.OptSlider (18043, 18024, HelpTextArea2, Window, 8, 30, 13688, 'Circle Feedback', DisplayHelpMarkerFeedback)
	GUIOPTControls.OptSlider (18043, 18025, HelpTextArea2, Window, 9, 31, 17769, 'Locator Feedback Level')

	GUIOPTControls.OptCheckbox (18043, 18026, HelpTextArea2, Window, 10, 32, 17149, 'Rolls')
	GUIOPTControls.OptCheckbox (18043, 18027, HelpTextArea2, Window, 11, 33, 17150, 'Combat Info')
	GUIOPTControls.OptCheckbox (18043, 18028, HelpTextArea2, Window, 12, 34, 17151, 'Actions')
	GUIOPTControls.OptCheckbox (18043, 18029, HelpTextArea2, Window, 13, 35, 17152, 'State Changes')
	GUIOPTControls.OptCheckbox (18043, 18030, HelpTextArea2, Window, 14, 36, 17181, 'Selection Text')
	GUIOPTControls.OptCheckbox (18043, 18031, HelpTextArea2, Window, 15, 37, 17153, 'Miscellaneous Text')

	Window.ShowModal (MODAL_SHADOW_GRAY)

	SubSubOptionsWindow = Window

	return

def DisplayHelpMarkerFeedback ():
	# same as HelpTextArea if not BG1
	HelpTextArea2.SetText (18024)
	GemRB.SetVar ("GUI Feedback Level", GemRB.GetVar ("Circle Feedback") + 1)

###################################################

def CloseAutopauseOptionsWindow ():
	# FIXME: don't need to do anything, since we save stuff immediately
	# ergo canceling does not work
	CloseSubSubOptionsWindow ()

def OpenAutopauseOptionsWindow ():
	"""Open autopause options window"""

	global SubSubOptionsWindow, HelpTextArea2

	Window = SubSubOptionsWindow
	CloseSubSubOptionsWindow ()

	Window = GemRB.LoadWindow (10)
	HelpTextArea2 = GUIOPTControls.OptHelpText ('AutopauseOptions', Window, 15, 18044)

	GUIOPTControls.OptDone (CloseAutopauseOptionsWindow, Window, 11)
	GUIOPTControls.OptCancel (CloseAutopauseOptionsWindow, Window, 14)

	# checkboxes OR the values if they associate to the same variable
	GUIOPTControls.OptCheckbox (18044, 18032, HelpTextArea2, Window, 1, 17, 17155, 'Auto Pause State', None, 4) # hit
	GUIOPTControls.OptCheckbox (18044, 18033, HelpTextArea2, Window, 2, 18, 17156, 'Auto Pause State', None, 8) # wounded
	GUIOPTControls.OptCheckbox (18044, 18034, HelpTextArea2, Window, 3, 19, 17157, 'Auto Pause State', None, 16) # dead
	GUIOPTControls.OptCheckbox (18044, 18035, HelpTextArea2, Window, 4, 20, 17158, 'Auto Pause State', None, 2) # attacked
	GUIOPTControls.OptCheckbox (18044, 18036, HelpTextArea2, Window, 5, 21, 17159, 'Auto Pause State', None, 1) # weapon unusable
	GUIOPTControls.OptCheckbox (18044, 18037, HelpTextArea2, Window, 13, 22, 17160, 'Auto Pause State', None, 32) # target gone
	GUIOPTControls.OptCheckbox (18044, 10640, HelpTextArea2, Window, 25, 24, 10639, 'Auto Pause State', None, 64) # end of round
	if GameCheck.IsIWD2():
		GUIOPTControls.OptCheckbox (18044, 23514, HelpTextArea2, Window, 30, 31, 23516, 'Auto Pause State', None, 128) # enemy sighted
		GUIOPTControls.OptCheckbox (18044, 18560, HelpTextArea2, Window, 26, 28, 16519, 'Auto Pause State', None, 256) # trap found
		GUIOPTControls.OptCheckbox (18044, 26311, HelpTextArea2, Window, 36, 37, 26310, 'Auto Pause State', None, 512) # spell cast
		GUIOPTControls.OptCheckbox (18044, 24888, HelpTextArea2, Window, 33, 34, 10574, 'Auto Pause Center', None, 1)
	elif not GameCheck.IsIWD1():
		GUIOPTControls.OptCheckbox (18044, 23514, HelpTextArea2, Window, 26, 27, 23516, 'Auto Pause State', None, 128) # enemy sighted
	if GameCheck.IsBG2():
		GUIOPTControls.OptCheckbox (18044, 58171, HelpTextArea2, Window, 31, 30, 31875, 'Auto Pause State', None, 512) # spell cast
		GUIOPTControls.OptCheckbox (18044, 31872, HelpTextArea2, Window, 34, 33, 57354, 'Auto Pause State', None, 256) # trap found
		GUIOPTControls.OptCheckbox (18044, 10571, HelpTextArea2, Window, 37, 36, 10574, 'Auto Pause Center', None, 1)

	SubSubOptionsWindow = Window

	Window.ShowModal (MODAL_SHADOW_GRAY)
	return

###################################################

def MoviePlayPress():
	s = GemRB.GetVar("MovieIndex")
	for i in range(0, MoviesTable.GetRowCount() ):
		t = MoviesTable.GetRowName(i)
		#temporarily out (change simultaneously with OpenMovieWindow)
		#if GemRB.GetVar(t)==1:
		if 1==1:
			if s==0:
				s = MoviesTable.GetRowName(i)
				GemRB.PlayMovie(s, 1)
				SubOptionsWindow.Invalidate()
				return
			s = s - 1
	return

def MovieCreditsPress():
	GemRB.PlayMovie("CREDITS")
	SubOptionsWindow.Invalidate()
	return

def OpenMovieWindow ():
	global SubOptionsWindow, TextAreaControl, MoviesTable

	GemRB.LoadWindowPack("GUIMOVIE", 800, 600)
	SubOptionsWindow = Window = GemRB.LoadWindow(2)
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
		#temporarily out too (see above)
		#if GemRB.GetVar(t)==1:
		if 1==1:
			s = MoviesTable.GetValue(i, 0)
			TextAreaControl.Append(s,-1)
	TextAreaControl.SetVarAssoc("MovieIndex",0)
	PlayButton.SetText(17318)
	CreditsButton.SetText(15591)
	DoneButton.SetText(11973)
	PlayButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, MoviePlayPress)
	CreditsButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, MovieCreditsPress)
	DoneButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, CloseSubOptionsWindow)
	Window.ShowModal (MODAL_SHADOW_GRAY)
	return

###################################################

def CloseSubOptionsWindow ():
	global SubOptionsWindow, GameOptionsWindow

	if SubOptionsWindow:
		SubOptionsWindow.Unload ()
		SubOptionsWindow = None
	if GameOptionsWindow:
		GameOptionsWindow.ShowModal (MODAL_SHADOW_GRAY)
	return

def CloseSubSubOptionsWindow ():
	global SubSubOptionsWindow, SubOptionsWindow

	if SubSubOptionsWindow:
		SubSubOptionsWindow.Unload ()
		SubSubOptionsWindow = None
	if SubOptionsWindow:
		SubOptionsWindow.ShowModal (MODAL_SHADOW_GRAY)
	return

def RestoreWinVisibility ():
	if OptionsWindow:
		OptionsWindow.SetVisible (WINDOW_VISIBLE)
	if GameOptionsWindow:
		GameOptionsWindow.SetVisible (WINDOW_VISIBLE)
	if PortraitWindow:
		PortraitWindow.SetVisible (WINDOW_VISIBLE)

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

	RestoreWinVisibility ()

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

	if not GameOptionsWindow:
		GUICommon.GameWindow.SetVisible(WINDOW_VISIBLE)

	RestoreWinVisibility ()

	return

def CloseQuitMsgWindow ():
	global QuitMsgWindow

	if QuitMsgWindow:
		QuitMsgWindow.Unload ()
	QuitMsgWindow = None

	GemRB.SetVar("AskAndExit", 0)
	return

###################################################
def OpenHotkeyOptionsWindow ():
	print("TODO: implement OpenHotkeyOptionsWindow!")
	# check if pst's guiopt's OpenKeyboardMappingsWindow is reusable
	return

def CloseHotkeyOptionsWindow ():
	return

###################################################
# End of file GUIOPT.py
