# GemRB port for Xbox (Original Xbox) using NXDK

## Overview

This port brings GemRB to the original Xbox console using the NXDK (Xbox Development Kit) homebrew toolchain. It allows you to play Baldur's Gate, Icewind Dale, and Planescape: Torment on your modded Xbox.

## Prerequisites

### Hardware Requirements
- Modded original Xbox console with homebrew capability
- Xbox hard drive with available space (at least 2GB recommended)
- Original Xbox controller
- Game data from supported titles

### Development Requirements
- NXDK (Xbox Development Kit) installed and configured
- Cross-compilation toolchain (i686-w64-mingw32)
- SDL 1.2 development libraries for NXDK
- Python 3.x development libraries (compiled for Xbox)

## Installation

### Setting up NXDK
1. Download and install NXDK from https://github.com/XboxDev/nxdk
2. Follow NXDK installation instructions for your platform
3. Ensure the NXDK environment variables are set correctly

### Installing game data
1. Copy your original game installation to `E:\GemRB\`
2. The structure should look like:
   ```
   E:\GemRB\
   ├── GemRB.cfg
   ├── GUIScripts\
   ├── override\
   ├── unhardcoded\
   └── [GameFolder]\  (e.g., BG1, BG2, IWD, etc.)
   ```

## Building

### Build Configuration
```bash
mkdir build && cd build
cmake .. \
  -DCMAKE_TOOLCHAIN_FILE=$NXDK_DIR/share/toolchain-nxdk.cmake \
  -DSDL_BACKEND=SDL \
  -DSTATIC_LINK=ON \
  -DDISABLE_WERROR=ON \
  -DXBOX=ON \
  -DUSE_OPENAL=OFF \
  -DUSE_FREETYPE=OFF \
  -DUSE_LIBVLC=OFF \
  -DCMAKE_BUILD_TYPE=Release
make
```

### Installing to Xbox
1. Copy the built executable to your Xbox: `E:\GemRB\GemRB.xbe`
2. Copy required data files:
   - `GUIScripts\` folder
   - `override\` folder  
   - `unhardcoded\` folder
3. Create or modify `GemRB.cfg` with Xbox-specific settings

## Configuration

### GemRB.cfg Settings for Xbox
```ini
# Basic video settings optimized for Xbox
Width=640
Height=480
Bpp=16
Fullscreen=1

# Audio settings
AudioDriver=sdlaudio

# Xbox filesystem paths
CachePath=E:\GemRB\Cache2\
GemRBPath=E:\GemRB\
GamePath=E:\GemRB\[YourGameFolder]\

# Performance optimizations for Xbox
GamepadPointerSpeed=5
MaxPartySize=6
TooltipDelay=500

# Memory optimizations
MemoryOptimizations=1
ReduceScriptingMemory=1
```

## Controls

### Xbox Controller Mapping
- **Left analog stick**: Mouse cursor movement
- **A button**: Left mouse click (select/confirm)
- **B button**: Right mouse click (context menu/cancel)
- **X button**: Open inventory
- **Y button**: Open map
- **D-Pad**: Scroll map/navigate interface
- **Right analog stick**: Alternative map scrolling
- **Left trigger**: Highlight objects
- **Right trigger**: Pause game
- **Back button**: Open main menu
- **Start button**: Escape/close current window

## Performance Considerations

### Memory Limitations
The original Xbox has only 64MB of RAM, so several optimizations are in place:
- Static linking to reduce memory overhead
- Disabled Python site packages and user directories
- Optimized texture loading and caching
- Reduced script memory allocation

### Recommended Game Settings
- Use 16-bit color depth for better performance
- Keep party size reasonable (max 6 characters)
- Avoid heavy scripting mods
- Use lower resolution textures when available

### Storage Notes
- Game data should be stored on the Xbox hard drive (E:\ drive)
- Cache directory will be created automatically
- Save games are stored in the game folder

## Limitations

### Current Limitations
- No network play support
- Limited Python scripting capabilities due to memory constraints
- No movie playback (LibVLC not supported)
- No OpenAL audio (uses SDL audio only)
- No TrueType font support (FreeFree disabled)

### Compatibility Notes
- Tested primarily with Baldur's Gate series
- Large mods may not work due to memory constraints
- Some advanced scripting features may be disabled

## Troubleshooting

### Common Issues
1. **Game won't start**: Check that GemRB.cfg paths are correct for Xbox filesystem
2. **Memory errors**: Reduce party size and disable unnecessary features
3. **Audio problems**: Ensure AudioDriver is set to "sdlaudio"
4. **Controller not working**: Verify controller is connected before starting

### Debug Information
Debug output is written to:
- Xbox debug console (visible with debugging tools)
- Log file at `E:\GemRB\gemrb.log`

## Building from Source

### Prerequisites
```bash
# Install NXDK
git clone https://github.com/XboxDev/nxdk.git
cd nxdk
make

# Set environment variables
export NXDK_DIR=/path/to/nxdk
export PATH=$NXDK_DIR/bin:$PATH
```

### Compilation
```bash
# Clone GemRB with Xbox support
git clone https://github.com/gemrb/gemrb.git
cd gemrb

# Configure for Xbox
mkdir build-xbox && cd build-xbox
cmake .. -DCMAKE_TOOLCHAIN_FILE=$NXDK_DIR/share/toolchain-nxdk.cmake \
         -DXBOX=ON -DSTATIC_LINK=ON -DSDL_BACKEND=SDL

# Build
make -j$(nproc)
```

## Credits

- GemRB development team for the original engine
- Xbox homebrew community for NXDK
- SDL developers for Xbox SDL support

## Support

For Xbox-specific issues, please:
1. Check this documentation first
2. Search existing GemRB issues
3. Create a new issue with "[Xbox]" prefix if needed

Remember that this is homebrew software - ensure your Xbox console is properly modified and you have legal backups of your games.