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
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#
# $Id$


# GUIREC.py - scripts to control stats/records windows from GUIREC winpack
###################################################
import GemRB
import GUICommonWindows
from GUIDefines import *
from ie_stats import *
from GUICommon import *
from LUCommon import CanLevelUp, GetNextLevelExp
from GUICommonWindows import *
from GUIWORLD import OpenReformPartyWindow

import Portrait

###################################################
RecordsWindow = None
InformationWindow = None
BiographyWindow = None
PortraitWindow = None
CustomPortraitWindow = None
OptionsWindow = None
CustomizeWindow = None
OldPortraitWindow = None
OldOptionsWindow = None

###################################################
def OpenRecordsWindow ():
	global RecordsWindow, PortraitWindow, OptionsWindow
	global OldPortraitWindow, OldOptionsWindow

	if CloseOtherWindow (OpenRecordsWindow):
		if InformationWindow: OpenInformationWindow ()

		if RecordsWindow:
			RecordsWindow.Unload ()
		if OptionsWindow:
			OptionsWindow.Unload ()
		if PortraitWindow:
			PortraitWindow.Unload ()
		if CustomPortraitWindow:
			CustomPortraitWindow.Unload()
		RecordsWindow = None
		GemRB.SetVar ("OtherWindow", -1)
		GemRB.SetVisible (0,1)
		GemRB.UnhideGUI ()
		GUICommonWindows.PortraitWindow = OldPortraitWindow
		OldPortraitWindow = None
		GUICommonWindows.OptionsWindow = OldOptionsWindow
		OldOptionsWindow = None
		SetSelectionChangeHandler (None)
		return

	GemRB.HideGUI ()
	GemRB.SetVisible (0,0)

	GemRB.LoadWindowPack ("GUIREC", 640, 480)
	RecordsWindow = Window = GemRB.LoadWindowObject (2)
	GemRB.SetVar ("OtherWindow", RecordsWindow.ID)
	#saving the original portrait window
	OldOptionsWindow = GUICommonWindows.OptionsWindow
	OptionsWindow = GemRB.LoadWindowObject (0)
	SetupMenuWindowControls (OptionsWindow, 0, "OpenRecordsWindow")
	OptionsWindow.SetFrame ()
	OldPortraitWindow = GUICommonWindows.PortraitWindow
	PortraitWindow = OpenPortraitWindow (0)

	# dual class
	Button = Window.GetControl (0)
	Button.SetText (7174)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "DualClassWindow")

	# levelup
	Button = Window.GetControl (37)
	Button.SetText (7175)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "OpenLevelUpWindow")

	# information
	Button = Window.GetControl (1)
	Button.SetText (11946)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "OpenInformationWindow")

	# reform party
	Button = Window.GetControl (51)
	Button.SetText (16559)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "OpenReformPartyWindow")

	# customize
	Button = Window.GetControl (50)
	Button.SetText (10645)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "OpenCustomizeWindow")

	# export
	Button = Window.GetControl (36)
	Button.SetText (13956)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "OpenExportWindow")

## 	# kit info
## 	Button = Window.GetControl (52)
## 	Button.SetText (61265)
## 	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "KitInfoWindow")

	SetSelectionChangeHandler (UpdateRecordsWindow)
	UpdateRecordsWindow ()
	OptionsWindow.SetVisible (1)
	Window.SetVisible (3)
	PortraitWindow.SetVisible (1)
	return

