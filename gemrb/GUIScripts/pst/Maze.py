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
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
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
from GUIDefines import STR_AREANAME

rooms = None
max = 0
dims = 0
entries = None
offx = (0,0,1,0,-1)
offy = (0,-1,0,1,0)
wallbits = (0, WALL_NORTH, WALL_EAST, WALL_SOUTH, WALL_WEST)
entrances = ("", "Entry3", "Entry4","Entry1","Entry2")

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
	traps = 0
	for i in range(MazeTable.GetRowCount()):
		Area = MazeTable.GetRowName(i)
		ACCESSED = MazeTable.GetValue(Area,"ACCESSED")
		TRAPTYPE = MazeTable.GetValue(Area,"TRAPTYPE")
		WALLS = MazeTable.GetValue(Area,"WALLS")
		SPECIAL = MazeTable.GetValue(Area,"SPECIAL")
		pos = int(Area[4:])-1
		GemRB.SetMazeEntry(pos, ME_ACCESSED, ACCESSED)
		GemRB.SetMazeEntry(pos, ME_TRAP, TRAPTYPE)
		GemRB.SetMazeEntry(pos, ME_WALLS, WALLS)
		GemRB.SetMazeEntry(pos, ME_SPECIAL, SPECIAL)
		if TRAPTYPE>=0:
			traps = traps+1

	#adding foyer coordinates (middle of bottom)
	GemRB.SetMazeData(MH_POS3X, size/2)
	GemRB.SetMazeData(MH_POS3Y, size-1)
	#adding trap
	GemRB.SetMazeData(MH_TRAPCOUNT, traps)
	#finish
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

	#adding foyer coordinates
	#TODO, how does the original prevent blocking by special rooms
	GemRB.SetMazeData(MH_POS3X, GemRB.Roll(1,dims,-1) )
	GemRB.SetMazeData(MH_POS3Y, dims-1)
	#adding traps
	GemRB.SetMazeData(MH_TRAPCOUNT, traps)
	#finish
	GemRB.SetMazeData(MH_INITED, 1)
	return

def FormatAreaName(pos):
	if pos<9:
		return "AR130"+str(pos+1)
	return "AR13"+str(pos+1)

def CustomizeMaze(AreaName):

	header = GemRB.GetMazeHeader()
	print "AreaName:", AreaName

	mainX = header['Pos1X']
	mainY = header['Pos1Y']
	nordomX = header['Pos2X']
	nordomY = header['Pos2Y']
	foyerX = header['Pos3X']
	foyerY = header['Pos3Y']
	print header
	#modron foyer
	if AreaName == "fy":
		#TODO modron foyer, only one entrance if EnginInMaze = 1
		tmp = foyerX+foyerY*MAZE_MAX_DIM
		entry = GemRB.GetMazeEntry(tmp)
		GemRB.SetMapExit ("exit1", FormatAreaName(tmp), "Entry3" )

		#disable engine room 
		if GemRB.GetGameVar("EnginInMaze")==1:
			GemRB.SetMapExit ("exit3" )
		else:
			GemRB.SetMapExit ("exit3", "AR13EN")
		GemRB.SetMapExit ("exit4" )
		return

	if AreaName == "en":
		#TODO engineering room
		#what to do here?
		return

	if AreaName == "wz":
		#TODO wizard's lair
		tmp = mainX+mainY*MAZE_MAX_DIM
		entry = GemRB.GetMazeEntry(tmp)
		print "TMP=", tmp
		print entry
		return

	if AreaName == "fd":
		#TODO nordom
		tmp = nordomX+nordomY*MAZE_MAX_DIM
		entry = GemRB.GetMazeEntry(tmp)
		print "TMP=", tmp
		print entry
		return

	tmp = int(AreaName)-1
	if tmp<0 or tmp>63:
		return

	entry = GemRB.GetMazeEntry(tmp)
	#TODO: customize maze area based on entry (walls, traps)
	print entry

	if entry['Special']:
		print "Inconsistence: somehow stumbled into a special area."
		return

	if not entry['Visited']:
		#already customized
		return

	trapped = entry['Trapped']
	if trapped>=0:
		print "Enable trap", 'Trap'+chr(trapped+65) 
		GemRB.EnableRegion('Trap'+chr(trapped+65) )

	GemRB.SetMazeEntry(tmp, ME_ACCESSED, 0)
	walls = entry['Walls']
	y = tmp / MAZE_MAX_DIM
	x = tmp - y*MAZE_MAX_DIM
	for i in range(1,5):
		if wallbits[i]&walls:
			x2 = x+offx[i]
			y2 = y+offy[i]
			print "Current: ", x,":", y, " New: ", x2, ":", y2
			set = 0
			if x2 == nordomX and y2 == nordomY:
				NewArea = "AR13FD"
			elif x2 == mainX and y2 == mainY:
				NewArea = "AR13WZ"
			elif x2 == foyerX and y2 == foyerY+1:
				NewArea = "AR13FY"
			else:
				if x2>=0 and x2<MAZE_MAX_DIM and y2>=0 and y2<MAZE_MAX_DIM:
					NewArea = FormatAreaName (x2+y2*MAZE_MAX_DIM)
				else:
					#maximum dimensions
					set = 1
		else:
			set = 1

		if set:
			#remove exit
			print "Set Wall ", str(i)
			GemRB.SetMapExit ("exit"+str(i) )
		else:
			print "Set Exit ", str(i), " to ", NewArea, " entrance:", entrances[i]
			#set exit
			GemRB.SetMapExit ("exit"+str(i), NewArea, entrances[i] )
	return

def CustomizeArea():
	Area = GemRB.GetGameString (STR_AREANAME)
	if Area[0:4] == "ar13":
		CustomizeMaze(Area[4:])
		return

	#TODO insert non maze area customization here (set own area scripts for special areas)
	return
