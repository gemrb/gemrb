import GemRB

GamePlayWindow = 0
TextAreaControl = 0

def OnLoad():
	global GamePlayWindow, TextAreaControl
	GemRB.LoadWindowPack("GUIOPT")
	GamePlayWindow = GemRB.LoadWindow(8)
	TextAreaControl = GemRB.GetControl(GamePlayWindow, 40)
	DelayButton = GemRB.GetControl(GamePlayWindow, 21)
	KeySpdButton = GemRB.GetControl(GamePlayWindow, 23)
	MouseSpdButton = GemRB.GetControl(GamePlayWindow, 22)
	DifficultyButton = GemRB.GetControl(GamePlayWindow, 24)
	BloodButton = GemRB.GetControl(GamePlayWindow, 27)
	ShowPGButton = GemRB.GetControl(GamePlayWindow, 25)
	InfravisionButton = GemRB.GetControl(GamePlayWindow, 44)
	WeatherButton = GemRB.GetControl(GamePlayWindow, 46)
	HealButton = GemRB.GetControl(GamePlayWindow, 48)
	HotKeyButton = GemRB.GetControl(GamePlayWindow, 51)
	FeedbackButton = GemRB.GetControl(GamePlayWindow, 5)
	AutoPauseButton = GemRB.GetControl(GamePlayWindow, 6)
	OkButton = GemRB.GetControl(GamePlayWindow, 7)
	CancelButton = GemRB.GetControl(GamePlayWindow, 20)
	GemRB.SetText(GamePlayWindow, TextAreaControl, 18042)
	GemRB.SetText(GamePlayWindow, OkButton, 11973)
	GemRB.SetText(GamePlayWindow, CancelButton, 13727)
	GemRB.SetText(GamePlayWindow, HotKeyButton, 816)
	GemRB.SetText(GamePlayWindow, FeedbackButton, 17163)
	GemRB.SetText(GamePlayWindow, AutoPauseButton, 17166)
	GemRB.SetEvent(GamePlayWindow, DelayButton, 0x00000000, "DelayPress")
	GemRB.SetEvent(GamePlayWindow, KeySpdButton, 0x00000000, "KeySpdPress")
	GemRB.SetEvent(GamePlayWindow, MouseSpdButton, 0x00000000, "MouseSpdPress")
	GemRB.SetEvent(GamePlayWindow, DifficultyButton, 0x00000000, "DifficultyPress")
	GemRB.SetEvent(GamePlayWindow, BloodButton, 0x00000000, "BloodPress")
	GemRB.SetEvent(GamePlayWindow, ShowPGButton, 0x00000000, "ShowPGPress")
	GemRB.SetEvent(GamePlayWindow, InfravisionButton, 0x00000000, "InfravisionPress")
	GemRB.SetEvent(GamePlayWindow, WeatherButton, 0x00000000, "WeatherPress")
	GemRB.SetEvent(GamePlayWindow, HealButton, 0x00000000, "HealPress")
	GemRB.SetEvent(GamePlayWindow, HotKeyButton, 0x00000000, "HotKeyPress")
	GemRB.SetEvent(GamePlayWindow, FeedbackButton, 0x00000000, "FeedbackPress")
	GemRB.SetEvent(GamePlayWindow, AutoPauseButton, 0x00000000, "AutoPausePress")
	GemRB.SetEvent(GamePlayWindow, OkButton, 0x00000000, "OkPress")
	GemRB.SetEvent(GamePlayWindow, CancelButton, 0x00000000, "CancelPress")
	GemRB.ShowModal(GamePlayWindow)
	return
	
def DelayPress():
	global GamePlayWindow, TextAreaControl
	GemRB.SetText(GamePlayWindow, TextAreaControl, 18017)
	return
	
def KeySpdPress():
	global GamePlayWindow, TextAreaControl
	GemRB.SetText(GamePlayWindow, TextAreaControl, 18019)
	return
	
def MouseSpdPress():
	global GamePlayWindow, TextAreaControl
	GemRB.SetText(GamePlayWindow, TextAreaControl, 18018)
	return
	
def DifficultyPress():
	global GamePlayWindow, TextAreaControl
	GemRB.SetText(GamePlayWindow, TextAreaControl, 18020)
	return
	
def BloodPress():
	global GamePlayWindow, TextAreaControl
	GemRB.SetText(GamePlayWindow, TextAreaControl, 18023)
	return
	
def ShowPGPress():
	global GamePlayWindow, TextAreaControl
	GemRB.SetText(GamePlayWindow, TextAreaControl, 18021)
	return
	
def InfravisionPress():
	global GamePlayWindow, TextAreaControl
	GemRB.SetText(GamePlayWindow, TextAreaControl, 11797)
	return
	
def WeatherPress():
	global GamePlayWindow, TextAreaControl
	GemRB.SetText(GamePlayWindow, TextAreaControl, 20619)
	return
	
def HealPress():
	global GamePlayWindow, TextAreaControl
	GemRB.SetText(GamePlayWindow, TextAreaControl, 2242)
	return
	
def HotKeyPress():
	global GamePlayWindow, TextAreaControl
	#GemRB.SetText(GamePlayWindow, TextAreaControl, 18016)
	return
	
def FeedbackPress():
	GemRB.SetVisible(GamePlayWindow, 0)
	GemRB.UnloadWindow(GamePlayWindow)
	GemRB.SetNextScript("GUIOPT9")
	return
	
def AutoPausePress():
	GemRB.SetVisible(GamePlayWindow, 0)
	GemRB.UnloadWindow(GamePlayWindow)
	GemRB.SetNextScript("GUIOPT10")
	return
	
def OkPress():
	global GamePlayWindow, TextAreaControl
	GemRB.SetVisible(GamePlayWindow, 0)
	GemRB.UnloadWindow(GamePlayWindow)
	GemRB.SetNextScript("StartOpt")
	return
	
def CancelPress():
	global GamePlayWindow, TextAreaControl
	GemRB.SetVisible(GamePlayWindow, 0)
	GemRB.UnloadWindow(GamePlayWindow)
	GemRB.SetNextScript("StartOpt")
	return
