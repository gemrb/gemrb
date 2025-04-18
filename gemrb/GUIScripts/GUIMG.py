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
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#


# GUIMG.py - scripts to control mage spells windows from the GUIMG winpack

###################################################

import GemRB
import GameCheck
import GUICommon
import CommonTables
import GUICommonWindows
import Spellbook
from GUICommon import BindControlCallbackParams
from GUIDefines import *
from ie_stats import *
from ie_spells import LS_MEMO

MageWindow = None
MageSpellLevel = 0

# bg2 stuff for handling triggers and contingencies
Sorcerer = None
OtherWindow = None
Exclusions = None
ContCond = None
ContTarg = None
SpellType = None
Level = 1

FlashResRef = "FLASHBR" if GameCheck.IsBG2OrEE () else "FLASH"

def ToggleSpellWindow (btn):
	global Sorcerer
	# added game check, since although sorcerers have almost no use for their spellbook, there's no other way to quickly check spell descriptions
	pc = GemRB.GameGetSelectedPCSingle ()
	Sorcerer = GameCheck.IsBG2OrEE () and Spellbook.HasSorcererBook (pc) > 0
	
	ToggleSpellWindow.Args = btn
	
	if Sorcerer:
		ToggleSorcererWindow (btn)
	else:
		ToggleMageWindow (btn)

def InitMageWindow (window):
	global MageWindow
	MageWindow = window

	Button = MageWindow.GetControl (1)
	Button.OnPress (MagePrevLevelPress)

	Button = MageWindow.GetControl (2)
	Button.OnPress (MageNextLevelPress)

	# contingencies
	Button = MageWindow.GetControl (55)
	if GameCheck.IsBG2EE ():
		Button = MageWindow.GetControl (59)
	if Button:
		Button.OnPress (OpenContingenciesWindow)
		pc = GemRB.GameGetSelectedPCSingle ()
		if GemRB.CountEffects (pc, "CastSpellOnCondition", -1, -1) + GemRB.CountEffects (pc, "Sequencer:Store", -1, -1) == 0:
			Button.SetState (IE_GUI_BUTTON_DISABLED)
			Button.SetText (None)
		else:
			Button.SetState (IE_GUI_BUTTON_ENABLED)
			Button.SetText (34586)

	#setup level buttons
	spellLevelOffset = None
	if GameCheck.IsBG2 ():
		spellLevelOffset = 56
	elif GameCheck.IsBG2EE ():
		spellLevelOffset = 61
	if spellLevelOffset:
		for i in range (9):
			Button = MageWindow.GetControl (spellLevelOffset + i)
			Button.OnPress (RefreshMageLevel)
			Button.SetFlags (IE_GUI_BUTTON_RADIOBUTTON, OP_OR)
			Button.SetVarAssoc ("MageSpellLevel", i)

	# Setup memorized spells buttons
	if not Sorcerer:
		for i in range (12):
			Button = MageWindow.GetControl (3 + i)
			color = {'r' : 0, 'g' : 0, 'b' :0, 'a' : 64}
			Button.SetBorder (0,color,0,1)
			Button.SetSprites ("SPELFRAM",0,0,0,0,0)
			Button.SetState (IE_GUI_BUTTON_LOCKED)
			Button.SetAnimation (None)
			Button.SetVarAssoc ("Memorized", i)

	# reposition from a 4x3 grid to the bottom or top
	# for mages that's the memoed spells, for sorcerers the known
	if GameCheck.IsBG2EE ():
		for i in range (12):
			Button = MageWindow.GetControl (3 + i + 24 * int(Sorcerer))
			row = i // 4
			col = i % 4
			width = 54 + 8 * (1 + int(Sorcerer)) # slot + padding
			# don't bother transposing 4x3 grid to 3x4 grid
			if Sorcerer:
				frame = Button.GetFrame ()
				x = frame["x"] - (314 - 166)
				y = frame["y"] - (536 - 234)
			else:
				x = 100 + row * width * 4 + col * width + 66 * int(Sorcerer)
				y = 644
			Button.SetPos (x, y)
			# the bam isn't centered, so let's cheat
			Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_OR)

		# some extra slot button, sorcerer button and a label on a button
		MageWindow.DeleteControl (51)
		if Sorcerer:
			MageWindow.DeleteControl (59)
		else:
			MageWindow.DeleteControl (0x10000000 + 51)

	# Setup book spells buttons
	for i in range (GUICommon.GetGUISpellButtonCount()):
		Button = MageWindow.GetControl (27 + i)
		Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_SET)
		Button.SetState (IE_GUI_BUTTON_LOCKED)
		Button.SetValue (i)

	UpdateMageWindow (MageWindow)
	return

