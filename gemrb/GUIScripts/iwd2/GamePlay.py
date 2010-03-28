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
#GamePlay Options Menu
import GemRB

GamePlayWindow = 0
TextAreaControl = 0

def OnLoad():
	global GamePlayWindow, TextAreaControl
	GemRB.LoadWindowPack("GUIOPT", 800, 600)
	GamePlayWindow = GemRB.LoadWindowObject(8)
	GamePlayWindow.SetFrame( )
	TextAreaControl = GamePlayWindow.GetControl(40)
	DelayButton = GamePlayWindow.GetControl(21)
	DelaySlider = GamePlayWindow.GetControl(1)
	MouseSpdButton = GamePlayWindow.GetControl(22)
	MouseSpdSlider = GamePlayWindow.GetControl(2)
	KeySpdButton = GamePlayWindow.GetControl(23)
	KeySpdSlider = GamePlayWindow.GetControl(3)
	DifficultyButton = GamePlayWindow.GetControl(24)
	DifficultySlider = GamePlayWindow.GetControl(12)
	DitherButton = GamePlayWindow.GetControl(25)
	DitherButtonB = GamePlayWindow.GetControl(14)
	InfravisionButton = GamePlayWindow.GetControl(44)
	InfravisionButtonB = GamePlayWindow.GetControl(42)
	WeatherButton = GamePlayWindow.GetControl(46)
	WeatherButtonB = GamePlayWindow.GetControl(47)
	MaxHPButton = GamePlayWindow.GetControl(49)
	MaxHPButtonB = GamePlayWindow.GetControl(50)
	BloodButton = GamePlayWindow.GetControl(27)
	BloodButtonB = GamePlayWindow.GetControl(19)
	FeedbackButton = GamePlayWindow.GetControl(5)
	AutoPauseButton = GamePlayWindow.GetControl(6)
	OkButton = GamePlayWindow.GetControl(7)
	CancelButton = GamePlayWindow.GetControl(20)
	TextAreaControl.SetText(18042)

	OkButton.SetText(11973)
	OkButton.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)

	CancelButton.SetText(13727)
	CancelButton.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)

	FeedbackButton.SetText(17163)
	AutoPauseButton.SetText(17166)
	DelayButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, "DelayPress")
	DelaySlider.SetEvent(IE_GUI_SLIDER_ON_CHANGE, "DelayPress")
	DelaySlider.SetVarAssoc("Tooltips",TOOLTIP_DELAY_FACTOR)

	KeySpdButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, "KeySpdPress")
	KeySpdSlider.SetEvent(IE_GUI_SLIDER_ON_CHANGE, "KeySpdPress")
	KeySpdSlider.SetVarAssoc("Keyboard Scroll Speed",36)

	MouseSpdButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, "MouseSpdPress")
	MouseSpdSlider.SetEvent(IE_GUI_SLIDER_ON_CHANGE, "MouseSpdPress")
	MouseSpdSlider.SetVarAssoc("Mouse Scroll Speed",36)

	DifficultyButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, "DifficultyPress")
	DifficultySlider.SetEvent(IE_GUI_SLIDER_ON_CHANGE, "DifficultyPress")
	DifficultySlider.SetVarAssoc("Difficulty Level",2)

	BloodButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, "BloodPress")
	BloodButtonB.SetEvent(IE_GUI_BUTTON_ON_PRESS, "BloodPress")
	BloodButtonB.SetFlags(IE_GUI_BUTTON_CHECKBOX,OP_OR)
	BloodButtonB.SetVarAssoc("Gore",1)
	BloodButtonB.SetSprites("GBTNOPT4", 0, 0, 1, 2, 3)

	DitherButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, "DitherPress")
	DitherButtonB.SetEvent(IE_GUI_BUTTON_ON_PRESS, "DitherPress")
	DitherButtonB.SetFlags(IE_GUI_BUTTON_CHECKBOX,OP_OR)
	DitherButtonB.SetVarAssoc("Always Dither",1)
	DitherButtonB.SetSprites("GBTNOPT4", 0, 0, 1, 2, 3)

	InfravisionButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, "InfravisionPress")
	InfravisionButtonB.SetEvent(IE_GUI_BUTTON_ON_PRESS, "InfravisionPress")
	InfravisionButtonB.SetFlags(IE_GUI_BUTTON_CHECKBOX,OP_OR)
	InfravisionButtonB.SetVarAssoc("Darkvision",1)
	InfravisionButtonB.SetSprites("GBTNOPT4", 0, 0, 1, 2, 3)

	WeatherButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, "WeatherPress")
	WeatherButtonB.SetEvent(IE_GUI_BUTTON_ON_PRESS, "WeatherPress")
	WeatherButtonB.SetFlags(IE_GUI_BUTTON_CHECKBOX,OP_OR)
	WeatherButtonB.SetVarAssoc("Weather",1)
	WeatherButtonB.SetSprites("GBTNOPT4", 0, 0, 1, 2, 3)

	MaxHPButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, "MaxHPPress")
	MaxHPButtonB.SetEvent(IE_GUI_BUTTON_ON_PRESS, "MaxHPPressB")
	MaxHPButtonB.SetFlags(IE_GUI_BUTTON_CHECKBOX,OP_OR)
	MaxHPButtonB.SetVarAssoc("Maximum HP",1)
	MaxHPButtonB.SetSprites("GBTNOPT4", 0, 0, 1, 2, 3)

	FeedbackButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, "FeedbackPress")
	AutoPauseButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, "AutoPausePress")
	OkButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, "OkPress")
	CancelButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, "CancelPress")
	GamePlayWindow.SetVisible(WINDOW_VISIBLE)
	return

def DelayPress():
	TextAreaControl.SetText(18017)
	GemRB.SetTooltipDelay (GemRB.GetVar ("Tooltips") )
	return

def KeySpdPress():
	TextAreaControl.SetText(18019)
	return

def MouseSpdPress():
	TextAreaControl.SetText(18018)
	GemRB.SetMouseScrollSpeed (GemRB.GetVar ("Mouse Scroll Speed") )
	return

def DifficultyPress():
	TextAreaControl.SetText(18020)
	return

def BloodPress():
	TextAreaControl.SetText(18023)
	return

def DitherPress():
	TextAreaControl.SetText(18021)
	return

def InfravisionPress():
	TextAreaControl.SetText(11797)
	return

def WeatherPress():
	TextAreaControl.SetText(20619)
	return

def RestUntilHealPress():
	TextAreaControl.SetText(2242)
	return

def MaxHPPress():
	TextAreaControl.SetText(15136)
	return

def FeedbackPress():
	GamePlayWindow.SetVisible(WINDOW_INVISIBLE)
	if GamePlayWindow:
		GamePlayWindow.Unload()
	GemRB.SetNextScript("Feedback")
	return

def AutoPausePress():
	GamePlayWindow.SetVisible(WINDOW_INVISIBLE)
	if GamePlayWindow:
		GamePlayWindow.Unload()
	GemRB.SetNextScript("AutoPause")
	return

def OkPress():
	GamePlayWindow.SetVisible(WINDOW_INVISIBLE)
	if GamePlayWindow:
		GamePlayWindow.Unload()
	GemRB.SetNextScript("Options")
	return

def CancelPress():
	GamePlayWindow.SetVisible(WINDOW_INVISIBLE)
	if GamePlayWindow:
		GamePlayWindow.Unload()
	GemRB.SetNextScript("Options")
	return
