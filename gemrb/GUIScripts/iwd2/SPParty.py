#Single Player Party Select
import GemRB

PartySelectWindow = 0
TextArea = 0
PartyCount = 0
ScrollBar = 0

def OnLoad():
	global PartySelectWindow, TextArea, PartyCount, ScrollBar
	GemRB.LoadWindowPack("GUISP")
	
	PartyCount = GemRB.GetINIPartyCount()
	
	PartySelectWindow = GemRB.LoadWindow(10)
	TextArea = GemRB.GetControl(PartySelectWindow, 6)
	ScrollBar = GemRB.GetControl(PartySelectWindow, 8)
	GemRB.SetEvent(PartySelectWindow, ScrollBar, IE_GUI_SCROLLBAR_ON_CHANGE, "ScrollBarPress")
	GemRB.SetVarAssoc(PartySelectWindow, ScrollBar, "TopIndex", PartyCount)
	
	ModifyButton = GemRB.GetControl(PartySelectWindow, 12)
	GemRB.SetText(PartySelectWindow, ModifyButton, 10316)
	CancelButton = GemRB.GetControl(PartySelectWindow, 11)
	GemRB.SetEvent(PartySelectWindow, CancelButton, IE_GUI_BUTTON_ON_PRESS, "CancelPress")
	GemRB.SetText(PartySelectWindow, CancelButton, 13727)
	DoneButton = GemRB.GetControl(PartySelectWindow, 10)
	GemRB.SetText(PartySelectWindow, DoneButton, 11973)
	
	GemRB.SetVar("PartyIdx",0)
	GemRB.SetVar("TopIndex",0)
	
	for i in range(0,6):
		Button = GemRB.GetControl(PartySelectWindow,i)
		GemRB.SetButtonFlags(PartySelectWindow, Button, IE_GUI_BUTTON_RADIOBUTTON, OP_OR)
		GemRB.SetEvent(PartySelectWindow, Button, IE_GUI_BUTTON_ON_PRESS, "PartyButtonPress")
		#GemRB.SetButtonState(PartySelectWindow, Button, IE_GUI_BUTTON_DISABLED)
		GemRB.SetVarAssoc(PartySelectWindow, Button, "PartyIdx",i)
	
	ScrollBarPress()
	PartyButtonPress()
	
	GemRB.SetVisible(PartySelectWindow, 1)
	
	return

def ScrollBarPress():
	global PartySelectWindow, PartyCount
	Pos = GemRB.GetVar("TopIndex")
	for i in range(0,6):
		ActPos = Pos + i
		Button = GemRB.GetControl(PartySelectWindow, i)
		if ActPos<PartyCount:
			GemRB.SetButtonState(PartySelectWindow, Button, IE_GUI_BUTTON_ENABLED)
			Tag = "Party " + str(ActPos)
			PartyDesc = GemRB.GetINIPartyKey(Tag, "Name", "")					
			GemRB.SetText(PartySelectWindow, Button, PartyDesc)
		else:
			GemRB.SetButtonState(PartySelectWindow, Button, IE_GUI_BUTTON_DISABLED)
			GemRB.SetText(PartySelectWindow, Button, "")
	return
	
def CancelPress():
	global PartySelectWindow
	GemRB.UnloadWindow(PartySelectWindow)
	GemRB.SetNextScript("Start")
	return
	
def PartyButtonPress():
	global PartySelectWindow, TextArea
	i = GemRB.GetVar("PartyIdx") + GemRB.GetVar("TopIndex")
	Tag = "Party " + str(i)
	PartyDesc = ""
	for j in range(1, 9):
		Key = "Descr" + str(j)
		NewString = GemRB.GetINIPartyKey(Tag, Key, "")
		if NewString != "":
			NewString = NewString + "\n\n"
			PartyDesc = PartyDesc + NewString
	
	GemRB.SetText(PartySelectWindow, TextArea, PartyDesc)
	return
