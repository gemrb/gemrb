# -*-python-*-
# vim: set ts=4 sw=4 expandtab:
# GemRB - Infinity Engine Emulator
# Copyright (C) 2014 The GemRB Project
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

import GameCheck

def CreateScrollBar(func):
	def wrapper(win, control, *args):
		if len(args) < 6:
			# append the BAM resref
			if GameCheck.IsBG2():
				args += ('GUISCRCW',)
			elif GameCheck.IsPST():
				args += ('CGSCRL1',)
			else:
				# this resource is in almost all games
				# but is not the main scrollbar except in BG1
				# probably we dont care about creating scrollbars elsewhere
				args += ('GUIWSBR',)

		if GameCheck.IsBG2():
			args += (0,0,1,2,3,5,4)
		else:
			args += tuple([0] + range(6))
		return func(win, control, *args)
	return wrapper
