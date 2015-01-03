# -*-python-*-
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


# MainWindow:
# 0 - main textarea
# 1 - its scrollbar
# 2 - WMCHRP character portrait
# 5 - STALIGN alignment
# 6 - STFCTION faction
# 7,8,9 - STCSTM (info, reform party, level up)
# 0x1000000a - name
#0x1000000b - ac
#0x1000000c, 0x1000000d hp now, hp max
#0x1000000e str
#0x1000000f int
#0x10000010 wis
#0x10000011 dex
#0x10000012 con
#0x10000013 chr
#x10000014 race
#x10000015 sex
#0x10000016 class

#31-36 stat buts
#37 ac but
#38 hp but?

###################################################
import GemRB
import GUICommon
import CommonTables
import GUICommonWindows
from GUIDefines import *
from ie_stats import *
import GUIWORLD

###################################################
LevelUpWindow = None
RecordsWindow = None
InformationWindow = None
BiographyWindow = None

###################################################
def OpenRecordsWindow ():
	global RecordsWindow
	global StatTable

	StatTable = GemRB.LoadTable("abcomm")
	
	if GUICommon.CloseOtherWindow (OpenRecordsWindow):
		GemRB.HideGUI ()
		if InformationWindow: OpenInformationWindow ()
		
		if RecordsWindow:
			RecordsWindow.Unload ()
		RecordsWindow = None
		GemRB.SetVar ("OtherWindow", -1)
		GUICommonWindows.SetSelectionChangeHandler (None)

		GemRB.UnhideGUI ()
		return	

	GemRB.HideGUI ()
	GemRB.LoadWindowPack ("GUIREC")
	RecordsWindow = Window = GemRB.LoadWindow (3)
	GemRB.SetVar ("OtherWindow", RecordsWindow.ID)


	# Information
	Button = Window.GetControl (7)
	Button.SetText (4245)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, OpenInformationWindow)
	
	# Reform Party
	Button = Window.GetControl (8)
	Button.SetText (4244)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, GUIWORLD.OpenReformPartyWindow)

	# Level Up
	Button = Window.GetControl (9)
	Button.SetText (4246)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, OpenLevelUpWindow)

	statevents = (OnRecordsHelpStrength, OnRecordsHelpIntelligence, OnRecordsHelpWisdom, OnRecordsHelpDexterity, OnRecordsHelpConstitution, OnRecordsHelpCharisma)
	# stat buttons
	for i in range (6):
		Button = Window.GetControl (31 + i)
		Button.SetFlags(IE_GUI_BUTTON_NO_IMAGE, OP_SET)
		Button.SetSprites("", 0, 0, 0, 0, 0)
		Button.SetState (IE_GUI_BUTTON_LOCKED)
		Button.SetEvent (IE_GUI_MOUSE_OVER_BUTTON, statevents[i])
		Button.SetEvent (IE_GUI_MOUSE_LEAVE_BUTTON, OnRecordsButtonLeave)

	# AC button
	Button = Window.GetControl (37)
	Button.SetFlags(IE_GUI_BUTTON_NO_IMAGE, OP_SET)
	Button.SetSprites("", 0, 0, 0, 0, 0)
	Button.SetState (IE_GUI_BUTTON_LOCKED)
	Button.SetEvent (IE_GUI_MOUSE_OVER_BUTTON, OnRecordsHelpArmorClass)
	Button.SetEvent (IE_GUI_MOUSE_LEAVE_BUTTON, OnRecordsButtonLeave)


	# HP button
	Button = Window.GetControl (38)
	Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_SET)
	Button.SetSprites ("", 0, 0, 0, 0, 0)
	Button.SetState (IE_GUI_BUTTON_LOCKED)
	Button.SetEvent (IE_GUI_MOUSE_OVER_BUTTON, OnRecordsHelpHitPoints)
	Button.SetEvent (IE_GUI_MOUSE_LEAVE_BUTTON, OnRecordsButtonLeave)


	GUICommonWindows.SetSelectionChangeHandler (UpdateRecordsWindow)
	UpdateRecordsWindow ()

	GemRB.UnhideGUI ()


