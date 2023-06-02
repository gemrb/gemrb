# GemRB - Infinity Engine Emulator
# Copyright (C) 2003-2005 The GemRB Project
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#


#Character Generation

###################################################

import GemRB
from GUIDefines import *
from ie_stats import *
from ie_spells import LS_MEMO
import GUICommon
import Spellbook
import CommonTables
import LUSkillsSelection
import PaperDoll

CharGenWindow = 0
CharGenState = 0
TextArea = 0
PortraitButton = 0
AcceptButton = 0

GenderButton = 0
GenderWindow = 0
GenderTextArea = 0
GenderDoneButton = 0

Portrait = 0
PortraitsTable = 0
PortraitPortraitButton = 0

RaceButton = 0
RaceWindow = 0
RaceTextArea = 0
RaceDoneButton = 0

ClassButton = 0
ClassWindow = 0
ClassTextArea = 0
ClassDoneButton = 0

ClassMultiWindow = 0
ClassMultiTextArea = 0
ClassMultiDoneButton = 0

KitTable = 0
KitWindow = 0
KitTextArea = 0
KitDoneButton = 0

AlignmentButton = 0
AlignmentWindow = 0
AlignmentTextArea = 0
AlignmentDoneButton = 0

AbilitiesButton = 0
AbilitiesWindow = 0
AbilitiesTable = 0
AbilitiesRaceAddTable = 0
AbilitiesRaceReqTable = 0
AbilitiesClassReqTable = 0
AbilitiesMinimum = 0
AbilitiesMaximum = 0
AbilitiesModifier = 0
AbilitiesTextArea = 0
AbilitiesRecallButton = 0
AbilitiesDoneButton = 0

SkillsButton = 0
SkillsWindow = 0
SkillsTable = 0
SkillsTextArea = 0
SkillsDoneButton = 0
SkillsPointsLeft = 0
SkillsState = 0

RacialEnemyButton = 0
RacialEnemyWindow = 0
RacialEnemyTable = 0
RacialEnemyTextArea = 0
RacialEnemyDoneButton = 0

ProficienciesWindow = 0
ProficienciesTable = 0
ProfsMaxTable = 0
ProficienciesTextArea = 0
ProficienciesDoneButton = 0
ProficienciesPointsLeft = 0

MageSpellsWindow = 0
MageSpellsTextArea = 0
MageSpellsDoneButton = 0
MageSpellsSelectPointsLeft = 0

MageMemorizeWindow = 0
MageMemorizeTextArea = 0
MageMemorizeDoneButton = 0
MageMemorizePointsLeft = 0

PriestMemorizeWindow = 0
PriestMemorizeTextArea = 0
PriestMemorizeDoneButton = 0
PriestMemorizePointsLeft = 0

AppearanceButton = 0

CharSoundWindow = 0
CharSoundTable = 0
CharSoundStrings = 0

BiographyButton = 0
BiographyWindow = 0
BiographyTextArea = 0

NameButton = 0
NameWindow = 0
NameField = 0
NameDoneButton = 0

SoundIndex = 0
VerbalConstants = None
HasStrExtra = 0
MyChar = 0
ImportedChar = 0

def OnLoad():
	global CharGenWindow, CharGenState, TextArea, PortraitButton, AcceptButton
	global GenderButton, RaceButton, ClassButton, AlignmentButton
	global AbilitiesButton, SkillsButton, AppearanceButton, BiographyButton, NameButton
	global KitTable, ProficienciesTable, RacialEnemyTable
	global AbilitiesTable, SkillsTable, PortraitsTable
	global MyChar, ImportedChar

	KitTable = GemRB.LoadTable ("magesch")
	ProficienciesTable = GemRB.LoadTable ("weapprof")
	RacialEnemyTable = GemRB.LoadTable ("haterace")
	AbilitiesTable = GemRB.LoadTable ("ability")
	SkillsTable = GemRB.LoadTable ("skills")
	PortraitsTable = GemRB.LoadTable ("pictures")
	CharGenWindow = GemRB.LoadWindow (0, "GUICG")
	CharGenState = 0
	MyChar = GemRB.GetVar ("Slot")
	ImportedChar = 0

	GenderButton = CharGenWindow.GetControl (0)
	GenderButton.SetState (IE_GUI_BUTTON_ENABLED)
	GenderButton.MakeDefault()
	GenderButton.OnPress (GenderPress)
	GenderButton.SetText (11956)

	RaceButton = CharGenWindow.GetControl (1)
	RaceButton.SetState (IE_GUI_BUTTON_DISABLED)
	RaceButton.OnPress (RacePress)
	RaceButton.SetText (11957)

	ClassButton = CharGenWindow.GetControl (2)
	ClassButton.SetState (IE_GUI_BUTTON_DISABLED)
	ClassButton.OnPress (ClassPress)
	ClassButton.SetText (11959)

	AlignmentButton = CharGenWindow.GetControl (3)
	AlignmentButton.SetState (IE_GUI_BUTTON_DISABLED)
	AlignmentButton.OnPress (AlignmentPress)
	AlignmentButton.SetText (11958)

	AbilitiesButton = CharGenWindow.GetControl (4)
	AbilitiesButton.SetState (IE_GUI_BUTTON_DISABLED)
	AbilitiesButton.OnPress (AbilitiesPress)
	AbilitiesButton.SetText (11960)

	SkillsButton = CharGenWindow.GetControl (5)
	SkillsButton.SetState (IE_GUI_BUTTON_DISABLED)
	SkillsButton.OnPress (SkillsPress)
	SkillsButton.SetText (11983)

	AppearanceButton = CharGenWindow.GetControl (6)
	AppearanceButton.SetState (IE_GUI_BUTTON_DISABLED)
	AppearanceButton.OnPress (AppearancePress)
	AppearanceButton.SetText (11961)

	BiographyButton = CharGenWindow.GetControl (16)
	BiographyButton.SetState (IE_GUI_BUTTON_DISABLED)
	BiographyButton.OnPress (BiographyPress)
	BiographyButton.SetText (18003)

	NameButton = CharGenWindow.GetControl (7)
	NameButton.SetState (IE_GUI_BUTTON_DISABLED)
	NameButton.OnPress (NamePress)
	NameButton.SetText (11963)

	BackButton = CharGenWindow.GetControl (11)
	BackButton.SetState (IE_GUI_BUTTON_ENABLED)
	BackButton.OnPress (BackPress)
	BackButton.MakeEscape()

	PortraitButton = CharGenWindow.GetControl (12)
	PortraitButton.SetFlags (IE_GUI_BUTTON_PICTURE|IE_GUI_BUTTON_NO_IMAGE, OP_SET)
	PortraitButton.SetState (IE_GUI_BUTTON_LOCKED)

	ImportButton = CharGenWindow.GetControl (13)
	ImportButton.SetState (IE_GUI_BUTTON_ENABLED)
	ImportButton.SetText (13955)
	ImportButton.OnPress (ImportPress)

	CancelButton = CharGenWindow.GetControl (15)
	CancelButton.SetState (IE_GUI_BUTTON_ENABLED)
	CancelButton.SetText (13727)
	CancelButton.OnPress (CancelPress)

	AcceptButton = CharGenWindow.GetControl (8)
	AcceptButton.SetState (IE_GUI_BUTTON_DISABLED)
	AcceptButton.SetText (11962)
	AcceptButton.OnPress (AcceptPress)

	TextArea = CharGenWindow.GetControl (9)
	TextArea.SetText (16575)

	return

def BackPress():
	global CharGenWindow, CharGenState, SkillsState
	global GenderButton, RaceButton, ClassButton, AlignmentButton, AbilitiesButton, SkillsButton, AppearanceButton, BiographyButton, NameButton

	if CharGenState > 0:
		CharGenState = CharGenState - 1
	else:
		CancelPress()
		return

	if CharGenState > 6:
		CharGenState = 6
		GemRB.SetToken ("CHARNAME","")

	if CharGenState == 0:
		RaceButton.SetState (IE_GUI_BUTTON_DISABLED)
		GenderButton.SetState (IE_GUI_BUTTON_ENABLED)
		GenderButton.MakeDefault()
	elif CharGenState == 1:
		ClassButton.SetState (IE_GUI_BUTTON_DISABLED)
		RaceButton.SetState (IE_GUI_BUTTON_ENABLED)
		RaceButton.MakeDefault()
	elif CharGenState == 2:
		AlignmentButton.SetState (IE_GUI_BUTTON_DISABLED)
		ClassButton.SetState (IE_GUI_BUTTON_ENABLED)
		ClassButton.MakeDefault()
	elif CharGenState == 3:
		AbilitiesButton.SetState (IE_GUI_BUTTON_DISABLED)
		AlignmentButton.SetState (IE_GUI_BUTTON_ENABLED)
		AlignmentButton.MakeDefault()
	elif CharGenState == 4:
		SkillsButton.SetState (IE_GUI_BUTTON_DISABLED)
		AbilitiesButton.SetState (IE_GUI_BUTTON_ENABLED)
		AbilitiesButton.MakeDefault()
	elif CharGenState == 5:
		AppearanceButton.SetState (IE_GUI_BUTTON_DISABLED)
		SkillsButton.SetState (IE_GUI_BUTTON_ENABLED)
		SkillsButton.MakeDefault()
		SkillsState = 0
	elif CharGenState == 6:
		NameButton.SetState (IE_GUI_BUTTON_DISABLED)
		BiographyButton.SetState (IE_GUI_BUTTON_DISABLED)
		AppearanceButton.SetState (IE_GUI_BUTTON_ENABLED)

	AcceptButton.SetState (IE_GUI_BUTTON_DISABLED)
	SetCharacterDescription()
	return

def CancelPress():
	global CharGenWindow

	if CharGenWindow:
		CharGenWindow.Close ()
	GemRB.CreatePlayer ("", MyChar | 0x8000 )
	GemRB.SetNextScript ("PartyFormation")
	return

def LearnSpells(MyChar):
	Kit = GemRB.GetPlayerStat (MyChar, IE_KIT)
	ClassName = GUICommon.GetClassRowName (MyChar)
	t = GemRB.GetPlayerStat (MyChar, IE_ALIGNMENT)

	# mage spells
	TableName = CommonTables.ClassSkills.GetValue (ClassName, "MAGESPELL", GTV_STR)
	if TableName != "*":
		# setting up just the first spell level is enough, since the rest will be granted on level-up
		Spellbook.SetupSpellLevels (MyChar, TableName, IE_SPELL_TYPE_WIZARD, 1)
		Learnable = Spellbook.GetLearnableMageSpells (Kit, t, 1)
		SpellBook = GemRB.GetVar ("MageSpellBook")
		MemoBook = GemRB.GetVar ("MageMemorized")
		j=1
		for i in range (len(Learnable) ):
			if SpellBook & j:
				memorize = LS_MEMO if MemoBook & j else 0
				GemRB.LearnSpell (MyChar, Learnable[i], memorize)
			j=j<<1

	#priest spells
	TableName = CommonTables.ClassSkills.GetValue (ClassName, "CLERICSPELL", GTV_STR)
	# druids and rangers have a column of their own
	if TableName == "*":
		TableName = CommonTables.ClassSkills.GetValue (ClassName, "DRUIDSPELL", GTV_STR)
	if TableName != "*":
		ClassFlag = GetClassFlag (TableName)
		TableName = Spellbook.GetPriestSpellTable (TableName)

		Spellbook.SetupSpellLevels (MyChar, TableName, IE_SPELL_TYPE_PRIEST, 1)
		Learnable = Spellbook.GetLearnablePriestSpells (ClassFlag, t, 1)
		PriestMemorized = GemRB.GetVar ("PriestMemorized")
		j = 0
		while PriestMemorized and PriestMemorized != 1<<j:
			j = j + 1
		for i in range (len(Learnable) ):
			GemRB.LearnSpell (MyChar, Learnable[i], 0)
		GemRB.MemorizeSpell (MyChar, IE_SPELL_TYPE_PRIEST, 0, j, 1)

