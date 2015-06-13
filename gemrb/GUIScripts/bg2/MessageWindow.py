# -*-python-*-
# GemRB - Infinity Engine Emulator
# Copyright (C) 2003-2005 The GemRB Project
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

# MessageWindow.py - scripts and GUI for the main (walk) window

###################################################

import GemRB
import GameCheck
import GUICommon
import GUICommonWindows
import CommonWindow
import GUIClasses
import CommonTables
from GameCheck import PARTY_SIZE
from GUIDefines import *
from CharGenEnd import GiveEquipment

MessageWindow = 0
PortraitWindow = 0
OptionsWindow = 0
ExpandButton = 0
ContractButton = 0
TMessageTA = 0 # for dialog code

def OnLoad():
	global PortraitWindow, OptionsWindow

	GemRB.GameSetPartySize(PARTY_SIZE)
	GemRB.GameSetProtagonistMode(1)
	GemRB.LoadWindowPack(GUICommon.GetWindowPack())

	GUICommonWindows.PortraitWindow = None
	GUICommonWindows.ActionsWindow = None
	GUICommonWindows.OptionsWindow = None

	OptionsWindow = GemRB.LoadWindow(0)
	GUICommonWindows.SetupMenuWindowControls (OptionsWindow, 1, None)
	PortraitWindow = GUICommonWindows.OpenPortraitWindow(1)

	# 1280 and higher don't have this control
	Button = OptionsWindow.GetControl (10)
	if Button:
		Button = OptionsWindow.GetControl (10)
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, MinimizeOptions)
		Button = PortraitWindow.GetControl (8)
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, GUICommonWindows.MinimizePortraits)

	ActionsWindow = GemRB.LoadWindow(3)
	GUICommonWindows.OpenActionsWindowControls (ActionsWindow)
	
	Button = ActionsWindow.GetControl(60)
	if Button:
		Button.SetEvent(IE_GUI_BUTTON_ON_PRESS, MaximizeOptions)
		Button=ActionsWindow.GetControl(61)
		Button.SetEvent(IE_GUI_BUTTON_ON_PRESS, MaximizePortraits)

	GemRB.SetVar("PortraitWindow", PortraitWindow.ID)
	GemRB.SetVar("ActionsWindow", ActionsWindow.ID)
	GemRB.SetVar("OptionsWindow", OptionsWindow.ID)
	GemRB.SetVar("TopWindow", -1)
	GemRB.SetVar("OtherWindow", -1)
	GemRB.SetVar("FloatWindow", -1)
	GemRB.SetVar("PortraitPosition", 2) #Right
	GemRB.SetVar("ActionsPosition", 4) #BottomAdded
	GemRB.SetVar("OptionsPosition", 0) #Left
	GemRB.SetVar("MessagePosition", 4) #BottomAdded
	GemRB.SetVar("OtherPosition", 5) #Inactivating
	GemRB.SetVar("TopPosition", 5) #Inactivating
	
	UpdateControlStatus()
	
def MinimizeOptions():
	GemRB.GameSetScreenFlags(GS_OPTIONPANE, OP_OR)

def MaximizeOptions():
	GemRB.GameSetScreenFlags(GS_OPTIONPANE, OP_NAND)

# MinimizePortraits is in GUICommonWindows for dependency reasons

def MaximizePortraits():
	GemRB.GameSetScreenFlags(GS_PORTRAITPANE, OP_NAND)

