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
#wallbits = (0, WALL_WEST, WALL_NORTH, WALL_EAST, WALL_SOUTH)
wallbits = (0, WALL_NORTH, WALL_EAST, WALL_SOUTH, WALL_WEST)
entrances = ("", "Entry3", "Entry4", "Entry1", "Entry2")
doors = ("", "northdoor", "eastdoor", "southdoor", "westdoor")
anims = ("", "a13xxdn", "a13xxde", "a13xxds", "a13xxdw")
aposx=(0,1012,1005,505,380)
aposy=(0,524,958,996,562)
cposx=(686,886,497)
cposy=(498,722,726)

def Possible(posx, posy):
	global entries

	pos = posx*MAZE_MAX_DIM+posy

	if entries[pos] == 2:
		return -1
	return pos

def GetPossible (pos):
	posx = pos/MAZE_MAX_DIM
	posy = pos-posx*MAZE_MAX_DIM
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
		OVERRIDE = MazeTable.GetValue(Area,"OVERRIDE")
		TRAPTYPE = MazeTable.GetValue(Area,"TRAPTYPE")
		WALLS = MazeTable.GetValue(Area,"WALLS")
		VISITED = MazeTable.GetValue(Area,"VISITED")
		pos = ConvertPos(int(Area[4:])-1)
		GemRB.SetMazeEntry(pos, ME_OVERRIDE, OVERRIDE)
		GemRB.SetMazeEntry(pos, ME_TRAP, TRAPTYPE)
		GemRB.SetMazeEntry(pos, ME_WALLS, WALLS)
		GemRB.SetMazeEntry(pos, ME_VISITED, VISITED)
		if TRAPTYPE>=0:
			traps = traps+1

	#disabling special rooms
	GemRB.SetMazeData(MH_POS1X, -1)
	GemRB.SetMazeData(MH_POS1Y, -1)
	GemRB.SetMazeData(MH_POS2X, -1)
	GemRB.SetMazeData(MH_POS2Y, -1)
	#adding foyer coordinates (middle of bottom)
	GemRB.SetMazeData(MH_POS3X, size/2)
	GemRB.SetMazeData(MH_POS3Y, size-1)
	#adding engine room coordinates (bottom right)
	GemRB.SetMazeData(MH_POS4X, size-1)
	GemRB.SetMazeData(MH_POS4Y, size-1)
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

	south = pos2x*MAZE_MAX_DIM+pos2y

	if south==pos:
		return False

	north = (pos2x+1)*MAZE_MAX_DIM+pos2y
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

	MazeX = header["MazeX"]
	MazeY = header["MazeY"]
	MainX = header["Pos1X"]
	MainY = header["Pos1Y"]
	NordomX = header["Pos2X"]
	NordomY = header["Pos2Y"]
	FoyerX = header["Pos3X"]

	print "Maze size is "+str(MazeX)+"X"+str(MazeY)
	for y in range (MazeY):
		line = ""
		for x in range (MazeX):
			pos = MAZE_MAX_DIM*x+y
			entry = GemRB.GetMazeEntry(pos)
			if entry["Walls"]&WALL_NORTH:
				line = line + "+ "
			else:
				line = line + "+-"
		print line+"+"
		line = ""
		for x in range (MazeX):
			pos = MAZE_MAX_DIM*x+y
			entry = GemRB.GetMazeEntry(pos)
			if entry["Walls"]&WALL_WEST:
				line = line + " "
			else:
				line = line + "|"
			if x == NordomX and y == NordomY:
				line = line + "N"
			elif x == MainX and y == MainY:
				line = line + "W"
			elif entry["Trapped"]>=0:
				line = line + chr(entry["Trapped"]+65)
			else:
				line = line + " "
		print line+"|"
	line = ""
	for x in range (MazeX):
		if FoyerX==x:
			line = line + "+ "
		else:
			line = line + "+-"
	print line+"+"
	return

