import GemRB
from CharGenCommon import * 
from GUICommonWindows import *;

def Imprt():
	GemRB.SetToken("NextScript","CharGen")
	GemRB.SetNextScript("ImportFile") #import
	return

def setPlayer():
	MyChar = GemRB.GetVar ("Slot")
	GemRB.CreatePlayer ("charbase", MyChar | 0x8000 )
	return False

def unsetPlayer():
	MyChar = GemRB.GetVar ("Slot")
	GemRB.CreatePlayer ("", MyChar | 0x8000 )
	return False

def unsetGender():
	#print "unset Gender"
	MyChar = GemRB.GetVar ("Slot")
	GemRB.SetPlayerStat (MyChar, IE_SEX, 0)

def unsetPortrait():
	#print "unset Portrait"
	GemRB.SetToken("SmallPortrait","")
	GemRB.SetToken("LargePortrait","")

def getGender(area):
	MyChar = GemRB.GetVar ("Slot")
	area.SetText(12135)
	area.Append (": ")
	if GemRB.GetPlayerStat(MyChar,IE_SEX) == 1:
		return area.Append(1050)
	else:
		return area.Append(1051)

#race
def unsetRace():
	MyChar = GemRB.GetVar ("Slot")
	GemRB.SetPlayerStat (MyChar, IE_RACE, 0)

def getRace(area):
	MyChar = GemRB.GetVar ("Slot")
	RaceID = GemRB.GetPlayerStat (MyChar, IE_RACE)
	RaceIndex = RaceTable.FindValue(3,RaceID)
	RaceCap = RaceTable.GetValue(RaceIndex,2)
	area.Append(1048,-1) # new line
	area.Append(": ")
	area.Append(RaceCap)

#class
def unsetClass():
	MyChar = GemRB.GetVar ("Slot")
	GemRB.SetPlayerStat (MyChar, IE_CLASS, 0)
	GemRB.SetPlayerStat (MyChar, IE_KIT, 0)
	
def getClass(area):
	MyChar = GemRB.GetVar ("Slot")
	ClassTitle = GetActorClassTitle(MyChar)

	area.Append(12136, -1)
	area.Append(": ")
	area.Append(ClassTitle)
	
def guardSpecialist():
	return GemRB.GetVar("Specialist") == 1

def guardMultiClass():
	return GemRB.GetVar("Multi Class") == 1


#Alignment
def unsetAlignment():
	MyChar = GemRB.GetVar ("Slot")
	GemRB.SetPlayerStat (MyChar, IE_ALIGNMENT,0)
	
def getAlignment(area):
	AlignmentTable = GemRB.LoadTableObject("aligns")
	
	MyChar = GemRB.GetVar ("Slot")
	AllignID = GemRB.GetPlayerStat (MyChar, IE_ALIGNMENT)
	
	area.Append(1049, -1)
	area.Append(": ")
	AllignIndex = AlignmentTable.FindValue(3,AllignID)
	AllignCap = AlignmentTable.GetValue(AllignIndex,2)
	area.Append(AllignCap)
	area.Append("\n")

#Abilties
def unsetAbilities():
	MyChar = GemRB.GetVar ("Slot")
	AbilityTable = GemRB.LoadTableObject ("ability")
	AbilityCount = AbilityTable.GetRowCount ()
	
	# set all our abilites to zero
	GemRB.SetPlayerStat (MyChar, IE_STREXTRA, 0)
	for i in range(AbilityCount):
		StatID = AbilityTable.GetValue (i, 3)
		GemRB.SetPlayerStat (MyChar, StatID, 0)

def getAbilities(area):
	MyChar = GemRB.GetVar ("Slot")
	AbilityTable = GemRB.LoadTableObject ("ability")
	AbilityCount = AbilityTable.GetRowCount ()
	for i in range(AbilityCount):
		v = AbilityTable.GetValue(i,2)
		id = AbilityTable.GetValue(i,3)
		area.Append(v, -1)
		area.Append(": "+str(GemRB.GetPlayerStat(MyChar,id)))
	area.Append("\n")
	area.Append("\n")

#Skill
def unsetHateRace():
	MyChar = GemRB.GetVar ("Slot")
	GemRB.SetPlayerStat(MyChar, IE_HATEDRACE, 0 )

