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


# GUIREC.py - scripts to control stats/records windows from GUIREC winpack

# GUIREC:
# 0,1,2 - common windows (time, message, menu)
# 3 - main statistics window
# 4 - level up win
# 5 - info - kills, weapons ...
# 6 - dual class list ???
# 7 - class panel
# 8 - skills panel
# 9 - choose mage spells panel
# 10 - some small win, 1 button
# 11 - some small win, 2 buttons
# 12 - biography?
# 13 - specialist mage panel
# 14 - proficiencies
# 15 - some 2 panel window
# 16 - some 2 panel window
# 17 - some 2 panel window

###################################################
import GemRB
import Game
import GUICommon
import CommonTables
import LevelUp
import LUCommon
import GUICommonWindows
import GUIRECCommon
import NewLife
import PartyReform
from GUIDefines import *
from ie_stats import *
import GUIWORLD
import LUSkillsSelection

###################################################
LevelUpWindow = None
RecordsWindow = None
InformationWindow = None
BiographyWindow = None

###################################################

LevelDiff = 0
Level = 0
Classes = 0
NumClasses = 0
NewCharacteristicPts = 0
LevelStats = { "FIGHTER" : IE_LEVEL , "MAGE": IE_LEVEL2, "THIEF": IE_LEVEL3 }

###################################################

def InitRecordsWindow (Window):
	global RecordsWindow
	global StatTable

	RecordsWindow = Window
	StatTable = GemRB.LoadTable("abcomm")

	# Information
	Button = Window.GetControl (7)
	Button.SetText (4245)
	Button.OnPress (OpenInformationWindow)

	# Reform Party
	Button = Window.GetControl (8)
	Button.SetText (4244)
	Button.OnPress (PartyReform.OpenReformPartyWindow)

	# Level Up
	Button = Window.GetControl (9)
	Button.SetText (4246)
	Button.OnPress (OpenLevelUpWindow)

	statevents = (OnRecordsHelpStrength, OnRecordsHelpIntelligence, OnRecordsHelpWisdom, OnRecordsHelpDexterity, OnRecordsHelpConstitution, OnRecordsHelpCharisma)
	# stat buttons
	for i in range (6):
		Button = Window.GetControl (31 + i)
		Button.SetFlags(IE_GUI_BUTTON_NO_IMAGE, OP_SET)
		Button.SetSprites("", 0, 0, 0, 0, 0)
		Button.SetState (IE_GUI_BUTTON_LOCKED)
		Button.OnMouseEnter (statevents[i])
		Button.OnMouseLeave (OnRecordsButtonLeave)

	# AC button
	Button = Window.GetControl (37)
	Button.SetFlags(IE_GUI_BUTTON_NO_IMAGE, OP_SET)
	Button.SetSprites("", 0, 0, 0, 0, 0)
	Button.SetState (IE_GUI_BUTTON_LOCKED)
	Button.OnMouseEnter (OnRecordsHelpArmorClass)
	Button.OnMouseLeave (OnRecordsButtonLeave)


	# HP button
	Button = Window.GetControl (38)
	Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_SET)
	Button.SetSprites ("", 0, 0, 0, 0, 0)
	Button.SetState (IE_GUI_BUTTON_LOCKED)
	Button.OnMouseEnter (OnRecordsHelpHitPoints)
	Button.OnMouseLeave (OnRecordsButtonLeave)

	return


stats_overview = None
faction_help = ''
alignment_help = ''
avatar_header = {'PrimClass': "", 'SecoClass': "", 'PrimLevel': 0, 'SecoLevel': 0, 'XP': 0, 'PrimNextLevXP': 0, 'SecoNextLevXP': 0}

