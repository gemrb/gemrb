#!/bin/bash

# TODO: create the whole dir struct first, i.e. get SDL2.0, get GemRb, create symlinks etc.
#       get rid of everything pelya if possible
#       figure out, what exactly openal needs

ENVROOT=$PWD
GEMRB_GIT_PATH=$1
GEMRB_VERSION=""

function build_vorbis {
  echo -en "Checking out libogg-vorbis.\n"
  git clone git://github.com/jcadam/libogg-vorbis-android.git &&
  echo -en "Building libogg-vorbis...\n" &&
  pushd "$ENVROOT/libogg-vorbis-android" &&
  ndk-build &&
  popd &&
  echo -en "Done with libogg-vorbis.\n"
}

function build_openal {
  # this still only works with a copied android.h from pelya/commandergenius
  # ifdef SDLVERSION somethingsomething in OpenALAudio solves this
  echo -en "Checking out openal.\n"
  git clone git://repo.or.cz/openal-soft/android.git &&
  mv "$ENVROOT/android" "$ENVROOT/openal" && # why would they name it "android" :(
  echo -en "Building openal...\n" &&
  pushd "$ENVROOT/openal/android" &&
  ndk-build &&
  popd &&
  echo -en "Done with openal.\n"
}

function build_libpng {
  echo -en "Checking out libpng...\n"
  git clone git://github.com/julienr/libpng-android.git &&
  echo -en "Building libpng...\n" &&
  pushd "$ENVROOT/libpng-android" &&
  ndk-build &&
  popd &&
  echo -en "Done with libpng.\n"
}

function get_freetype {
  # can't precompile freetype, at least not as it comes from upstream
  git clone git://github.com/cdave1/freetype2-android.git
}

function build_deps {
  build_openal &&
  build_vorbis &&
  build_libpng &&
  get_freetype
}

function setup_dir_struct {
  echo -en "Checking out SDL2...\n"
  # get SDL2
  hg clone http://hg.libsdl.org/SDL &&
  # and do what it says in its README.android
  echo -en "Creating the directory structure for the project..." &&
  mkdir build &&
  cp -r "$ENVROOT/SDL/android-project" build/ &&
  mv "$ENVROOT/build/android-project" "$ENVROOT/build/gemrb" &&
  echo -en "Done.\n" &&
  echo -en "Symlinking the GemRB-git path..." &&
  ln -s "$GEMRB_GIT_PATH" "$ENVROOT/build/gemrb/jni/src/main" &&
  ln -s "$ENVROOT/SDL" "$ENVROOT/build/gemrb/jni/SDL"
}