def guardHateRace():
	MyChar = GemRB.GetVar ("Slot")
	Class = GemRB.GetPlayerStat(MyChar,IE_CLASS)
	TmpTable = GemRB.LoadTableObject("clskills")
	ClassName = TmpTable.GetRowName(Class)
	TableName = TmpTable.GetValue(ClassName, "HATERACE")
	return TableName != "*"

def getHatedRace(TextAreaControl):
	MyChar = GemRB.GetVar ("Slot")
	Race = GemRB.GetPlayerStat(MyChar, IE_HATEDRACE)
	if Race:
		HateRaceTable = GemRB.LoadTableObject ("HATERACE")
		Row = HateRaceTable.FindValue (1, Race)
		info = GemRB.GetString (HateRaceTable.GetValue(Row, 0))
		if info != "":
			#TextAreaControl.Append("\n")
			info = ": " + info + "\n"
			TextAreaControl.Append(15982)
			TextAreaControl.Append(info)

def unsetMageSpells():
	print("unsetMageSpells")
	MyChar = GemRB.GetVar ("Slot")
	
	RemoveKnownSpells (MyChar, IE_SPELL_TYPE_WIZARD, 1, 5, 1)
	GemRB.SetMemorizableSpellsCount(MyChar, 0, IE_SPELL_TYPE_WIZARD, 1)
	
	
def guardMageSpells():
	MyChar = GemRB.GetVar ("Slot")
	Class = GemRB.GetPlayerStat(MyChar,IE_CLASS)
	TmpTable = GemRB.LoadTableObject("clskills")
	TableName = TmpTable.GetValue(Class, 2)
	return TableName != "*"

def getMageSpells(TextAreaControl):
	MyChar = GemRB.GetVar ("Slot")
	# arcane spells
	info = ""
	for level in range(0, 9):
		for j in range(0, GemRB.GetKnownSpellsCount (MyChar, IE_SPELL_TYPE_WIZARD, level) ):
			Spell = GemRB.GetKnownSpell (MyChar, IE_SPELL_TYPE_WIZARD, level, j)
			Spell = GemRB.GetSpell (Spell['SpellResRef'], 1)['SpellName']
			info += GemRB.GetString (Spell) + "\n"
	if info != "":
		info = "\n" + info + "\n"
		TextAreaControl.Append (11027)
		TextAreaControl.Append (info)

def guardSkills():
	SkillTable = GemRB.LoadTableObject("skills")
	RowCount = SkillTable.GetRowCount()-2

	MyChar = GemRB.GetVar ("Slot")
	Kit = GemRB.GetPlayerStat (MyChar, IE_KIT)

	if Kit != 0:
		KitList = GemRB.LoadTableObject("kitlist")
		KitName = KitList.GetValue(Kit, 0) #rowname is just a number
		PointsLeft = SkillTable.GetValue("FIRST_LEVEL",KitName)
		if PointsLeft < 0:
			Kit = 0
		
	if Kit == 0:
		ClassID = GemRB.GetPlayerStat (MyChar, IE_CLASS)
		ClassIndex = ClassTable.FindValue(5,ClassID)
		KitName = ClassTable.GetRowName(ClassIndex)

	for i in range(RowCount):
		SkillName = SkillTable.GetRowName(i+2)
		if SkillTable.GetValue(SkillName, KitName)==1:
			return True

	return False
		
def unsetSkill():
	from LUSkillsSelection import SkillsNullify,SkillsSave
	MyChar = GemRB.GetVar ("Slot")
	SkillsNullify ()
	SkillsSave (MyChar)
	
