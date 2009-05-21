# -*-python-*-
# GemRB - Infinity Engine Emulator
# Copyright (C) 2003-2009 The GemRB Project
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
# $Id$


# GUIREC.py - scripts to control stats/records windows from the GUIREC winpack

###################################################
import string
import GemRB
import GUICommonWindows
from GUIDefines import *
from ie_stats import *
from ie_restype import *
from GUICommon import CloseOtherWindow
from GUICommonWindows import *
from GUIWORLD import OpenReformPartyWindow

###################################################
RecordsWindow = None
InformationWindow = None
BiographyWindow = None
PortraitWindow = None
OptionsWindow = None
CustomizeWindow = None
SubCustomizeWindow = None
SubSubCustomizeWindow = None
OldPortraitWindow = None
OldOptionsWindow = None
ExportWindow = None
KitInfoWindow = None
ExportDoneButton = None
ExportFileName = ""
PortraitsTable = None
ScriptsTable = None
ScriptTextArea = None

# the available sounds
SoundSequence = [ 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', \
				'm', 's', 't', 'u', 'v', '_', 'x', 'y', 'z', '0', '1', '2', \
				'3', '4', '5', '6', '7', '8', '9']
SoundIndex = 0

###################################################
def OpenRecordsWindow ():
	global RecordsWindow, OptionsWindow, PortraitWindow
	global OldPortraitWindow, OldOptionsWindow

	if CloseOtherWindow (OpenRecordsWindow):
		if InformationWindow: OpenInformationWindow ()

		if RecordsWindow:
			RecordsWindow.Unload ()
		if OptionsWindow:
			OptionsWindow.Unload ()
		if PortraitWindow:
			PortraitWindow.Unload ()

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
	MarkMenuButton (OptionsWindow)
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
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "OpenRecReformPartyWindow")

	# customize
	Button = Window.GetControl (50)
	Button.SetText (10645)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "OpenCustomizeWindow")

	# export
	Button = Window.GetControl (36)
	Button.SetText (13956)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "OpenExportWindow")

	# kit info
	Button = Window.GetControl (52)
	Button.SetText (61265)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "KitInfoWindow")

	SetSelectionChangeHandler (UpdateRecordsWindow)
	UpdateRecordsWindow ()
	OptionsWindow.SetVisible (1)
	Window.SetVisible (3)
	PortraitWindow.SetVisible (1)
	return

#original returns to game before continuing...
def OpenRecReformPartyWindow ():
	OpenRecordsWindow ()
	GemRB.SetTimedEvent ("OpenReformPartyWindow", 1)
	return

def GetNextLevelExp (Level, Class):
	NextLevelTable = GemRB.LoadTableObject ("XPLEVEL")
	Row = NextLevelTable.GetRowIndex (Class)
	if Level < NextLevelTable.GetColumnCount (Row):
		return str (NextLevelTable.GetValue (Row, Level) )

	return 0

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
	ac = GemRB.GetPlayerStat (pc, IE_ARMORCLASS)
	#This is a temporary solution, the core engine should set the stat correctly!
	ac += GemRB.GetAbilityBonus (IE_DEX, 2, GemRB.GetPlayerStat (pc, IE_DEX) )
	Label.SetText (str (ac))
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
	cstr = GetStatColor (pc, IE_STR)
	sstrx = GemRB.GetPlayerStat (pc, IE_STREXTRA)

	if sstrx > 0 and sstr==18:
		sstr = "%d/%02d" %(sstr, sstrx % 100)
	else:
		sstr = str (sstr)
	sint = str (GemRB.GetPlayerStat (pc, IE_INT))
	cint = GetStatColor (pc, IE_INT)
	swis = str (GemRB.GetPlayerStat (pc, IE_WIS))
	cwis = GetStatColor (pc, IE_WIS)
	sdex = str (GemRB.GetPlayerStat (pc, IE_DEX))
	cdex = GetStatColor (pc, IE_DEX)
	scon = str (GemRB.GetPlayerStat (pc, IE_CON))
	ccon = GetStatColor (pc, IE_CON)
	schr = str (GemRB.GetPlayerStat (pc, IE_CHR))
	cchr = GetStatColor (pc, IE_CHR)

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
	Table = GemRB.LoadTableObject ("races")
	text = Table.GetValue (Table.FindValue (3, GemRB.GetPlayerStat (pc, IE_RACE)) , 0)

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
	a = GemRB.GetPlayerStat (pc, stat)
	b = GemRB.GetPlayerStat (pc, stat, 1)
	if a==b:
		return (255,255,255)
	if a<b:
		return (255,255,0)
	return (0,255,0)

