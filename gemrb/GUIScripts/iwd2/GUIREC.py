# -*-python-*-
# GemRB - Infinity Engine Emulator
# Copyright (C) 2003-2004 The GemRB Project
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


# GUIREC.py - scripts to control character record windows from GUIREC winpack

###################################################

import GemRB
import GUICommon
import CommonTables
import GUICommonWindows
import GUIRECCommon
from GUIDefines import *
from ie_stats import *
from ie_restype import *
from ie_feats import FEAT_WEAPON_FINESSE, FEAT_ARMORED_ARCANA

SelectWindow = 0
Topic = None
HelpTable = None
DescTable = None
RecordsWindow = None
RecordsTextArea = None
ItemInfoWindow = None
ItemAmountWindow = None
ItemIdentifyWindow = None
InformationWindow = None
PortraitWindow = None
OptionsWindow = None
OldPortraitWindow = None
OldOptionsWindow = None
BonusSpellTable = None
HateRaceTable = None

#barbarian, bard, cleric, druid, fighter, monk, paladin, ranger, rogue, sorcerer, wizard
Classes = [IE_LEVELBARBARIAN, IE_LEVELBARD, IE_LEVELCLERIC, IE_LEVELDRUID, \
IE_LEVEL, IE_LEVELMONK, IE_LEVELPALADIN, IE_LEVELRANGER, IE_LEVEL3, \
IE_LEVELSORCERER, IE_LEVEL2]

#don't allow exporting polymorphed or dead characters
def Exportable(pc):
	if not (GemRB.GetPlayerStat (pc, IE_MC_FLAGS)&MC_EXPORTABLE): return False
	if GemRB.GetPlayerStat (pc, IE_POLYMORPHED): return False
	if GemRB.GetPlayerStat (pc, IE_STATE_ID)&STATE_DEAD: return False
	return True

def OpenRecordsWindow ():
	global RecordsWindow, OptionsWindow, PortraitWindow
	global OldPortraitWindow, OldOptionsWindow, SelectWindow
	global BonusSpellTable, HateRaceTable

	if GUICommon.CloseOtherWindow (OpenRecordsWindow):
		if RecordsWindow:
			RecordsWindow.Unload ()
		if OptionsWindow:
			OptionsWindow.Unload ()
		if PortraitWindow:
			PortraitWindow.Unload ()

		RecordsWindow = None
		GemRB.SetVar ("OtherWindow", -1)
		GUICommon.GameWindow.SetVisible(WINDOW_VISIBLE)
		GemRB.UnhideGUI ()
		GUICommonWindows.PortraitWindow = OldPortraitWindow
		OldPortraitWindow = None
		GUICommonWindows.OptionsWindow = OldOptionsWindow
		OldOptionsWindow = None
		GUICommonWindows.SetSelectionChangeHandler (None)
		return

	GemRB.HideGUI ()
	GUICommon.GameWindow.SetVisible(WINDOW_INVISIBLE)

	GemRB.LoadWindowPack ("GUIREC", 800, 600)
	RecordsWindow = Window = GemRB.LoadWindow (2)
	GemRB.SetVar ("OtherWindow", RecordsWindow.ID)
	#saving the original portrait window
	OldPortraitWindow = GUICommonWindows.PortraitWindow
	PortraitWindow = GUICommonWindows.OpenPortraitWindow ()
	OldOptionsWindow = GUICommonWindows.OptionsWindow
	OptionsWindow = GemRB.LoadWindow (0)
	GUICommonWindows.SetupMenuWindowControls (OptionsWindow, 0, OpenRecordsWindow)
	Window.SetFrame ()

	if not BonusSpellTable:
		BonusSpellTable = GemRB.LoadTable ("mxsplbon")
	if not HateRaceTable:
		HateRaceTable = GemRB.LoadTable ("haterace")

	#portrait icon
	Button = Window.GetControl (2)
	Button.SetState (IE_GUI_BUTTON_LOCKED)
	Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE | IE_GUI_BUTTON_PICTURE, OP_SET)

	#information (help files)
	Button = Window.GetControl (1)
	Button.SetText (11946)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, OpenHelpWindow)

	#biography
	Button = Window.GetControl (59)
	Button.SetText (18003)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, GUIRECCommon.OpenBiographyWindow)

	#export
	Button = Window.GetControl (36)
	Button.SetText (13956)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, GUIRECCommon.OpenExportWindow)

	#customize
	Button = Window.GetControl (50)
	Button.SetText (10645)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, GUIRECCommon.OpenCustomizeWindow)

	#general
	GemRB.SetVar ("SelectWindow", 1)

	Button = Window.GetControl (60)
	Button.SetTooltip (40316)
	Button.SetVarAssoc ("SelectWindow", 1)
	Button.SetFlags (IE_GUI_BUTTON_RADIOBUTTON, OP_OR)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, UpdateRecordsWindow)

	#weapons and armour
	Button = Window.GetControl (61)
	Button.SetTooltip (40317)
	Button.SetVarAssoc ("SelectWindow", 2)
	Button.SetFlags (IE_GUI_BUTTON_RADIOBUTTON, OP_OR)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, UpdateRecordsWindow)

	#skills and feats
	Button = Window.GetControl (62)
	Button.SetTooltip (40318)
	Button.SetVarAssoc ("SelectWindow", 3)
	Button.SetFlags (IE_GUI_BUTTON_RADIOBUTTON, OP_OR)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, UpdateRecordsWindow)

	#miscellaneous
	Button = Window.GetControl (63)
	Button.SetTooltip (33500)
	Button.SetVarAssoc ("SelectWindow", 4)
	Button.SetFlags (IE_GUI_BUTTON_RADIOBUTTON, OP_OR)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, UpdateRecordsWindow)

	#level up
	Button = Window.GetControl (37)
	Button.SetText (7175)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, UpdateRecordsWindow) #TODO: OpenLevelUpWindow

	GUICommonWindows.SetSelectionChangeHandler (UpdateRecordsWindow)

	UpdateRecordsWindow ()
	return

def ColorDiff (Window, Label, diff):
	if diff>0:
		Label.SetTextColor (0, 255, 0)
	elif diff<0:
		Label.SetTextColor (255, 0, 0)
	else:
		Label.SetTextColor (255, 255, 255)
	return