stats_overview = None
faction_help = ''
alignment_help = ''
avatar_header = {'PrimClass': "", 'SecoClass': "", 'PrimLevel': 0, 'SecoLevel': 0, 'XP': 0, 'PrimNextLevXP': 0, 'SecoNextLevXP': 0}

def UpdateRecordsWindow ():
	global stats_overview, faction_help, alignment_help
	
	Window = RecordsWindow
	if not RecordsWindow:
		print "SelectionChange handler points to non existing window\n"
		return

	pc = GemRB.GameGetSelectedPCSingle ()
	
	# Setting up the character information
	GetCharacterHeader (pc)

	# Checking whether character has leveled up.
	Button = Window.GetControl (9)
	if avatar_header['SecoLevel'] == 0:
		if avatar_header['XP'] >= avatar_header['PrimNextLevXP']:
			Button.SetState (IE_GUI_BUTTON_ENABLED)
		else:
			Button.SetState (IE_GUI_BUTTON_DISABLED)
	else:
		# Character is Multi-Class
		if avatar_header['XP'] >= avatar_header['PrimNextLevXP'] or avatar_header['XP'] >= avatar_header['SecoNextLevXP']:
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

	sstr = GemRB.GetPlayerStat (pc, IE_STR)
	bstr = GemRB.GetPlayerStat (pc, IE_STR,1)
	sstrx = GemRB.GetPlayerStat (pc, IE_STREXTRA)
	bstrx = GemRB.GetPlayerStat (pc, IE_STREXTRA,1)
	if (sstrx > 0) and (sstr==18):
		sstr = "%d/%02d" %(sstr, sstrx % 100)
	if (bstrx > 0) and (bstr==18):
		bstr = "%d/%02d" %(bstr, bstrx % 100)
	sint = GemRB.GetPlayerStat (pc, IE_INT)
	bint = GemRB.GetPlayerStat (pc, IE_INT,1)
	swis = GemRB.GetPlayerStat (pc, IE_WIS)
	bwis = GemRB.GetPlayerStat (pc, IE_WIS,1)
	sdex = GemRB.GetPlayerStat (pc, IE_DEX)
	bdex = GemRB.GetPlayerStat (pc, IE_DEX,1)
	scon = GemRB.GetPlayerStat (pc, IE_CON)
	bcon = GemRB.GetPlayerStat (pc, IE_CON,1)
	schr = GemRB.GetPlayerStat (pc, IE_CHR)
	bchr = GemRB.GetPlayerStat (pc, IE_CHR,1)

	stats = (sstr, sint, swis, sdex, scon, schr)
	basestats = (bstr, bint, bwis, bdex, bcon, bchr)
	for i in range (6):
		Label = Window.GetControl (0x1000000e + i)
		if stats[i]!=basestats[i]:
			Label.SetTextColor (255, 0, 0)
		else:
			Label.SetTextColor (255, 255, 255)
		Label.SetText (str (stats[i]))

	# race
	# HACK: for some strange reason, Morte's race is 1 (Human), instead of 45 (Morte)
	print "species: %d  race: %d" %(GemRB.GetPlayerStat (pc, IE_SPECIES), GemRB.GetPlayerStat (pc, IE_RACE))
	#be careful, some saves got this field corrupted
	race = GemRB.GetPlayerStat (pc, IE_SPECIES) - 1

	text = CommonTables.Races.GetValue (race, 0)
	
	Label = Window.GetControl (0x10000014)
	Label.SetText (text)


	# sex
	GenderTable = GemRB.LoadTable ("GENDERS")
	text = GenderTable.GetValue (GemRB.GetPlayerStat (pc, IE_SEX) - 1, 0)
	
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
	frame = (3 * int (align / 16) + align % 16) - 4
	
	Button = Window.GetControl (5)
	Button.SetState (IE_GUI_BUTTON_LOCKED)
	Button.SetSprites ('STALIGN', 0, frame, 0, 0, 0)
	Button.SetEvent (IE_GUI_MOUSE_OVER_BUTTON, OnRecordsHelpAlignment)
	Button.SetEvent (IE_GUI_MOUSE_LEAVE_BUTTON, OnRecordsButtonLeave)


	# faction
	faction = GemRB.GetPlayerStat (pc, IE_FACTION)
	FactionTable = GemRB.LoadTable ("FACTIONS")
	faction_help = FactionTable.GetValue (faction, 0, GTV_REF)
	frame = FactionTable.GetValue (faction, 1)
	
	Button = Window.GetControl (6)
	Button.SetState (IE_GUI_BUTTON_LOCKED)
	Button.SetSprites ('STFCTION', 0, frame, 0, 0, 0)
	Button.SetEvent (IE_GUI_MOUSE_OVER_BUTTON, OnRecordsHelpFaction)
	Button.SetEvent (IE_GUI_MOUSE_LEAVE_BUTTON, OnRecordsButtonLeave)
	
	# help, info textarea
	stats_overview = GetStatOverview (pc)
	Text = Window.GetControl (0)
	Text.SetText (stats_overview)
	return

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
		TextArea.Append ("\n\n" + GemRB.StatComment (StatTable.GetValue(str(row),col), bon1, bon2) )
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
	Specific = "%d"%GemRB.GetPlayerStat (pc, IE_SPECIFIC)

	#Nameless is Specific == 1
	avatar_header['Specific'] = Specific

	# Nameless is a special case (dual class)
	if Specific == 1:
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
			avatar_header['XP'] = avatar_header['XP'] / 2
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
	str_None = GemRB.GetString (41275)
	
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
	stats.append ((67204, GS (IE_ACSLASHINGMOD), ''))
	#   67205 AC vs. Piercing
	stats.append ((67205, GS (IE_ACPIERCINGMOD), ''))
	#   67206 AC vs. Crushing
	stats.append ((67206, GS (IE_ACCRUSHINGMOD), ''))
	#   67207 AC vs. Missile
	stats.append ((67207, GS (IE_ACMISSILEMOD), ''))
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
	stats.append ((4208, GS (IE_TOHIT), ''))
	#   4209 Number of Attacks
	tmp = GemRB.GetCombatDetails(pc, 0)["APR"]

	if (tmp&1):
		tmp2 = str(tmp/2) + chr(189)
	else:
		tmp2 = str(tmp/2)

	stats.append ((4209, tmp2, ''))
	#   4210 Lore
	stats.append ((4210, GS (IE_LORE), ''))
	#   4211 Open Locks
	stats.append ((4211, GS (IE_LOCKPICKING), '%'))
	#   4212 Stealth
	stats.append ((4212, GS (IE_STEALTH), '%'))
	#   4213 Find/Remove Traps
	stats.append ((4213, GS (IE_TRAPS), '%'))
	#   4214 Pick Pockets
	stats.append ((4214, GS (IE_PICKPOCKET), '%'))
	#   4215 Tracking
	stats.append ((4215, GS (IE_TRACKING), ''))
	#   4216 Reputation
	stats.append ((4216, GS (IE_REPUTATION), ''))
	#   4217 Turn Undead Level
	stats.append ((4217, GS (IE_TURNUNDEADLEVEL), ''))
	#   4218 Lay on Hands Amount
	stats.append ((4218, GS (IE_LAYONHANDSAMOUNT), ''))
	#   4219 Backstab Damage
	stats.append ((4219, GS (IE_BACKSTABDAMAGEMULTIPLIER), 'x'))
	stats.append (None)

	# 4221 Saving Throws
	stats.append (4221)
	#   4222 Paralyze/Poison/Death
	stats.append ((4222, GS (IE_SAVEVSDEATH), ''))
	#   4223 Rod/Staff/Wand
	stats.append ((4223, GS (IE_SAVEVSWANDS), ''))
	#   4224 Petrify/Polymorph
	stats.append ((4224, GS (IE_SAVEVSPOLY), ''))
	#   4225 Breath Weapon
	stats.append ((4225, GS (IE_SAVEVSBREATH), ''))
	#   4226 Spells
	stats.append ((4226, GS (IE_SAVEVSSPELL), ''))
	stats.append (None)
	
	# 4227 Weapon Proficiencies
	stats.append (4227)
	#   55011 Unused Slots
	stats.append ((55011, GS (IE_FREESLOTS), ''))
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
	stats.append (4228)
	#   4229 To Hit
	#   4230 Damage
	#   4231 Open Doors
	#   4232 Weight Allowance
	#   4233 Armor Class Bonus
	#   4234 Missile Adjustment
	stats.append ((4234, GS (IE_ACMISSILEMOD), ''))
	#   4236 CON HP Bonus/Level
	#   4240 Reaction
	stats.append (None)

	# 4238 Magical Defense Adjustment
	stats.append (4238)
	#   4239 Bonus Priest Spells
	stats.append ((4239, GS (IE_CASTINGLEVELBONUSCLERIC), ''))
	stats.append (None)
	
	# 4237 Chance to learn spell
	#SpellLearnChance = won + GemRB.GetString (4237) + woff

	# ??? 4235 Reaction Adjustment

	res = []
	lines = 0
	for s in stats:
		try:
			strref, val, type = s
			if val == 0 and type != '0':
				continue
			if type == '+':
				res.append (GemRB.GetString (strref) + ' '+ '+' * val)
			elif type == 'd': #strref is an already resolved string
				res.append (strref)
			elif type == 'x':
				res.append (GemRB.GetString (strref) + ': x' + str (val))
			else:
				res.append (GemRB.GetString (strref) + ': ' + str (val) + type)
			
			lines = 1
		except:
			if s != None:
				res.append (won + GemRB.GetString (s) + woff)
				lines = 0
			else:
				if not lines:
					res.append (str_None)
				res.append ("")
				lines = 0

	return "\n".join (res)
	pass