def AcceptPress():
	if ImportedChar:
		GemRB.SetPlayerName (MyChar, GemRB.GetToken ("CHARNAME"), 0)
		GemRB.SetToken ("CHARNAME","")
		LargePortrait = GemRB.GetToken ("LargePortrait")
		SmallPortrait = GemRB.GetToken ("SmallPortrait")
		GemRB.FillPlayerInfo (MyChar, LargePortrait, SmallPortrait)

		if CharGenWindow:
			CharGenWindow.Close ()
		GemRB.SetNextScript ("PartyFormation")
		return

	# apply class/kit abilities
	ClassName = GUICommon.GetClassRowName (MyChar)
	GUICommon.ResolveClassAbilities (MyChar, ClassName)
	t = GemRB.GetPlayerStat (MyChar, IE_ALIGNMENT)

	TmpTable = GemRB.LoadTable ("repstart")
	t = CommonTables.Aligns.FindValue (3, t)
	t = TmpTable.GetValue (t, 0) * 10
	GemRB.SetPlayerStat (MyChar, IE_REPUTATION, t)
	# set the party rep if this in the main char
	if MyChar == 1:
		GemRB.GameSetReputation (t)

	TmpTable = GemRB.LoadTable ("strtgold")
	a = TmpTable.GetValue (ClassName, "ROLLS") #number of dice
	b = TmpTable.GetValue (ClassName, "SIDES") #size
	c = TmpTable.GetValue (ClassName, "MODIFIER") #adjustment
	d = TmpTable.GetValue (ClassName, "MULTIPLIER") #external multiplier
	e = TmpTable.GetValue (ClassName, "BONUS_PER_LEVEL") #level bonus rate (iwd only!)
	t = GemRB.GetPlayerStat (MyChar, IE_LEVEL)
	if t>1:
		e=e*(t-1)
	else:
		e=0
	t = GemRB.Roll (a,b,c)*d+e
	GemRB.SetPlayerStat (MyChar, IE_GOLD, t)
	GemRB.SetPlayerStat (MyChar, IE_EA, 2 )

	GemRB.SetPlayerName (MyChar, GemRB.GetToken ("CHARNAME"), 0)
	GemRB.SetToken ("CHARNAME","")
	GemRB.SetPlayerStat (MyChar, IE_XP, CommonTables.ClassSkills.GetValue (ClassName, "STARTXP"))

	GUICommon.SetColorStat (MyChar, IE_SKIN_COLOR, GemRB.GetVar ("SkinColor") )
	GUICommon.SetColorStat (MyChar, IE_HAIR_COLOR, GemRB.GetVar ("HairColor") )
	GUICommon.SetColorStat (MyChar, IE_MAJOR_COLOR, GemRB.GetVar ("MajorColor") )
	GUICommon.SetColorStat (MyChar, IE_MINOR_COLOR, GemRB.GetVar ("MinorColor") )
	GUICommon.SetColorStat (MyChar, IE_METAL_COLOR, 0x1B )
	GUICommon.SetColorStat (MyChar, IE_LEATHER_COLOR, 0x16 )
	GUICommon.SetColorStat (MyChar, IE_ARMOR_COLOR, 0x17 )

	#does all the rest
	LargePortrait = GemRB.GetToken ("LargePortrait")
	SmallPortrait = GemRB.GetToken ("SmallPortrait")
	GemRB.FillPlayerInfo (MyChar, LargePortrait, SmallPortrait)

	#10 is a weapon slot (see slottype.2da row 10)
	GemRB.CreateItem (MyChar, "staf01", 10, 1, 0, 0)
	GemRB.SetEquippedQuickSlot (MyChar, 0)

	if CharGenWindow:
		CharGenWindow.Close ()
	GemRB.SetNextScript ("PartyFormation")
	return

def SetCharacterDescription():
	global CharGenWindow, TextArea, CharGenState, ClassFlag
	global MyChar

	TextArea.Clear()
	if CharGenState > 7:
		TextArea.Append (1047)
		TextArea.Append (": ")
		TextArea.Append (GemRB.GetToken ("CHARNAME"))
		TextArea.Append ("\n")
	if CharGenState > 0:
		TextArea.Append (12135)
		TextArea.Append (": ")
		if GemRB.GetPlayerStat (MyChar, IE_SEX) == 1:
			TextArea.Append (1050)
		else:
			TextArea.Append (1051)
		TextArea.Append ("\n")
	if CharGenState > 2:
		ClassName = GUICommon.GetClassRowName (MyChar)
		TextArea.Append (12136)
		TextArea.Append (": ")
		#this is only mage school in iwd
		Kit = GemRB.GetPlayerStat (MyChar, IE_KIT)
		KitIndex = KitTable.FindValue (3, Kit)
		if KitIndex <= 0:
			ClassTitle = CommonTables.Classes.GetValue (ClassName, "CAP_REF")
		else:
			ClassTitle = KitTable.GetValue (KitIndex, 2)
		TextArea.Append (ClassTitle)
		TextArea.Append ("\n")

	if CharGenState > 1:
		TextArea.Append (1048)
		TextArea.Append (": ")
		Race = GemRB.GetPlayerStat (MyChar, IE_RACE)
		Race = CommonTables.Races.FindValue (3, Race)
		TextArea.Append (CommonTables.Races.GetValue (Race, 2) )
		TextArea.Append ("\n")
	if CharGenState > 3:
		TextArea.Append (1049)
		TextArea.Append (": ")
		Alignment = CommonTables.Aligns.FindValue (3, GemRB.GetPlayerStat(MyChar, IE_ALIGNMENT))
		TextArea.Append (CommonTables.Aligns.GetValue (Alignment, 2))
		TextArea.Append ("\n")
	if CharGenState > 4:
		strextra = GemRB.GetPlayerStat (MyChar, IE_STREXTRA)
		TextArea.Append ("\n")
		for i in range (6):
			TextArea.Append (AbilitiesTable.GetValue (i, 2))
			TextArea.Append (": " )
			StatID = AbilitiesTable.GetValue (i, 3)
			stat = GemRB.GetPlayerStat (MyChar, StatID)
			if (i == 0) and HasStrExtra and (stat==18):
				TextArea.Append (str(stat) + "/" + str(strextra) )
			else:
				TextArea.Append (str(stat) )
			TextArea.Append ("\n")
	if CharGenState > 5:
		DruidSpell = CommonTables.ClassSkills.GetValue (ClassName, "DRUIDSPELL")
		PriestSpell = CommonTables.ClassSkills.GetValue (ClassName, "CLERICSPELL")
		MageSpell = CommonTables.ClassSkills.GetValue (ClassName, "MAGESPELL")
		IsBard = CommonTables.ClassSkills.GetValue (ClassName, "BARDSKILL")
		IsThief = CommonTables.ClassSkills.GetValue (ClassName, "THIEFSKILL")

		if IsThief!="*":
			TextArea.Append ("\n")
			TextArea.Append (8442)
			TextArea.Append ("\n")
			for i in range (4):
				TextArea.Append (SkillsTable.GetValue (i+2, 2))
				StatID = SkillsTable.GetValue (i+2, 3)
				TextArea.Append (": " )
				TextArea.Append (str(GemRB.GetPlayerStat (MyChar, StatID)) )
				TextArea.Append ("%\n")
		elif DruidSpell!="*":
			PositiveStats = []
			for i in range (4):
				StatID = SkillsTable.GetValue (i+2, 3)
				Stat = GemRB.GetPlayerStat (MyChar, StatID)
				if Stat>0:
					PositiveStats.append ((i, Stat))
			if PositiveStats:
				TextArea.Append ("\n")
				TextArea.Append (8442)
				TextArea.Append ("\n")
				for i, Stat in PositiveStats:
					TextArea.Append (SkillsTable.GetValue (i+2, 2))
					TextArea.Append (": " )
					TextArea.Append (str(Stat) )
					TextArea.Append ("%\n")

			RacialEnemy = GemRB.GetVar ("RacialEnemyIndex") + GemRB.GetVar ("RacialEnemy") - 1
			if RacialEnemy != -1:
				TextArea.Append ("\n")
				TextArea.Append (15982)
				TextArea.Append (": " )
				TextArea.Append (RacialEnemyTable.GetValue (RacialEnemy, 3))
				TextArea.Append ("\n")
		elif IsBard!="*":
			TextArea.Append ("\n")
			TextArea.Append (8442)
			TextArea.Append ("\n")
			for i in range (4):
				StatID = SkillsTable.GetValue (i+2, 3)
				Stat = GemRB.GetPlayerStat (MyChar, StatID)
				if Stat>0:
					TextArea.Append (SkillsTable.GetValue (i+2, 2))
					TextArea.Append (": " )
					TextArea.Append (str(Stat) )
					TextArea.Append ("%\n")

		if MageSpell != "*":
			info = Spellbook.GetKnownSpellsDescription (MyChar, IE_SPELL_TYPE_WIZARD)
			if info:
				TextArea.Append ("\n" + GemRB.GetString (11027) + "\n" + info)

		if PriestSpell == "*":
			PriestSpell = DruidSpell
		if PriestSpell != "*":
			info = Spellbook.GetKnownSpellsDescription (MyChar, IE_SPELL_TYPE_PRIEST)
			if info:
				TextArea.Append ("\n" + GemRB.GetString (11028) + "\n" + info)

		TextArea.Append ("\n")
		TextArea.Append (9466)
		TextArea.Append ("\n")
		for i in range (15):
			StatID = ProficienciesTable.GetValue (i, 0)
			ProficiencyValue = GemRB.GetPlayerStat (MyChar, StatID )
			if ProficiencyValue > 0:
				TextArea.Append (ProficienciesTable.GetValue (i, 3))
				TextArea.Append (" ")
				j = 0
				while j < ProficiencyValue:
					TextArea.Append ("+")
					j = j + 1
				TextArea.Append ("\n")
	return

def GetClassFlag(TableName):
	if TableName in ("MXSPLPRS", "MXSPLPAL"):
		return 0x4000
	elif TableName in ("MXSPLDRU", "MXSPLRAN"):
		return 0x8000
	else:
		return 0

# Gender Selection
def GenderPress():
	global CharGenWindow, GenderWindow, GenderDoneButton, GenderTextArea
	global MyChar

	GenderWindow = GemRB.LoadWindow (1)
	GemRB.SetVar ("Gender", 0)
	GemRB.CreatePlayer ("charbase", MyChar | 0x8000 )

	MaleButton = GenderWindow.GetControl (2)
	MaleButton.SetState (IE_GUI_BUTTON_ENABLED)
	MaleButton.SetFlags (IE_GUI_BUTTON_RADIOBUTTON,OP_OR)
	MaleButton.OnPress (MalePress)

	FemaleButton = GenderWindow.GetControl (3)
	FemaleButton.SetState (IE_GUI_BUTTON_ENABLED)
	FemaleButton.SetFlags (IE_GUI_BUTTON_RADIOBUTTON,OP_OR)
	FemaleButton.OnPress (FemalePress)

	MaleButton.SetVarAssoc ("Gender", 1)
	FemaleButton.SetVarAssoc ("Gender", 2)

	GenderTextArea = GenderWindow.GetControl (5)
	GenderTextArea.SetText (17236)

	GenderDoneButton = GenderWindow.GetControl (0)
	GenderDoneButton.SetState (IE_GUI_BUTTON_DISABLED)
	GenderDoneButton.OnPress (GenderDonePress)
	GenderDoneButton.SetText (11973)
	GenderDoneButton.MakeDefault()

	GenderCancelButton = GenderWindow.GetControl (6)
	GenderCancelButton.SetState (IE_GUI_BUTTON_ENABLED)
	GenderCancelButton.OnPress (GenderCancelPress)
	GenderCancelButton.SetText (13727)
	GenderCancelButton.MakeEscape()

	GenderWindow.ShowModal(MODAL_SHADOW_NONE)
	return

def MalePress():
	global GenderWindow, GenderDoneButton, GenderTextArea

	GenderTextArea.SetText (13083)
	GenderDoneButton.SetState (IE_GUI_BUTTON_ENABLED)
	return

def FemalePress():
	global GenderWindow, GenderDoneButton, GenderTextArea

	GenderTextArea.SetText (13084)
	GenderDoneButton.SetState (IE_GUI_BUTTON_ENABLED)
	return

def GenderDonePress():
	global CharGenWindow, GenderWindow
	global MyChar

	if GenderWindow:
		GenderWindow.Close ()
	Gender = GemRB.GetVar ("Gender")
	GemRB.SetPlayerStat (MyChar, IE_SEX, Gender)

	PortraitSelect()
	return

def GenderCancelPress():
	global CharGenWindow, GenderWindow
	global MyChar

	GemRB.SetVar ("Gender", 0)
	GemRB.SetPlayerStat (MyChar, IE_SEX, 0)
	if GenderWindow:
		GenderWindow.Close ()
	return

def PortraitSelect():
	global CharGenWindow, PortraitWindow, Portrait, PortraitPortraitButton
	global MyChar

	PortraitWindow = GemRB.LoadWindow (11)

	# this is not the correct one, but I don't know which is
	Portrait = 0

	PortraitPortraitButton = PortraitWindow.GetControl (1)
	PortraitPortraitButton.SetState (IE_GUI_BUTTON_DISABLED)
	PortraitPortraitButton.SetFlags (IE_GUI_BUTTON_PICTURE|IE_GUI_BUTTON_NO_IMAGE, OP_SET)

	PortraitLeftButton = PortraitWindow.GetControl (2)
	PortraitLeftButton.SetState (IE_GUI_BUTTON_ENABLED)
	PortraitLeftButton.OnPress (CGPortraitLeftPress)
	PortraitLeftButton.SetFlags (IE_GUI_BUTTON_RADIOBUTTON, OP_OR)

	PortraitRightButton = PortraitWindow.GetControl (3)
	PortraitRightButton.SetState (IE_GUI_BUTTON_ENABLED)
	PortraitRightButton.OnPress (CGPortraitRightPress)
	PortraitRightButton.SetFlags (IE_GUI_BUTTON_RADIOBUTTON, OP_OR)

	PortraitCustomButton = PortraitWindow.GetControl (6)
	PortraitCustomButton.SetState (IE_GUI_BUTTON_ENABLED)
	PortraitCustomButton.OnPress (PortraitCustomPress)
	PortraitCustomButton.SetText (17545)

	PortraitDoneButton = PortraitWindow.GetControl (0)
	PortraitDoneButton.SetState (IE_GUI_BUTTON_ENABLED)
	PortraitDoneButton.OnPress (CGPortraitDonePress)
	PortraitDoneButton.SetText (11973)
	PortraitDoneButton.MakeDefault()

	PortraitCancelButton = PortraitWindow.GetControl (5)
	PortraitCancelButton.SetState (IE_GUI_BUTTON_ENABLED)
	PortraitCancelButton.OnPress (CGPortraitCancelPress)
	PortraitCancelButton.SetText (13727)
	PortraitCancelButton.MakeEscape()

	while PortraitsTable.GetValue (Portrait, 0) != GemRB.GetPlayerStat (MyChar, IE_SEX):
		Portrait = Portrait + 1
	PortraitPortraitButton.SetPicture (PortraitsTable.GetRowName (Portrait) + "G")

	PortraitWindow.ShowModal(MODAL_SHADOW_NONE)
	return

