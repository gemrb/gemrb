#ToB start window, precedes the SoA window
import GemRB

StartWindow = 0

def OnLoad():
	global StartWindow

	GemRB.LoadWindowPack("START")
	StartWindow = GemRB.LoadWindow(7)
	GemRB.CreateLabel(StartWindow, 0x0fff0000, 0,0,640,30, "REALMS", "", 1);
	Label=GemRB.GetControl(StartWindow, 0x0fff0000)
	GemRB.SetText(StartWindow, Label,GEMRB_VERSION)

	TextArea = GemRB.GetControl(StartWindow, 0)
	GemRB.SetText(StartWindow, TextArea, 73245)
	TextArea = GemRB.GetControl(StartWindow, 1)
	GemRB.SetText(StartWindow, TextArea, 73246)
	SoAButton = GemRB.GetControl(StartWindow, 2)
	GemRB.SetText(StartWindow, SoAButton, 73247)
	ToBButton = GemRB.GetControl(StartWindow, 3)
	GemRB.SetText(StartWindow, ToBButton, 73248)
	ExitButton = GemRB.GetControl(StartWindow, 4)
	GemRB.SetText(StartWindow, ExitButton, 13731)
	GemRB.SetEvent(StartWindow, SoAButton, IE_GUI_BUTTON_ON_PRESS, "SoAPress")
	GemRB.SetEvent(StartWindow, ToBButton, IE_GUI_BUTTON_ON_PRESS, "ToBPress")
	GemRB.SetEvent(StartWindow, ExitButton, IE_GUI_BUTTON_ON_PRESS, "ExitPress")
	GemRB.SetVisible(StartWindow, 1)
	GemRB.LoadMusicPL("Cred.mus")
	return
	
def SoAPress():
	GemRB.SetMasterScript("BALDUR","WORLDMAP")
	GemRB.SetVar("oldgame",1)
	GemRB.UnloadWindow(StartWindow)
	GemRB.SetNextScript("Start2")
	return

def ToBPress():
	GemRB.SetMasterScript("BALDUR25","WORLDMP25")
	GemRB.SetVar("oldgame",0)
	GemRB.UnloadWindow(StartWindow)
	GemRB.SetNextScript("Start2")
	return

def ExitPress():
	GemRB.Quit()
	return
