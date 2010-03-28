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
#gameplay
import GemRB

GamePlayWindow = 0
TextAreaControl = 0

def OnLoad():
	global GamePlayWindow, TextAreaControl
	GemRB.LoadWindowPack("GUIOPT", 640, 480)
	GamePlayWindow = GemRB.LoadWindowObject(8)
	TextAreaControl = GamePlayWindow.GetControl(40)
	DelayButton = GamePlayWindow.GetControl(21)
	DelaySlider = GamePlayWindow.GetControl(1)
	MouseSpdButton = GamePlayWindow.GetControl(22)
	MouseSpdSlider = GamePlayWindow.GetControl(2)
	KeySpdButton = GamePlayWindow.GetControl(23)
	KeySpdSlider = GamePlayWindow.GetControl(3)
	DifficultyButton = GamePlayWindow.GetControl(24)
	DifficultySlider = GamePlayWindow.GetControl(12)
	BloodButton = GamePlayWindow.GetControl(27)
	BloodButtonB = GamePlayWindow.GetControl(19)
	DitherButton = GamePlayWindow.GetControl(25)
	DitherButtonB = GamePlayWindow.GetControl(14)
	InfravisionButton = GamePlayWindow.GetControl(44)
	InfravisionButtonB = GamePlayWindow.GetControl(42)
	WeatherButton = GamePlayWindow.GetControl(46)
	WeatherButtonB = GamePlayWindow.GetControl(47)
	HealButton = GamePlayWindow.GetControl(48)
	HealButtonB = GamePlayWindow.GetControl(50)
	HotKeyButton = GamePlayWindow.GetControl(51)
	FeedbackButton = GamePlayWindow.GetControl(5)
	AutoPauseButton = GamePlayWindow.GetControl(6)
	OkButton = GamePlayWindow.GetControl(7)
	CancelButton = GamePlayWindow.GetControl(20)
	TextAreaControl.SetText(18042)
	OkButton.SetText(11973)
	CancelButton.SetText(13727)
	HotKeyButton.SetText(816)
	FeedbackButton.SetText(17163)
	AutoPauseButton.SetText(17166)
	DelayButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, "DelayPress")
	DelaySlider.SetEvent(IE_GUI_SLIDER_ON_CHANGE, "DelayPress")
	DelaySlider.SetVarAssoc("Tooltips",0)

	KeySpdButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, "KeySpdPress")
	KeySpdSlider.SetEvent(IE_GUI_SLIDER_ON_CHANGE, "KeySpdPress")
	KeySpdSlider.SetVarAssoc("Keyboard Scroll Speed",0)

	MouseSpdButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, "MouseSpdPress")
	MouseSpdSlider.SetEvent(IE_GUI_SLIDER_ON_CHANGE, "MouseSpdPress")
	MouseSpdSlider.SetVarAssoc("Mouse Scroll Speed",0)

	DifficultyButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, "DifficultyPress")
	DifficultySlider.SetEvent(IE_GUI_SLIDER_ON_CHANGE, "DifficultyPress")
	DifficultySlider.SetVarAssoc("Difficulty Level",0)

	BloodButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, "BloodPress")
	BloodButtonB.SetEvent(IE_GUI_BUTTON_ON_PRESS, "BloodPress")
	BloodButtonB.SetFlags(IE_GUI_BUTTON_CHECKBOX,OP_OR)
	BloodButtonB.SetVarAssoc("Gore",1)

	DitherButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, "DitherPress")
	DitherButtonB.SetEvent(IE_GUI_BUTTON_ON_PRESS, "DitherPress")
	DitherButtonB.SetFlags(IE_GUI_BUTTON_CHECKBOX,OP_OR)
	DitherButtonB.SetVarAssoc("Always Dither",1)

	InfravisionButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, "InfravisionPress")
	InfravisionButtonB.SetEvent(IE_GUI_BUTTON_ON_PRESS, "InfravisionPress")
	InfravisionButtonB.SetFlags(IE_GUI_BUTTON_CHECKBOX,OP_OR)
	InfravisionButtonB.SetVarAssoc("Infravision",1)

	WeatherButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, "WeatherPress")
	WeatherButtonB.SetEvent(IE_GUI_BUTTON_ON_PRESS, "WeatherPress")
	WeatherButtonB.SetFlags(IE_GUI_BUTTON_CHECKBOX,OP_OR)
	WeatherButtonB.SetVarAssoc("Weather",1)

	HealButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, "HealPress")
	HealButtonB.SetEvent(IE_GUI_BUTTON_ON_PRESS, "HealPress")
	HealButtonB.SetFlags(IE_GUI_BUTTON_CHECKBOX,OP_OR)
	HealButtonB.SetVarAssoc("Heal Party on Rest",1)

	HotKeyButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, "HotKeyPress")
	FeedbackButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, "FeedbackPress")
	AutoPauseButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, "AutoPausePress")
	OkButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, "OkPress")
	OkButton.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)
	CancelButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, "CancelPress")
	CancelButton.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)
	GamePlayWindow.SetVisible(1)
	return
	
def DelayPress():
	TextAreaControl.SetText(18017)
	return
	
def KeySpdPress():
	TextAreaControl.SetText(18019)
	return
	
def MouseSpdPress():
	TextAreaControl.SetText(18018)
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
	
def HealPress():
	TextAreaControl.SetText(2242)
	return
	
def HotKeyPress():
	#TextAreaControl.SetText(18016)
	return
	
def FeedbackPress():
	GamePlayWindow.SetVisible(0)
	if GamePlayWindow:
		GamePlayWindow.Unload()
	GemRB.SetNextScript("GUIOPT9")
	return
	
def AutoPausePress():
	GamePlayWindow.SetVisible(0)
	if GamePlayWindow:
		GamePlayWindow.Unload()
	GemRB.SetNextScript("GUIOPT10")
	return
	
def OkPress():
	GamePlayWindow.SetVisible(0)
	if GamePlayWindow:
		GamePlayWindow.Unload()
	GemRB.SetNextScript("StartOpt")
	return
	
def CancelPress():
	GamePlayWindow.SetVisible(0)
	if GamePlayWindow:
		GamePlayWindow.Unload()
	GemRB.SetNextScript("StartOpt")
	return