function move_libraries {
  echo -en "Creating directories and copying Makefiles for prebuilt libraries...\n"

  # freetype2 is special, it's not supposed to be precompiled according to upstream,
  # additionally, upstream makefile is actually broken and we need makefiles in every
  # directory up to where the current one is stored, because ndk-build doesn't see
  # the right one otherwise
  # the alternative would probably be to store the makefile at the root of the
  # freetype directory, but im not sure in how far that messes with library placement
  cp -r "$ENVROOT/freetype2-android" "$ENVROOT/build/gemrb/jni/" &&
  cp "$ENVROOT/FREETYPEBUILD_Android.mk" "$ENVROOT/build/gemrb/jni/freetype2-android/Android/jni/Android.mk" &&
  cp "$ENVROOT/RECURSE_Android.mk" "$ENVROOT/build/gemrb/jni/freetype2-android/Android/Android.mk" &&
  cp "$ENVROOT/RECURSE_Android.mk" "$ENVROOT/build/gemrb/jni/freetype2-android/Android.mk" &&
  # im not happy with this, but it's ok for now i guess

  mkdir build/gemrb/jni/{libogg,libvorbis,libpng,openal} &&
  cp "$ENVROOT/OGG_Android.mk" "$ENVROOT/build/gemrb/jni/libogg/Android.mk" &&
  cp "$ENVROOT/VORBIS_Android.mk" "$ENVROOT/build/gemrb/jni/libvorbis/Android.mk" &&
  cp "$ENVROOT/OPENAL_Android.mk" "$ENVROOT/build/gemrb/jni/openal/Android.mk" &&
  cp "$ENVROOT/PNG_Android.mk" "$ENVROOT/build/gemrb/jni/libpng/Android.mk" &&

  echo -en "Copying prebuilt libraries and linking header directories...\n" &&

  # libogg
  cp "$ENVROOT/libogg-vorbis-android/libs/armeabi/libogg.so" "$ENVROOT/build/gemrb/jni/libogg/" &&
  ln -s "$ENVROOT/libogg-vorbis-android/jni/include/" "$ENVROOT/build/gemrb/jni/libogg/include" &&

  # vorbis
  cp "$ENVROOT/libogg-vorbis-android/libs/armeabi/libvorbis.so" "$ENVROOT/build/gemrb/jni/libvorbis/" &&
  ln -s "$ENVROOT/libogg-vorbis-android/jni/include/" "$ENVROOT/build/gemrb/jni/libvorbis/include" &&
  # those two are a little bit messy, because they both need their include directory
  # this is because they can't both be defined as prebuilt libraries in the same makefile and directory,
  # because that messes with makefile variables for some reason

  # png
  cp "$ENVROOT/libpng-android/obj/local/armeabi/libpng.a" "$ENVROOT/build/gemrb/jni/libpng/" &&
  ln -s "$ENVROOT/libpng-android/jni/" "$ENVROOT/build/gemrb/jni/libpng/include" &&

  # openal
  cp "$ENVROOT/openal/android/libs/armeabi/libopenal.so" "$ENVROOT/build/gemrb/jni/openal/" &&
  ln -s "$ENVROOT/openal/include" "$ENVROOT/build/gemrb/jni/openal/include" &&

  # python
  wget http://sourceforge.net/projects/gemrb/files/Other%20Binaries/android/libpython-2.6.2-pelya.tar.bz2 -O "$ENVROOT/libpython.tar" &&
  tar -xf "$ENVROOT/libpython.tar" -C "$ENVROOT/build/gemrb/jni/" &&

  echo -en "Done.\n"
}

