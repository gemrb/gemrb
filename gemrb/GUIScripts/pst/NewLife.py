import GemRB

NewLifeWindow = 0
TextArea = 0

StrLabel = 0
DexLabel = 0
ConLabel = 0
WisLabel = 0
IntLabel = 0
ChaLabel = 0
TotLabel = 0
AcLabel = 0
HpLabel = 0

Str = 0
Dex = 0
Con = 0
Wis = 0
Int = 0
Cha = 0
TotPoints = 0
AcPoints = 0
HpPoints = 0

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
	global NewLifeWindow, TotPoints, AcPoints, HpPoints
	global Str, Dex, Con, Wis, Int, Cha
	global TotLabel, AcLabel, HpLabel
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
	TotPoints = 21
	
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

	Button = GemRB.GetControl(NewLifeWindow, 8)
	GemRB.SetButtonFlags(NewLifeWindow, Button, IE_GUI_BUTTON_RADIOBUTTON, OP_SET)
	GemRB.SetButtonState(NewLifeWindow, Button, IE_GUI_BUTTON_SELECTED)
	GemRB.SetButtonSprites(NewLifeWindow, Button, "", 0, 0, 0, 0, 0)
	GemRB.SetText(NewLifeWindow, Button, 5025)
	GemRB.SetEvent(NewLifeWindow,Button, IE_GUI_BUTTON_ON_PRESS, "PointPress")

	Button = GemRB.GetControl(NewLifeWindow, 9)
	GemRB.SetButtonFlags(NewLifeWindow, Button, IE_GUI_BUTTON_RADIOBUTTON, OP_SET)
	GemRB.SetButtonState(NewLifeWindow, Button, IE_GUI_BUTTON_SELECTED)
	GemRB.SetButtonSprites(NewLifeWindow, Button, "", 0, 0, 0, 0, 0)
	GemRB.SetText(NewLifeWindow, Button, 5026)
	GemRB.SetEvent(NewLifeWindow,Button, IE_GUI_BUTTON_ON_PRESS, "AcPress")

	Button = GemRB.GetControl(NewLifeWindow, 10)
	GemRB.SetButtonFlags(NewLifeWindow, Button, IE_GUI_BUTTON_RADIOBUTTON, OP_SET)
	GemRB.SetButtonState(NewLifeWindow, Button, IE_GUI_BUTTON_SELECTED)
	GemRB.SetButtonSprites(NewLifeWindow, Button, "", 0, 0, 0, 0, 0)
	GemRB.SetText(NewLifeWindow, Button, 5027)
	GemRB.SetEvent(NewLifeWindow,Button, IE_GUI_BUTTON_ON_PRESS, "HpPress")

	Button = GemRB.GetControl(NewLifeWindow, 11) #str +
	GemRB.SetEvent(NewLifeWindow, Button, IE_GUI_BUTTON_ON_PRESS, "IncreasePress")
	GemRB.SetVarAssoc(NewLifeWindow, Button, "Pressed", 0)
	
	Button = GemRB.GetControl(NewLifeWindow, 13) #int +
	GemRB.SetEvent(NewLifeWindow, Button,IE_GUI_BUTTON_ON_PRESS,  "IncreasePress")
	GemRB.SetVarAssoc(NewLifeWindow, Button, "Pressed", 1)
	
	Button = GemRB.GetControl(NewLifeWindow, 15) #wis +
	GemRB.SetEvent(NewLifeWindow, Button,IE_GUI_BUTTON_ON_PRESS,  "IncreasePress")
	GemRB.SetVarAssoc(NewLifeWindow, Button, "Pressed", 2)
	
	Button = GemRB.GetControl(NewLifeWindow, 17) #dex +
	GemRB.SetEvent(NewLifeWindow, Button,IE_GUI_BUTTON_ON_PRESS,  "IncreasePress")
	GemRB.SetVarAssoc(NewLifeWindow, Button, "Pressed", 3)
	
	Button = GemRB.GetControl(NewLifeWindow, 19) #con +
	GemRB.SetEvent(NewLifeWindow, Button,IE_GUI_BUTTON_ON_PRESS, "IncreasePress")
	GemRB.SetVarAssoc(NewLifeWindow, Button, "Pressed", 4)
	
	Button = GemRB.GetControl(NewLifeWindow, 21) #chr +
	GemRB.SetEvent(NewLifeWindow, Button,IE_GUI_BUTTON_ON_PRESS, "IncreasePress")
	GemRB.SetVarAssoc(NewLifeWindow, Button, "Pressed", 5)
	
	Button = GemRB.GetControl(NewLifeWindow, 12) #str +
	GemRB.SetEvent(NewLifeWindow, Button,IE_GUI_BUTTON_ON_PRESS, "DecreasePress")
	GemRB.SetVarAssoc(NewLifeWindow, Button, "Pressed", 0)
	
	Button = GemRB.GetControl(NewLifeWindow, 14) #int +
	GemRB.SetEvent(NewLifeWindow, Button,IE_GUI_BUTTON_ON_PRESS, "DecreasePress")
	GemRB.SetVarAssoc(NewLifeWindow, Button, "Pressed", 1)
	
	Button = GemRB.GetControl(NewLifeWindow, 16) #wis +
	GemRB.SetEvent(NewLifeWindow, Button,IE_GUI_BUTTON_ON_PRESS, "DecreasePress")
	GemRB.SetVarAssoc(NewLifeWindow, Button, "Pressed", 2)
	
	Button = GemRB.GetControl(NewLifeWindow, 18) #dex +
	GemRB.SetEvent(NewLifeWindow, Button,IE_GUI_BUTTON_ON_PRESS, "DecreasePress")
	GemRB.SetVarAssoc(NewLifeWindow, Button, "Pressed", 3)
	
	Button = GemRB.GetControl(NewLifeWindow,  20) #con +
	GemRB.SetEvent(NewLifeWindow, Button,IE_GUI_BUTTON_ON_PRESS, "DecreasePress")
	GemRB.SetVarAssoc(NewLifeWindow, Button, "Pressed", 4)
	
	Button = GemRB.GetControl(NewLifeWindow,  22) #chr +
	GemRB.SetEvent(NewLifeWindow, Button,IE_GUI_BUTTON_ON_PRESS, "DecreasePress")
	GemRB.SetVarAssoc(NewLifeWindow, Button, "Pressed", 5)
	
	NewLifeLabel = GemRB.GetControl(NewLifeWindow, 0x10000023)
	GemRB.SetText(NewLifeWindow, NewLifeLabel, 1899)
	
	TextArea = GemRB.GetControl(NewLifeWindow, 23)
	GemRB.SetText(NewLifeWindow, TextArea, 18495)
	
	TotLabel = GemRB.GetControl(NewLifeWindow, 0x10000020)
	AcLabel = GemRB.GetControl(NewLifeWindow, 0x1000001E)
	HpLabel = GemRB.GetControl(NewLifeWindow, 0x1000001F)

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
	GemRB.SetEvent(NewLifeWindow, AcceptButton, IE_GUI_BUTTON_ON_PRESS, "CancelPress")
	
	UpdateLabels()
	
	GemRB.SetVisible(NewLifeWindow, 1)
	return
	