def UpdateControlStatus():
	global MessageWindow, ExpandButton, ContractButton, TMessageTA
	
	TMessageWindow = 0
	TMessageTA = 0
	GSFlags = GemRB.GetMessageWindowSize()
	Expand = GSFlags&GS_DIALOGMASK
	Override = GSFlags&GS_DIALOG
	GSFlags = GSFlags-Expand

	#a dialogue is running, setting messagewindow size to maximum
	if Override:
		Expand = GS_LARGEDIALOG

	GemRB.LoadWindowPack(GUICommon.GetWindowPack())

	if Expand == GS_MEDIUMDIALOG:
		TMessageWindow = GemRB.LoadWindow(12)
		TMessageTA = TMessageWindow.GetControl(1)
		ExpandButton = TMessageWindow.GetControl(0)
		ExpandButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, CommonWindow.OnIncreaseSize)
		ContractButton = TMessageWindow.GetControl(3)
		ContractButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, CommonWindow.OnDecreaseSize)

	elif Expand == GS_LARGEDIALOG:
		TMessageWindow = GemRB.LoadWindow(7)
		TMessageTA = TMessageWindow.GetControl(1)
		ContractButton = TMessageWindow.GetControl(0)
		ContractButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, CommonWindow.OnDecreaseSize)
	else:
		TMessageWindow = GemRB.LoadWindow(4)
		TMessageTA = TMessageWindow.GetControl(3)
		ExpandButton = TMessageWindow.GetControl(2)
		ExpandButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, CommonWindow.OnIncreaseSize)

	hideflag = GemRB.HideGUI()
	MessageWindow = GemRB.GetVar("MessageWindow")
	MessageTA = GUIClasses.GTextArea(ID=GemRB.GetVar("MessageTextArea"))
	if MessageWindow > 0 and MessageWindow != TMessageWindow.ID:
		TMessageTA = MessageTA.SubstituteForControl(TMessageTA)
		GUIClasses.GWindow(MessageWindow).Unload()

	TMessageTA.SetFlags(IE_GUI_TEXTAREA_AUTOSCROLL|IE_GUI_TEXTAREA_HISTORY)
	GemRB.SetVar("MessageWindow", TMessageWindow.ID)
	GemRB.SetVar("MessageTextArea", TMessageTA.ID)
	if Override:
		TMessageTA.SetStatus (IE_GUI_CONTROL_FOCUSED)
	else:
		GUICommon.GameControl.SetStatus(IE_GUI_CONTROL_FOCUSED)

	if GSFlags & GS_OPTIONPANE:
		GemRB.SetVar("OptionsWindow", -1)
	else:
		GemRB.SetVar("OptionsWindow", OptionsWindow.ID)

	if GSFlags & GS_PORTRAITPANE:
		GemRB.SetVar("PortraitWindow", -1)
	else:
		GemRB.SetVar("PortraitWindow", PortraitWindow.ID)

	if hideflag:
		GemRB.UnhideGUI()
	return

def RemoveYoshimo( idx):
	GemRB.DisplayString(72046, 0xF5F596)
	#WARNING:multiple strings are executed in reverse order
	GemRB.ExecuteString('ApplySpellRES("destself",myself)', idx)
	GemRB.ExecuteString('GivePartyAllEquipment()', idx)
	return

def RemoveImoen( idx):
	GemRB.DisplayString(72047, 0xF5F596)
	GemRB.ExecuteString('ApplySpellRES("destself",myself)', idx)
	GemRB.ExecuteString('GivePartyAllEquipment()', idx)
	return

def FixEdwin( idx):
	GemRB.ApplySpell(idx, "SPIN661")
	return

def FixAnomen( idx):
	#lawful neutral
	if (GemRB.GetPlayerStat(idx, IE_ALIGNMENT) == 0x12):
		GemRB.ApplySpell(idx, "SPIN678")
	return

#do all the stuff not done yet
def FixProtagonist( idx):
	ClassName = GUICommon.GetClassRowName (idx)
	KitIndex = GUICommon.GetKitIndex (idx)
	# only give a few items for transitions from soa
	if GemRB.GetVar("oldgame"):
		# give the Amulet of Seldarine to the pc's first empty inventory slot
		invslot = GemRB.GetSlots (idx, -1, -1)[0]
		GemRB.CreateItem(idx, "AMUL27", invslot, 1, 0, 0)
		GemRB.ChangeItemFlag (idx, invslot, IE_INV_ITEM_IDENTIFIED, OP_OR)
		# TODO: give bag19/bag19a (.../19e?) based on the experience
		invslot = GemRB.GetSlots (idx, -1, -1)[0]
		GemRB.CreateItem(idx, "BAG19A", invslot, 1, 0, 0)
		GemRB.ChangeItemFlag (idx, invslot, IE_INV_ITEM_IDENTIFIED, OP_OR)

		# adjust the XP to the minimum if it is lower and set a difficulty variable to the difference
		xp = GemRB.GetPlayerStat(idx, IE_XP)
		# FIXME: not fair for disabled dual-classes (should take the inactive level xp into account)
		xp2 = CommonTables.ClassSkills.GetValue (ClassName, "STARTXP2")
		if xp2 > xp:
			GemRB.SetPlayerStat(idx, IE_XP, xp2)
			GemRB.SetGlobal("XPGIVEN","GLOBAL", xp2-xp)

		FixFamiliar (idx)
		FixInnates (idx)
	else:
		GiveEquipment(idx, ClassName, KitIndex)
		FixInnates (idx)
	return