# LevelDiff is used only from the level up code and holds the level
# difference for each class
def GetStatOverview (pc, LevelDiff=[0,0,0]):
	print "Function GUIREC.GetStatOverview:"
	StateTable = GemRB.LoadTableObject ("statdesc")
	str_None = GemRB.GetString (61560)

	GS = lambda s, pc=pc: GemRB.GetPlayerStat (pc, s)
	GA = lambda s, col, pc=pc: GemRB.GetAbilityBonus (s, col, GS (s) )

	stats = []
	# class levels
	# 16480 <CLASS>: Level <LEVEL>
	# Experience: <EXPERIENCE>
	# Next Level: <NEXTLEVEL>

	# collecting tokens for stat overview
	ClassTitle = GetActorClassTitle (pc)
	KitTable = GemRB.LoadTableObject ("kitlist")
	GemRB.SetToken ("CLASS", ClassTitle)
	Class = GemRB.GetPlayerStat (pc, IE_CLASS)
	ClassTable = GemRB.LoadTableObject ("classes")
	Class = ClassTable.FindValue (5, Class)
	Class = ClassTable.GetRowName (Class)
	Dual = IsDualClassed (pc, 1)
	Multi = IsMultiClassed (pc, 1)
	XP = GemRB.GetPlayerStat (pc, IE_XP)

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
			GemRB.SetToken ("LEVEL", str (Levels[i]+LevelDiff[i]) )
			GemRB.SetToken ("NEXTLEVEL", GetNextLevelExp (Levels[i]+LevelDiff[i], Class) )
			GemRB.SetToken ("EXPERIENCE", str (XP/Multi[0]) )
			#resolve string immediately
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
		GemRB.SetToken ("LEVEL", str (Levels[0]))
		Class = ClassTable.GetRowName (Dual[2])
		GemRB.SetToken ("NEXTLEVEL", GetNextLevelExp (Levels[0], Class) )

		XP2 = GemRB.GetPlayerStat (pc, IE_XP)
		GemRB.SetToken ("EXPERIENCE", str (XP2) )

		stats.append ( (GemRB.GetString (16480),"",'d') )
		stats.append (None)

		# the first class (shown second)
		if Dual[0] == 1:
			ClassTitle = GemRB.GetString (KitTable.GetValue (Dual[1], 2))
		elif Dual[0] == 2:
			ClassTitle = GemRB.GetString (ClassTable.GetValue (Dual[1], 2))
		GemRB.SetToken ("CLASS", ClassTitle)
		GemRB.SetToken ("LEVEL", str (Levels[1]) )

		# the xp table contains only classes
		XPTable = GemRB.LoadTableObject ("xplevel")
		if Dual[0] == 2:
			BaseClass = ClassTable.GetRowName (Dual[1])
		else:
			BaseClass = GetKitIndex (pc)
			BaseClass = KitTable.GetValue (BaseClass, 7)
			BaseClass = ClassTable.FindValue (5, BaseClass)
			BaseClass = ClassTable.GetRowName (BaseClass)
		# the first class' XP is discarded and set to the minimum level
		# requirement, so if you don't dual class right after a levelup,
		# the game would eat some of your XP
		XP1 = XPTable.GetValue (BaseClass, str (Levels[1]))
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
		GemRB.SetToken ("LEVEL", str (Level))
		GemRB.SetToken ("NEXTLEVEL", GetNextLevelExp (Level, Class) )
		GemRB.SetToken ("EXPERIENCE", str (XP) )
		stats.append ( (16480,1,'c') )
		stats.append (None)
		print "\t\tClass (Level):",Class,"(",Level,")"

	# check to see if we have a level diff anywhere
	if sum (LevelDiff) > 0:
		effects = GemRB.GetPlayerStates (pc)
		if len (effects):
			for c in effects:
				tmp = StateTable.GetValue (ord(c)-66, 0)
				stats.append ( (tmp,c,'a') )
			stats.append (None)

	stats.append ( (8442,1,'c') )

	stats.append ( (61932, GS (IE_THAC0), '') )
	stats.append ( (9457, GS (IE_THAC0), '') )
	tmp = GS (IE_NUMBEROFATTACKS)
	if (tmp&1):
		tmp2 = str(tmp/2) + chr(188)
	else:
		tmp2 = str(tmp/2)
	stats.append ( (9458, tmp2, '') )
	stats.append ( (9459, GS (IE_LORE), '') )
	reptxt = GetReputation (GemRB.GameGetReputation ()/10)
	stats.append ( (9465, reptxt, '') )
	stats.append ( (9460, GS (IE_LOCKPICKING), '') )
	stats.append ( (9462, GS (IE_TRAPS), '') )
	stats.append ( (9463, GS (IE_PICKPOCKET), '') )
	stats.append ( (9461, GS (IE_STEALTH), '') )
	HatedRace = GS (IE_HATEDRACE)
	if HatedRace:
		HateTable = GemRB.LoadTableObject ("haterace")
		Racist = HateTable.FindValue (1, HatedRace)
		if Racist != -1:
			HatedRace = HateTable.GetValue (Racist, 0)
			stats.append ( (15982, GemRB.GetString (HatedRace), '') )

	stats.append ( (34120, GS (IE_HIDEINSHADOWS), '') )
	stats.append ( (34121, GS (IE_DETECTILLUSIONS), '') )
	stats.append ( (34122, GS (IE_SETTRAPS), '') )
	stats.append ( (12128, GS (IE_BACKSTABDAMAGEMULTIPLIER), 'x') )
	stats.append ( (12126, GS (IE_TURNUNDEADLEVEL), '') )
	stats.append ( (12127, GS (IE_LAYONHANDSAMOUNT), '') )
	#script
	aiscript = GemRB.GetPlayerScript (pc )
	stats.append ( (2078, aiscript, '') )
	stats.append (None)


	# 17379 Saving Throws
	stats.append (17379)
	# 17380 Death
	stats.append ( (17380, GS (IE_SAVEVSDEATH), '') )
	# 17381
	stats.append ( (17381, GS (IE_SAVEVSWANDS), '') )
	# 17382 AC vs. Crushing
	stats.append ( (17382, GS (IE_SAVEVSPOLY), '') )
	# 17383 Rod
	stats.append ( (17383, GS (IE_SAVEVSBREATH), '') )
	# 17384 Spells
	stats.append ( (17384, GS (IE_SAVEVSSPELL), '') )
	stats.append (None)

	# 9466 Weapon profs
	stats.append (9466)
	table = GemRB.LoadTableObject ("weapprof")
	RowCount = table.GetRowCount ()
	# the first 7 profs are foobared
	for i in range(8,RowCount):
		text = table.GetValue (i, 1)
		stat = table.GetValue (i, 0)
		stats.append ( (text, GS(stat), '+') )
	stats.append (None)

	# 11766 AC Bonuses
	stats.append (11766)
	# 11770 AC vs. Crushing
	stats.append ((11770, GS (IE_ACCRUSHINGMOD), ''))
	# 11767 AC vs. Missile
	stats.append ((11767, GS (IE_ACMISSILEMOD), ''))
	# 11769 AC vs. Piercing
	stats.append ((11769, GS (IE_ACPIERCINGMOD), ''))
	# 11768 AC vs. Slashing
	stats.append ((11768, GS (IE_ACSLASHINGMOD), ''))
	stats.append (None)

	# 10315 Ability bonuses
	stats.append (10315)
	value = GemRB.GetPlayerStat (pc, IE_STR)
	ex = GemRB.GetPlayerStat (pc, IE_STREXTRA)
	# 10332 to hit
	stats.append ( (10332, GemRB.GetAbilityBonus (IE_STR,0,value,ex), '0') )
	# 10336 damage
	stats.append ( (10336, GemRB.GetAbilityBonus (IE_STR,1,value,ex), '0') )
	# 10337 open doors (bend bars lift gates)
	stats.append ( (10337, GemRB.GetAbilityBonus (IE_STR,2,value,ex), '0') )
	# 10338 weight allowance
	stats.append ( (10338, GemRB.GetAbilityBonus (IE_STR,3,value,ex), '0') )
	# 10339 AC
	stats.append ( (10339, GA(IE_DEX,2), '0') )
	# 10340 Missile
	stats.append ( (10340, GA(IE_DEX,1), '0') )
	# 10341 Reaction
	stats.append ( (10341, GA(IE_DEX,0), '0') )
	# 10342 Hp/Level
	stats.append ( (10342, GA(IE_CON,0), '0') )
	# 10343 Chance To Learn spell
	if GemRB.GetMemorizableSpellsCount (pc, IE_SPELL_TYPE_WIZARD, 0, 0)>0:
		stats.append ((10343, GA (IE_INT,0), '%' ))
	# 10347 Reaction
	stats.append ( (10347, GA(IE_CHR,0), '0') )
	stats.append (None)

	#Bonus spells
	if GemRB.GetMemorizableSpellsCount (pc, IE_SPELL_TYPE_PRIEST, 0, 0)>0:
		stats.append (10344)
		for level in range(7):
			GemRB.SetToken ("SPELLLEVEL", str(level+1) )
			#get the bonus spell count
			base = GemRB.GetMemorizableSpellsCount (pc, IE_SPELL_TYPE_PRIEST, level, 0)
			if base:
				count = GemRB.GetMemorizableSpellsCount (pc, IE_SPELL_TYPE_PRIEST, level)
				stats.append ( (GemRB.GetString(10345), count-base, 'b') )
		stats.append (None)

	# 32204 Resistances
	stats.append (32204)
	# 32213 Normal Fire
	stats.append ((32213, GS (IE_RESISTFIRE), '%'))
	# 32222 Magic Fire
	stats.append ((32222, GS (IE_RESISTMAGICFIRE), '%'))
	# 32214 Normal Cold
	stats.append ((32214, GS (IE_RESISTCOLD), '%'))
	# 32223 Magic Cold
	stats.append ((32223, GS (IE_RESISTMAGICCOLD), '%'))
	# 32220 Electricity
	stats.append ((32220, GS (IE_RESISTELECTRICITY), '%'))
	# 32221 Acid
	stats.append ((32221, GS (IE_RESISTACID), '%'))
	# 32233 Magic Damage
	stats.append ((32233, GS (IE_RESISTMAGIC), '%'))
	# 67216 Slashing Attacks
	stats.append ((67216, GS (IE_RESISTSLASHING), '%'))
	# 67217 Piercing Attacks
	stats.append ((67217, GS (IE_RESISTPIERCING), '%'))
	# 67218 Crushing Attacks
	stats.append ((67218, GS (IE_RESISTCRUSHING), '%'))
	# 67219 Missile Attacks
	stats.append ((67219, GS (IE_RESISTMISSILE), '%'))
	stats.append (None)

	#Weapon Style bonuses
	stats.append (32131)
	#
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
				res.append ("[capital=0]"+GemRB.GetString (strref)+': x' + str (val) )
			elif type == 'a': #value (portrait icon) + string
				res.append ("[capital=2]"+val+" "+GemRB.GetString (strref))
			elif type == 'b': #strref is an already resolved string
				res.append ("[capital=0]"+strref+": "+str(val) )
			elif type == 'c': #normal string
				res.append ("[capital=0]"+GemRB.GetString (strref) )
			elif type == 'd': #strref is an already resolved string
				res.append ("[capital=0]"+strref)
			elif type == '0': #normal value
				res.append (GemRB.GetString (strref) + ': ' + str (val) )
			else: #normal value + type character, for example percent sign
				res.append (GemRB.GetString (strref) + ': ' + str (val) + type)
			lines = 1
		except:
			if s != None:
				res.append ("[capital=0]"+ GemRB.GetString (s) )
				lines = 0
			else:
				if not lines:
					res.append (str_None)
				res.append ("")
				lines = 0

	return string.join (res, "\n")


