# !!! Keep these synchronized with maze definitions in Game.h !!!


#maze header indices
MH_POS1X = 0
MH_POS1Y = 1
MH_POS2X = 2
MH_POS2Y = 3
MH_POS3X = 4
MH_POS3Y = 5
MH_POS4X = 6
MH_POS4Y = 7
MH_TRAPCOUNT = 8
MH_INITED = 9
MH_UNKNOWN28 = 10
MH_UNKNOWN2C = 11

#maze entry indices
ME_OVERRIDE = 0
ME_VALID = 1
ME_ACCESSIBLE = 2
ME_TRAP = 3
ME_WALLS = 4
ME_VISITED = 5

# Wall directions
WALL_SOUTH = 1
WALL_NORTH = 2
WALL_EAST = 4
WALL_WEST = 8

#maximum maze entry
MAZE_ENTRY_COUNT = 64
MAZE_MAX_DIM = 8
