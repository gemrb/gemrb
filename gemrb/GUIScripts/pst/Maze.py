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

# Maze.py - script to generate the modron maze in PST

###################################################

import GemRB
from maze_defs import *

rooms = None
max = 0
dims = 0
entries = None

def Possible(posx, posy):
	global entries

	pos = posy*MAZE_MAX_DIM+posx

	if entries[pos] == 2:
		return -1
	return pos

def GetPossible (pos):
	posy = pos/MAZE_MAX_DIM
	posx = pos-posy*MAZE_MAX_DIM
	possible = []

	if posx>0:
		newpos = Possible(posx-1, posy)
		if newpos>0:
			possible[:0] = [newpos]
	if posy>0:
		newpos = Possible(posx, posy-1)
		if newpos>0:
			possible[:0] = [newpos]
	if posx<MAZE_MAX_DIM-1:
		newpos = Possible(posx+1, posy)
		if newpos>0:
			possible[:0] = [newpos]
	if posy<MAZE_MAX_DIM-1:
		newpos = Possible(posx, posy+1)
		if newpos>0:
			possible[:0] = [newpos]

	return possible

#loads a 2da and sets it up as maze
def LoadMazeFrom2da(tablename):
	MazeTable = GemRB.LoadTable(tablename)
	if MazeTable == None:
		return
	size = MazeTable.GetValue(-1,-1)
	GemRB.SetupMaze(size, size)
	entries = zeros(MAZE_ENTRY_COUNT)
	traps = 0
	for i in entries:
		Area = MazeTable.GetRowName(i)
		FIELD_0 = MazeTable.GetValue(Area,"FIELD_0")
		TRAPTYPE = MazeTable.GetValue(Area,"TRAPTYPE")
		WALLS = MazeTable.GetValue(Area,"WALLS")
		FIELD_16 = MazeTable.GetValue(Area,"FIELD_16")
		pos = int(Area[3:])-1
		print Area,":", pos
		GemRB.SetMazeEntry(pos, ME_0, FIELD_0)
		GemRB.SetMazeEntry(pos, ME_TRAP, TRAPTYPE)
		GemRB.SetMazeEntry(pos, ME_WALLS, WALLS)
		GemRB.SetMazeEntry(pos, ME_16, FIELD_16)
		if TRAPTYPE>=0:
			traps = traps+1

	GemRB.SetMazeData(MH_TRAPCOUNT, traps)
	GemRB.SetMazeData(MH_INITED, 1)
	return

def AddRoom (pos):
	global rooms
	global entries

	rooms[:0]=[pos]
	entries[pos] = 1
	return

def MainRoomFits (pos2x, pos2y, pos):
	global entries

	south = pos2y*MAZE_MAX_DIM+pos2x

	if south==pos:
		return False

	north = (pos2y+1)*MAZE_MAX_DIM+pos2x
	if north==pos:
		return False

	GemRB.SetMazeEntry(south, ME_WALLS, WALL_NORTH)
	entries[north]=2
	AddRoom(south)
	return True

def zeros (size):
	return size*[0]

def PrintMaze():
	header = GemRB.GetMazeHeader()
	if header==None or header["Inited"]==0:
		print "There is maze or it is not initialized!"
		return

	MazeX = header["XSize"]
	MazeY = header["YSize"]
	MainX = header["Pos1X"]
	MainY = header["Pos1Y"]
	NordomX = header["Pos2X"]
	NordomY = header["Pos2Y"]
	for y in range (MazeY):
		for x in range (MazeX):
			pos = 8*y+x
			entry = GemRB.GetMazeEntry(pos)
			if entry["Special"]:
				if x == NordomX and y == NordomY:
					str = str + "N"
				elif x == MainX and y == MainY:
					str = str + "M"
				else:
					str = str + "!"
			else:
				if entry["Visited"]:
					str = str + " "
				else:
					str = str + "?"
		print str
	return

