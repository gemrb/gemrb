# GemRB port for PS Vita

## Install
Download gemrb_data.zip and gemrb.vpk files from https://github.com/Northfear/gemrb-vita/releases.

Install gemrb.vpk to your Vita. Extract "GemRB" folder from gemrb_data.zip to ux0:data.

Copy original game folder to ux0:data/GemRB/ and edit ux0:data/GemRB/GemRB.cfg file (set correct "GameType" and "GamePath" parameters. Game auto detection isn't working, so set "GameType" manually).

rePatch reDux0 plugin is required for proper suspend/resume support

https://github.com/dots-tb/rePatch-reDux0

## Building

### Prerequisites
- DolceSDK
- libSDL 1.2
- SDL_mixer
- libpython

This one 

https://github.com/uyjulian/python_vita

Or this one (checkout and just do make && make install)

https://github.com/Northfear/Python-2.7.3-vita

are both working fine

### Build & installation
```
mkdir build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=$DOLCESDK/share/dolce.toolchain.cmake -DSDL_BACKEND=SDL -DSTATIC_LINK=enabled -DDISABLE_WERROR=enabled -DCMAKE_MAKE_PROGRAM=make -DVITA=true -DUSE_OPENAL=false -DUSE_FREETYPE=false -DCMAKE_BUILD_TYPE=None -DNOCOLOR=1
make
```
Generated VPK file is located in build/gemrb folder. Install it to your Vita system.

Copy folders "GUIScripts", "override" and "unhardcoded" from gemrb folder into ux0:data/GemRB/

Rename build/gemrb/GemRB.cfg.sample into GemRB.cfg, copy it to ux0:data/GemRB/ and change the following options:

```
Bpp=16 #recommended for better performance.
AudioDriver=sdlaudio
CachePath=ux0:data/GemRB/Cache2/
GemRBPath=ux0:data/GemRB/
```

Debug output can be previewed with psp2shell

https://github.com/Cpasjuste/psp2shell

## Port info

### Input

Keyboard input is done with D-Pad (on character creation and game saves. 'a-z', '0-9' and 'space' are supported).

- Left - Remove character
- Right - Add new character
- Down - Next character (alphabetically)
- Up - Previous character
- R1, L1 - Switch current character between uppercase/lowercase

### Vita specific options

Pointer movement speed can be changed with 'GamepadPointerSpeed' parameter in GemRB.cfg.

Use "Fullscreen=1" to scale game area to native Vita resolution or "Fullscreen=0" to keep game area at the center of the screen.

VitaKeepAspectRatio=1 keeps aspect ratio of original image when scaling. VitaKeepAspectRatio=0 just scales it to 960x544.

### Performance

Widescreen mod is supported, but performance with native resolution can be poor in areas with a lot of characters.

The game is pretty IO heavy. Loading can take quite some time. And a big number of sound effects playing at the same time can cause a lot of stuttering. To improve the situation I STRONGLY recommend disabling character movement and attack sounds in game options. Disabling sound altogether (by seting "AudioDriver = none" in GemRB.cfg) can improve it even further.

"Bpp=16" option is recommended for better performance.

## Controls
- Left analog stick - Pointer movement
- X button - Left mouse button
- O button - Right mouse button
- SQUARE button - Open map
- TRIANGLE button - Open inventory
- D-Pad, Right analog stick  - Map scrolling
- R1 - Pause
- L1 - Highlight items
- SELECT - Open menu
- START - Escape