def UpdateRecordsWindow ():
	global stats_overview, alignment_help

	Window = RecordsWindow
	if not RecordsWindow:
		print "SelectionChange handler points to non existing window\n"
		return

	pc = GemRB.GameGetSelectedPCSingle ()

	# exportable
	Button = Window.GetControl (36)
	if GemRB.GetPlayerStat (pc, IE_MC_FLAGS)&MC_EXPORTABLE:
		Button.SetState (IE_GUI_BUTTON_ENABLED)
	else:
		Button.SetState (IE_GUI_BUTTON_DISABLED)

	# dual-classable
	Button = Window.GetControl (0)
	if CanDualClass (pc):
		Button.SetState (IE_GUI_BUTTON_DISABLED)
	else:
		Button.SetState (IE_GUI_BUTTON_ENABLED)
	
	# levelup
	Button = Window.GetControl (37)
	if CanLevelUp (pc):
		Button.SetState (IE_GUI_BUTTON_ENABLED)
	else:
		Button.SetState (IE_GUI_BUTTON_DISABLED)

	# name
	Label = Window.GetControl (0x1000000e)
	Label.SetText (GemRB.GetPlayerName (pc, 0))

	# portrait
	Button = Window.GetControl (2)
	Button.SetState (IE_GUI_BUTTON_LOCKED)
	Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE | IE_GUI_BUTTON_PICTURE, OP_SET)
	Button.SetPicture (GemRB.GetPlayerPortrait (pc,0), "NOPORTMD")

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

	# stats

	sstr = GemRB.GetPlayerStat (pc, IE_STR)
	sstrx = GemRB.GetPlayerStat (pc, IE_STREXTRA)
	cstr = GetStatColor(pc, IE_STR)
	if sstrx > 0 and sstr==18:
		sstr = "%d/%02d" %(sstr, sstrx % 100)
	else:
		sstr = str(sstr)
	sint = str(GemRB.GetPlayerStat (pc, IE_INT))
	cint = GetStatColor(pc, IE_INT)
	swis = str(GemRB.GetPlayerStat (pc, IE_WIS))
	cwis = GetStatColor(pc, IE_WIS)
	sdex = str(GemRB.GetPlayerStat (pc, IE_DEX))
	cdex = GetStatColor(pc, IE_DEX)
	scon = str(GemRB.GetPlayerStat (pc, IE_CON))
	ccon = GetStatColor(pc, IE_CON)
	schr = str(GemRB.GetPlayerStat (pc, IE_CHR))
	cchr = GetStatColor(pc, IE_CHR)

	Label = Window.GetControl (0x1000002f)
	Label.SetText (sstr)
	Label.SetTextColor (cstr[0], cstr[1], cstr[2])

	Label = Window.GetControl (0x10000009)
	Label.SetText (sdex)
	Label.SetTextColor (cdex[0], cdex[1], cdex[2])

	Label = Window.GetControl (0x1000000a)
	Label.SetText (scon)
	Label.SetTextColor (ccon[0], ccon[1], ccon[2])

	Label = Window.GetControl (0x1000000b)
	Label.SetText (sint)
	Label.SetTextColor (cint[0], cint[1], cint[2])

	Label = Window.GetControl (0x1000000c)
	Label.SetText (swis)
	Label.SetTextColor (cwis[0], cwis[1], cwis[2])

	Label = Window.GetControl (0x1000000d)
	Label.SetText (schr)
	Label.SetTextColor (cchr[0], cchr[1], cchr[2])

	# class
	ClassTitle = GetActorClassTitle (pc)
	Label = Window.GetControl (0x10000030)
	Label.SetText (ClassTitle)

	# race
	text = RaceTable.GetValue (RaceTable.FindValue (3, GemRB.GetPlayerStat (pc, IE_RACE)) ,
 0)

	Label = Window.GetControl (0x1000000f)
	Label.SetText (text)

	Table = GemRB.LoadTableObject ("aligns")

	text = Table.GetValue (Table.FindValue ( 3, GemRB.GetPlayerStat (pc, IE_ALIGNMENT) ), 0)
	Label = Window.GetControl (0x10000010)
	Label.SetText (text)

	Label = Window.GetControl (0x10000011)
	if GemRB.GetPlayerStat (pc, IE_SEX) == 1:
		Label.SetText (7198)
	else:
		Label.SetText (7199)

	# help, info textarea
	stats_overview = GetStatOverview (pc)
	Text = Window.GetControl (45)
	Text.SetText (stats_overview)
	#making window visible/shaded depending on the pc's state
	Window.SetVisible (1)
	return
	
def GetStatColor (pc, stat):
	a = GemRB.GetPlayerStat(pc, stat)
	b = GemRB.GetPlayerStat(pc, stat, 1)
	if a==b:
		return (255,255,255)
	if a<b:
		return (255,255,0)
	return (0,255,0)

# GemRB.GetPlayerStat wrapper that only returns nonnegative values
def GSNN (pc, stat):
	val = GemRB.GetPlayerStat (pc, stat)
	if val >= 0:
		return val
	else:
		return 0

