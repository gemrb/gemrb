#Ingame Options Menu
import GemRB
from GUIDefines import *
OptionsWindow = None
VideoOptionsWindow = None
AudioOptionsWindow = None
GameplayOptionsWindow = None
FeedbackOptionsWindow = None
AutopauseOptionsWindow = None
LoadMsgWindow = None
SaveMsgWindow = None
QuitMsgWindow = None
MoviesWindow = None
KeysWindow = None

def OpenOptionsWindow ():
	global OptionsWindow

	GemRB.HideGUI()

	if OptionsWindow:
		if VideoOptionsWindow: OpenVideoOptionsWindow ()
		if AudioOptionsWindow: OpenAudioOptionsWindow ()
		if GameplayOptionsWindow: OpenGameplayOptionsWindow ()
		if FeedbackOptionsWindow: OpenFeedbackOptionsWindow ()
		if AutopauseOptionsWindow: OpenAutopauseOptionsWindow ()
		if LoadMsgWindow: OpenLoadMsgWindow ()
		if SaveMsgWindow: OpenSaveMsgWindow ()
		if QuitMsgWindow: OpenQuitMsgWindow ()
		if KeysWindow: OpenKeysWindow ()
		if MoviesWindow: OpenMoviesWindow ()

		GemRB.UnloadWindow (OptionsWindow)
		OptionsWindow = None
		GemRB.SetVar ("OtherWindow", -1)

		GemRB.UnhideGUI ()
		return

	GemRB.LoadWindowPack("GUIOPT")
	OptionsWindow = GemRB.LoadWindow(2)
	GemRB.SetVar ("OtherWindow", OptionsWindow)
	
	VersionLabel = GemRB.GetControl(OptionsWindow, 0x1000000B)
	GemRB.SetText(OptionsWindow, VersionLabel, GEMRB_VERSION)
	
	LoadButton = GemRB.GetControl(OptionsWindow, 5)
	SaveButton = GemRB.GetControl(OptionsWindow, 6)
	QuitButton = GemRB.GetControl(OptionsWindow, 10)
	GraphicsButton = GemRB.GetControl(OptionsWindow, 7)
	SoundButton = GemRB.GetControl(OptionsWindow, 8)
	GamePlayButton = GemRB.GetControl(OptionsWindow, 9)
	MoviesButton = GemRB.GetControl(OptionsWindow, 14)
	KeyboardButton = GemRB.GetControl(OptionsWindow, 13)
	ReturnButton = GemRB.GetControl(OptionsWindow, 11)
	
	GemRB.SetText(OptionsWindow, LoadButton, 13729)
	GemRB.SetEvent(OptionsWindow, LoadButton, IE_GUI_BUTTON_ON_PRESS, "OpenLoadMsgWindow")
	GemRB.SetText(OptionsWindow, SaveButton, 13730)
	GemRB.SetEvent(OptionsWindow, SaveButton, IE_GUI_BUTTON_ON_PRESS, "SaveGamePress")
	GemRB.SetText(OptionsWindow, QuitButton, 13731)
	GemRB.SetEvent(OptionsWindow, QuitButton, IE_GUI_BUTTON_ON_PRESS, "OpenQuitMsgWindow")
	GemRB.SetText(OptionsWindow, GraphicsButton, 17162)
	GemRB.SetEvent(OptionsWindow, GraphicsButton, IE_GUI_BUTTON_ON_PRESS, "GraphicsPress")
	GemRB.SetText(OptionsWindow, SoundButton, 17164)
	GemRB.SetEvent(OptionsWindow, SoundButton, IE_GUI_BUTTON_ON_PRESS, "SoundPress")
	GemRB.SetText(OptionsWindow, GamePlayButton, 17165)
	GemRB.SetEvent(OptionsWindow, GamePlayButton, IE_GUI_BUTTON_ON_PRESS, "GamePlayPress")
	GemRB.SetText(OptionsWindow, MoviesButton, 15415)
	GemRB.SetEvent(OptionsWindow, MoviesButton, IE_GUI_BUTTON_ON_PRESS, "MoviePress")
	GemRB.SetText(OptionsWindow, KeyboardButton, 33468)

	GemRB.SetText(OptionsWindow, ReturnButton, 10308)
	GemRB.SetEvent(OptionsWindow, ReturnButton, IE_GUI_BUTTON_ON_PRESS, "OpenOptionsWindow")
	
	GemRB.UnhideGUI ()
	return

def LoadGamePress():
	OpenOptionsWindow ()
	GemRB.QuitGame ()
	GemRB.SetNextScript ("GUILOAD")
	return

def SaveGamePress():
	OpenOptionsWindow ()
	GemRB.QuitGame ()
	GemRB.SetNextScript ('GUISAVE')
	return

def QuitGamePress():
	OpenOptionsWindow ()
	GemRB.QuitGame ()
	GemRB.SetNextScript ("Start")
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

def OpenLoadMsgWindow ():
	global LoadMsgWindow

	GemRB.HideGUI()

	if LoadMsgWindow:
		GemRB.UnloadWindow (LoadMsgWindow)
		LoadMsgWindow = None
		GemRB.SetVar ("FloatWindow", -1)

		GemRB.UnhideGUI ()
		return

	LoadMsgWindow = Window = GemRB.LoadWindow (4)
	GemRB.SetVar ("FloatWindow", LoadMsgWindow)

	# Load
	Button = GemRB.GetControl (Window, 0)
	GemRB.SetText (Window, Button, 15590)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "LoadGamePress")

	# Cancel
	Button = GemRB.GetControl (Window, 1)
	GemRB.SetText (Window, Button, 13727)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenLoadMsgWindow")

	# Loading a game will destroy ...
	Text = GemRB.GetControl (Window, 3)
	GemRB.SetText (Window, Text, 19531)

	GemRB.UnhideGUI ()
	GemRB.ShowModal (Window, MODAL_SHADOW_GRAY)
	return

def OpenQuitMsgWindow ():
	global QuitMsgWindow

	GemRB.HideGUI()

	if QuitMsgWindow:
		GemRB.UnloadWindow (QuitMsgWindow)
		QuitMsgWindow = None
		GemRB.SetVar ("FloatWindow", -1)
		GemRB.SetVisible (GemRB.GetVar ("OtherWindow"), 1)

		GemRB.UnhideGUI ()
		return

	QuitMsgWindow = Window = GemRB.LoadWindow (5)
	GemRB.SetVar ("FloatWindow", QuitMsgWindow)
	GemRB.SetVisible (GemRB.GetVar ("OtherWindow"), 0)

	# Save
	Button = GemRB.GetControl (Window, 0)
	GemRB.SetText (Window, Button, 15589)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "SaveGamePress")

	# Quit Game
	Button = GemRB.GetControl (Window, 1)
	GemRB.SetText (Window, Button, 15417)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "QuitGamePress")

	# Cancel
	Button = GemRB.GetControl (Window, 2)
	GemRB.SetText (Window, Button, 13727)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenQuitMsgWindow")

	# The game has not been saved ....
	Text = GemRB.GetControl (Window, 3)
	GemRB.SetText (Window, Text, 16456)

	GemRB.UnhideGUI ()
	GemRB.ShowModal (Window, MODAL_SHADOW_GRAY)
	return
