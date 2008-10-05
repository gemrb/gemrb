#Options Menu
import GemRB

OptionsWindow = 0

def OnLoad():
	global OptionsWindow
	GemRB.LoadWindowPack("GUIOPT", 800, 600)
	
	MessageBarWindow = GemRB.LoadWindow(0)
	GemRB.SetVisible(MessageBarWindow, 1) #This will startup the window as grayed
	
	CharactersBarWindow = GemRB.LoadWindow(1)
	GemRB.SetVisible(CharactersBarWindow, 1)
	
	GemRB.DrawWindows()
	
	GemRB.SetVisible(MessageBarWindow, 0)
	GemRB.SetVisible(CharactersBarWindow, 0)
	
	GemRB.UnloadWindow(MessageBarWindow)
	GemRB.UnloadWindow(CharactersBarWindow)
	
	OptionsWindow = GemRB.LoadWindow(13)
	GemRB.SetWindowFrame (OptionsWindow)
	
	VersionLabel = GemRB.GetControl(OptionsWindow, 0x1000000B)
	GemRB.SetText(OptionsWindow, VersionLabel, GEMRB_VERSION)
	
	GraphicsButton = GemRB.GetControl(OptionsWindow, 7)
	SoundButton = GemRB.GetControl(OptionsWindow, 8)
	GamePlayButton = GemRB.GetControl(OptionsWindow, 9)
	MoviesButton = GemRB.GetControl(OptionsWindow, 14)
	KeyboardButton = GemRB.GetControl(OptionsWindow, 13)
	ReturnButton = GemRB.GetControl(OptionsWindow, 11)
	
	GemRB.SetText(OptionsWindow, GraphicsButton, 17162)
	GemRB.SetEvent(OptionsWindow, GraphicsButton, IE_GUI_BUTTON_ON_PRESS, "GraphicsPress")
	GemRB.SetText(OptionsWindow, SoundButton, 17164)
	GemRB.SetEvent(OptionsWindow, SoundButton, IE_GUI_BUTTON_ON_PRESS, "SoundPress")
	GemRB.SetText(OptionsWindow, GamePlayButton, 17165)
	GemRB.SetEvent(OptionsWindow, GamePlayButton, IE_GUI_BUTTON_ON_PRESS, "GamePlayPress")
	GemRB.SetText(OptionsWindow, MoviesButton, 15415)
	GemRB.SetEvent(OptionsWindow, MoviesButton, IE_GUI_BUTTON_ON_PRESS, "MoviePress")
	GemRB.SetText(OptionsWindow, KeyboardButton, 33468)
	GemRB.SetEvent(OptionsWindow, MoviesButton, IE_GUI_BUTTON_ON_PRESS, "KeyboardPress")
	GemRB.SetText(OptionsWindow, ReturnButton, 10308)
	GemRB.SetEvent(OptionsWindow, ReturnButton, IE_GUI_BUTTON_ON_PRESS, "ReturnPress")
	
	GemRB.SetVisible(OptionsWindow, 1)
	
	return
	
def ReturnPress():
	global OptionsWindow
	GemRB.UnloadWindow(OptionsWindow)
	GemRB.SetNextScript("Start")
	return
	
def GraphicsPress():
	global OptionsWindow
	GemRB.UnloadWindow(OptionsWindow)
	GemRB.SetNextScript("Graphics")
	return
	
def SoundPress():
	global OptionsWindow
	GemRB.UnloadWindow(OptionsWindow)
	GemRB.SetNextScript("Sound")
	return
	
def GamePlayPress():
	global OptionsWindow
	GemRB.UnloadWindow(OptionsWindow)
	GemRB.SetNextScript("GamePlay")
	return

def MoviePress():
	global OptionsWindow
	GemRB.UnloadWindow(OptionsWindow)
	GemRB.SetNextScript("Movies")
	return
