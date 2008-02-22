#character generation (GUICG 0)
import GemRB
from ie_stats import *
from GUICommon import SetColorStat


CharGenWindow = 0
TextAreaControl = 0
BioWindow = 0
EditControl = 0
AlignmentTable = 0
PortraitName = ""

def OnLoad():
	global CharGenWindow, TextAreaControl, AlignmentTable, PortraitName

	GemRB.LoadWindowPack("GUICG", 800, 600)
	CharGenWindow = GemRB.LoadWindow (0)
	GemRB.SetWindowFrame ( CharGenWindow)
	PortraitButton = GemRB.GetControl (CharGenWindow, 12)
	GemRB.SetButtonFlags (CharGenWindow, PortraitButton, IE_GUI_BUTTON_PICTURE|IE_GUI_BUTTON_NO_IMAGE,OP_SET)
	PortraitName = GemRB.GetToken ("LargePortrait")
	GemRB.SetButtonPicture(CharGenWindow, PortraitButton, PortraitName,"NOPORTLG")

	RaceTable = GemRB.LoadTable ("races")
	ClassTable = GemRB.LoadTable ("classes")
	KitTable = GemRB.LoadTable ("kitlist")
	AlignmentTable = GemRB.LoadTable ("aligns")
	AbilityTable = GemRB.LoadTable ("ability")

	GenderButton = GemRB.GetControl (CharGenWindow,0)
	GemRB.SetText (CharGenWindow,GenderButton,11956)
	GemRB.SetButtonState (CharGenWindow,GenderButton,IE_GUI_BUTTON_DISABLED)

	RaceButton = GemRB.GetControl (CharGenWindow,1)
	GemRB.SetText (CharGenWindow,RaceButton, 11957)
	GemRB.SetButtonState (CharGenWindow,RaceButton,IE_GUI_BUTTON_DISABLED)

	ClassButton = GemRB.GetControl (CharGenWindow,2)
	GemRB.SetText (CharGenWindow,ClassButton, 11959)
	GemRB.SetButtonState (CharGenWindow,ClassButton,IE_GUI_BUTTON_DISABLED)

	AlignmentButton = GemRB.GetControl (CharGenWindow,3)
	GemRB.SetText (CharGenWindow,AlignmentButton, 11958)
	GemRB.SetButtonState (CharGenWindow,AlignmentButton,IE_GUI_BUTTON_DISABLED)

	AbilitiesButton = GemRB.GetControl (CharGenWindow,4)
	GemRB.SetText (CharGenWindow,AbilitiesButton, 11960)
	GemRB.SetButtonState (CharGenWindow,AbilitiesButton,IE_GUI_BUTTON_DISABLED)

	SkillButton = GemRB.GetControl (CharGenWindow,5)
	GemRB.SetText (CharGenWindow,SkillButton, 17372)
	GemRB.SetButtonState (CharGenWindow,SkillButton,IE_GUI_BUTTON_DISABLED)

	AppearanceButton = GemRB.GetControl (CharGenWindow,6)
	GemRB.SetText (CharGenWindow,AppearanceButton, 11961)
	GemRB.SetButtonState (CharGenWindow,AppearanceButton,IE_GUI_BUTTON_DISABLED)

	NameButton = GemRB.GetControl (CharGenWindow,7)
	GemRB.SetText (CharGenWindow,NameButton, 11963)
	GemRB.SetButtonState (CharGenWindow,NameButton,IE_GUI_BUTTON_DISABLED)

	BackButton = GemRB.GetControl (CharGenWindow, 11)
	GemRB.SetText (CharGenWindow, BackButton, 15416)
	GemRB.SetButtonState (CharGenWindow,BackButton,IE_GUI_BUTTON_ENABLED)

	AcceptButton = GemRB.GetControl (CharGenWindow, 8)
	GemRB.SetText (CharGenWindow, AcceptButton, 11962)
	GemRB.SetButtonState (CharGenWindow,AcceptButton,IE_GUI_BUTTON_ENABLED)
	GemRB.SetButtonFlags (CharGenWindow,AcceptButton, IE_GUI_BUTTON_DEFAULT,OP_OR)

	ImportButton = GemRB.GetControl (CharGenWindow, 13)
	GemRB.SetText (CharGenWindow, ImportButton, 13955)
	GemRB.SetButtonState (CharGenWindow,ImportButton,IE_GUI_BUTTON_DISABLED)

	CancelButton = GemRB.GetControl (CharGenWindow, 15)
	GemRB.SetText (CharGenWindow, CancelButton, 36788)
	GemRB.SetButtonState (CharGenWindow,CancelButton,IE_GUI_BUTTON_ENABLED)

	BiographyButton = GemRB.GetControl (CharGenWindow, 16)
	GemRB.SetText (CharGenWindow, BiographyButton, 18003)
	GemRB.SetButtonState (CharGenWindow,BiographyButton,IE_GUI_BUTTON_ENABLED)
	TextAreaControl = GemRB.GetControl (CharGenWindow,9)
	
	GemRB.SetText (CharGenWindow, TextAreaControl, 1047)
	GemRB.TextAreaAppend (CharGenWindow, TextAreaControl, ": ")
	GemRB.TextAreaAppend (CharGenWindow, TextAreaControl, GemRB.GetToken ("CHARNAME") )

	GemRB.TextAreaAppend (CharGenWindow, TextAreaControl, 12135, -1)
	GemRB.TextAreaAppend (CharGenWindow, TextAreaControl,": ")
	if GemRB.GetVar ("Gender") == 1:
		GemRB.TextAreaAppend (CharGenWindow, TextAreaControl, 1050)
	else:
		GemRB.TextAreaAppend (CharGenWindow, TextAreaControl, 1051)
	GemRB.TextAreaAppend (CharGenWindow, TextAreaControl,1048,-1) # new line
	GemRB.TextAreaAppend (CharGenWindow, TextAreaControl,": ")
	row = GemRB.FindTableValue (RaceTable, 3, GemRB.GetVar ("Race") )
	GemRB.TextAreaAppend (CharGenWindow, TextAreaControl,GemRB.GetTableValue (RaceTable,row,2))
	GemRB.TextAreaAppend (CharGenWindow, TextAreaControl,12136, -1)
	GemRB.TextAreaAppend (CharGenWindow, TextAreaControl,": ")
	KitIndex = GemRB.GetVar ("Class Kit")
	if KitIndex == 0:
		GemRB.TextAreaAppend (CharGenWindow, TextAreaControl,GemRB.GetTableValue (ClassTable,GemRB.GetVar ("Class")-1,0))
	else:
		GemRB.TextAreaAppend (CharGenWindow, TextAreaControl, GemRB.GetTableValue (KitTable, KitIndex,2) )
	GemRB.TextAreaAppend (CharGenWindow, TextAreaControl,1049, -1)
	GemRB.TextAreaAppend (CharGenWindow, TextAreaControl,": ")
	GemRB.TextAreaAppend(CharGenWindow, TextAreaControl,GemRB.GetTableValue (AlignmentTable,GemRB.GetVar("Alignment")-1,0))
	GemRB.TextAreaAppend (CharGenWindow, TextAreaControl,"\n[color=FFFF00]",-1) #2 new lines
	GemRB.TextAreaAppend (CharGenWindow, TextAreaControl,17088)
	GemRB.TextAreaAppend (CharGenWindow, TextAreaControl,"[/color]")
	for i in range(0,6):
		v = GemRB.GetTableValue (AbilityTable, i,2)
		GemRB.TextAreaAppend (CharGenWindow, TextAreaControl, v, -1)
		GemRB.TextAreaAppend (CharGenWindow, TextAreaControl,": "+str(GemRB.GetVar ("Ability "+str(i))))

	#skills
	GemRB.TextAreaAppend (CharGenWindow, TextAreaControl,"\n[color=FFFF00]",-1) #2 new lines
	GemRB.TextAreaAppend (CharGenWindow, TextAreaControl,11983)
	GemRB.TextAreaAppend (CharGenWindow, TextAreaControl,"[/color]")
	SkillTable = GemRB.LoadTable ("skillsta")
	SkillName = GemRB.LoadTable ("skills")
	rows = GemRB.GetTableRowCount (SkillTable)
	for i in range(rows):
		stat = GemRB.GetTableValue (SkillTable, i, 0, 2)
		value = GemRB.GetVar("Skill "+str(i) )

		if value:
			skill = GemRB.GetTableValue (SkillName, i, 1)
			GemRB.TextAreaAppend (CharGenWindow, TextAreaControl, skill, -1)
			GemRB.TextAreaAppend (CharGenWindow, TextAreaControl, ": "+str(value) )
	GemRB.UnloadTable (SkillTable)
	GemRB.UnloadTable (SkillName)
 
	#feats
	GemRB.TextAreaAppend(CharGenWindow, TextAreaControl,"\n[color=FFFF00]",-1) #2 new lines
	GemRB.TextAreaAppend(CharGenWindow, TextAreaControl,36310)
	GemRB.TextAreaAppend(CharGenWindow, TextAreaControl,"[/color]")
	FeatTable = GemRB.LoadTable ("featreq")
	FeatName = GemRB.LoadTable ("feats")
	rows = GemRB.GetTableRowCount (FeatTable)
	for i in range(rows):
		value = GemRB.GetVar("Feat "+str(i) )
		if value:
			feat = GemRB.GetTableValue (FeatName, i, 1)
			GemRB.TextAreaAppend (CharGenWindow, TextAreaControl, feat, -1)
			stat = GemRB.GetTableValue (FeatTable, i, 9, 2) 
			if stat:
				GemRB.TextAreaAppend (CharGenWindow, TextAreaControl, ": "+str(value) )

	GemRB.UnloadTable (FeatTable)
	GemRB.UnloadTable (FeatName)

	GemRB.SetEvent (CharGenWindow, CancelButton, IE_GUI_BUTTON_ON_PRESS, "CancelPress")
	GemRB.SetEvent (CharGenWindow, BackButton, IE_GUI_BUTTON_ON_PRESS, "BackPress")
	GemRB.SetEvent (CharGenWindow, AcceptButton, IE_GUI_BUTTON_ON_PRESS, "NextPress")
	GemRB.SetEvent (CharGenWindow, BiographyButton, IE_GUI_BUTTON_ON_PRESS, "BioPress")
	GemRB.SetVisible (CharGenWindow, 1)
	return
	