# LevelDiff is used only from the level up code and holds the level
# difference for each class
def GetStatOverview (pc, LevelDiff=[0,0,0]):
	StateTable = GemRB.LoadTableObject ("statdesc")

	GS = lambda s, pc=pc: GemRB.GetPlayerStat (pc, s)
	GA = lambda s, col, pc=pc: GemRB.GetAbilityBonus (s, col, GS (s) )

	stats = []
	# class levels
	# 16480 <CLASS>: Level <LEVEL>
	# Experience: <EXPERIENCE>
	# Next Level: <NEXTLEVEL>

	# collecting tokens for stat overview
	ClassTitle = GetActorClassTitle (pc)
	GemRB.SetToken ("CLASS", ClassTitle)
	Class = GemRB.GetPlayerStat (pc, IE_CLASS)
	Class = ClassTable.FindValue (5, Class)
	Class = ClassTable.GetRowName (Class)
	Dual = IsDualClassed (pc, 1)
	Multi = IsMultiClassed (pc, 1)
	XP = GemRB.GetPlayerStat (pc, IE_XP)
	LevelDrain = GS (IE_LEVELDRAIN)

	if Multi[0] > 1: # we're multiclassed
		print "\tMulticlassed"
		Levels = [GemRB.GetPlayerStat (pc, IE_LEVEL), GemRB.GetPlayerStat (pc, IE_LEVEL2), GemRB.GetPlayerStat (pc, IE_LEVEL3)]

		stats.append ( (19721,1,'c') )
		stats.append (None)
		for i in range (Multi[0]):
			ClassIndex = ClassTable.FindValue (5, Multi[i+1])
			ClassTitle = GemRB.GetString (ClassTable.GetValue (ClassIndex, 2))
			GemRB.SetToken ("CLASS", ClassTitle)
			Class = ClassTable.GetRowName (ClassIndex)
			GemRB.SetToken ("LEVEL", str (Levels[i]+LevelDiff[i]-int(LevelDrain/Multi[0])) )
			GemRB.SetToken ("EXPERIENCE", str (XP/Multi[0]) )
			if LevelDrain:
				stats.append ( (GemRB.GetString (19720),1,'d') )
				stats.append ( (GemRB.GetString (57435),1,'d') ) # LEVEL DRAINED
			else:
				GemRB.SetToken ("NEXTLEVEL", GetNextLevelExp (Levels[i]+LevelDiff[i], Class) )
				stats.append ( (GemRB.GetString (16480),"",'d') )
			stats.append (None)
			print "\t\tClass (Level):",Class,"(",Levels[i],")"

	elif Dual[0] > 0: # dual classed; first show the new class
		print "\tDual classed"
		stats.append ( (19722,1,'c') )
		stats.append (None)

		Levels = [GemRB.GetPlayerStat (pc, IE_LEVEL), GemRB.GetPlayerStat (pc, IE_LEVEL2), GemRB.GetPlayerStat (pc, IE_LEVEL3)]

		# the levels are stored in the class order (eg. FIGHTER_MAGE)
		# the current active class does not matter!
		if IsDualSwap (pc):
			Levels = [Levels[1], Levels[0], Levels[2]]

		Levels[0] += LevelDiff[0]

		ClassTitle = GemRB.GetString (ClassTable.GetValue (Dual[2], 2))
		GemRB.SetToken ("CLASS", ClassTitle)
		GemRB.SetToken ("LEVEL", str (Levels[0]-LevelDrain))
		Class = ClassTable.GetRowName (Dual[2])
		XP2 = GemRB.GetPlayerStat (pc, IE_XP)
		GemRB.SetToken ("EXPERIENCE", str (XP2) )
		if LevelDrain:
			stats.append ( (GemRB.GetString (19720),1,'d') )
			stats.append ( (GemRB.GetString (57435),1,'d') ) # LEVEL DRAINED
		else:
			GemRB.SetToken ("NEXTLEVEL", GetNextLevelExp (Levels[0], Class) )
			stats.append ( (GemRB.GetString (16480),"",'d') )
		stats.append (None)
		# the first class (shown second)
		if Dual[0] == 1:
			ClassTitle = GemRB.GetString (KitListTable.GetValue (Dual[1], 2))
		elif Dual[0] == 2:
			ClassTitle = GemRB.GetString (ClassTable.GetValue (Dual[1], 2))
		GemRB.SetToken ("CLASS", ClassTitle)
		GemRB.SetToken ("LEVEL", str (Levels[1]) )

		# the xp table contains only classes, so we have to determine the base class for kits
		if Dual[0] == 2:
			BaseClass = ClassTable.GetRowName (Dual[1])
		else:
			BaseClass = GetKitIndex (pc)
			BaseClass = KitListTable.GetValue (BaseClass, 7)
			BaseClass = ClassTable.FindValue (5, BaseClass)
			BaseClass = ClassTable.GetRowName (BaseClass)
		# the first class' XP is discarded and set to the minimum level
		# requirement, so if you don't dual class right after a levelup,
		# the game would eat some of your XP
		XP1 = NextLevelTable.GetValue (BaseClass, str (Levels[1]))
		GemRB.SetToken ("EXPERIENCE", str (XP1) )

		# inactive until the new class SURPASSES the former
		if Levels[0] <= Levels[1]:
			# inactive
			stats.append ( (19719,1,'c') )
		else:
			stats.append ( (19720,1,'c') )
		stats.append (None)
	else: # single classed
		print "\tSingle classed"
		Level = GemRB.GetPlayerStat (pc, IE_LEVEL) + LevelDiff[0]
		GemRB.SetToken ("LEVEL", str (Level-LevelDrain))
		GemRB.SetToken ("EXPERIENCE", str (XP) )
		if LevelDrain:
			stats.append ( (19720,1,'c') )
			stats.append ( (57435,1,'c') ) # LEVEL DRAINED
		else:
			GemRB.SetToken ("NEXTLEVEL", GetNextLevelExp (Level, Class) )
			stats.append ( (16480,1,'c') )
		stats.append (None)
		print "\t\tClass (Level):",Class,"(",Level,")"

	# check to see if we have a level diff anywhere
	if sum (LevelDiff) == 0:
		effects = GemRB.GetPlayerStates (pc)
		if len (effects):
			for c in effects:
				tmp = StateTable.GetValue (ord(c)-66, 0)
				stats.append ( (tmp,c,'a') )
			stats.append (None)

	stats.append (None)

	#proficiencies
	stats.append ( (8442,1,'c') )

	stats.append ( (9457, GemRB.GetCombatDetails(pc, 0)["ToHit"], '0') )
	tmp = GS (IE_NUMBEROFATTACKS)
	if (tmp&1):
		tmp2 = str (tmp/2) + chr (188)
	else:
		tmp2 = str (tmp/2)
	stats.append ( (9458, tmp2, '') )
	stats.append ( (9459, GSNN (pc, IE_LORE), '0') )
	stats.append ( (19224, GS (IE_RESISTMAGIC), '') )

	# party's reputation
	reptxt = GetReputation (GemRB.GameGetReputation()/10)
	stats.append ( (9465, reptxt, '') )
	stats.append ( (9460, GSNN (pc, IE_LOCKPICKING), '') )
	stats.append ( (9462, GSNN (pc, IE_TRAPS), '') )
	stats.append ( (9463, GSNN (pc, IE_PICKPOCKET), '') )
	stats.append ( (9461, GSNN (pc, IE_STEALTH), '') )
	stats.append ( (34120, GSNN (pc, IE_HIDEINSHADOWS), '') )
	stats.append ( (34121, GSNN (pc, IE_DETECTILLUSIONS), '') )
	stats.append ( (34122, GSNN (pc, IE_SETTRAPS), '') )
	HatedRace = GS (IE_HATEDRACE)
	if HatedRace:
		HateTable = GemRB.LoadTableObject ("haterace")
		Racist = HateTable.FindValue (1, HatedRace)
		if Racist != -1:
			HatedRace = HateTable.GetValue (Racist, 0)
			stats.append ( (15982, GemRB.GetString (HatedRace), '') )
	stats.append ( (12128, GS (IE_BACKSTABDAMAGEMULTIPLIER), 'x') )
	stats.append ( (12126, GS (IE_TURNUNDEADLEVEL), '') )

	#this hack only displays LOH if we know the spell
	#TODO: the core should just not set LOH if the paladin can't learn it
	if (HasSpell (pc, IE_SPELL_TYPE_INNATE, 0, "SPCL211") >= 0):
		stats.append ( (12127, GS (IE_LAYONHANDSAMOUNT), '') )

	#script
	aiscript = GemRB.GetPlayerScript (pc )
	stats.append ( (2078, aiscript, '') )
	stats.append (None)

	#   17379 Saving Throws
	stats.append (17379)
	#   17380 Death
	stats.append ( (17380, GS (IE_SAVEVSDEATH), '0') )
	#   17381
	stats.append ( (17381, GS (IE_SAVEVSWANDS), '0') )
	#   17382 AC vs. Crushing
	stats.append ( (17382, GS (IE_SAVEVSPOLY), '0') )
	#   17383 Rod
	stats.append ( (17383, GS (IE_SAVEVSBREATH), '0') )
	#   17384 Spells
	stats.append ( (17384, GS (IE_SAVEVSSPELL), '0') )
	stats.append (None)

	# 9466 Weapon proficiencies
	stats.append (9466)
	table = GemRB.LoadTableObject ("weapprof")
	RowCount = table.GetRowCount ()
	for i in range(RowCount):
		text = table.GetValue (i, 3)
		stat = table.GetValue (i, 0)
		stats.append ( (text, GS(stat)&0x07, '+') )
	stats.append (None)

	# 11766 AC Bonuses
	stats.append (11766)
	#   11770 AC vs. Crushing
	stats.append ((11770, GS (IE_ACCRUSHINGMOD), ''))
	#   11767 AC vs. Missile
	stats.append ((11767, GS (IE_ACMISSILEMOD), ''))
	#   11769 AC vs. Piercing
	stats.append ((11769, GS (IE_ACPIERCINGMOD), ''))
	#   11768 AC vs. Slashing
	stats.append ((11768, GS (IE_ACSLASHINGMOD), ''))
	stats.append (None)

	# 10315 Ability bonuses
	stats.append (10315)
	value = GemRB.GetPlayerStat (pc, IE_STR)
	ex = GemRB.GetPlayerStat (pc, IE_STREXTRA)
	# 10332 to hit
	stats.append ( (10332, GemRB.GetAbilityBonus(IE_STR,0,value,ex), '0') )
	# 10336 damage
	stats.append ( (10336, GemRB.GetAbilityBonus(IE_STR,1,value,ex), '0') )
	# 10337 open doors (bend bars lift gates)
	stats.append ( (10337, GemRB.GetAbilityBonus(IE_STR,2,value,ex), '0') )
	# 10338 weight allowance
	stats.append ( (10338, GemRB.GetAbilityBonus(IE_STR,3,value,ex), '0') )
	# 10339 AC
	stats.append ( (10339, GA(IE_DEX,2), '0') )
	# 10340 Missile adjustment
	stats.append ( (10340, GA(IE_DEX,1), '0') )
	# 10341 Reaction adjustment
	stats.append ( (10341, GA(IE_DEX,0), '0') )
	# 10342 CON HP Bonus/Level
	stats.append ( (10342, GA(IE_CON,0), '0') )
	# 10343 Chance To Learn spell
	if GemRB.GetMemorizableSpellsCount (pc, IE_SPELL_TYPE_WIZARD, 0, 0)>0:
		stats.append ((10343, GA (IE_INT,0), '%' ))
	# 10347 Reaction
	stats.append ( (10347, GA(IE_REPUTATION,0), '0') )
	stats.append (None)

	# 10344 Bonus Priest spells
	if GemRB.GetMemorizableSpellsCount (pc, IE_SPELL_TYPE_PRIEST, 0, 0)>0:
		stats.append (10344)
		for level in range (7):
			GemRB.SetToken ("SPELLLEVEL", str (level+1) )
			#get the bonus spell count
			base = GemRB.GetMemorizableSpellsCount (pc, IE_SPELL_TYPE_PRIEST, level, 0)
			if base:
				count = GemRB.GetMemorizableSpellsCount (pc, IE_SPELL_TYPE_PRIEST, level)
				stats.append ( (GemRB.GetString (10345), count-base, 'b') )
		stats.append (None)

	# 32204 Resistances
	stats.append (15544)
	#   67216 Slashing Attacks
	stats.append ((11768, GS (IE_RESISTSLASHING), '%'))
	#   67217 Piercing Attacks
	stats.append ((11769, GS (IE_RESISTPIERCING), '%'))
	#   67218 Crushing Attacks
	stats.append ((11770, GS (IE_RESISTCRUSHING), '%'))
	#   67219 Missile Attacks
	stats.append ((11767, GS (IE_RESISTMISSILE), '%'))
	# Poison
	stats.append ((14017, GS (IE_RESISTPOISON), '%'))
	stats.append (None)

	res = []
	lines = 0
	for s in stats:
		try:
			strref, val, type = s
			if val == 0 and type != '0':
				continue
			if type == '+': #pluses
				res.append ("[capital=0]"+GemRB.GetString (strref) + ' '+ '+' * val)
			elif type == 'x': #x character before value
				res.append ("[capital=0]"+GemRB.GetString (strref) +': x' + str (val) )
			elif type == 'a': #value (portrait icon) + string
				res.append ("[capital=2]"+val+" "+GemRB.GetString (strref))
			elif type == 'b': #strref is an already resolved string
				res.append ("[capital=0]"+strref+": "+str(val) )
			elif type == 'c': #normal string
				res.append ("[capital=0]"+GemRB.GetString (strref))
			elif type == 'd': #strref is an already resolved string
				res.append ("[capital=0]"+strref)
			elif type == '0': #normal value
				res.append (GemRB.GetString (strref) + ': ' + str (val) )
			else: #normal value + type character, for example percent sign
				res.append ("[capital=0]"+GemRB.GetString (strref) + ': ' + str (val) + type)
			lines = 1
		except:
			if s != None:
				res.append (GemRB.GetString (s) )
				lines = 0
			else:
				if lines:
					res.append ("")
				lines = 0

	return "\n".join (res)