def GetReputation (repvalue):
	table = GemRB.LoadTableObject ("reptxt")
	if repvalue>20:
		repvalue=20
	txt = GemRB.GetString (table.GetValue (repvalue, 0) )
	return txt+" ("+str(repvalue)+")"


def OpenInformationWindow ():
	global InformationWindow

	if InformationWindow != None:
		if BiographyWindow:
			OpenBiographyWindow ()

		if InformationWindow:
			InformationWindow.Unload ()
		OptionsWindow.SetVisible (3)
		RecordsWindow.SetVisible (3)
		PortraitWindow.SetVisible (3)
		InformationWindow = None

		return

	InformationWindow = Window = GemRB.LoadWindowObject (4)

	# Biography
	Button = Window.GetControl (26)
	Button.SetText (18003)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "OpenBiographyWindow")

	# Done
	Button = Window.GetControl (24)
	Button.SetText (11973)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "OpenInformationWindow")

	TotalPartyExp = 0
	ChapterPartyExp = 0
	TotalCount = 0
	ChapterCount = 0
	for i in range (1, GemRB.GetPartySize () + 1):
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
	ClassTitle = GetActorClassTitle (pc)
	Label = Window.GetControl (0x10000018)
	Label.SetText (ClassTitle)

	#most powerful vanquished
	Label = Window.GetControl (0x10000005)
	#we need getstring, so -1 will translate to empty string
	Label.SetText (GemRB.GetString (stat['BestKilledName']))

	# NOTE: currentTime is in seconds, joinTime is in seconds * 15
	# (script updates???). In each case, there are 60 seconds
	# in a minute, 24 hours in a day, but ONLY 5 minutes in an hour!!
	# Hence currentTime (and joinTime after div by 15) has
	# 7200 secs a day (60 * 5 * 24)
	currentTime = GemRB.GetGameTime ()
	joinTime = stat['JoinDate'] - stat['AwayTime']

	party_time = currentTime - (joinTime / 15)
	print "CurrentTime",currentTime
	days = party_time / 7200
	hours = (party_time % 7200) / 300

	GemRB.SetToken ('GAMEDAY', str (days))
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
		InformationWindow.ShowModal (MODAL_SHADOW_GRAY)
		return

	BiographyWindow = Window = GemRB.LoadWindowObject (12)

	TextArea = Window.GetControl (0)
	pc = GemRB.GameGetSelectedPCSingle ()
	TextArea.SetText (GemRB.GetPlayerString (pc, 74) )

	# Done
	Button = Window.GetControl (2)
	Button.SetText (11973)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "OpenBiographyWindow")

	Window.ShowModal (MODAL_SHADOW_GRAY)
	return