def ColorDiff2 (Window, Label, diff):
	if diff:
		Label.SetTextColor (255, 255, 0)
	else:
		Label.SetTextColor (255, 255, 255)
	return

def GetBonusSpells (pc):
	bonusSpells = {}
	classes = []
	# cheack each class/kit
	for i in range(11):
		level = GemRB.GetPlayerStat (pc, Classes[i])
		if not level:
			continue

		ClassTitle = GUICommonWindows.GetActorClassTitle (pc, i)
		# find the casting stat
		ClassName = GUICommon.GetClassRowName (i, "index")
		Stat = CommonTables.ClassSkills.GetValue (ClassName, "CASTING")
		if Stat == "*":
			continue
		Stat = GemRB.GetPlayerStat (pc, Stat)
		if Stat < 12: # boni start with positive modifiers
			continue

		# get max spell level we can cast, since only usable boni are displayed
		# check the relevant mxspl* table
		SpellTable = CommonTables.ClassSkills.GetValue (ClassName, "CLERICSPELL")
		if SpellTable == "*":
			SpellTable = CommonTables.ClassSkills.GetValue (ClassName, "MAGESPELL")
		SpellTable = GemRB.LoadTable (SpellTable)
		maxLevel = 0
		for i in range(SpellTable.GetColumnCount()):
			spells = SpellTable.GetValue (str(level), str(i+1)) # not all tables start at 1, so use a named lookup
			if not spells:
				break
			maxLevel = i+1

		classes.append(ClassTitle)
		# check if at casting stat size, there is any bonus spell in BonusSpellTable
		bonusSpells[ClassTitle] = [0] * maxLevel
		for level in range (1, maxLevel+1):
			bonusSpells[ClassTitle][level-1] = BonusSpellTable.GetValue (Stat-12, level-1)

	return bonusSpells, classes

def HasClassFeatures (pc):
	#clerics turning
	if GemRB.GetPlayerStat(pc, IE_TURNUNDEADLEVEL):
		return True
	#thieves backstabbing damage
	if GemRB.GetPlayerStat(pc, IE_BACKSTABDAMAGEMULTIPLIER):
		return True
	#paladins healing
	if GemRB.GetPlayerStat(pc, IE_LAYONHANDSAMOUNT):
		return True
	# wisdom ac bonus, wholeness of body
	if GemRB.GetPlayerStat (pc, IE_LEVELMONK):
		return True
	return False

def DisplayFavouredEnemy (pc, RangerLevel, second=-1):
	RaceID = 0
	if second == -1:
		RaceID = GemRB.GetPlayerStat(pc, IE_HATEDRACE)
	else:
		RaceID = GemRB.GetPlayerStat(pc, IE_HATEDRACE2+second)
	if RaceID:
		FavouredIndex = HateRaceTable.FindValue (1, RaceID)
		if FavouredIndex == -1:
			return
		FavouredName = HateRaceTable.GetValue (FavouredIndex, 0)
		if second == -1:
			RecordsTextArea.Append (delimited_txt(FavouredName, ":", PlusMinusStat((RangerLevel+4)/5)))
		else:
			RecordsTextArea.Append (delimited_txt(FavouredName, ":", PlusMinusStat((RangerLevel+4)/5-second-1)))

def GetFavoredClass (pc, code):
	if GemRB.GetPlayerStat (pc, IE_SEX)==1:
		code = code&15
	else:
		code = (code>>8)&15

	return code-1

# returns the race or subrace
def GetRace (pc):
	Race = GemRB.GetPlayerStat (pc, IE_RACE)
	Subrace = GemRB.GetPlayerStat (pc, IE_SUBRACE)
	if Subrace:
		Race = Race<<16 | Subrace
	return CommonTables.Races.FindValue (3, Race)

# returns the effective character level modifier
def GetECL (pc):
	RaceIndex = GetRace (pc)
	RaceRowName = CommonTables.Races.GetRowName (RaceIndex)
	return CommonTables.Races.GetValue (RaceRowName, "ECL")

#class is ignored
def GetNextLevelExp (Level, Adjustment, string=0):
	if Adjustment>5:
		Adjustment = 5
	if (Level < CommonTables.NextLevel.GetColumnCount (4) - 5):
		if string:
			return str(CommonTables.NextLevel.GetValue (4, Level + Adjustment))
		return CommonTables.NextLevel.GetValue (4, Level + Adjustment )

	if string:
		return GemRB.GetString(24342) #godhood
	return 0

def DisplayCommon (pc):
	Window = RecordsWindow

	Value = GemRB.GetPlayerStat(pc,IE_RACE)
	Value2 = GemRB.GetPlayerStat(pc,IE_SUBRACE)
	if Value2:
		Value = Value<<16 | Value2
	tmp = CommonTables.Races.FindValue (3, Value)
	Race = CommonTables.Races.GetValue (tmp, 2)
	Label = Window.GetControl (0x1000000f)
	Label.SetText (Race)

	Button = Window.GetControl (36)
	if Exportable (pc):
		Button.SetState (IE_GUI_BUTTON_ENABLED)
	else:
		Button.SetState (IE_GUI_BUTTON_DISABLED)
	return

def DisplaySavingThrows (pc):
	RecordsTextArea.Append ("\n\n[color=ffff00]" + GemRB.GetString(17379) + "[/color]\n")

	tmp = GemRB.GetPlayerStat (pc, IE_SAVEFORTITUDE)
	RecordsTextArea.Append (delimited_txt(17380, ":", PlusMinusStat(tmp)))

	tmp = GemRB.GetPlayerStat (pc, IE_SAVEREFLEX)
	RecordsTextArea.Append (delimited_txt(17381, ":", PlusMinusStat(tmp)))

	tmp = GemRB.GetPlayerStat (pc, IE_SAVEWILL)
	RecordsTextArea.Append (delimited_txt(17382, ":", PlusMinusStat(tmp), 0))

