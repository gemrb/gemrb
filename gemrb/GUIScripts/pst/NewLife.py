import GemRB

NewLifeWindow = 0
TextArea = 0

StrLabel = 0
DexLabel = 0
ConLabel = 0
WisLabel = 0
IntLabel = 0
ChaLabel = 0

TotPoints = 0
Str = 0
Dex = 0
Con = 0
Wis = 0
Int = 0
Cha = 0

IE_SEX =        	35
IE_HATEDRACE =  	49
IE_KIT =        	152
IE_RACE =       	201
IE_CLASS =		202
IE_METAL_COLOR =	208
IE_MINOR_COLOR =	209
IE_MAJOR_COLOR =	210
IE_SKIN_COLOR = 	211
IE_LEATHER_COLOR = 	212
IE_ARMOR_COLOR = 	213
IE_HAIR_COLOR =		214
IE_ALIGNMENT =  	217

def OnLoad():
	global NewLifeWindow, TotPoints, Str, Dex, Con, Wis, Int, Cha
	global StrLabel, DexLabel, ConLabel, WisLabel, IntLabel, ChaLabel
	global TextArea

	GemRB.LoadWindowPack("GUICG")
	NewLifeWindow = GemRB.LoadWindow(0)
	
	Str = 9
	Dex = 9
	Con = 9
	Wis = 9
	Int = 9
	Cha = 9
	
	StrLabel = GemRB.GetControl(NewLifeWindow, 0x10000018)
	DexLabel = GemRB.GetControl(NewLifeWindow, 0x1000001B)
	ConLabel = GemRB.GetControl(NewLifeWindow, 0x1000001C)
	WisLabel = GemRB.GetControl(NewLifeWindow, 0x1000001A)
	IntLabel = GemRB.GetControl(NewLifeWindow, 0x10000019)
	ChaLabel = GemRB.GetControl(NewLifeWindow, 0x1000001D)
	
	Button = GemRB.GetControl(NewLifeWindow, 2)
	GemRB.SetButtonFlags(NewLifeWindow, Button, IE_GUI_BUTTON_NO_IMAGE, OP_SET)
	Button = GemRB.GetControl(NewLifeWindow, 3)
	GemRB.SetButtonFlags(NewLifeWindow, Button, IE_GUI_BUTTON_NO_IMAGE, OP_SET)
	Button = GemRB.GetControl(NewLifeWindow, 4)
	GemRB.SetButtonFlags(NewLifeWindow, Button, IE_GUI_BUTTON_NO_IMAGE, OP_SET)
	Button = GemRB.GetControl(NewLifeWindow, 5)
	GemRB.SetButtonFlags(NewLifeWindow, Button, IE_GUI_BUTTON_NO_IMAGE, OP_SET)
	Button = GemRB.GetControl(NewLifeWindow, 6)
	GemRB.SetButtonFlags(NewLifeWindow, Button, IE_GUI_BUTTON_NO_IMAGE, OP_SET)
	Button = GemRB.GetControl(NewLifeWindow, 7)
	GemRB.SetButtonFlags(NewLifeWindow, Button, IE_GUI_BUTTON_NO_IMAGE, OP_SET)
	
	NewLifeLabel = GemRB.GetControl(NewLifeWindow, 0x10000023)
	GemRB.SetText(NewLifeWindow, NewLifeLabel, 1899)
	
	TextArea = GemRB.GetControl(NewLifeWindow, 23)
	GemRB.SetText(NewLifeWindow, TextArea, 18495)
	
	Label = GemRB.GetControl(NewLifeWindow, 0x10000020)
	GemRB.SetText(NewLifeWindow, Label, 5027)
	Label = GemRB.GetControl(NewLifeWindow, 0x1000001E)
	GemRB.SetText(NewLifeWindow, Label, 5025)
	Label = GemRB.GetControl(NewLifeWindow, 0x1000001F)
	GemRB.SetText(NewLifeWindow, Label, 5026)
	
	Label = GemRB.GetControl(NewLifeWindow, 0x10000021)
	GemRB.SetText(NewLifeWindow, Label, 254)
	
	PhotoButton = GemRB.GetControl(NewLifeWindow, 35)
	GemRB.SetButtonState(NewLifeWindow, PhotoButton, IE_GUI_BUTTON_DISABLED)
	GemRB.SetButtonFlags(NewLifeWindow, PhotoButton, IE_GUI_BUTTON_NO_IMAGE | IE_GUI_BUTTON_PICTURE, OP_SET)
	GemRB.SetButtonPicture(NewLifeWindow, PhotoButton, "STPNOC")
	
	AcceptButton = GemRB.GetControl(NewLifeWindow, 0)
	GemRB.SetText(NewLifeWindow, AcceptButton, 4192)
	GemRB.SetEvent(NewLifeWindow, AcceptButton, IE_GUI_BUTTON_ON_PRESS, "AcceptPress")
	
	CancelButton = GemRB.GetControl(NewLifeWindow, 1)
	GemRB.SetText(NewLifeWindow, CancelButton, 4196)	
	
	UpdateLabels()
	
	GemRB.SetVisible(NewLifeWindow, 1)
	return
	
def UpdateLabels():
	GemRB.SetText(NewLifeWindow, StrLabel, str(Str))
	GemRB.SetText(NewLifeWindow, DexLabel, str(Dex))
	GemRB.SetText(NewLifeWindow, ConLabel, str(Con))
	GemRB.SetText(NewLifeWindow, WisLabel, str(Wis))
	GemRB.SetText(NewLifeWindow, IntLabel, str(Int))
	GemRB.SetText(NewLifeWindow, ChaLabel, str(Cha))
	return
	
def AcceptPress():
	GemRB.UnloadWindow(NewLifeWindow)
	#set my character up
	MyChar = GemRB.CreatePlayer("charbase", 0 ) 
	GemRB.FillPlayerInfo(MyChar) #does all the rest
	#LETS PLAY!!
	GemRB.EnterGame()
	return