# GemRB - Infinity Engine Emulator
# Copyright (C) 2003 The GemRB Project
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#
#
#character generation (GUICG 0)
import GemRB
from GUIDefines import *
from ie_stats import *
from ie_feats import *
import GUICommon
import CommonTables
import CharOverview

BioWindow = 0
EditControl = 0
AlignmentTable = 0
PortraitName = ""

def OnLoad():
	CharOverview.UpdateOverview(9)
	if CharOverview.CharGenWindow:
		CharOverview.PersistButtons['Next'].SetState(IE_GUI_BUTTON_UNPRESSED) # Fixes button being pre-pressed
	return

def SetRaceAbilities(MyChar, racetitle):
	ability = GemRB.LoadTable ("racespab")
	resource = ability.GetValue (racetitle, "SPECIAL_ABILITIES_FILE")
	if resource=="*":
		return

	ability = GemRB.LoadTable (resource)
	rows = ability.GetRowCount ()
	for i in range(rows):
		resource = ability.GetValue (i, 0)
		count = ability.GetValue (i,1)
		for j in range(count):
			GemRB.LearnSpell (MyChar, resource)
	return

def SetRaceResistances(MyChar, racetitle):
	resistances = GemRB.LoadTable ("racersmd")
	GemRB.SetPlayerStat (MyChar, IE_RESISTFIRE, resistances.GetValue ( racetitle, "FIRE") )
	GemRB.SetPlayerStat (MyChar, IE_RESISTCOLD, resistances.GetValue ( racetitle, "COLD") )
	GemRB.SetPlayerStat (MyChar, IE_RESISTELECTRICITY, resistances.GetValue ( racetitle, "ELEC") )
	GemRB.SetPlayerStat (MyChar, IE_RESISTACID, resistances.GetValue ( racetitle, "ACID") )
	GemRB.SetPlayerStat (MyChar, IE_RESISTMAGIC, resistances.GetValue ( racetitle, "SPELL") )
	GemRB.SetPlayerStat (MyChar, IE_RESISTMAGICFIRE, resistances.GetValue ( racetitle, "MAGIC_FIRE") )
	GemRB.SetPlayerStat (MyChar, IE_RESISTMAGICCOLD, resistances.GetValue ( racetitle, "MAGIC_COLD") )
	GemRB.SetPlayerStat (MyChar, IE_RESISTSLASHING, resistances.GetValue ( racetitle, "SLASHING") )
	GemRB.SetPlayerStat (MyChar, IE_RESISTCRUSHING, resistances.GetValue ( racetitle, "BLUDGEONING") )
	GemRB.SetPlayerStat (MyChar, IE_RESISTPIERCING, resistances.GetValue ( racetitle, "PIERCING") )
	GemRB.SetPlayerStat (MyChar, IE_RESISTMISSILE, resistances.GetValue ( racetitle, "MISSILE") )

	resistances = GemRB.LoadTable ("raceflag")

	#set infravision and nondetection
	orig = GemRB.GetPlayerStat (MyChar, IE_STATE_ID, 0)
	GemRB.SetPlayerStat (MyChar, IE_STATE_ID, orig | resistances.GetValue( racetitle, "FLAG") )

	#set base AC
	orig = GemRB.GetPlayerStat (MyChar, IE_ARMORCLASS, 0)
	GemRB.SetPlayerStat (MyChar, IE_ARMORCLASS, orig + resistances.GetValue( racetitle, "AC") )

	return

def ClearPress():
	global BioData

	GemRB.SetToken("BIO", "")
	EditControl.SetText (GemRB.GetToken("BIO") )
	return

def RevertPress():
	BioTable = GemRB.LoadTable ("bios")
	Class = GemRB.GetVar ("BaseClass")
	StrRef = BioTable.GetValue(Class,1)
	GemRB.SetToken ("BIO", GemRB.GetString(StrRef) )
	EditControl.SetText (GemRB.GetToken("BIO") )
	return

def BioCancelPress():
	GemRB.SetToken("BIO",BioData)
	if BioWindow:
		BioWindow.Unload ()
	return

def BioDonePress():
	if BioWindow:
		BioWindow.Unload ()
	return

