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
# character generation end
import BGCommon
import GemRB
import GameCheck
import GUICommon
import Spellbook
import CommonTables
import LUCommon
from GUIDefines import *
from ie_slots import *
from ie_stats import *
from ie_spells import *

def OnLoad():
	# Lay on hands, turn undead and backstab multiplier get set by the core
	# set my character up
	MyChar = GemRB.GetVar ("Slot")

	#reputation
	BGCommon.SetReputation ()

	# don't reset most stats of imported chars
	if GemRB.GetVar ("ImportedChar"):
		CustomizeChar (MyChar)
		RunGame (MyChar)
		return

	ClassName = GUICommon.GetClassRowName (MyChar)

	# weapon proficiencies
	# set the base number of attacks; effects will add the proficiency bonus
	# 2 means 1 attack, because this is the number of attacks in 2 rounds
	GemRB.SetPlayerStat (MyChar, IE_NUMBEROFATTACKS, 2)

	#lore, thac0, hp, and saves
	GemRB.SetPlayerStat (MyChar, IE_MAXHITPOINTS, 0)
	GemRB.SetPlayerStat (MyChar, IE_HITPOINTS, 0)
	LUCommon.SetupSavingThrows (MyChar)
	LUCommon.SetupThaco (MyChar)
	LUCommon.SetupLore (MyChar)
	LUCommon.SetupHP (MyChar)

	# apply class/kit abilities
	GUICommon.ResolveClassAbilities (MyChar, ClassName)

	# alignment based bhaal powers are added in FixInnates later
	# unless we're starting with SoA
	if GemRB.GetVar("oldgame") == 1:
		AlignmentAbbrev = CommonTables.Aligns.FindValue ("VALUE", GemRB.GetPlayerStat (MyChar, IE_ALIGNMENT))
		GUICommon.AddClassAbilities (MyChar, "abstart", 6,6, AlignmentAbbrev)

	# setup starting gold (uses a roll dictated by class
	TmpTable = GemRB.LoadTable ("strtgold")
	temp = GemRB.Roll (TmpTable.GetValue (ClassName, "ROLLS"), TmpTable.GetValue (ClassName, "SIDES"), TmpTable.GetValue (ClassName, "MODIFIER"))
	GemRB.SetPlayerStat (MyChar, IE_GOLD, temp * TmpTable.GetValue (ClassName, "MULTIPLIER"))

	GemRB.SetPlayerStat (MyChar, IE_EA, 2 )

	CustomizeChar (MyChar)
	RunGame (MyChar)

def CustomizeChar(MyChar):
	# save the name and starting xp (can level right away in game)
	GemRB.SetPlayerName (MyChar, GemRB.GetToken ("CHARNAME"), 0)

	# does all the rest
	LargePortrait = GemRB.GetToken ("LargePortrait")
	SmallPortrait = GemRB.GetToken ("SmallPortrait")
	GemRB.FillPlayerInfo (MyChar, LargePortrait, SmallPortrait)

	# biography
	Bio = GemRB.GetToken("BIO")
	BioStrRef = 33347
	if Bio:
		BioStrRef = 62016
		GemRB.CreateString (BioStrRef, Bio)
	GemRB.SetPlayerString (MyChar, 74, BioStrRef)

def RunGame(MyChar):
	if GameCheck.IsTOB():
		# will also add the starting inventory for tob
		GemRB.GameSetExpansion (4)

	playmode = GemRB.GetVar ("PlayMode")
	if playmode is not None:
		GemRB.SaveCharacter (MyChar, "gembak")
		#LETS PLAY!!
		import CharGenCommon, CommonWindow
		CharGenCommon.CharGenWindow.Close ()

		# avoid briefly showing the GUI before the bg2 intro cutscene starts
		# however BGT and TUTU users can start bg1 through the same code and
		# that definitely should not hide the GUI
		if not GameCheck.IsBGT ("bg1"):
			CommonWindow.SetGameGUIHidden (True)

		GemRB.EnterGame()
		if GameCheck.IsTOB () and Spellbook.HasSorcererBook (MyChar):
			# delay for sorcerers, since their class pcf needs to run first to set up their spellbook properly
			GemRB.SetTimedEvent (lambda: GemRB.ChargeSpells (MyChar), 1)
		else:
			GemRB.SetTimedEvent (lambda: GemRB.ExecuteString ("EquipMostDamagingMelee()", MyChar), 1)
	else:
		#when export is done, go to start
		if GameCheck.HasTOB():
			GemRB.SetToken ("NextScript","Start2")
		else:
			GemRB.SetToken ("NextScript","Start")
		GemRB.SetNextScript ("ExportFile") #export

