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
import IDLUCommon
import LUCommon
from GUIDefines import *
from ie_stats import *
from ie_restype import *
from ie_feats import FEAT_ARMORED_ARCANA

SelectWindow = 0
Topic = None
HelpTable = None
DescTable = None
RecordsWindow = None
RecordsTextArea = None
InformationWindow = None
PortraitWindow = None
OptionsWindow = None
OldPortraitWindow = None
OldOptionsWindow = None
BonusSpellTable = None
HateRaceTable = None
PauseState = None

if not BonusSpellTable:
	BonusSpellTable = GemRB.LoadTable ("mxsplbon")
if not HateRaceTable:
	HateRaceTable = GemRB.LoadTable ("haterace")

# class level stats
Classes = IDLUCommon.Levels

#don't allow exporting polymorphed or dead characters
def Exportable(pc):
	if not (GemRB.GetPlayerStat (pc, IE_MC_FLAGS)&MC_EXPORTABLE): return False
	if GemRB.GetPlayerStat (pc, IE_POLYMORPHED): return False
	if GemRB.GetPlayerStat (pc, IE_STATE_ID)&STATE_DEAD: return False
	return True

def OpenRecordsWindow ():
	global RecordsWindow, OptionsWindow, PortraitWindow
	global OldPortraitWindow, OldOptionsWindow, SelectWindow
	global BonusSpellTable, HateRaceTable, PauseState

	if GUICommon.CloseOtherWindow (OpenRecordsWindow):

		GUIRECCommon.CloseSubSubCustomizeWindow ()
		GUIRECCommon.CloseSubCustomizeWindow ()
		GUIRECCommon.CloseCustomizeWindow ()
		GUIRECCommon.ExportCancelPress()
		GUIRECCommon.CloseBiographyWindow ()
		CloseHelpWindow ()

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
		GemRB.GamePause (PauseState, 3)
		return

	PauseState = GemRB.GamePause (3, 1)
	GemRB.GamePause (1, 3)

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

	Window.SetKeyPressEvent (GUICommonWindows.SwitchPCByKey)

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
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, OpenLevelUpWindow)

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

def DisplayFavouredEnemy (pc, RangerLevel, second, RecordsTextArea):
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
			RecordsTextArea.Append (DelimitedText(FavouredName, PlusMinusStat((RangerLevel+4)/5), 0))
		else:
			RecordsTextArea.Append (DelimitedText(FavouredName, PlusMinusStat((RangerLevel+4)/5-second-1), 0))

def GetFavoredClass (pc, code):
	if GemRB.GetPlayerStat (pc, IE_SEX)==1:
		code = code&15
	else:	
		code = (code>>8)&15

	return code-1

# returns the effective character level modifier
def GetECL (pc):
	RaceIndex = IDLUCommon.GetRace (pc)
	RaceRowName = CommonTables.Races.GetRowName (RaceIndex)
	return CommonTables.Races.GetValue (RaceRowName, "ECL")

#class is ignored
def GetNextLevelExp (Level, Adjustment, string=0):
	if Adjustment>5:
		Adjustment = 5
	if (Level < CommonTables.NextLevel.GetColumnCount (4) - 5):
		exp = CommonTables.NextLevel.GetValue (4, Level + Adjustment)
		if string:
			return str(exp)
		return exp

	if string:
		return GemRB.GetString(24342) #godhood
	return 0

def DisplayCommon (pc):
	Window = RecordsWindow

	Label = Window.GetControl (0x1000000f)
	Label.SetText (GUICommonWindows.GetActorRaceTitle (pc))

	Button = Window.GetControl (36)
	if Exportable (pc):
		Button.SetState (IE_GUI_BUTTON_ENABLED)
	else:
		Button.SetState (IE_GUI_BUTTON_DISABLED)
	return

def DisplaySavingThrows (pc, RecordsTextArea):
	RecordsTextArea.Append ("\n[color=ffff00]" + GemRB.GetString(17379) + "[/color]\n")

	tmp = GemRB.GetPlayerStat (pc, IE_SAVEFORTITUDE)
	RecordsTextArea.Append (DelimitedText(17380, PlusMinusStat(tmp), 0))

	tmp = GemRB.GetPlayerStat (pc, IE_SAVEREFLEX)
	RecordsTextArea.Append (DelimitedText(17381, PlusMinusStat(tmp), 0))

	tmp = GemRB.GetPlayerStat (pc, IE_SAVEWILL)
	RecordsTextArea.Append (DelimitedText(17382, PlusMinusStat(tmp), 0))

# screenshots at http:// lparchive.org/Icewind-Dale-2/Update%2013/
def GNZS(pc, s1, st, force=False):
	value = GemRB.GetPlayerStat (pc, st)
	if value or force:
		RecordsTextArea.Append (DelimitedText(s1, value, 0))
	return

