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
	GemRB.SetEvent(SoundWindow, AmbientButton, 0x00000000, "AmbientPress")
	GemRB.SetEvent(SoundWindow, SoundEffectsButton, 0x00000000, "SoundEffectsPress")
	GemRB.SetEvent(SoundWindow, DialogueButton, 0x00000000, "DialoguePress")
	GemRB.SetEvent(SoundWindow, MusicButton, 0x00000000, "MusicPress")
	GemRB.SetEvent(SoundWindow, MoviesButton, 0x00000000, "MoviesPress")
	GemRB.SetEvent(SoundWindow, AmbientalButton, 0x00000000, "AmbientalPress")
	GemRB.SetEvent(SoundWindow, CharacterSoundButton, 0x00000000, "CharacterSoundPress")
	GemRB.SetEvent(SoundWindow, OkButton, 0x00000000, "OkPress")
	GemRB.SetEvent(SoundWindow, CancelButton, 0x00000000, "CancelPress")
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
