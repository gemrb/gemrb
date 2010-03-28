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
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
#
#
import GemRB
import LoadScreen
global LoadScreenWindow
from LoadScreen import * 

def OnLoad():
	print "Testing basic guiscript functions"
	#case insensitive on token key, but case sensitive on value
	print "1. Testing Set/GetToken"
	GemRB.SetToken("CHARname","Avenger")
	print "Avenger == ",GemRB.GetToken("charNAME")
	#case insensitive on key
	print "2. Testing SetVar & GetVar"
	GemRB.SetVar("Strref On",1)
	print "Strref On was turned to ",GemRB.GetVar("STRREF ON")
	#actually this breaks the script atm
	#print "3. Testing PlaySound - non-existent resource"
	#GemRB.PlaySound("nothere")
	print "4. Testing PlaySound, uncompressed wav"
	GemRB.PlaySound("alivad12")
	print "5. Testing PlaySound, compressed wav"
	GemRB.PlaySound("compress")
	print "6. Enabling cheatkeys"
	GemRB.EnableCheatKeys(1)

	# for loadscreen testing
	#StartLoadScreen()

	# for store testing
	#GemRB.EnterStore("bag01")
	#GemRB.SetNextScript("GUISTORE")
	#return

	# for character sets
	GemRB.SetNextScript("CharSet")
	#return