def OpenExportWindow ():
	global ExportWindow, NameField, ExportDoneButton

	ExportWindow = GemRB.LoadWindowObject (13)

	TextArea = ExportWindow.GetControl (2)
	TextArea.SetText (10962)

	TextArea = ExportWindow.GetControl (0)
	TextArea.GetCharacters ()

	ExportDoneButton = ExportWindow.GetControl (4)
	ExportDoneButton.SetText (11973)
	ExportDoneButton.SetState (IE_GUI_BUTTON_DISABLED)

	CancelButton = ExportWindow.GetControl (5)
	CancelButton.SetText (13727)
	CancelButton.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)

	NameField = ExportWindow.GetControl (6)

	ExportDoneButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, "ExportDonePress")
	CancelButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, "ExportCancelPress")
	NameField.SetEvent (IE_GUI_EDIT_ON_CHANGE, "ExportEditChanged")
	ExportWindow.ShowModal (MODAL_SHADOW_GRAY)
	NameField.SetStatus (IE_GUI_CONTROL_FOCUSED)
	return

def ExportDonePress():
	if ExportWindow:
		ExportWindow.Unload()
	#save file under name from EditControl
	pc = GemRB.GameGetSelectedPCSingle ()
	GemRB.SaveCharacter (pc, ExportFileName)
	return

def ExportCancelPress():
	if ExportWindow:
		ExportWindow.Unload()
	return