function move_and_edit_projectfiles {
  echo -en "Copying and editing files..."
  mkdir -p "$ENVROOT/build/gemrb/src/net/sourceforge/gemrb/" &&

  # copy the gemrb activity
  cp "$ENVROOT/GemRB.java" "$ENVROOT/build/gemrb/src/net/sourceforge/gemrb/" &&

  # copy the packaged config file
  mkdir -p "$ENVROOT/build/gemrb/assets" &&
  cp "$ENVROOT/packaged.GemRB.cfg" "$ENVROOT/build/gemrb/assets" &&

  mkdir -p "$ENVROOT/build/gemrb/res/drawable-ldpi/" &&
  # copy the icons
  cp "$GEMRB_GIT_PATH/artwork/gemrb-logo-glow-36px.png" "$ENVROOT/build/gemrb/res/drawable-ldpi/ic_launcher.png" &&
  cp "$GEMRB_GIT_PATH/artwork/gemrb-logo-glow-48px.png" "$ENVROOT/build/gemrb/res/drawable-mdpi/ic_launcher.png" &&
  cp "$GEMRB_GIT_PATH/artwork/gemrb-logo-glow-72px.png" "$ENVROOT/build/gemrb/res/drawable-hdpi/ic_launcher.png" &&
  cp "$GEMRB_GIT_PATH/artwork/gemrb-logo-glow-96px.png" "$ENVROOT/build/gemrb/res/drawable-xhdpi/ic_launcher.png" &&
  cp "$GEMRB_GIT_PATH/artwork/gemrb-logo-glow-144px.png" "$ENVROOT/build/gemrb/res/drawable-xxhdpi/ic_launcher.png" &&

  # copy the makefile
  cp "$ENVROOT/GEMRB_Android.mk" "$ENVROOT/build/gemrb/jni/src/Android.mk" &&

  # and the Application.mk
  cp "$ENVROOT/GEMRB_Application.mk" "$ENVROOT/build/gemrb/jni/Application.mk" &&
  echo -en "Done.\n" &&

  # add the neccessary libraries to the base activity
  echo -en "Performing neccessary edits...\n" &&
  sed -i -e '/System.loadLibrary("SDL2")/ a\
          System.loadLibrary("ogg"); \
          System.loadLibrary("vorbis"); \
          System.loadLibrary("openal"); \
          System.loadLibrary("python");' "$ENVROOT/build/gemrb/src/org/libsdl/app/SDLActivity.java" &&

  sed -i -e 's,sdlFormat = 0x8,sdlFormat = 0x1,g' "$ENVROOT/build/gemrb/src/org/libsdl/app/SDLActivity.java" &&

  sed -i -e 's,SDL_app,GemRB,' "$ENVROOT/build/gemrb/jni/SDL/src/main/android/SDL_android_main.cpp" &&
  sed -i -e 's,//exit,exit,' "$ENVROOT/build/gemrb/jni/SDL/src/main/android/SDL_android_main.cpp" &&

  # change activity class and application name, as well as enable debuggable
  sed -i -e s,org.libsdl.app,net.sourceforge.gemrb, "$ENVROOT/build/gemrb/AndroidManifest.xml" &&
  sed -i -e s,SDLActivity,GemRB, "$ENVROOT/build/gemrb/AndroidManifest.xml" &&
  sed -i -e '/GemRB.*/ a android:screenOrientation="landscape" android:configChanges="orientation"' "$ENVROOT/build/gemrb/AndroidManifest.xml" &&
  sed -i -e s,android:versionName=.*,android:versionName=$GEMRB_VERSION, "$ENVROOT/build/gemrb/AndroidManifest.xml" &&
  sed -i -e '21 a\
                 android:debuggable="true"' "$ENVROOT/build/gemrb/AndroidManifest.xml" &&
  sed -i -e s,SDL\ App,GemRB, build/gemrb/res/values/strings.xml &&

  echo -en "Copying GemRB override, unhardcoded and GUIScripts folders..." &&
  mkdir -p "$ENVROOT/build/gemrb/assets" &&
  cp -r "$ENVROOT/build/gemrb/jni/src/main/gemrb/override" "$ENVROOT/build/gemrb/assets/" &&
  cp -r "$ENVROOT/build/gemrb/jni/src/main/gemrb/unhardcoded" "$ENVROOT/build/gemrb/assets/" &&
  cp -r "$ENVROOT/build/gemrb/jni/src/main/gemrb/GUIScripts" "$ENVROOT/build/gemrb/assets/" &&

  echo -en "Done.\n"
}

function finished {
  echo -en "That should be it, provided all the commands ran succesfully.\n\n" # TODO: Error checking beyond $1
  echo -en "To build:\n"
  echo -en "  cd build/gemrb\n"
  echo -en "  ndk-build && ant debug\n\n"
  echo -en "alternatively, for ndk-gdb debuggable builds: \n"
  echo -en "  cd build/gemrb\n"
  echo -en "  ndk-build NDK_DEBUG=1 && ant debug\n\n"
  echo -en "The finished apk will be $ENVROOT/build/gemrb/bin/SDLActivity-debug.apk\n\n"
}

# Flow control starts here

if [ -z "$1" ]
then
  echo -en "Error: No argument supplied.\n
Usage:
  $0 /absolute/path/to/gemrb/git\n"
  exit 1
fi

GEMRB_VERSION=$(grep "#define VERSION_GEMRB" "$GEMRB_GIT_PATH/gemrb/includes/globals.h" | awk -F' ' '{print $3}') &&
setup_dir_struct &&
move_and_edit_projectfiles &&
build_deps &&
move_libraries &&
android update project -t android-17 -p "$ENVROOT/build/gemrb" &&
finished || {
  echo 'Building failed!'
  exit 2
}