def GetReputation (repvalue):
	table = GemRB.LoadTableObject ("reptxt")
	if repvalue>20:
		repvalue=20
	txt = GemRB.GetString (table.GetValue (repvalue, 0) )
	return txt+"("+str(repvalue)+")"

def OpenInformationWindow ():
	global InformationWindow

	if InformationWindow != None:
		if BiographyWindow: OpenBiographyWindow ()

		if InformationWindow:
			InformationWindow.Unload ()
		InformationWindow = None
		GemRB.SetVar ("FloatWindow", -1)
		return

	InformationWindow = Window = GemRB.LoadWindowObject (4)
	GemRB.SetVar ("FloatWindow", InformationWindow.ID)


	#Biography
	Button = Window.GetControl (26)
	Button.SetText (18003)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "OpenBiographyWindow")

	#Done
	Button = Window.GetControl (24)
	Button.SetText (11973)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "OpenInformationWindow")

	TotalPartyExp = 0
	ChapterPartyExp = 0
	TotalCount = 0
	ChapterCount = 0
	for i in range (1, GemRB.GetPartySize() + 1):
		stat = GemRB.GetPCStats(i)
		TotalPartyExp = TotalPartyExp + stat['KillsChapterXP']
		ChapterPartyExp = ChapterPartyExp + stat['KillsTotalXP']
		TotalCount = TotalCount + stat['KillsChapterCount']
		ChapterCount = ChapterCount + stat['KillsTotalCount']

	# These are used to get the stats
	pc = GemRB.GameGetSelectedPCSingle ()
	stat = GemRB.GetPCStats (pc)

	Label = Window.GetControl (0x10000000)
	Label.SetText (GemRB.GetPlayerName (pc, 1))
	# class
	ClassTitle = GetActorClassTitle(pc)
	Label = Window.GetControl (0x10000018)
	Label.SetText (ClassTitle)

	#most powerful vanquished
	Label = Window.GetControl (0x10000005)
	#we need getstring, so -1 will translate to empty string
	Label.SetText (GemRB.GetString (stat['BestKilledName']))

	# NOTE: currentTime is in seconds, joinTime is in seconds * 15
	#   (script updates???). In each case, there are 60 seconds
	#   in a minute, 24 hours in a day, but ONLY 5 minutes in an hour!!
	# Hence currentTime (and joinTime after div by 15) has
	#   7200 secs a day (60 * 5 * 24)
	currentTime = GemRB.GetGameTime()
	joinTime = stat['JoinDate'] - stat['AwayTime']

	party_time = currentTime - (joinTime / 15)
	days = party_time / 7200
	hours = (party_time % 7200) / 300

	GemRB.SetToken ('GAMEDAYS', str (days))
	GemRB.SetToken ('HOUR', str (hours))
	Label = Window.GetControl (0x10000006)
	#actually it is 16043 <DURATION>, but duration is translated to
	#16041, hopefully this won't cause problem with international version
	Label.SetText (16041)

	#favourite spell
	Label = Window.GetControl (0x10000007)
	Label.SetText (stat['FavouriteSpell'])

	#favourite weapon
	Label = Window.GetControl (0x10000008)
	#actually it is 10479 <WEAPONNAME>, but weaponname is translated to
	#the real weapon name (which we should set using SetToken)
	#there are other strings like bow+wname/xbow+wname/sling+wname
	#are they used?
	Label.SetText (stat['FavouriteWeapon'])

	#total xp
	Label = Window.GetControl (0x1000000f)
	if TotalPartyExp != 0:
		PartyExp = int ((stat['KillsTotalXP'] * 100) / TotalPartyExp)
		Label.SetText (str (PartyExp) + '%')
	else:
		Label.SetText ("0%")

	Label = Window.GetControl (0x10000013)
	if ChapterPartyExp != 0:
		PartyExp = int ((stat['KillsChapterXP'] * 100) / ChapterPartyExp)
		Label.SetText (str (PartyExp) + '%')
	else:
		Label.SetText ("0%")

	#total xp
	Label = Window.GetControl (0x10000010)
	if TotalCount != 0:
		PartyExp = int ((stat['KillsTotalCount'] * 100) / TotalCount)
		Label.SetText (str (PartyExp) + '%')
	else:
		Label.SetText ("0%")

	Label = Window.GetControl (0x10000014)
	if ChapterCount != 0:
		PartyExp = int ((stat['KillsChapterCount'] * 100) / ChapterCount)
		Label.SetText (str (PartyExp) + '%')
	else:
		Label.SetText ("0%")

	Label = Window.GetControl (0x10000011)
	Label.SetText (str (stat['KillsChapterXP']))
	Label = Window.GetControl (0x10000015)
	Label.SetText (str (stat['KillsTotalXP']))

	#count of kills in chapter/game
	Label = Window.GetControl (0x10000012)
	Label.SetText (str (stat['KillsChapterCount']))
	Label = Window.GetControl (0x10000016)
	Label.SetText (str (stat['KillsTotalCount']))

	Window.ShowModal (MODAL_SHADOW_GRAY)
	return