###################################################
def CreateMaze ():
	global max
	global dims
	global entries
	global rooms

	if GemRB.GetGameVar("EnginInMaze")>0:
		LoadMazeFrom2da("easymaze")
		GemBR.UnloadTable("easymaze")
		return

	mazedifficulty = GemRB.GetGameVar("MazeDifficulty")

	#make sure there are no more traps than rooms
	#make sure dimensions don't exceed maximum possible
	if mazedifficulty==0:
		dims = 3
		traps = 5
	elif mazedifficulty==1:
		dims = 5
		traps = 12
	else:
		dims = 8
		traps = 18

	max = dims*dims
	entries = zeros(MAZE_ENTRY_COUNT)
	rooms = []

	GemRB.SetupMaze(dims, dims)
	for x in range(dims, MAZE_MAX_DIM):
		for y in range(dims, MAZE_MAX_DIM):
			pos = y*MAZE_MAX_DIM+x;
			entries[pos] = 2

	nordomx = GemRB.Roll(1, dims-1, -1)
	nordomy = GemRB.Roll(1, dims, -1)
	pos = nordomy*MAZE_MAX_DIM+nordomx
	entries[pos] = 2
	GemRB.SetMazeEntry(pos, ME_WALLS, WALL_EAST)
	pos = nordomy*MAZE_MAX_DIM+nordomx+1
	AddRoom(pos)
	if (mazedifficulty>1):
		GemRB.SetMazeData(MH_POS1X, nordomx)
		GemRB.SetMazeData(MH_POS1Y, nordomy)
		pos2x = GemRB.Roll(1, dims, -1)
		pos2y = GemRB.Roll(1, dims-1, -1)
		while not MainRoomFits(pos2x, pos2y, pos):
			pos2x = GemRB.Roll(1, dims, -1)
			pos2y = GemRB.Roll(1, dims-1, -1)

		GemRB.SetMazeData(MH_POS2X, -1)
		GemRB.SetMazeData(MH_POS2Y, -1)
	else:
		GemRB.SetMazeData(MH_POS1X, -1)
		GemRB.SetMazeData(MH_POS1Y, -1)
		GemRB.SetMazeData(MH_POS2X, -1)
		GemRB.SetMazeData(MH_POS2Y, -1)

	oldentries = entries
	for i in range(traps):
		posx = GemRB.Roll(1, dims, -1)
		posy = GemRB.Roll(1, dims, -1)
		pos = posy*MAZE_MAX_DIM+posx
		while entries[pos]:
			pos = pos + 1
			if pos>=MAZE_ENTRY_COUNT:
				posx = 0
				posy = 0
				pos = 0
			else:
				posy = pos/MAZE_MAX_DIM
				posx = pos-posy*MAZE_MAX_DIM
		GemRB.SetMazeEntry(pos, ME_TRAP, GemRB.Roll(1, 3, -1) )

	entries = oldentries
	while len(rooms)>0:
		pos = rooms[0]
		rooms[0:1] = []
		posy = pos/MAZE_MAX_DIM
		posx = pos-posy*MAZE_MAX_DIM
		possible = GetPossible(pos)
		plen = len(possible)
		if plen>0:
			if plen==1:
				newpos = possible[0]
			else:
				newpos = possible[GemRB.Roll(1, plen, -1) ]
			if entries[newpos]==0:
				if newpos+1 == pos:
					GemRB.SetMazeEntry(pos, ME_WALLS, WALL_EAST)
				elif pos+1 == newpos:
					GemRB.SetMazeEntry(pos, ME_WALLS, WALL_WEST)
				elif pos+MAZE_MAX_DIM == newpos:
					GemRB.SetMazeEntry(pos, ME_WALLS, WALL_NORTH)
				elif newpos+MAZE_MAX_DIM == pos:
					GemRB.SetMazeEntry(pos, ME_WALLS, WALL_SOUTH)
				else:
					print "Something went wrong at pos: ", pos, " newpos: ", newpos
				AddRoom(newpos)

	GemRB.SetMazeData(MH_TRAPCOUNT, traps)
	GemRB.SetMazeData(MH_INITED, 1)
	return
