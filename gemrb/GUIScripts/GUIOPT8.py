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
	GemRB.SetEvent(GamePlayWindow, DelayButton, IE_GUI_BUTTON_ON_PRESS, "DelayPress")
	GemRB.SetEvent(GamePlayWindow, KeySpdButton, IE_GUI_BUTTON_ON_PRESS, "KeySpdPress")
	GemRB.SetEvent(GamePlayWindow, MouseSpdButton, IE_GUI_BUTTON_ON_PRESS, "MouseSpdPress")
	GemRB.SetEvent(GamePlayWindow, DifficultyButton, IE_GUI_BUTTON_ON_PRESS, "DifficultyPress")
	GemRB.SetEvent(GamePlayWindow, BloodButton, IE_GUI_BUTTON_ON_PRESS, "BloodPress")
	GemRB.SetEvent(GamePlayWindow, ShowPGButton, IE_GUI_BUTTON_ON_PRESS, "ShowPGPress")
	GemRB.SetEvent(GamePlayWindow, InfravisionButton, IE_GUI_BUTTON_ON_PRESS, "InfravisionPress")
	GemRB.SetEvent(GamePlayWindow, WeatherButton, IE_GUI_BUTTON_ON_PRESS, "WeatherPress")
	GemRB.SetEvent(GamePlayWindow, HealButton, IE_GUI_BUTTON_ON_PRESS, "HealPress")
	GemRB.SetEvent(GamePlayWindow, HotKeyButton, IE_GUI_BUTTON_ON_PRESS, "HotKeyPress")
	GemRB.SetEvent(GamePlayWindow, FeedbackButton, IE_GUI_BUTTON_ON_PRESS, "FeedbackPress")
	GemRB.SetEvent(GamePlayWindow, AutoPauseButton, IE_GUI_BUTTON_ON_PRESS, "AutoPausePress")
	GemRB.SetEvent(GamePlayWindow, OkButton, IE_GUI_BUTTON_ON_PRESS, "OkPress")
	GemRB.SetEvent(GamePlayWindow, CancelButton, IE_GUI_BUTTON_ON_PRESS, "CancelPress")
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