def CGPortraitLeftPress():
	global PortraitWindow, Portrait, PortraitPortraitButton
	global MyChar

	while True:
		Portrait = Portrait - 1
		if Portrait < 0:
			Portrait = PortraitsTable.GetRowCount () - 1
		if PortraitsTable.GetValue (Portrait, 0) == GemRB.GetPlayerStat(MyChar, IE_SEX):
			PortraitPortraitButton.SetPicture (PortraitsTable.GetRowName (Portrait) + "G")
			return

def CGPortraitRightPress():
	global PortraitWindow, Portrait, PortraitPortraitButton
	global MyChar

	while True:
		Portrait = Portrait + 1
		if Portrait == PortraitsTable.GetRowCount():
			Portrait = 0
		if PortraitsTable.GetValue (Portrait, 0) == GemRB.GetPlayerStat(MyChar, IE_SEX):
			PortraitPortraitButton.SetPicture (PortraitsTable.GetRowName (Portrait) + "G")
			return

def CustomDone():
	global CharGenWindow, PortraitWindow
	global PortraitButton, GenderButton, RaceButton
	global CharGenState, Portrait

	Window = CustomWindow

	PortraitName = PortraitList2.QueryText ()
	GemRB.SetToken ("SmallPortrait", PortraitName)
	PortraitName = PortraitList1.QueryText ()
	GemRB.SetToken ("LargePortrait", PortraitName)
	if Window:
		Window.Close ()

	if PortraitWindow:
		PortraitWindow.Close ()
	PortraitButton.SetPicture(PortraitName)
	GenderButton.SetState (IE_GUI_BUTTON_DISABLED)
	RaceButton.SetState (IE_GUI_BUTTON_ENABLED)
	RaceButton.MakeDefault()
	CharGenState = 1
	Portrait = -1
	SetCharacterDescription()
	return

def CustomAbort():
	if CustomWindow:
		CustomWindow.Close ()
	return

def CGLargeCustomPortrait():
	Window = CustomWindow

	Portrait = PortraitList1.QueryText ()
	#small hack
	if GemRB.GetVar ("Row1") == RowCount1:
		return

	Label = Window.GetControl (0x10000007)
	Label.SetText (Portrait)

	Button = Window.GetControl (6)
	if Portrait=="":
		Portrait = "NOPORTMD"
		Button.SetState (IE_GUI_BUTTON_DISABLED)
	elif PortraitList2.QueryText () != "":
		Button.SetState (IE_GUI_BUTTON_ENABLED)

	Button = Window.GetControl (0)
	Button.SetPicture (Portrait, "NOPORTMD")
	return

def CGSmallCustomPortrait():
	Window = CustomWindow

	Portrait = PortraitList2.QueryText ()
	#small hack
	if GemRB.GetVar ("Row2") == RowCount2:
		return

	Label = Window.GetControl (0x10000008)
	Label.SetText (Portrait)

	Button = Window.GetControl (6)
	if Portrait=="":
		Portrait = "NOPORTSM"
		Button.SetState (IE_GUI_BUTTON_DISABLED)
	elif PortraitList1.QueryText () != "":
		Button.SetState (IE_GUI_BUTTON_ENABLED)

	Button = Window.GetControl (1)
	Button.SetPicture (Portrait, "NOPORTSM")
	return

def PortraitCustomPress():
	global PortraitList1, PortraitList2
	global RowCount1, RowCount2
	global CustomWindow

	CustomWindow = Window = GemRB.LoadWindow (18)
	PortraitList1 = Window.GetControl (2)
	RowCount1 = len(PortraitList1.ListResources (CHR_PORTRAITS, 1))
	PortraitList1.OnSelect (CGLargeCustomPortrait)
	PortraitList1.SetVarAssoc ("Row1",RowCount1)

	PortraitList2 = Window.GetControl (4)
	RowCount2 = len(PortraitList2.ListResources (CHR_PORTRAITS, 0))
	PortraitList2.OnSelect (CGSmallCustomPortrait)
	PortraitList2.SetVarAssoc ("Row2",RowCount2)

	Button = Window.GetControl (6)
	Button.SetText (11973)
	Button.OnPress (CustomDone)
	Button.SetState (IE_GUI_BUTTON_DISABLED)

	Button = Window.GetControl (7)
	Button.SetText (13727)
	Button.OnPress (CustomAbort)

	Button = Window.GetControl (0)
	PortraitName = PortraitsTable.GetRowName (Portrait)+"L"
	Button.SetPicture (PortraitName, "NOPORTLG")
	Button.SetState (IE_GUI_BUTTON_LOCKED)

	Button = Window.GetControl (1)
	PortraitName = PortraitsTable.GetRowName (Portrait)+"S"
	Button.SetPicture (PortraitName, "NOPORTSM")
	Button.SetState (IE_GUI_BUTTON_LOCKED)

	Window.ShowModal (MODAL_SHADOW_NONE)
	return

def CGPortraitDonePress():
	global CharGenWindow, PortraitWindow, PortraitButton, GenderButton, RaceButton
	global CharGenState, Portrait

	PortraitName = PortraitsTable.GetRowName (Portrait )
	GemRB.SetToken ("SmallPortrait", PortraitName+"S")
	GemRB.SetToken ("LargePortrait", PortraitName+"L")
	PortraitButton.SetPicture(PortraitsTable.GetRowName (Portrait) + "L")
	GenderButton.SetState (IE_GUI_BUTTON_DISABLED)
	RaceButton.SetState (IE_GUI_BUTTON_ENABLED)
	RaceButton.MakeDefault()
	CharGenState = 1
	SetCharacterDescription()
	if PortraitWindow:
		PortraitWindow.Close ()
	return

def CGPortraitCancelPress():
	global CharGenWindow, PortraitWindow

	if PortraitWindow:
		PortraitWindow.Close ()
	return

# Race Selection

def RacePress():
	global CharGenWindow, RaceWindow, RaceDoneButton, RaceTextArea

	RaceWindow = GemRB.LoadWindow (8)
	GemRB.SetVar ("Race", 0)

	for i in range (2, 8):
		RaceSelectButton = RaceWindow.GetControl (i)
		RaceSelectButton.SetFlags (IE_GUI_BUTTON_RADIOBUTTON, OP_OR)

	for i in range (2, 8):
		RaceSelectButton = RaceWindow.GetControl (i)
		RaceSelectButton.SetState (IE_GUI_BUTTON_ENABLED)
		RaceSelectButton.OnPress (RaceSelectPress)
		RaceSelectButton.SetText (CommonTables.Races.GetValue (i - 2, 0))
		RaceSelectButton.SetVarAssoc ("Race", i - 1)

	RaceTextArea = RaceWindow.GetControl (8)
	RaceTextArea.SetText (17237)

	RaceDoneButton = RaceWindow.GetControl (0)
	RaceDoneButton.SetState (IE_GUI_BUTTON_DISABLED)
	RaceDoneButton.OnPress (RaceDonePress)
	RaceDoneButton.SetText (11973)
	RaceDoneButton.MakeDefault()

	RaceCancelButton = RaceWindow.GetControl (10)
	RaceCancelButton.SetState (IE_GUI_BUTTON_ENABLED)
	RaceCancelButton.OnPress (RaceCancelPress)
	RaceCancelButton.SetText (13727)
	RaceCancelButton.MakeEscape()

	RaceWindow.ShowModal(MODAL_SHADOW_NONE)
	return

def RaceSelectPress():
	global RaceWindow, RaceDoneButton, RaceTextArea

	Race = GemRB.GetVar ("Race") - 1
	RaceTextArea.SetText (CommonTables.Races.GetValue (Race, 1) )
	RaceDoneButton.SetState (IE_GUI_BUTTON_ENABLED)
	return

def RaceDonePress():
	global CharGenWindow, CharGenState, RaceWindow, RaceButton, ClassButton

	if RaceWindow:
		RaceWindow.Close ()
	RaceButton.SetState (IE_GUI_BUTTON_DISABLED)
	ClassButton.SetState (IE_GUI_BUTTON_ENABLED)
	ClassButton.MakeDefault()
	CharGenState = 2

	Race = GemRB.GetVar ("Race")-1
	Race = CommonTables.Races.GetValue (Race, 3)
	GemRB.SetPlayerStat (MyChar, IE_RACE, Race)
	SetCharacterDescription()
	return

def RaceCancelPress():
	global CharGenWindow, RaceWindow

	if RaceWindow:
		RaceWindow.Close ()
	return

# Class Selection

def ClassPress():
	global CharGenWindow, ClassWindow, ClassTextArea, ClassDoneButton

	ClassWindow = GemRB.LoadWindow (2)
	ClassCount = CommonTables.Classes.GetRowCount ()
	RaceRow = CommonTables.Races.FindValue (3, GemRB.GetPlayerStat (MyChar, IE_RACE) )
	RaceName = CommonTables.Races.GetRowName (RaceRow)
	GemRB.SetVar ("Class", 0)
	GemRB.SetVar ("Class Kit", 0)
	GemRB.SetVar ("MAGESCHOOL", 0)

	for i in range (2, 10):
		ClassSelectButton = ClassWindow.GetControl (i)
		ClassSelectButton.SetFlags (IE_GUI_BUTTON_RADIOBUTTON, OP_SET)

	HasMulti = 0
	j = 2
	for i in range (ClassCount):
		ClassRowName = CommonTables.Classes.GetRowName (i)
		Allowed = CommonTables.Classes.GetValue (ClassRowName, RaceName)
		if CommonTables.Classes.GetValue (ClassRowName, "MULTI"):
			if Allowed != 0:
				HasMulti = 1
		else:
			ClassSelectButton = ClassWindow.GetControl (j)
			j = j + 1
			if Allowed == 1 or (Allowed == 2 and ClassRowName != "MAGE"):
				ClassSelectButton.SetState (IE_GUI_BUTTON_ENABLED)
			else:
				ClassSelectButton.SetState (IE_GUI_BUTTON_DISABLED)
			ClassSelectButton.OnPress (ClassSelectPress)
			ClassSelectButton.SetText (CommonTables.Classes.GetValue (ClassRowName, "NAME_REF"))
			ClassSelectButton.SetVarAssoc ("Class", i + 1)

	ClassMultiButton = ClassWindow.GetControl (10)
	if HasMulti == 0:
		ClassMultiButton.SetState (IE_GUI_BUTTON_DISABLED)
	else:
		ClassMultiButton.SetState (IE_GUI_BUTTON_ENABLED)
	ClassMultiButton.OnPress (ClassMultiPress)
	ClassMultiButton.SetText (11993)

	KitButton = ClassWindow.GetControl (11)
	#only the mage class has schools
	Allowed = CommonTables.Classes.GetValue ("MAGE", RaceName)
	if Allowed:
		KitButton.SetState (IE_GUI_BUTTON_ENABLED)
	else:
		KitButton.SetState (IE_GUI_BUTTON_DISABLED)
	KitButton.OnPress (KitPress)
	KitButton.SetText (11994)

	ClassTextArea = ClassWindow.GetControl (13)
	ClassTextArea.SetText (17242)

	ClassDoneButton = ClassWindow.GetControl (0)
	ClassDoneButton.SetState (IE_GUI_BUTTON_DISABLED)
	ClassDoneButton.OnPress (ClassDonePress)
	ClassDoneButton.SetText (11973)
	ClassDoneButton.MakeDefault()

	ClassCancelButton = ClassWindow.GetControl (14)
	ClassCancelButton.SetState (IE_GUI_BUTTON_ENABLED)
	ClassCancelButton.OnPress (ClassCancelPress)
	ClassCancelButton.SetText (13727)
	ClassCancelButton.MakeEscape()

	ClassWindow.ShowModal(MODAL_SHADOW_NONE)
	return

def ClassSelectPress():
	global ClassWindow, ClassTextArea, ClassDoneButton

	ClassName = GUICommon.GetClassRowName (GemRB.GetVar ("Class")-1, "index")
	ClassTextArea.SetText (CommonTables.Classes.GetValue (ClassName, "DESC_REF"))
	ClassDoneButton.SetState (IE_GUI_BUTTON_ENABLED)
	return

def ClassMultiPress():
	global ClassWindow, ClassMultiWindow, ClassMultiTextArea, ClassMultiDoneButton

	ClassWindow.SetVisible(False)
	ClassMultiWindow = GemRB.LoadWindow (10)
	ClassCount = CommonTables.Classes.GetRowCount ()
	RaceRow = CommonTables.Races.FindValue (3, GemRB.GetPlayerStat (MyChar, IE_RACE) )
	RaceName = CommonTables.Races.GetRowName (RaceRow)

	for i in range (2, 10):
		ClassMultiSelectButton = ClassMultiWindow.GetControl (i)
		ClassMultiSelectButton.SetFlags (IE_GUI_BUTTON_RADIOBUTTON, OP_SET)

	j = 2
	for i in range (ClassCount):
		ClassName = CommonTables.Classes.GetRowName (i)
		if CommonTables.Classes.GetValue (ClassName, "MULTI") > 0:
			ClassMultiSelectButton = ClassMultiWindow.GetControl (j)
			j = j + 1
			if CommonTables.Classes.GetValue (ClassName, RaceName) > 0:
				ClassMultiSelectButton.SetState (IE_GUI_BUTTON_ENABLED)
			else:
				ClassMultiSelectButton.SetState (IE_GUI_BUTTON_DISABLED)
			ClassMultiSelectButton.OnPress (ClassMultiSelectPress)
			ClassMultiSelectButton.SetText (CommonTables.Classes.GetValue (ClassName, "NAME_REF"))
			ClassMultiSelectButton.SetVarAssoc ("Class", i + 1)

	ClassMultiTextArea = ClassMultiWindow.GetControl (12)
	ClassMultiTextArea.SetText (17244)

	ClassMultiDoneButton = ClassMultiWindow.GetControl (0)
	ClassMultiDoneButton.SetState (IE_GUI_BUTTON_DISABLED)
	ClassMultiDoneButton.OnPress (ClassMultiDonePress)
	ClassMultiDoneButton.SetText (11973)
	ClassMultiDoneButton.MakeDefault()

	ClassMultiCancelButton = ClassMultiWindow.GetControl (14)
	ClassMultiCancelButton.SetState (IE_GUI_BUTTON_ENABLED)
	ClassMultiCancelButton.OnPress (ClassMultiCancelPress)
	ClassMultiCancelButton.SetText (13727)
	ClassMultiCancelButton.MakeEscape()

	ClassMultiWindow.ShowModal(MODAL_SHADOW_NONE)
	return

