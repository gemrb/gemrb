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
def OnLoad():
	GemRB.LoadWindowPack ("testing")
	Window = GemRB.LoadWindow (0)
	start = 64
	end = 255
	for i in range(end-start):
		x = i & 15
		y = i >> 4
		GemRB.CreateButton (Window, i, x*40+10, y*40+10, 40, 40)
		Button = GemRB.GetControl (Window, i)
		GemRB.SetText (Window, i, chr(i+start) )
		#GemRB.SetButtonSprites (Window, i, "NORMAL",i+start,0,0,0,0)
		#GemRB.SetButtonBAM (Window, i, "NORMAL",i+start,0,0xf)
		GemRB.SetTooltip (Window, i, str(i+start) )
	GemRB.SetVisible (Window, 1)