# replace the familiar with the improved version
# NOTE: fx_familiar_marker destroys the old one and creates a new one (as proper actors)
def FixFamiliar(pc):
	# if the critter is outside, summoned, the effect will upgrade it (fx_familiar_marker runs on it)
	# if the critter is packed in your inventory, it will be upgraded as soon as it gets released
	# after picking it up again, also the inventory item will be new
	pass

# replace bhaal powers with the improved versions
# or add the new ones, since soa transitioners lost them in hell
# abstart.2da was not updated for tob
def FixInnates(pc):
	import Spellbook
	from ie_spells import LS_MEMO
	from ie_stats import IE_ALIGNMENT
	# adds the spell: SPIN822 (slayer change) if needed
	if Spellbook.HasSpell (pc, IE_SPELL_TYPE_INNATE, 1, "SPIN822") == -1:
		GemRB.LearnSpell (pc, "SPIN822", LS_MEMO)

	# apply starting (alignment dictated) abilities (just to be replaced later)
	# pc, table, new level, level diff, alignment
	AlignmentAbbrev = CommonTables.Aligns.FindValue ("VALUE", GemRB.GetPlayerStat (pc, IE_ALIGNMENT))
	GUICommon.AddClassAbilities (pc, "abstart", 6,6, AlignmentAbbrev)

	# some old/new pairs are the same, so we can skip them
	# all the innates are doubled
	old = [ "SPIN101", "SPIN102", "SPIN104", "SPIN105" ]
	new = [ "SPIN200", "SPIN201", "SPIN202", "SPIN203" ]
	for i in range(len(old)):
		if GemRB.RemoveSpell (pc, old[i]):
			Spellbook.LearnSpell (pc, new[i], IE_SPELL_TYPE_INNATE, 3, 2, LS_MEMO)

#upgrade savegame to next version
def GameExpansion():

	version = GemRB.GameGetExpansion()
	if version<3:
		GemRB.GameSetReputation(100)

	if not GameCheck.HasTOB():
		return

	if version < 5 and not GemRB.GetVar("PlayMode") and GemRB.GetVar("oldgame"):
		#upgrade SoA to ToB/SoA
		if GemRB.GameSetExpansion(4):
			GemRB.AddNewArea("xnewarea")
		return

	if not GemRB.GameSetExpansion(5):
		return

	#upgrade to ToB only
	GemRB.SetVar ("SaveDir", 1)
	GemRB.SetMasterScript("BALDUR25","WORLDM25")
	GemRB.SetGlobal("INTOB","GLOBAL",1)
	GemRB.SetGlobal("HADELLESIMEDREAM1","GLOBAL", 1)
	GemRB.SetGlobal("HADELLESIMEDREAM2","GLOBAL", 1)
	GemRB.SetGlobal("HADIMOENDREAM1","GLOBAL", 1)
	GemRB.SetGlobal("HADSLAYERDREAM","GLOBAL", 1)
	GemRB.SetGlobal("HADJONDREAM1","GLOBAL", 1)
	GemRB.SetGlobal("HADJONDREAM2","GLOBAL", 1)
	idx = GemRB.GetPartySize()
	PDialogTable = GemRB.LoadTable ("pdialog")
	
	while idx:
		name = GemRB.GetPlayerName(idx, 2) #scripting name
		# change the override script to the new one
		if name != "none":
			newScript = PDialogTable.GetValue (name.upper(), "25OVERRIDE_SCRIPT_FILE")
			newDialog = PDialogTable.GetValue (name.upper(), "25JOIN_DIALOG_FILE")
			SetPlayerScript (idx, newScript, 0) # 0 is SCR_OVERRIDE, the override script slot
			SetPlayerDialog (idx, newDialog)
		
			if name == "yoshimo":
				RemoveYoshimo(idx)
			elif name == "imoen":
				RemoveImoen(idx)
			elif name == "edwin":
				FixEdwin(idx)
			elif name == "anomen":
				FixAnomen(idx)
		else:
			FixProtagonist(idx)
			GemRB.GameSelectPC (idx, True, SELECT_REPLACE)
		idx=idx-1
	return
