#gameplay
import GemRB

GamePlayWindow = 0
TextAreaControl = 0

def OnLoad():
	global GamePlayWindow, TextAreaControl
	GemRB.LoadWindowPack("GUIOPT", 640, 480)
	GamePlayWindow = GemRB.LoadWindow(8)
	TextAreaControl = GemRB.GetControl(GamePlayWindow, 40)
	DelayButton = GemRB.GetControl(GamePlayWindow, 21)
	DelaySlider = GemRB.GetControl(GamePlayWindow, 1)
	MouseSpdButton = GemRB.GetControl(GamePlayWindow, 22)
	MouseSpdSlider = GemRB.GetControl(GamePlayWindow, 2)
	KeySpdButton = GemRB.GetControl(GamePlayWindow, 23)
	KeySpdSlider = GemRB.GetControl(GamePlayWindow, 3)
	DifficultyButton = GemRB.GetControl(GamePlayWindow, 24)
	DifficultySlider = GemRB.GetControl(GamePlayWindow, 12)
	BloodButton = GemRB.GetControl(GamePlayWindow, 27)
	BloodButtonB = GemRB.GetControl(GamePlayWindow, 19)
	DitherButton = GemRB.GetControl(GamePlayWindow, 25)
	DitherButtonB = GemRB.GetControl(GamePlayWindow, 14)
	InfravisionButton = GemRB.GetControl(GamePlayWindow, 44)
	InfravisionButtonB = GemRB.GetControl(GamePlayWindow, 42)
	WeatherButton = GemRB.GetControl(GamePlayWindow, 46)
	WeatherButtonB = GemRB.GetControl(GamePlayWindow, 47)
	HealButton = GemRB.GetControl(GamePlayWindow, 48)
	HealButtonB = GemRB.GetControl(GamePlayWindow, 50)
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
	GemRB.SetEvent(GamePlayWindow, DelaySlider, IE_GUI_SLIDER_ON_CHANGE, "DelayPress")
	GemRB.SetVarAssoc(GamePlayWindow, DelaySlider, "Tooltips",0)

	GemRB.SetEvent(GamePlayWindow, KeySpdButton, IE_GUI_BUTTON_ON_PRESS, "KeySpdPress")
	GemRB.SetEvent(GamePlayWindow, KeySpdSlider, IE_GUI_SLIDER_ON_CHANGE, "KeySpdPress")
	GemRB.SetVarAssoc(GamePlayWindow, KeySpdSlider, "Keyboard Scroll Speed",0)

	GemRB.SetEvent(GamePlayWindow, MouseSpdButton, IE_GUI_BUTTON_ON_PRESS, "MouseSpdPress")
	GemRB.SetEvent(GamePlayWindow, MouseSpdSlider, IE_GUI_SLIDER_ON_CHANGE, "MouseSpdPress")
	GemRB.SetVarAssoc(GamePlayWindow, MouseSpdSlider, "Mouse Scroll Speed",0)

	GemRB.SetEvent(GamePlayWindow, DifficultyButton, IE_GUI_BUTTON_ON_PRESS, "DifficultyPress")
	GemRB.SetEvent(GamePlayWindow, DifficultySlider, IE_GUI_SLIDER_ON_CHANGE, "DifficultyPress")
	GemRB.SetVarAssoc(GamePlayWindow, DifficultySlider, "Difficulty Level",0)

	GemRB.SetEvent(GamePlayWindow, BloodButton, IE_GUI_BUTTON_ON_PRESS, "BloodPress")
	GemRB.SetEvent(GamePlayWindow, BloodButtonB, IE_GUI_BUTTON_ON_PRESS, "BloodPress")
	GemRB.SetButtonFlags(GamePlayWindow, BloodButtonB, IE_GUI_BUTTON_CHECKBOX,OP_OR)
	GemRB.SetVarAssoc(GamePlayWindow, BloodButtonB, "Gore",1)

	GemRB.SetEvent(GamePlayWindow, DitherButton, IE_GUI_BUTTON_ON_PRESS, "DitherPress")
	GemRB.SetEvent(GamePlayWindow, DitherButtonB, IE_GUI_BUTTON_ON_PRESS, "DitherPress")
	GemRB.SetButtonFlags(GamePlayWindow, DitherButtonB, IE_GUI_BUTTON_CHECKBOX,OP_OR)
	GemRB.SetVarAssoc(GamePlayWindow, DitherButtonB, "Always Dither",1)

	GemRB.SetEvent(GamePlayWindow, InfravisionButton, IE_GUI_BUTTON_ON_PRESS, "InfravisionPress")
	GemRB.SetEvent(GamePlayWindow, InfravisionButtonB, IE_GUI_BUTTON_ON_PRESS, "InfravisionPress")
	GemRB.SetButtonFlags(GamePlayWindow, InfravisionButtonB, IE_GUI_BUTTON_CHECKBOX,OP_OR)
	GemRB.SetVarAssoc(GamePlayWindow, InfravisionButtonB, "Infravision",1)

	GemRB.SetEvent(GamePlayWindow, WeatherButton, IE_GUI_BUTTON_ON_PRESS, "WeatherPress")
	GemRB.SetEvent(GamePlayWindow, WeatherButtonB, IE_GUI_BUTTON_ON_PRESS, "WeatherPress")
	GemRB.SetButtonFlags(GamePlayWindow, WeatherButtonB, IE_GUI_BUTTON_CHECKBOX,OP_OR)
	GemRB.SetVarAssoc(GamePlayWindow, WeatherButtonB, "Weather",1)

	GemRB.SetEvent(GamePlayWindow, HealButton, IE_GUI_BUTTON_ON_PRESS, "HealPress")
	GemRB.SetEvent(GamePlayWindow, HealButtonB, IE_GUI_BUTTON_ON_PRESS, "HealPressB")
	GemRB.SetButtonFlags(GamePlayWindow, HealButtonB, IE_GUI_BUTTON_CHECKBOX,OP_OR)
	GemRB.SetVarAssoc(GamePlayWindow, HealButtonB, "Heal Party on Rest",1)

	GemRB.SetEvent(GamePlayWindow, HotKeyButton, IE_GUI_BUTTON_ON_PRESS, "HotKeyPress")
	GemRB.SetEvent(GamePlayWindow, FeedbackButton, IE_GUI_BUTTON_ON_PRESS, "FeedbackPress")
	GemRB.SetEvent(GamePlayWindow, AutoPauseButton, IE_GUI_BUTTON_ON_PRESS, "AutoPausePress")
	GemRB.SetEvent(GamePlayWindow, OkButton, IE_GUI_BUTTON_ON_PRESS, "OkPress")
	GemRB.SetEvent(GamePlayWindow, CancelButton, IE_GUI_BUTTON_ON_PRESS, "CancelPress")
	GemRB.SetVisible(GamePlayWindow,1)
	return
	