def ClassMultiSelectPress():
	global ClassMultiWindow, ClassMultiTextArea, ClassMultiDoneButton

	ClassName = GUICommon.GetClassRowName (GemRB.GetVar ("Class")-1, "index")
	ClassMultiTextArea.SetText (CommonTables.Classes.GetValue (ClassName, "DESC_REF"))
	ClassMultiDoneButton.SetState (IE_GUI_BUTTON_ENABLED)
	return

def ClassMultiDonePress():
	global ClassMultiWindow

	if ClassMultiWindow:
		ClassMultiWindow.Close ()
	ClassDonePress()
	return

def ClassMultiCancelPress():
	global ClassWindow, ClassMultiWindow

	if ClassMultiWindow:
		ClassMultiWindow.Close ()
	ClassWindow.ShowModal(MODAL_SHADOW_NONE)
	return

def KitPress():
	global ClassWindow, KitWindow, KitTextArea, KitDoneButton

	ClassWindow.SetVisible(False)
	KitWindow = GemRB.LoadWindow (12)

	#only mage class (1) has schools. It is the sixth button
	GemRB.SetVar ("Class", 6)
	GemRB.SetVar ("Class Kit",0)
	GemRB.SetVar ("MAGESCHOOL",0)

	# potentially exclude kits, eg. gnomes can only be illusionists
	RaceRow = CommonTables.Races.FindValue (3, GemRB.GetPlayerStat (MyChar, IE_RACE))
	RaceName = CommonTables.Races.GetRowName (RaceRow)
	Allowed = CommonTables.Classes.GetValue ("MAGE", RaceName)

	for i in range (8):
		Button = KitWindow.GetControl (i+2)
		Button.SetFlags (IE_GUI_BUTTON_RADIOBUTTON, OP_OR)
		Button.SetText (KitTable.GetValue (i+1, 0) )
		Button.SetVarAssoc ("MAGESCHOOL", i+1)
		Button.OnPress (KitSelectPress)
		if Allowed == 2 and i + 1 != 5: #illusionist
			Button.SetState (IE_GUI_BUTTON_DISABLED)
		else:
			Button.SetState (IE_GUI_BUTTON_ENABLED)

	KitTextArea = KitWindow.GetControl (11)
	KitTextArea.SetText (17245)

	KitDoneButton = KitWindow.GetControl (0)
	KitDoneButton.SetState (IE_GUI_BUTTON_DISABLED)
	KitDoneButton.OnPress (KitDonePress)
	KitDoneButton.SetText (11973)
	KitDoneButton.MakeDefault()

	KitCancelButton = KitWindow.GetControl (12)
	KitCancelButton.SetState (IE_GUI_BUTTON_ENABLED)
	KitCancelButton.OnPress (KitCancelPress)
	KitCancelButton.SetText (13727)
	KitCancelButton.MakeEscape()

	KitWindow.ShowModal(MODAL_SHADOW_NONE)
	return

def KitSelectPress():
	global KitWindow, KitTextArea

	Kit = GemRB.GetVar ("MAGESCHOOL")
	KitTextArea.SetText (KitTable.GetValue (Kit, 1))
	KitDoneButton.SetState (IE_GUI_BUTTON_ENABLED)
	return

def KitDonePress():
	global KitWindow

	if KitWindow:
		KitWindow.Close ()
	ClassDonePress()
	return

def KitCancelPress():
	global ClassWindow, KitWindow

	if KitWindow:
		KitWindow.Close ()
	ClassWindow.ShowModal(MODAL_SHADOW_NONE)
	return

def ClassDonePress():
	global CharGenWindow, CharGenState, ClassWindow, ClassButton, AlignmentButton
	global MyChar

	if ClassWindow:
		ClassWindow.Close ()
	ClassButton.SetState (IE_GUI_BUTTON_DISABLED)
	AlignmentButton.SetState (IE_GUI_BUTTON_ENABLED)
	AlignmentButton.MakeDefault()

	ClassName = GUICommon.GetClassRowName (GemRB.GetVar ("Class")-1, "index")
	Class = CommonTables.Classes.GetValue (ClassName, "ID")
	GemRB.SetPlayerStat (MyChar, IE_CLASS, Class)

	Kit = KitTable.GetValue (GemRB.GetVar ("MAGESCHOOL"), 3 )
	if (Kit == -1 ):
		Kit = 0x4000

	GemRB.SetPlayerStat (MyChar, IE_KIT, Kit)

	CharGenState = 3
	SetCharacterDescription()
	return

def ClassCancelPress():
	global CharGenWindow, ClassWindow

	if ClassWindow:
		ClassWindow.Close ()
	return

# Alignment Selection

def AlignmentPress():
	global CharGenWindow, AlignmentWindow, AlignmentTextArea, AlignmentDoneButton

	AlignmentWindow = GemRB.LoadWindow (3)
	ClassAlignmentTable = GemRB.LoadTable ("alignmnt")
	ClassName = GUICommon.GetClassRowName (MyChar)
	GemRB.SetVar ("Alignment", 0)

	for i in range (9):
		AlignmentSelectButton = AlignmentWindow.GetControl (i + 2)
		AlignmentSelectButton.SetFlags (IE_GUI_BUTTON_RADIOBUTTON, OP_OR)
		if ClassAlignmentTable.GetValue (ClassName, CommonTables.Aligns.GetValue(i, 4)) == 0:
			AlignmentSelectButton.SetState (IE_GUI_BUTTON_DISABLED)
		else:
			AlignmentSelectButton.SetState (IE_GUI_BUTTON_ENABLED)
		AlignmentSelectButton.OnPress (AlignmentSelectPress)
		AlignmentSelectButton.SetText (CommonTables.Aligns.GetValue (i, 0))
		AlignmentSelectButton.SetVarAssoc ("Alignment", i + 1)

	AlignmentTextArea = AlignmentWindow.GetControl (11)
	AlignmentTextArea.SetText (9602)

	AlignmentDoneButton = AlignmentWindow.GetControl (0)
	AlignmentDoneButton.SetState (IE_GUI_BUTTON_DISABLED)
	AlignmentDoneButton.OnPress (AlignmentDonePress)
	AlignmentDoneButton.SetText (11973)
	AlignmentDoneButton.MakeDefault()

	AlignmentCancelButton = AlignmentWindow.GetControl (13)
	AlignmentCancelButton.SetState (IE_GUI_BUTTON_ENABLED)
	AlignmentCancelButton.OnPress (AlignmentCancelPress)
	AlignmentCancelButton.SetText (13727)
	AlignmentCancelButton.MakeEscape()

	AlignmentWindow.ShowModal(MODAL_SHADOW_NONE)
	return

def AlignmentSelectPress():
	global AlignmentWindow, AlignmentTextArea, AlignmentDoneButton

	Alignment = GemRB.GetVar ("Alignment") - 1
	AlignmentTextArea.SetText (CommonTables.Aligns.GetValue (Alignment, 1))
	AlignmentDoneButton.SetState (IE_GUI_BUTTON_ENABLED)
	return

def AlignmentDonePress():
	global CharGenWindow, CharGenState, AlignmentWindow, AlignmentButton, AbilitiesButton
	global MyChar

	if AlignmentWindow:
		AlignmentWindow.Close ()
	AlignmentButton.SetState (IE_GUI_BUTTON_DISABLED)
	AbilitiesButton.SetState (IE_GUI_BUTTON_ENABLED)
	AbilitiesButton.MakeDefault()

	Alignment = CommonTables.Aligns.GetValue (GemRB.GetVar ("Alignment")-1, 3)
	GemRB.SetPlayerStat (MyChar, IE_ALIGNMENT, Alignment )

	CharGenState = 4
	SetCharacterDescription()
	return

def AlignmentCancelPress():
	global CharGenWindow, AlignmentWindow

	if AlignmentWindow:
		AlignmentWindow.Close ()
	return

# Abilities Selection

def AbilitiesPress():
	global CharGenWindow, AbilitiesWindow
	global AbilitiesTextArea, AbilitiesRecallButton, AbilitiesDoneButton
	global AbilitiesRaceAddTable, AbilitiesRaceReqTable, AbilitiesClassReqTable
	global HasStrExtra

	AbilitiesWindow = GemRB.LoadWindow (4)
	AbilitiesRaceAddTable = GemRB.LoadTable ("ABRACEAD")
	AbilitiesRaceReqTable = GemRB.LoadTable ("ABRACERQ")
	AbilitiesClassReqTable = GemRB.LoadTable ("ABCLASRQ")

	ClassName = GUICommon.GetClassRowName (MyChar)
	HasStrExtra = CommonTables.Classes.GetValue (ClassName, "STREXTRA", GTV_INT)

	for i in range (6):
		AbilitiesLabelButton = AbilitiesWindow.GetControl (30 + i)
		AbilitiesLabelButton.SetState (IE_GUI_BUTTON_ENABLED)
		AbilitiesLabelButton.OnPress (AbilitiesLabelPress)
		AbilitiesLabelButton.SetVarAssoc ("AbilityIndex", i + 1)

		AbilitiesPlusButton = AbilitiesWindow.GetControl (16 + i * 2)
		AbilitiesPlusButton.SetState (IE_GUI_BUTTON_ENABLED)
		AbilitiesPlusButton.OnPress (AbilitiesPlusPress)
		AbilitiesPlusButton.SetVarAssoc ("AbilityIndex", i + 1)
		AbilitiesPlusButton.SetActionInterval (200)

		AbilitiesMinusButton = AbilitiesWindow.GetControl (17 + i * 2)
		AbilitiesMinusButton.SetState (IE_GUI_BUTTON_ENABLED)
		AbilitiesMinusButton.OnPress (AbilitiesMinusPress)
		AbilitiesMinusButton.SetVarAssoc ("AbilityIndex", i + 1)
		AbilitiesMinusButton.SetActionInterval (200)

	AbilitiesStoreButton = AbilitiesWindow.GetControl (37)
	AbilitiesStoreButton.SetState (IE_GUI_BUTTON_ENABLED)
	AbilitiesStoreButton.OnPress (AbilitiesStorePress)
	AbilitiesStoreButton.SetText (17373)

	AbilitiesRecallButton = AbilitiesWindow.GetControl (38)
	AbilitiesRecallButton.SetState (IE_GUI_BUTTON_DISABLED)
	AbilitiesRecallButton.OnPress (AbilitiesRecallPress)
	AbilitiesRecallButton.SetText (17374)

	AbilitiesRerollButton = AbilitiesWindow.GetControl (2)
	AbilitiesRerollButton.SetState (IE_GUI_BUTTON_ENABLED)
	AbilitiesRerollButton.OnPress (AbilitiesRerollPress)
	AbilitiesRerollButton.SetText (11982)

	AbilitiesTextArea = AbilitiesWindow.GetControl (29)
	AbilitiesTextArea.SetText (17247)

	AbilitiesDoneButton = AbilitiesWindow.GetControl (0)
	AbilitiesDoneButton.SetState (IE_GUI_BUTTON_ENABLED)
	AbilitiesDoneButton.OnPress (AbilitiesDonePress)
	AbilitiesDoneButton.SetText (11973)
	AbilitiesDoneButton.MakeDefault()

	AbilitiesCancelButton = AbilitiesWindow.GetControl (36)
	AbilitiesCancelButton.SetState (IE_GUI_BUTTON_ENABLED)
	AbilitiesCancelButton.OnPress (AbilitiesCancelPress)
	AbilitiesCancelButton.SetText (13727)
	AbilitiesCancelButton.MakeEscape()

	AbilitiesRerollPress()

	AbilitiesWindow.ShowModal(MODAL_SHADOW_NONE)
	return

def AbilitiesCalcLimits(Index):
	global AbilitiesRaceReqTable, AbilitiesRaceAddTable, AbilitiesClassReqTable
	global AbilitiesMinimum, AbilitiesMaximum, AbilitiesModifier

	Race = CommonTables.Races.FindValue (3, GemRB.GetPlayerStat (MyChar, IE_RACE))
	RaceName = CommonTables.Races.GetRowName (Race)
	Race = AbilitiesRaceReqTable.GetRowIndex (RaceName)
	AbilitiesMinimum = AbilitiesRaceReqTable.GetValue (Race, Index * 2)
	AbilitiesMaximum = AbilitiesRaceReqTable.GetValue (Race, Index * 2 + 1)
	AbilitiesModifier = AbilitiesRaceAddTable.GetValue (Race, Index)

	ClassName = GUICommon.GetClassRowName (MyChar)
	ClassIndex = AbilitiesClassReqTable.GetRowIndex (ClassName)
	Min = AbilitiesClassReqTable.GetValue (ClassIndex, Index)
	if Min > 0 and AbilitiesMinimum < Min:
		AbilitiesMinimum = Min

	AbilitiesMinimum = AbilitiesMinimum + AbilitiesModifier
	AbilitiesMaximum = AbilitiesMaximum + AbilitiesModifier
	return

