#Options Menu
import GemRB

OptionsWindow = 0

def OnLoad():
	global OptionsWindow
	GemRB.LoadWindowPack("GUIOPT")
	
	MessageBarWindow = GemRB.LoadWindow(0)
	GemRB.SetVisible(MessageBarWindow, 1) #This will startup the window as grayed
	
	CharactersBarWindow = GemRB.LoadWindow(1)
	GemRB.SetVisible(CharactersBarWindow, 1)
	
	GemRB.DrawWindows()
	
	GemRB.SetVisible(MessageBarWindow, 0)
	GemRB.SetVisible(CharactersBarWindow, 0)
	
	OptionsWindow = GemRB.LoadWindow(2)
	#Hiding unused buttons
	Button = GemRB.GetControl(OptionsWindow, 9)
	GemRB.SetButtonFlags(OptionsWindow, Button, IE_GUI_BUTTON_NO_IMAGE, OP_OR)
	GemRB.SetButtonState(OptionsWindow, Button, IE_GUI_BUTTON_DISABLED)
	
	Button = GemRB.GetControl(OptionsWindow, 14)
	GemRB.SetButtonFlags(OptionsWindow, Button, IE_GUI_BUTTON_NO_IMAGE, OP_OR)
	GemRB.SetButtonState(OptionsWindow, Button, IE_GUI_BUTTON_DISABLED)
	
	Button = GemRB.GetControl(OptionsWindow, 13)
	GemRB.SetButtonFlags(OptionsWindow, Button, IE_GUI_BUTTON_NO_IMAGE, OP_OR)
	GemRB.SetButtonState(OptionsWindow, Button, IE_GUI_BUTTON_DISABLED)
	
	GemRB.SetVisible(OptionsWindow, 1)
	
	return
	
	