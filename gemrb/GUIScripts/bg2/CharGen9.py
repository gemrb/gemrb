#character generation (GUICG 0)
import GemRB
from CharGenCommon import *
from ie_stats import *
from ie_slots import *
from GUICommon import *

CharGenWindow = 0

def OnLoad():
	global CharGenWindow
	DisplayOverview (9)

	return

# we need to redefine this or we're stuck in an include loop with CharGenCommon
def NextPress():
	FinishCharGen()
	return

def FinishCharGen():
	#set my character up
	MyChar = GemRB.GetVar ("Slot")
	GemRB.SetPlayerStat (MyChar, IE_SEX, GemRB.GetVar ("Gender") )
	KitTable = GemRB.LoadTableObject ("kitlist")
	RaceTable = GemRB.LoadTableObject ("races")
	Race = GemRB.GetVar ("Race")-1
	GemRB.SetPlayerStat (MyChar, IE_RACE, RaceTable.GetValue (Race, 3) )
	t = GemRB.GetVar ("Alignment")
	GemRB.SetPlayerStat (MyChar, IE_ALIGNMENT, t)

	#a little explanation for the different handling of mage kit values:
	#Originally, the IE had only mage schools, and the kit field
	#was simply an unusability field (with a single bit set)
	#then BG2 crammed in a lot more kits, and 32 bits were not enough.
	#They solved this by making the generalist value 0x4000 to hold
	#the kit index in the lower portions.
	#When you see 0x4000 in a kit value, you need to translate
	#the kit index to unusability value, using the kitlist
	#So, for mages, the kit equals to the unusability value
	#but for others there is an additional mapping by kitlist.2da

	ClassTable = GemRB.LoadTableObject ("classes")
	ClassIndex = GemRB.GetVar ("Class")-1
	Class = ClassTable.GetValue (ClassIndex, 5)
	ClassName = ClassTable.GetRowName (ClassIndex)
	GemRB.SetPlayerStat (MyChar, IE_CLASS, Class)

	ClassSkillsTable = GemRB.LoadTableObject ("clskills")
	
	#mage spells
	TableName = ClassSkillsTable.GetValue (Class, 2, 0)

	# TODO check if this is really needed
	KitIndex = GemRB.GetVar ("Class Kit")
	if TableName != "*":
		KitValue = KitTable.GetValue(KitIndex, 6)
		if KitValue == "*":
			KitValue = 0x4000
		SetupSpellLevels(MyChar, TableName, IE_SPELL_TYPE_WIZARD, 1)
	else:
		KitValue = (0x4000 + KitIndex)<<16

	print "KitValue**********:",KitValue
	GemRB.SetPlayerStat (MyChar, IE_KIT, KitValue)

	# apply class/kit abilities
	if KitIndex:
		ABTable = KitTable.GetValue (str(KitIndex), "ABILITIES")
	else:
		ABTable = ClassSkillsTable.GetValue (ClassName, "ABILITIES")
	if not KitIndex and "," in ABTable:
		# multiclass
		classes = ABTable.split(",")
		for j in classes:
			AddClassAbilities (MyChar, "CLAB"+j)
	else:
		if ABTable != "*" and ABTable[:6] != "CLABMA": # mage kits specify ability tables which don't exist
			AddClassAbilities (MyChar, ABTable)

	# Lay on hands, turn undead and backstab multiplier get set by the core

	# set the starting reputation
	TmpTable=GemRB.LoadTableObject ("repstart")
	AlignmentTable = GemRB.LoadTableObject ("aligns")
	t = AlignmentTable.FindValue (3, t)
	t = TmpTable.GetValue (t,0) * 10
	GemRB.SetPlayerStat (MyChar, IE_REPUTATION, t)
	#slot 1 is the protagonist
	if MyChar == 1:
		GemRB.GameSetReputation( t )

	# apply starting (alignment dictated) abilities
	TmpTable = GemRB.LoadTableObject ("abstart")
	AlignmentAbbrev = AlignmentTable.FindValue (3, GemRB.GetPlayerStat (MyChar, IE_ALIGNMENT))
	AlignmentAbbrev = AlignmentTable.GetValue (AlignmentAbbrev, 4)
	for i in range(TmpTable.GetColumnCount()):
		ab = TmpTable.GetValue (AlignmentAbbrev, str(i))
		GemRB.LearnSpell (MyChar, ab)

	TmpTable=GemRB.LoadTableObject ("strtgold")
	t = GemRB.Roll (TmpTable.GetValue (Class,1),TmpTable.GetValue (Class,0), TmpTable.GetValue (Class,2) )
	GemRB.SetPlayerStat (MyChar, IE_GOLD, t*TmpTable.GetValue (Class,3) )

	GemRB.SetPlayerStat (MyChar, IE_HATEDRACE, GemRB.GetVar ("HatedRace") )
	TmpTable=GemRB.LoadTableObject ("ability")
	AbilityCount = TmpTable.GetRowCount ()
	for i in range(AbilityCount):
		StatID = TmpTable.GetValue (i,3)
		if StatID == IE_STR:
			strength = GemRB.GetVar ("Ability 0")
			GemRB.SetPlayerStat (MyChar, StatID, strength)
			if strength == 18:
				GemRB.SetPlayerStat (MyChar, IE_STREXTRA, GemRB.GetVar ("StrExtra"))
			else:
				GemRB.SetPlayerStat (MyChar, IE_STREXTRA, 0)
		else:
			GemRB.SetPlayerStat (MyChar, StatID, GemRB.GetVar ("Ability "+str(i) ) )

	# set the base number of attacks; effects will add the proficiency bonus
	GemRB.SetPlayerStat (MyChar, IE_NUMBEROFATTACKS, 2)
	TmpTable=GemRB.LoadTableObject ("weapprof")
	ProfCount = TmpTable.GetRowCount ()
	#bg2 weapprof.2da contains the bg1 proficiencies too, skipping those
	for i in range(ProfCount-8):
		StatID = TmpTable.GetValue (i+8, 0)
		Value = GemRB.GetVar ("Prof "+str(i) )
		if Value:
			GemRB.ApplyEffect (MyChar, "Proficiency", Value, StatID )

	SetColorStat (MyChar, IE_HAIR_COLOR, GemRB.GetVar ("HairColor") )
	SetColorStat (MyChar, IE_SKIN_COLOR, GemRB.GetVar ("SkinColor") )
	SetColorStat (MyChar, IE_MAJOR_COLOR, GemRB.GetVar ("MajorColor") )
	SetColorStat (MyChar, IE_MINOR_COLOR, GemRB.GetVar ("MinorColor") )
	#SetColorStat (MyChar, IE_METAL_COLOR, 0x1B )
	#SetColorStat (MyChar, IE_LEATHER_COLOR, 0x16 )
	#SetColorStat (MyChar, IE_ARMOR_COLOR, 0x17 )
	GemRB.SetPlayerStat (MyChar, IE_EA, 2 )

	#setting skills (thieving/ranger)
	TmpTable = GemRB.LoadTableObject ("skills")
	RowCount = TmpTable.GetRowCount ()-2

	for i in range(RowCount):
		stat = TmpTable.GetValue (i+2, 2)
		value = GemRB.GetVar ("Skill "+str(i) )
		GemRB.SetPlayerStat (MyChar, stat, value )

	GemRB.SetPlayerName (MyChar, GemRB.GetToken ("CHARNAME"), 0)
	GemRB.SetPlayerStat (MyChar, IE_XP, ClassSkillsTable.GetValue (Class, 3) )  #this will also set the level (automatically)

	#does all the rest
	LargePortrait = GemRB.GetToken ("LargePortrait")
	SmallPortrait = GemRB.GetToken ("SmallPortrait")
	GemRB.FillPlayerInfo (MyChar, LargePortrait, SmallPortrait)

	# add the starting inventory for tob
	if GameIsTOB():
		KitIndex = GetKitIndex (MyChar)
		if KitIndex == 0:
			EquipmentColName = ClassName
			# sorcerers are missing from the table, use the mage equipment instead
			if EquipmentColName == "SORCERER":
				EquipmentColName = "MAGE"
		else:
			EquipmentColName = KitTable.GetValue (KitIndex, 0)
		EquipmentTable = GemRB.LoadTableObject ("25stweap")
		# a map of slots in the table to the real slots
		# SLOT_BAG is invalid, so use the inventory (first occurence)
		# SLOT_INVENTORY: use -1 instead, that's what CreateItem expects
		RealSlots = [ SLOT_ARMOUR, SLOT_SHIELD, SLOT_HELM, -1, SLOT_RING, \
					SLOT_RING, SLOT_CLOAK, SLOT_BOOT, SLOT_AMULET, SLOT_GLOVE, \
					SLOT_BELT, SLOT_QUIVER, SLOT_QUIVER, SLOT_QUIVER, \
					SLOT_ITEM, SLOT_ITEM, SLOT_ITEM, -1, -1, SLOT_WEAPON ]
		#loop over rows - item slots
		for slot in range(0, EquipmentTable.GetRowCount ()):
			slotname = EquipmentTable.GetRowName (slot)
			item = EquipmentTable.GetValue (slotname, EquipmentColName)
			if item == "*":
				continue
			realslot = GemRB.GetSlots (MyChar, RealSlots[slot], -1)
			
			if realslot == (): # this shouldn't happen!
				continue

			# if an item contains a comma, the rest of the value is the stack
			if "," in item:
				item = item.split(",")
				count = int(item[1])
				item = item[0]
			else:
				count = 0

			targetslot = realslot[0]
			SlotType = GemRB.GetSlotType (targetslot, MyChar)
			i = 1
			# if there are no free slots, CreateItem will create the item on the ground
			while not GemRB.CanUseItemType (SlotType["Type"], item, MyChar) and i < len(realslot):
				targetslot = realslot[i]
				SlotType = GemRB.GetSlotType (targetslot, MyChar)
				i = i + 1

			GemRB.CreateItem(MyChar, item, targetslot, count, 0, 0)
 			GemRB.ChangeItemFlag (MyChar, targetslot, IE_INV_ITEM_IDENTIFIED, OP_OR)

	playmode = GemRB.GetVar ("PlayMode")
	if playmode >=0:
		#LETS PLAY!!
		GemRB.EnterGame()
	else:
		#leaving multi player pregen
		if CharGenWindow:
			CharGenWindow.Unload ()
		#when export is done, go to start
		if GameIsTOB():
			GemRB.SetToken ("NextScript","Start2")
		else:
			GemRB.SetToken ("NextScript","Start")
		GemRB.SetNextScript ("ExportFile") #export
	return

def AddClassAbilities (MyChar, TmpTable):
	TmpTable = GemRB.LoadTableObject (TmpTable)
	for i in range(TmpTable.GetRowCount()):
		ab = TmpTable.GetValue (i, 0) # only add level 1 abilities
		if ab != "****":
			# apply spell (AP_) or gain spell (GA_)
			if ab[:2] == "AP":
				GemRB.ApplySpell (MyChar, ab[3:])
			elif ab[:2] == "GA":
				GemRB.LearnSpell (MyChar, ab[3:])
			else:
				print "ERROR, unknown class ability (type): ", ab