def UpdateRecordsWindow (Window):
	global stats_overview, faction_help, alignment_help

	pc = GemRB.GameGetSelectedPCSingle ()

	# Setting up the character information
	GetCharacterHeader (pc)

	# Checking whether character has leveled up.
	Button = Window.GetControl (9)
	if LUCommon.CanLevelUp (pc):
		Button.SetState (IE_GUI_BUTTON_ENABLED)
	else:
		Button.SetState (IE_GUI_BUTTON_DISABLED)

	# name
	Label = Window.GetControl (0x1000000a)
	Label.SetText (GemRB.GetPlayerName (pc, 1))

	# portrait
	Image = Window.GetControl (2)
	Image.SetState (IE_GUI_BUTTON_LOCKED)
	Image.SetFlags(IE_GUI_BUTTON_NO_IMAGE | IE_GUI_BUTTON_PICTURE, OP_SET)
	Image.SetPicture (GUICommonWindows.GetActorPortrait (pc, 'STATS'))

	# armorclass
	Label = Window.GetControl (0x1000000b)
	Label.SetText (str (GemRB.GetPlayerStat (pc, IE_ARMORCLASS)))
	Label.SetTooltip (4197)

	# hp now
	Label = Window.GetControl (0x1000000c)
	Label.SetText (str (GemRB.GetPlayerStat (pc, IE_HITPOINTS)))
	Label.SetTooltip (4198)

	# hp max
	Label = Window.GetControl (0x1000000d)
	Label.SetText (str (GemRB.GetPlayerStat (pc, IE_MAXHITPOINTS)))
	Label.SetTooltip (4199)

	# stats
	statIdx = [ IE_STR, IE_INT, IE_WIS, IE_DEX, IE_CON, IE_CHR ]
	stats = [ GemRB.GetPlayerStat (pc, stat) for stat in statIdx ]
	basestats = [ GemRB.GetPlayerStat (pc, stat, 1) for stat in statIdx ]

	sstrx = GemRB.GetPlayerStat (pc, IE_STREXTRA)
	bstrx = GemRB.GetPlayerStat (pc, IE_STREXTRA, 1)
	if sstrx > 0 and stats[0] == 18:
		stats[0] = "%d/%02d" %(stats[0], sstrx % 100)
	if bstrx > 0 and basestats[0] == 18:
		basestats[0] = "%d/%02d" %(basestats[0], bstrx % 100)

	for i in range (6):
		Label = Window.GetControl (0x1000000e + i)
		if stats[i]!=basestats[i]:
			Label.SetColor ({'r' : 255, 'g' : 0, 'b' : 0})
		else:
			Label.SetColor ({'r' : 255, 'g' : 255, 'b' : 255})
		Label.SetText (str (stats[i]))

	# race

	# this is -1 to lookup the value in the table
	race = GemRB.GetPlayerStat (pc, IE_SPECIES) - 1

	# workaround for original saves that don't have the characters species stat set properly
	if race == -1:
		if GemRB.GetPlayerStat (pc, IE_SPECIFIC) == 3: # Vhailor
			race = 50 # Changes from GHOST to RESTLESS_SPIRIT
		elif  GemRB.GetPlayerStat (pc, IE_SPECIFIC) == 9: # Morte
			race = 44 # Changes from HUMAN to MORTE. Morte is Morte :)
		else:
			race = GemRB.GetPlayerStat (pc, IE_RACE) - 1

	text = CommonTables.Races.GetValue (race, 0)

	Label = Window.GetControl (0x10000014)
	Label.SetText (text)


	# sex
	GenderTable = GemRB.LoadTable ("GENDERS")
	text = GenderTable.GetValue (GemRB.GetPlayerStat (pc, IE_SEX) - 1, GTV_STR)

	Label = Window.GetControl (0x10000015)
	Label.SetText (text)


	# class
	text = CommonTables.Classes.GetValue (GUICommon.GetClassRowName (pc), "NAME_REF")

	Label = Window.GetControl (0x10000016)
	Label.SetText (text)

	# alignment
	align = GemRB.GetPlayerStat (pc, IE_ALIGNMENT)
	ss = GemRB.LoadSymbol ("ALIGN")
	sym = ss.GetValue (align)

	AlignmentTable = GemRB.LoadTable ("ALIGNS")
	alignment_help = AlignmentTable.GetValue (sym, 'DESC_REF', GTV_REF)
	frame = (3 * align // 16 + align % 16) - 4

	Button = Window.GetControl (5)
	Button.SetState (IE_GUI_BUTTON_LOCKED)
	Button.SetSprites ('STALIGN', 0, frame, 0, 0, 0)
	Button.OnMouseEnter (OnRecordsHelpAlignment)
	Button.OnMouseLeave (OnRecordsButtonLeave)


	# faction
	faction = GemRB.GetPlayerStat (pc, IE_FACTION)
	FactionTable = GemRB.LoadTable ("FACTIONS")
	faction_help = FactionTable.GetValue (faction, 0, GTV_REF)
	frame = FactionTable.GetValue (faction, 1)

	Button = Window.GetControl (6)
	Button.SetState (IE_GUI_BUTTON_LOCKED)
	Button.SetSprites ('STFCTION', 0, frame, 0, 0, 0)
	Button.OnMouseEnter (OnRecordsHelpFaction)
	Button.OnMouseLeave (OnRecordsButtonLeave)

	# help, info textarea
	stats_overview = GetStatOverview (pc)
	Text = Window.GetControl (0)
	Text.SetText (stats_overview)
	return

ToggleRecordsWindow = GUICommonWindows.CreateTopWinLoader(3, "GUIREC", GUICommonWindows.ToggleWindow, InitRecordsWindow, UpdateRecordsWindow, True)
OpenRecordsWindow = GUICommonWindows.CreateTopWinLoader(3, "GUIREC", GUICommonWindows.OpenWindowOnce, InitRecordsWindow, UpdateRecordsWindow, True)

# puts default info to textarea (overview of PC's bonuses, saves, etc.
def OnRecordsButtonLeave ():
	OnRecordsHelpStat (-1, 0, stats_overview)
	return

def OnRecordsHelpFaction ():
	Help = GemRB.GetString (20106) + "\n\n" + faction_help
	OnRecordsHelpStat (-1, 0, Help)
	return

def OnRecordsHelpArmorClass ():
	OnRecordsHelpStat (-1, 0, 18493)
	return

def OnRecordsHelpHitPoints ():
	OnRecordsHelpStat (-1, 0, 18494)
	return

def OnRecordsHelpAlignment ():
	Help = GemRB.GetString (20105) + "\n\n" + alignment_help
	OnRecordsHelpStat (-1, 0, Help)
	return

#Bio:
# 38787 no
# 39423 morte
# 39424 annah
# 39425 dakkon
# 39426 ffg
# 39427 ignus
# 39428 nordom
# 39429 vhailor

def OnRecordsHelpStat (row, col, strref, bon1=0, bon2=0):
	TextArea = RecordsWindow.GetControl (0)
	TextArea.SetText (strref)
	if row != -1:
		TextArea.Append ("\n\n" + GemRB.GetString(StatTable.GetValue(str(row),col), STRING_FLAGS_RESOLVE_TAGS).format(bon1, bon2))
	return

def OnRecordsHelpStrength ():
	# These are used to get the stats
	pc = GemRB.GameGetSelectedPCSingle ()

	# Getting the character's strength
	s = GemRB.GetPlayerStat (pc, IE_STR)
	e = GemRB.GetPlayerStat (pc, IE_STREXTRA)

	x = CommonTables.StrMod.GetValue(s, 0) + CommonTables.StrModEx.GetValue(e, 0)
	y = CommonTables.StrMod.GetValue(s, 1) + CommonTables.StrModEx.GetValue(e, 1)
	if x==0:
		x=y
		y=0
	if e>60:
		s=19

	OnRecordsHelpStat (s, "STR", 18489, x, y)
	return

def OnRecordsHelpDexterity ():
	# Loading table of modifications
	Table = GemRB.LoadTable("dexmod")

	# These are used to get the stats
	pc = GemRB.GameGetSelectedPCSingle ()

	# Getting the character's dexterity
	Dex = GemRB.GetPlayerStat (pc, IE_DEX)

	# Getting the dexterity description
	x = -Table.GetValue(Dex,2)

	OnRecordsHelpStat (Dex, "DEX", 18487, x)
	return

def OnRecordsHelpIntelligence ():
	# These are used to get the stats
	pc = GemRB.GameGetSelectedPCSingle ()

	# Getting the character's intelligence
	Int = GemRB.GetPlayerStat (pc, IE_INT)
	OnRecordsHelpStat (Int, "INT", 18488)
	return

def OnRecordsHelpWisdom ():
	# These are used to get the stats
	pc = GemRB.GameGetSelectedPCSingle ()

	# Getting the character's wisdom
	Wis = GemRB.GetPlayerStat (pc, IE_WIS)
	OnRecordsHelpStat (Wis, "WIS", 18490)
	return

def OnRecordsHelpConstitution ():
	# Loading table of modifications
	Table = GemRB.LoadTable("hpconbon")

	# These are used to get the stats
	pc = GemRB.GameGetSelectedPCSingle ()

	# Getting the character's constitution
	Con = GemRB.GetPlayerStat (pc, IE_CON)

	# Getting the constitution description
	x = Table.GetValue(Con-1,1)

	OnRecordsHelpStat (Con, "CON", 18491, x)
	return

def OnRecordsHelpCharisma ():
	# These are used to get the stats
	pc = GemRB.GameGetSelectedPCSingle ()

	# Getting the character's charisma
	Cha = GemRB.GetPlayerStat (pc, IE_CHR)
	OnRecordsHelpStat (Cha, "CHR", 1903)
	return

def GetCharacterHeader (pc):
	global avatar_header

	Class = GemRB.GetPlayerStat (pc, IE_CLASS) - 1
	Multi = GUICommon.HasMultiClassBits (pc)

	# Nameless is a special case (dual class)
	if GUICommon.IsNamelessOne(pc):
		avatar_header['PrimClass'] = CommonTables.Classes.GetRowName (Class)
		avatar_header['SecoClass'] = "*"

		avatar_header['SecoLevel'] = 0

		if avatar_header['PrimClass'] == "FIGHTER":
			avatar_header['PrimLevel'] = GemRB.GetPlayerStat (pc, IE_LEVEL)
			avatar_header['XP'] = GemRB.GetPlayerStat (pc, IE_XP)
		elif avatar_header['PrimClass'] == "MAGE":
			avatar_header['PrimLevel'] = GemRB.GetPlayerStat (pc, IE_LEVEL2)
			avatar_header['XP'] = GemRB.GetPlayerStat (pc, IE_XP_MAGE)
		else:
			avatar_header['PrimLevel'] = GemRB.GetPlayerStat (pc, IE_LEVEL3)
			avatar_header['XP'] = GemRB.GetPlayerStat (pc, IE_XP_THIEF)

		avatar_header['PrimNextLevXP'] = GetNextLevelExp (avatar_header['PrimLevel'], avatar_header['PrimClass'])
		avatar_header['SecoNextLevXP'] = 0
	else:
		# PC is not NAMELESS_ONE
		avatar_header['PrimLevel'] = GemRB.GetPlayerStat (pc, IE_LEVEL)
		avatar_header['XP'] = GemRB.GetPlayerStat (pc, IE_XP)
		if Multi:
			avatar_header['XP'] = avatar_header['XP'] // 2
			avatar_header['SecoLevel'] = GemRB.GetPlayerStat (pc, IE_LEVEL2)

			avatar_header['PrimClass'] = "FIGHTER"
			if Multi == 3:
				#fighter/mage
				Class = 0
			else:
				#fighter/thief
				Class = 3
			avatar_header['SecoClass'] = CommonTables.Classes.GetRowName (Class)

			avatar_header['PrimNextLevXP'] = GetNextLevelExp (avatar_header['PrimLevel'], avatar_header['PrimClass'])
			avatar_header['SecoNextLevXP'] = GetNextLevelExp (avatar_header['SecoLevel'], avatar_header['SecoClass'])

			# Converting to the displayable format
			avatar_header['SecoClass'] = CommonTables.Classes.GetValue (avatar_header['SecoClass'], "NAME_REF", GTV_REF)
		else:
			avatar_header['SecoLevel'] = 0
			avatar_header['PrimClass'] = CommonTables.Classes.GetRowName (Class)
			avatar_header['SecoClass'] = "*"
			avatar_header['PrimNextLevXP'] = GetNextLevelExp (avatar_header['PrimLevel'], avatar_header['PrimClass'])
			avatar_header['SecoNextLevXP'] = 0

	# Converting to the displayable format
	avatar_header['PrimClass'] = CommonTables.Classes.GetValue (avatar_header['PrimClass'], "NAME_REF", GTV_REF)



def GetNextLevelExp (Level, Class):
	if (Level < 20):
		NextLevel = CommonTables.NextLevel.GetValue (Class, str (Level + 1))
	else:
		After21ExpTable = GemRB.LoadTable ("LVL21PLS")
		ExpGap = After21ExpTable.GetValue (Class, 'XPGAP')
		LevDiff = Level - 19
		Lev20Exp = CommonTables.NextLevel.GetValue (Class, "20")
		NextLevel = Lev20Exp + (LevDiff * ExpGap)

	return NextLevel


def GetStatOverview (pc):
	won = "[color=FFFFFF]"
	woff = "[/color]"

	GB = lambda s, pc=pc: GemRB.GetPlayerStat (pc, s, 1)
	GS = lambda s, pc=pc: GemRB.GetPlayerStat (pc, s)

	stats = []

	# Displaying Class, Level, Experience and Next Level Experience
	if (avatar_header['SecoLevel'] == 0):
		stats.append ((avatar_header['PrimClass'], "", 'd'))
		stats.append ((48156, avatar_header['PrimLevel'], ''))
		stats.append ((19673, avatar_header['XP'], ''))
		stats.append ((19674, avatar_header['PrimNextLevXP'], ''))
	else:
		stats.append ((19414, "", ''))
		stats.append (None)
		stats.append ((avatar_header['PrimClass'], "", 'd'))
		stats.append ((48156, avatar_header['PrimLevel'], ''))
		stats.append ((19673, avatar_header['XP'], ''))
		stats.append ((19674, avatar_header['PrimNextLevXP'], ''))
		stats.append (None)
		stats.append ((avatar_header['SecoClass'], "", 'd'))
		stats.append ((48156, avatar_header['SecoLevel'], ''))
		stats.append ((19673, avatar_header['XP'], ''))
		stats.append ((19674, avatar_header['SecoNextLevXP'], ''))

	# 59856 Current State
	stats.append (None)
	StatesTable = GemRB.LoadTable ("states")
	StateID = GS (IE_STATE_ID)
	State = StatesTable.GetValue (str (StateID), "NAME_REF", GTV_REF)
	stats.append ((won + GemRB.GetString (59856) + woff, "", 'd'))
	stats.append ((State, "", 'd'))
	stats.append (None)

	# 67049 AC Bonuses
	stats.append (67049)
	#   67204 AC vs. Slashing
	stats.append ((67204, -1 * GS (IE_ACSLASHINGMOD), 'p'))
	#   67205 AC vs. Piercing
	stats.append ((67205, -1 * GS (IE_ACPIERCINGMOD), 'p'))
	#   67206 AC vs. Crushing
	stats.append ((67206, -1 * GS (IE_ACCRUSHINGMOD), 'p'))
	#   67207 AC vs. Missile
	stats.append ((67207, -1 * GS (IE_ACMISSILEMOD), 'p'))
	stats.append (None)


	# 67208 Resistances
	stats.append (67208)
	#   67209 Normal Fire
	stats.append ((67209, GS (IE_RESISTFIRE), '%'))
	#   67210 Magic Fire
	stats.append ((67210, GS (IE_RESISTMAGICFIRE), '%'))
	#   67211 Normal Cold
	stats.append ((67211, GS (IE_RESISTCOLD), '%'))
	#   67212 Magic Cold
	stats.append ((67212, GS (IE_RESISTMAGICCOLD), '%'))
	#   67213 Electricity
	stats.append ((67213, GS (IE_RESISTELECTRICITY), '%'))
	#   67214 Acid
	stats.append ((67214, GS (IE_RESISTACID), '%'))
	#   67215 Magic
	stats.append ((67215, GS (IE_RESISTMAGIC), '%'))
	#   67216 Slashing Attacks
	stats.append ((67216, GS (IE_RESISTSLASHING), '%'))
	#   67217 Piercing Attacks
	stats.append ((67217, GS (IE_RESISTPIERCING), '%'))
	#   67218 Crushing Attacks
	stats.append ((67218, GS (IE_RESISTCRUSHING), '%'))
	#   67219 Missile Attacks
	stats.append ((67219, GS (IE_RESISTMISSILE), '%'))
	stats.append (None)

	# 4220 Proficiencies
	stats.append (4220)
	#   4208 THAC0
	stats.append ((4208, IE_TOHIT, 's'))
	#   4209 Number of Attacks
	tmp = GemRB.GetCombatDetails(pc, 0)["APR"]

	if (tmp&1):
		tmp2 = str(tmp // 2) + chr(189)
	else:
		tmp2 = str(tmp // 2)

	stats.append ((4209, tmp2, ''))
	#   4210 Lore
	stats.append ((4210, GS (IE_LORE), ''))
	#   4211 Open Locks
	className = GUICommon.GetClassRowName (pc)
	stats.append ((4211, GUIRECCommon.GetValidSkill (pc, className, IE_LOCKPICKING), '%'))
	#   4212 Stealth
	stats.append ((4212, GUIRECCommon.GetValidSkill (pc, className, IE_STEALTH), '%'))
	#   4213 Find/Remove Traps
	stats.append ((4213, GUIRECCommon.GetValidSkill (pc, className, IE_TRAPS), '%'))
	#   4214 Pick Pockets
	stats.append ((4214, GUIRECCommon.GetValidSkill (pc, className, IE_PICKPOCKET), '%'))
	#   4215 Tracking
	stats.append ((4215, GS (IE_TRACKING), ''))
	#   4216 Reputation - not shown
	#   4217 Turn Undead Level - unused
	#   4218 Lay on Hands Amount
	stats.append ((4218, GS (IE_LAYONHANDSAMOUNT), ''))
	#   4219 Backstab Damage
	if "THIEF" in className: # for TNO display it only if he's currently a thief
		stats.append ((4219, GS (IE_BACKSTABDAMAGEMULTIPLIER), 'x'))
	stats.append (None)

	# 4221 Saving Throws
	stats.append (4221)
	#   4222 Paralyze/Poison/Death
	stats.append ((4222, IE_SAVEVSDEATH, 's'))
	#   4223 Rod/Staff/Wand
	stats.append ((4223, IE_SAVEVSWANDS, 's'))
	#   4224 Petrify/Polymorph
	stats.append ((4224, IE_SAVEVSPOLY, 's'))
	#   4225 Breath Weapon
	stats.append ((4225, IE_SAVEVSBREATH, 's'))
	#   4226 Spells
	stats.append ((4226, IE_SAVEVSSPELL, 's'))
	stats.append (None)

	# 4227 Weapon Proficiencies
	stats.append (4227)
	#   55011 Unused Slots
	if GUICommon.IsNamelessOne (pc):
		stats.append ((55011, GS (IE_FREESLOTS), '0'))
	#   33642 Fist
	stats.append ((33642, GS (IE_PROFICIENCYBASTARDSWORD), '+'))
	#   33649 Edged Weapon
	stats.append ((33649, GS (IE_PROFICIENCYLONGSWORD), '+'))
	#   33651 Hammer
	stats.append ((33651, GS (IE_PROFICIENCYSHORTSWORD), '+'))
	#   44990 Axe
	stats.append ((44990, GS (IE_PROFICIENCYAXE), '+'))
	#   33653 Club
	stats.append ((33653, GS (IE_PROFICIENCYTWOHANDEDSWORD), '+'))
	#   33655 Bow
	stats.append ((33655, GS (IE_PROFICIENCYKATANA), '+'))
	stats.append (None)

	# 4228 Ability Bonuses
	abbon = GUIRECCommon.GetAbilityBonuses (pc, False)
	stats += abbon
	stats.append (None)

	# 4238 Magical Defense Adjustment
	if "CLERIC" in className:
		stats.append (4238)
		abbon = GUIRECCommon.GetBonusSpells (pc, False)
		stats += abbon
		stats.append (None)

	statDesc = GUIRECCommon.TypeSetStats (stats, pc, True)
	return statDesc

def OpenInformationWindow ():
	global InformationWindow

	if InformationWindow != None:
		if BiographyWindow: OpenBiographyWindow ()

		if InformationWindow:
			InformationWindow.Close ()
		InformationWindow = None

		return

	InformationWindow = Window = GemRB.LoadWindow (5, "GUIREC")

	# Biography
	Button = Window.GetControl (1)
	Button.SetText (4247)
	Button.OnPress (OpenBiographyWindow)

	# Done
	Button = Window.GetControl (0)
	Button.SetText (1403)
	Button.OnPress (OpenInformationWindow)
	Button.MakeEscape()

	TotalPartyExp = 0
	TotalPartyKills = 0
	for i in range (1, GemRB.GetPartySize() + 1):
		stat = GemRB.GetPCStats(i)
		TotalPartyExp = TotalPartyExp + stat['KillsTotalXP']
		TotalPartyKills = TotalPartyKills + stat['KillsTotalCount']

	# These are used to get the stats
	pc = GemRB.GameGetSelectedPCSingle ()
	stat = GemRB.GetPCStats (pc)

	Label = Window.GetControl (0x10000001)
	Label.SetText (GemRB.GetPlayerName (pc, 1))

	# class
	ClassTitle = GUICommon.GetActorClassTitle (pc)
	Label = Window.GetControl (0x1000000A)
	Label.SetText (ClassTitle)

	Label = Window.GetControl (0x10000002)
	if stat['BestKilledName'] == -1:
		Label.SetText (GemRB.GetString (41275))
	else:
		Label.SetText (GemRB.GetString (stat['BestKilledName']))

	Label = Window.GetControl (0x10000003)
	GUICommon.SetCurrentDateTokens (stat, True)
	Label.SetText (41277)

	Label = Window.GetControl (0x10000004)
	Label.SetText (stat['FavouriteSpell'])

	Label = Window.GetControl (0x10000005)
	Label.SetText (stat['FavouriteWeapon'])

	Label = Window.GetControl (0x10000006)
	if TotalPartyExp != 0:
		PartyExp = (stat['KillsTotalXP'] * 100) // TotalPartyExp
		Label.SetText (str (PartyExp) + '%')
	else:
		Label.SetText ("0%")

	Label = Window.GetControl (0x10000007)
	if TotalPartyKills != 0:
		PartyKills = (stat['KillsTotalCount'] * 100) // TotalPartyKills
		Label.SetText (str (PartyKills) + '%')
	else:
		Label.SetText ("0%")

	Label = Window.GetControl (0x10000008)
	Label.SetText (str (stat['KillsTotalXP']))

	Label = Window.GetControl (0x10000009)
	Label.SetText (str (stat['KillsTotalCount']))

	White = {'r' : 255, 'g' : 255, 'b' : 255}
	Label = Window.GetControl (0x1000000B)
	Label.SetColor (White)

	Label = Window.GetControl (0x1000000C)
	Label.SetColor (White)

	Label = Window.GetControl (0x1000000D)
	Label.SetColor (White)

	Label = Window.GetControl (0x1000000E)
	Label.SetColor (White)

	Label = Window.GetControl (0x1000000F)
	Label.SetColor (White)

	Label = Window.GetControl (0x10000010)
	Label.SetColor (White)

	Label = Window.GetControl (0x10000011)
	Label.SetColor (White)

	Label = Window.GetControl (0x10000012)
	Label.SetColor (White)

	Window.ShowModal (MODAL_SHADOW_GRAY)


def OpenBiographyWindow ():
	global BiographyWindow

	if BiographyWindow != None:
		if BiographyWindow:
			BiographyWindow.Close ()
		BiographyWindow = None

		InformationWindow.ShowModal (MODAL_SHADOW_GRAY)
		return

	BiographyWindow = Window = GemRB.LoadWindow (12, "GUIREC")

	# These are used to get the bio
	pc = GemRB.GameGetSelectedPCSingle ()

	BioTable = GemRB.LoadTable ("bios")
	Specific = GemRB.GetPlayerStat (pc, IE_SPECIFIC)
	BioText = int (BioTable.GetValue (BioTable.GetRowName (Specific), 'BIO'))

	TextArea = Window.GetControl (0)
	TextArea.SetText (BioText)


	# Done
	Button = Window.GetControl (2)
	Button.SetText (1403)
	Button.OnPress (OpenBiographyWindow)
	Button.MakeEscape()

	Window.ShowModal (MODAL_SHADOW_GRAY)


def AcceptLevelUp():
	#do level up
	pc = GemRB.GameGetSelectedPCSingle ()
	GemRB.SetPlayerStat (pc, IE_SAVEVSDEATH, SavThrows[0])
	GemRB.SetPlayerStat (pc, IE_SAVEVSWANDS, SavThrows[1])
	GemRB.SetPlayerStat (pc, IE_SAVEVSPOLY, SavThrows[2])
	GemRB.SetPlayerStat (pc, IE_SAVEVSBREATH, SavThrows[3])
	GemRB.SetPlayerStat (pc, IE_SAVEVSSPELL, SavThrows[4])
	oldhp = GemRB.GetPlayerStat (pc, IE_MAXHITPOINTS, 1)
	GemRB.SetPlayerStat (pc, IE_MAXHITPOINTS, HPGained+oldhp)
	oldhp = GemRB.GetPlayerStat (pc, IE_HITPOINTS, 1)
	GemRB.SetPlayerStat (pc, IE_HITPOINTS, HPGained+oldhp)

	# Weapon Proficiencies
	if WeapProfType != -1:
		# Companion NPC's get points added directly to their chosen weapon
		GemRB.SetPlayerStat (pc, IE_PROFICIENCYBASTARDSWORD+WeapProfType, CurrWeapProf + WeapProfGained)
	else:
		# TNO has points added to the "Unused Slots" dummy proficiency
		freeSlots = GemRB.GetPlayerStat(pc, IE_FREESLOTS)
		GemRB.SetPlayerStat (pc, IE_FREESLOTS, freeSlots + WeapProfGained)

	SwitcherClass = GUICommon.NamelessOneClass(pc)
	if SwitcherClass:
		# Handle saving of TNO class level in the correct CRE stat
		NewLevel = avatar_header['PrimLevel'] + NumOfPrimLevUp
		GemRB.SetPlayerStat (pc, LevelStats[SwitcherClass], NewLevel)
	else:
		GemRB.SetPlayerStat (pc, IE_LEVEL, GemRB.GetPlayerStat (pc, IE_LEVEL)+NumOfPrimLevUp)
		Game.CheckKarachUpgrade (pc, 0, NumOfPrimLevUp)
		if avatar_header['SecoLevel'] != 0:
			GemRB.SetPlayerStat (pc, IE_LEVEL2, GemRB.GetPlayerStat (pc, IE_LEVEL2)+NumOfSecoLevUp)
	
	LUSkillsSelection.SkillsSave (pc)

	# Spells
	LevelUp.pc = pc
	LevelUp.Classes = Classes
	LevelUp.NumClasses = NumClasses
	# (we need to override the globals this function uses there since they wouldn't have been set)
	LevelUp.SaveNewSpells()

	LevelUpWindow.Close()
	NewLife.OpenLUStatsWindow(True, NewCharacteristicPts)

def HandleSpecializationBonuses (pc, className, oldLevel, newLevel):
	global feedbackText
	# GLOBAL["Specialist"] = 1 // Fighter | was the first class to level 7
	# GLOBAL["Specialist"] = 2 // Thief   | was the first class to level 7
	# GLOBAL["Specialist"] = 3 // Mage    | was the first class to level 7
	#
	# GLOBAL["Specialist"] = 4 // Fighter | was the first class to level 7 and level 12
	# GLOBAL["Specialist"] = 5 // Thief   | was the first class to level 7 and level 12
	# GLOBAL["Specialist"] = 6 // Mage    | was the first class to level 7 and level 12
	#
	# GLOBAL["Specialist"] = 7 // Fighter | was the first class to level 12 (but not to level 7)
	# GLOBAL["Specialist"] = 8 // Thief   | was the first class to level 12 (but not to level 7)
	# GLOBAL["Specialist"] = 9 // Mage    | was the first class to level 12 (but not to level 7)
	specialist = GemRB.GetGameVar ("Specialist") or 0
	if specialist >= 4:
		# all upgrades have been done already
		return ""

	feedbackText = ""

	def bumpStat (pc, stat, diff, delay = 0):
		global feedbackText

		base = GemRB.GetPlayerStat (pc, stat, 1)
		GemRB.SetPlayerStat (pc, stat, base + diff)

		# the original applied effects and relied on their feedback for floating messages
		# that and PermanentStatChangeFeedback would have the same problem
		# but it also printed all the feedback to the text area
		statMsgs = { IE_STR: 41274, IE_CON: 41273, IE_DEX: 41276, IE_INT: 39440, IE_WIS: 41271, IE_LUCK: 41272, IE_LORE: 39439, IE_HITPOINTS: 39438 }
		if stat in statMsgs:
			if statMsgs[stat] >= statMsgs[IE_INT]:
				action = "FloatMessage({}, {})".format(pc, statMsgs[stat] + 1000000)
				if delay:
					# stagger messages, so they don't overlap
					GemRB.SetTimedEvent(lambda: GemRB.ExecuteString (action, pc), delay)
				else:
					GemRB.ExecuteString (action, pc)
			feedbackText += GemRB.GetString (statMsgs[stat]) + " "
		return

	def grantFirstBonus (pc, className, specBase = 0):
		if className == "FIGHTER":
			bumpStat (pc, IE_STR, 1)
			# Unlocks Proficiency 4 for a Weapon; this is handled by trainers checking the Specialist global
			specVal = 1
		elif className == "THIEF":
			bumpStat (pc, IE_DEX, 1)
			specVal = 2
		else:
			bumpStat (pc, IE_INT, 1)
			specVal = 3
		GemRB.SetGlobal ("Specialist", "GLOBAL", specBase + specVal)
		return

	if specialist == 0:
		if newLevel < 7:
			return feedbackText

		# we just (b)reached 7 for the first time, give the first bonus
		grantFirstBonus (pc, className)
		return feedbackText

	# from here on specialist is 1, 2 or 3
	if newLevel < 12:
		return feedbackText

	# we just (b)reached 12, give the second bonus
	if className == "FIGHTER":
		if specialist == 1:
			# 4: upgrade fighter specialization
			bumpStat (pc, IE_STR, 1)
			bumpStat (pc, IE_CON, 1, 2)
			bumpStat (pc, IE_MAXHITPOINTS, 3)
			bumpStat (pc, IE_HITPOINTS, 3) # we can't display it the same way, but it wasn't drawn in the original either
			# Unlocks Proficiency 5 for a Weapon; this is handled by trainers checking the Specialist global
			GemRB.SetGlobal ("Specialist", "GLOBAL", 4)
		else:
			# 7: was mage or thief, grant first fighter specialization
			grantFirstBonus (pc, className, 6)
	elif className == "THIEF":
		if specialist == 2:
			# 5: upgrade thief specialization
			bumpStat (pc, IE_DEX, 2)
			bumpStat (pc, IE_LUCK, 1, 2)
			GemRB.SetGlobal ("Specialist", "GLOBAL", 5)
		else:
			# 8: was mage or fighter, grant first thief specialization
			grantFirstBonus (pc, className, 6)
	else:
		if specialist == 3:
			# 6: upgrade mage specialization
			bumpStat (pc, IE_INT, 2)
			bumpStat (pc, IE_WIS, 1, 2)
			bumpStat (pc, IE_LORE, 5)
			GemRB.SetGlobal ("Specialist", "GLOBAL", 6)
		else:
			# 9: was thief or fighter, grant first mage specialization
			grantFirstBonus (pc, className, 6)
	return feedbackText

def RedrawSkills():
	DoneButton = LevelUpWindow.GetControl(0)

	if GemRB.GetVar ("SkillPointsLeft") == 0:
		DoneButton.SetState(IE_GUI_BUTTON_ENABLED)
	else:
		DoneButton.SetState(IE_GUI_BUTTON_DISABLED)
	return

def OpenLevelUpWindow ():
	global LevelUpWindow
	global SavThrows
	global HPGained
	global WeapProfType, CurrWeapProf, WeapProfGained
	global NumOfPrimLevUp, NumOfSecoLevUp

	global LevelDiff, Level, Classes, NumClasses, NewCharacteristicPts

	LevelUpWindow = Window = GemRB.LoadWindow (4, "GUIREC") # since we get called from NewLife

	# Accept
	Button = Window.GetControl (0)
	Button.SetText (4192)
	Button.OnPress (AcceptLevelUp)

	pc = GemRB.GameGetSelectedPCSingle ()

	# These are used to identify Nameless One
	BioTable = GemRB.LoadTable ("bios")
	Specific = GemRB.GetPlayerStat (pc, IE_SPECIFIC)
	AvatarName = BioTable.GetRowName (Specific)

	# These will be used for saving throws
	SavThrUpdated = False
	SavThrows = [0,0,0,0,0]
	SavThrows[0] = GemRB.GetPlayerStat (pc, IE_SAVEVSDEATH)
	SavThrows[1] = GemRB.GetPlayerStat (pc, IE_SAVEVSWANDS)
	SavThrows[2] = GemRB.GetPlayerStat (pc, IE_SAVEVSPOLY)
	SavThrows[3] = GemRB.GetPlayerStat (pc, IE_SAVEVSBREATH)
	SavThrows[4] = GemRB.GetPlayerStat (pc, IE_SAVEVSSPELL)

	HPGained = 0
	ConHPBon = 0
	Thac0Updated = False
	Thac0 = 0
	WeapProfGained = 0

	WeapProfType = -1
	CurrWeapProf = 0

	# Count the number of existing weapon procifiencies
	if GUICommon.IsNamelessOne(pc):
		# TNO: Count the total amount of unassigned proficiencies
		CurrWeapProf = GemRB.GetPlayerStat(pc, IE_FREESLOTS)
	else:
		# Scan the weapprof table for the characters favoured weapon proficiency (WeapProfType)
		# This does not apply to Nameless since he uses unused slots system
		for i in range (6):
			WeapProfName = CommonTables.WeapProfs.GetRowName (i)
			value = CommonTables.WeapProfs.GetValue (WeapProfName,AvatarName)
			if value == 1:
				WeapProfType = i
				break

	for i in range (6):
		CurrWeapProf += GemRB.GetPlayerStat (pc, IE_PROFICIENCYBASTARDSWORD + i)

	# What is the avatar's class (Which we can use to lookup XP)
	Class = GUICommon.GetClassRowName (pc)

	# name
	Label = Window.GetControl (0x10000000)
	Label.SetText (GemRB.GetPlayerName (pc, 1))

	# class
	Label = Window.GetControl (0x10000001)
	Label.SetText (CommonTables.Classes.GetValue (Class, "NAME_REF"))

	# Armor Class
	Label = Window.GetControl (0x10000023)
	Label.SetText (str (GemRB.GetPlayerStat (pc, IE_ARMORCLASS)))

	# our multiclass variables
	IsMulti = GUICommon.IsMultiClassed (pc, 1)
	Classes = [IsMulti[1], IsMulti[2], IsMulti[3]]
	NumClasses = IsMulti[0] # 2 or 3 if IsMulti; 0 otherwise
	IsMulti = NumClasses > 1

	if not IsMulti:
		NumClasses = 1
		Classes = [GemRB.GetPlayerStat (pc, IE_CLASS)]

	if GUICommon.IsNamelessOne(pc):
		# Override the multiclass info for TNO
		IsMulti = 1
		NumClasses = 3
		# Fighter, Mage, Thief ID
		Classes = [2, 1, 4]

	Level = LUCommon.GetNextLevels(pc, Classes)
	LevelDiff = LUCommon.GetLevelDiff(pc, Level)

	# calculate the new spells (results are stored in global variables in LevelUp)
	LevelUp.GetNewSpells(pc, Classes, Level, LevelDiff)

	# Thief Skills
	Level1 = []
	for i in range (len (Level)):
		Level1.append (Level[i]-LevelDiff[i])
	LUSkillsSelection.SetupSkillsWindow (pc, LUSkillsSelection.LUSKILLS_TYPE_LEVELUP, LevelUpWindow, RedrawSkills, Level1, Level, 0, False)
	RedrawSkills()

	# Is avatar multi-class?
	if avatar_header['SecoLevel'] == 0:
		# avatar is single class
		# What will be avatar's next level?
		NextLevel = avatar_header['PrimLevel'] + 1
		while avatar_header['XP'] >= GetNextLevelExp (NextLevel, Class):
			NextLevel = NextLevel + 1
		NumOfPrimLevUp = NextLevel - avatar_header['PrimLevel'] # How many levels did we go up?

		#How many weapon procifiencies we get
		for i in range (NumOfPrimLevUp):
			WeapProfGained += GainedWeapProfs (pc, CurrWeapProf + WeapProfGained, avatar_header['PrimLevel'] + i, AvatarName)

		# Hit Points Gained and Hit Points from Constitution Bonus
		for i in range (NumOfPrimLevUp):
			HPGained = HPGained + GetSingleClassHP (Class, avatar_header['PrimLevel'])
		if Class == "FIGHTER":
			CONType = 0
		else:
			CONType = 1
		ConHPBon = GetConHPBonus (pc, NumOfPrimLevUp, 0, CONType)

		# Thac0
		Thac0 = GetThac0 (Class, NextLevel)
		# Is the new thac0 better than old? (The smaller, the better)
		if Thac0 < GemRB.GetPlayerStat (pc, IE_TOHIT, 1):
			Thac0Updated = True

		# Saving Throws
		if GUICommon.IsNamelessOne(pc):
			# Nameless One always uses the best possible save from each class
			FigSavThrTable = GemRB.LoadTable ("SAVEWAR")
			MagSavThrTable = GemRB.LoadTable ("SAVEWIZ")
			ThiSavThrTable = GemRB.LoadTable ("SAVEROG")

			FighterLevel = GemRB.GetPlayerStat (pc, IE_LEVEL) - 1
			MageLevel = GemRB.GetPlayerStat (pc, IE_LEVEL2) - 1
			ThiefLevel = GemRB.GetPlayerStat (pc, IE_LEVEL3) - 1

			# We are leveling up one of those levels. Therefore, one of them has to be updated.
			if Class == "FIGHTER":
				FighterLevel = NextLevel - 1
			elif Class == "MAGE":
				MageLevel = NextLevel - 1
			else:
				ThiefLevel = NextLevel - 1

			# Now we need to update the saving throws with the best values from those tables.
			# The smaller the number, the better saving throw it is.
			# We also need to check if any of the levels are larger than 21, since
			# after that point the table runs out, and the throws remain the
			# same
			if FighterLevel < 21:
				for i in range (5):
					Throw = FigSavThrTable.GetValue (i, FighterLevel)
					if Throw < SavThrows[i]:
						SavThrows[i] = Throw
						SavThrUpdated = True
			if MageLevel < 21:
				for i in range (5):
					Throw = MagSavThrTable.GetValue (i, MageLevel)
					if Throw < SavThrows[i]:
						SavThrows[i] = Throw
						SavThrUpdated = True
			if ThiefLevel < 21:
				for i in range (5):
					Throw = ThiSavThrTable.GetValue (i, ThiefLevel)
					if Throw < SavThrows[i]:
						SavThrows[i] = Throw
						SavThrUpdated = True
		else:
			SavThrTable = GemRB.LoadTable (CommonTables.Classes.GetValue (Class, "SAVE"))
			# Updating the current saving throws. They are changed only if the
			# new ones are better than current. The smaller the number, the better.
			# We need to subtract one from the NextLevel, so that we get right values.
			# We also need to check if NextLevel is larger than 21, since after that point
			# the table runs out, and the throws remain the same
			if NextLevel < 22:
				for i in range (5):
					Throw = SavThrTable.GetValue (i, NextLevel-1)
					if Throw < SavThrows[i]:
						SavThrows[i] = Throw
						SavThrUpdated = True



	else:
		# avatar is multi class
		# we have only fighter/X multiclasses, so this
		# part is a bit hardcoded
		PrimNextLevel = 0
		SecoNextLevel = 0
		NumOfPrimLevUp = 0
		NumOfSecoLevUp = 0

		# What will be avatar's next levels?
		PrimNextLevel = avatar_header['PrimLevel']
		while avatar_header['XP'] >= GetNextLevelExp (PrimNextLevel, "FIGHTER"):
			PrimNextLevel = PrimNextLevel + 1
		# How many primary levels did we go up?
		NumOfPrimLevUp = PrimNextLevel - avatar_header['PrimLevel']

		for i in range (NumOfPrimLevUp):
			WeapProfGained += GainedWeapProfs (pc, CurrWeapProf + WeapProfGained, avatar_header['PrimLevel'] + i, AvatarName)

		# Saving Throws
		FigSavThrTable = GemRB.LoadTable ("SAVEWAR")
		if PrimNextLevel < 22:
			for i in range (5):
				Throw = FigSavThrTable.GetValue (i, PrimNextLevel - 1)
				if Throw < SavThrows[i]:
					SavThrows[i] = Throw
					SavThrUpdated = True
		# Which multi class is it?
		if GemRB.GetPlayerStat (pc, IE_CLASS) == 7:
			# avatar is Fighter/Mage (Dak'kon)
			Class = "MAGE"
			SavThrTable = GemRB.LoadTable ("SAVEWIZ")
		else:
			# avatar is Fighter/Thief (Annah)
			Class = "THIEF"
			SavThrTable = GemRB.LoadTable ("SAVEROG")

		SecoNextLevel = avatar_header['SecoLevel']
		while avatar_header['XP'] >= GetNextLevelExp (SecoNextLevel, Class):
			SecoNextLevel = SecoNextLevel + 1
		# How many secondary levels did we go up?
		NumOfSecoLevUp = SecoNextLevel - avatar_header['SecoLevel']
		if SecoNextLevel < 22:
			for i in range (5):
				Throw = SavThrTable.GetValue (i, SecoNextLevel - 1)
				if Throw < SavThrows[i]:
					SavThrows[i] = Throw
					SavThrUpdated = True

		# Hit Points Gained and Hit Points from Constitution Bonus (multiclass)
		for i in range (NumOfPrimLevUp):
			HPGained = HPGained + GetSingleClassHP ("FIGHTER", avatar_header['PrimLevel']) // 2

		for i in range (NumOfSecoLevUp):
			HPGained = HPGained + GetSingleClassHP (Class, avatar_header['SecoLevel']) // 2
		ConHPBon = GetConHPBonus (pc, NumOfPrimLevUp, NumOfSecoLevUp, 2)

		# Thac0
		# Multi class use the primary class level to determine Thac0
		Thac0 = GetThac0 (Class, PrimNextLevel)
		# Is the new thac0 better than old? (The smaller the better)
		if Thac0 < GemRB.GetPlayerStat (pc, IE_TOHIT, 1):
			Thac0Updated = True


	# Displaying the saving throws
	# Death
	Label = Window.GetControl (0x10000019)
	Label.SetText (str (SavThrows[0]))
	# Wand
	Label = Window.GetControl (0x1000001B)
	Label.SetText (str (SavThrows[1]))
	# Polymorph
	Label = Window.GetControl (0x1000001D)
	Label.SetText (str (SavThrows[2]))
	# Breath
	Label = Window.GetControl (0x1000001F)
	Label.SetText (str (SavThrows[3]))
	# Spell
	Label = Window.GetControl (0x10000021)
	Label.SetText (str (SavThrows[4]))

	FinalCurHP = GemRB.GetPlayerStat (pc, IE_HITPOINTS) + HPGained
	FinalMaxHP = GemRB.GetPlayerStat (pc, IE_MAXHITPOINTS) + HPGained

	# Current HP
	Label = Window.GetControl (0x10000025)
	Label.SetText (str (FinalCurHP))

	# Max HP
	Label = Window.GetControl (0x10000027)
	Label.SetText (str (FinalMaxHP))

	# Displaying level up info
	overview = ""
	if CurrWeapProf!=-1 and WeapProfGained>0:
		# pc dependent feedback: TNO gets generic, everyone else their chosen weapon
		profRef = 38715
		if WeapProfType != -1:
			WeapProfName = CommonTables.WeapProfs.GetRowName (WeapProfType)
			profRef = CommonTables.WeapProfs.GetValue (WeapProfName, "NAME_REF", GTV_INT)
		overview = overview + '+' + str (WeapProfGained) + " " + GemRB.GetString (profRef) + '\n'

	# +x characteristic points gained
	# but only grant extra attribute points the first time we reach a level
	if GUICommon.IsNamelessOne (pc):
		MaxOldLevel = max(Level1)
		NewLevel = avatar_header['PrimLevel'] + NumOfPrimLevUp
		NewCharacteristicPts = 0 if NewLevel <= MaxOldLevel else NewLevel - MaxOldLevel
		if NewCharacteristicPts:
			overview += str (NewCharacteristicPts) + " "  + GemRB.GetString (38714) + '\n'

		# specialization bonuses and feedback
		specOverview = HandleSpecializationBonuses (pc, Class, avatar_header['PrimLevel'], NewLevel)
		overview += "" if specOverview == "" else specOverview + '\n'

	if SavThrUpdated:
		overview = overview + GemRB.GetString (38719) + '\n'

	overview = overview + str (HPGained) + " " + GemRB.GetString (38713) + '\n'
	if ConHPBon:
		overview = overview + str (ConHPBon) + " " + GemRB.GetString (38727) + '\n'

	if Thac0Updated:
		GemRB.SetPlayerStat (pc, IE_TOHIT, Thac0)
		overview = overview + GemRB.GetString (38718) + '\n'

	Text = Window.GetControl (3)
	Text.SetText (overview)

	Window.ShowModal (MODAL_SHADOW_GRAY)

def GetSingleClassHP (Class, Level):
	HPTable = GemRB.LoadTable (CommonTables.Classes.GetValue (Class, "HP"))

	# We need to check if Level is larger than 20, since after that point
	# the table runs out, and the formula remain the same.
	if Level > 20:
		Level = 20

	# We need the Level as a string, so that we can use the column names
	Level = str (Level)

	Sides = HPTable.GetValue (Level, "SIDES")
	Rolls = HPTable.GetValue (Level, "ROLLS")
	Modif = HPTable.GetValue (Level, "MODIFIER")


	return GemRB.Roll (Rolls, Sides, Modif)

def GetConHPBonus (pc, numPrimLevels, numSecoLevels, levelUpType):
	ConHPBonTable = GemRB.LoadTable ("HPCONBON")

	con = str (GemRB.GetPlayerStat (pc, IE_CON))
	if levelUpType == 0:
		# Pure fighter
		return ConHPBonTable.GetValue (con, "WARRIOR") * numPrimLevels
	if levelUpType == 1:
		# Mage, Priest or Thief
		return ConHPBonTable.GetValue (con, "OTHER") * numPrimLevels
	return ConHPBonTable.GetValue (con, "WARRIOR") * numPrimLevels // 2 + ConHPBonTable.GetValue (con, "OTHER") * numSecoLevels // 2

def GetThac0 (Class, Level):
	Thac0Table = GemRB.LoadTable ("THAC0")
	# We need to check if Level is larger than 60, since after that point
	# the table runs out, and the value remains the same.
	if Level > 60:
		Level = 60

	return Thac0Table.GetValue (Class, str (Level))

# each gained level is checked for how many new prof points gained
def GainedWeapProfs (pc, currProf, currLevel, AvatarName):

	# Actually looking at the next level
	nextLevel = currLevel + 1

	# The table stops at level 20
	if nextLevel < 21:
		maxProf = CommonTables.CharProfs.GetValue(str(nextLevel), AvatarName)
		return maxProf - currProf

	# Nameless continues gaining points forever	at a rate of 1 every 3 levels
	elif GUICommon.IsNamelessOne(pc) and (currProf - 3) <= (nextLevel // 3):
		return 1

	return 0

###################################################
# End of file GUIREC.py