def GiveEquipment(MyChar, ClassName, KitIndex):
		# get the kit (or use class if no kit) to load the start table
		if KitIndex == 0:
			EquipmentColName = ClassName
			# sorcerers are missing from the table, use the mage equipment instead
			if EquipmentColName == "SORCERER":
				EquipmentColName = "MAGE"
		else:
			EquipmentColName = CommonTables.KitList.GetValue (KitIndex, 0)

		EquipmentTable = GemRB.LoadTable ("25stweap")

		# a map of slots in the table to the real slots
		# SLOT_BAG is invalid, so use the inventory (first occurrence)
		# SLOT_INVENTORY: use -1 instead, that's what CreateItem expects
		RealSlots = [ SLOT_ARMOUR, SLOT_SHIELD, SLOT_HELM, -1, SLOT_RING, \
					SLOT_RING, SLOT_CLOAK, SLOT_BOOT, SLOT_AMULET, SLOT_GLOVE, \
					SLOT_BELT, SLOT_QUIVER, SLOT_QUIVER, SLOT_QUIVER, \
					SLOT_ITEM, SLOT_ITEM, SLOT_ITEM, SLOT_WEAPON, SLOT_WEAPON, SLOT_WEAPON ]
		inventory_exclusion = 0

		#loop over rows - item slots
		for slot in range(0, EquipmentTable.GetRowCount ()):
			slotname = EquipmentTable.GetRowName (slot)
			item_resref = EquipmentTable.GetValue (slotname, EquipmentColName)

			# no item - go to next
			if item_resref == "*":
				continue

			# the table has typos for kitted bard's armor
			if item_resref == "LEATH14":
				item_resref = "LEAT14"

			# get empty slots of the requested type
			realslot = GemRB.GetSlots (MyChar, RealSlots[slot], -1)
			if RealSlots[slot] == SLOT_WEAPON:
				# exclude the shield slot, so the offhand stays empty
				realslot = realslot[1:]

			if realslot == (): # fallback to the inventory
				realslot = GemRB.GetSlots (MyChar, -1, -1)

			if realslot == (): # this shouldn't happen!
				print("Eeek! No free slots for", item_resref)
				continue

			# if an item contains a comma, the rest of the value is the stack
			if "," in item_resref:
				item_resref = item_resref.split(",")
				count = int(item_resref[1])
				item_resref = item_resref[0]
			else:
				count = 0

			# skip items already there (imported chars)
			if GemRB.FindItem (MyChar, item_resref) != -1:
				continue

			targetslot = realslot[0]
			SlotType = GemRB.GetSlotType (targetslot, MyChar)
			i = 1
			item = GemRB.GetItem (item_resref)
			if not item:
				print("Error: could not find item", item_resref, "that is listed in 25stweap.2da")
				continue

			if inventory_exclusion & item['Exclusion']:
				# oops, too many magic items to equip, so just dump it to the inventory
				targetslot = GemRB.GetSlots (MyChar, -1, -1)[0]
				SlotType = -1
			else:
				# if there are no free slots, CreateItem will create the item on the ground
				while not GemRB.CanUseItemType (SlotType["Type"], item_resref, MyChar) \
				and i < len(realslot):
					targetslot = realslot[i]
					SlotType = GemRB.GetSlotType (targetslot, MyChar)
					i = i + 1

			GemRB.CreateItem(MyChar, item_resref, targetslot, count, 0, 0)
			GemRB.ChangeItemFlag (MyChar, targetslot, IE_INV_ITEM_IDENTIFIED, OP_OR)
			inventory_exclusion |= item['Exclusion']

		return