def getSkills(TextAreaControl):
	MyChar = GemRB.GetVar ("Slot")
	# thieving and other skills
	info = ""
	SkillTable = GemRB.LoadTableObject ("skills")
	ClassID = GemRB.GetPlayerStat (MyChar, IE_CLASS)
	Class = ClassTable.FindValue (5, ClassID)
	ClassName = ClassTable.GetRowName(Class)
	RangerSkills = ClassSkillsTable.GetValue (ClassName, "RANGERSKILL")
	BardSkills = ClassSkillsTable.GetValue (ClassName, "BARDSKILL")
	KitName = GetKitIndex (MyChar)
	if KitName == 0:
		KitName = ClassName
	else:
		KitName = KitListTable.GetValue (KitName, 0)
		
	if SkillTable.GetValue ("RATE", KitName) != -1 or BardSkills != "*" or RangerSkills != "*":
		for skill in range(SkillTable.GetRowCount () - 2):
			name = GemRB.GetString (SkillTable.GetValue (skill+2, 1))
			id = SkillTable.GetValue (skill+2, 2)
			available = SkillTable.GetValue (SkillTable.GetRowName (skill+2), KitName)
			value = GemRB.GetPlayerStat(MyChar,id)
			if value >= 0 and available != -1:
				info += name + ": " + str(value) + "\n"
				
	if info != "":
		info = "\n" + info + "\n"
		TextAreaControl.Append("\n")
		TextAreaControl.Append (8442)
		TextAreaControl.Append (info)
	
def unsetProfi():
	from GUICG9 import ProfsNullify,ProfsSave
	MyChar = GemRB.GetVar ("Slot")
	ProfsNullify ()
	ProfsSave(MyChar)

def getProfi(TextAreaControl):
	MyChar = GemRB.GetVar ("Slot")
	# weapon proficiencies
	TextAreaControl.Append ("\n")
	TextAreaControl.Append (9466)
	TextAreaControl.Append ("\n")
	TmpTable=GemRB.LoadTableObject ("weapprof")
	ProfCount = TmpTable.GetRowCount ()
	#bg2 weapprof.2da contains the bg1 proficiencies too, skipping those
	for i in range(ProfCount):
		# 4294967296 overflows to -1 on some arches, so we use a smaller invalid strref
		id = TmpTable.GetValue (i, 0)+IE_PROFICIENCYBASTARDSWORD
		Weapon = GemRB.GetString (TmpTable.GetValue (i, 1))
		Value = GemRB.GetPlayerStat (MyChar,id)
		if Value:
			pluses = " "
			for plus in range(0, Value):
				pluses += "+"
			TextAreaControl.Append (Weapon + pluses + "\n")

#Appearance
def unsetColors():
	MyChar = GemRB.GetVar ("Slot")
	SetColorStat (MyChar, IE_HAIR_COLOR, 0 )
	SetColorStat (MyChar, IE_SKIN_COLOR, 0 )
	SetColorStat (MyChar, IE_MAJOR_COLOR, 0 )
	SetColorStat (MyChar, IE_MINOR_COLOR, 0 )

def unsetSounds():
	MyChar = GemRB.GetVar ("Slot")
	GemRB.SetPlayerSound(MyChar,"")


#name
def unsetName():
	MyChar = GemRB.GetVar ("Slot")
	GemRB.SetPlayerName (MyChar, "", 0)
	
def getName(TextAreaControl):
	MyChar = GemRB.GetVar ("Slot")
	name = GemRB.GetPlayerName(MyChar)
	if(name != ""):
		TextAreaControl.Append(name + "\n")

#divine spells
def setDivineSpells():
	MyChar = GemRB.GetVar ("Slot")
	
	ClassID = GemRB.GetPlayerStat (MyChar, IE_CLASS)
	Class = ClassTable.FindValue (5, ClassID)
	ClassName = ClassTable.GetRowName(Class)
	
	DruidTable = ClassSkillsTable.GetValue (ClassName, "DRUIDSPELL")
	ClericTable = ClassSkillsTable.GetValue (ClassName, "CLERICSPELL")
	
	print("CGG setDivineSpells: CP1",ClassID,Class,ClassName,DruidTable,ClericTable)
	
	AllignID = GemRB.GetPlayerStat (MyChar, IE_ALIGNMENT)
	
	if ClericTable != "*":
		learnDivine(MyChar,0x4000,ClericTable,AllignID)
	if DruidTable != "*":
		learnDivine(MyChar,0x8000,DruidTable,AllignID)
		
	return False