# screenshots at http:// lparchive.org/Icewind-Dale-2/Update%2013/
def GNZS(pc, s1, st, force=False):
	value = GemRB.GetPlayerStat (pc, st)
	if value or force:
		RecordsTextArea.Append (s1)
		RecordsTextArea.Append (": " + str(value) + "\n")
	return

def DisplayGeneral (pc):
	Window = RecordsWindow

	#levels
	# get special level penalty for subrace
	adj = GetECL (pc)
	levelsum = GemRB.GetPlayerStat (pc, IE_CLASSLEVELSUM)
	RecordsTextArea.Append ("[color=ffff00]" + GemRB.GetString(40308) + " - " +
                            GemRB.GetString(40309) + ": " + str(levelsum) + "\n[/color]")

	#the class name for highest
	highest = None
	tmp = 0
	for i in range(11):
		level = GemRB.GetPlayerStat (pc, Classes[i])

		if level:
			Class = GUICommonWindows.GetActorClassTitle (pc, i )
			RecordsTextArea.Append (Class)
			RecordsTextArea.Append (": " + str(level) + "\n")
			if tmp<level:
				highest = i
				tmp = level

	RecordsTextArea.Append ("\n")
	#effective character level
	if adj:
		RecordsTextArea.Append (40311)
		RecordsTextArea.Append (": " + str(levelsum+adj) + "\n")

	#favoured class
	RecordsTextArea.Append (40310)

	#get the subrace value
	RaceIndex = GetRace (pc)
	Race = CommonTables.Races.GetValue (RaceIndex, 2)
	tmp = CommonTables.Races.GetValue (RaceIndex, 8)

	if tmp == -1:
		tmp = highest
	else:
		tmp = GetFavoredClass(pc, tmp)

	tmp = CommonTables.Classes.GetValue (CommonTables.Classes.GetRowName(tmp), "NAME_REF")
	RecordsTextArea.Append (": ")
	RecordsTextArea.Append (tmp)

	#experience
	RecordsTextArea.Append ("\n\n[color=ffff00]" + GemRB.GetString(17089) + "[/color]\n")

	RecordsTextArea.Append (36928)
	xp = GemRB.GetPlayerStat (pc, IE_XP)
	RecordsTextArea.Append (str(xp) + "\n")
	RecordsTextArea.Append (17091)
	tmp = GetNextLevelExp (levelsum, adj, 1)
	RecordsTextArea.Append (": "+tmp )

	#current effects
	effects = GemRB.GetPlayerStates (pc)
	if len(effects):
		RecordsTextArea.Append ("\n\n[color=ffff00]" + GemRB.GetString(32052) + "[/color]\n")
		StateTable = GemRB.LoadTable ("statdesc")
		for c in effects:
			tmp = StateTable.GetValue (str(ord(c)-66), "DESCRIPTION")
			RecordsTextArea.Append ("[cap]"+c+"%[/cap][p]" + GemRB.GetString (tmp) + "[/p]")

	# TODO: Active Feats (eg. Power attack 4)

	#race
	RecordsTextArea.Append ("\n\n[color=ffff00]" + GemRB.GetString(1048) + "[/color]\n")
	RecordsTextArea.Append (Race)

	#alignment
	RecordsTextArea.Append ("\n\n[color=ffff00]" + GemRB.GetString(1049) + "[/color]\n")
	tmp = CommonTables.Aligns.FindValue (3, GemRB.GetPlayerStat (pc, IE_ALIGNMENT))
	Align = CommonTables.Aligns.GetValue (tmp, 2)
	RecordsTextArea.Append (Align)

	#saving throws
	DisplaySavingThrows (pc)

	#class features
	if HasClassFeatures(pc):
		RecordsTextArea.Append ("\n\n[color=ffff00]" + GemRB.GetString(40314) + "[/color]\n")
		tmp = GemRB.GetPlayerStat (pc, IE_TURNUNDEADLEVEL)
		if tmp:
			RecordsTextArea.Append (12126)
			RecordsTextArea.Append (": "+str(tmp) )
		# 1d6 at level 1 and +1d6 every two extra rogue levels
		tmp = GemRB.GetPlayerStat (pc, IE_LEVELTHIEF)
		if tmp:
			tmp = (tmp+1)//2
			RecordsTextArea.Append (24898)
			RecordsTextArea.Append (": "+str(tmp)+"d6" )
		tmp = GemRB.GetPlayerStat (pc, IE_LAYONHANDSAMOUNT)
		if tmp:
			RecordsTextArea.Append (12127)
			RecordsTextArea.Append (": "+str(tmp) )
		MonkLevel = GemRB.GetPlayerStat (pc, IE_LEVELMONK)
		if MonkLevel:
			AC = GemRB.GetCombatDetails(pc, 0)["AC"]
			GemRB.SetToken ("number", PlusMinusStat (AC["Wisdom"]))
			RecordsTextArea.Append (39431)
			# wholeness of body
			RecordsTextArea.Append (39749)
			RecordsTextArea.Append (": "+str(MonkLevel*2))

	# favoured enemies; eg Goblins: +2 & Harpies: +1
	RangerLevel = GemRB.GetPlayerStat (pc, IE_LEVELRANGER)
	if RangerLevel:
		RangerString = "\n\n[color=ffff00]"
		if RangerLevel > 5:
			RangerString += GemRB.GetString (15982)
		else:
			RangerString += GemRB.GetString (15897)
		RecordsTextArea.Append (RangerString + "[/color]\n")
		DisplayFavouredEnemy (pc, RangerLevel)
		for i in range (7):
			DisplayFavouredEnemy (pc, RangerLevel, i)

	#bonus spells
	bonusSpells, classes = GetBonusSpells(pc)
	if len(bonusSpells):
		RecordsTextArea.Append ("\n\n[color=ffff00]" + GemRB.GetString(10344) + "[/color]\n")
		for c in classes:
			if not len(bonusSpells[c]):
				continue
			# class/kit name
			RecordsTextArea.Append ("\n")
			RecordsTextArea.Append (c)
			for level in range(len(bonusSpells[c])):
				AddIndent()
				# Level X: +Y
				RecordsTextArea.Append (delimited_txt(7192, " " + str(level+1)+":", "+" + str(bonusSpells[c][level]), 0))

	#ability statistics
	RecordsTextArea.Append ("\n\n[color=ffff00]" + GemRB.GetString(40315) + "[/color]\n")

	# Weight Allowance
	tmp = GemRB.GetAbilityBonus( IE_STR, 3, GemRB.GetPlayerStat(pc, IE_STR) )
	RecordsTextArea.Append (delimited_txt (10338, ":", str(tmp) + " lb."))
	# constitution bonus to hitpoints
	tmp = GUICommon.GetAbilityBonus(pc, IE_CON)
	RecordsTextArea.Append (delimited_txt (10342, ":", PlusMinusStat(tmp)))

	# Magic
	GNZS(pc, 15581, IE_RESISTMAGIC, True)
	#Fire
	GNZS(pc, 14012, IE_RESISTFIRE)
	#Magic Fire
	GNZS(pc, 14077, IE_RESISTMAGICFIRE)
	#Cold
	GNZS(pc, 14014, IE_RESISTCOLD)
	#Magic Cold
	GNZS(pc, 14078, IE_RESISTMAGICCOLD)
	#Electricity
	GNZS(pc, 14013, IE_RESISTELECTRICITY)
	#Acid
	GNZS(pc, 14015, IE_RESISTACID)
	#Spell
	GNZS(pc, 40319, IE_MAGICDAMAGERESISTANCE)
	# Missile
	GNZS(pc, 11767, IE_RESISTMISSILE)
	# Slashing
	GNZS(pc, 11768, IE_RESISTSLASHING)
	# Piercing
	GNZS(pc, 11769, IE_RESISTPIERCING)
	# Crushing
	GNZS(pc, 11770, IE_RESISTCRUSHING)
	# Poison
	GNZS(pc, 14017, IE_RESISTPOISON)

	# Damage Reduction - Spell Effect: Damage Reduction [436] (plain boni are mentioned above as resistances)
	BaseString = 0
	DisplayedHeader = 0
	for damageType in 0, 1:
		if damageType:
			# missile
			BaseString = 11767
		else:
			# general (physical)
			BaseString = 40316

		# cheat a bit and use the reduction at +10 as a modifier to remove from all other values
		# it will hold all the non-enchantment related resistance (there are no +10 weapons)
		mod = GemRB.GetDamageReduction (pc, damageType, 10)
		if mod == -1:
			mod = 0
		for enchantment in range(6):
			damage = GemRB.GetDamageReduction (pc, damageType, enchantment)
			if damage == -1 or damage-mod <= 0:
				continue
			if not DisplayedHeader:
				RecordsTextArea.Append ("\n\n[color=ffff00]" + GemRB.GetString(39325) + "[/color]\n")
				DisplayedHeader = 1

			enchantment += 1 # since we were checking what is allowable, not what bypasses it
			if enchantment:
				enchantment = "+" + str(enchantment)
			else:
				enchantment = "-"
			reduction = "%d/%s" %(damage-mod, enchantment)
			RecordsTextArea.Append (delimited_txt(BaseString, ":", reduction, 1))

	return