def ExportEditChanged():
	global ExportFileName

	ExportFileName = NameField.QueryText()
	if ExportFileName == "":
		ExportDoneButton.SetState(IE_GUI_BUTTON_DISABLED)
	else:
		ExportDoneButton.SetState(IE_GUI_BUTTON_ENABLED)
	return

# returns boolean - true if can level
def CanLevelUp(actor):
	# get our class and placements for Multi'd and Dual'd characters
	Class = GemRB.GetPlayerStat (actor, IE_CLASS)
	ClassTable = GemRB.LoadTableObject ("classes")
	Class = ClassTable.FindValue (5, Class)
	Class = ClassTable.GetRowName (Class)
	Multi = IsMultiClassed (actor, 1)
	Dual = IsDualClassed (actor, 1)

	# get all the levels and overall xp here
	Levels = [GemRB.GetPlayerStat (actor, IE_LEVEL), GemRB.GetPlayerStat (actor, IE_LEVEL2), GemRB.GetPlayerStat (actor, IE_LEVEL3)]
	xp = GemRB.GetPlayerStat (actor, IE_XP)

	if Multi[0] > 1: # multiclassed
		xp = xp/Multi[0] # divide the xp evenly between the classes
		for i in range (Multi[0]):
			# if any class can level, return 1
			ClassIndex = ClassTable.FindValue (5, Multi[i+1])
			if int(GetNextLevelExp (Levels[i], ClassTable.GetRowName (ClassIndex))) <= xp:
				return 1

		# didn't find a class that could level
		return 0
	elif Dual[0] > 0: # dual classed
		if IsDualSwap (actor): # swap the levels if we have to
			Levels = [Levels[1], Levels[0], Levels[2]]
		# get the class we can level
		Class = ClassTable.GetRowName (Dual[2])

	# check the class that can be level (single or dual)
	return int(GetNextLevelExp (Levels[0], Class)) <= xp

def CanDualClass(actor):
	# human
	if GemRB.GetPlayerStat (actor, IE_RACE) != 1:
		return 1

	# already dualclassed
	Dual = IsDualClassed (actor,0)
	if Dual[0] > 0:
		return 1

	DualClassTable = GemRB.LoadTableObject ("dualclas")
	Class = GemRB.GetPlayerStat (actor, IE_CLASS)
	ClassTable = GemRB.LoadTableObject ("classes")
	ClassIndex = ClassTable.FindValue (5, Class)
	ClassName = ClassTable.GetRowName (ClassIndex)
	KitIndex = GetKitIndex (actor)
	if KitIndex == 0:
		ClassTitle = ClassName
	else:
		KitTable = GemRB.LoadTableObject ("kitlist")
		ClassTitle = KitTable.GetValue (KitIndex, 0)
	Row = DualClassTable.GetRowIndex (ClassTitle)

	# a lookup table for the DualClassTable columns
	classes = [ "FIGHTER", "CLERIC", "MAGE", "THIEF", "DRUID", "RANGER" ]
	matches = []
	Sum = 0
	for col in range(0, DualClassTable.GetColumnCount ()):
		value = DualClassTable.GetValue (Row, col)
		Sum += value
		if value == 1:
			matches.append (classes[col])

	# cannot dc if all the columns of the DualClassTable are 0
	if Sum == 0:
		return 1

	# if the only choice for dc is already the same as the actors base class
	if Sum == 1 and ClassName in matches and KitIndex == 0:
		return 1

	AlignmentTable = GemRB.LoadTableObject ("alignmnt")
	AlignsTable = GemRB.LoadTableObject ("aligns")
	Alignment = GemRB.GetPlayerStat (actor, IE_ALIGNMENT)
	AlignmentColName = AlignsTable.FindValue (3, Alignment)
	AlignmentColName = AlignsTable.GetValue (AlignmentColName, 4)
	Sum = 0
	for classy in matches:
		Sum += AlignmentTable.GetValue (classy, AlignmentColName)

	# cannot dc if all the available classes forbid the chars alignment
	if Sum == 0:
		return 1

	# check current class' stat limitations
	StatTable = GemRB.LoadTableObject ("abdcscrq")
	ClassStatIndex = StatTable.GetRowIndex (ClassTitle)
	for stat in range(6):
		minimum = StatTable.GetValue (ClassStatIndex, stat)
		name = StatTable.GetColumnName (stat)
		if GemRB.GetPlayerStat (actor, eval("IE_" + name[4:])) < minimum:
			return 1

	# check new class' stat limitations - make sure there are any good class choices
	StatTable = GemRB.LoadTableObject ("abdcdsrq")
	for match in matches:
		ClassStatIndex = StatTable.GetRowIndex (match)
		for stat in range(6):
			minimum = StatTable.GetValue (ClassStatIndex, stat)
			name = StatTable.GetColumnName (stat)
			if GemRB.GetPlayerStat (actor, eval("IE_" + name[4:])) < minimum:
				matches.remove(match)
				break
	if len(matches) == 0:
		return 1

	# must be at least level 2
	if GemRB.GetPlayerStat (actor, IE_LEVEL) == 1:
		return 1
	return 0