def SetRaceAbilities(MyChar, racetitle):
	ability = GemRB.LoadTable ("racespab")
	resource = GemRB.GetTableValue (ability, racetitle, "SPECIAL_ABILITIES_FILE")
	GemRB.UnloadTable (ability)
	if resource=="*":
		return

	ability = GemRB.LoadTable (resource)
	rows = GemRB.GetTableRowCount (ability)
	for i in range(rows):
		resource = GemRB.GetTableValue (ability, i, 0)
		count = GemRB.GetTableValue (ability, i,1)
		for j in range(count):
			GemRB.LearnSpell (MyChar, resource)
	GemRB.UnloadTable (ability)
	return

def SetRaceResistances(MyChar, racetitle):
	resistances = GemRB.LoadTable ("racersmd")
	GemRB.SetPlayerStat (MyChar, IE_RESISTFIRE, GemRB.GetTableValue ( resistances, racetitle, "FIRE") )
	GemRB.SetPlayerStat (MyChar, IE_RESISTCOLD, GemRB.GetTableValue ( resistances, racetitle, "COLD") )
	GemRB.SetPlayerStat (MyChar, IE_RESISTELECTRICITY, GemRB.GetTableValue ( resistances, racetitle, "ELEC") )
	GemRB.SetPlayerStat (MyChar, IE_RESISTACID, GemRB.GetTableValue ( resistances, racetitle, "ACID") )
	GemRB.SetPlayerStat (MyChar, IE_RESISTMAGIC, GemRB.GetTableValue ( resistances, racetitle, "SPELL") )
	GemRB.SetPlayerStat (MyChar, IE_RESISTMAGICFIRE, GemRB.GetTableValue ( resistances, racetitle, "MAGIC_FIRE") )
	GemRB.SetPlayerStat (MyChar, IE_RESISTMAGICCOLD, GemRB.GetTableValue ( resistances, racetitle, "MAGIC_COLD") )
	GemRB.SetPlayerStat (MyChar, IE_RESISTSLASHING, GemRB.GetTableValue ( resistances, racetitle, "SLASHING") )
	GemRB.SetPlayerStat (MyChar, IE_RESISTCRUSHING, GemRB.GetTableValue ( resistances, racetitle, "BLUDGEONING") )
	GemRB.SetPlayerStat (MyChar, IE_RESISTPIERCING, GemRB.GetTableValue ( resistances, racetitle, "PIERCING") )
	GemRB.SetPlayerStat (MyChar, IE_RESISTMISSILE, GemRB.GetTableValue ( resistances, racetitle, "MISSILE") )
	GemRB.UnloadTable (resistances)
	return

