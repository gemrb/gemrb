#character generation (GUICG 0)
import GemRB
from ie_stats import *
from GUICommon import *

CharGenWindow = 0
TextAreaControl = 0
PortraitName = ""

def OnLoad():
	global CharGenWindow, TextAreaControl, PortraitName

	GemRB.LoadWindowPack ("GUICG", 640, 480)
	CharGenWindow = GemRB.LoadWindowObject (0)
	PortraitButton = CharGenWindow.GetControl (12)
	PortraitButton.SetFlags(IE_GUI_BUTTON_PICTURE|IE_GUI_BUTTON_NO_IMAGE,OP_SET)
	PortraitName = GemRB.GetToken ("LargePortrait")
	PortraitButton.SetPicture (PortraitName,"NOPORTLG")

	RaceTable = GemRB.LoadTableObject ("races")
	ClassTable = GemRB.LoadTableObject ("classes")
	KitTable = GemRB.LoadTableObject ("kitlist")
	AlignmentTable = GemRB.LoadTableObject ("aligns")
	AbilityTable = GemRB.LoadTableObject ("ability")

	GenderButton = CharGenWindow.GetControl (0)
	GenderButton.SetText (11956)
	GenderButton.SetState (IE_GUI_BUTTON_DISABLED)

	RaceButton = CharGenWindow.GetControl (1)
	RaceButton.SetText (11957)
	RaceButton.SetState (IE_GUI_BUTTON_DISABLED)

	ClassButton = CharGenWindow.GetControl (2)
	ClassButton.SetText (11959)
	ClassButton.SetState (IE_GUI_BUTTON_DISABLED)

	AlignmentButton = CharGenWindow.GetControl (3)
	AlignmentButton.SetText (11958)
	AlignmentButton.SetState (IE_GUI_BUTTON_DISABLED)

	AbilitiesButton = CharGenWindow.GetControl (4)
	AbilitiesButton.SetText (11960)
	AbilitiesButton.SetState (IE_GUI_BUTTON_DISABLED)

	SkillButton = CharGenWindow.GetControl (5)
	SkillButton.SetText (17372)
	SkillButton.SetState (IE_GUI_BUTTON_DISABLED)

	AppearanceButton = CharGenWindow.GetControl (6)
	AppearanceButton.SetText (11961)
	AppearanceButton.SetState (IE_GUI_BUTTON_DISABLED)

	NameButton = CharGenWindow.GetControl (7)
	NameButton.SetText (11963)
	NameButton.SetState (IE_GUI_BUTTON_DISABLED)

	BackButton = CharGenWindow.GetControl (11)
	BackButton.SetState (IE_GUI_BUTTON_ENABLED)

	AcceptButton = CharGenWindow.GetControl (8)
	AcceptButton.SetText (11962)
	AcceptButton.SetState (IE_GUI_BUTTON_ENABLED)
	AcceptButton.SetFlags(IE_GUI_BUTTON_DEFAULT,OP_OR)

	ImportButton = CharGenWindow.GetControl (13)
	ImportButton.SetText (13955)
	ImportButton.SetState (IE_GUI_BUTTON_ENABLED)

	CancelButton = CharGenWindow.GetControl (15)
	CancelButton.SetText (13727)
	CancelButton.SetState (IE_GUI_BUTTON_ENABLED)

	TextAreaControl= CharGenWindow.GetControl (9)

	TextAreaControl.SetText (1047)
	TextAreaControl.Append (": ")
	TextAreaControl.Append (GemRB.GetToken ("CHARNAME") )

	TextAreaControl.Append (12135, -1)
	TextAreaControl.Append (": ")
	if GemRB.GetVar ("Gender") == 1:
		TextAreaControl.Append (1050)
	else:
		TextAreaControl.Append (1051)
	TextAreaControl.Append (1048,-1) # new line
	TextAreaControl.Append (": ")
	TextAreaControl.Append (RaceTable.GetValue (GemRB.GetVar ("Race")-1,2))
	TextAreaControl.Append (12136, -1)
	TextAreaControl.Append (": ")
	KitIndex = GemRB.GetVar ("Class Kit")
	if KitIndex == 0:
		TextAreaControl.Append (ClassTable.GetValue (GemRB.GetVar ("Class")-1,2))
	else:
		TextAreaControl.Append (KitTable.GetValue (KitIndex,2) )
	TextAreaControl.Append (1049, -1)
	TextAreaControl.Append (": ")
	v = AlignmentTable.FindValue (3,GemRB.GetVar ("Alignment"))
	TextAreaControl.Append (AlignmentTable.GetValue (v,2))
	for i in range(6):
		v = AbilityTable.GetValue (i,2)
		TextAreaControl.Append (v, -1)
		TextAreaControl.Append (": "+str(GemRB.GetVar ("Ability "+str(i))))

	CancelButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, "CancelPress")
	BackButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, "BackPress")
	AcceptButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, "NextPress")
	ImportButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, "ImportPress")
	CharGenWindow.SetVisible (1)
	return

