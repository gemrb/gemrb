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

# Actor.py: Contains the actor class.

import GemRB
import GUICommon
from GUIDefines import *
from ie_stats import *
#this import is primarily for the tables
import CommonTables

##############################################################################
## GLOBALS TO BE INITIALIZED ONCE
##############################################################################
dualswap = None
classcount = None
levelslots = [IE_LEVEL, IE_LEVEL2, IE_LEVEL3]

class Actor:
	"""Holds information of a PC."""

	def __init__ (self, pc):
		"""Load up basic information."""

		#setup our basic Actor
		self.Reset (pc)

		#setup globals if they are blank
		if dualswap == None:
			self.__setup_globals ()

	def __setup_globals (self):
		"""Initializes all globals used for quick referencing.

		Will only be called by the first Actor created."""

		global classcount, dualswap
		classcount = CommonTables.Classes.GetRowCount ()
		dualswap = [0]*classcount

		for i in range(classcount):
			rowname = CommonTables.Classes.GetRowName(i)
			classid = CommonTables.Classes.GetValue (rowname, "ID")
			classnames = rowname.split("_")

			#set the MC_WAS_ID of the first class
			if len(classnames) == 2:
				dualswap[classid-1] = CommonTables.Classes.GetValue (rowname, "MC_WAS_ID")

	def Classes (self):
		"""Returns a list with all the class IDs."""

		if self.__classes == None:
			#already reversed in ClassNames
			self.__classes = [CommonTables.Classes.GetValue (name, "ID", 1) for name in self.ClassNames()]
		return self.__classes

	def ClassNames (self):
		"""Returns a list will all the class names."""

		if self.__classnames == None:
			self.__classnames = GUICommon.GetClassRowName (self.classid, "class").split("_")
			if self.IsDualSwap():
				self.__classnames.reverse()
		return self.__classnames

	def ClassTitle (self):
		"""Returns the class title as a displayable string."""

		if self.__classtitle != None:
			return self.__classtitle

		self.__classtitle = GemRB.GetPlayerStat (self.pc, IE_TITLE1)

		if self.__classtitle == 0:
			if self.multiclass and self.isdual == 0:
				self.__classtitle = CommonTables.Classes.GetValue ("_".join(self.__classnames), "CAP_REF")
				self.__classtitle = GemRB.GetString (self.__classtitle)
			elif self.isdual:
				# first (previous) kit or class of the dual class
				self.Classes()
				self.ClassNames()
				if self.KitIndex():
					self.__classtitle = CommonTables.KitList.GetValue (self.__kitindex, 2)
				else:
					self.__classtitle = CommonTables.Classes.GetValue (self.__classnames[1], "CAP_REF")
				self.__classtitle = GemRB.GetString (self.__classtitle) + " / " + \
					GemRB.GetString (CommonTables.Classes.GetValue (self.__classnames[0], "CAP_REF") )
			else: # ordinary class or kit
				if self.KitIndex():
					self.__classtitle = CommonTables.KitList.GetValue (self.__kitindex, 2)
				else:
					self.__classtitle = CommonTables.Classes.GetValue ("_".join(self.__classnames), "CAP_REF")
				self.__classtitle = GemRB.GetString (self.__classtitle)

		if self.__classtitle == "*":
			self.__classtitle = 0
		return self.__classtitle

	def IsDualSwap (self):
		"""Returns true if IE_LEVEL is opposite of expectations."""

		if self.__dualswap == None:
			self.__dualswap = (self.isdual & CommonTables.Classes.GetValue \
				(self.ClassNames()[0], "MC_WAS_ID", 1)) > 0
		return self.__dualswap

	def KitIndex (self):
		"""Returns the kit index in relation to kitlist.2da."""

		if self.__kitindex != None:
			return self.__kitindex

		Kit = GemRB.GetPlayerStat (self.pc, IE_KIT)
		self.__kitindex = 0

		if Kit & 0xc000 == 0x4000:
			self.__kitindex = Kit & 0xfff

		# carefully looking for kit by the usability flag
		# since the barbarian kit id clashes with the no-kit value
		if self.__kitindex == 0 and Kit != 0x4000:
			self.__kitindex = CommonTables.KitList.FindValue (6, Kit)
			if self.__kitindex == -1:
				self.__kitindex = 0

		return self.__kitindex

	def LevelDiffs (self):
		"""Returns the differences between the current and next classes."""
		return [(next-current) for current,next in zip(self.Levels(),
			self.NextLevels())]

	def Levels (self):
		"""Returns the current level of each class."""
		if self.__levels == None:
			self.__levels = [level for slot in levelslots for level \
				in [GemRB.GetPlayerStat (self.pc, slot)] if level>0]
			if self.IsDualSwap():
				self.__levels.reverse()
		return self.__levels

	def NextLevelExp (self):
		"""Returns the experience required to level each class."""

		#filtering the old dual class out seems unnecessary
		#just be sure to use NumClasses() or isdual to check
		return [CommonTables.NextLevel.GetValue (name, str(level+1)) for name,level \
			in zip(self.ClassNames(), self.Levels())]

	def NextLevels (self):
		"""Returns the next level for each class."""

		if self.__nextlevels != None:
			return self.__nextlevels

		xp = GemRB.GetPlayerStat (self.pc, IE_XP) / self.NumClasses()

		self.__nextlevels = []
		for name, level in zip(self.ClassNames(), self.Levels() ):
			next = level

			#we only want the current level for the old part of a dual-class
			if len(self.__nextlevels) < self.__numclasses:
				for current in range(level+1, CommonTables.NextLevel.GetColumnCount () ):
					if CommonTables.NextLevel.GetValue (name, str(current)) <= xp:
						next = current
					else:
						break
			self.__nextlevels.append(next)

		return self.__nextlevels
	
	def NumClasses (self):
		"""Returns the number of *active* classes."""
		if self.__numclasses == None:
			if self.isdual:
				self.__numclasses = 1
			else:
				self.__numclasses = len(self.ClassNames() )
		return self.__numclasses


	def RaceName (self):
		"""Returns the race string."""
		pass

	def Reset (self, pc):
		"""Resets all internal variables.

		This should be called after any fundemental changes to the pc.
		This includes: dualclassing, leveling."""

		#accessible variables
		self.pc = pc
		self.classid = GemRB.GetPlayerStat (self.pc, IE_CLASS)
		self.isdual = GemRB.GetPlayerStat (self.pc, IE_MC_FLAGS) & MC_WAS_ANY_CLASS
		self.multiclass = CommonTables.Classes.GetValue (GUICommon.GetClassRowName (pc), "MULTI")

		#internal variables - these are only intialized on the first
		#call to their respective function, and stored thereafter
		self.__classes = None
		self.__classnames = None
		self.__classtitle = None
		self.__dualswap = None
		self.__kitindex = None
		self.__levels = None
		self.__nextlevels = None
		self.__numclasses = None