def DisplayGeneral (pc, targetTextArea):
	global RecordsTextArea
	RecordsTextArea = targetTextArea

	#levels
	# get special level penalty for subrace
	adj = GetECL (pc)
	levelsum = GemRB.GetPlayerStat (pc, IE_CLASSLEVELSUM)
	RecordsTextArea.Append ("[color=ffff00]" + GemRB.GetString(40308) + " - " +
                            GemRB.GetString(40309) + ": " + str(levelsum) + "[/color]\n")

	#the class name for highest
	highest = None
	tmp = 0
	for i in range(11):
		level = GemRB.GetPlayerStat (pc, Classes[i])

		if level:
			Class = GUICommonWindows.GetActorClassTitle (pc, i )
			RecordsTextArea.Append (DelimitedText (Class, level, 0))
			if tmp<level:
				highest = i
				tmp = level

	RecordsTextArea.Append ("\n")
	#effective character level
	if adj:
		RecordsTextArea.Append (DelimitedText (40311, levelsum+adj, 0))

	#favoured class
	#get the subrace value
	RaceIndex = IDLUCommon.GetRace (pc)
	Race = CommonTables.Races.GetValue (RaceIndex, 2)
	tmp = CommonTables.Races.GetValue (RaceIndex, 8)

	if tmp == -1:
		tmp = highest
	else:
		tmp = GetFavoredClass(pc, tmp)

	tmp = CommonTables.Classes.GetValue (CommonTables.Classes.GetRowName(tmp), "NAME_REF")
	RecordsTextArea.Append (DelimitedStrRefs (40310, tmp, 0))

	#experience
	RecordsTextArea.Append ("\n[color=ffff00]" + GemRB.GetString(17089) + "[/color]\n")

	xp = GemRB.GetPlayerStat (pc, IE_XP)
	RecordsTextArea.Append (DelimitedText (36928, xp, 0, ""))
	tmp = GetNextLevelExp (levelsum, adj, 1)
	RecordsTextArea.Append (DelimitedText (17091, tmp, 0))
	# Multiclassing penalty
	tmp = GemRB.GetMultiClassPenalty (pc)
	if tmp != 0:
		GemRB.SetToken ("XPPENALTY", str(tmp)+"%")
		RecordsTextArea.Append (DelimitedText (39418, "", 0, ""))

	#current effects
	effects = GemRB.GetPlayerStates (pc)
	if len(effects):
		RecordsTextArea.Append ("\n[color=ffff00]" + GemRB.GetString(32052) + "[/color]\n")
		StateTable = GemRB.LoadTable ("statdesc")
		for c in effects:
			tmp = StateTable.GetValue (str(ord(c)-66), "DESCRIPTION", GTV_REF)
			RecordsTextArea.Append ("[cap]"+c+"%[/cap][p]" + tmp + "[/p]")

	# TODO: Active Feats (eg. Power attack 4)

	#race
	RecordsTextArea.Append ("\n[color=ffff00]" + GemRB.GetString(1048) + "[/color]\n")
	RecordsTextArea.Append ("[p]" + GemRB.GetString(Race) + "[/p]")

	#alignment
	RecordsTextArea.Append ("\n[color=ffff00]" + GemRB.GetString(1049) + "[/color]\n")
	tmp = CommonTables.Aligns.FindValue (3, GemRB.GetPlayerStat (pc, IE_ALIGNMENT))
	Align = CommonTables.Aligns.GetValue (tmp, 2, GTV_REF)
	RecordsTextArea.Append ("[p]" + Align + "[/p]")

	#saving throws
	DisplaySavingThrows (pc, RecordsTextArea)

	#class features
	if HasClassFeatures(pc):
		RecordsTextArea.Append ("\n[color=ffff00]" + GemRB.GetString(40314) + "[/color]\n")
		tmp = GemRB.GetPlayerStat (pc, IE_TURNUNDEADLEVEL)
		if tmp:
			RecordsTextArea.Append (DelimitedText (12126, tmp, 0))
		# 1d6 at level 1 and +1d6 every two extra rogue levels
		tmp = GemRB.GetPlayerStat (pc, IE_LEVELTHIEF)
		if tmp:
			tmp = (tmp+1)//2
			RecordsTextArea.Append (DelimitedText (24898, str(tmp)+"d6", 0))
		tmp = GemRB.GetPlayerStat (pc, IE_LAYONHANDSAMOUNT)
		if tmp:
			RecordsTextArea.Append (DelimitedText (12127, tmp, 0))
		MonkLevel = GemRB.GetPlayerStat (pc, IE_LEVELMONK)
		if MonkLevel:
			AC = GemRB.GetCombatDetails(pc, 0)["AC"]
			GemRB.SetToken ("number", PlusMinusStat (AC["Wisdom"]))
			RecordsTextArea.Append (DelimitedText (39431, "", 0, ""))
			# wholeness of body
			RecordsTextArea.Append (DelimitedText (39749, MonkLevel*2, 0))

	# favoured enemies; eg Goblins: +2 & Harpies: +1
	RangerLevel = GemRB.GetPlayerStat (pc, IE_LEVELRANGER)
	if RangerLevel:
		RangerString = "\n[color=ffff00]"
		if RangerLevel > 5:
			RangerString += GemRB.GetString (15982)
		else:
			RangerString += GemRB.GetString (15897)
		RecordsTextArea.Append (RangerString + "[/color]\n")
		DisplayFavouredEnemy (pc, RangerLevel, -1, RecordsTextArea)
		for i in range (7):
			DisplayFavouredEnemy (pc, RangerLevel, i, RecordsTextArea)

	#bonus spells
	bonusSpells, classes = GetBonusSpells(pc)
	if len(bonusSpells):
		RecordsTextArea.Append ("\n[color=ffff00]" + GemRB.GetString(10344) + "[/color]\n")
		for c in classes:
			if not len(bonusSpells[c]):
				continue
			# class/kit name
			RecordsTextArea.Append ("[p]" + GemRB.GetString (c) + "[/p]")
			for level in range(len(bonusSpells[c])):
				AddIndent()
				# Level X: +Y
				RecordsTextArea.Append (DelimitedText(7192, "+" + str(bonusSpells[c][level]), 0, " " + str(level+1)+": "))

	#ability statistics
	RecordsTextArea.Append ("\n[color=ffff00]" + GemRB.GetString(40315) + "[/color]\n")

	# Weight Allowance
	tmp = GemRB.GetAbilityBonus( IE_STR, 3, GemRB.GetPlayerStat(pc, IE_STR) )
	RecordsTextArea.Append (DelimitedText (10338, str(tmp) + " lb."))
	# constitution bonus to hitpoints
	tmp = GUICommon.GetAbilityBonus(pc, IE_CON)
	RecordsTextArea.Append (DelimitedText (10342, PlusMinusStat(tmp)))

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
				RecordsTextArea.Append ("\n[color=ffff00]" + GemRB.GetString(39325) + "[/color]\n")
				DisplayedHeader = 1

			enchantment += 1 # since we were checking what is allowable, not what bypasses it
			if enchantment:
				enchantment = "+" + str(enchantment)
			else:
				enchantment = "-"
			reduction = "%d/%s" %(damage-mod, enchantment)
			RecordsTextArea.Append (DelimitedText(BaseString, reduction))

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
		RecordsTextArea.Append (DelimitedText (733, hits, 0))
	else:
		# account for the fact that the total apr contains the one for the other hand too
		if dualwielding:
			apr = combatdet["APR"]//2 - 1
		else:
			apr = combatdet["APR"]//2
		hits = CascadeToHit(tohit["Total"], ac, apr, combatdet["Slot"])
		RecordsTextArea.Append (DelimitedText (734, hits, 0))

	# Base
	AddIndent()
	hits = CascadeToHit(tohit["Base"], ac, apr, combatdet["Slot"])
	RecordsTextArea.Append (DelimitedText (31353, hits, 0))
	# Weapon bonus
	if tohit["Weapon"]:
		AddIndent()
		RecordsTextArea.Append (DelimitedText (32560, PlusMinusStat(tohit["Weapon"]), 0))
	# Proficiency bonus
	if tohit["Proficiency"]:
		AddIndent()
		RecordsTextArea.Append (DelimitedText (32561, PlusMinusStat(tohit["Proficiency"]), 0))
	# Armor Penalty
	if tohit["Armor"]:
		AddIndent()
		RecordsTextArea.Append (DelimitedText (39816, PlusMinusStat(tohit["Armor"]), 0))
	# Shield Penalty (if you don't have the shield proficiency feat)
	if tohit["Shield"]:
		AddIndent()
		RecordsTextArea.Append (DelimitedText (39822, PlusMinusStat(tohit["Shield"]), 0))
	# Ability  bonus
	if tohit["Ability"]:
		AddIndent()
		RecordsTextArea.Append (DelimitedText (33547, PlusMinusStat(tohit["Ability"]), 0))
	# Others
	if tohit["Generic"]:
		AddIndent()
		RecordsTextArea.Append (DelimitedText (33548, PlusMinusStat(tohit["Generic"]), 0))
	RecordsTextArea.Append ("\n")

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
		RecordsTextArea.Append (DelimitedStrRefs (41123, ammo["ItemNameIdentified"], 0, " - "))
	else:
		if dualwielding and left:
			RecordsTextArea.Append (DelimitedStrRefs (733, item["ItemNameIdentified"], 0, " - "))
		else:
			RecordsTextArea.Append (DelimitedStrRefs (734, item["ItemNameIdentified"], 0, " - "))

	# Damage
	# display the unresolved damage string (2d6)
	# this is ammo, launcher details come later
	wdice = combatdet["HitHeaderNumDice"]
	wsides = combatdet["HitHeaderDiceSides"]
	wbonus = combatdet["HitHeaderDiceBonus"]
	AddIndent()
	if wbonus:
		RecordsTextArea.Append (DelimitedText (39518, str (wdice)+"d"+str(wsides)+PlusMinusStat(wbonus), 0))
	else:
		RecordsTextArea.Append (DelimitedText (39518, str (wdice)+"d"+str(wsides), 0))
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

		dicestr = dos["TypeName"].title() + ": " + dicestr
		if dbonus:
			dicestr += PlusMinusStat(dbonus) + dchance
		else:
			dicestr += dchance
		RecordsTextArea.Append ("[p]" + dicestr + "[/p]")

		dosmax += ddice*dsides + dbonus

	# Strength
	abonus = combatdet["WeaponStrBonus"]
	if abonus:
		AddIndent()
		RecordsTextArea.Append (DelimitedText (1145, PlusMinusStat (abonus), 0))
	# Proficiency (bonus)
	pbonus = combatdet["ProfDmgBon"]
	if pbonus:
		AddIndent()
		RecordsTextArea.Append (DelimitedText (32561, PlusMinusStat(pbonus), 0))
	# Launcher
	lbonus = combatdet["LauncherDmgBon"]
	if lbonus:
		AddIndent()
		RecordsTextArea.Append (DelimitedText (41408, PlusMinusStat(lbonus), 0))
	#TODO: Power Attack has its own row
	# Damage Potential (2-12)
	# add any other bonus to the ammo damage calc
	AddIndent()
	wmin = wdice + wbonus + abonus + pbonus + lbonus + dosmin
	wmax = wdice*wsides + wbonus + abonus + pbonus + lbonus + dosmax
	RecordsTextArea.Append (DelimitedText (41120, str (wmin)+"-"+str(wmax), 0))
	# Critical Hit (19-20 / x2)
	crange = combatdet["CriticalRange"]
	cmulti = combatdet["CriticalMultiplier"]
	if crange == 20:
		crange = "20 / x" + str(cmulti)
	else:
		crange = str(crange) + "-20 / x" + str(cmulti)
	AddIndent()
	RecordsTextArea.Append (DelimitedText (41122, crange, 0))
	if not left and dualwielding:
		RecordsTextArea.Append ("\n")

