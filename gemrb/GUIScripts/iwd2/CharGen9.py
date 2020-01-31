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
import GUICommon
import CommonTables
import CharOverview
import LUCommon
import IDLUCommon

BioWindow = 0
BioData = 0
BioStrRef = 0
EditControl = 0
PortraitName = ""

def OnLoad():
	CharOverview.UpdateOverview(9)
	if CharOverview.CharGenWindow:
		CharOverview.PersistButtons['Next'].SetState(IE_GUI_BUTTON_UNPRESSED) # Fixes button being pre-pressed
	RevertPress()
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
		# luckily they're all level 1
		import Spellbook
		Spellbook.LearnSpell (MyChar, resource, IE_IWD2_SPELL_INNATE, 0, count)
	return

def SetRaceBonuses(MyChar, racetitle):
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
	global BioStrRef
	BioTable = GemRB.LoadTable ("bios")
	ClassName = GUICommon.GetClassRowName (GemRB.GetVar ("BaseClass")-1, "index")
	BioStrRef = BioTable.GetValue (ClassName, "BIO")
	GemRB.SetToken ("BIO", GemRB.GetString(BioStrRef) )
	if type (EditControl) != type (7350): # just some int
		EditControl.SetText (GemRB.GetToken("BIO") )
	return

def BioCancelPress():
	GemRB.SetToken("BIO",BioData)
	if BioWindow:
		BioWindow.Unload ()
	return

def BioDonePress():
	GemRB.SetToken ("BIO", EditControl.QueryText())
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
	EditControl.SetStatus (IE_GUI_CONTROL_FOCUSED)
	return

def NextPress():
	#set my character up
	MyChar = GemRB.GetVar ("Slot")

	racename = CommonTables.Races.GetRowName (IDLUCommon.GetRace (MyChar))

	#base class
	Class=GemRB.GetVar ("BaseClass")
	BaseClassName = CommonTables.Classes.GetRowName (Class-1)
	#kit
	kitrow = GemRB.GetVar ("Class")-1
	if (CommonTables.Classes.GetValue(kitrow, 3) == 0):
		#baseclass
		clssname = BaseClassName
	else:
		#kit
		clssname = CommonTables.Classes.GetRowName (kitrow)
	IDLUCommon.AddResistances (MyChar, clssname, "clssrsmd")

	IDLUCommon.AddResistances (MyChar, racename, "racersmd")
	SetRaceBonuses(MyChar, racename)
	SetRaceAbilities (MyChar, racename)

	# setup saving throws
	IDLUCommon.SetupSavingThrows (MyChar, Class, True)

	# 10 is a weapon slot (see slottype.2da row 10)
	if not GemRB.GetVar ("ImportedChar"):
		GemRB.CreateItem (MyChar, "00staf01", 10, 1, 0, 0)
	GemRB.SetEquippedQuickSlot (MyChar, 0)

	# reset hitpoints
	if not GemRB.GetVar ("ImportedChar"):
		GemRB.SetPlayerStat (MyChar, IE_MAXHITPOINTS, 0, 0)
		GemRB.SetPlayerStat (MyChar, IE_HITPOINTS, 0, 0)
	LUCommon.SetupHP (MyChar)

	t=GemRB.GetVar ("Alignment")
	GemRB.SetPlayerStat (MyChar, IE_ALIGNMENT, CommonTables.Aligns.GetValue (t, 3))
	TmpTable=GemRB.LoadTable ("repstart")
	#t=CommonTables.Aligns.FindValue (3,t)
	t=TmpTable.GetValue (t,0)
	GemRB.SetPlayerStat (MyChar, IE_REPUTATION, t)
	TmpTable=GemRB.LoadTable ("strtgold")
	a = TmpTable.GetValue (Class, 1) #number of dice
	b = TmpTable.GetValue (Class, 0) #size
	c = TmpTable.GetValue (Class, 2) #adjustment
	d = TmpTable.GetValue (Class, 3) #external multiplier
	e = TmpTable.GetValue (Class, 4) #level bonus rate
	t = GemRB.GetPlayerStat (MyChar, IE_CLASSLEVELSUM)
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

	# feats are set already in the Feats step

	#does all the rest
	LargePortrait = GemRB.GetToken ("LargePortrait")
	SmallPortrait = GemRB.GetToken ("SmallPortrait")
	GemRB.FillPlayerInfo (MyChar, LargePortrait, SmallPortrait, 1)
	GemRB.SetNextScript ("SPPartyFormation")

	# apply class/kit abilities
	# reset levels, so pcf_level can apply any clabs
	# this way we don't need to port GUICommon.ResolveClassAbilities, as core does everything
	Levels = IDLUCommon.Levels
	GemRB.SetPlayerStat (MyChar, Levels[Class-1], 0, 0)
	GemRB.SetPlayerStat (MyChar, Levels[Class-1], 1)

	#starting xp is race dependent
	TmpTable = GemRB.LoadTable ("strtxp")
	xp = TmpTable.GetValue (racename, "VALUE")
	GemRB.SetPlayerStat (MyChar, IE_XP, xp )

	BioData = GemRB.GetToken("BIO")
	if BioData == GemRB.GetString (BioStrRef):
		NewRef = BioStrRef
	else:
		NewRef = GemRB.CreateString (62015+MyChar, BioData)
	GemRB.SetPlayerString (MyChar, 63, NewRef)
	GemRB.SetVar ("ImportedChar", 0)

	# set memorized spells as non-depleted - ready to use
	GemRB.ChargeSpells (MyChar)

	# core will call this for us on area load, so no need to repeat
	#LUCommon.ApplyFeats(MyChar)
	return