def ConvertPos (pos):
	return ((pos&7)<<3)|(pos>>3)

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
		dims = 4
		traps = 5
	elif mazedifficulty==1:
		dims = 6
		traps = 12
	else:
		dims = 8
		traps = 20

	entries = zeros(MAZE_ENTRY_COUNT)
	rooms = []

	GemRB.SetupMaze(dims, dims)
	for x in range(dims, MAZE_MAX_DIM):
		for y in range(dims, MAZE_MAX_DIM):
			pos = x*MAZE_MAX_DIM+y;
			entries[pos] = 2

	nordomx = GemRB.Roll(1, dims-1, -1)
	nordomy = GemRB.Roll(1, dims, -1)
	pos = nordomx*MAZE_MAX_DIM+nordomy
	entries[pos] = 2
	GemRB.SetMazeEntry(pos, ME_WALLS, WALL_EAST)
	pos = nordomx*MAZE_MAX_DIM+nordomy+MAZE_MAX_DIM
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
		pos = posx*MAZE_MAX_DIM+posy
		while entries[pos]:
			pos = pos + 1
			if pos>=MAZE_ENTRY_COUNT:
				posx = 0
				posy = 0
				pos = 0
			else:
				posx = pos/MAZE_MAX_DIM
				posy = pos-posx*MAZE_MAX_DIM
		GemRB.SetMazeEntry(pos, ME_TRAP, GemRB.Roll(1, 3, -1) )

	entries = oldentries
	while len(rooms)>0:
		pos = rooms[0]
		rooms[0:1] = []
		posx = pos/MAZE_MAX_DIM
		posy = pos-posx*MAZE_MAX_DIM
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
	x = GemRB.Roll(1,dims,-1)
	while x!=nordomx and dims-1!=nordomy:
		x=GemRB.Roll(1,dims,-1)

	GemRB.SetMazeData(MH_POS3X, GemRB.Roll(1,dims,-1) )
	GemRB.SetMazeData(MH_POS3Y, dims-1)
	
	#setting engine room coordinates to hidden (accessible from foyer)
	GemRB.SetMazeData(MH_POS4X, -1)
	GemRB.SetMazeData(MH_POS4Y, -1)
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

	mainX = header['Pos1X']
	mainY = header['Pos1Y']
	nordomX = header['Pos2X']
	nordomY = header['Pos2Y']
	foyerX = header['Pos3X']
	foyerY = header['Pos3Y']
	engineX = header['Pos4X']
	engineY = header['Pos4Y']
	#modron foyer
	if AreaName == "fy":
		#TODO modron foyer, only one entrance if EnginInMaze = 1
		tmp = foyerX+foyerY*MAZE_MAX_DIM
		GemRB.SetMapExit ("exit1", FormatAreaName(tmp), "Entry3" )

		#disable engine room
		if GemRB.GetGameVar("EnginInMaze")==1:
			GemRB.SetMapExit ("exit3" )
			GemRB.SetMapDoor (doors[3], 0)
			GemRB.SetMapAnimation(aposx[3], aposy[3], anims[3])
		else:
			GemRB.SetMapExit ("exit3", "AR13EN")
			
		GemRB.SetMapExit ("exit4" )
		GemRB.SetMapAnimation(aposx[4], aposy[4], anims[4])
		return

	if AreaName == "en":
		if GemRB.GetGameVar("EnginInMaze")==1:
			tmp = engineX+(engineY-1)*MAZE_MAX_DIM
			GemRB.SetMapExit ("exit1", FormatAreaName(tmp), "Entry3" )
		else:
			GemRB.SetMapExit ("exit1", "AR13FY" )
		return

	if AreaName == "wz":
		#TODO wizard's lair
		tmp = mainY+mainX*MAZE_MAX_DIM
		entry = GemRB.GetMazeEntry(tmp)
		tmp = mainX+mainY*MAZE_MAX_DIM
		GemRB.SetMapExit ("exit3", FormatAreaName(tmp), "Entry1" )
		GemRB.SetMapDoor (doors[3], 1)
		return

	if AreaName == "fd":
		#TODO nordom
		tmp = nordomY+nordomX*MAZE_MAX_DIM
		entry = GemRB.GetMazeEntry(tmp)
		tmp = nordomX+nordomY*MAZE_MAX_DIM
		GemRB.SetMapExit ("exit2", FormatAreaName(tmp), "Entry4" )
		GemRB.SetMapDoor (doors[2], 1)
		return

	tmp = int(AreaName)-1
	if tmp<0 or tmp>63:
		return

	pos = ConvertPos(tmp)
	entry = GemRB.GetMazeEntry(pos)
	#TODO: customize maze area based on entry (walls, traps)

	if entry['Visited']:
		#already customized
		return

	difficulty = GemRB.GetGameVar("MazeDifficulty")
	if difficulty == 0:
		name = "CLOW"
	elif difficulty == 1:
		name = "CMOD"
	else:
		name = "CHIGH"

	ccount = GemRB.Roll(1,3,0)

	for i in range(ccount):
		GemRB.CreateCreature(0, name, cposx[i], cposy[i])

	trapped = entry['Trapped']
	if trapped>=0:
		type = GemRB.Roll(1,4,0)
		GemRB.SetMapRegion('Trap'+chr(trapped+65), '1300trp'+str(type) )

	GemRB.SetMazeEntry(pos, ME_VISITED, 1)
	walls = entry['Walls']
	y = tmp / MAZE_MAX_DIM
	x = tmp - y*MAZE_MAX_DIM
	for i in range(1,5):
		if wallbits[i]&walls:
			x2 = x+offx[i]
			y2 = y+offy[i]
			set = 0
			if x2 == nordomX and y2 == nordomY:
				NewArea = "AR13FD"
			elif x2 == mainX and y2 == mainY:
				NewArea = "AR13WZ"
			elif x2 == foyerX and y2 == foyerY+1:
				NewArea = "AR13FY"
			elif x2 == engineX and y2 == engineY:
				NewArea = "AR13EN"
			else:
				if x2>=0 and x2<MAZE_MAX_DIM and y2>=0 and y2<MAZE_MAX_DIM:
					#reversed coordinates
					NewArea = FormatAreaName (x2+y2*MAZE_MAX_DIM)
				else:
					#maximum dimensions
					set = 1
		else:
			set = 1

		if set:
			#remove exit
			GemRB.SetMapExit ("exit"+str(i) )
			GemRB.SetMapDoor (doors[i], 0)
			GemRB.SetMapAnimation(aposx[i], aposy[i], anims[i])
		else:
			#set exit
			GemRB.SetMapExit ("exit"+str(i), NewArea, entrances[i] )
			GemRB.SetMapDoor (doors[i], 1)
			GemRB.SetMapAnimation(-1, -1, "", 0, 0)
	return

def CustomizeArea():
	Area = GemRB.GetGameString (STR_AREANAME)
	if Area[0:4] == "ar13":
		CustomizeMaze(Area[4:])
		return

	#TODO insert non maze area customization here (set own area scripts for special areas)
	return