def BioPress():
	global BioWindow, EditControl, BioData

	BioData = GemRB.GetToken("BIO")
	BioWindow = Window = GemRB.LoadWindow (51)
	Button = Window.GetControl (5)
	Button.SetText (2240)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, RevertPress)

	Button = Window.GetControl (6)
	Button.SetText (18622)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, ClearPress)

	Button = Window.GetControl (1)
	Button.SetText (11962)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, BioDonePress)

	Button = Window.GetControl (2)
	Button.SetText (36788)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, BioCancelPress)

	EditControl = Window.GetControl (4)
	BioData = GemRB.GetToken("BIO")
	if BioData == "":
		RevertPress()
	else:
		EditControl.SetText (BioData )
	Window.ShowModal (MODAL_SHADOW_GRAY)
	return

def NextPress():
	#set my character up
	MyChar = GemRB.GetVar ("Slot")
	GemRB.SetPlayerStat (MyChar, IE_SEX, GemRB.GetVar ("Gender") )
	GemRB.SetPlayerStat (MyChar, IE_RACE, GemRB.GetVar ("BaseRace") )
	race = GemRB.GetVar ("Race")
	GemRB.SetPlayerStat (MyChar, IE_SUBRACE, race & 255 )
	row = CommonTables.Races.FindValue (3, race )
	racename = CommonTables.Races.GetRowName (row)
	if row!=-1:
		SetRaceResistances( MyChar, racename )
		SetRaceAbilities( MyChar, racename )

	#base class
	Class=GemRB.GetVar ("BaseClass")
	GemRB.SetPlayerStat (MyChar, IE_CLASS, Class)
	#kit
	GemRB.SetPlayerStat (MyChar, IE_KIT, GemRB.GetVar ("Class") )
	AlignmentTable = GemRB.LoadTable ("aligns")
	t=GemRB.GetVar ("Alignment")
	GemRB.SetPlayerStat (MyChar, IE_ALIGNMENT, AlignmentTable.GetValue (t, 3) )
	TmpTable=GemRB.LoadTable ("repstart")
	#t=AlignmentTable.FindValue (3,t)
	t=TmpTable.GetValue (t,0)
	GemRB.SetPlayerStat (MyChar, IE_REPUTATION, t)
	TmpTable=GemRB.LoadTable ("strtgold")
	a = TmpTable.GetValue (Class, 1) #number of dice
	b = TmpTable.GetValue (Class, 0) #size
	c = TmpTable.GetValue (Class, 2) #adjustment
	d = TmpTable.GetValue (Class, 3) #external multiplier
	e = TmpTable.GetValue (Class, 4) #level bonus rate
	t = GemRB.GetPlayerStat (MyChar, IE_LEVEL)
	if t>1:
		e=e*(t-1)
	else:
		e=0
	t = GemRB.Roll(a,b,c)*d+e
	GemRB.SetPlayerStat (MyChar, IE_GOLD, t)
	GemRB.SetPlayerStat (MyChar, IE_HATEDRACE, GemRB.GetVar ("HatedRace") )

	GUICommon.SetColorStat (MyChar, IE_HAIR_COLOR, GemRB.GetVar ("Color1") )
	GUICommon.SetColorStat (MyChar, IE_SKIN_COLOR, GemRB.GetVar ("Color2") )
	GUICommon.SetColorStat (MyChar, IE_MAJOR_COLOR, GemRB.GetVar ("Color4") )
	GUICommon.SetColorStat (MyChar, IE_MINOR_COLOR, GemRB.GetVar ("Color3") )
	GUICommon.SetColorStat (MyChar, IE_METAL_COLOR, 0x1B )
	GUICommon.SetColorStat (MyChar, IE_LEATHER_COLOR, 0x16 )
	GUICommon.SetColorStat (MyChar, IE_ARMOR_COLOR, 0x17 )
	GemRB.SetPlayerStat (MyChar, IE_EA, 2 )
	GemRB.SetPlayerName (MyChar, GemRB.GetToken ("CHARNAME"), 0)

	#setting skills
	TmpTable = GemRB.LoadTable ("skillsta")
	SkillCount = TmpTable.GetRowCount ()
	for i in range (SkillCount):
		StatID=TmpTable.GetValue (i, 2)
		GemRB.SetPlayerStat (MyChar, StatID, GemRB.GetVar ("Skill "+str(i) ) )

	#setting feats
	TmpTable = GemRB.LoadTable ("featreq")
	FeatCount = TmpTable.GetRowCount ()
	ProfOffset = IE_PROFICIENCYBASTARDSWORD
	for i in range (FeatCount):
		GemRB.SetFeat (MyChar, i, GemRB.GetVar ("Feat "+str(i) ) )

	#extra rage
	level = GemRB.GetPlayerStat(MyChar, IE_LEVELBARBARIAN)
	if level>0:
		if level>=15:
			RemoveSpell(MyChar, "SPIN236")
			Spell = "SPIN260"
		else:
			RemoveSpell(MyChar, "SPIN260")
			Spell = "SPIN236"
		cnt = GemRB.GetVar ("Feat 20")+(level+3)/4
		MakeSpellCount(MyChar, Spell, cnt)
	else:
		RemoveSpell(MyChar, "SPIN236")
		RemoveSpell(MyChar, "SPIN260")
	#extra shapeshifting
	#MakeSpellCount(MyChar, "", cnt)

	#feats giving a single innate ability
	SetSpell(MyChar, "SPIN111", FEAT_WILDSHAPE_BOAR)
	SetSpell(MyChar, "SPIN197", FEAT_MAXIMIZED_ATTACKS)
	SetSpell(MyChar, "SPIN231", FEAT_ENVENOM_WEAPON)
	SetSpell(MyChar, "SPIN245", FEAT_WILDSHAPE_PANTHER)
	SetSpell(MyChar, "SPIN246", FEAT_WILDSHAPE_SHAMBLER)
	SetSpell(MyChar, "SPIN275", FEAT_POWER_ATTACK)
	SetSpell(MyChar, "SPIN276", FEAT_EXPERTISE)
	SetSpell(MyChar, "SPIN277", FEAT_ARTERIAL_STRIKE)
	SetSpell(MyChar, "SPIN278", FEAT_HAMSTRING)
	SetSpell(MyChar, "SPIN279", FEAT_RAPID_SHOT)

	#extra smiting
	level = GemRB.GetPlayerStat(MyChar, IE_LEVELPALADIN)
	if level>1:
		cnt = GemRB.GetVar ("Feat 22") + 1
		MakeSpellCount(MyChar, "SPIN152", cnt)
	else:
		RemoveSpell(MyChar, "SPIN152")
	
	#extra turning
	level = GemRB.GetPlayerStat(MyChar, IE_TURNUNDEADLEVEL)
	if level>0:
		cnt = GetAbilityBonus(MyChar, IE_CHR) + 3
		if cnt<1: cnt = 1
		cnt += GemRB.GetVar ("Feat 23")
		MakeSpellCount(MyChar, "SPIN970", cnt)
	else:
		RemoveSpell(MyChar, "SPIN970")

	#stunning fist
	if GemRB.GetVar ("Feat 67"):
		cnt = GemRB.GetPlayerStat(MyChar, IE_CLASSLEVELSUM) / 4
		MakeSpellCount(MyChar, "SPIN232", cnt)
	else:
		RemoveSpell(MyChar, "SPIN232")

	#does all the rest
	LargePortrait = GemRB.GetToken ("LargePortrait")
	SmallPortrait = GemRB.GetToken ("SmallPortrait")
	GemRB.FillPlayerInfo (MyChar, LargePortrait, SmallPortrait)
	GemRB.SetNextScript ("SPPartyFormation")

	TmpTable = GemRB.LoadTable ("strtxp")

	#starting xp is race dependent
	xp = TmpTable.GetValue (racename, "VALUE")
	GemRB.SetPlayerStat (MyChar, IE_XP, xp )

	return

def RemoveSpell(pc, SpellName):
	SpellIndex = Spellbook.HasSpell (pc, IE_SPELL_TYPE_INNATE, 0, SpellName)
	while SpellIndex>=0:
		GemRB.RemoveSpell(pc, IE_SPELL_TYPE_INNATE, 0, SpellIndex)
		SpellIndex = Spellbook.HasSpell (pc, IE_SPELL_TYPE_INNATE, 0, SpellName)
	return

def SetSpell(pc, SpellName, Feat)
	if GemRB.GetVar ("Feat "+Feat):
		MakeSpellCount(pc, SpellName, 1)
	else:
		RemoveSpell(pc, SpellName)
	return