def AbilitiesLabelPress():
	global AbilitiesWindow, AbilitiesTextArea

	AbilityIndex = GemRB.GetVar ("AbilityIndex") - 1
	AbilitiesCalcLimits(AbilityIndex)
	GemRB.SetToken ("MINIMUM", str(AbilitiesMinimum) )
	GemRB.SetToken ("MAXIMUM", str(AbilitiesMaximum) )
	AbilitiesTextArea.SetText (AbilitiesTable.GetValue (AbilityIndex, 1) )
	return

def AbilitiesPlusPress():
	global AbilitiesWindow, AbilitiesTextArea
	global AbilitiesMinimum, AbilitiesMaximum

	Abidx = GemRB.GetVar ("AbilityIndex") - 1
	AbilitiesCalcLimits(Abidx)
	GemRB.SetToken ("MINIMUM", str(AbilitiesMinimum) )
	GemRB.SetToken ("MAXIMUM", str(AbilitiesMaximum) )
	AbilitiesTextArea.SetText (AbilitiesTable.GetValue (Abidx, 1) )
	PointsLeft = GemRB.GetVar ("Ability0")
	Ability = GemRB.GetVar ("Ability" + str(Abidx + 1) )
	if PointsLeft > 0 and Ability < AbilitiesMaximum:
		PointsLeft = PointsLeft - 1
		GemRB.SetVar ("Ability0", PointsLeft)
		PointsLeftLabel = AbilitiesWindow.GetControl (0x10000002)
		PointsLeftLabel.SetText (str(PointsLeft) )
		Ability = Ability + 1
		GemRB.SetVar ("Ability" + str(Abidx + 1), Ability)
		Label = AbilitiesWindow.GetControl (0x10000003 + Abidx)
		StrExtra = GemRB.GetVar("StrExtra")
		if Abidx==0 and Ability==18 and HasStrExtra:
			Label.SetText("18/"+str(StrExtra) )
		else:
			Label.SetText(str(Ability) )

		if PointsLeft == 0:
			AbilitiesDoneButton.SetState (IE_GUI_BUTTON_ENABLED)
	return

def AbilitiesMinusPress():
	global AbilitiesWindow, AbilitiesTextArea
	global AbilitiesMinimum, AbilitiesMaximum

	Abidx = GemRB.GetVar ("AbilityIndex") - 1
	AbilitiesCalcLimits(Abidx)
	GemRB.SetToken ("MINIMUM", str(AbilitiesMinimum) )
	GemRB.SetToken ("MAXIMUM", str(AbilitiesMaximum) )
	AbilitiesTextArea.SetText (AbilitiesTable.GetValue (Abidx, 1) )
	PointsLeft = GemRB.GetVar ("Ability0")
	Ability = GemRB.GetVar ("Ability" + str(Abidx + 1) )
	if Ability > AbilitiesMinimum:
		Ability = Ability - 1
		GemRB.SetVar ("Ability" + str(Abidx + 1), Ability)
		Label = AbilitiesWindow.GetControl (0x10000003 + Abidx)
		StrExtra = GemRB.GetVar("StrExtra")
		if Abidx==0 and Ability==18 and HasStrExtra:
			Label.SetText("18/"+str(StrExtra) )
		else:
			Label.SetText(str(Ability) )

		PointsLeft = PointsLeft + 1
		GemRB.SetVar ("Ability0", PointsLeft)
		PointsLeftLabel = AbilitiesWindow.GetControl (0x10000002)
		PointsLeftLabel.SetText (str(PointsLeft) )
		AbilitiesDoneButton.SetState (IE_GUI_BUTTON_DISABLED)
	return

def AbilitiesStorePress():
	global AbilitiesWindow, AbilitiesRecallButton

	GemRB.SetVar("StoredStrExtra", GemRB.GetVar ("StrExtra") )
	for i in range (7):
		GemRB.SetVar ("Stored" + str(i), GemRB.GetVar ("Ability" + str(i)))
	AbilitiesRecallButton.SetState (IE_GUI_BUTTON_ENABLED)
	return

def AbilitiesRecallPress():
	global AbilitiesWindow

	e=GemRB.GetVar("StoredStrExtra")
	GemRB.SetVar("StrExtra",e)
	for i in range (7):
		v =  GemRB.GetVar ("Stored" + str(i))
		GemRB.SetVar ("Ability" + str(i), v)
		Label = AbilitiesWindow.GetControl (0x10000002 + i)
		if i==0 and v==18 and HasStrExtra==1:
			Label.SetText("18/"+str(e) )
		else:
			Label.SetText(str(v) )

	PointsLeft = GemRB.GetVar("Ability0")
	if PointsLeft == 0:
		AbilitiesDoneButton.SetState(IE_GUI_BUTTON_ENABLED)
	else:
		AbilitiesDoneButton.SetState(IE_GUI_BUTTON_DISABLED)
	return

def AbilitiesRerollPress():
	global AbilitiesWindow, AbilitiesMinimum, AbilitiesMaximum, AbilitiesModifier

	GemRB.SetVar ("Ability0", 0)
	PointsLeftLabel = AbilitiesWindow.GetControl (0x10000002)
	PointsLeftLabel.SetText ("0")
	Dices = 3
	Sides = 5

	#roll strextra even when the current stat is not 18
	if HasStrExtra:
		e = GemRB.Roll (1,100,0)
	else:
		e = 0
	GemRB.SetVar("StrExtra", e)
	for i in range (6):
		AbilitiesCalcLimits(i)
		Value = GemRB.Roll (Dices, Sides, AbilitiesModifier+3)
		if Value < AbilitiesMinimum:
			Value = AbilitiesMinimum
		if Value > AbilitiesMaximum:
			Value = AbilitiesMaximum
		GemRB.SetVar ("Ability" + str(i + 1), Value)
		Label = AbilitiesWindow.GetControl (0x10000003 + i)

		if i==0 and HasStrExtra and Value==18:
			Label.SetText("18/"+str(e) )
		else:
			Label.SetText(str(Value) )

	AbilitiesDoneButton.SetState(IE_GUI_BUTTON_ENABLED)
	return

def AbilitiesDonePress():
	global CharGenWindow, CharGenState, AbilitiesWindow, AbilitiesButton, SkillsButton, SkillsState

	if AbilitiesWindow:
		AbilitiesWindow.Close ()
	AbilitiesButton.SetState (IE_GUI_BUTTON_DISABLED)
	SkillsButton.SetState (IE_GUI_BUTTON_ENABLED)
	SkillsButton.MakeDefault()

	Str = GemRB.GetVar ("Ability1")
	GemRB.SetPlayerStat (MyChar, IE_STR, Str)
	if Str == 18:
		GemRB.SetPlayerStat (MyChar, IE_STREXTRA, GemRB.GetVar ("StrExtra"))
	else:
		GemRB.SetPlayerStat (MyChar, IE_STREXTRA, 0)

	GemRB.SetPlayerStat (MyChar, IE_DEX, GemRB.GetVar ("Ability2"))
	GemRB.SetPlayerStat (MyChar, IE_CON, GemRB.GetVar ("Ability3"))
	GemRB.SetPlayerStat (MyChar, IE_INT, GemRB.GetVar ("Ability4"))
	GemRB.SetPlayerStat (MyChar, IE_WIS, GemRB.GetVar ("Ability5"))
	GemRB.SetPlayerStat (MyChar, IE_CHR, GemRB.GetVar ("Ability6"))

	CharGenState = 5
	SkillsState = 0
	SetCharacterDescription()
	return

def AbilitiesCancelPress():
	global CharGenWindow, AbilitiesWindow

	if AbilitiesWindow:
		AbilitiesWindow.Close ()
	return

# Skills Selection

def SkillsPress():
	global CharGenWindow, AppearanceButton
	global SkillsState, SkillsButton, CharGenState, ClassFlag

	Level = 1
	SpellLevel = 1
	ClassName = GUICommon.GetClassRowName (MyChar)
	DruidSpell = CommonTables.ClassSkills.GetValue (ClassName, "DRUIDSPELL")
	PriestSpell = CommonTables.ClassSkills.GetValue (ClassName, "CLERICSPELL")
	MageSpell = CommonTables.ClassSkills.GetValue (ClassName, "MAGESPELL")
	IsBard = CommonTables.ClassSkills.GetValue (ClassName, "BARDSKILL")
	IsThief = CommonTables.ClassSkills.GetValue (ClassName, "THIEFSKILL")

	if SkillsState == 0:
		LUSkillsSelection.SkillsNullify (MyChar)
		HateRace = CommonTables.ClassSkills.GetValue(ClassName, "HATERACE")
		if DruidSpell != "*" and HateRace != "*":
			Skill = GemRB.LoadTable("SKILLRNG").GetValue(str(Level), "STEALTH")
			GemRB.SetPlayerStat (MyChar, IE_STEALTH, Skill)
		elif IsBard != "*":
			Skill = GemRB.LoadTable(IsBard).GetValue(str(Level), "PICK_POCKETS")
			GemRB.SetPlayerStat (MyChar, IE_PICKPOCKET, Skill)

		GemRB.SetVar ("HatedRace", 0)
		GemRB.SetVar ("RacialEnemy", 0)
		GemRB.SetVar ("RacialEnemyIndex", 0)
		if HateRace != "*":
			RacialEnemySelect()
		elif IsThief != "*":
			SkillsSelect()
		else:
			SkillsState = 1

	if SkillsState == 1:
		ProficienciesSelect()

	if SkillsState == 2:
		if MageSpell!="*":
			MageSpellsSelect(MageSpell, Level, SpellLevel)
		else:
			SkillsState = 3

	if SkillsState == 3:
		if MageSpell!="*":
			MageSpellsMemorize(MageSpell, Level, SpellLevel)
		else:
			SkillsState = 4

	if SkillsState == 4:
		if PriestSpell == "MXSPLPRS":
			ClassFlag = 0x4000
			PriestSpellsMemorize(PriestSpell, Level, SpellLevel)
		elif DruidSpell == "MXSPLDRU":
			# no separate spell progression by default
			DruidSpell = Spellbook.GetPriestSpellTable (DruidSpell)
			ClassFlag = 0x8000
			PriestSpellsMemorize(DruidSpell, Level, SpellLevel)
		else:
			SkillsState = 5

	if SkillsState == 5:
		SkillsButton.SetState (IE_GUI_BUTTON_DISABLED)
		AppearanceButton.SetState (IE_GUI_BUTTON_ENABLED)
		AppearanceButton.MakeDefault()

		Race = GemRB.GetVar ("HatedRace")
		GemRB.SetPlayerStat (MyChar, IE_HATEDRACE, Race)

		ProfCount = ProficienciesTable.GetRowCount ()
		for i in range(ProfCount):
			StatID = ProficienciesTable.GetValue (i, 0)
			Value = GemRB.GetVar ("Proficiency"+str(i) )
			GemRB.SetPlayerStat (MyChar, StatID, Value )

		CharGenState = 6
		SetCharacterDescription()
	return

def SkillsSelect():
	global CharGenWindow, SkillsWindow, SkillsTextArea, SkillsDoneButton, SkillsPointsLeft

	SkillsWindow = GemRB.LoadWindow (6)

	Levels = [GemRB.GetPlayerStat (MyChar, IE_LEVEL),
		GemRB.GetPlayerStat (MyChar, IE_LEVEL2),
		GemRB.GetPlayerStat (MyChar, IE_LEVEL3)]

	LUSkillsSelection.SetupSkillsWindow (MyChar,
		LUSkillsSelection.LUSKILLS_TYPE_CHARGEN, SkillsWindow, RedrawSkills, [0,0,0], Levels, 0, False)

	SkillsPointsLeft = GemRB.GetVar ("SkillPointsLeft")
	if SkillsPointsLeft<=0:
		SkillsDonePress()
		return

	SkillsDoneButton = SkillsWindow.GetControl (0)
	SkillsDoneButton.SetState (IE_GUI_BUTTON_DISABLED)
	SkillsDoneButton.OnPress (SkillsDonePress)
	SkillsDoneButton.SetText (11973)
	SkillsDoneButton.MakeDefault()

	SkillsCancelButton = SkillsWindow.GetControl (25)
	SkillsCancelButton.SetState (IE_GUI_BUTTON_ENABLED)
	SkillsCancelButton.OnPress (SkillsCancelPress)
	SkillsCancelButton.SetText (13727)
	SkillsCancelButton.MakeEscape()

	RedrawSkills()
	SkillsWindow.ShowModal(MODAL_SHADOW_NONE)
	return

def RedrawSkills():
	PointsLeft = GemRB.GetVar ("SkillPointsLeft")
	if PointsLeft == 0:
		SkillsDoneButton.SetState(IE_GUI_BUTTON_ENABLED)
	else:
		SkillsDoneButton.SetState(IE_GUI_BUTTON_DISABLED)
	return

def SkillsDonePress():
	global CharGenWindow, SkillsWindow, SkillsState

	# save all the skills
	LUSkillsSelection.SkillsSave (MyChar)

	if SkillsWindow:
		SkillsWindow.Close ()
	SkillsState = 1
	SkillsPress()
	return

def SkillsCancelPress():
	global CharGenWindow, SkillsWindow, SkillsState

	if SkillsWindow:
		SkillsWindow.Close ()
	SkillsState = 0
	return

