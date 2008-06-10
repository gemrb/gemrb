#character generation (GUICG 0)
import GemRB
from CharGenCommon import *
from GUICG7 import RemoveKnownSpells

def OnLoad():
	for i in range(0,6):
		GemRB.SetVar("Skill "+str(i),0) #thieving skills
	for i in range(0,32):
		GemRB.SetVar("Prof "+str(i),0)  #proficiencies
	GemRB.SetVar("HateRace",0)

	DisplayOverview (6)

	RemoveKnownSpells (IE_SPELL_TYPE_WIZARD)
	RemoveKnownSpells (IE_SPELL_TYPE_PRIEST)

	# learn divine spells if appropriate
	ClassSkillsTable = GemRB.LoadTable ("clskills")
	#change this to GetPlayerStat once IE_CLASS is directly stored
	ClassTable = GemRB.LoadTable ("classes")
	ClassIndex = GemRB.GetVar ("Class")-1
	Class = GemRB.GetTableValue (ClassTable, ClassIndex, 5)
	MyChar = GemRB.GetVar ("Slot")
	TableName = GemRB.GetTableValue (ClassSkillsTable, Class, 1, 0)

	if TableName == "*":
		# it isn't a cleric or paladin, so check for druids and rangers
		TableName = GemRB.GetTableValue (ClassSkillsTable, Class, 0, 0)
		ClassFlag = 0x4000
	else:
		ClassFlag = 0x8000
	# check for cleric/rangers, who get access to all the spells
	# possibly redundant block (see SPL bit 0x0020)
	if TableName == "MXSPLPRS" and GemRB.GetTableValue (ClassSkillsTable, Class, 0, 0) != "*":
		ClassFlag = 0

	# nulify the memorizable spell counts
	for type in [ "MXSPLPRS", "MXSPLPAL", "MXSPLRAN", "MXSPLDRU" ]:
		UnsetupSpellLevels (MyChar, type, IE_SPELL_TYPE_PRIEST, 1)
	for type in [ "MXSPLWIZ", "MXSPLSRC", "MXSPLBRD" ]:
		UnsetupSpellLevels (MyChar, type, IE_SPELL_TYPE_WIZARD, 1)

	if TableName != "*":
		SetupSpellLevels(MyChar, TableName, IE_SPELL_TYPE_PRIEST, 1)
		Learnable = GetLearnablePriestSpells( ClassFlag, GemRB.GetVar ("Alignment"), 1)
		for i in range(len(Learnable) ):
			GemRB.LearnSpell (MyChar, Learnable[i], 0)
	GemRB.UnloadTable (ClassSkillsTable)
	GemRB.UnloadTable (ClassTable)

	return
