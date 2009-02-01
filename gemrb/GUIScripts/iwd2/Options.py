#Options Menu
import GemRB

OptionsWindow = 0

def OnLoad():
	global OptionsWindow
	GemRB.LoadWindowPack("GUIOPT", 800, 600)
	
	MessageBarWindow = GemRB.LoadWindowObject(0)
	MessageBarWindow.SetVisible(1) #This will startup the window as grayed
	
	CharactersBarWindow = GemRB.LoadWindowObject(1)
	CharactersBarWindow.SetVisible(1)
	
	GemRB.DrawWindows()
	
	MessageBarWindow.SetVisible(0)
	CharactersBarWindow.SetVisible(0)
	
	if MessageBarWindow:
		MessageBarWindow.Unload()
	if CharactersBarWindow:
		CharactersBarWindow.Unload()
	
	OptionsWindow = GemRB.LoadWindowObject(13)
	OptionsWindow.SetFrame ()
	
	VersionLabel = OptionsWindow.GetControl(0x1000000B)
	VersionLabel.SetText(GEMRB_VERSION)
	
	GraphicsButton = OptionsWindow.GetControl(7)
	SoundButton = OptionsWindow.GetControl(8)
	GamePlayButton = OptionsWindow.GetControl(9)
	MoviesButton = OptionsWindow.GetControl(14)
	KeyboardButton = OptionsWindow.GetControl(13)
	ReturnButton = OptionsWindow.GetControl(11)
	
	GraphicsButton.SetText(17162)
	GraphicsButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, "GraphicsPress")
	SoundButton.SetText(17164)
	SoundButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, "SoundPress")
	GamePlayButton.SetText(17165)
	GamePlayButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, "GamePlayPress")
	MoviesButton.SetText(15415)
	MoviesButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, "MoviePress")
	KeyboardButton.SetText(33468)
	MoviesButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, "KeyboardPress")
	ReturnButton.SetText(10308)
	ReturnButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, "ReturnPress")
	
	OptionsWindow.SetVisible(1)
	
	return
	
def ReturnPress():
	global OptionsWindow
	if OptionsWindow:
		OptionsWindow.Unload()
	GemRB.SetNextScript("Start")
	return
	
def GraphicsPress():
	global OptionsWindow
	if OptionsWindow:
		OptionsWindow.Unload()
	GemRB.SetNextScript("Graphics")
	return
	
def SoundPress():
	global OptionsWindow
	if OptionsWindow:
		OptionsWindow.Unload()
	GemRB.SetNextScript("Sound")
	return
	
def GamePlayPress():
	global OptionsWindow
	if OptionsWindow:
		OptionsWindow.Unload()
	GemRB.SetNextScript("GamePlay")
	return

def MoviePress():
	global OptionsWindow
	if OptionsWindow:
		OptionsWindow.Unload()
	GemRB.SetNextScript("Movies")
	return