# Racial Enemy Selection

def RacialEnemySelect():
	global CharGenWindow, RacialEnemyWindow, RacialEnemyTextArea, RacialEnemyDoneButton

	RacialEnemyWindow = GemRB.LoadWindow (15)
	RacialEnemyCount = RacialEnemyTable.GetRowCount ()

	for i in range (2, 8):
		RacialEnemySelectButton = RacialEnemyWindow.GetControl (i)
		RacialEnemySelectButton.SetFlags (IE_GUI_BUTTON_RADIOBUTTON, OP_OR)

	for i in range (2, 8):
		RacialEnemySelectButton = RacialEnemyWindow.GetControl (i)
		RacialEnemySelectButton.SetState (IE_GUI_BUTTON_ENABLED)
		RacialEnemySelectButton.OnPress (RacialEnemySelectPress)
		RacialEnemySelectButton.SetVarAssoc ("RacialEnemy", i - 1)

	GemRB.SetVar ("RacialEnemyIndex", 0)
	GemRB.SetVar ("HatedRace", 0)
	RacialEnemyScrollBar = RacialEnemyWindow.GetControl (1)
	RacialEnemyScrollBar.SetVarAssoc ("RacialEnemyIndex", RacialEnemyCount - 6)
	RacialEnemyScrollBar.OnChange (DisplayRacialEnemies)

	RacialEnemyTextArea = RacialEnemyWindow.GetControl (8)
	RacialEnemyTextArea.SetText (17256)

	RacialEnemyDoneButton = RacialEnemyWindow.GetControl (11)
	RacialEnemyDoneButton.SetState (IE_GUI_BUTTON_DISABLED)
	RacialEnemyDoneButton.OnPress (RacialEnemyDonePress)
	RacialEnemyDoneButton.SetText (11973)
	RacialEnemyDoneButton.MakeDefault()

	RacialEnemyCancelButton = RacialEnemyWindow.GetControl (10)
	RacialEnemyCancelButton.SetState (IE_GUI_BUTTON_ENABLED)
	RacialEnemyCancelButton.OnPress (RacialEnemyCancelPress)
	RacialEnemyCancelButton.SetText (13727)
	RacialEnemyCancelButton.MakeEscape()

	DisplayRacialEnemies()
	RacialEnemyWindow.ShowModal(MODAL_SHADOW_NONE)
	return

def DisplayRacialEnemies():
	global RacialEnemyWindow

	RacialEnemyIndex = GemRB.GetVar ("RacialEnemyIndex")
	for i in range (2, 8):
		RacialEnemySelectButton = RacialEnemyWindow.GetControl (i)
		RacialEnemySelectButton.SetText (RacialEnemyTable.GetValue (RacialEnemyIndex + i - 2, 0))
	return

def RacialEnemySelectPress():
	global RacialEnemyWindow, RacialEnemyDoneButton, RacialEnemyTextArea

	RacialEnemy = GemRB.GetVar ("RacialEnemyIndex") + GemRB.GetVar ("RacialEnemy") - 1
	RacialEnemyTextArea.SetText (RacialEnemyTable.GetValue (RacialEnemy, 2) )
	GemRB.SetVar ("HatedRace", RacialEnemyTable.GetValue (RacialEnemy, 1) )
	RacialEnemyDoneButton.SetState (IE_GUI_BUTTON_ENABLED)
	return

def RacialEnemyDonePress():
	global CharGenWindow, RacialEnemyWindow, SkillsState

	if RacialEnemyWindow:
		RacialEnemyWindow.Close ()

	SkillsState = 1
	SkillsPress()
	return

def RacialEnemyCancelPress():
	global CharGenWindow, RacialEnemyWindow, SkillsState

	if RacialEnemyWindow:
		RacialEnemyWindow.Close ()
	SkillsState = 0
	return


# Weapon Proficiencies Selection

def ProficienciesSelect():
	global CharGenWindow, ProficienciesWindow, ProficienciesTextArea
	global ProficienciesPointsLeft, ProficienciesDoneButton, ProfsMaxTable

	ProficienciesWindow = GemRB.LoadWindow (9)
	ProfsTable = GemRB.LoadTable ("profs")
	ProfsMaxTable = GemRB.LoadTable ("profsmax")
	ClassWeaponsTable = GemRB.LoadTable ("clasweap")

	# remove all known spells and nullify the memorizable counts
	Spellbook.RemoveKnownSpells (MyChar, IE_SPELL_TYPE_WIZARD, 1,9, 1)
	Spellbook.RemoveKnownSpells (MyChar, IE_SPELL_TYPE_PRIEST, 1,7, 1)
	GemRB.SetVar ("MageMemorized", 0)
	GemRB.SetVar ("MageSpellBook", 0)

	ClassName = GUICommon.GetClassRowName (MyChar)
	ProficienciesPointsLeft = ProfsTable.GetValue (ClassName, "FIRST_LEVEL")
	PointsLeftLabel = ProficienciesWindow.GetControl (0x10000009)
	PointsLeftLabel.SetText (str(ProficienciesPointsLeft))

	for i in range (8):
		ProficienciesLabel = ProficienciesWindow.GetControl (69 + i)
		ProficienciesLabel.SetState (IE_GUI_BUTTON_ENABLED)
		ProficienciesLabel.OnPress (ProficienciesLabelPress)
		ProficienciesLabel.SetVarAssoc ("ProficienciesIndex", i + 1)

		for j in range (5):
			ProficienciesMark = ProficienciesWindow.GetControl (27 + i * 5 + j)
			ProficienciesMark.SetSprites("GUIPFC", 0, 0, 0, 0, 0)
			ProficienciesMark.SetState (IE_GUI_BUTTON_DISABLED)
			ProficienciesMark.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_OR)

		Allowed = ClassWeaponsTable.GetValue (ClassName, ProficienciesTable.GetRowName (i))

		ProficienciesPlusButton = ProficienciesWindow.GetControl (11 + i * 2)
		if Allowed == 0:
			ProficienciesPlusButton.SetState (IE_GUI_BUTTON_DISABLED)
			ProficienciesPlusButton.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_OR)
		else:
			ProficienciesPlusButton.SetState (IE_GUI_BUTTON_ENABLED)
		ProficienciesPlusButton.OnPress (ProficienciesPlusPress)
		ProficienciesPlusButton.SetVarAssoc ("ProficienciesIndex", i + 1)
		ProficienciesPlusButton.SetActionInterval (200)

		ProficienciesMinusButton = ProficienciesWindow.GetControl (12 + i * 2)
		if Allowed == 0:
			ProficienciesMinusButton.SetState (IE_GUI_BUTTON_DISABLED)
			ProficienciesMinusButton.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_OR)
		else:
			ProficienciesMinusButton.SetState (IE_GUI_BUTTON_ENABLED)
		ProficienciesMinusButton.OnPress (ProficienciesMinusPress)
		ProficienciesMinusButton.SetVarAssoc ("ProficienciesIndex", i + 1)
		ProficienciesMinusButton.SetActionInterval (200)

	for i in range (7):
		ProficienciesLabel = ProficienciesWindow.GetControl (85 + i)
		ProficienciesLabel.SetState (IE_GUI_BUTTON_ENABLED)
		ProficienciesLabel.OnPress (ProficienciesLabelPress)
		ProficienciesLabel.SetVarAssoc ("ProficienciesIndex", i + 9)

		for j in range (5):
			ProficienciesMark = ProficienciesWindow.GetControl (92 + i * 5 + j)
			ProficienciesMark.SetSprites("GUIPFC", 0, 0, 0, 0, 0)
			ProficienciesMark.SetState (IE_GUI_BUTTON_DISABLED)
			ProficienciesMark.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_OR)

		Allowed = ClassWeaponsTable.GetValue (ClassName, ProficienciesTable.GetRowName (i + 8))

		ProficienciesPlusButton = ProficienciesWindow.GetControl (127 + i * 2)
		if Allowed == 0:
			ProficienciesPlusButton.SetState (IE_GUI_BUTTON_DISABLED)
			ProficienciesPlusButton.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_OR)
		else:
			ProficienciesPlusButton.SetState (IE_GUI_BUTTON_ENABLED)
		ProficienciesPlusButton.OnPress (ProficienciesPlusPress)
		ProficienciesPlusButton.SetVarAssoc ("ProficienciesIndex", i + 9)
		ProficienciesPlusButton.SetActionInterval (200)

		ProficienciesMinusButton = ProficienciesWindow.GetControl (128 + i * 2)
		if Allowed == 0:
			ProficienciesMinusButton.SetState (IE_GUI_BUTTON_DISABLED)
			ProficienciesMinusButton.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_OR)
		else:
			ProficienciesMinusButton.SetState (IE_GUI_BUTTON_ENABLED)
		ProficienciesMinusButton.OnPress (ProficienciesMinusPress)
		ProficienciesMinusButton.SetVarAssoc ("ProficienciesIndex", i + 9)
		ProficienciesMinusButton.SetActionInterval (200)

	for i in range (15):
		GemRB.SetVar ("Proficiency" + str(i), 0)

	GemRB.SetToken ("number", str(ProficienciesPointsLeft) )
	ProficienciesTextArea = ProficienciesWindow.GetControl (68)
	ProficienciesTextArea.SetText (9588)

	ProficienciesDoneButton = ProficienciesWindow.GetControl (0)
	ProficienciesDoneButton.SetState (IE_GUI_BUTTON_DISABLED)
	ProficienciesDoneButton.OnPress (ProficienciesDonePress)
	ProficienciesDoneButton.SetText (11973)
	ProficienciesDoneButton.MakeDefault()

	ProficienciesCancelButton = ProficienciesWindow.GetControl (77)
	ProficienciesCancelButton.SetState (IE_GUI_BUTTON_ENABLED)
	ProficienciesCancelButton.OnPress (ProficienciesCancelPress)
	ProficienciesCancelButton.SetText (13727)
	ProficienciesCancelButton.MakeEscape()

	ProficienciesWindow.ShowModal(MODAL_SHADOW_NONE)
	return

def ProficienciesLabelPress():
	global ProficienciesWindow, ProficienciesTextArea

	ProficienciesIndex = GemRB.GetVar ("ProficienciesIndex") - 1
	ProficienciesTextArea.SetText (ProficienciesTable.GetValue (ProficienciesIndex, 2) )
	return

def ProficienciesPlusPress():
	global ProficienciesWindow, ProficienciesTextArea
	global ProficienciesPointsLeft, ProfsMaxTable

	ProficienciesIndex = GemRB.GetVar ("ProficienciesIndex") - 1
	ProficienciesValue = GemRB.GetVar ("Proficiency" + str(ProficienciesIndex) )
	ClassName = GUICommon.GetClassRowName (MyChar)
	if ProficienciesPointsLeft > 0 and ProficienciesValue < ProfsMaxTable.GetValue (ClassName, "FIRST_LEVEL"):
		ProficienciesPointsLeft = ProficienciesPointsLeft - 1
		PointsLeftLabel = ProficienciesWindow.GetControl (0x10000009)
		PointsLeftLabel.SetText (str(ProficienciesPointsLeft))
		if ProficienciesPointsLeft == 0:
			ProficienciesDoneButton.SetState (IE_GUI_BUTTON_ENABLED)

		ProficienciesValue = ProficienciesValue + 1
		GemRB.SetVar ("Proficiency" + str(ProficienciesIndex), ProficienciesValue)
		if ProficienciesIndex < 8:
			ControlID = 26 + ProficienciesIndex * 5 + ProficienciesValue
		else:
			ControlID = 51 + ProficienciesIndex * 5 + ProficienciesValue
		ProficienciesMark = ProficienciesWindow.GetControl (ControlID)
		ProficienciesMark.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_NAND)

	ProficienciesTextArea.SetText (ProficienciesTable.GetValue (ProficienciesIndex, 2) )
	return

def ProficienciesMinusPress():
	global ProficienciesWindow, ProficienciesTextArea, ProficienciesPointsLeft

	ProficienciesIndex = GemRB.GetVar ("ProficienciesIndex") - 1
	ProficienciesValue = GemRB.GetVar ("Proficiency" + str(ProficienciesIndex) )
	if ProficienciesValue > 0:
		ProficienciesValue = ProficienciesValue - 1
		GemRB.SetVar ("Proficiency" + str(ProficienciesIndex), ProficienciesValue)
		if ProficienciesIndex < 8:
			ControlID = 27 + ProficienciesIndex * 5 + ProficienciesValue
		else:
			ControlID = 52 + ProficienciesIndex * 5 + ProficienciesValue
		ProficienciesMark = ProficienciesWindow.GetControl (ControlID)
		ProficienciesMark.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_OR)

		ProficienciesPointsLeft = ProficienciesPointsLeft + 1
		PointsLeftLabel = ProficienciesWindow.GetControl (0x10000009)
		PointsLeftLabel.SetText (str(ProficienciesPointsLeft))
		ProficienciesDoneButton.SetState (IE_GUI_BUTTON_DISABLED)

	ProficienciesTextArea.SetText (ProficienciesTable.GetValue (ProficienciesIndex, 2) )
	return

def ProficienciesDonePress():
	global CharGenWindow, ProficienciesWindow, SkillsState

	if ProficienciesWindow:
		ProficienciesWindow.Close ()
	SkillsState = 2
	SkillsPress()
	return

def ProficienciesCancelPress():
	global CharGenWindow, ProficienciesWindow, SkillsState

	if ProficienciesWindow:
		ProficienciesWindow.Close ()
	SkillsState = 0
	return

# Spells Selection

