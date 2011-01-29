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
walls = None

def Possible(posx, posy):
	pos = posy*dims+posx
	if entries[pos]==2:
		return false
	return true

def GetPossible (pos):
	posy = pos/max
	posx = pos-posy
	possible = []

	if posx>0:
		pos = Possible(posx-1, posy)
		if pos>0:
			possible[:0] = pos
	if posy>0:
		pos = Possible(posx, posy-1)
		if pos>0:
			possible[:0] = pos
	if posx<dims-1:
		pos = Possible(posx+1, posy)
		if pos>0:
			possible[:0] = pos
	if posy<dims-1:
		pos = Possible(posx, posy+1)
		if pos>0:
			possible[:0] = pos
	return possible

#loads a 2da and sets it up as maze
def LoadMazeFrom2da(tablename):
	MazeTable = GemRB.LoadTable(tablename)
	if MazeTable == None:
		return
	size = MazeTable.GetValue(-1,-1)
	GemRB.SetupMaze(size, size)
	entries = size*size
	traps = 0
	for i in entries:
		Area = MazeTable.GetRowName(i)
		FIELD_0 = MazeTable.GetValue(Area,"FIELD_0")
		TRAPTYPE = MazeTable.GetValue(Area,"TRAPTYPE")
		WALLS = MazeTable.GetValue(Area,"WALLS")
		FIELD_16 = MazeTable.GetValue(Area,"FIELD_16")
		pos = int(Area[3:])
		GemRB.SetMazeEntry(pos, ME_0, FIELD_0)
		GemRB.SetMazeEntry(pos, ME_TRAP, TRAPTYPE)
		GemRB.SetMazeEntry(pos, ME_WALLS, WALLS)
		GemRB.SetMazeEntry(pos, ME_16, FIELD_16)
		if TRAPTYPE>=0:
			traps = traps+1

	GemRB.SetMazeData(MH_TRAPS, traps)
	GemRB.SetMazeData(MH_INITED, 1)
	return

def AddRoom (pos):
	rooms[:0]=[pos]
	entries[pos] = 1
	return

def MainRoomFits (pos2x, pos2y, pos):
	south = pos2y*dims+pos2x

	if south==pos:
		return false

	north = (pos2y+1)*dims+pos2x
	if north==pos:
		return false

	GemRB.SetMazeEntry(south, ME_WALLS, WALL_NORTH)
	entries[north]=2
	AddRoom(south)
	return true

def zeros (size):
	return size*[0]

def PrintMaze():
	return

###################################################
def CreateMaze ():
	global max
	global dims
	global entries
	global walls

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
	entries = zeros(max)
	walls = zeros(max)

	GemRB.SetupMaze(dims, dims)

	nordomx = GemRB.Roll(1, dims-1, 0)
	nordomy = GemRB.Roll(1, dims, 0)
	pos = nordomy*dims+nordomx
	entries[pos] = 2
	GemRB.SetMazeEntry(pos, ME_WALLS, WALL_EAST)
	pos = (nordomy+1)*dims+nordomx
	AddRoom(pos)
	if (mazedifficulty>1):
		GemRB.SetMazeData(MAZE_POS1X, nordomx)
		GemRB.SetMazeData(MAZE_POS1Y, nordomy)
		pos2x = GemRB.Roll(1, dims, 0)
		pos2y = GemRB.Roll(1, dims-1, 0)
		while not MainroomFits(pos2x, pos2y, pos):
			pos2x = GemRB.Roll(1, dims, 0)
			pos2y = GemRB.Roll(1, dims-1, 0)

		GemRB.SetMazeData(MAZE_POS2X, -1)
		GemRB.SetMazeData(MAZE_POS2Y, -1)
	else:
		GemRB.SetMazeData(MAZE_POS1X, -1)
		GemRB.SetMazeData(MAZE_POS1Y, -1)
		GemRB.SetMazeData(MAZE_POS2X, -1)
		GemRB.SetMazeData(MAZE_POS2Y, -1)
		

	for i in traps:
		posx = GemRB.Roll(1, dims, 0)
		posy = GemRB.Roll(1, dims, 0)
		pos = posy*dims+posx;
		while entries[pos]:
			pos = pos + 1
			if pos>=max:
				posx = 0
				posy = 0
				pos = 0
			else:
				posy = pos/dims
				posx = pos-posy
		GemRB.SetMazeEntry(pos, MAZE_TRAP, GemRB.Roll(1, 3, 0) )

	entries = zeros(max)
	while rooms.len()>0:
		pos = rooms[0]
		rooms[0:1] = []
		posy = pos/dims
		posx = pos-posy
		possible = GetPossible(pos)
		if possible.len()>0:
			if possible.len()==1:
				newpos = possible[0]
			else:
				newpos = possible[GemRB.Roll(1, possible.len(), 0) ]
			if entries[newpos]==0:
				AddRoom(newpos)

	GemRB.SetMazeData(MAZE_TRAPS, traps)
	GemRB.SetMazeData(MAZE_INITED, 1)
	return