def OpenBiographyWindow ():
	global BiographyWindow

	if BiographyWindow != None:
		if BiographyWindow:
			BiographyWindow.Unload ()
		BiographyWindow = None
		GemRB.SetVar ("FloatWindow", InformationWindow.ID)

		InformationWindow.ShowModal (MODAL_SHADOW_GRAY)
		return

	BiographyWindow = Window = GemRB.LoadWindowObject (12)
	GemRB.SetVar ("FloatWindow", BiographyWindow.ID)

	TextArea = Window.GetControl (0)
	pc = GemRB.GameGetSelectedPCSingle ()
	TextArea.SetText (GemRB.GetPlayerString(pc, 63) )

	# Done
	Button = Window.GetControl (2)
	Button.SetText (11973)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "OpenBiographyWindow")

	Window.ShowModal (MODAL_SHADOW_GRAY)
	return

def OpenExportWindow ():
	global ExportWindow, NameField, ExportDoneButton

	ExportWindow = GemRB.LoadWindowObject(13)

	TextArea = ExportWindow.GetControl(2)
	TextArea.SetText(10962)

	TextArea = ExportWindow.GetControl(0)
	TextArea.GetCharacters ()

	ExportDoneButton = ExportWindow.GetControl(4)
	ExportDoneButton.SetText(11973)
	ExportDoneButton.SetState(IE_GUI_BUTTON_DISABLED)

	CancelButton = ExportWindow.GetControl(5)
	CancelButton.SetText(13727)
	CancelButton.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)

	NameField = ExportWindow.GetControl(6)

	ExportDoneButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, "ExportDonePress")
	CancelButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, "ExportCancelPress")
	NameField.SetEvent(IE_GUI_EDIT_ON_CHANGE, "ExportEditChanged")
	ExportWindow.ShowModal (MODAL_SHADOW_GRAY)
	NameField.SetStatus (IE_GUI_CONTROL_FOCUSED)
	return