def MageSpellsSelect(SpellTable, Level, SpellLevel):
	global CharGenWindow, MageSpellsWindow, MageSpellsTextArea, MageSpellsDoneButton, MageSpellsSelectPointsLeft, Learnable

	MageSpellsWindow = GemRB.LoadWindow (7)
	#kit (school), alignment, level
	k = GemRB.GetPlayerStat (MyChar, IE_KIT)
	t = GemRB.GetPlayerStat (MyChar, IE_ALIGNMENT)
	Learnable = Spellbook.GetLearnableMageSpells(k, t, SpellLevel)
	GemRB.SetVar ("MageSpellBook", 0)
	GemRB.SetVar ("SpellMask", 0)

	if len(Learnable) < 1 or GemRB.GetPlayerStat (MyChar, IE_CLASS) == 5: # no bards
		MageSpellsDonePress()
		return

	if k & ~0x4000 > 0:
		MageSpellsSelectPointsLeft = 3
	else:
		MageSpellsSelectPointsLeft = 2
	PointsLeftLabel = MageSpellsWindow.GetControl (0x1000001b)
	PointsLeftLabel.SetText (str(MageSpellsSelectPointsLeft))

	for i in range (24):
		SpellButton = MageSpellsWindow.GetControl (i + 2)
		SpellButton.SetFlags (IE_GUI_BUTTON_PICTURE|IE_GUI_BUTTON_CHECKBOX, OP_OR)
		if i < len(Learnable):
			Spell = GemRB.GetSpell (Learnable[i])
			SpellButton.SetSpellIcon(Learnable[i], 1)
			SpellButton.SetState (IE_GUI_BUTTON_ENABLED)
			SpellButton.OnPress (MageSpellsSelectPress)
			SpellButton.SetVarAssoc ("SpellMask", 1 << i)
			SpellButton.SetTooltip(Spell["SpellName"])
		else:
			SpellButton.SetState (IE_GUI_BUTTON_DISABLED)

	GemRB.SetToken ("number", str(MageSpellsSelectPointsLeft))
	MageSpellsTextArea = MageSpellsWindow.GetControl (27)
	MageSpellsTextArea.SetText (17250)

	MageSpellsDoneButton = MageSpellsWindow.GetControl (0)
	MageSpellsDoneButton.SetState (IE_GUI_BUTTON_DISABLED)
	MageSpellsDoneButton.OnPress (MageSpellsDonePress)
	MageSpellsDoneButton.SetText (11973)
	MageSpellsDoneButton.MakeDefault()

	MageSpellsCancelButton = MageSpellsWindow.GetControl (29)
	MageSpellsCancelButton.SetState (IE_GUI_BUTTON_ENABLED)
	MageSpellsCancelButton.OnPress (MageSpellsCancelPress)
	MageSpellsCancelButton.SetText (13727)
	MageSpellsCancelButton.MakeEscape()

	MageSpellsWindow.ShowModal(MODAL_SHADOW_NONE)
	return

def MageSpellsSelectPress():
	global MageSpellsWindow, MageSpellsTextArea, MageSpellsDoneButton, MageSpellsSelectPointsLeft, Learnable

	MageSpellBook = GemRB.GetVar ("MageSpellBook")
	SpellMask = GemRB.GetVar ("SpellMask")

	#getting the bit index
	Spell = abs(MageSpellBook - SpellMask)
	i = -1
	while (Spell > 0):
		i = i + 1
		Spell = Spell >> 1

	Spell = GemRB.GetSpell (Learnable[i])
	MageSpellsTextArea.SetText (Spell["SpellDesc"])

	if SpellMask < MageSpellBook:
		MageSpellsSelectPointsLeft = MageSpellsSelectPointsLeft + 1
	else:
		if MageSpellsSelectPointsLeft==0:
			SpellMask = MageSpellBook
			GemRB.SetVar ("SpellMask", SpellMask)
		else:
			MageSpellsSelectPointsLeft = MageSpellsSelectPointsLeft - 1

	if MageSpellsSelectPointsLeft == 0:
		MageSpellsDoneButton.SetState (IE_GUI_BUTTON_ENABLED)
	else:
		MageSpellsDoneButton.SetState (IE_GUI_BUTTON_DISABLED)

	for i in range (len(Learnable)):
		SpellButton = MageSpellsWindow.GetControl (i + 2)
		if ((1 << i) & SpellMask) == 0:
			SpellButton.SetState (IE_GUI_BUTTON_LOCKED)

	PointsLeftLabel = MageSpellsWindow.GetControl (0x1000001b)
	PointsLeftLabel.SetText (str(MageSpellsSelectPointsLeft))
	GemRB.SetVar ("MageSpellBook", SpellMask)
	return

def MageSpellsDonePress():
	global CharGenWindow, MageSpellsWindow, SkillsState

	if MageSpellsWindow:
		MageSpellsWindow.Close ()
	SkillsState = 3
	SkillsPress()
	return

def MageSpellsCancelPress():
	global CharGenWindow, MageSpellsWindow, SkillsState

	if MageSpellsWindow:
		MageSpellsWindow.Close ()
	SkillsState = 0
	return


# Mage Spells Memorize

def MageSpellsMemorize(SpellTable, Level, SpellLevel):
	global CharGenWindow, MageMemorizeWindow, MageMemorizeTextArea, MageMemorizeDoneButton, MageMemorizePointsLeft

	MageMemorizeWindow = GemRB.LoadWindow (16)
	MaxSpellsMageTable = GemRB.LoadTable (SpellTable)
	MageSpellBook = GemRB.GetVar ("MageSpellBook")
	GemRB.SetVar ("MageMemorized", 0)
	GemRB.SetVar ("SpellMask", 0)

	MageMemorizePointsLeft = MaxSpellsMageTable.GetValue (str(Level), str(SpellLevel) )
	if MageMemorizePointsLeft<1 or len(Learnable)<1:
		MageMemorizeDonePress()
		return

	# one more spell for specialists
	k = GemRB.GetPlayerStat (MyChar, IE_KIT)
	if k & ~0x4000 > 0:
		MageMemorizePointsLeft = MageMemorizePointsLeft + 1

	PointsLeftLabel = MageMemorizeWindow.GetControl (0x1000001b)
	PointsLeftLabel.SetText (str(MageMemorizePointsLeft))

	j = 0
	for i in range (12):
		SpellButton = MageMemorizeWindow.GetControl (i + 2)
		SpellButton.SetFlags (IE_GUI_BUTTON_PICTURE|IE_GUI_BUTTON_CHECKBOX, OP_OR)
		while (j < len(Learnable)) and (((1 << j) & MageSpellBook) == 0):
			j = j + 1
		if j < len(Learnable):
			Spell = GemRB.GetSpell (Learnable[j])
			SpellButton.SetTooltip(Spell["SpellName"])
			SpellButton.SetSpellIcon(Learnable[j], 1)
			SpellButton.SetState (IE_GUI_BUTTON_ENABLED)
			SpellButton.OnPress (MageMemorizeSelectPress)
			SpellButton.SetVarAssoc ("SpellMask", 1 << j)
			j = j + 1
		else:
			SpellButton.SetState (IE_GUI_BUTTON_DISABLED)

	GemRB.SetToken ("number", str(MageMemorizePointsLeft))
	MageMemorizeTextArea = MageMemorizeWindow.GetControl (27)
	MageMemorizeTextArea.SetText (17253)

	MageMemorizeDoneButton = MageMemorizeWindow.GetControl (0)
	MageMemorizeDoneButton.SetState (IE_GUI_BUTTON_DISABLED)
	MageMemorizeDoneButton.OnPress (MageMemorizeDonePress)
	MageMemorizeDoneButton.SetText (11973)
	MageMemorizeDoneButton.MakeDefault()

	MageMemorizeCancelButton = MageMemorizeWindow.GetControl (29)
	MageMemorizeCancelButton.SetState (IE_GUI_BUTTON_ENABLED)
	MageMemorizeCancelButton.OnPress (MageMemorizeCancelPress)
	MageMemorizeCancelButton.SetText (13727)
	MageMemorizeCancelButton.MakeEscape()

	MageMemorizeWindow.ShowModal(MODAL_SHADOW_NONE)
	return

def MageMemorizeSelectPress():
	global MageMemorizeWindow, MageMemorizeTextArea, MageMemorizeDoneButton, MageMemorizePointsLeft, Learnable

	MageSpellBook = GemRB.GetVar ("MageSpellBook")
	MageMemorized = GemRB.GetVar ("MageMemorized")
	SpellMask = GemRB.GetVar ("SpellMask")

	Spell = abs(MageMemorized - SpellMask)
	i = -1
	while (Spell > 0):
		i = i + 1
		Spell = Spell >> 1

	Spell = GemRB.GetSpell (Learnable[i])
	MageMemorizeTextArea.SetText (Spell["SpellDesc"])

	if SpellMask < MageMemorized:
		MageMemorizePointsLeft = MageMemorizePointsLeft + 1
		j = 0
		for i in range (12):
			SpellButton = MageMemorizeWindow.GetControl (i + 2)
			while (j < len(Learnable) ) and (((1 << j) & MageSpellBook) == 0):
				j = j + 1
			if j < len(Learnable):
				SpellButton.SetState (IE_GUI_BUTTON_ENABLED)
				j = j + 1
		MageMemorizeDoneButton.SetState (IE_GUI_BUTTON_DISABLED)
	else:
		MageMemorizePointsLeft = MageMemorizePointsLeft - 1
		if MageMemorizePointsLeft == 0:
			j = 0
			for i in range (12):
				SpellButton = MageMemorizeWindow.GetControl (i + 2)
				while (j < len(Learnable) ) and (((1 << j) & MageSpellBook) == 0):
					j = j + 1
				if j < len(Learnable):
					if ((1 << j) & SpellMask) == 0:
						SpellButton.SetState (IE_GUI_BUTTON_DISABLED)
					j = j + 1
			MageMemorizeDoneButton.SetState (IE_GUI_BUTTON_ENABLED)

	PointsLeftLabel = MageMemorizeWindow.GetControl (0x1000001b)
	PointsLeftLabel.SetText (str(MageMemorizePointsLeft))
	GemRB.SetVar ("MageMemorized", SpellMask)
	return

def MageMemorizeDonePress():
	global CharGenWindow, MageMemorizeWindow, SkillsState, MyChar

	if MageMemorizeWindow:
		MageMemorizeWindow.Close ()
	LearnSpells (MyChar)
	SkillsState = 4
	SkillsPress()
	return

def MageMemorizeCancelPress():
	global CharGenWindow, MageMemorizeWindow, SkillsState

	if MageMemorizeWindow:
		MageMemorizeWindow.Close ()
	SkillsState = 0
	return

# Priest Spells Memorize

def PriestSpellsMemorize(SpellTable, Level, SpellLevel):
	global CharGenWindow, PriestMemorizeWindow, Learnable, ClassFlag
	global PriestMemorizeTextArea, PriestMemorizeDoneButton, PriestMemorizePointsLeft

	PriestMemorizeWindow = GemRB.LoadWindow (17)
	t = CommonTables.Aligns.GetValue (GemRB.GetVar ("Alignment")-1, 3)
	Learnable = Spellbook.GetLearnablePriestSpells( ClassFlag, t, SpellLevel)

	MaxSpellsPriestTable = GemRB.LoadTable (SpellTable)
	GemRB.SetVar ("PriestMemorized", 0)
	GemRB.SetVar ("SpellMask", 0)

	PriestMemorizePointsLeft = MaxSpellsPriestTable.GetValue (str(Level), str(SpellLevel) )
	if PriestMemorizePointsLeft<1 or len(Learnable)<1:
		PriestMemorizeDonePress()
		return

	PointsLeftLabel = PriestMemorizeWindow.GetControl (0x1000001b)
	PointsLeftLabel.SetText (str(PriestMemorizePointsLeft))

	for i in range (12):
		SpellButton = PriestMemorizeWindow.GetControl (i + 2)
		SpellButton.SetFlags (IE_GUI_BUTTON_PICTURE|IE_GUI_BUTTON_CHECKBOX, OP_OR)
		if i < len(Learnable):
			Spell = GemRB.GetSpell (Learnable[i])
			SpellButton.SetTooltip(Spell["SpellName"])
			SpellButton.SetSpellIcon(Learnable[i], 1)
			SpellButton.SetState (IE_GUI_BUTTON_ENABLED)
			SpellButton.OnPress (PriestMemorizeSelectPress)
			SpellButton.SetVarAssoc ("SpellMask", 1 << i)
		else:
			SpellButton.SetState (IE_GUI_BUTTON_DISABLED)

	GemRB.SetToken ("number", str(PriestMemorizePointsLeft))
	PriestMemorizeTextArea = PriestMemorizeWindow.GetControl (27)
	PriestMemorizeTextArea.SetText (17253)

	PriestMemorizeDoneButton = PriestMemorizeWindow.GetControl (0)
	PriestMemorizeDoneButton.SetState (IE_GUI_BUTTON_DISABLED)
	PriestMemorizeDoneButton.OnPress (PriestMemorizeDonePress)
	PriestMemorizeDoneButton.SetText (11973)
	PriestMemorizeDoneButton.MakeDefault()

	PriestMemorizeCancelButton = PriestMemorizeWindow.GetControl (29)
	PriestMemorizeCancelButton.SetState (IE_GUI_BUTTON_ENABLED)
	PriestMemorizeCancelButton.OnPress (PriestMemorizeCancelPress)
	PriestMemorizeCancelButton.SetText (13727)
	PriestMemorizeCancelButton.MakeEscape()

	PriestMemorizeWindow.ShowModal(MODAL_SHADOW_NONE)
	return