def KitInfoWindow():
	global KitInfoWindow

	KitInfoWindow = GemRB.LoadWindowObject (24)

	#back button (setting first, to be less error prone)
	DoneButton = KitInfoWindow.GetControl (2)
	DoneButton.SetText (11973)
	DoneButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, "KitDonePress")

	#kit or class description
	TextArea = KitInfoWindow.GetControl (0)

	pc = GemRB.GameGetSelectedPCSingle ()
	Class = GemRB.GetPlayerStat (pc, IE_CLASS)
	ClassTable = GemRB.LoadTableObject ("classes")
	ClassIndex = ClassTable.FindValue (5, Class)
	Multi = ClassTable.GetValue (ClassIndex, 4)
	Dual = IsDualClassed (pc, 1)

	if Multi and Dual[0] == 0: # true multi class
		text = ClassTable.GetValue (ClassIndex, 1)
		TextArea.SetText (text)
		KitInfoWindow.ShowModal (MODAL_SHADOW_GRAY)
		return

	KitTable = GemRB.LoadTableObject ("kitlist")
	KitIndex = GetKitIndex (pc)

	if Dual[0]: # dual class
		# first (previous) kit or class of the dual class
		if Dual[0] == 1:
			text = KitTable.GetValue (Dual[1], 3)
		elif Dual[0] == 2:
			text = ClassTable.GetValue (Dual[1], 1)

		TextArea.SetText (text)
		TextArea.Append ("\n\n")
		text = ClassTable.GetValue (Dual[2], 1)

	else: # ordinary class or kit
		if KitIndex:
			text = KitTable.GetValue (KitIndex, 3)
		else:
			text = ClassTable.GetValue (ClassIndex, 1)

	TextArea.Append (text)

	KitInfoWindow.ShowModal (MODAL_SHADOW_GRAY)
	return

def KitDonePress():
	if KitInfoWindow:
		KitInfoWindow.Unload()
	return

def OpenCustomizeWindow ():
	global CustomizeWindow, PortraitsTable, ScriptsTable

	pc = GemRB.GameGetSelectedPCSingle ()
	if GemRB.GetPlayerStat (pc, IE_MC_FLAGS)&MC_EXPORTABLE:
		Exportable = 1
	else:
		Exportable = 0

	PortraitsTable = GemRB.LoadTableObject ("PICTURES")
	ScriptsTable = GemRB.LoadTableObject ("SCRPDESC")
	CustomizeWindow = GemRB.LoadWindowObject (17)

	AppearanceButton = CustomizeWindow.GetControl (0)
	AppearanceButton.SetText (11961)
	if not Exportable:
		AppearanceButton.SetState (IE_GUI_BUTTON_DISABLED)

	SoundButton = CustomizeWindow.GetControl (1)
	SoundButton.SetText (10647)
	if not Exportable:
		SoundButton.SetState (IE_GUI_BUTTON_DISABLED)

	ColorButton = CustomizeWindow.GetControl (2)
	ColorButton.SetText (10646)
	if not Exportable:
		ColorButton.SetState (IE_GUI_BUTTON_DISABLED)

	ScriptButton = CustomizeWindow.GetControl (3)
	ScriptButton.SetText (17111)

	BiographyButton = CustomizeWindow.GetControl (9)
	BiographyButton.SetText (18003)
	if not Exportable:
		BiographyButton.SetState (IE_GUI_BUTTON_DISABLED)

	TextArea = CustomizeWindow.GetControl (5)
	TextArea.SetText (11327)

	DoneButton = CustomizeWindow.GetControl (7)
	DoneButton.SetText (11973)
	DoneButton.SetState (IE_GUI_BUTTON_ENABLED)

	CancelButton = CustomizeWindow.GetControl (8);
	CancelButton.SetText (13727)
	CancelButton.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)

	AppearanceButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, "OpenAppearanceWindow")
	SoundButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, "OpenSoundWindow")
	ColorButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, "OpenColorWindow")
	ScriptButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, "OpenScriptWindow")
	BiographyButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, "OpenBiographyEditWindow")
	DoneButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, "CustomizeDonePress")
	CancelButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, "CustomizeCancelPress")

	CustomizeWindow.ShowModal (MODAL_SHADOW_NONE)
	return

def CustomizeDonePress ():
	if CustomizeWindow:
		CustomizeWindow.Unload ()
	return

def CustomizeCancelPress ():
	if CustomizeWindow:
		CustomizeWindow.Unload ()
	return