def OpenInformationWindow ():
	global InformationWindow
	
	GemRB.HideGUI ()
	
	if InformationWindow != None:
		if BiographyWindow: OpenBiographyWindow ()

		if InformationWindow:
			InformationWindow.Unload ()
		InformationWindow = None
		GemRB.SetVar ("FloatWindow", -1)
		
		GemRB.UnhideGUI()
		return

	InformationWindow = Window = GemRB.LoadWindow (5)
	GemRB.SetVar ("FloatWindow", InformationWindow.ID)

	# Biography
	Button = Window.GetControl (1)
	Button.SetText (4247)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, OpenBiographyWindow)

	# Done
	Button = Window.GetControl (0)
	Button.SetText (1403)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, OpenInformationWindow)
	Button.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)

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
	GUICommon.SetCurrentDateTokens (stat)
	Label.SetText (41277)

	Label = Window.GetControl (0x10000004)
	Label.SetText (stat['FavouriteSpell'])

	Label = Window.GetControl (0x10000005)
	Label.SetText (stat['FavouriteWeapon'])

	Label = Window.GetControl (0x10000006)
	if TotalPartyExp != 0:
		PartyExp = int ((stat['KillsTotalXP'] * 100) / TotalPartyExp)
		Label.SetText (str (PartyExp) + '%')
	else:
		Label.SetText ("0%")

	Label = Window.GetControl (0x10000007)
	if TotalPartyKills != 0:
		PartyKills = int ((stat['KillsTotalCount'] * 100) / TotalPartyKills)
		Label.SetText (str (PartyKills) + '%')
	else:
		Label.SetText ("0%")

	Label = Window.GetControl (0x10000008)
	Label.SetText (str (stat['KillsTotalXP']))

	Label = Window.GetControl (0x10000009)
	Label.SetText (str (stat['KillsTotalCount']))


	Label = Window.GetControl (0x1000000B)
	Label.SetTextColor (255, 255, 255)

	Label = Window.GetControl (0x1000000C)
	Label.SetTextColor (255, 255, 255)

	Label = Window.GetControl (0x1000000D)
	Label.SetTextColor (255, 255, 255)

	Label = Window.GetControl (0x1000000E)
	Label.SetTextColor (255, 255, 255)

	Label = Window.GetControl (0x1000000F)
	Label.SetTextColor (255, 255, 255)

	Label = Window.GetControl (0x10000010)
	Label.SetTextColor (255, 255, 255)

	Label = Window.GetControl (0x10000011)
	Label.SetTextColor (255, 255, 255)

	Label = Window.GetControl (0x10000012)
	Label.SetTextColor (255, 255, 255)

	GemRB.UnhideGUI ()
	Window.ShowModal (MODAL_SHADOW_GRAY)