def learnDivine(MyChar,ClassFlag,TableName,AllignID):
	#print ("CGG setDivineSpells: CP2",MyChar,ClassFlag,TableName,AllignID )
	SetupSpellLevels(MyChar, TableName, IE_SPELL_TYPE_PRIEST, 1)
	Learnable = GetLearnablePriestSpells( ClassFlag, AllignID, 1)
	for i in range(len(Learnable) ):
		#print ("CGG setDivineSpells: CP3",Learnable[i])
		if -1 == HasSpell(MyChar,IE_SPELL_TYPE_PRIEST,1,Learnable[i]):
			#print ("CGG setDivineSpells: CP4",Learnable[i])
			GemRB.LearnSpell (MyChar, Learnable[i], 0)

def unsetDivineSpells():
	print("unsetDivineSpells")
	MyChar = GemRB.GetVar ("Slot")
	for level in range(0, 5):
		for j in range(0, GemRB.GetKnownSpellsCount (MyChar, IE_SPELL_TYPE_PRIEST, level) ):
			GemRB.RemoveSpell(MyChar,IE_SPELL_TYPE_PRIEST, level, 0)
	GemRB.SetMemorizableSpellsCount(MyChar, 0, IE_SPELL_TYPE_PRIEST, 1)
			

def getDivineSpells(TextAreaControl):
	MyChar = GemRB.GetVar ("Slot")
	# divine spells
	info = ""
	for level in range(0, 7):
		for j in range(0, GemRB.GetKnownSpellsCount (MyChar, IE_SPELL_TYPE_PRIEST, level) ):
			Spell = GemRB.GetKnownSpell (MyChar, IE_SPELL_TYPE_PRIEST, level, j)
			Spell = GemRB.GetSpell (Spell['SpellResRef'], 1)['SpellName']
			info += GemRB.GetString (Spell) + "\n"
	if info != "":
		info = "\n" + info + "\n"
		TextAreaControl.Append (11028)
		TextAreaControl.Append (info)
		
#finish
def setAccept():	
	#set my character up
	MyChar = GemRB.GetVar ("Slot")

	ClassID = GemRB.GetPlayerStat (MyChar, IE_CLASS)
	Class = ClassTable.FindValue (5, ClassID)
	ClassName = ClassTable.GetRowName(Class)
	
	#reputation
	AllignID = GemRB.GetPlayerStat (MyChar, IE_ALIGNMENT)
	
	TmpTable=GemRB.LoadTableObject ("repstart")
	AlignmentTable = GemRB.LoadTableObject ("aligns")
	t = TmpTable.GetValue (AllignID,0) * 10
	GemRB.SetPlayerStat (MyChar, IE_REPUTATION, t)

	#slot 1 is the protagonist
	if MyChar == 1:
		GemRB.GameSetReputation( t )

	#gold
	TmpTable=GemRB.LoadTableObject ("strtgold")
	t = GemRB.Roll (TmpTable.GetValue (ClassName,"ROLLS"),TmpTable.GetValue(ClassName,"SIDES"), TmpTable.GetValue (ClassName,"MODIFIER") )
	GemRB.SetPlayerStat (MyChar, IE_GOLD, t*TmpTable.GetValue (ClassName,"MULTIPLIER") )

	#set the base number of attacks; effects will add the proficiency bonus
	GemRB.SetPlayerStat (MyChar, IE_NUMBEROFATTACKS, 2)

	#colors
	SetColorStat (MyChar, IE_METAL_COLOR, 0x1B )
	SetColorStat (MyChar, IE_LEATHER_COLOR, 0x16 )
	SetColorStat (MyChar, IE_ARMOR_COLOR, 0x17 )

	#does all the rest
	LargePortrait = GemRB.GetToken ("LargePortrait")
	SmallPortrait = GemRB.GetToken ("SmallPortrait")
	GemRB.FillPlayerInfo (MyChar, LargePortrait, SmallPortrait)
	#10 is a weapon slot (see slottype.2da row 10)
	GemRB.CreateItem (MyChar, "staf01", 10, 1, 0, 0)
	GemRB.SetEquippedQuickSlot (MyChar, 0)
	#LETS PLAY!!
	playmode = GemRB.GetVar ("PlayMode")
	
	CloseOtherWindow(None)
	close()
	
	if playmode >=0:
		GemRB.EnterGame()
	else:
		#leaving multi player pregen
		GemRB.SetNextScript ("GUIMP")