def ExportDonePress():
	if ExportWindow:
		ExportWindow.Unload()
	#save file under name from EditControl
	return

def ExportCancelPress():
	if ExportWindow:
		ExportWindow.Unload()
	return

def ExportEditChanged():
	ExportFileName = NameField.QueryText()
	if ExportFileName == "":
		ExportDoneButton.SetState(IE_GUI_BUTTON_DISABLED)
	else:
		ExportDoneButton.SetState(IE_GUI_BUTTON_ENABLED)
	return

def OpenCustomizeWindow ():
	global CustomizeWindow

	CustomizeWindow = GemRB.LoadWindowObject (17)

	PortraitSelectButton = CustomizeWindow.GetControl (0)
	PortraitSelectButton.SetText (11961)

	SoundButton = CustomizeWindow.GetControl (1)
	SoundButton.SetText (10647)

	ColorButton = CustomizeWindow.GetControl (2)
	ColorButton.SetText (10646)

	ScriptButton = CustomizeWindow.GetControl (3)
	ScriptButton.SetText (17111)

	BiographyButton = CustomizeWindow.GetControl (9)
	BiographyButton.SetText (18003)

	TextArea = CustomizeWindow.GetControl (5)
	TextArea.SetText (11327)

	CustomizeDoneButton = CustomizeWindow.GetControl (7)
	CustomizeDoneButton.SetText (11973)
	CustomizeDoneButton.SetState (IE_GUI_BUTTON_ENABLED)

	CancelButton = CustomizeWindow.GetControl (8);
	CancelButton.SetText (13727)
	CancelButton.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)

	PortraitSelectButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, "OpenPortraitSelectWindow")
	SoundButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, "OpenSoundWindow")
	ColorButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, "OpenColorWindow")
	ScriptButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, "OpenScriptWindow")
	BiographyButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, "OpenBiographyEditWindow")
	CustomizeDoneButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, "CustomizeDonePress")
	CancelButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, "CustomizeCancelPress")

	CustomizeWindow.ShowModal (MODAL_SHADOW_GRAY)
	return