def OpenBiographyWindow ():
	global BiographyWindow

	GemRB.HideGUI ()
	
	if BiographyWindow != None:
		if BiographyWindow:
			BiographyWindow.Unload ()
		BiographyWindow = None
		GemRB.SetVar ("FloatWindow", InformationWindow.ID)
		
		GemRB.UnhideGUI()
		InformationWindow.ShowModal (MODAL_SHADOW_GRAY)
		return

	BiographyWindow = Window = GemRB.LoadWindow (12)
	GemRB.SetVar ("FloatWindow", BiographyWindow.ID)

	# These are used to get the bio
	pc = GemRB.GameGetSelectedPCSingle ()

	BioTable = GemRB.LoadTable ("bios")
	Specific = GemRB.GetPlayerStat (pc, IE_SPECIFIC)
	BioText = int (BioTable.GetValue (BioTable.GetRowName (Specific+1), 'BIO'))

	TextArea = Window.GetControl (0)
	TextArea.SetText (BioText)

	
	# Done
	Button = Window.GetControl (2)
	Button.SetText (1403)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, OpenBiographyWindow)
	Button.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)
	
	GemRB.UnhideGUI ()
	Window.ShowModal (MODAL_SHADOW_GRAY)
	

def AcceptLevelUp():
	OpenLevelUpWindow()
	#do level up
	pc = GemRB.GameGetSelectedPCSingle ()
	GemRB.SetPlayerStat (pc, IE_SAVEVSDEATH, SavThrows[0])
	GemRB.SetPlayerStat (pc, IE_SAVEVSWANDS, SavThrows[1])
	GemRB.SetPlayerStat (pc, IE_SAVEVSPOLY, SavThrows[2])
	GemRB.SetPlayerStat (pc, IE_SAVEVSBREATH, SavThrows[3])
	GemRB.SetPlayerStat (pc, IE_SAVEVSSPELL, SavThrows[4])
	oldhp = GemRB.GetPlayerStat (pc, IE_HITPOINTS)
	GemRB.SetPlayerStat (pc, IE_HITPOINTS, HPGained+oldhp)
	oldhp = GemRB.GetPlayerStat (pc, IE_MAXHITPOINTS)
	GemRB.SetPlayerStat (pc, IE_MAXHITPOINTS, HPGained+oldhp)
	#increase weapon proficiency if needed
	if WeapProfType!=-1:
		GemRB.SetPlayerStat (pc, WeapProfType, CurrWeapProf + WeapProfGained )
	Specific = GemRB.GetPlayerStat (pc, IE_SPECIFIC)
	if Specific == 1:
		#TODO:
		#the nameless one is dual classed
		#so we have to determine which level to increase
		GemRB.SetPlayerStat (pc, IE_LEVEL, GemRB.GetPlayerStat (pc, IE_LEVEL)+NumOfPrimLevUp)
	else:
		GemRB.SetPlayerStat (pc, IE_LEVEL, GemRB.GetPlayerStat (pc, IE_LEVEL)+NumOfPrimLevUp)
		if avatar_header['SecoLevel'] != 0:
			GemRB.SetPlayerStat (pc, IE_LEVEL2, GemRB.GetPlayerStat (pc, IE_LEVEL2)+NumOfSecoLevUp)
	UpdateRecordsWindow ()