def DisplayWeapons (pc):
	GS = lambda s, pc=pc: GemRB.GetPlayerStat (pc, s)

	###################
	# Attack Roll Modifiers
	RecordsTextArea.Append ("[color=ffff00]" + GemRB.GetString(9457) + "[/color]\n")

	combatdet = GemRB.GetCombatDetails(pc, 0)
	combatdet2 = combatdet # placeholder for the potential offhand numbers
	ac = combatdet["AC"]
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
		RecordsTextArea.Append (DelimitedText (9458, str (combatdet["APR"]//2-1)+"+1"))
	else:
		RecordsTextArea.Append (DelimitedText (9458, str (combatdet["APR"]//2)))

	###################
	# Armor Class
	RecordsTextArea.Append ("[color=ffff00]" + GemRB.GetString(33553) + "[/color]\n")
	RecordsTextArea.Append (DelimitedText (33553, str (GS(IE_ARMORCLASS)), 0)) # same as ac["Total"]

	# Base
	AddIndent()
	RecordsTextArea.Append (DelimitedText (31353, str (ac["Natural"]), 0))
	# Armor
	if ac["Armor"]:
		AddIndent()
		RecordsTextArea.Append (DelimitedText (11997, PlusMinusStat (ac["Armor"]), 0))
	# Shield
	if ac["Shield"]:
		AddIndent()
		RecordsTextArea.Append (DelimitedText (6347, PlusMinusStat (ac["Shield"]), 0))
	# Deflection
	if ac["Deflection"]:
		AddIndent()
		RecordsTextArea.Append (DelimitedText (33551, PlusMinusStat (ac["Deflection"]), 0))
	# Generic
	if ac["Generic"]:
		AddIndent()
		RecordsTextArea.Append (DelimitedText (33552, PlusMinusStat (ac["Generic"]), 0))
	# Dexterity
	if ac["Dexterity"]:
		AddIndent()
		RecordsTextArea.Append (DelimitedText (1151, PlusMinusStat (ac["Dexterity"]), 0))
	# Monk Wisdom Bonus: <number> to AC
	if ac["Wisdom"]:
		GemRB.SetToken ("number", PlusMinusStat (ac["Wisdom"]))
		AddIndent()
		RecordsTextArea.Append (DelimitedText (39431, "", 0, ""))

	###################
	# Armor Class Modifiers
	stat = GS (IE_ACMISSILEMOD) + GS (IE_ACSLASHINGMOD) + GS (IE_ACPIERCINGMOD) + GS (IE_ACCRUSHINGMOD)
	if stat:
		RecordsTextArea.Append ("\n[color=ffff00]" + GemRB.GetString(11766) + "[/color]\n")

		# Missile
		if GS (IE_ACMISSILEMOD):
			AddIndent()
			RecordsTextArea.Append (DelimitedText (11767, PlusMinusStat(GS (IE_ACMISSILEMOD)), 0))
		# Slashing
		if GS (IE_ACSLASHINGMOD):
			AddIndent()
			RecordsTextArea.Append (DelimitedText (11768, PlusMinusStat (GS (IE_ACSLASHINGMOD)), 0))
		# Piercing
		if GS (IE_ACPIERCINGMOD):
			AddIndent()
			RecordsTextArea.Append (DelimitedText (11769, PlusMinusStat (GS (IE_ACPIERCINGMOD)), 0))
		# Bludgeoning
		if GS (IE_ACCRUSHINGMOD):
			AddIndent()
			RecordsTextArea.Append (DelimitedText (11770, PlusMinusStat (GS (IE_ACCRUSHINGMOD))))

	###################
	# Arcane spell failure
	if GS(IE_LEVELBARD) + GS(IE_LEVELSORCERER) + GS(IE_LEVELMAGE):
		RecordsTextArea.Append ("\n[color=ffff00]" + GemRB.GetString(41391) + "[/color]\n")

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
		RecordsTextArea.Append (DelimitedText (41390, str (total)+"%", 0))
		# Armor Penalty (same as for skills and everything else)
		if verbose and failure["Armor"]:
			AddIndent()
			RecordsTextArea.Append (DelimitedText (39816, PlusMinusStat(5*failure["Armor"])+"%", 0))
		# Shield Penalty
		if verbose and failure["Shield"]:
			AddIndent()
			RecordsTextArea.Append (DelimitedText (39822, PlusMinusStat(5*failure["Shield"])+"%", 0))
		if arcana:
			AddIndent()
			RecordsTextArea.Append (DelimitedText (36352, PlusMinusStat(-arcana)+"%", 0))
		# Other, just a guess to show the remainder
		if other:
			AddIndent()
			RecordsTextArea.Append (DelimitedText (33548, PlusMinusStat(other)+"%", 0))

	###################
	# Weapon Statistics
	RecordsTextArea.Append ("\n[color=ffff00]" + GemRB.GetString(41119) + "[/color]\n")

	# Main hand
	WeaponOfHand(pc, combatdet, dualwielding)

	# Off-hand (if any)
	if dualwielding:
		WeaponOfHand(pc, combatdet2, dualwielding, 1)

	DisplaySavingThrows (pc, RecordsTextArea)

	return

def DisplaySkills (pc, SkillsArea):
	
	def PrintStatTable (title, tabname):
		feats = True if tabname == "feats" else False
		lookup = "featreq" if feats else "skillsta"

		nameTab = GemRB.LoadTable (tabname)
		itemTab = GemRB.LoadTable (lookup)
		rows = itemTab.GetRowCount ()
		
		SkillsArea.Append ("[color=ffff00]" + title + "[/color]\n")
		
		items = []
		for i in range(rows):
			item = itemTab.GetValue (i, 0, GTV_STAT)
			name = nameTab.GetValue (i, 1, GTV_REF)

			if feats and GemRB.HasFeat(pc, i):
				value = (name,) if not item else (name, GemRB.GetPlayerStat (pc, item),)
				items.append (value)
			elif not feats:
				value = GemRB.GetPlayerStat (pc, item)
				base = GemRB.GetPlayerStat (pc, item, 1)
				untrained = nameTab.GetValue (i, 3)
				# only show positive (modified) skills that either don't require training or had it already
				if (untrained and value == base and value > 0):
					items.append((name, str(value),))
				elif (value>0 and untrained) or (not untrained and base):
					items.append((name, str(value) + " (" + str(base) + ")",))

		items.sort()
		for item in items:
			if len(item) > 1:
				SkillsArea.Append ("[p]" + item[0] + ": " + str(item[1]) + "[/p]")
			else:
				SkillsArea.Append ("[p]" + item[0] + "[/p]")
		return

	PrintStatTable (GemRB.GetString(11983), "skills")
	SkillsArea.Append ("\n")
	PrintStatTable (GemRB.GetString(36361), "feats")

	return

def DelimitedStrRefs(strref1, strref2, newlines=1, delimiter=": "):
	text = GemRB.GetString(strref2) 
	return DelimitedText(strref1, text, newlines, delimiter)

def DelimitedText(strref, text, newlines=1, delimiter=": "):
	return "[p]" + GemRB.GetString(strref) + delimiter + str(text) + "[/p]" + ("\n" * newlines)

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
	RecordsTextArea.Append (DelimitedStrRefs (11949, stat['FavouriteSpell'], 0))
	RecordsTextArea.Append (DelimitedStrRefs (11950, stat['FavouriteWeapon'], 0))

	# combat details
	RecordsTextArea.Append ("\n[color=ffff00]" + GemRB.GetString(40322) + "[/color]\n")

	#most powerful vanquished, time spent, xp and kills
	RecordsTextArea.Append (DelimitedStrRefs (11947, stat['BestKilledName'], 0))

	time = GUICommon.SetCurrentDateTokens (stat)
	RecordsTextArea.Append (DelimitedText (11948, time, 0))

	# Experience Value of Kills
	RecordsTextArea.Append (DelimitedText (11953, stat['KillsTotalXP'], 0))

	# Number of Kills
	RecordsTextArea.Append (DelimitedText (11954, stat['KillsTotalCount'], 0))

	# Total Experience Value in Party
	if TotalPartyExp:
		val = stat['KillsTotalXP']*100/TotalPartyExp
	else:
		val = 0
	RecordsTextArea.Append (DelimitedText (11951, str(val) + "%", 0))

	# Percentage of Total Kills in Party
	if TotalPartyExp:
		val = stat['KillsTotalCount']*100/TotalCount
	else:
		val = 0
	RecordsTextArea.Append (DelimitedText (11952, str(val) + "%", 0))

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
		DisplayGeneral (pc, RecordsTextArea)
	elif SelectWindow == 2:
		DisplayWeapons (pc)
	elif SelectWindow == 3:
		DisplaySkills (pc, RecordsTextArea)
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
		#Button = Window.GetControl (i+27)
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

LUWindow = None
LUKitWindow = None
LevelDiff = 1
def CloseLUWindow ():
	global LUWindow, LUKitWindow

	if LUKitWindow:
		LUKitWindow.Unload ()
		LUKitWindow = None

	if LUWindow:
		LUWindow.Unload ()
		LUWindow = None

def OpenLevelUpWindow ():
	global LUWindow, LevelDiff

	LUWindow = Window = GemRB.LoadWindow (54)

	# Figure out the level difference
	# the original ignored all but the fighter row in xplevel.2da
	# we do the same, since IE_CLASS becomes useless for mc actors
	# (iwd2 never updates it; it's not a bitfield like IE_KIT)
	pc = GemRB.GameGetSelectedPCSingle ()
	xp = GemRB.GetPlayerStat (pc, IE_XP)
	nextLevel = LUCommon.GetNextLevelFromExp (xp, 5)
	levelSum = GemRB.GetPlayerStat (pc, IE_CLASSLEVELSUM)
	LevelDiff = nextLevel - levelSum - GetECL(pc)
	print 1111111, nextLevel, levelSum, GetECL(pc), LevelDiff

	# next
	Button = Window.GetControl (0)
	Button.SetText (36789)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, OpenLUKitWindow)
	Button.SetState (IE_GUI_BUTTON_DISABLED)
	Button.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)

	# static Class selection
	#Label = Window.GetControl (0x10000000)
	#Label.SetText (7175)

	# 2-12 are the class name buttons
	# 15-25 are the class level labels
	GemRB.SetVar ("LUClass", -1)
	for i in range(2,13):
		Button = Window.GetControl (i)
		Label = Window.GetControl (0x10000000 + i+13)

		ClassTitle = CommonTables.Classes.GetRowName (i-2)
		ClassTitle = CommonTables.Classes.GetValue (ClassTitle, "NAME_REF", GTV_REF)
		Button.SetText (ClassTitle)
		level = GemRB.GetPlayerStat (pc, Classes[i-2])
		if level > 0:
			Label.SetText (str(level))

		# disable monks/paladins due to order restrictions?
		specflag = GemRB.GetPlayerStat (pc, IE_SPECFLAGS)
		if specflag&SPECF_MONKOFF and i == 7:
			Button.SetState (IE_GUI_BUTTON_DISABLED)
		elif specflag&SPECF_PALADINOFF and i == 8:
			Button.SetState (IE_GUI_BUTTON_DISABLED)
		else:
			Button.SetState (IE_GUI_BUTTON_ENABLED)
		Button.SetVarAssoc ("LUClass", i-2)
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, LUClassPress)
		Button.SetFlags (IE_GUI_BUTTON_RADIOBUTTON, OP_OR)

	# description
	TextArea = Window.GetControl (13)
	TextArea.SetText (17242)

	# 14 scrollbar
	# 14 static Level label
	# 26 does not exist

	# cancel
	Button = Window.GetControl (27)
	Button.SetText (13727)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, CloseLUWindow)
	Button.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)

	Window.ShowModal (MODAL_SHADOW_NONE)

OldLevelLabel = 0
def LUClassPress ():
	global OldLevelLabel

	Window = LUWindow
	Button = Window.GetControl (0)
	Button.SetState (IE_GUI_BUTTON_ENABLED)

	# display class info
	TextArea = Window.GetControl (13)
	i = GemRB.GetVar ("LUClass")
	ClassDesc = CommonTables.Classes.GetRowName (i)
	ClassDesc = CommonTables.Classes.GetValue (ClassDesc, "DESC_REF", GTV_REF)
	TextArea.SetText (ClassDesc)

	# increase/decrease level label by LevelDiff (usually 1)
	if OldLevelLabel != i:
		pc = GemRB.GameGetSelectedPCSingle ()
		j = OldLevelLabel
		Label = Window.GetControl (0x10000000 + j+15)
		level = Label.QueryText ()
		if level == "":
			level = 1
		level = int(level)
		if level-LevelDiff > 0:
			Label.SetText (str(level - LevelDiff))
		else:
			Label.SetText ("")

		OldLevelLabel = i
		Label = Window.GetControl (0x10000000 + i+15)
		level = GemRB.GetPlayerStat (pc, Classes[i])
		Label.SetText (str(level + LevelDiff))

# if the pc is a paladin or a monk, multiclassing is limited;
# only their school's favoured class is fair game, otherwise
# they lose access to further paladin/monk progression
def HandleSpecFlagExclusion(pc, lucls, lukit):
	# RESEARCH: it's unclear what happened if you multiclassed into a (monk) from a bad class
	monkLevel = GemRB.GetPlayerStat (pc, IE_LEVELMONK)
	paladinLevel = GemRB.GetPlayerStat (pc, IE_LEVELPALADIN)
	if monkLevel == 0 and paladinLevel == 0 and lucls != 6 and lucls != 7:
		return

	# TODO: unhardcode this kit->kit/class dict
	# paladins: ilmater, helm->fighter, mystra->wizard
	# monks: old->rogue, broken->ilmater, dark moon->sorcerer
	orderFavs = { 1: 0x8000, 2: 5, 4: 11, 8: 9, 0x10: 0x8000, 0x20: 10 }
	kit = GemRB.GetPlayerStat (pc, IE_KIT, 1)

	# the most common level-up paths - same class
	if GemRB.GetPlayerStat (pc, IDLUCommon.Levels[lucls-1]) > 0:
		if lukit == 0:
			# kitless class, matching current one(s)
			# not going to happen with vanilla data
			return
		else:
			# picked kit, matching current one(s)
			if lukit&kit:
				return
			else:
				raise RuntimeError, "Picked second or non-existing kit (%d) of same class (%d)!?" %(lukit, lucls)
	else: # picked a new class
		kitBits = [((kit >> x) & 1)<<x  for x in range(31, -1, -1)]
		kitBits = filter(lambda v: v > 0, kitBits)
		# check all the current kits - kit bits
		for k in kitBits:
			if k not in orderFavs:
				continue
			# picked a new kitless class
			if lukit == 0 and orderFavs[k] == lucls:
				return
			# picked a new kit
			if lukit > 0 and orderFavs[k] == lukit:
				return

	# only bad combinations remain
	# disable further level-ups in the base class
	specFlags = GemRB.GetPlayerStat (pc, IE_SPECFLAGS)
	if monkLevel:
		specFlags |= SPECF_MONKOFF
	if paladinLevel:
		specFlags |= SPECF_PALADINOFF
	GemRB.SetPlayerStat (pc, IE_SPECFLAGS, specFlags)

def OpenLUKitWindow ():
	global LUKitWindow

	# check if kit selection is needed at all
	# does the class have any kits at all?
	# if we're levelling up, we must also check we dont't already
	# have a kit with this class
	pc = GemRB.GameGetSelectedPCSingle ()
	LUClass = GemRB.GetVar ("LUClass") # index, not ID
	LUClassName = CommonTables.Classes.GetRowName (LUClass)
	LUClassID = CommonTables.Classes.GetValue (LUClassName, "ID")
	hasKits = CommonTables.Classes.FindValue ("CLASS", LUClassID)
	kitIndex = GUICommonWindows.GetKitIndex (pc, LUClass)
	if hasKits == -1 or kitIndex > 0:
		kitName = CommonTables.Classes.GetRowName (kitIndex)
		kitID = CommonTables.Classes.GetValue (kitName, "ID", GTV_INT)
		if hasKits == -1:
			kitID = 0
		HandleSpecFlagExclusion(pc, LUClassID, kitID)
		LUNextPress ()
		return

	LUKitWindow = Window = GemRB.LoadWindow (6)

	# title
	Label = Window.GetControl (0x10000000)
	Label.SetText (9894)

	# next
	Button = Window.GetControl (0)
	Button.SetText (36789)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, LUNextPress)
	Button.SetState (IE_GUI_BUTTON_DISABLED)
	Button.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)

	# 1 does not exist, 2-10 are kit buttons
	# 11 scrollbar, 12 back
	Window.DeleteControl (12)

	# description
	TextArea = Window.GetControl (13)
	TextArea.SetText ("")

    # skip to the first kit of this class
	kitOffset = hasKits

	GemRB.SetVar ("LUKit", -1)
	for i in range(9):
		Button = Window.GetControl (i+2)

		kitClassName = CommonTables.Classes.GetRowName (kitOffset+i)
		kitClass = CommonTables.Classes.GetValue (kitClassName, "CLASS", GTV_INT)
		if kitClass != LUClassID:
			Button.SetState (IE_GUI_BUTTON_DISABLED)
			continue

		kitTitle = CommonTables.Classes.GetRowName (kitOffset+i)
		kitTitle = CommonTables.Classes.GetValue (kitTitle, "NAME_REF", GTV_REF)
		Button.SetText (kitTitle)
		Button.SetState (IE_GUI_BUTTON_ENABLED)
		Button.SetVarAssoc ("LUKit", i)
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, LUKitPress)
		Button.SetFlags (IE_GUI_BUTTON_RADIOBUTTON, OP_OR)

	Window.ShowModal (MODAL_SHADOW_NONE)