def ClearPress():
	global BioData

	GemRB.SetToken("BIO", "")
	GemRB.SetText (BioWindow, EditControl, GemRB.GetToken("BIO") )
	return

def RevertPress():
	BioTable = GemRB.LoadTable ("bios")
	Class = GemRB.GetVar ("BaseClass")
	StrRef = GemRB.GetTableValue(BioTable,Class,1)
	GemRB.SetToken ("BIO", GemRB.GetString(StrRef) )
	GemRB.UnloadTable (BioTable)
	GemRB.SetText (BioWindow, EditControl, GemRB.GetToken("BIO") )
	return

def BioCancelPress():
	GemRB.SetToken("BIO",BioData)
	GemRB.UnloadWindow (BioWindow)
	return

def BioDonePress():
	GemRB.UnloadWindow (BioWindow)
	return

def BioPress():
	global BioWindow, EditControl, BioData

	BioData = GemRB.GetToken("BIO")
	BioWindow = Window = GemRB.LoadWindow (51)
	Button = GemRB.GetControl (Window, 5)
	GemRB.SetText (Window, Button, 2240)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "RevertPress")

	Button = GemRB.GetControl (Window, 6)
	GemRB.SetText (Window, Button, 18622)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "ClearPress")

	Button = GemRB.GetControl (Window, 1)
	GemRB.SetText (Window, Button, 11962)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "BioDonePress")
	
	Button = GemRB.GetControl (Window, 2)
	GemRB.SetText (Window, Button, 36788)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "BioCancelPress")
	
	EditControl = GemRB.GetControl (Window, 4)
	BioData = GemRB.GetToken("BIO")
	if BioData == "":
		RevertPress()
	else:
		GemRB.SetText (Window, EditControl, BioData )
	GemRB.ShowModal (Window, MODAL_SHADOW_GRAY)
	return