def NextPress():
	if CharGenWindow:
		CharGenWindow.Unload ()
	#set my character up
	MyChar = GemRB.GetVar ("Slot")
	GemRB.CreatePlayer ("charbase", MyChar | 0x8000 )
	GemRB.SetPlayerStat (MyChar, IE_SEX, GemRB.GetVar ("Gender") )
	GemRB.SetPlayerStat (MyChar, IE_RACE, GemRB.GetVar ("Race") )
	ClassTable = GemRB.LoadTableObject ("classes")
	ClassIndex = GemRB.GetVar ("Class")-1
	Class = ClassTable.GetValue (ClassIndex, 5)
	GemRB.SetPlayerStat (MyChar, IE_CLASS, Class)
	KitIndex = GemRB.GetVar ("Class Kit")
	GemRB.SetPlayerStat (MyChar, IE_KIT, KitIndex)
	t = GemRB.GetVar ("Alignment")
	GemRB.SetPlayerStat (MyChar, IE_ALIGNMENT, t)

	TmpTable = GemRB.LoadTableObject ("clskills")
	#mage spells
	TableName = TmpTable.GetValue (Class, 1, 0)
	if TableName != "*":
		SetupSpellLevels(MyChar, TableName, IE_SPELL_TYPE_WIZARD, 1)
		Learnable = GetLearnableMageSpells( KitIndex, t, 1)
		SpellBook = GemRB.GetVar ("MageSpellBook")
		j=1
		for i in range(len(Learnable) ):
			if SpellBook & j:
				GemRB.LearnSpell (MyChar, Learnable[i], 0)
			j=j<<1

	#priest spells
	TableName = TmpTable.GetValue (Class, 1, 0)
	if TableName != "*":
		SetupSpellLevels(MyChar, TableName, IE_SPELL_TYPE_PRIEST, 1)
		ClassFlag = 0 #set this according to class
		Learnable = GetLearnablePriestSpells( ClassFlag, t, 1)
		for i in range(len(Learnable) ):
			GemRB.LearnSpell (MyChar, Learnable[i], 0)

	TmpTable=GemRB.LoadTableObject ("repstart")
	AlignmentTable = GemRB.LoadTableObject ("aligns")
	t = AlignmentTable.FindValue (3, t)
	t = TmpTable.GetValue (t,0) * 10
	GemRB.SetPlayerStat (MyChar, IE_REPUTATION, t)

	#slot 1 is the protagonist
	if MyChar == 1:
		GemRB.GameSetReputation( t )

	TmpTable=GemRB.LoadTableObject ("strtgold")
	t = GemRB.Roll (TmpTable.GetValue (Class,1),TmpTable.GetValue (Class,0), TmpTable.GetValue (Class,2) )
	GemRB.SetPlayerStat (MyChar, IE_GOLD, t*TmpTable.GetValue (Class,3) )

	GemRB.SetPlayerStat (MyChar, IE_HATEDRACE, GemRB.GetVar ("HatedRace") )
	TmpTable=GemRB.LoadTableObject ("ability")
	AbilityCount = TmpTable.GetRowCount ()
	for i in range(AbilityCount):
		StatID=TmpTable.GetValue (i,4)
		GemRB.SetPlayerStat (MyChar, StatID, GemRB.GetVar ("Ability "+str(i) ) )

	TmpTable=GemRB.LoadTableObject ("weapprof")
	ProfCount = TmpTable.GetRowCount ()
	for i in range(7,ProfCount):
		StatID=TmpTable.GetValue (i, 0)
		GemRB.SetPlayerStat (MyChar, StatID, GemRB.GetVar ("Prof "+str(i) ) )

	SetColorStat (MyChar, IE_HAIR_COLOR, GemRB.GetVar ("HairColor") )
	SetColorStat (MyChar, IE_SKIN_COLOR, GemRB.GetVar ("SkinColor") )
	SetColorStat (MyChar, IE_MAJOR_COLOR, GemRB.GetVar ("MajorColor") )
	SetColorStat (MyChar, IE_MINOR_COLOR, GemRB.GetVar ("MinorColor") )
	SetColorStat (MyChar, IE_METAL_COLOR, 0x1B )
	SetColorStat (MyChar, IE_LEATHER_COLOR, 0x16 )
	SetColorStat (MyChar, IE_ARMOR_COLOR, 0x17 )
	GemRB.SetPlayerStat (MyChar, IE_EA, 2 )
	Str=GemRB.GetVar ("Ability 0")
	GemRB.SetPlayerStat (MyChar, IE_STR, Str)
	if Str==18:
		GemRB.SetPlayerStat (MyChar,IE_STREXTRA,GemRB.GetVar ("StrExtra"))
	else:
		GemRB.SetPlayerStat (MyChar, IE_STREXTRA,0)

	GemRB.SetPlayerStat (MyChar, IE_DEX, GemRB.GetVar ("Ability 1"))
	GemRB.SetPlayerStat (MyChar, IE_CON, GemRB.GetVar ("Ability 2"))
	GemRB.SetPlayerStat (MyChar, IE_INT, GemRB.GetVar ("Ability 3"))
	GemRB.SetPlayerStat (MyChar, IE_WIS, GemRB.GetVar ("Ability 4"))
	GemRB.SetPlayerStat (MyChar, IE_CHR, GemRB.GetVar ("Ability 5"))

	#setting skills (thieving/ranger)
	TmpTable = GemRB.LoadTableObject ("skills")
	RowCount = TmpTable.GetRowCount ()-2

	for i in range(RowCount):
		stat = TmpTable.GetValue (i+2, 2)
		value = GemRB.GetVar ("Skill "+str(i) )
		GemRB.SetPlayerStat (MyChar, stat, value )

	GemRB.SetPlayerName (MyChar, GemRB.GetToken ("CHARNAME"), 0)
	#does all the rest
	LargePortrait = GemRB.GetToken ("LargePortrait")
	SmallPortrait = GemRB.GetToken ("SmallPortrait")
	GemRB.FillPlayerInfo (MyChar, LargePortrait, SmallPortrait)
	#10 is a weapon slot (see slottype.2da row 10)
	GemRB.CreateItem (MyChar, "staf01", 10, 1, 0, 0)
	GemRB.SetEquippedQuickSlot (MyChar, 0)
	#LETS PLAY!!
	playmode = GemRB.GetVar ("PlayMode")
	if playmode >=0:
		GemRB.EnterGame()
	else:
		#leaving multi player pregen
		if CharGenWindow:
			CharGenWindow.Unload ()
		GemRB.SetNextScript ("GUIMP")
	return

def CancelPress():
	if CharGenWindow:
		CharGenWindow.Unload ()
	GemRB.SetNextScript ("CharGen")
	return

def BackPress():
	if CharGenWindow:
		CharGenWindow.Unload ()
	GemRB.SetNextScript ("CharGen8") #name
	return

def ImportPress():
	if CharGenWindow:
		CharGenWindow.Unload ()
	GemRB.SetToken ("NextScript","CharGen9")
	GemRB.SetNextScript ("ImportFile") #import
	return