def LUKitPress ():
	Window = LUKitWindow
	Button = Window.GetControl (0)
	Button.SetState (IE_GUI_BUTTON_ENABLED)

	# display kit info
	TextArea = Window.GetControl (13)
	i = GemRB.GetVar ("LUKit")
	LUClass = GemRB.GetVar ("LUClass")
	LUClassName = CommonTables.Classes.GetRowName (LUClass)
	LUClassID = CommonTables.Classes.GetValue (LUClassName, "ID")
	kitOffset = CommonTables.Classes.FindValue ("CLASS", LUClassID)
	kitName = CommonTables.Classes.GetRowName (i+kitOffset)
	kitDesc = CommonTables.Classes.GetValue (kitName, "DESC_REF", GTV_REF)
	TextArea.SetText (kitDesc)

	# set it to the kit value, so we don't need these gimnastics later
	kitID = CommonTables.Classes.GetValue (kitName, "ID", GTV_INT)
	GemRB.SetVar ("LUKit", kitID)
	pc = GemRB.GameGetSelectedPCSingle ()
	HandleSpecFlagExclusion(pc, LUClassID, kitID)

	oldKits = GemRB.GetPlayerStat (pc, IE_KIT, 1)
	GemRB.SetPlayerStat (pc, IE_KIT, oldKits|kitID)