def PriestMemorizeSelectPress():
	global PriestMemorizeWindow, Learnable, PriestMemorizeTextArea, PriestMemorizeDoneButton, PriestMemorizePointsLeft

	PriestMemorized = GemRB.GetVar ("PriestMemorized")
	SpellMask = GemRB.GetVar ("SpellMask")
	Spell = abs(PriestMemorized - SpellMask)

	i = -1
	while (Spell > 0):
		i = i + 1
		Spell = Spell >> 1

	Spell=GemRB.GetSpell (Learnable[i])
	PriestMemorizeTextArea.SetText (Spell["SpellDesc"])

	if SpellMask < PriestMemorized:
		PriestMemorizePointsLeft = PriestMemorizePointsLeft + 1
		for i in range (len(Learnable)):
			SpellButton = PriestMemorizeWindow.GetControl (i + 2)
			if (((1 << i) & SpellMask) == 0):
				SpellButton.SetState (IE_GUI_BUTTON_ENABLED)
		PriestMemorizeDoneButton.SetState (IE_GUI_BUTTON_DISABLED)
	else:
		PriestMemorizePointsLeft = PriestMemorizePointsLeft - 1
		if PriestMemorizePointsLeft == 0:
			for i in range (len(Learnable)):
				SpellButton = PriestMemorizeWindow.GetControl (i + 2)
				if ((1 << i) & SpellMask) == 0:
					SpellButton.SetState (IE_GUI_BUTTON_DISABLED)
			PriestMemorizeDoneButton.SetState (IE_GUI_BUTTON_ENABLED)

	PointsLeftLabel = PriestMemorizeWindow.GetControl (0x1000001b)
	PointsLeftLabel.SetText (str(PriestMemorizePointsLeft))
	GemRB.SetVar ("PriestMemorized", SpellMask)
	return

def PriestMemorizeDonePress():
	global CharGenWindow, PriestMemorizeWindow, SkillsState, MyChar

	if PriestMemorizeWindow:
		PriestMemorizeWindow.Close ()
	LearnSpells (MyChar)
	SkillsState = 5
	SkillsPress()
	return

def PriestMemorizeCancelPress():
	global CharGenWindow, PriestMemorizeWindow, SkillsState

	if PriestMemorizeWindow:
		PriestMemorizeWindow.Close ()
	SkillsState = 0
	return

# Appearance Selection
def AppearancePress():
	pc = GemRB.GetVar("Slot")
	stats = PaperDoll.ColorStatsFromPortrait(Portrait)
	AppearanceWindow = PaperDoll.OpenPaperDollWindow(pc, "GUICG", stats)
	
	def AppearanceDonePress():
		PaperDoll.SaveStats(stats, pc)
		AppearanceWindow.Close()
		CharSoundSelect()
		return
		
	DoneButton = AppearanceWindow.GetControl(0)
	DoneButton.OnPress(AppearanceDonePress)

	return

def CharSoundSelect():
	global CharGenWindow, CharSoundWindow, CharSoundTable, CharSoundStrings
	global CharSoundVoiceList, VerbalConstants

	CharSoundWindow = GemRB.LoadWindow (19)
	CharSoundTable = GemRB.LoadTable ("CHARSND")
	CharSoundStrings = GemRB.LoadTable ("CHARSTR")

	VerbalConstants =  [CharSoundTable.GetRowName(i) for i in range(CharSoundTable.GetRowCount())]
	CharSoundVoiceList = CharSoundWindow.GetControl (45)

	if GemRB.GetVar ("Gender") == 2:
		GemRB.SetVar ("Selected", 0) #first female sound
	else:
		GemRB.SetVar ("Selected", 15)
	CharSoundVoiceList.SetVarAssoc ("Selected", 0)
	CharSoundVoiceList.ListResources (CHR_SOUNDS)

	CharSoundPlayButton = CharSoundWindow.GetControl (47)
	CharSoundPlayButton.SetState (IE_GUI_BUTTON_ENABLED)
	CharSoundPlayButton.OnPress (CharSoundPlayPress)
	CharSoundPlayButton.SetText (17318)

	CharSoundTextArea = CharSoundWindow.GetControl (50)
	CharSoundTextArea.SetText (11315)

	CharSoundDoneButton = CharSoundWindow.GetControl (0)
	CharSoundDoneButton.SetState (IE_GUI_BUTTON_ENABLED)
	CharSoundDoneButton.OnPress (CharSoundDonePress)
	CharSoundDoneButton.SetText (11973)
	CharSoundDoneButton.MakeDefault()

	CharSoundCancelButton = CharSoundWindow.GetControl (10)
	CharSoundCancelButton.SetState (IE_GUI_BUTTON_ENABLED)
	CharSoundCancelButton.OnPress (CharSoundCancelPress)
	CharSoundCancelButton.SetText (13727)
	CharSoundCancelButton.MakeEscape()

	CharSoundWindow.ShowModal(MODAL_SHADOW_NONE)
	return

def CharSoundPlayPress():
	global CharGenWindow, CharSoundWindow, CharSoundTable, CharSoundStrings
	global CharSoundVoiceList, SoundIndex, VerbalConstants

	row = CharSoundVoiceList.QueryText ()
	GemRB.SetPlayerSound (MyChar, row)

	#play sound as sound slot
	GemRB.VerbalConstant (MyChar, int(VerbalConstants[SoundIndex]))

	SoundIndex += 1
	if SoundIndex >= len(VerbalConstants):
		SoundIndex = 0
	return

def CharSoundDonePress():
	global CharGenWindow, CharSoundWindow, AppearanceButton, BiographyButton, NameButton, CharGenState

	if CharSoundWindow:
		CharSoundWindow.Close ()
	AppearanceButton.SetState (IE_GUI_BUTTON_DISABLED)
	BiographyButton.SetState (IE_GUI_BUTTON_ENABLED)
	NameButton.SetState (IE_GUI_BUTTON_ENABLED)
	NameButton.MakeDefault()
	CharGenState = 7
	SetCharacterDescription()
	return

def CharSoundCancelPress():
	global CharGenWindow, CharSoundWindow

	if CharSoundWindow:
		CharSoundWindow.Close ()
	return

# Biography Selection

def BiographyPress():
	global CharGenWindow, BiographyWindow, BiographyTextArea

	BiographyWindow = GemRB.LoadWindow (51)
	BiographyTextArea = BiographyWindow.ReplaceSubview(4, IE_GUI_TEXTAREA, "NORMAL")
	BiographyTextArea.SetFlags(IE_GUI_TEXTAREA_EDITABLE, OP_OR)
	BiographyTextArea.Focus()
	BiographyTextArea.SetColor (ColorWhitish, TA_COLOR_NORMAL)

	BIO = GemRB.GetToken("Biography")
	if BIO:
		BiographyTextArea.SetText (BIO)
	else:
		BiographyTextArea.SetText (19423)

	BiographyClearButton = BiographyWindow.GetControl (5)
	BiographyClearButton.SetState (IE_GUI_BUTTON_ENABLED)
	BiographyClearButton.OnPress (lambda: BiographyClearPress (BiographyTextArea))
	BiographyClearButton.SetText (18622)

	BiographyCancelButton = BiographyWindow.GetControl (2)
	BiographyCancelButton.SetState (IE_GUI_BUTTON_ENABLED)
	BiographyCancelButton.OnPress (BiographyWindow.Close)
	BiographyCancelButton.SetText (13727)
	BiographyCancelButton.MakeEscape()

	BiographyDoneButton = BiographyWindow.GetControl (1)
	BiographyDoneButton.SetState (IE_GUI_BUTTON_ENABLED)
	BiographyDoneButton.OnPress (BiographyDonePress)
	BiographyDoneButton.SetText (11973)

	BiographyWindow.ShowModal(MODAL_SHADOW_NONE)
	return

def BiographyClearPress(TA):
	TA.Clear ()
	TA.Focus ()

def BiographyDonePress():
	global CharGenWindow, BiographyWindow, BiographyTextArea

	BIO = BiographyTextArea.QueryText ()
	GemRB.SetToken ("Biography", BIO) # just for any window reopens
	BioStrRefSlot = 63
	DefaultBIO = 19423
	if BIO == GemRB.GetString (DefaultBIO):
		GemRB.SetPlayerString (MyChar, BioStrRefSlot, DefaultBIO)
	else:
		# unlike tob, iwd has no marked placeholders (or strings) at 62015; but we have special magic in place ...
		# still, use the returned strref in case anything unexpected happened
		ref = GemRB.CreateString (62015+MyChar, BIO)
		GemRB.SetPlayerString (MyChar, BioStrRefSlot, ref)

	if BiographyWindow:
		BiographyWindow.Close ()
	return

# Name Selection

def NamePress():
	global CharGenWindow, NameWindow, NameDoneButton, NameField

	NameWindow = GemRB.LoadWindow (5)

	NameDoneButton = NameWindow.GetControl (0)
	NameDoneButton.SetState (IE_GUI_BUTTON_DISABLED)
	NameDoneButton.OnPress (NameDonePress)
	NameDoneButton.SetText (11973)
	NameDoneButton.MakeDefault()

	NameCancelButton = NameWindow.GetControl (3)
	NameCancelButton.SetState (IE_GUI_BUTTON_ENABLED)
	NameCancelButton.OnPress (NameCancelPress)
	NameCancelButton.SetText (13727)
	NameCancelButton.MakeEscape()

	NameField = NameWindow.GetControl (2)
	NameField.OnChange (NameEditChange)
	NameField.SetText (GemRB.GetToken ("CHARNAME") )
	NameField.Focus()

	NameWindow.ShowModal(MODAL_SHADOW_NONE)
	NameEditChange()
	return

def NameEditChange():
	global NameField

	if NameField.QueryText () == "":
		NameDoneButton.SetState (IE_GUI_BUTTON_DISABLED)
	else:
		NameDoneButton.SetState (IE_GUI_BUTTON_ENABLED)
	return

def NameDonePress():
	global CharGenWindow, CharGenState, NameWindow, NameField, AcceptButton

	GemRB.SetToken ("CHARNAME", NameField.QueryText () )
	if NameWindow:
		NameWindow.Close ()
	CharGenState = 8
	AcceptButton.SetState (IE_GUI_BUTTON_ENABLED)
	AcceptButton.MakeDefault()
	SetCharacterDescription()
	return

def NameCancelPress():
	global CharGenWindow, NameWindow

	GemRB.SetToken ("CHARNAME", "")
	if NameWindow:
		NameWindow.Close ()
	return

# Import Character

def ImportPress():
	global CharGenWindow, ImportWindow
	global CharImportList

	ImportWindow = GemRB.LoadWindow (20)

	TextAreaControl = ImportWindow.GetControl(4)
	TextAreaControl.SetText(10963)

	GemRB.SetVar ("Selected", 0)
	CharImportList = ImportWindow.GetControl(2)
	CharImportList.SetVarAssoc ("Selected",0)
	CharImportList.ListResources(CHR_EXPORTS)

	ImportDoneButton = ImportWindow.GetControl (0)
	ImportDoneButton.SetState (IE_GUI_BUTTON_ENABLED)
	ImportDoneButton.OnPress (ImportDonePress)
	ImportDoneButton.SetText (11973)
	ImportDoneButton.MakeDefault()

	ImportCancelButton = ImportWindow.GetControl (1)
	ImportCancelButton.SetState (IE_GUI_BUTTON_ENABLED)
	ImportCancelButton.OnPress (ImportCancelPress)
	ImportCancelButton.SetText (13727)
	ImportCancelButton.MakeEscape()

	ImportWindow.ShowModal(MODAL_SHADOW_NONE)
	return

def ImportDonePress():
	global CharGenWindow, ImportWindow, CharImportList
	global CharGenState, SkillsState, Portrait, ImportedChar, HasStrExtra

	# Import the character from the chosen name
	GemRB.CreatePlayer (CharImportList.QueryText(), MyChar|0x8000, 1)

	GemRB.SetToken ("CHARNAME", GemRB.GetPlayerName (MyChar))
	GemRB.SetToken ("SmallPortrait", GemRB.GetPlayerPortrait (MyChar, 1)["ResRef"])
	Portrait = GemRB.GetPlayerPortrait (MyChar, 0)
	GemRB.SetToken ("LargePortrait", Portrait["ResRef"])
	PortraitButton.SetPicture (Portrait["Sprite"], "NOPORTLG")
	Portrait = -1

	ClassName = GUICommon.GetClassRowName (MyChar)
	HasStrExtra = CommonTables.Classes.GetValue (ClassName, "STREXTRA", GTV_INT)

	ImportedChar = 1
	CharGenState = 7
	SkillsState = 5
	SetCharacterDescription ()
	GenderButton.SetState (IE_GUI_BUTTON_DISABLED)
	RaceButton.SetState (IE_GUI_BUTTON_DISABLED)
	ClassButton.SetState (IE_GUI_BUTTON_DISABLED)
	AlignmentButton.SetState (IE_GUI_BUTTON_DISABLED)
	AbilitiesButton.SetState (IE_GUI_BUTTON_DISABLED)
	SkillsButton.SetState (IE_GUI_BUTTON_DISABLED)
	AppearanceButton.SetState (IE_GUI_BUTTON_ENABLED)
	BiographyButton.SetState (IE_GUI_BUTTON_DISABLED)
	NameButton.SetState (IE_GUI_BUTTON_DISABLED)
	NameButton.MakeDefault()
	if ImportWindow:
		ImportWindow.Close ()
	return

def ImportCancelPress():
	global CharGenWindow, ImportWindow

	if ImportWindow:
		ImportWindow.Close ()
	return
