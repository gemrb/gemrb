import GemRB

SoundWindow = 0
TextAreaControl = 0

def OnLoad():
	global SoundWindow, TextAreaControl
	GemRB.LoadWindowPack("GUIOPT")
	SoundWindow = GemRB.LoadWindow(7)
	TextAreaControl = GemRB.GetControl(SoundWindow, 14)
	AmbientButton = GemRB.GetControl(SoundWindow, 16)
	SoundEffectsButton = GemRB.GetControl(SoundWindow, 17)
	DialogueButton = GemRB.GetControl(SoundWindow, 18)
	MusicButton = GemRB.GetControl(SoundWindow, 19)
	MoviesButton = GemRB.GetControl(SoundWindow, 20)
	AmbientalButton = GemRB.GetControl(SoundWindow, 28)
	CharacterSoundButton = GemRB.GetControl(SoundWindow, 13)
	OkButton = GemRB.GetControl(SoundWindow, 24)
	CancelButton = GemRB.GetControl(SoundWindow, 25)
	GemRB.SetText(SoundWindow, TextAreaControl, 18040)
	GemRB.SetText(SoundWindow, CharacterSoundButton, 17778)
	GemRB.SetText(SoundWindow, OkButton, 11973)
	GemRB.SetText(SoundWindow, CancelButton, 13727)
	GemRB.SetButtonFlags(SoundWindow, AmbientButton, IE_GUI_BUTTON_NO_IMAGE, IE_GUI_BUTTON_NO_PICTURE, IE_GUI_BUTTON_NO_SOUND)
	GemRB.SetButtonFlags(SoundWindow, SoundEffectsButton, IE_GUI_BUTTON_NO_IMAGE, IE_GUI_BUTTON_NO_PICTURE, IE_GUI_BUTTON_NO_SOUND)
	GemRB.SetButtonFlags(SoundWindow, DialogueButton, IE_GUI_BUTTON_NO_IMAGE, IE_GUI_BUTTON_NO_PICTURE, IE_GUI_BUTTON_NO_SOUND)
	GemRB.SetButtonFlags(SoundWindow, MusicButton, IE_GUI_BUTTON_NO_IMAGE, IE_GUI_BUTTON_NO_PICTURE, IE_GUI_BUTTON_NO_SOUND)
	GemRB.SetButtonFlags(SoundWindow, MoviesButton, IE_GUI_BUTTON_NO_IMAGE, IE_GUI_BUTTON_NO_PICTURE, IE_GUI_BUTTON_NO_SOUND)
	GemRB.SetButtonFlags(SoundWindow, AmbientalButton, IE_GUI_BUTTON_NO_IMAGE, IE_GUI_BUTTON_NO_PICTURE, IE_GUI_BUTTON_NO_SOUND)
	GemRB.SetButtonFlags(SoundWindow, CharacterSoundButton, IE_GUI_BUTTON_IMAGE, IE_GUI_BUTTON_NO_PICTURE, IE_GUI_BUTTON_SOUND)
	GemRB.SetButtonFlags(SoundWindow, OkButton, IE_GUI_BUTTON_IMAGE, IE_GUI_BUTTON_NO_PICTURE, IE_GUI_BUTTON_SOUND)
	GemRB.SetButtonFlags(SoundWindow, CancelButton, IE_GUI_BUTTON_IMAGE, IE_GUI_BUTTON_NO_PICTURE, IE_GUI_BUTTON_SOUND)
	GemRB.SetEvent(SoundWindow, AmbientButton, IE_GUI_BUTTON_ON_PRESS, "AmbientPress")
	GemRB.SetEvent(SoundWindow, SoundEffectsButton, IE_GUI_BUTTON_ON_PRESS, "SoundEffectsPress")
	GemRB.SetEvent(SoundWindow, DialogueButton, IE_GUI_BUTTON_ON_PRESS, "DialoguePress")
	GemRB.SetEvent(SoundWindow, MusicButton, IE_GUI_BUTTON_ON_PRESS, "MusicPress")
	GemRB.SetEvent(SoundWindow, MoviesButton, IE_GUI_BUTTON_ON_PRESS, "MoviesPress")
	GemRB.SetEvent(SoundWindow, AmbientalButton, IE_GUI_BUTTON_ON_PRESS, "AmbientalPress")
	GemRB.SetEvent(SoundWindow, CharacterSoundButton, IE_GUI_BUTTON_ON_PRESS, "CharacterSoundPress")
	GemRB.SetEvent(SoundWindow, OkButton, IE_GUI_BUTTON_ON_PRESS, "OkPress")
	GemRB.SetEvent(SoundWindow, CancelButton, IE_GUI_BUTTON_ON_PRESS, "CancelPress")
	GemRB.ShowModal(SoundWindow)
	return
	
def AmbientPress():
	global SoundWindow, TextAreaControl
	GemRB.SetText(SoundWindow, TextAreaControl, 18008)
	return
	
def SoundEffectsPress():
	global SoundWindow, TextAreaControl
	GemRB.SetText(SoundWindow, TextAreaControl, 18009)
	return
	
def DialoguePress():
	global SoundWindow, TextAreaControl
	GemRB.SetText(SoundWindow, TextAreaControl, 18010)
	return
	
def MusicPress():
	global SoundWindow, TextAreaControl
	GemRB.SetText(SoundWindow, TextAreaControl, 18011)
	return
	
def MoviesPress():
	global SoundWindow, TextAreaControl
	GemRB.SetText(SoundWindow, TextAreaControl, 18012)
	return
	
def AmbientalPress():
	global SoundWindow, TextAreaControl
	GemRB.SetText(SoundWindow, TextAreaControl, 18022)
	return
	
def CharacterSoundPress():
	global SoundWindow, TextAreaControl
	GemRB.UnloadWindow(SoundWindow)
	GemRB.SetNextScript("GUIOPT12")
	return
	
def OkPress():
	global SoundWindow, TextAreaControl
	GemRB.UnloadWindow(SoundWindow)
	GemRB.SetNextScript("StartOpt")
	return
	
def CancelPress():
	global SoundWindow, TextAreaControl
	GemRB.UnloadWindow(SoundWindow)
	GemRB.SetNextScript("StartOpt")
	return
