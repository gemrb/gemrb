#ToB start window, precedes the SoA window
import GemRB

StartWindow = 0

#the worldmap resource indicator
IE_WMP_CLASS_ID = 0x000003F7

def OnLoad():
	global StartWindow, skip_videos

	GemRB.EnableCheatKeys(1)
	skip_videos = GemRB.GetVar ("SkipIntroVideos")
	if not skip_videos:
		GemRB.PlayMovie ("BISLOGO")
		GemRB.PlayMovie ("BWDRAGON")
		GemRB.PlayMovie ("WOTC")

	# Find proper window border for higher resolutions
	screen_width = GemRB.GetSystemVariable (SV_WIDTH)
	screen_height = GemRB.GetSystemVariable (SV_HEIGHT)
	if screen_width == 800:
		GemRB.LoadWindowFrame("STON08L", "STON08R", "STON08T", "STON08B")
	elif screen_width == 1024:
		GemRB.LoadWindowFrame("STON10L", "STON10R", "STON10T", "STON10B")

	#if not detected tob, we go right to the main menu
	if not GemRB.HasResource("worldm25",IE_WMP_CLASS_ID):
		GemRB.SetMasterScript("BALDUR","WORLDMAP")
		GemRB.SetVar("oldgame",1)
		GemRB.SetNextScript("Start2")
		if not skip_videos:
			GemRB.PlayMovie ("INTRO15F")
		return

	GemRB.LoadWindowPack("START", 640, 480)
	StartWindow = GemRB.LoadWindow(7)
	GemRB.SetWindowFrame (StartWindow)
	GemRB.CreateLabel(StartWindow, 0x0fff0000, 0,0,640,30, "REALMS", "", 1)
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
	GemRB.UnloadWindow(StartWindow)
	GemRB.SetMasterScript("BALDUR","WORLDMAP")
	GemRB.SetVar("oldgame",1)
	GemRB.SetNextScript("Start2")
	if not skip_videos:
		GemRB.PlayMovie ("INTRO15F")
	return

def ToBPress():
	GemRB.SetMasterScript("BALDUR25","WORLDM25")
	GemRB.SetVar("oldgame",0)
	GemRB.UnloadWindow(StartWindow)
	GemRB.SetNextScript("Start2")
	if not skip_videos:
		GemRB.PlayMovie ("INTRO")
	return

def ExitPress():
	GemRB.Quit()
	return