# some of the displayed stats are manually indented
def AddIndent():
	RecordsTextArea.Append ("   ")

def PlusMinusStat(value):
	if value >= 0:
		return "+" + str(value)
	return str(value)

def CascadeToHit(total, ac, apr, slot):
	cascade = PlusMinusStat(total)
	babDec = 5
	if ac["Wisdom"]:
		if slot == 10: # fist slot - nothing is equipped
			babDec = 3
	for i in range(1, apr):
		if total-i*babDec > 0: # skips negative ones, meaning a lower number of attacks can be displayed
			cascade = cascade  + "/" + PlusMinusStat(total-i*babDec)
	return cascade

def ToHitOfHand(combatdet, dualwielding, left=0):
	ac = combatdet["AC"]
	tohit = combatdet["ToHitStats"]

	# display all the (successive) attack values (+15/+10/+5)
	if left:
		apr = 1 # offhand gives just one extra attack
		hits = CascadeToHit(tohit["Total"], ac, apr, combatdet["Slot"])
		RecordsTextArea.Append (delimited_txt (733, ":", hits, 0))
	else:
		# account for the fact that the total apr contains the one for the other hand too
		if dualwielding:
			apr = combatdet["APR"]//2 - 1
		else:
			apr = combatdet["APR"]//2
		hits = CascadeToHit(tohit["Total"], ac, apr, combatdet["Slot"])
		RecordsTextArea.Append (delimited_txt (734, ":", hits, 0))

	# Base
	AddIndent()
	hits = CascadeToHit(tohit["Base"], ac, apr, combatdet["Slot"])
	RecordsTextArea.Append (delimited_txt (31353, ":", hits, 0))
	# Weapon bonus
	if tohit["Weapon"]:
		AddIndent()
		RecordsTextArea.Append (delimited_txt (32560 , ":", PlusMinusStat(tohit["Weapon"]), 0))
	# Proficiency bonus
	if tohit["Proficiency"]:
		AddIndent()
		RecordsTextArea.Append (delimited_txt (32561, ":", PlusMinusStat(tohit["Proficiency"]), 0))
	# Armor Penalty
	if tohit["Armor"]:
		AddIndent()
		RecordsTextArea.Append (delimited_txt (39816 , ":", PlusMinusStat(tohit["Armor"]), 0))
	# Shield Penalty (if you don't have the shield proficiency feat)
	if tohit["Shield"]:
		AddIndent()
		RecordsTextArea.Append (delimited_txt (39822 , ":", PlusMinusStat(tohit["Shield"]), 0))
	# Ability  bonus
	if tohit["Ability"]:
		AddIndent()
		RecordsTextArea.Append (delimited_txt (33547, ":", PlusMinusStat(tohit["Ability"]), 0))
	# Others
	if tohit["Generic"]:
		AddIndent()
		RecordsTextArea.Append (delimited_txt (33548, ":", PlusMinusStat(tohit["Generic"]), 0))
	RecordsTextArea.Append ("\n\n")