def OpenAppearanceWindow():
	global SubCustomizeWindow
	global PortraitButton
	global Gender, LastPortrait

	SubCustomizeWindow = GemRB.LoadWindowObject (18)
	pc = GemRB.GameGetSelectedPCSingle ()
	Gender = GemRB.GetPlayerStat (pc, IE_SEX, 1)
	PortraitName = GemRB.GetPlayerPortrait (pc, 0)
	LastPortrait = PortraitsTable.GetRowIndex (PortraitName[0:len(PortraitName)-1])

	PortraitButton = SubCustomizeWindow.GetControl (0)
	PortraitButton.SetFlags (IE_GUI_BUTTON_PICTURE|IE_GUI_BUTTON_NO_IMAGE,OP_SET)

	LeftButton = SubCustomizeWindow.GetControl (1)
	RightButton = SubCustomizeWindow.GetControl (2)

	DoneButton = SubCustomizeWindow.GetControl (3)
	DoneButton.SetText (11973)
	DoneButton.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)

	CancelButton = SubCustomizeWindow.GetControl (4)
	CancelButton.SetText (13727)
	CancelButton.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)

	OwnPortraitButton = SubCustomizeWindow.GetControl (5)
	OwnPortraitButton.SetText (17545)

	LeftButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, "PortraitLeftPress")
	RightButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, "PortraitRightPress")
	DoneButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, "DoneSubCustomizeWindow")
	CancelButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, "CloseSubCustomizeWindow")
	OwnPortraitButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, "OpenOwnPortraitWindow")
	SubCustomizeWindow.ShowModal (MODAL_SHADOW_NONE)

	while True:
		if PortraitsTable.GetValue (LastPortrait, 0) == Gender:
			UpdatePortrait()
			break
		LastPortrait = LastPortrait + 1

	return

def PortraitRightPress():
	global LastPortrait

	while True:
		LastPortrait = LastPortrait + 1
		if LastPortrait >= PortraitsTable.GetRowCount ():
			LastPortrait = 0
		if PortraitsTable.GetValue (LastPortrait, 0) == Gender:
			UpdatePortrait ()
			return

	return

def PortraitLeftPress():
	global LastPortrait

	while True:
		LastPortrait = LastPortrait - 1
		if LastPortrait < 0:
			LastPortrait = PortraitsTable.GetRowCount ()-1
		if PortraitsTable.GetValue (LastPortrait, 0) == Gender:
			UpdatePortrait ()
			return

	return

def UpdatePortrait():
	PortraitName = PortraitsTable.GetRowName (LastPortrait)+"L"
	PortraitButton.SetPicture (PortraitName, "NOPORTLG")
	return

def OpenOwnPortraitWindow():
	SubSubCustomizeWindow = GemRB.LoadWindowObject (19)

	SmallPortraitButton = SubSubCustomizeWindow.GetControl (1)
	SmallPortraitButton.SetFlags (IE_GUI_BUTTON_PICTURE|IE_GUI_BUTTON_NO_IMAGE,OP_SET)

	LargePortraitButton = SubSubCustomizeWindow.GetControl (0)
	LargePortraitButton.SetFlags (IE_GUI_BUTTON_PICTURE|IE_GUI_BUTTON_NO_IMAGE,OP_SET)

	DoneButton = SubSubCustomizeWindow.GetControl (12)
	DoneButton.SetText (11973)
	DoneButton.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)

	CancelButton = SubSubCustomizeWindow.GetControl (13)
	CancelButton.SetText (13727)
	CancelButton.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)

	DoneButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, "DoneSubSubCustomizeWindow")
	CancelButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, "CloseSubSubCustomizeWindow")
	SubSubCustomizeWindow.ShowModal (MODAL_SHADOW_NONE)
	return

def OpenSoundWindow():
	global SubCustomizeWindow
	global VoiceList
	global Gender

	SubCustomizeWindow = GemRB.LoadWindowObject (20)

	VoiceList = SubCustomizeWindow.GetControl (5)
	VoiceList.SetFlags (IE_GUI_TEXTAREA_SELECTABLE)
	pc = GemRB.GameGetSelectedPCSingle ()
	Gender = GemRB.GetPlayerStat (pc, IE_SEX, 1)

	VoiceList.SetVarAssoc ("Selected", 0)
	RowCount=VoiceList.GetCharSounds()

	PlayButton = SubCustomizeWindow.GetControl (7)
	PlayButton.SetText (17318)

	TextArea = SubCustomizeWindow.GetControl (8)
	TextArea.SetText (11315)

	DoneButton = SubCustomizeWindow.GetControl (10)
	DoneButton.SetText (11973)
	DoneButton.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)

	CancelButton = SubCustomizeWindow.GetControl (11)
	CancelButton.SetText (13727)
	CancelButton.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)

	PlayButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, "PlaySoundPressed")
	DoneButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, "DoneSubCustomizeWindow")
	CancelButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, "CloseSubCustomizeWindow")
	SubCustomizeWindow.ShowModal (MODAL_SHADOW_NONE)
	return

def PlaySoundPressed():
	global CharSoundWindow, SoundIndex, SoundSequence

	CharSound = VoiceList.QueryText()
	while (not GemRB.HasResource (CharSound + SoundSequence[SoundIndex], RES_WAV)):
		NextSound()
	GemRB.PlaySound (CharSound + SoundSequence[SoundIndex], 0, 0, 4)
	return

def OpenColorWindow():
	global SubCustomizeWindow
	global PortraitWindow

	SubCustomizeWindow = GemRB.LoadWindowObject (21)

	PortraitButton = SubCustomizeWindow.GetControl (0)
	PortraitButton.SetFlags (IE_GUI_BUTTON_PICTURE|IE_GUI_BUTTON_NO_IMAGE,OP_SET)

	DoneButton = SubCustomizeWindow.GetControl (12)
	DoneButton.SetText (11973)
	DoneButton.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)

	CancelButton = SubCustomizeWindow.GetControl (13)
	CancelButton.SetText (13727)
	CancelButton.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)

	DoneButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, "DoneSubCustomizeWindow")
	CancelButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, "CloseSubCustomizeWindow")
	SubCustomizeWindow.ShowModal (MODAL_SHADOW_NONE)
	return

