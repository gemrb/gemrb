LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := main

SDL_PATH := ../SDL

LOCAL_C_INCLUDES := $(LOCAL_PATH)/$(SDL_PATH)/include $(LOCAL_PATH)/main/gemrb/includes/ \
		$(LOCAL_PATH)/main/gemrb/plugins/SAVImporter/ $(LOCAL_PATH)/main/gemrb/plugins/BMPWriter/ \
		$(LOCAL_PATH)/main/gemrb/plugins/TISImporter/ $(LOCAL_PATH)/main/gemrb/plugins/CHUImporter/ \
		$(LOCAL_PATH)/main/gemrb/plugins/2DAImporter/ $(LOCAL_PATH)/main/gemrb/plugins/BAMImporter/ \
		$(LOCAL_PATH)/main/gemrb/plugins/ITMImporter/ $(LOCAL_PATH)/main/gemrb/plugins/BMPImporter/ \
		$(LOCAL_PATH)/main/gemrb/plugins/WEDImporter/ $(LOCAL_PATH)/main/gemrb/plugins/IDSImporter/ \
		$(LOCAL_PATH)/main/gemrb/plugins/ZLibManager/ $(LOCAL_PATH)/main/gemrb/plugins/DLGImporter/ \
		$(LOCAL_PATH)/main/gemrb/plugins/GUIScript/ $(LOCAL_PATH)/main/gemrb/plugins/WAVReader/ \
		$(LOCAL_PATH)/main/gemrb/plugins/STOImporter/ $(LOCAL_PATH)/main/gemrb/plugins/TTFImporter/ \
		$(LOCAL_PATH)/main/gemrb/plugins/WMPImporter/ $(LOCAL_PATH)/main/gemrb/plugins/INIImporter/ \
		$(LOCAL_PATH)/main/gemrb/plugins/MOSImporter/ $(LOCAL_PATH)/main/gemrb/plugins/TLKImporter/ \
		$(LOCAL_PATH)/main/gemrb/plugins/BIKPlayer/ $(LOCAL_PATH)/main/gemrb/plugins/SPLImporter/ \
		$(LOCAL_PATH)/main/gemrb/plugins/NullSound/ $(LOCAL_PATH)/main/gemrb/plugins/EFFImporter/ \
		$(LOCAL_PATH)/main/gemrb/plugins/SDLAudio/ $(LOCAL_PATH)/main/gemrb/plugins/CREImporter/ \
		$(LOCAL_PATH)/main/gemrb/plugins/OpenALAudio/ $(LOCAL_PATH)/main/gemrb/plugins/PROImporter/ \
		$(LOCAL_PATH)/main/gemrb/plugins/ACMReader/ $(LOCAL_PATH)/main/gemrb/plugins/GAMImporter/ \
		$(LOCAL_PATH)/main/gemrb/plugins/SDLVideo/ $(LOCAL_PATH)/main/gemrb/plugins/BIFImporter/ \
		$(LOCAL_PATH)/main/gemrb/plugins/KEYImporter/ $(LOCAL_PATH)/main/gemrb/plugins/AREImporter/ \
		$(LOCAL_PATH)/main/gemrb/plugins/DirectoryImporter/ \
		$(LOCAL_PATH)/main/gemrb/plugins/OGGReader/ $(LOCAL_PATH)/main/gemrb/plugins/MVEPlayer/ \
		$(LOCAL_PATH)/main/gemrb/plugins/NullSource/ $(LOCAL_PATH)/main/gemrb/plugins/PLTImporter/ \
		$(LOCAL_PATH)/main/gemrb/plugins/PNGImporter/ $(LOCAL_PATH)/main/gemrb/plugins/MUSImporter/ \
		$(LOCAL_PATH)/main/gemrb/core/Scriptable/ $(LOCAL_PATH)/main/gemrb/core/System/Logger/ \
		$(LOCAL_PATH)/main/gemrb/core/System/ $(LOCAL_PATH)/main/gemrb/core/ \
		$(LOCAL_PATH)/../freetype2-android/include \
		$(LOCAL_PATH)/../SDL_mixer-1.2.12/ \
		$(LOCAL_PATH)/../openal/include/ \
		$(LOCAL_PATH)/../SDL/src/core/android/ \
		$(LOCAL_PATH)/../libogg/include/ \
		$(LOCAL_PATH)/../libvorbis/include/ \
		$(LOCAL_PATH)/../libpython/include/ \
		$(LOCAL_PATH)/../libpng/include/ \

