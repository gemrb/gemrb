#sound options
import GemRB

SoundWindow = 0
TextAreaControl = 0

def OnLoad():
	global SoundWindow, TextAreaControl
	GemRB.LoadWindowPack("GUIOPT")
	SoundWindow = GemRB.LoadWindow(7)
	TextAreaControl = GemRB.GetControl(SoundWindow, 14)
	AmbientButton = GemRB.GetControl(SoundWindow, 16)
	AmbientSlider = GemRB.GetControl(SoundWindow, 1)
	SoundEffectsButton = GemRB.GetControl(SoundWindow, 17)
	SoundEffectsSlider = GemRB.GetControl(SoundWindow, 2)
	DialogueButton = GemRB.GetControl(SoundWindow, 18)
	DialogueSlider = GemRB.GetControl(SoundWindow, 3)
	MusicButton = GemRB.GetControl(SoundWindow, 19)
	MusicSlider = GemRB.GetControl(SoundWindow, 4)
	MoviesButton = GemRB.GetControl(SoundWindow, 20)
	MoviesSlider = GemRB.GetControl(SoundWindow, 22)
	EnvironmentalButton = GemRB.GetControl(SoundWindow, 28)
	EnvironmentalButtonB = GemRB.GetControl(SoundWindow, 26)
	CharacterSoundButton = GemRB.GetControl(SoundWindow, 13)
	OkButton = GemRB.GetControl(SoundWindow, 24)
	CancelButton = GemRB.GetControl(SoundWindow, 25)
	GemRB.SetText(SoundWindow, TextAreaControl, 18040)
	GemRB.SetText(SoundWindow, CharacterSoundButton, 17778)
	GemRB.SetText(SoundWindow, OkButton, 11973)
	GemRB.SetText(SoundWindow, CancelButton, 13727)
	GemRB.SetEvent(SoundWindow, AmbientButton, IE_GUI_BUTTON_ON_PRESS, "AmbientPress")
	GemRB.SetEvent(SoundWindow, AmbientSlider, IE_GUI_SLIDER_ON_CHANGE, "AmbientPressB")
	GemRB.SetEvent(SoundWindow, SoundEffectsButton, IE_GUI_BUTTON_ON_PRESS, "SoundEffectsPress")
	GemRB.SetEvent(SoundWindow, SoundEffectsSlider, IE_GUI_SLIDER_ON_CHANGE, "SoundEffectsPressB")
	GemRB.SetEvent(SoundWindow, DialogueButton, IE_GUI_BUTTON_ON_PRESS, "DialoguePress")
	GemRB.SetEvent(SoundWindow, DialogueSlider, IE_GUI_SLIDER_ON_CHANGE, "DialoguePressB")
	GemRB.SetEvent(SoundWindow, MusicButton, IE_GUI_BUTTON_ON_PRESS, "MusicPress")
	GemRB.SetEvent(SoundWindow, MusicSlider, IE_GUI_SLIDER_ON_CHANGE, "MusicPressB")
	GemRB.SetEvent(SoundWindow, MoviesButton, IE_GUI_BUTTON_ON_PRESS, "MoviesPress")
	GemRB.SetEvent(SoundWindow, MoviesSlider, IE_GUI_SLIDER_ON_CHANGE, "MoviesPressB")
	GemRB.SetEvent(SoundWindow, EnvironmentalButton, IE_GUI_BUTTON_ON_PRESS, "EnvironmentalPress")
	GemRB.SetEvent(SoundWindow, EnvironmentalButtonB, IE_GUI_BUTTON_ON_PRESS, "EnvironmentalPressB")
	GemRB.SetEvent(SoundWindow, CharacterSoundButton, IE_GUI_BUTTON_ON_PRESS, "CharacterSoundPress")
	GemRB.SetEvent(SoundWindow, OkButton, IE_GUI_BUTTON_ON_PRESS, "OkPress")
	GemRB.SetEvent(SoundWindow, CancelButton, IE_GUI_BUTTON_ON_PRESS, "CancelPress")
	GemRB.ShowModal(SoundWindow)
	return
	
def AmbientPress():
	GemRB.SetText(SoundWindow, TextAreaControl, 18008)
	return
	
def AmbientPressB():
	GemRB.SetText(SoundWindow, TextAreaControl, 18008)
#retrieve slider data
	return
	
def SoundEffectsPress():
	GemRB.SetText(SoundWindow, TextAreaControl, 18009)
	return
	
def SoundEffectsPressB():
	GemRB.SetText(SoundWindow, TextAreaControl, 18009)
#retrieve slider data
	return
	
def DialoguePress():
	GemRB.SetText(SoundWindow, TextAreaControl, 18010)
	return
	
def DialoguePressB():
	GemRB.SetText(SoundWindow, TextAreaControl, 18010)
#retrieve slider data
	return
	
def MusicPress():
	GemRB.SetText(SoundWindow, TextAreaControl, 18011)
	return
	
def MusicPressB():
	GemRB.SetText(SoundWindow, TextAreaControl, 18011)
#retrieve slider data
	return
	
def MoviesPress():
	GemRB.SetText(SoundWindow, TextAreaControl, 18012)
	return
	
def MoviesPressB():
	GemRB.SetText(SoundWindow, TextAreaControl, 18012)
#retrieve slider data
	return
	
def EnvironmentalPress():
	GemRB.SetText(SoundWindow, TextAreaControl, 18022)
	return
	
def EnvironmentalPressB():
	GemRB.SetText(SoundWindow, TextAreaControl, 18022)
#retrieve EAX button state
	return
	
def CharacterSoundPress():
	GemRB.UnloadWindow(SoundWindow)
	GemRB.SetNextScript("GUIOPT12")
	return
	
def OkPress():
	GemRB.UnloadWindow(SoundWindow)
	GemRB.SetNextScript("StartOpt")
	return
	
def CancelPress():
	GemRB.UnloadWindow(SoundWindow)
	GemRB.SetNextScript("StartOpt")
	return