def DelayPress():
	GemRB.SetText(GamePlayWindow, TextAreaControl, 18017)
	return
	
def KeySpdPress():
	GemRB.SetText(GamePlayWindow, TextAreaControl, 18019)
	return
	
def MouseSpdPress():
	GemRB.SetText(GamePlayWindow, TextAreaControl, 18018)
	return
	
def DifficultyPress():
	GemRB.SetText(GamePlayWindow, TextAreaControl, 18020)
	return
	
def BloodPress():
	GemRB.SetText(GamePlayWindow, TextAreaControl, 18023)
	return
	
def DitherPress():
	GemRB.SetText(GamePlayWindow, TextAreaControl, 18021)
	return
	
def InfravisionPress():
	GemRB.SetText(GamePlayWindow, TextAreaControl, 11797)
	return
	
def WeatherPress():
	GemRB.SetText(GamePlayWindow, TextAreaControl, 20619)
	return
	
def HealPress():
	GemRB.SetText(GamePlayWindow, TextAreaControl, 2242)
	return
	
def HotKeyPress():
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
	GemRB.SetVisible(GamePlayWindow, 0)
	GemRB.UnloadWindow(GamePlayWindow)
	GemRB.SetNextScript("StartOpt")
	return
	
def CancelPress():
	GemRB.SetVisible(GamePlayWindow, 0)
	GemRB.UnloadWindow(GamePlayWindow)
	GemRB.SetNextScript("StartOpt")
	return