def WeaponOfHand(pc, combatdet, dualwielding, left=0):
	slot = combatdet["Slot"]
	slot_item = GemRB.GetSlotItem (pc, slot, 1)
	if not slot_item:
		print "ARGHH, no slot item at slot %d, bailing out!" %(combatdet["Slot"])
		return
	item = GemRB.GetItem (slot_item["ItemResRef"])
	ammo = None
	if not left: # only the main hand can have ranged weapons, even one-handed
		ammoslot = GemRB.GetEquippedAmmunition (pc)
		if ammoslot != -1:
			ammosi = GemRB.GetSlotItem (pc, ammoslot)
			ammo = GemRB.GetItem (ammosi["ItemResRef"])

	# Main Hand - weapon name
	#  or Ranged - ammo
	if combatdet["Flags"]&15 == 2 and ammo: # this is basically wi.wflags & WEAPON_STYLEMASK == WEAPON_RANGED
		RecordsTextArea.Append (delimited_str (41123, " -", ammo["ItemNameIdentified"], 0))
	else:
		if dualwielding and left:
			RecordsTextArea.Append (delimited_str (733, " -", item["ItemNameIdentified"], 0))
		else:
			RecordsTextArea.Append (delimited_str (734, " -", item["ItemNameIdentified"], 0))

	# Damage
	# display the unresolved damage string (2d6)
	# this is ammo, launcher details come later
	wdice = combatdet["HitHeaderNumDice"]
	wsides = combatdet["HitHeaderDiceSides"]
	wbonus = combatdet["HitHeaderDiceBonus"]
	AddIndent()
	if wbonus:
		RecordsTextArea.Append (delimited_txt (39518, ":", str (wdice)+"d"+str(wsides)+PlusMinusStat(wbonus), 0))
	else:
		RecordsTextArea.Append (delimited_txt (39518, ":", str (wdice)+"d"+str(wsides), 0))
	# any extended headers with damage, eg. Fire: +1d6, which is also computed for the total (00arow08)
	alldos = combatdet["DamageOpcodes"]
	dosmin = 0
	dosmax = 0
	for dos in alldos:
		ddice = dos["NumDice"]
		dsides = dos["DiceSides"]
		dbonus = dos["DiceBonus"]
		dchance = dos["Chance"]
		AddIndent()
		# only display the chance when it isn't 100%
		if dchance == 100:
			dchance = ""
			dosmin += ddice + dbonus
		else:
			dchance = " (%d%%)" % dchance
		dicestr = ""
		if ddice:
			dicestr = "+%dd%d" %(ddice, dsides)
		if dbonus:
			RecordsTextArea.Append (dos["TypeName"] + ": " + dicestr + PlusMinusStat(dbonus)+dchance)
		else:
			RecordsTextArea.Append (dos["TypeName"] + ": " + dicestr + dchance)
		dosmax += ddice*dsides + dbonus

	# Strength
	abonus = combatdet["WeaponStrBonus"]
	if abonus:
		AddIndent()
		RecordsTextArea.Append (delimited_txt (1145, ":", PlusMinusStat (abonus), 0))
	# Proficiency (bonus)
	pbonus = combatdet["ProfDmgBon"]
	if pbonus:
		AddIndent()
		RecordsTextArea.Append (delimited_txt (32561, ":", PlusMinusStat(pbonus), 0))
	# Launcher
	lbonus = combatdet["LauncherDmgBon"]
	if lbonus:
		AddIndent()
		RecordsTextArea.Append (delimited_txt (41408, ":", PlusMinusStat(lbonus), 0))
	#TODO: Power Attack has its own row
	# Damage Potential (2-12)
	# add any other bonus to the ammo damage calc
	AddIndent()
	wmin = wdice + wbonus + abonus + pbonus + lbonus + dosmin
	wmax = wdice*wsides + wbonus + abonus + pbonus + lbonus + dosmax
	RecordsTextArea.Append (delimited_txt (41120, ":", str (wmin)+"-"+str(wmax), 0))
	# Critical Hit (19-20 / x2)
	crange = combatdet["CriticalRange"]
	cmulti = combatdet["CriticalMultiplier"]
	if crange == 20:
		crange = "20 / x" + str(cmulti)
	else:
		crange = str(crange) + "-20 / x" + str(cmulti)
	AddIndent()
	RecordsTextArea.Append (delimited_txt (41122, ":", crange, 0))
	if not left and dualwielding:
		RecordsTextArea.Append ("\n")