def OpenLevelUpWindow ():
	global LevelUpWindow
	global SavThrows
	global HPGained
	global WeapProfType, CurrWeapProf, WeapProfGained
	global NumOfPrimLevUp, NumOfSecoLevUp

	GemRB.HideGUI ()

	if LevelUpWindow != None:
		if LevelUpWindow:
			LevelUpWindow.Unload ()
		LevelUpWindow = None
		GemRB.SetVar ("FloatWindow", -1)

		GemRB.UnhideGUI()
		return

	LevelUpWindow = Window = GemRB.LoadWindow (4)
	GemRB.SetVar ("FloatWindow", LevelUpWindow.ID)

	# Accept
	Button = Window.GetControl (0)
	Button.SetText (4192)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, AcceptLevelUp)

	pc = GemRB.GameGetSelectedPCSingle ()

	# These are used to identify Nameless One
	BioTable = GemRB.LoadTable ("bios")
	Specific = GemRB.GetPlayerStat (pc, IE_SPECIFIC)
	AvatarName = BioTable.GetRowName (Specific+1)

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

	ClasWeapTable = GemRB.LoadTable ("weapprof")
	WeapProfType = -1
	CurrWeapProf = -1
	#This does not apply to Nameless since he uses unused slots system
	#Nameless is Specific == 1
	if Specific != "1":
		# Searching for the column name where value is 1
		for i in range (5):
			WeapProfName = ClasWeapTable.GetRowName (i)
			value = ClasWeapTable.GetValue (AvatarName, WeapProfName)
			if value == 1:
				WeapProfType = i
				break

	if WeapProfType!=-1:
		CurrWeapProf = GemRB.GetPlayerStat (pc, IE_WEAPPROF+WeapProfType)

	# Recording this avatar's current proficiency level
	# Since Nameless one is not covered, hammer and club can't occur
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

	# Thief Skills
	# For now we shall set them to the current levels and disable the increment buttons
	# Points Left
	Label = Window.GetControl (0x10000006)
	Label.SetText ('0')
	# Stealth
	Label = Window.GetControl (0x10000008)
	Label.SetText (str (GemRB.GetPlayerStat (pc, IE_STEALTH)) + '%')
	# Detect Traps
	Label = Window.GetControl (0x1000000A)
	Label.SetText (str (GemRB.GetPlayerStat (pc, IE_TRAPS)) + '%')
	# Pick Pockets
	Label = Window.GetControl (0x1000000C)
	Label.SetText (str (GemRB.GetPlayerStat (pc, IE_PICKPOCKET)) + '%')
	# Open Doors
	Label = Window.GetControl (0x1000000E)
	Label.SetText (str (GemRB.GetPlayerStat (pc, IE_LOCKPICKING)) + '%')
	# Plus and Minus buttons
	for i in range (8):
		Button = Window.GetControl (16 + i)
		Button.SetState (IE_GUI_BUTTON_LOCKED)

	# Is avatar multi-class?
	if avatar_header['SecoLevel'] == 0:
		# avatar is single class
		# What will be avatar's next level?
		NextLevel = avatar_header['PrimLevel'] + 1
		while avatar_header['XP'] >= GetNextLevelExp (NextLevel, Class):
			NextLevel = NextLevel + 1
		NumOfPrimLevUp = NextLevel - avatar_header['PrimLevel'] # How many levels did we go up?

		# Is avatar Nameless One?
		if Specific == "1":
			# Saving Throws
			# Nameless One gets the best possible throws from all the classes except Priest
			FigSavThrTable = GemRB.LoadTable ("SAVEWAR")
			MagSavThrTable = GemRB.LoadTable ("SAVEWIZ")
			ThiSavThrTable = GemRB.LoadTable ("SAVEROG")
			# For Nameless, we also need to know his levels in every class, so that we pick
			# the right values from the corresponding tables. Also we substract one, so
			# that we may use them as column indices in tables.
			FighterLevel = GemRB.GetPlayerStat (pc, IE_LEVEL) - 1
			MageLevel = GemRB.GetPlayerStat (pc, IE_LEVEL2) - 1
			ThiefLevel = GemRB.GetPlayerStat (pc, IE_LEVEL3) - 1
			# this is the constitution bonus type for this level
			CONType = 1
			# We are leveling up one of those levels. Therefore, one of them has to be updated.
			if avatar_header['PrimClass'] == "FIGHTER":
				FighterLevel = NextLevel - 1
				CONType = 0
			elif avatar_header['PrimClass'] == "MAGE":
				MageLevel = NextLevel - 1
			else:
				ThiefLevel = NextLevel - 1

			ConHPBon = GetConHPBonus (pc, NumOfPrimLevUp, 0, CONType)
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
			# Cleaning up
		else:
			#How many weapon procifiencies we get
			for i in range (NumOfPrimLevUp):
				if HasGainedWeapProf (pc, CurrWeapProf + WeapProfGained, avatar_header['PrimLevel'] + i, avatar_header['PrimClass']):
					WeapProfGained += 1

			# Saving Throws
			# Loading the right saving throw table
			SavThrTable = GemRB.LoadTable (CommonTables.Classes.GetValue (Class, "SAVE"))
			# Updating the current saving throws. They are changed only if the
			# new ones are better than current. The smaller the number, the better.
			# We need to substract one from the NextLevel, so that we get right values.
			# We also need to check if NextLevel is larger than 21, since after that point
			# the table runs out, and the throws remain the same 
			if NextLevel < 22:
				for i in range (5):
					Throw = SavThrTable.GetValue (i, NextLevel-1)
					if Throw < SavThrows[i]:
						SavThrows[i] = Throw
						SavThrUpdated = True
			# Cleaning Up

			# Hit Points Gained and Hit Points from Constitution Bonus
			for i in range (NumOfPrimLevUp):
				HPGained = HPGained + GetSingleClassHP (Class, avatar_header['PrimLevel'])

			if avatar_header['PrimClass'] == "FIGHTER":
				CONType = 0
			else:
				CONType = 1
			ConHPBon = GetConHPBonus (pc, NumOfPrimLevUp, 0, CONType)

			# Thac0
			Thac0 = GetThac0 (Class, NextLevel)
			# Is the new thac0 better than old? (The smaller, the better)
			if Thac0 < GemRB.GetPlayerStat (pc, IE_TOHIT, 1):
				Thac0Updated = True

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
			if HasGainedWeapProf (pc, CurrWeapProf + WeapProfGained, avatar_header['PrimLevel'] + i, avatar_header['PrimClass']):
				WeapProfGained += 1

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
			HPGained = HPGained + GetSingleClassHP ("FIGHTER", avatar_header['PrimLevel'])/2

		for i in range (NumOfSecoLevUp):
			HPGained = HPGained + GetSingleClassHP (Class, avatar_header['SecoLevel'])/2
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
		overview = overview + '+' + str (WeapProfGained) + ' ' + GemRB.GetString (WeapProfDispStr) + '\n'

	overview = overview + str (HPGained) + " " + GemRB.GetString (38713) + '\n'
	overview = overview + str (ConHPBon) + " " + GemRB.GetString (38727) + '\n'

	if SavThrUpdated:
		overview = overview + GemRB.GetString (38719) + '\n'
	if Thac0Updated:
		overview = overview + GemRB.GetString (38718) + '\n'

	Text = Window.GetControl (3)
	Text.SetText (overview)

	GemRB.UnhideGUI ()
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
	return ConHPBonTable.GetValue (con, "WARRIOR") * numPrimLevels / 2 + ConHPBonTable.GetValue (con, "OTHER") * numSecoLevels / 2

def GetThac0 (Class, Level):
	Thac0Table = GemRB.LoadTable ("THAC0")
	# We need to check if Level is larger than 60, since after that point
	# the table runs out, and the value remains the same.
	if Level > 60:
		Level = 60

	return Thac0Table.GetValue (Class, str (Level))

#apparently the original code doesn't follow profsmax/profs tables
def HasGainedWeapProf (pc, currProf, currLevel, Class):
	#only fighters gain weapon proficiencies
	if Class!="FIGHTER":
		return False
	#hardcoded limit is 4
	if currProf>3:
		return False
	if CurrProf>(currLevel-1)/3:
		return False
	return True

###################################################
# End of file GUIREC.py