def UpdateLabels():
	global AcPoints, HpPoints

	GemRB.SetText(NewLifeWindow, StrLabel, str(Str))
	GemRB.SetText(NewLifeWindow, DexLabel, str(Dex))
	GemRB.SetText(NewLifeWindow, ConLabel, str(Con))
	GemRB.SetText(NewLifeWindow, WisLabel, str(Wis))
	GemRB.SetText(NewLifeWindow, IntLabel, str(Int))
	GemRB.SetText(NewLifeWindow, ChaLabel, str(Cha))
	GemRB.SetText(NewLifeWindow, TotLabel, str(TotPoints))
	AcPoints = 10
	if Dex>14:
		AcPoints = AcPoints + (Dex-14)

	HpPoints = 20
	if Con>14:
		HpPoints = HpPoints + (Con-14)

	GemRB.SetText(NewLifeWindow, AcLabel, str(AcPoints))
	GemRB.SetText(NewLifeWindow, HpLabel, str(HpPoints))
	return
	
def AcceptPress():
	GemRB.UnloadWindow(NewLifeWindow)
	#set my character up
	MyChar = GemRB.CreatePlayer("charbase", 0 ) 
	GemRB.FillPlayerInfo(MyChar) #does all the rest
	#LETS PLAY!!
	GemRB.EnterGame()
	return

def CancelPress():
	GemRB.UnloadWindow(NewLifeWindow)
	GemRB.SetNextScript("Start")
	return

def PointPress():
	GemRB.SetText(NewLifeWindow, TextArea, 18492)
	return

def AcPress():
	GemRB.SetText(NewLifeWindow, TextArea, 18493)
	return

def HpPress():
	GemRB.SetText(NewLifeWindow, TextArea, 18494)
	return

def DecreasePress():
	global TotPoints
	global Str, Int, Wis, Dex, Con, Chr

	Pressed = GemRB.GetVar("Pressed")
	if Pressed == 0:
		Sum = Str
	if Pressed == 1:
		Sum = Int
	if Pressed == 2:
		Sum = Wis 
	if Pressed == 3:
		Sum = Dex
	if Pressed == 4:
		Sum = Con
	if Pressed == 5:
		Sum = Chr
	if Sum<=9:
		return
	TotPoints = TotPoints+1
	Sum = Sum-1
	if Pressed == 0:
		Str = Sum
	if Pressed == 1:
		Int = Sum
	if Pressed == 2:
		Wis = Sum
	if Pressed == 3:
		Dex = Sum
	if Pressed == 4:
		Con = Sum
	if Pressed == 5:
		Chr = Sum
	UpdateLabels()
	return

def IncreasePress():
	global TotPoints
	global Str, Int, Wis, Dex, Con, Chr

	if TotPoints<=0:
		return
	Pressed = GemRB.GetVar("Pressed")
	if Pressed == 0:
		Sum = Str
	if Pressed == 1:
		Sum = Int
	if Pressed == 2:
		Sum = Wis 
	if Pressed == 3:
		Sum = Dex
	if Pressed == 4:
		Sum = Con
	if Pressed == 5:
		Sum = Chr
	if Sum>=18:
		return
	TotPoints = TotPoints-1
	Sum = Sum+1
	if Pressed == 0:
		Str = Sum
	if Pressed == 1:
		Int = Sum
	if Pressed == 2:
		Wis = Sum
	if Pressed == 3:
		Dex = Sum
	if Pressed == 4:
		Con = Sum
	if Pressed == 5:
		Chr = Sum
	UpdateLabels()
	return