def UpdateMageWindow (MageWindow):
	pc = GemRB.GameGetSelectedPCSingle ()
	spelltype = IE_SPELL_TYPE_WIZARD
	level = MageSpellLevel
	max_mem_cnt = GemRB.GetMemorizableSpellsCount (pc, spelltype, level, 1)

	GUICommon.AdjustWindowVisibility (MageWindow, pc, GUICommon.CantUseSpellbookWindow (pc))

	Label = MageWindow.GetControl (0x10000032)
	if GameCheck.IsBG2OrEE ():
		GemRB.SetToken ("SPELLLEVEL", str(level + 1))
		Label.SetText (10345)
	else:
		GemRB.SetToken ('LEVEL', str(level + 1))
		Label.SetText (12137)

	Name = GemRB.GetPlayerName (pc, 0)
	Label = MageWindow.GetControl (0x10000035)
	Label.SetText (Name)

	known_cnt = GemRB.GetKnownSpellsCount (pc, spelltype, level)
	mem_cnt = GemRB.GetMemorizedSpellsCount (pc, spelltype, level, False)
	true_mem_cnt = GemRB.GetMemorizedSpellsCount (pc, spelltype, level, True)
	if not Sorcerer:
		for i in range (12):
			Button = MageWindow.GetControl (3 + i)

			if i < mem_cnt:
				ms = GemRB.GetMemorizedSpell (pc, spelltype, level, i)
				Button.SetSpellIcon (ms['SpellResRef'], 0)
				if not GameCheck.IsBG2EE ():
					Button.SetFlags (IE_GUI_BUTTON_PICTURE, OP_SET)
				if ms['Flags']:
					Button.OnPress (OpenMageSpellUnmemorizeWindow)
				else:
					Button.OnPress (OnMageUnmemorizeSpell)
				spell = GemRB.GetSpell (ms['SpellResRef'])
				Button.OnRightPress(BindControlCallbackParams(OpenMageSpellInfoWindow, spell, Button.VarName))
				Button.EnableBorder (0, ms['Flags'] == 0)
				Button.SetTooltip (spell['SpellName'])
			else:
				if i < max_mem_cnt:
					Button.SetFlags (IE_GUI_BUTTON_NORMAL, OP_SET)
				else:
					Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_SET)
				Button.OnPress (None)
				Button.OnRightPress (None)
				Button.SetTooltip ('')
				Button.EnableBorder (0, 0)
	else:
		label = MageWindow.GetControl (0x10000040)
		if known_cnt:
			# we give sorcerers all charges for all the spells, so some extra math is needed
			label.SetText (GemRB.GetString(61256) + " " + str(true_mem_cnt // known_cnt) + "/" + str(max_mem_cnt))
		else:
			label.SetText ("")
	
	btncount = GUICommon.GetGUISpellButtonCount()
	i = 0
	for i in range (known_cnt):
		Button = MageWindow.GetControl (27 + i)
		
		ks = GemRB.GetKnownSpell (pc, spelltype, level, i)
		Button.SetSpellIcon (ks['SpellResRef'], 0)
		Button.OnPress (OnMageMemorizeSpell)
		spell = GemRB.GetSpell (ks['SpellResRef'])
		spell['CurrentIndex'] = i # save it for later
		Button.OnRightPress (BindControlCallbackParams(OpenMageSpellInfoWindow, spell, Button.VarName))
		Button.SetTooltip (spell['SpellName'])

	if known_cnt == 0: i = -1
	for i in range (i + 1, btncount):
		Button = MageWindow.GetControl (27 + i)
		
		Button.SetFlags (IE_GUI_BUTTON_PICTURE, OP_NAND)
		Button.OnPress (None)
		Button.OnRightPress (None)
		Button.SetTooltip ('')
		Button.EnableBorder (0, 0)
	
	return

def MageSelectionChanged (oldwin):
	global Sorcerer
	# added game check, since although sorcerers have almost no use for their spellbook, there's no other way to quickly check spell descriptions
	
	UpdateMageWindow(oldwin)
	
	pc = GemRB.GameGetSelectedPCSingle ()
	Sorcerer = GameCheck.IsBG2OrEE () and Spellbook.HasSorcererBook (pc)

	if Sorcerer:
		OpenSorcererWindow (ToggleSpellWindow.Args)
	else:
		OpenMageWindow (ToggleSpellWindow.Args)

ToggleMageWindow = GUICommonWindows.CreateTopWinLoader(2, "GUIMG", GUICommonWindows.ToggleWindow, InitMageWindow, MageSelectionChanged, True)
OpenMageWindow = GUICommonWindows.CreateTopWinLoader(2, "GUIMG", GUICommonWindows.OpenWindowOnce, InitMageWindow, MageSelectionChanged, True)

ToggleSorcererWindow = GUICommonWindows.CreateTopWinLoader(8, "GUIMG", GUICommonWindows.ToggleWindow, InitMageWindow, MageSelectionChanged, True)
OpenSorcererWindow = GUICommonWindows.CreateTopWinLoader(8, "GUIMG", GUICommonWindows.OpenWindowOnce, InitMageWindow, MageSelectionChanged, True)

def MagePrevLevelPress ():
	global MageSpellLevel

	if MageSpellLevel > 0:
		MageSpellLevel = MageSpellLevel - 1
		UpdateMageWindow (MageWindow)
	return

def MageNextLevelPress ():
	global MageSpellLevel

	if MageSpellLevel < 8:
		MageSpellLevel = MageSpellLevel + 1
		UpdateMageWindow (MageWindow)
	return

def RefreshMageLevel ():
	global MageSpellLevel

	MageSpellLevel = GemRB.GetVar ("MageSpellLevel")
	UpdateMageWindow (MageWindow)
	return

def OpenMageSpellInfoWindow (spell, kind):
	Window = GemRB.LoadWindow (3, "GUIMG")

	#back
	Button = Window.GetControl (5)
	Button.SetText (15416)
	Button.OnPress (Window.Close)

	#erase
	Button = Window.GetControl (6)
	if Button:
		if kind == "Memorized" or Sorcerer:
			Button.OnPress (None)
			Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_SET)
		else:
			Button.OnPress (lambda: OpenMageSpellRemoveWindow(Window, spell['CurrentIndex']))
			Button.SetText (63668)

	Label = Window.GetControl (0x0fffffff)
	Label.SetText (spell['SpellName'])

	Button = Window.GetControl (2)
	Button.SetSpellIcon (spell['SpellResRef'], 1)

	Text = Window.GetControl (3)
	Text.SetText (spell['SpellDesc'])

	Window.ShowModal (MODAL_SHADOW_GRAY)
	return

def OnMageMemorizeSpell (btn):
	pc = GemRB.GameGetSelectedPCSingle ()
	level = MageSpellLevel
	spelltype = IE_SPELL_TYPE_WIZARD
	Window = btn.Window

	def Complete():
		mem_cnt = GemRB.GetMemorizedSpellsCount (pc, spelltype, level, False)
		AnimBtn = Window.GetControl(mem_cnt + 2)
		AnimBtn.SetAnimation(FlashResRef, 0, A_ANI_PLAYONCE | A_ANI_BLEND)
		AnimBtn.OnAnimEnd(lambda: UpdateMageWindow(Window))
		UpdateMageWindow(Window)

	index = btn.Value
	if GemRB.MemorizeSpell (pc, spelltype, level, index):
		GemRB.PlaySound ("GAM_24")
		Button = MageWindow.GetControl(index + 27)
		Button.SetAnimation (FlashResRef, 0, A_ANI_PLAYONCE | A_ANI_BLEND)
		Button.OnAnimEnd(Complete)

	return

def OpenMageSpellRemoveWindow (parentWin, spellIndex):
	if GameCheck.IsBG2OrEE ():
		Window = GemRB.LoadWindow (101, "GUIMG")
	else:
		Window = GemRB.LoadWindow (5, "GUIMG")

	# "Are you sure you want to ....?"
	TextArea = Window.GetControl (3)
	TextArea.SetText (63745)

	# Remove
	Button = Window.GetControl (0)
	Button.SetText (17507)
	
	def RemoveSpell (spellIndex):
		OnMageRemoveSpell (spellIndex)
		Window.Close()
		parentWin.Close()
	
	Button.OnPress (lambda: RemoveSpell(spellIndex))
	Button.MakeDefault()

	# Cancel
	Button = Window.GetControl (1)
	Button.SetText (13727)
	Button.OnPress (Window.Close)
	Button.MakeEscape()

	Window.ShowModal (MODAL_SHADOW_GRAY)
	return

def OpenMageSpellUnmemorizeWindow (btn):
	if GameCheck.IsBG2OrEE ():
		Window = GemRB.LoadWindow (101, "GUIMG")
	else:
		Window = GemRB.LoadWindow (5, "GUIMG")

	# "Are you sure you want to ....?"
	TextArea = Window.GetControl (3)
	TextArea.SetText (11824)

	# Remove
	Button = Window.GetControl (0)
	Button.SetText (17507)
	
	def Unmemorize (btn):
		OnMageUnmemorizeSpell (btn)
		Window.Close()

	Button.SetValue (btn.Value)
	Button.OnPress (Unmemorize)
	Button.MakeDefault()

	# Cancel
	Button = Window.GetControl (1)
	Button.SetText (13727)
	Button.OnPress (Window.Close)
	Button.MakeEscape()

	Window.ShowModal (MODAL_SHADOW_GRAY)
	return

def OnMageUnmemorizeSpell (btn):
	pc = GemRB.GameGetSelectedPCSingle ()
	level = MageSpellLevel
	spelltype = IE_SPELL_TYPE_WIZARD
	index = btn.Value

	if GemRB.UnmemorizeSpell (pc, spelltype, level, index):
		GemRB.PlaySound ("GAM_44")
		Button = MageWindow.GetControl(index + 3)
		Button.SetAnimation (FlashResRef, 0, A_ANI_PLAYONCE | A_ANI_BLEND)
		Button.OnAnimEnd(lambda: UpdateMageWindow (MageWindow))
	return

def OnMageRemoveSpell (spellIndex):
	pc = GemRB.GameGetSelectedPCSingle ()
	level = MageSpellLevel
	spelltype = IE_SPELL_TYPE_WIZARD

	#remove spell from book
	GemRB.RemoveSpell (pc, spelltype, level, spellIndex)
	UpdateMageWindow (MageWindow)
	return

def LoadCondition ():
	global ContCond, ContTarg

	Table = GemRB.LoadTable ("contcond")
	CondCount = Table.GetRowCount ()
	ContCond = [0] * CondCount

	for i in range (CondCount):
		#get the condition's name and description
		CondTuple = (Table.GetValue (i, 0), Table.GetValue (i, 1))
		ContCond[i] = CondTuple

	Table = GemRB.LoadTable ("conttarg")
	TargCount = Table.GetRowCount ()
	ContTarg = [0] * TargCount

	for i in range (TargCount):
		#get the condition's name and description
		CondTuple = (Table.GetValue (i, 0), Table.GetValue (i, 1))
		ContTarg[i] = CondTuple
	return

def OpenSequencerWindow ():
	global OtherWindow
	global ContingencyTextArea, TypeButton, OkButton
	global Level, MaxLevel, Target, Count
	global pc
	global ContCond, ContTarg
	global Spell1, Spell2, Spell3, Source

	if ContCond == None:
		LoadCondition()

	Level = 1
	Spell1 = ""
	Spell2 = ""
	Spell3 = ""
	#the target player (who receives the contingency or sequencer)
	pc = GemRB.GetVar("P0")
	ClassName = GUICommon.GetClassRowName (pc)
	#maximum spell level
	MaxLevel = GemRB.GetVar("P1")
	#target 0 - any, 1 - caster only, 2 - sequencer
	#spell count 1-3
	p2 = GemRB.GetVar("P2")
	Source = GemRB.GetSpellCastOn(pc)

	print("Source: ", Source)
	Target = p2>>16
	Count = p2&255
	if Count > 3:
		Count = 3

	#saving the original portrait window
	OtherWindow = Window = GemRB.LoadWindow (6, "GUIMG")

	Title = Window.GetControl (0x0fffffff)

	ContingencyTextArea = Window.GetControl (25)

	CondSelect = Window.GetControl (4)
	CondLabel = Window.GetControl (0x10000000)
	TargSelect = Window.GetControl (6)
	TargLabel = Window.GetControl (0x10000001)
	TypeButton = Window.GetControl (8)

	#no cleric spells available
	if CommonTables.ClassSkills.GetValue (ClassName, "DRUIDSPELL") == "*" and\
			CommonTables.ClassSkills.GetValue (ClassName, "CLERICSPELL") == "*":
		TypeButton.SetState (IE_GUI_BUTTON_DISABLED)

	if Target == 2:
		CondSelect.SetVisible (False)
		CondLabel.SetVisible (False)
		TargSelect.SetVisible (False)
		TargLabel.SetVisible (False)
	else:
		CondSelect.OnSelect (ContingencyHelpCondition)
		CondSelect.SetOptions ([elem[0] for elem in ContCond], "ContCond", 0)

		TargSelect.OnSelect (ContingencyHelpTarget)
		if Target:
			TargSelect.SetOptions ([ContTarg[0][0]], "ContTarg", 1)
		else:
			TargSelect.SetOptions ([elem[0] for elem in ContTarg], "ContTarg", 0)

	GemRB.SetVar ("SpellType", 0)
	TypeButton.SetVarAssoc ("SpellType", 0)
	TypeButton.SetFlags (IE_GUI_BUTTON_CHECKBOX, OP_OR)
	TypeButton.OnPress (ContTypePressed)

	Button = Window.GetControl (9)
	Button.OnPress (LevelIncrease)
	Button = Window.GetControl (10)
	Button.OnPress (LevelDecrease)

	OkButton = Window.GetControl (27)
	OkButton.SetText (11973)
	OkButton.MakeDefault()
	OkButton.SetState (IE_GUI_BUTTON_DISABLED)

	CancelButton = Window.GetControl (29)
	CancelButton.SetText (13727)
	CancelButton.MakeEscape()

	OkButton.OnPress (ContingencyOk)
	CancelButton.OnPress (ContingencyCancel)
	ContTypePressed ()
	Window.ShowModal (MODAL_SHADOW_GRAY)

	# this is here because core runs the selection change event handler too often
	if Target == 2:
		if Count < 3:
			Title.SetText (55374)
			ContingencyTextArea.SetText (60420)
		else:
			Title.SetText (55375)
			ContingencyTextArea.SetText (55372)
	else:
		if Count < 3:
			Title.SetText (11941)
		else:
			Title.SetText (55376)
		ContingencyTextArea.SetText (55373)

	return

def UpdateSpellList ():
	Window = OtherWindow

	if SpellType:
		Type = IE_SPELL_TYPE_PRIEST
	else:
		Type = IE_SPELL_TYPE_WIZARD

	Label = Window.GetControl (0x1000001d)
	GemRB.SetToken ('LEVEL', str(Level))
	Label.SetText (12137)

	BuildSpellList(pc, Type, Level-1)

	names = list(SpellList.keys())
	names.sort()

	cnt = len(names)
	j = 0
	for i in range(11, 22):
		Button = Window.GetControl (i)
		Button.SetFont ("NUMBER")
		Button.SetFlags (IE_GUI_BUTTON_ALIGN_RIGHT|IE_GUI_BUTTON_ALIGN_BOTTOM,OP_OR)
		if j<cnt:
			Button.SetSpellIcon (names[j], 1)
			Button.SetText( str(SpellList[names[j]]) )
			Button.SetVarAssoc("PickedSpell", j)
			Button.OnPress (ContingencyHelpSpell)
			Button.SetState (IE_GUI_BUTTON_ENABLED)
		else:
			Button.SetSpellIcon("")
			Button.SetText("")
			Button.SetState (IE_GUI_BUTTON_DISABLED)
		j = j+1

	Button = Window.GetControl (22)
	Button.SetSpellIcon(Spell1, 1)
	Button.OnPress (DeleteSpell1)
	Button = Window.GetControl (23)
	Button.SetSpellIcon(Spell2, 1)
	Button.OnPress (DeleteSpell2)
	Button = Window.GetControl (24)
	Button.SetSpellIcon(Spell3, 1)
	Button.OnPress (DeleteSpell3)

	if not Spell1:
		OkButton.SetState (IE_GUI_BUTTON_DISABLED)
	else:
		OkButton.SetState (IE_GUI_BUTTON_ENABLED)
	return

def BuildSpellList (pc, spelltype, level):
	global SpellList

	import Spellbook

	if not Exclusions:
		LoadExclusions()

	SpellList = {}
	memoedSpells = Spellbook.GetMemorizedSpells (pc, spelltype, level)

	# are we a sorcerer-style?
	SorcererBook = Spellbook.HasSorcererBook (pc)

	names = []
	for s in memoedSpells:
		names.append(s["SpellResRef"])

	if SorcererBook:
		dec = 0
		for selected in Spell1, Spell2, Spell3:
			if selected in names:
				dec += 1
		# decrease count for all spells
		if dec > 0:
			for ms in memoedSpells:
				ms["MemoCount"] -= dec
	else:
		for s in memoedSpells:
			sref = s["SpellResRef"]
			for selected in Spell1, Spell2, Spell3:
				if sref == selected:
					s["MemoCount"] -= 1

	for s in memoedSpells:
		if s["MemoCount"] < 1:
			continue

		spell = s["SpellResRef"]
		if spell in Exclusions[level]:
			continue
		SpellList[spell] = s["MemoCount"]
	return

def ContTypePressed ():
	global SpellType

	Label = OtherWindow.GetControl (0x10000002)
	SpellType = GemRB.GetVar ("SpellType")

	if SpellType:
		Label.SetText (54352)
		TypeButton.SetText (18039)
	else:
		Label.SetText (21836)
		TypeButton.SetText (7204)
	UpdateSpellList()
	return

def ContingencyOk ():
	global OtherWindow

	#set the storage
	if Target == 2:
		GemRB.ApplyEffect (pc, "Sequencer:Store", 0, 0, Spell1, Spell2, Spell3, Source)
	else:
		GemRB.ApplyEffect (pc, "CastSpellOnCondition", GemRB.GetVar ("ContTarg"), GemRB.GetVar ("ContCond"), Spell1, Spell2, Spell3, Source)
	#set the innate
	if GemRB.LearnSpell (pc, Source+"d", LS_MEMO):
		GemRB.Log (LOG_ERROR, "ContingencyOk", "Failed to learn sequencer/contingency!")
	OtherWindow.Close ()
	return

def ContingencyCancel ():
	global OtherWindow

	GemRB.SetPlayerStat (pc, IE_IDENTIFYMODE, 0)
	OtherWindow.Close ()
	return

def ContingencyHelpSpell ():
	global Spell1, Spell2, Spell3

	names = list(SpellList.keys())
	names.sort()
	i = GemRB.GetVar("PickedSpell")
	spell = names[i]

	if Spell1=="" and Count>0:
		Spell1 = spell
	elif Spell2=="" and Count>1:
		Spell2 = spell
	elif Spell3=="" and Count>2:
		Spell3 = spell

	spl = GemRB.GetSpell(spell)
	ContingencyTextArea.SetText (spl["SpellDesc"])
	UpdateSpellList ()
	return

def DeleteSpell1 ():
	global Spell1, Spell2, Spell3

	Spell1 = Spell2
	Spell2 = Spell3
	Spell3 = ""
	ContingencyTextArea.SetText("")
	UpdateSpellList ()
	return

def DeleteSpell2 ():
	global Spell2, Spell3

	Spell2 = Spell3
	Spell3 = ""
	ContingencyTextArea.SetText("")
	UpdateSpellList ()
	return

def DeleteSpell3 ():
	global Spell3

	Spell3 = ""
	ContingencyTextArea.SetText("")
	UpdateSpellList ()
	return

def ContingencyHelpCondition ():
	i = GemRB.GetVar ("ContCond")
	ContingencyTextArea.SetText (ContCond[i][1])
	return

def ContingencyHelpTarget ():
	i = GemRB.GetVar ("ContTarg")
	ContingencyTextArea.SetText (ContTarg[i][1])
	return

def LoadExclusions():
	global Exclusions

	ExclusionTable = GemRB.LoadTable ("contingx")
	Columns = ExclusionTable.GetColumnCount ()
	Rows = ExclusionTable.GetRowCount ()
	Exclusions = []
	for i in range (Columns):
		Exclusions.append ([])
		for j in range (Rows):
			spell = ExclusionTable.GetValue (j, i, GTV_STR)
			if spell[0]=="*":
				break
			Exclusions[i].append (spell.lower())

	return

def LevelIncrease():
	global Level

	if Level<MaxLevel:
		Level = Level+1
	UpdateSpellList()
	return

def LevelDecrease():
	global Level

	if Level>1:
		Level = Level-1
	UpdateSpellList()
	return

def OpenContingenciesWindow():
	global OtherWindow

	OtherWindow = Window = GemRB.LoadWindow (7, "GUIMG")

	OkButton = Window.GetControl (2)
	OkButton.SetText (11973)
	OkButton.OnPress (Window.Close)
	OkButton.MakeDefault ()

	pc = GemRB.GameGetSelectedPCSingle ()
	contingencies = GemRB.GetEffects (pc, "CastSpellOnCondition")
	sequencers = GemRB.GetEffects (pc, "Sequencer:Store")
	targets = [ 30992, 30990, 30994, "Anyone"]
	conditions = [ "Was hit", 30973, 30975, 30982, 30984, 30986, 30988, "Attacked", "Near 4'", "Near 10'", "Every round", "Took damage", "Killed", "Time of day", "In personal space", "State", "I died", "Someone died", "Got turned", "HP <", "HP% <", "Spell state" ]
	for cidx in range(5):
		rowID1 = Window.GetControl (0x10000000 + 31 + cidx)
		rowID2 = Window.GetControl (0x10000000 + 37 + cidx)
		rowID3 = Window.GetControl (0x10000000 + 43 + cidx)
		spellBtn1 = Window.GetControl (25 + cidx)
		spellBtn2 = Window.GetControl (31 + cidx)
		spellBtn3 = Window.GetControl (37 + cidx)
		handBtn = Window.GetControl (43 + cidx)
		spellBtn1.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_OR)
		spellBtn2.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_OR)
		spellBtn3.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_OR)
		spellBtn1.SetState (IE_GUI_BUTTON_LOCKED)
		spellBtn2.SetState (IE_GUI_BUTTON_LOCKED)
		spellBtn3.SetState (IE_GUI_BUTTON_LOCKED)
		if cidx < len(contingencies):
			rowID1.SetText (str(cidx + 1))
			cont = contingencies[cidx]
			rowID2.SetText (conditions[cont["Param2"]])
			rowID3.SetText (targets[cont["Param1"]])

			spellBtn1.SetBAM (cont["Spell1Icon"][:-1] + "b", 0, 0)
			if cont["Resource2"] != "":
				spellBtn2.SetBAM (cont["Spell2Icon"][:-1] + "b", 0, 0)
			if cont["Resource3"] != "":
				spellBtn3.SetBAM (cont["Spell3Icon"][:-1] + "b", 0, 0)

			# warning: can take out too many
			handBtn.OnPress (lambda: GemRB.DispelEffect (pc, "CastSpellOnCondition", cont["Param2"]))
		elif cidx < len(contingencies) + len(sequencers):
			rowID1.SetText (str(cidx + 1))
			cont = sequencers[cidx - len(contingencies)]
			rowID2.SetText (25951)
			rowID3.SetText (None)

			spellBtn1.SetBAM (cont["Spell1Icon"][:-1] + "b", 0, 0)
			if cont["Resource2"] != "":
				spellBtn2.SetBAM (cont["Spell2Icon"][:-1] + "b", 0, 0)
			if cont["Resource3"] != "":
				spellBtn3.SetBAM (cont["Spell3Icon"][:-1] + "b", 0, 0)

			# warning: can take out too many
			handBtn.OnPress (lambda: GemRB.DispelEffect (pc, "CastSpellOnCondition", cont["Param2"]))
		else:
			rowID1.SetText (None)
			rowID2.SetText (None)
			rowID3.SetText (None)
			spellBtn1.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_SET)
			spellBtn1.SetState (IE_GUI_BUTTON_DISABLED)
			spellBtn2.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_SET)
			spellBtn2.SetState (IE_GUI_BUTTON_DISABLED)
			spellBtn3.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_SET)
			spellBtn3.SetState (IE_GUI_BUTTON_DISABLED)
			handBtn.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_OR)


###################################################
# End of file GUIMG.py
