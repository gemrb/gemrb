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
	GemRB.SetEvent(SoundWindow, AmbientSlider, IE_GUI_SLIDER_ON_CHANGE, "AmbientPress")
	GemRB.SetVarAssoc(SoundWindow, AmbientSlider, "Volume Ambients",10);

	GemRB.SetEvent(SoundWindow, SoundEffectsButton, IE_GUI_BUTTON_ON_PRESS, "SoundEffectsPress")
	GemRB.SetEvent(SoundWindow, SoundEffectsSlider, IE_GUI_SLIDER_ON_CHANGE, "SoundEffectsPress")
	GemRB.SetVarAssoc(SoundWindow, SoundEffectsSlider, "Volume SFX",10);
	
	GemRB.SetEvent(SoundWindow, DialogueButton, IE_GUI_BUTTON_ON_PRESS, "DialoguePress")
	GemRB.SetEvent(SoundWindow, DialogueSlider, IE_GUI_SLIDER_ON_CHANGE, "DialoguePress")
	GemRB.SetVarAssoc(SoundWindow, DialogueSlider, "Volume Voices",10);

	GemRB.SetEvent(SoundWindow, MusicButton, IE_GUI_BUTTON_ON_PRESS, "MusicPress")
	GemRB.SetEvent(SoundWindow, MusicSlider, IE_GUI_SLIDER_ON_CHANGE, "MusicPress")
	GemRB.SetVarAssoc(SoundWindow, MusicSlider, "Volume Music",10);

	GemRB.SetEvent(SoundWindow, MoviesButton, IE_GUI_BUTTON_ON_PRESS, "MoviesPress")
	GemRB.SetEvent(SoundWindow, MoviesSlider, IE_GUI_SLIDER_ON_CHANGE, "MoviesPress")
	GemRB.SetVarAssoc(SoundWindow, MoviesSlider, "Volume Movie",10);

	GemRB.SetEvent(SoundWindow, EnvironmentalButton, IE_GUI_BUTTON_ON_PRESS, "EnvironmentalPress")
	GemRB.SetEvent(SoundWindow, EnvironmentalButtonB, IE_GUI_BUTTON_ON_PRESS, "EnvironmentalPress")
	GemRB.SetButtonFlags(SoundWindow, EnvironmentalButtonB,IE_GUI_BUTTON_CHECKBOX,OP_OR)
	GemRB.SetVarAssoc(SoundWindow, EnvironmentalButtonB,"Environmental Audio",1)
	GemRB.SetButtonSprites(SoundWindow, EnvironmentalButtonB, "GBTNOPT4", 0, 0, 1, 2, 3)

	GemRB.SetEvent(SoundWindow, CharacterSoundButton, IE_GUI_BUTTON_ON_PRESS, "CharacterSoundPress")
	GemRB.SetEvent(SoundWindow, OkButton, IE_GUI_BUTTON_ON_PRESS, "OkPress")
	GemRB.SetEvent(SoundWindow, CancelButton, IE_GUI_BUTTON_ON_PRESS, "CancelPress")
	GemRB.SetVisible(SoundWindow,1)
	return
	
def AmbientPress():
	GemRB.SetText(SoundWindow, TextAreaControl, 18008)
	return
	
def SoundEffectsPress():
	GemRB.SetText(SoundWindow, TextAreaControl, 18009)
	return
	
def DialoguePress():
	GemRB.SetText(SoundWindow, TextAreaControl, 18010)
	return
	
def MusicPress():
	GemRB.SetText(SoundWindow, TextAreaControl, 18011)
	return
	
def MoviesPress():
	GemRB.SetText(SoundWindow, TextAreaControl, 18012)
	return
	
def EnvironmentalPress():
	GemRB.SetText(SoundWindow, TextAreaControl, 18022)
	return
	
def CharacterSoundPress():
	GemRB.UnloadWindow(SoundWindow)
	GemRB.SetNextScript("CharSound")
	return
	
def OkPress():
	GemRB.UnloadWindow(SoundWindow)
	GemRB.SetNextScript("Options")
	return
	
def CancelPress():
	GemRB.UnloadWindow(SoundWindow)
	GemRB.SetNextScript("Options")
	return