def UpdatePaperDoll():

	pc = GemRB.GameGetSelectedPCSingle ()
	Color1 = GemRB.GetPlayerStat (pc, IE_METAL_COLOR)
	Color2 = GemRB.GetPlayerStat (pc, IE_MINOR_COLOR)
	Color3 = GemRB.GetPlayerStat (pc, IE_MAJOR_COLOR)
	Color4 = GemRB.GetPlayerStat (pc, IE_SKIN_COLOR)
	Color5 = GemRB.GetPlayerStat (pc, IE_LEATHER_COLOR)
	Color6 = GemRB.GetPlayerStat (pc, IE_ARMOR_COLOR)
	Color7 = GemRB.GetPlayerStat (pc, IE_HAIR_COLOR)
	PortraitButton.SetPLT (GetActorPaperDoll (pc),
		Color1, Color2, Color3, Color4, Color5, Color6, Color7, 0, 0)
	return

def OpenScriptWindow():
	global SubCustomizeWindow
	global ScriptTextArea, SelectedTextArea

	SubCustomizeWindow = GemRB.LoadWindowObject (11)

	ScriptTextArea = SubCustomizeWindow.GetControl (2)
	ScriptTextArea.SetFlags (IE_GUI_TEXTAREA_SELECTABLE)
	ScriptTextArea.SetVarAssoc ("Selected", 0)

	SelectedTextArea = SubCustomizeWindow.GetControl (4)
	FillScriptList ()
	UpdateScriptSelection ()

	DoneButton = SubCustomizeWindow.GetControl (5)
	DoneButton.SetText (11973)
	DoneButton.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)

	CancelButton = SubCustomizeWindow.GetControl (6)
	CancelButton.SetText (13727)
	CancelButton.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)

	DoneButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, "DoneSubCustomizeWindow")
	CancelButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, "CloseSubCustomizeWindow")
	ScriptTextArea.SetEvent (IE_GUI_TEXTAREA_ON_CHANGE, "UpdateScriptSelection")
	SubCustomizeWindow.ShowModal (MODAL_SHADOW_NONE)
	return

def FillScriptList():
	ScriptTextArea.Clear ()
	row = ScriptsTable.GetRowCount ()
	for i in range (row):
		GemRB.SetToken ("script", ScriptsTable.GetRowName (i) )
		title = ScriptsTable.GetValue (i,0)
		if title!=-1:
			desc = ScriptsTable.GetValue (i,1)
			txt = GemRB.GetString (title)

			if (desc!=-1):
				txt += GemRB.GetString (desc)

			ScriptTextArea.Append (txt+"\n", -1)

		else:
			ScriptTextArea.Append (ScriptsTable.GetRowName (i)+"\n" ,-1)

	return

def UpdateScriptSelection():
	text = ScriptTextArea.QueryText ()
	SelectedTextArea.SetText (text)
	print "Selected script: ", ScriptsTable.GetRowName(GemRB.GetVar("Selected"))
	return

def OpenBiographyEditWindow():
	global SubCustomizeWindow
	global BioStrRef

	Changed = 0
	BioStrRef = GemRB.GetPlayerString(pc, 74)
	if BioStrRef != 33347:
		Changed = 1

	SubCustomizeWindow = GemRB.LoadWindowObject (23)

	ClearButton = SubCustomizeWindow.GetControl (5)
	DoneButton = SubCustomizeWindow.GetControl (1)
	DoneButton.SetText (11973)
	DoneButton.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)

	RevertButton = SubCustomizeWindow.GetControl (3)
	if not Changed:
		RevertButton.SetState (IE_GUI_BUTTON_DISABLED)

	CancelButton = SubCustomizeWindow.GetControl (2)
	CancelButton.SetText (13727)
	CancelButton.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)

	TextArea = SubCustomizeWindow.GetControl (4)
	TextArea.SetText (BioStrRef)

	ClearButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, "ClearBiography")
	DoneButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, "DoneSubCustomizeWindow")
	RevertButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, "RevertBiography")
	CancelButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, "CloseSubCustomizeWindow")
	SubCustomizeWindow.ShowModal (MODAL_SHADOW_NONE)
	return

def ClearBiography():
	BioStrRef = None
	TextArea.SetText ("")
	return

def RevertBiography():
	global BioStrRef

	BioStrRef = 33347
	TextArea.SetText (33347)
	return

def CloseSubCustomizeWindow():
	if SubCustomizeWindow:
		SubCustomizeWindow.Unload ()
	return

def DoneSubCustomizeWindow():
	if SubCustomizeWindow:
		SubCustomizeWindow.Unload ()
	return

def CloseSubSubCustomizeWindow():
	if SubSubCustomizeWindow:
		SubSubCustomizeWindow.Unload ()
	return

###################################################
# End of file GUIREC.py
