// IE specific cursor types
#ifndef IE_CURSORS_H
#define IE_CURSORS_H

namespace GemRB {

#define IE_CURSOR_INVALID -1
#define IE_CURSOR_NORMAL  0
#define IE_CURSOR_TAKE    2  //over pile type containers
#define IE_CURSOR_WALK    4
#define IE_CURSOR_BLOCKED 6
#define IE_CURSOR_USE     8  //never hardcoded
#define IE_CURSOR_WAIT    10 //hourglass
#define IE_CURSOR_ATTACK  12
#define IE_CURSOR_SWAP    14 //dragging portraits
#define IE_CURSOR_DEFEND  16
#define IE_CURSOR_TALK    18
#define IE_CURSOR_CAST    20 //targeting with non weapon
#define IE_CURSOR_INFO    22 //never hardcoded
#define IE_CURSOR_LOCK    24 //locked door
#define IE_CURSOR_LOCK2   26 //locked container
#define IE_CURSOR_STAIR   28 //never hardcoded
#define IE_CURSOR_DOOR    30 //doors
#define IE_CURSOR_CHEST   32
#define IE_CURSOR_TRAVEL  34
#define IE_CURSOR_STEALTH 36
#define IE_CURSOR_TRAP    38
#define IE_CURSOR_PICK    40 //pickpocket
#define IE_CURSOR_PASS    42 //never hardcoded
#define IE_CURSOR_GRAB    44
#define IE_CURSOR_WAY     46 //waypoint (not in PST)
#define IE_CURSOR_INFO2   46 //PST
#define IE_CURSOR_PORTAL  48 //PST
#define IE_CURSOR_STAIR2  50 //PST
#define IE_CURSOR_EXTRA   52 //PST

#define IE_CURSOR_MASK    127
#define IE_CURSOR_GRAY    128

}

#endif // ! IE_CURSORS_H