def NextPress():

	GemRB.UnloadWindow (CharGenWindow)
	#set my character up
	MyChar = GemRB.GetVar ("Slot") 
	GemRB.CreatePlayer ("charbase", MyChar | 0x8000 )
	GemRB.SetPlayerStat (MyChar, IE_SEX, GemRB.GetVar ("Gender") )
	GemRB.SetPlayerStat (MyChar, IE_RACE, GemRB.GetVar ("BaseRace") )
	race = GemRB.GetVar ("Race")
	GemRB.SetPlayerStat (MyChar, IE_SUBRACE, race & 255 )
	TmpTable = GemRB.LoadTable ("races")
	row = GemRB.FindTableValue (TmpTable, 3, race )
	racename = GemRB.GetTableRowName (TmpTable, row)
	if row!=-1:
		SetRaceResistances( MyChar, racename )
		SetRaceAbilities( MyChar, racename )
	GemRB.UnloadTable (TmpTable)

	#base class
	Class=GemRB.GetVar ("BaseClass")
	GemRB.SetPlayerStat (MyChar, IE_CLASS, Class)
	#kit
	GemRB.SetPlayerStat (MyChar, IE_KIT, GemRB.GetVar ("Class") )
	AlignmentTable = GemRB.LoadTable ("aligns")
	t=GemRB.GetVar ("Alignment")
	GemRB.SetPlayerStat (MyChar, IE_ALIGNMENT, GemRB.GetTableValue (AlignmentTable, t, 3) )
	TmpTable=GemRB.LoadTable ("repstart")
	#t=GemRB.FindTableValue (AlignmentTable,3,t)
	t=GemRB.GetTableValue (TmpTable,t,0)
	GemRB.SetPlayerStat (MyChar, IE_REPUTATION, t)
	GemRB.UnloadTable (TmpTable)
	TmpTable=GemRB.LoadTable ("strtgold")
	a = GemRB.GetTableValue (TmpTable, Class, 1) #number of dice
	b = GemRB.GetTableValue (TmpTable, Class, 0) #size
	c = GemRB.GetTableValue (TmpTable, Class, 2) #adjustment
	d = GemRB.GetTableValue (TmpTable, Class, 3) #external multiplier
	e = GemRB.GetTableValue (TmpTable, Class, 4) #level bonus rate
	t = GemRB.GetPlayerStat (MyChar, IE_LEVEL) 
	if t>1:
		e=e*(t-1)
	else:
		e=0
	GemRB.UnloadTable (AlignmentTable)
	GemRB.UnloadTable (TmpTable)
	t = GemRB.Roll(a,b,c)*d+e
	GemRB.SetPlayerStat (MyChar, IE_GOLD, t)
	GemRB.SetPlayerStat (MyChar, IE_HATEDRACE, GemRB.GetVar ("HatedRace") )
	TmpTable = GemRB.LoadTable ("ability")
	AbilityCount = GemRB.GetTableRowCount (TmpTable)
	for i in range (AbilityCount):
		StatID=GemRB.GetTableValue (TmpTable, i,4)
		GemRB.SetPlayerStat (MyChar, StatID, GemRB.GetVar ("Ability "+str(i) ) )
	GemRB.UnloadTable (TmpTable)

