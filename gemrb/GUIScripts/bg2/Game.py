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

###################################################

import GemRB
import GUICommon
import CommonTables
import MessageWindow
import LUCommon

from GameCheck import MAX_PARTY_SIZE
from GUIDefines import *
from CharGenEnd import GiveEquipment
from ie_stats import *

RemoveColor = {'r' : 0xf5, 'g' : 0xf5, 'b' : 0x96, 'a' : 255}

def EnterGame():
	GemRB.GameSetPartySize(MAX_PARTY_SIZE)
	GemRB.GameSetProtagonistMode(1)

	MessageWindow.OnLoad()

def RemoveYoshimo( idx):
	GemRB.DisplayString(72046, RemoveColor)
	#WARNING:multiple strings are executed in reverse order
	GemRB.ExecuteString('ApplySpellRES("destself",myself)', idx)
	GemRB.ExecuteString('GivePartyAllEquipment()', idx)
	return

def RemoveImoen( idx):
	GemRB.DisplayString(72047, RemoveColor)
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

	# fresh tob game? Grant all the equipment if we're not importing a pc (they get a poorer bag, but only if low-level enough)
	if not (GemRB.GetVar ("oldgame") or GemRB.GetVar ("ImportedChar")):
		GiveEquipment (idx, ClassName, KitIndex)
		FixInnates (idx)
		return

	# prepare to adjust the XP to the minimum if it is lower than what new characters start with
	# but don't let dualclassed characters get more xp than the rest
	XP = GemRB.GetPlayerStat (idx, IE_XP)
	XP2 = CommonTables.ClassSkills.GetValue (ClassName, "STARTXP2", GTV_INT)
	Dual = GUICommon.IsDualClassed (idx, True)
	if Dual[0] > 0:
		OldLevel = GemRB.GetPlayerStat (idx, IE_LEVEL)
		if GUICommon.IsDualSwap (idx):
			OldLevel = GemRB.GetPlayerStat (idx, IE_LEVEL2)

		OldClassIndex = Dual[1]
		if Dual[0] == 1:
			# was not a class before
			KittedClass = CommonTables.KitList.GetValue (Dual[1], 7)
			OldClassIndex = CommonTables.Classes.FindValue ("ID", KittedClass)
		ClassName = GUICommon.GetClassRowName (OldClassIndex, "index")
		OldXP = LUCommon.GetNextLevelExp (OldLevel, ClassName)
		XP += OldXP

	# only give a few items for transitions from soa or if we're upgrading an imported character
	# give the Amulet of Seldarine to the pc's first empty inventory slot
	invslot = GemRB.GetSlots (idx, -1, -1)
	GemRB.CreateItem (idx, "AMUL27", invslot[0], 1, 0, 0)
	GemRB.ChangeItemFlag (idx, invslot[0], IE_INV_ITEM_IDENTIFIED, OP_OR)

	# adjust the XP and set a hardcoded difficulty variable to the difference
	if XP <= XP2:
		GemRB.SetPlayerStat (idx, IE_XP, XP2)
		GemRB.SetGlobal ("XPGIVEN", "GLOBAL", XP2 - XP)
		# extra bag of goodies to go along with all that learning
		# bag19b-bag19e are unused and identical
		if GemRB.GetVar ("ImportedChar"):
			GemRB.CreateItem (idx, "BAG19A", invslot[1], 1, 0, 0)
		else:
			GemRB.CreateItem (idx, "BAG19", invslot[1], 1, 0, 0)
		GemRB.ChangeItemFlag (idx, invslot[1], IE_INV_ITEM_IDENTIFIED, OP_OR)

	FixFamiliar (idx)
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

	# old singleplayer soa or bgt soa/tutorial hybrid
	# bgt reuses the tutorial for its soa mode (playmode==0 is bg1)
	if version < 5 and (GemRB.GetVar ("PlayMode") == 0 or GameCheck.IsBGT ("soa")) and GemRB.GetVar ("oldgame"):
		#upgrade SoA to ToB/SoA
		if GemRB.GameSetExpansion(4):
			GemRB.AddNewArea("xnewarea")
		return

	if not GemRB.GameSetExpansion(5):
		return

	#upgrade to ToB only
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
		name = GemRB.GetPlayerName(idx, 2).lower() #scripting name
		# change the override script to the new one
		if name != "none":
			newScript = PDialogTable.GetValue (name.upper(), "25OVERRIDE_SCRIPT_FILE")
			newDialog = PDialogTable.GetValue (name.upper(), "25JOIN_DIALOG_FILE")
			GemRB.SetPlayerScript (idx, newScript, 0) # 0 is SCR_OVERRIDE, the override script slot
			GemRB.SetPlayerDialog (idx, newDialog)
		
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