def CustomizeDonePress ():
	CloseCustomizeWindow ()
	return

def CustomizeCancelPress ():
	CloseCustomizeWindow ()
	return

def CloseCustomizeWindow ():
	global CustomizeWindow

	if CustomizeWindow:
		CustomizeWindow.Unload ()
		CustomizeWindow = None
	return


def OpenPortraitSelectWindow ():
	global CharGenWindow, PortraitWindow, PortraitPictureButton 

	PortraitWindow = GemRB.LoadWindowObject (18)

	PortraitPictureButton = PortraitWindow.GetControl (0)
	PortraitPictureButton.SetFlags (IE_GUI_BUTTON_PICTURE|IE_GUI_BUTTON_NO_IMAGE, OP_SET)

	PortraitLeftButton = PortraitWindow.GetControl (1)
	PortraitLeftButton.SetState (IE_GUI_BUTTON_ENABLED)
	PortraitLeftButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, "PortraitLeftPress")
	PortraitLeftButton.SetFlags (IE_GUI_BUTTON_RADIOBUTTON, OP_OR)

	PortraitRightButton = PortraitWindow.GetControl (2)
	PortraitRightButton.SetState (IE_GUI_BUTTON_ENABLED)
	PortraitRightButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, "PortraitRightPress")
	PortraitRightButton.SetFlags (IE_GUI_BUTTON_RADIOBUTTON, OP_OR)

	PortraitCustomButton = PortraitWindow.GetControl (5)
	PortraitCustomButton.SetState (IE_GUI_BUTTON_ENABLED)
	PortraitCustomButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, "OpenCustomPortraitWindow")
	PortraitCustomButton.SetText (17545)

	PortraitDoneButton = PortraitWindow.GetControl (3)
	PortraitDoneButton.SetState (IE_GUI_BUTTON_ENABLED)
	PortraitDoneButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, "PortraitDonePress")
	PortraitDoneButton.SetText (11973)
	PortraitDoneButton.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)

	PortraitCancelButton = PortraitWindow.GetControl (4)
	PortraitCancelButton.SetState (IE_GUI_BUTTON_ENABLED)
	PortraitCancelButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, "PortraitCancelPress")
	PortraitCancelButton.SetText (13727)
	PortraitCancelButton.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)

	# get players gender and portrait
	Pc = GemRB.GameGetSelectedPCSingle ()
	PcGender = GemRB.GetPlayerStat (Pc, IE_SEX)
	PcPortrait = GemRB.GetPlayerPortrait(Pc,0)	

	# initialize and set portrait
	Portrait.Init (PcGender)
	Portrait.Set (PcPortrait)
	PortraitPictureButton.SetPicture (Portrait.Name () + "G")

	PortraitWindow.ShowModal (MODAL_SHADOW_GRAY)
	return

def PortraitLeftPress ():
	global PortraitPictureButton

	PortraitPictureButton.SetPicture (Portrait.Previous () + "G")

def PortraitRightPress ():
	global PortraitPictureButton

	PortraitPictureButton.SetPicture (Portrait.Next () + "G" )