def DisplayWeapons (pc):
	Window = RecordsWindow

	GS = lambda s, pc=pc: GemRB.GetPlayerStat (pc, s)
	GB = lambda s, pc=pc: GemRB.GetPlayerStat (pc, s, 1)
	# maybe add an iwd2 mode to GemRB.GetAbilityBonus
	GA = lambda s, pc=pc: int((GS(s)-10)/2)

	###################
	# Attack Roll Modifiers
	RecordsTextArea.Append ("[color=ffff00]" + GemRB.GetString(9457) + "[/color]\n")

	combatdet = GemRB.GetCombatDetails(pc, 0)
	combatdet2 = combatdet # placeholder for the potential offhand numbers
	ac = combatdet["AC"]
	tohit = combatdet["ToHitStats"]
	dualwielding = GemRB.IsDualWielding(pc)
	if dualwielding:
		combatdet2 = GemRB.GetCombatDetails(pc, 1)

	# Main Hand
	ToHitOfHand (combatdet, dualwielding)

	# Off Hand
	if dualwielding:
		ToHitOfHand (combatdet2, dualwielding, 1)


	###################
	# Number of Attacks
	if dualwielding:
		# only one extra offhand attack and it is displayed as eg. 2+1
		RecordsTextArea.Append (delimited_txt (9458, ":", str (combatdet["APR"]//2-1)+"+1"))
	else:
		RecordsTextArea.Append (delimited_txt (9458, ":", str (combatdet["APR"]//2)))
	RecordsTextArea.Append ("\n")

	###################
	# Armor Class
	RecordsTextArea.Append ("[color=ffff00]" + GemRB.GetString(33553) + "[/color]\n")
	RecordsTextArea.Append (delimited_txt (33553, ":", str (GS(IE_ARMORCLASS)), 0)) # same as ac["Total"]

	# Base
	AddIndent()
	RecordsTextArea.Append (delimited_txt (31353, ":", str (ac["Natural"]), 0))
	# Armor
	if ac["Armor"]:
		AddIndent()
		RecordsTextArea.Append (delimited_txt (11997, ":", PlusMinusStat (ac["Armor"]), 0))
	# Shield
	if ac["Shield"]:
		AddIndent()
		RecordsTextArea.Append (delimited_txt (6347, ":", PlusMinusStat (ac["Shield"]), 0))
	# Deflection
	if ac["Deflection"]:
		AddIndent()
		RecordsTextArea.Append (delimited_txt (33551, ":", PlusMinusStat (ac["Deflection"]), 0))
	# Generic
	if ac["Generic"]:
		AddIndent()
		RecordsTextArea.Append (delimited_txt (33552, ":", PlusMinusStat (ac["Generic"]), 0))
	# Dexterity
	if ac["Dexterity"]:
		AddIndent()
		RecordsTextArea.Append (delimited_txt (1151, ":", PlusMinusStat (ac["Dexterity"]), 0))
	# Monk Wisdom Bonus: <number> to AC
	if ac["Wisdom"]:
		GemRB.SetToken ("number", PlusMinusStat (ac["Wisdom"]))
		RecordsTextArea.Append (394311)

	RecordsTextArea.Append ("\n\n")

	###################
	# Armor Class Modifiers
	stat = GS (IE_ACMISSILEMOD) + GS (IE_ACSLASHINGMOD) + GS (IE_ACPIERCINGMOD) + GS (IE_ACCRUSHINGMOD)
	if stat:
		RecordsTextArea.Append ("[color=ffff00]" + GemRB.GetString(11766) + "[/color]")

		# Missile
		if GS (IE_ACMISSILEMOD):
			AddIndent()
			RecordsTextArea.Append (delimited_txt (11767, ":", PlusMinusStat(GS (IE_ACMISSILEMOD)), 0))
		# Slashing
		if GS (IE_ACSLASHINGMOD):
			AddIndent()
			RecordsTextArea.Append (delimited_txt (11768, ":", PlusMinusStat (GS (IE_ACSLASHINGMOD)), 0))
		# Piercing
		if GS (IE_ACPIERCINGMOD):
			AddIndent()
			RecordsTextArea.Append (delimited_txt (11769, ":", PlusMinusStat (GS (IE_ACPIERCINGMOD)), 0))
		# Bludgeoning
		if GS (IE_ACCRUSHINGMOD):
			AddIndent()
			RecordsTextArea.Append (delimited_txt (11770, ":", PlusMinusStat (GS (IE_ACCRUSHINGMOD))))

		RecordsTextArea.Append ("\n\n")

	###################
	# Arcane spell failure
	if GS(IE_LEVELBARD) + GS(IE_LEVELSORCERER) + GS(IE_LEVELMAGE):
		RecordsTextArea.Append ("[color=ffff00]" + GemRB.GetString(41391) + "[/color]\n")

		# Casting Failure
		failure = GemRB.GetSpellFailure (pc)
		total = failure["Total"]
		arcana = 5*GemRB.HasFeat(pc, FEAT_ARMORED_ARCANA)
		other = 5*(failure["Armor"] + failure["Shield"]) - arcana
		# account for armored arcana only if it doesn't cause a negative value (eating into the misc bonus)
		verbose = 0
		if other < 0:
			other = total
		else:
			other = total - other
			verbose = 1
		if total < 0:
			total = 0
		RecordsTextArea.Append (delimited_txt (41390 , ":", str (total)+"%", 0))
		# Armor Penalty (same as for skills and everything else)
		if verbose and failure["Armor"]:
			AddIndent()
			RecordsTextArea.Append (delimited_txt (39816 , ":", PlusMinusStat(5*failure["Armor"])+"%", 0))
		# Shield Penalty
		if verbose and failure["Shield"]:
			AddIndent()
			RecordsTextArea.Append (delimited_txt (39822, ":", PlusMinusStat(5*failure["Shield"])+"%", 0))
		if arcana:
			AddIndent()
			RecordsTextArea.Append (delimited_txt (36352, ":", PlusMinusStat(arcana)+"%", 0))
		# Other, just a guess to show the remainder
		if other:
			AddIndent()
			RecordsTextArea.Append (delimited_txt (33548, ":", PlusMinusStat(other)+"%", 0))

		RecordsTextArea.Append ("\n\n")

	###################
	# Weapon Statistics
	RecordsTextArea.Append ("[color=ffff00]" + GemRB.GetString(41119) + "[/color]\n")

	# Main hand
	WeaponOfHand(pc, combatdet, dualwielding)

	# Off-hand (if any)
	if dualwielding:
		RecordsTextArea.Append ("\n")
		WeaponOfHand(pc, combatdet2, dualwielding, 1)

	DisplaySavingThrows (pc)

	return

def DisplaySkills (pc):
	Window = RecordsWindow

	SkillTable = GemRB.LoadTable ("skillsta")
	SkillName = GemRB.LoadTable ("skills")
	rows = SkillTable.GetRowCount ()

	#skills
	RecordsTextArea.Append ("[color=ffff00]" + GemRB.GetString(11983) + "[/color]\n")

	skills = []
	for i in range(rows):
		stat = SkillTable.GetValue (i, 0, 2)
		value = GemRB.GetPlayerStat (pc, stat)
		base = GemRB.GetPlayerStat (pc, stat, 1)
		untrained = SkillName.GetValue (i, 3)

		# only show (modified) skills that either don't require training or had it already
		if (value and untrained) or (not untrained and base):
			skill = SkillName.GetValue (i, 1)
			skills.append (GemRB.GetString(skill) + ": " + str(value) + " (" + str(base) + ")\n")

	skills.sort()
	for i in skills:
		RecordsTextArea.Append (i)

	FeatTable = GemRB.LoadTable ("featreq")
	FeatName = GemRB.LoadTable ("feats")
	rows = FeatTable.GetRowCount ()
	#feats
	RecordsTextArea.Append ("\n\n[color=ffff00]" + GemRB.GetString(36361) + "[/color]\n")

	feats = []
	for i in range(rows):
		if GemRB.HasFeat(pc, i):
			feat = FeatName.GetValue (i, 1)
			stat = FeatTable.GetValue (i, 0, 2)
			if stat:
				multi = GemRB.GetPlayerStat (pc, stat)
				feats.append (GemRB.GetString(feat) + ": " + str(multi) + "\n")
			else:
				feats.append (GemRB.GetString(feat) + "\n")

	feats.sort()
	for i in feats:
		RecordsTextArea.Append (i)

	return

def delimited_str(strref, delimiter, strref2, newline=1):
	if strref2:
		val = GemRB.GetString(strref) + delimiter + " " + GemRB.GetString(strref2)
	else:
		val = GemRB.GetString(strref) + delimiter
	if newline:
		return val + "\n"
	else:
		return val

def delimited_txt(strref, delimiter, text, newline=1):
	val = GemRB.GetString(strref) + delimiter + " " + str(text)
	if newline:
		return val + "\n"
	else:
		return val

#character information
def DisplayMisc (pc):
	Window = RecordsWindow

	TotalPartyExp = 0
	TotalCount = 0
	for i in range (1, GemRB.GetPartySize() + 1):
		stat = GemRB.GetPCStats(i)
		TotalPartyExp = TotalPartyExp + stat['KillsTotalXP']
		TotalCount = TotalCount + stat['KillsTotalCount']

	stat = GemRB.GetPCStats (pc)

	#favourites
	RecordsTextArea.Append ("[color=ffff00]" + GemRB.GetString(40320) + "[/color]\n")

	#favourite spell and weapon
	RecordsTextArea.Append (delimited_str (11949, ":", stat['FavouriteSpell']))
	RecordsTextArea.Append (delimited_str (11950, ":", stat['FavouriteWeapon']))

	# combat details
	RecordsTextArea.Append ("\n[color=ffff00]" + GemRB.GetString(40322) + "[/color]\n")

	#most powerful vanquished, time spent, xp and kills
	RecordsTextArea.Append (delimited_str (11947, ":", stat['BestKilledName']))

	days, hours = GUICommon.SetCurrentDateTokens (stat)
	# iwd2 is special here
	# construct <GAMEDAYS> days ~and~ ~<HOUR> hours~
	if days == 1:
		time = GemRB.GetString (10698)
	else:
		time = GemRB.GetString (10697)
	time += " " + GemRB.GetString (10699) + " "
	if hours == 1:
		time += GemRB.GetString (10701)
	else:
		time += GemRB.GetString (10700)

	RecordsTextArea.Append (delimited_txt (11948, ":", time))

	# Experience Value of Kills
	RecordsTextArea.Append (delimited_txt (11953, ":", stat['KillsTotalXP']))

	# Number of Kills
	RecordsTextArea.Append (delimited_txt (11954, ":", stat['KillsTotalCount']))

	# Total Experience Value in Party
	if TotalPartyExp:
		val = stat['KillsTotalXP']*100/TotalPartyExp
	else:
		val = 0
	RecordsTextArea.Append (delimited_txt (11951, ":", str(val) + "%"))

	# Percentage of Total Kills in Party
	if TotalPartyExp:
		val = stat['KillsTotalCount']*100/TotalCount
	else:
		val = 0
	RecordsTextArea.Append (delimited_txt (11954, ":", str(val) + "%"))

	return

def UpdateRecordsWindow ():
	global RecordsTextArea

	Window = RecordsWindow

	pc = GemRB.GameGetSelectedPCSingle ()

	#name
	Label = Window.GetControl (0x1000000e)
	Label.SetText (GemRB.GetPlayerName (pc, 0))

	#portrait
	Button = Window.GetControl (2)
	Button.SetPicture (GemRB.GetPlayerPortrait (pc,0))

	# armorclass
	Label = Window.GetControl (0x10000028)
	Label.SetText (str (GemRB.GetPlayerStat (pc, IE_ARMORCLASS)))
	Label.SetTooltip (17183)

	# hp now
	Label = Window.GetControl (0x10000029)
	Label.SetText (str (GemRB.GetPlayerStat (pc, IE_HITPOINTS)))
	Label.SetTooltip (17184)

	# hp max
	Label = Window.GetControl (0x1000002a)
	Label.SetText (str (GemRB.GetPlayerStat (pc, IE_MAXHITPOINTS)))
	Label.SetTooltip (17378)

	#level up
	Button = Window.GetControl (37)
	levelsum = GemRB.GetPlayerStat (pc, IE_CLASSLEVELSUM)
	if GetNextLevelExp(levelsum, GetECL(pc)) <= GemRB.GetPlayerStat (pc, IE_XP):
		Button.SetState (IE_GUI_BUTTON_ENABLED)
	else:
		Button.SetState (IE_GUI_BUTTON_DISABLED)

	# stats

	sstr = GemRB.GetPlayerStat (pc, IE_STR)
	dstr = sstr-GemRB.GetPlayerStat (pc, IE_STR,1)
	bstr = GUICommon.GetAbilityBonus (pc, IE_STR)
	sint = GemRB.GetPlayerStat (pc, IE_INT)
	dint = sint-GemRB.GetPlayerStat (pc, IE_INT,1)
	bint = GUICommon.GetAbilityBonus (pc, IE_INT)
	swis = GemRB.GetPlayerStat (pc, IE_WIS)
	dwis = swis-GemRB.GetPlayerStat (pc, IE_WIS,1)
	bwis = GUICommon.GetAbilityBonus (pc, IE_WIS)
	sdex = GemRB.GetPlayerStat (pc, IE_DEX)
	ddex = sdex-GemRB.GetPlayerStat (pc, IE_DEX,1)
	bdex = GUICommon.GetAbilityBonus (pc, IE_DEX)
	scon = GemRB.GetPlayerStat (pc, IE_CON)
	dcon = scon-GemRB.GetPlayerStat (pc, IE_CON,1)
	bcon = GUICommon.GetAbilityBonus (pc, IE_CON)
	schr = GemRB.GetPlayerStat (pc, IE_CHR)
	dchr = schr-GemRB.GetPlayerStat (pc, IE_CHR,1)
	bchr = GUICommon.GetAbilityBonus (pc, IE_CHR)

	Label = Window.GetControl (0x1000002f)
	Label.SetText (str(sstr))
	ColorDiff2 (Window, Label, dstr)

	Label = Window.GetControl (0x10000009)
	Label.SetText (str(sdex))
	ColorDiff2 (Window, Label, ddex)

	Label = Window.GetControl (0x1000000a)
	Label.SetText (str(scon))
	ColorDiff2 (Window, Label, dcon)

	Label = Window.GetControl (0x1000000b)
	Label.SetText (str(sint))
	ColorDiff2 (Window, Label, dint)

	Label = Window.GetControl (0x1000000c)
	Label.SetText (str(swis))
	ColorDiff2 (Window, Label, dwis)

	Label = Window.GetControl (0x1000000d)
	Label.SetText (str(schr))
	ColorDiff2 (Window, Label, dchr)

	Label = Window.GetControl (0x10000035)
	Label.SetText (PlusMinusStat(bstr))
	ColorDiff (Window, Label, bstr)

	Label = Window.GetControl (0x10000036)
	Label.SetText (PlusMinusStat(bdex))
	ColorDiff (Window, Label, bdex)

	Label = Window.GetControl (0x10000037)
	Label.SetText (PlusMinusStat(bcon))
	ColorDiff (Window, Label, bcon)

	Label = Window.GetControl (0x10000038)
	Label.SetText (PlusMinusStat(bint))
	ColorDiff (Window, Label, bint)

	Label = Window.GetControl (0x10000039)
	Label.SetText (PlusMinusStat(bwis))
	ColorDiff (Window, Label, bwis)

	Label = Window.GetControl (0x1000003a)
	Label.SetText (PlusMinusStat(bchr))
	ColorDiff (Window, Label, bchr)

	RecordsTextArea = Window.GetControl (45)
	RecordsTextArea.SetText ("")

	DisplayCommon (pc)

	SelectWindow = GemRB.GetVar ("SelectWindow")
	if SelectWindow == 1:
		DisplayGeneral (pc)
	elif SelectWindow == 2:
		DisplayWeapons (pc)
	elif SelectWindow == 3:
		DisplaySkills (pc)
	elif SelectWindow == 4:
		DisplayMisc (pc)

	#if actor is uncontrollable, make this grayed
	GUICommon.AdjustWindowVisibility (Window, pc, 0)
	PortraitWindow.SetVisible (WINDOW_VISIBLE)
	OptionsWindow.SetVisible (WINDOW_VISIBLE)
	return

def CloseHelpWindow ():
	global DescTable, InformationWindow

	if InformationWindow:
		InformationWindow.Unload ()
		InformationWindow = None
	if DescTable:
		DescTable = None
	return

#ingame help
def OpenHelpWindow ():
	global HelpTable, InformationWindow

	InformationWindow = Window = GemRB.LoadWindow (57)

	HelpTable = GemRB.LoadTable ("topics")
	GemRB.SetVar("Topic", 0)
	GemRB.SetVar("TopIndex", 0)

	for i in range(11):
		title = HelpTable.GetValue (i, 0)
		Button = Window.GetControl (i+27)
		Label = Window.GetControl (i+0x10000004)

		Button.SetVarAssoc ("Topic", i)
		if title:
			Label.SetText (title)
			Button.SetState (IE_GUI_BUTTON_LOCKED)
			Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, UpdateHelpWindow)
		else:
			Label.SetText ("")
			Button.SetState (IE_GUI_BUTTON_DISABLED)

	#done
	Button = Window.GetControl (1)
	Button.SetText (11973)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, CloseHelpWindow)
	Window.ShowModal (MODAL_SHADOW_GRAY)
	UpdateHelpWindow ()
	return

def UpdateHelpWindow ():
	global DescTable, Topic

	Window = InformationWindow

	if Topic!=GemRB.GetVar ("Topic"):
		Topic = GemRB.GetVar ("Topic")
		GemRB.SetVar ("TopIndex",0)
		GemRB.SetVar ("Selected",0)

	for i in range(11):
		Button = Window.GetControl (i+27)
		Label = Window.GetControl (i+0x10000004)
		if Topic==i:
			Label.SetTextColor (255,255,0)
		else:
			Label.SetTextColor (255,255,255)

	resource = HelpTable.GetValue (Topic, 1)
	if DescTable:
		DescTable = None

	DescTable = GemRB.LoadTable (resource)

	ScrollBar = Window.GetControl (4)

	startrow = HelpTable.GetValue (Topic, 4)
	if startrow<0:
		i=-startrow-10
	else:
		i = DescTable.GetRowCount ()-10-startrow

	if i<1: i=1

	ScrollBar.SetVarAssoc ("TopIndex", i)
	ScrollBar.SetEvent (IE_GUI_SCROLLBAR_ON_CHANGE, RefreshHelpWindow)

	RefreshHelpWindow ()
	return

def RefreshHelpWindow ():
	Window = InformationWindow
	Topic = GemRB.GetVar ("Topic")
	TopIndex = GemRB.GetVar ("TopIndex")
	Selected = GemRB.GetVar ("Selected")

	titlecol = HelpTable.GetValue (Topic, 2)
	desccol = HelpTable.GetValue (Topic, 3)
	startrow = HelpTable.GetValue (Topic, 4)
	if startrow<0: startrow = 0

	for i in range(11):
		title = DescTable.GetValue (i+startrow+TopIndex, titlecol)

		Button = Window.GetControl (i+71)
		Label = Window.GetControl (i+0x10000030)

		if i+TopIndex==Selected:
			Label.SetTextColor (255,255,0)
		else:
			Label.SetTextColor (255,255,255)
		if title>0:
			Label.SetText (title)
			Button.SetState (IE_GUI_BUTTON_LOCKED)
			Button.SetVarAssoc ("Selected", i+TopIndex)
			Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, RefreshHelpWindow)
		else:
			Label.SetText ("")
			Button.SetState (IE_GUI_BUTTON_DISABLED)

	if Selected<0:
		desc=""
	else:
		desc = DescTable.GetValue (Selected+startrow, desccol)

	TextArea = Window.GetControl (2)
	TextArea.SetText (desc)
	return

###################################################
# End of file GUIREC.py
