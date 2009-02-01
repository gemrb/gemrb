#ToB start window, precedes the SoA window
import GemRB
from GUICommon import GameIsTOB

StartWindow = 0

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
	if not GameIsTOB():
		GemRB.SetMasterScript("BALDUR","WORLDMAP")
		GemRB.SetVar("oldgame",1)
		GemRB.SetNextScript("Start2")
		return

	GemRB.LoadWindowPack("START", 640, 480)
	StartWindow = GemRB.LoadWindowObject(7)
	StartWindow.SetFrame ()
	StartWindow.CreateLabel(0x0fff0000, 0,0,640,30, "REALMS", "", 1)
	Label=StartWindow.GetControl(0x0fff0000)
	Label.SetText(GEMRB_VERSION)

	TextArea = StartWindow.GetControl(0)
	TextArea.SetText(73245)
	TextArea = StartWindow.GetControl(1)
	TextArea.SetText(73246)
	SoAButton = StartWindow.GetControl(2)
	SoAButton.SetText(73247)
	ToBButton = StartWindow.GetControl(3)
	ToBButton.SetText(73248)
	ExitButton = StartWindow.GetControl(4)
	ExitButton.SetText(13731)
	SoAButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, "SoAPress")
	ToBButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, "ToBPress")
	ExitButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, "ExitPress")
	StartWindow.SetVisible(1)
	GemRB.LoadMusicPL("Cred.mus")
	return
	
def SoAPress():
	if StartWindow:
		StartWindow.Unload()
	GemRB.SetMasterScript("BALDUR","WORLDMAP")
	GemRB.SetVar("oldgame",1)
	GemRB.SetNextScript("Start2")
	if not skip_videos:
		GemRB.PlayMovie ("INTRO15F")
	return

def ToBPress():
	GemRB.SetMasterScript("BALDUR25","WORLDM25")
	GemRB.SetVar("oldgame",0)
	if StartWindow:
		StartWindow.Unload()
	GemRB.SetNextScript("Start2")
	if not skip_videos:
		GemRB.PlayMovie ("INTRO")
	return

def ExitPress():
	GemRB.Quit()
	return