def ClosePortraitSelectWindow ():
	global PortraitWindow

	if PortraitWindow:
		PortraitWindow.Unload ()
		PortraitWindow = None

	#UpdateRecordsWindow ()
	CustomizeWindow.ShowModal (MODAL_SHADOW_GRAY) 
	return 

def PortraitCancelPress ():
	ClosePortraitSelectWindow ()
	return 

def PortraitDonePress ():

	Pc = GemRB.GameGetSelectedPCSingle ()

	GemRB.FillPlayerInfo (Pc, Portrait.Name () + "L", Portrait.Name () + "S")
	ClosePortraitWindow ()
	return

def ClosePortraitWindow ():
	global PortraitWindow

	if PortraitWindow:
		PortraitWindow.Unload ()
		PortraitWindow = None
	
	#UpdateRecordsWindow ()
	# causes error, but dunno how to solve it properly
	CustomizeWindow.ShowModal (MODAL_SHADOW_GRAY) 
	return 

def OpenCustomPortraitWindow ():
	global CustomPortraitWindow
	global PortraitList1, PortraitList2
	global RowCount1, RowCount2

	CustomPortraitWindow = GemRB.LoadWindowObject (19)

	CustomPortraitDoneButton = CustomPortraitWindow.GetControl (10)
	CustomPortraitDoneButton.SetState (IE_GUI_BUTTON_DISABLED)
	CustomPortraitDoneButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, "CustomPortraitDonePress")
	CustomPortraitDoneButton.SetText (11973)
	CustomPortraitDoneButton.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)

	CustomPortraitCancelButton = CustomPortraitWindow.GetControl (11)
	CustomPortraitCancelButton.SetState (IE_GUI_BUTTON_ENABLED)
	CustomPortraitCancelButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, "CloseCustomPortraitWindow")
	CustomPortraitCancelButton.SetText (13727)
	CustomPortraitCancelButton.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)

	# Portrait List Large
	PortraitList1 = CustomPortraitWindow.GetControl (2)
	RowCount1 = PortraitList1.GetPortraits (0)
	PortraitList1.SetEvent (IE_GUI_TEXTAREA_ON_CHANGE, "LargeCustomPortrait")
	GemRB.SetVar ("Row1", RowCount1)
	PortraitList1.SetVarAssoc ("Row1",RowCount1)

	# Portrait List Small
	PortraitList2 = CustomPortraitWindow.GetControl (3)
	RowCount2 = PortraitList2.GetPortraits (1)
	PortraitList2.SetEvent (IE_GUI_TEXTAREA_ON_CHANGE, "SmallCustomPortrait")
	GemRB.SetVar ("Row2", RowCount2)
	PortraitList2.SetVarAssoc ("Row2",RowCount2)

	CustomPortraitWindow.ShowModal (MODAL_SHADOW_GRAY)

	return

def CustomPortraitDonePress ():
	global PortraitList1, PortraitList2
	Pc = GemRB.GameGetSelectedPCSingle ()

	GemRB.FillPlayerInfo (Pc, PortraitList1.QueryText () , PortraitList2.QueryText ())

	CloseCustomPortraitWindow ()
	ClosePortraitWindow ()
	return

def CloseCustomPortraitWindow ():
	global CustomPortraitWindow

	if CustomPortraitWindow:
		CustomPortraitWindow.ShowModal (MODAL_SHADOW_BLACK)
		CustomPortraitWindow.Unload ()
		CustomPortraitWindow = None
	
	return

def LargeCustomPortrait():
	global PortraitList1

	Window = CustomPortraitWindow

	Portrait = PortraitList1.QueryText ()
	#small hack
	if GemRB.GetVar ("Row1") == RowCount1:
		return

	Label = Window.GetControl (0x10000007)
	Label.SetText (Portrait)

	Button = Window.GetControl (10)
	if Portrait=="":
		Portrait = "NOPORTMD"
		Button.SetState (IE_GUI_BUTTON_DISABLED)
	else:
		if PortraitList2.QueryText ()!="":
			Button.SetState (IE_GUI_BUTTON_ENABLED)

	Button = Window.GetControl (0)
	Button.SetPicture (Portrait, "NOPORTMD")
	return

def SmallCustomPortrait():
	global PortraitList2

	Window = CustomPortraitWindow

	Portrait = PortraitList2.QueryText ()
	#small hack
	if GemRB.GetVar ("Row2") == RowCount2:
		return

	Label = Window.GetControl (0x10000008)
	Label.SetText (Portrait)

	Button = Window.GetControl (10)
	if Portrait=="":
		Portrait = "NOPORTSM"
		Button.SetState (IE_GUI_BUTTON_DISABLED)
	else:
		if PortraitList1.QueryText ()!="":
			Button.SetState (IE_GUI_BUTTON_ENABLED)

	Button = Window.GetControl (1)
	Button.SetPicture (Portrait, "NOPORTSM")
	return


###################################################
# End of file GUIREC.py