# continue with level up via chargen methods
def LUNextPress ():
	CloseLUWindow ()

	# pass on LevelDiff and selected class (already in LUClass)
	GemRB.SetVar ("LevelDiff", LevelDiff)

	# grant an ability point or three (each 4 levels)
	pc = GemRB.GameGetSelectedPCSingle ()
	levelSum = GemRB.GetPlayerStat (pc, IE_CLASSLEVELSUM)
	rankDiff = (levelSum+LevelDiff)//4 - levelSum//4
	if rankDiff > 0:
		import Abilities
		Abilities.OpenAbilitiesWindow (0, rankDiff)
	else:
		import Enemy
		Enemy.OpenEnemyWindow ()
	# both fire up the rest of the chain

def FinishLevelUp():
	# kit
	pc = GemRB.GameGetSelectedPCSingle ()
	LUKit = GemRB.GetVar ("LUKit")

	# saving throws
	LUClass = GemRB.GetVar ("LUClass") # index, not ID
	LUClassName = CommonTables.Classes.GetRowName (LUClass)
	LUClassID = CommonTables.Classes.GetValue (LUClassName, "ID")
	IDLUCommon.SetupSavingThrows (pc, LUClassID)

	# hit points
	Levels = [ GemRB.GetPlayerStat (pc, l) for l in IDLUCommon.Levels ]
	LevelDiff = GemRB.GetVar ("LevelDiff")
	LevelDiffs = [ 0 ] * len(Levels)
	LevelDiffs[LUClass] = LevelDiff
	# SetupHP expects the target level already
	Levels[LUClass] += LevelDiff
	LUCommon.SetupHP (pc, Levels, LevelDiffs)

	# add class/kit resistances iff we chose a new class
	levelStat = IDLUCommon.Levels[LUClass]
	oldLevel = GemRB.GetPlayerStat(pc, levelStat, 1)
	# use the kit name if it is available
	LUKitName = LUClassName
	if LUKit != 0:
		kitIndex = CommonTables.Classes.FindValue ("ID", LUKit)
		LUKitName = CommonTables.Classes.GetRowName (kitIndex)
	if oldLevel == 0:
		IDLUCommon.AddResistances (pc, LUKitName, "clssrsmd")

	# bab (to hit)
	BABTable = CommonTables.Classes.GetValue (LUClassName, "TOHIT")
	BABTable = GemRB.LoadTable (BABTable)
	currentBAB = GemRB.GetPlayerStat (pc, IE_TOHIT, 1)
	oldBAB = BABTable.GetValue (str(oldLevel), "BASE_ATTACK")
	newLevel = oldLevel + LevelDiff
	newBAB = BABTable.GetValue (str(newLevel), "BASE_ATTACK")
	GemRB.SetPlayerStat (pc, IE_TOHIT, currentBAB + newBAB - oldBAB)

	# class level
	GemRB.SetPlayerStat (pc, levelStat, newLevel)

	# now we're finally done
	GemRB.SetVar ("LevelDiff", 0)
	GemRB.SetVar ("LUClass", -1)
	GemRB.SetVar ("LUKit", 0)
	# DisplayGeneral (pc) is not enough for a refresh refresh
	OpenRecordsWindow ()
	OpenRecordsWindow ()

###################################################
# End of file GUIREC.py