#	TmpTable=GemRB.LoadTable ("weapprof")
#	ProfCount = GemRB.GetTableRowCount (TmpTable)
#	for i in range(ProfCount):
#		StatID=GemRB.GetTableValue (TmpTable, i, 0)
#		GemRB.SetPlayerStat (MyChar, StatID, GemRB.GetVar ("Prof "+str(i) ) )
	SetColorStat (MyChar, IE_HAIR_COLOR, GemRB.GetVar ("Color1") )
	SetColorStat (MyChar, IE_SKIN_COLOR, GemRB.GetVar ("Color2") )
	SetColorStat (MyChar, IE_MAJOR_COLOR, GemRB.GetVar ("Color4") )
	SetColorStat (MyChar, IE_MINOR_COLOR, GemRB.GetVar ("Color3") )
	SetColorStat (MyChar, IE_METAL_COLOR, 0x1B )
	SetColorStat (MyChar, IE_LEATHER_COLOR, 0x16 )
	SetColorStat (MyChar, IE_ARMOR_COLOR, 0x17 )
	GemRB.SetPlayerStat (MyChar, IE_EA, 2 )
	Str=GemRB.GetVar ("Ability 1")
	GemRB.SetPlayerStat (MyChar, IE_STR, Str)
	if Str==18:
		GemRB.SetPlayerStat (MyChar,IE_STREXTRA,GemRB.GetVar ("StrExtra"))
	else:
		GemRB.SetPlayerStat (MyChar, IE_STREXTRA,0)

	GemRB.SetPlayerStat (MyChar, IE_DEX, GemRB.GetVar ("Ability 2"))
	GemRB.SetPlayerStat (MyChar, IE_CON, GemRB.GetVar ("Ability 3"))
	GemRB.SetPlayerStat (MyChar, IE_INT, GemRB.GetVar ("Ability 4"))
	GemRB.SetPlayerStat (MyChar, IE_WIS, GemRB.GetVar ("Ability 5"))
	GemRB.SetPlayerStat (MyChar, IE_CHR, GemRB.GetVar ("Ability 6"))
	GemRB.SetPlayerName (MyChar, GemRB.GetToken ("CHARNAME"), 0)

	#setting skills
	TmpTable = GemRB.LoadTable ("skillsta")
	SkillCount = GemRB.GetTableRowCount (TmpTable)
	for i in range (SkillCount):
		StatID=GemRB.GetTableValue (TmpTable, i, 2)
		GemRB.SetPlayerStat (MyChar, StatID, GemRB.GetVar ("Skill "+str(i) ) )
	GemRB.UnloadTable (TmpTable)

	#setting feats

	#does all the rest
	LargePortrait = GemRB.GetToken ("LargePortrait")
	SmallPortrait = GemRB.GetToken ("SmallPortrait")
	GemRB.FillPlayerInfo(MyChar, LargePortrait, SmallPortrait) 
 	GemRB.SetNextScript ("SPPartyFormation")

	TmpTable = GemRB.LoadTable ("strtxp")

	#starting xp is race dependent
	xp = GemRB.GetTableValue (TmpTable, racename, "VALUE")
	GemRB.SetPlayerStat (MyChar, IE_XP, xp ) 

	return

def CancelPress():
	GemRB.UnloadWindow (CharGenWindow)
	GemRB.SetNextScript ("CharGen")
	BioData = None
	return

def BackPress():
	GemRB.UnloadWindow (CharGenWindow)
	GemRB.SetNextScript ("CharGen8") #name
	BioData = None
	return