# Add your application source files here...
LOCAL_SRC_FILES :=  main/gemrb/plugins/SAVImporter/SAVImporter.cpp \
		    main/gemrb/plugins/BMPWriter/BMPWriter.cpp \
		    main/gemrb/plugins/TISImporter/TISImporter.cpp \
		    main/gemrb/plugins/CHUImporter/CHUImporter.cpp \
		    main/gemrb/plugins/2DAImporter/2DAImporter.cpp \
		    main/gemrb/plugins/BAMImporter/BAMFontManager.cpp \
		    main/gemrb/plugins/BAMImporter/BAMFont.cpp \
		    main/gemrb/plugins/BAMImporter/BAMImporter.cpp \
		    main/gemrb/plugins/BAMImporter/BAMSprite2D.cpp \
		    main/gemrb/plugins/PSTOpcodes/PSTOpcodes.cpp \
		    main/gemrb/plugins/ITMImporter/ITMImporter.cpp \
		    main/gemrb/plugins/BMPImporter/BMPImporter.cpp \
		    main/gemrb/plugins/WEDImporter/WEDImporter.cpp \
		    main/gemrb/plugins/IDSImporter/IDSImporter.cpp \
		    main/gemrb/plugins/ZLibManager/ZLibManager.cpp \
		    main/gemrb/plugins/DLGImporter/DLGImporter.cpp \
		    main/gemrb/plugins/GUIScript/PythonHelpers.cpp \
		    main/gemrb/plugins/GUIScript/GUIScript.cpp \
		    main/gemrb/plugins/WAVReader/WAVReader.cpp \
		    main/gemrb/plugins/STOImporter/STOImporter.cpp \
		    main/gemrb/plugins/TTFImporter/TTFFontManager.cpp \
		    main/gemrb/plugins/TTFImporter/Freetype.cpp \
		    main/gemrb/plugins/TTFImporter/TTFFont.cpp \
		    main/gemrb/plugins/WMPImporter/WMPImporter.cpp \
		    main/gemrb/plugins/INIImporter/INIImporter.cpp \
		    main/gemrb/plugins/MOSImporter/MOSImporter.cpp \
		    main/gemrb/plugins/TLKImporter/TLKImporter.cpp \
		    main/gemrb/plugins/TLKImporter/TlkOverride.cpp \
		    main/gemrb/plugins/BIKPlayer/dct.cpp \
		    main/gemrb/plugins/BIKPlayer/BIKPlayer.cpp \
		    main/gemrb/plugins/BIKPlayer/rational.cpp \
		    main/gemrb/plugins/BIKPlayer/mem.cpp \
		    main/gemrb/plugins/BIKPlayer/GetBitContext.cpp \
		    main/gemrb/plugins/BIKPlayer/fft.cpp \
		    main/gemrb/plugins/BIKPlayer/rdft.cpp \
		    main/gemrb/plugins/SPLImporter/SPLImporter.cpp \
		    main/gemrb/plugins/NullSound/NullSound.cpp \
		    main/gemrb/plugins/EFFImporter/EFFImporter.cpp \
		    main/gemrb/plugins/FXOpcodes/FXOpcodes.cpp \
		    main/gemrb/plugins/CREImporter/CREImporter.cpp \
		    main/gemrb/plugins/OpenALAudio/OpenALAudio.cpp \
		    main/gemrb/plugins/OpenALAudio/StackLock.cpp \
		    main/gemrb/plugins/OpenALAudio/AmbientMgrAL.cpp \
		    main/gemrb/plugins/PROImporter/PROImporter.cpp \
		    main/gemrb/plugins/ACMReader/decoder.cpp \
		    main/gemrb/plugins/ACMReader/ACMReader.cpp \
		    main/gemrb/plugins/ACMReader/unpacker.cpp \
		    main/gemrb/plugins/GAMImporter/GAMImporter.cpp \
		    main/gemrb/plugins/SDLVideo/SDL20Video.cpp \
		    main/gemrb/plugins/SDLVideo/SDLVideo.cpp \
		    main/gemrb/plugins/SDLVideo/SDLSurfaceSprite2D.cpp \
		    main/gemrb/plugins/BIFImporter/BIFImporter.cpp \
		    main/gemrb/plugins/KEYImporter/KEYImporter.cpp \
		    main/gemrb/plugins/AREImporter/AREImporter.cpp \
		    main/gemrb/plugins/DirectoryImporter/DirectoryImporter.cpp \
		    main/gemrb/plugins/IWDOpcodes/IWDOpcodes.cpp \
		    main/gemrb/plugins/OGGReader/OGGReader.cpp \
		    main/gemrb/plugins/MVEPlayer/mveaudiodec.cpp \
		    main/gemrb/plugins/MVEPlayer/mvevideodec8.cpp \
		    main/gemrb/plugins/MVEPlayer/MVEPlayer.cpp \
		    main/gemrb/plugins/MVEPlayer/mve_player.cpp \
		    main/gemrb/plugins/MVEPlayer/mvevideodec16.cpp \
		    main/gemrb/plugins/NullSource/NullSource.cpp \
		    main/gemrb/plugins/PLTImporter/PLTImporter.cpp \
		    main/gemrb/plugins/PNGImporter/PNGImporter.cpp \
		    main/gemrb/plugins/MUSImporter/MUSImporter.cpp \
		    main/gemrb/core/Projectile.cpp \
		    main/gemrb/core/AnimationMgr.cpp \
		    main/gemrb/core/Store.cpp \
		    main/gemrb/core/Ambient.cpp \
		    main/gemrb/core/TableMgr.cpp \
		    main/gemrb/core/ScriptEngine.cpp \
		    main/gemrb/core/WindowMgr.cpp \
		    main/gemrb/core/Tile.cpp \
		    main/gemrb/core/DisplayMessage.cpp \
		    main/gemrb/core/Scriptable/InfoPoint.cpp \
		    main/gemrb/core/Scriptable/CombatInfo.cpp \
		    main/gemrb/core/Scriptable/Container.cpp \
		    main/gemrb/core/Scriptable/PCStatStruct.cpp \
		    main/gemrb/core/Scriptable/Door.cpp \
		    main/gemrb/core/Scriptable/Scriptable.cpp \
		    main/gemrb/core/Scriptable/Actor.cpp \
		    main/gemrb/core/IniSpawn.cpp \
		    main/gemrb/core/CharAnimations.cpp \
		    main/gemrb/core/Plugin.cpp \
		    main/gemrb/core/GameData.cpp \
		    main/gemrb/core/MusicMgr.cpp \
		    main/gemrb/core/PluginLoader.cpp \
		    main/gemrb/core/WorldMap.cpp \
		    main/gemrb/core/ItemMgr.cpp \
		    main/gemrb/core/SoundMgr.cpp \
		    main/gemrb/core/TileMap.cpp \
		    main/gemrb/core/AmbientMgr.cpp \
		    main/gemrb/core/Inventory.cpp \
		    main/gemrb/core/ScriptedAnimation.cpp \
		    main/gemrb/core/WorldMapMgr.cpp \
		    main/gemrb/core/Spellbook.cpp \
		    main/gemrb/core/Factory.cpp \
		    main/gemrb/core/TileSetMgr.cpp \
		    main/gemrb/core/SymbolMgr.cpp \
		    main/gemrb/core/DialogMgr.cpp \
		    main/gemrb/core/ImageMgr.cpp \
		    main/gemrb/core/LRUCache.cpp \
		    main/gemrb/core/Sprite2D.cpp \
		    main/gemrb/core/Dialog.cpp \
		    main/gemrb/core/Calendar.cpp \
		    main/gemrb/core/DialogHandler.cpp \
		    main/gemrb/core/System/Logger.cpp \
		    main/gemrb/core/System/Logger/MessageWindowLogger.cpp \
		    main/gemrb/core/System/Logger/File.cpp \
		    main/gemrb/core/System/Logger/Stdio.cpp \
		    main/gemrb/core/System/Logger/Android.cpp \
		    main/gemrb/core/System/StringBuffer.cpp \
		    main/gemrb/core/System/VFS.cpp \
		    main/gemrb/core/System/String.cpp \
		    main/gemrb/core/System/Logging.cpp \
		    main/gemrb/core/System/FileStream.cpp \
		    main/gemrb/core/System/MemoryStream.cpp \
		    main/gemrb/core/System/DataStream.cpp \
		    main/gemrb/core/System/SlicedStream.cpp \
		    main/gemrb/core/ResourceDesc.cpp \
		    main/gemrb/core/Item.cpp \
		    main/gemrb/core/SaveGameIterator.cpp \
		    main/gemrb/core/Callback.cpp \
		    main/gemrb/core/ArchiveImporter.cpp \
		    main/gemrb/core/StringMgr.cpp \
		    main/gemrb/core/ControlAnimation.cpp \
		    main/gemrb/core/Region.cpp \
		    main/gemrb/core/ProjectileMgr.cpp \
		    main/gemrb/core/ActorMgr.cpp \
		    main/gemrb/core/Resource.cpp \
		    main/gemrb/core/StoreMgr.cpp \
		    main/gemrb/core/Animation.cpp \
		    main/gemrb/core/ResourceSource.cpp \
		    main/gemrb/core/Map.cpp \
		    main/gemrb/core/Variables.cpp \
		    main/gemrb/core/FactoryObject.cpp \
		    main/gemrb/core/DataFileMgr.cpp \
		    main/gemrb/core/ResourceManager.cpp \
		    main/gemrb/core/Video.cpp \
		    main/gemrb/core/SpriteCover.cpp \
		    main/gemrb/core/EffectQueue.cpp \
		    main/gemrb/core/TileOverlay.cpp \
		    main/gemrb/core/KeyMap.cpp \
		    main/gemrb/core/Audio.cpp \
		    main/gemrb/core/AnimationFactory.cpp \
		    main/gemrb/core/SaveGameMgr.cpp \
		    main/gemrb/core/ImageFactory.cpp \
		    main/gemrb/core/Bitmap.cpp \
		    main/gemrb/core/TileMapMgr.cpp \
		    main/gemrb/core/GlobalTimer.cpp \
		    main/gemrb/core/GameScript/GameScript.cpp \
		    main/gemrb/core/GameScript/Triggers.cpp \
		    main/gemrb/core/GameScript/GSUtils.cpp \
		    main/gemrb/core/GameScript/Matching.cpp \
		    main/gemrb/core/GameScript/Actions.cpp \
		    main/gemrb/core/GameScript/Objects.cpp \
		    main/gemrb/core/Polygon.cpp \
		    main/gemrb/core/GUI/MapControl.cpp \
		    main/gemrb/core/GUI/Label.cpp \
		    main/gemrb/core/GUI/ScrollBar.cpp \
		    main/gemrb/core/GUI/Control.cpp \
		    main/gemrb/core/GUI/WorldMapControl.cpp \
		    main/gemrb/core/GUI/TextEdit.cpp \
		    main/gemrb/core/GUI/Window.cpp \
		    main/gemrb/core/GUI/Button.cpp \
		    main/gemrb/core/GUI/GameControl.cpp \
		    main/gemrb/core/GUI/TextArea.cpp \
		    main/gemrb/core/GUI/EventMgr.cpp \
		    main/gemrb/core/GUI/Progressbar.cpp \
		    main/gemrb/core/GUI/Console.cpp \
		    main/gemrb/core/GUI/Slider.cpp \
		    main/gemrb/core/FontManager.cpp \
		    main/gemrb/core/MoviePlayer.cpp \
		    main/gemrb/core/Font.cpp \
		    main/gemrb/core/MapMgr.cpp \
		    main/gemrb/core/Compressor.cpp \
		    main/gemrb/core/PalettedImageMgr.cpp \
		    main/gemrb/core/PluginMgr.cpp \
		    main/gemrb/core/IndexedArchive.cpp \
		    main/gemrb/core/Spell.cpp \
		    main/gemrb/core/Core.cpp \
		    main/gemrb/core/Image.cpp \
		    main/gemrb/core/SpellMgr.cpp \
		    main/gemrb/core/Particles.cpp \
		    main/gemrb/core/ProjectileServer.cpp \
		    main/gemrb/core/EffectMgr.cpp \
		    main/gemrb/core/Game.cpp \
		    main/gemrb/core/ImageWriter.cpp \
		    main/gemrb/core/Interface.cpp \
		    main/gemrb/core/InterfaceConfig.cpp \
		    main/gemrb/core/Cache.cpp \
		    main/gemrb/core/FileCache.cpp \
		    main/gemrb/core/Palette.cpp \
		    main/gemrb/core/System/swab.c \
		    main/gemrb/GemRB.cpp \
		    ../SDL/src/main/android/SDL_android_main.cpp \
		    # main/gemrb/plugins/SDLAudio/SDLAudio.cpp \

LOCAL_SHARED_LIBRARIES := SDL2 openal ogg vorbis python
LOCAL_STATIC_LIBRARIES := freetype2-static png gnustl_static

LOCAL_CPPFLAGS += -fexceptions -finline-functions -O0 -DSTATIC_LINK=Yes -DHAVE_SNPRINTF

LOCAL_LDLIBS := -lGLESv1_CM -llog -lz -ldl
# LOCAL_LDLIBS += -L$(LOCAL_PATH)/../../../gemrb/obj/local/armeabi/ -logg -lvorbis -lSDL_mixer -lpython2.7 -l freetype2-static -lpng -lgnustl_static

LOCAL_C_INCLUDES += $(NDK_PATH)/sources/cxx-stl/gnu-libstdc++/include
LOCAL_LDLIBS += -L$(NDK_PATH)/sources/cxx-stl/gnu-libstdc++/libs/$(TARGET_ARCH_ABI) -lstdc++

include $(BUILD_SHARED_LIBRARY)

include $(call all-subdir-makefiles)
