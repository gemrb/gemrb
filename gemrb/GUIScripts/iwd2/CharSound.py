#Character Sound Options Menu
import GemRB

CSoundWindow = 0
TextAreaControl = 0

def OnLoad():
	global CSoundWindow, TextAreaControl

	GemRB.LoadWindowPack("GUIOPT")
	CSoundWindow = GemRB.LoadWindow(12)
	TextAreaControl = GemRB.GetControl(CSoundWindow, 16)
	SubtitlesButton = GemRB.GetControl(CSoundWindow, 20)
	AttackSoundButton = GemRB.GetControl(CSoundWindow, 18)
	MovementSoundButton = GemRB.GetControl(CSoundWindow, 19)
	CommandSoundButton = GemRB.GetControl(CSoundWindow, 21)
	SelectionSoundButton = GemRB.GetControl(CSoundWindow, 57)
	SubtitlesButtonB = GemRB.GetControl(CSoundWindow, 5)
	AttackSoundButtonB = GemRB.GetControl(CSoundWindow, 6)
	MovementSoundButtonB = GemRB.GetControl(CSoundWindow, 7)
	CSAlwaysButtonB = GemRB.GetControl(CSoundWindow, 8)
	CSSeldomButtonB = GemRB.GetControl(CSoundWindow, 9)
	CSNeverButtonB = GemRB.GetControl(CSoundWindow, 10)
	SSAlwaysButtonB = GemRB.GetControl(CSoundWindow, 58)
	SSSeldomButtonB = GemRB.GetControl(CSoundWindow, 59)
	SSNeverButtonB = GemRB.GetControl(CSoundWindow, 60)
	OkButton = GemRB.GetControl(CSoundWindow, 24)
	CancelButton = GemRB.GetControl(CSoundWindow, 25)
	GemRB.SetText(CSoundWindow, TextAreaControl, 18040)
	GemRB.SetText(CSoundWindow, OkButton, 11973)
	GemRB.SetText(CSoundWindow, CancelButton, 13727)
	
	GemRB.SetEvent(CSoundWindow, SubtitlesButton, IE_GUI_BUTTON_ON_PRESS, "SubtitlesPress")
	GemRB.SetEvent(CSoundWindow, SubtitlesButtonB, IE_GUI_BUTTON_ON_PRESS, "SubtitlesPress")
	GemRB.SetButtonFlags(CSoundWindow, SubtitlesButtonB, IE_GUI_BUTTON_CHECKBOX, OP_OR)
	GemRB.SetVarAssoc(CSoundWindow, SubtitlesButtonB, "Subtitles", 1)
	GemRB.SetButtonSprites(CSoundWindow, SubtitlesButtonB, "GBTNOPT4", 0, 0, 1, 2, 3)
	
	GemRB.SetEvent(CSoundWindow, AttackSoundButton, IE_GUI_BUTTON_ON_PRESS, "AttackSoundPress")
	GemRB.SetEvent(CSoundWindow, AttackSoundButtonB, IE_GUI_BUTTON_ON_PRESS, "AttackSoundPress")
	GemRB.SetButtonFlags(CSoundWindow, AttackSoundButtonB, IE_GUI_BUTTON_CHECKBOX, OP_OR)
	GemRB.SetVarAssoc(CSoundWindow, AttackSoundButtonB, "Attack Sound", 1) #can't find the right variable name, this is a dummy name
	GemRB.SetButtonSprites(CSoundWindow, AttackSoundButtonB, "GBTNOPT4", 0, 0, 1, 2, 3)
	
	GemRB.SetEvent(CSoundWindow, MovementSoundButton, IE_GUI_BUTTON_ON_PRESS, "MovementSoundPress")
	GemRB.SetEvent(CSoundWindow, MovementSoundButtonB, IE_GUI_BUTTON_ON_PRESS, "MovementSoundPress")
	GemRB.SetButtonFlags(CSoundWindow, MovementSoundButtonB, IE_GUI_BUTTON_CHECKBOX, OP_OR)
	GemRB.SetVarAssoc(CSoundWindow, MovementSoundButtonB, "Movement Sound", 1) #can't find the right variable name, this is a dummy name
	GemRB.SetButtonSprites(CSoundWindow, MovementSoundButtonB, "GBTNOPT4", 0, 0, 1, 2, 3)
	
	GemRB.SetEvent(CSoundWindow, CommandSoundButton, IE_GUI_BUTTON_ON_PRESS, "CommandSoundPress")
	GemRB.SetEvent(CSoundWindow, CSAlwaysButtonB, IE_GUI_BUTTON_ON_PRESS, "CommandSoundPress")
	GemRB.SetButtonFlags(CSoundWindow, CSAlwaysButtonB, IE_GUI_BUTTON_RADIOBUTTON, OP_OR)
	GemRB.SetVarAssoc(CSoundWindow, CSAlwaysButtonB, "Command Sounds Frequency", 3)
	GemRB.SetButtonSprites(CSoundWindow, CSAlwaysButtonB, "GBTNOPT4", 0, 0, 1, 2, 3)
	GemRB.SetEvent(CSoundWindow, CSSeldomButtonB, IE_GUI_BUTTON_ON_PRESS, "CommandSoundPress")
	GemRB.SetButtonFlags(CSoundWindow, CSSeldomButtonB, IE_GUI_BUTTON_RADIOBUTTON, OP_OR)
	GemRB.SetVarAssoc(CSoundWindow, CSSeldomButtonB, "Command Sounds Frequency", 2)
	GemRB.SetButtonSprites(CSoundWindow, CSSeldomButtonB, "GBTNOPT4", 0, 0, 1, 2, 3)
	GemRB.SetEvent(CSoundWindow, CSNeverButtonB, IE_GUI_BUTTON_ON_PRESS, "CommandSoundPress")
	GemRB.SetButtonFlags(CSoundWindow, CSNeverButtonB, IE_GUI_BUTTON_RADIOBUTTON, OP_OR)
	GemRB.SetVarAssoc(CSoundWindow, CSNeverButtonB, "Command Sounds Frequency", 1)
	GemRB.SetButtonSprites(CSoundWindow, CSNeverButtonB, "GBTNOPT4", 0, 0, 1, 2, 3)
	
	GemRB.SetEvent(CSoundWindow, SelectionSoundButton, IE_GUI_BUTTON_ON_PRESS, "SelectionSoundPress")
	GemRB.SetEvent(CSoundWindow, SSAlwaysButtonB, IE_GUI_BUTTON_ON_PRESS, "SelectionSoundPress")
	GemRB.SetButtonFlags(CSoundWindow, SSAlwaysButtonB, IE_GUI_BUTTON_RADIOBUTTON, OP_OR)
	GemRB.SetVarAssoc(CSoundWindow, SSAlwaysButtonB, "Selection Sounds Frequency", 3)
	GemRB.SetButtonSprites(CSoundWindow, SSAlwaysButtonB, "GBTNOPT4", 0, 0, 1, 2, 3)
	GemRB.SetEvent(CSoundWindow, SSSeldomButtonB, IE_GUI_BUTTON_ON_PRESS, "SelectionSoundPress")
	GemRB.SetButtonFlags(CSoundWindow, SSSeldomButtonB, IE_GUI_BUTTON_RADIOBUTTON, OP_OR)
	GemRB.SetVarAssoc(CSoundWindow, SSSeldomButtonB, "Selection Sounds Frequency", 2)
	GemRB.SetButtonSprites(CSoundWindow, SSSeldomButtonB, "GBTNOPT4", 0, 0, 1, 2, 3)
	GemRB.SetEvent(CSoundWindow, SSNeverButtonB, IE_GUI_BUTTON_ON_PRESS, "SelectionSoundPress")
	GemRB.SetButtonFlags(CSoundWindow, SSNeverButtonB, IE_GUI_BUTTON_RADIOBUTTON, OP_OR)
	GemRB.SetVarAssoc(CSoundWindow, SSNeverButtonB, "Selection Sounds Frequency", 1)
	GemRB.SetButtonSprites(CSoundWindow, SSNeverButtonB, "GBTNOPT4", 0, 0, 1, 2, 3)
	
	GemRB.SetEvent(CSoundWindow, OkButton, IE_GUI_BUTTON_ON_PRESS, "OkPress")
	GemRB.SetEvent(CSoundWindow, CancelButton, IE_GUI_BUTTON_ON_PRESS, "CancelPress")
	GemRB.SetVisible(CSoundWindow,1)
	return
	
def AmbientPress():
	GemRB.SetText(CSoundWindow, TextAreaControl, 18008)
	return
	
def SoundEffectsPress():
	GemRB.SetText(CSoundWindow, TextAreaControl, 18009)
	return
	
def DialoguePress():
	GemRB.SetText(CSoundWindow, TextAreaControl, 18010)
	return
	
def MusicPress():
	GemRB.SetText(CSoundWindow, TextAreaControl, 18011)
	return
	
def MoviesPress():
	GemRB.SetText(CSoundWindow, TextAreaControl, 18012)
	return
	
def EnvironmentalPress():
	GemRB.SetText(CSoundWindow, TextAreaControl, 18022)
	return
	
def CharacterSoundPress():
	GemRB.UnloadWindow(CSoundWindow)
	GemRB.SetNextScript("GUIOPT12")
	return
	
def OkPress():
	GemRB.UnloadWindow(CSoundWindow)
	GemRB.SetNextScript("Options")
	return
	
def CancelPress():
	GemRB.UnloadWindow(CSoundWindow)
	GemRB.SetNextScript("Options")
	return