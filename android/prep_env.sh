#!/bin/bash

# TODO: create the whole dir struct first, i.e. get SDL2.0, get GemRb, create symlinks etc.
#       get rid of everything pelya if possible
#       figure out, what exactly openal needs

GEMRB_GIT_PATH=$1
ENVROOT=${2:-$1/android}
GEMRB_VERSION=""

function get_sources {
  local url=$1
  local expected=$2

  if [[ -z $expected ]]; then
    # use last component of url
    expected=${url##*/}
    expected=${expected%.git}
  fi

  if [[ -d $expected ]]; then
    cd $expected &&
    git pull
    rc=$?
    cd -
    return $rc
  else
    git clone $url $expected
  fi
}

function build_vorbis {
  echo -en "Checking out libogg-vorbis.\n"
  get_sources https://github.com/nwertzberger/libogg-vorbis-android.git &&
  echo -en "Building libogg-vorbis...\n" &&
  pushd "$ENVROOT/libogg-vorbis-android/main" &&
  ndk-build &&
  popd &&
  echo -en "Done with libogg-vorbis.\n"
}

function build_openal {
  # this still only works with a copied android.h from pelya/commandergenius
  # ifdef SDLVERSION somethingsomething in OpenALAudio solves this
  echo -en "Checking out openal.\n"
  get_sources http://repo.or.cz/openal-soft/android.git openal &&
  echo -en "Building openal...\n" &&
  pushd "$ENVROOT/openal/android" &&
  ndk-build &&
  popd &&
  echo -en "Done with openal.\n"
}

function build_libpng {
  echo -en "Checking out libpng...\n"
  get_sources https://github.com/julienr/libpng-android.git &&
  echo -en "Building libpng...\n" &&
  pushd "$ENVROOT/libpng-android" &&
  ndk-build &&
  popd &&
  echo -en "Done with libpng.\n"
}

function get_freetype {
  # can't precompile freetype, at least not as it comes from upstream
  get_sources https://github.com/cdave1/freetype2-android.git
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
  pushd "$ENVROOT" &&
  if [[ -d SDL ]]; then
    cd SDL
    hg update; rc=$?
    cd -
    (exit $rc) # hack to reset the hg return value
  else
    hg clone http://hg.libsdl.org/SDL
  fi &&
  # and do what it says in its README.android
  echo -en "Creating the directory structure for the project..." &&
  mkdir -p build &&
  cp -r "$ENVROOT/SDL/android-project" build/ &&
  cp -r "$ENVROOT/build/android-project" "$ENVROOT/build/gemrb" &&
  echo -en "Done.\n" &&
  echo -en "Symlinking the GemRB-git path..." &&
  mkdir -p "$ENVROOT/build/gemrb/jni" &&
  rm -fr "$ENVROOT/build/gemrb/jni/SDL" "$ENVROOT/build/gemrb/jni/src/main" &&
  ln -sf "$GEMRB_GIT_PATH" "$ENVROOT/build/gemrb/jni/src/main" &&
  ln -sf "$ENVROOT/SDL" "$ENVROOT/build/gemrb/jni/SDL"
}

function move_libraries {
  echo -en "Creating directories and copying Makefiles for prebuilt libraries...\n"

  # freetype2 is special, it's not supposed to be precompiled according to upstream,
  # additionally, upstream makefile is actually broken and we need makefiles in every
  # directory up to where the current one is stored, because ndk-build doesn't see
  # the right one otherwise
  # the alternative would probably be to store the makefile at the root of the
  # freetype directory, but im not sure in how far that messes with library placement
  cp -rf "$ENVROOT/freetype2-android" "$ENVROOT/build/gemrb/jni/" &&
  cp "$ENVROOT/FREETYPEBUILD_Android.mk" "$ENVROOT/build/gemrb/jni/freetype2-android/Android/jni/Android.mk" &&
  cp "$ENVROOT/RECURSE_Android.mk" "$ENVROOT/build/gemrb/jni/freetype2-android/Android/Android.mk" &&
  cp "$ENVROOT/RECURSE_Android.mk" "$ENVROOT/build/gemrb/jni/freetype2-android/Android.mk" &&
  # im not happy with this, but it's ok for now i guess

  mkdir -p build/gemrb/jni/{libogg,libvorbis,libpng,openal} &&
  cp "$ENVROOT/OGG_Android.mk" "$ENVROOT/build/gemrb/jni/libogg/Android.mk" &&
  cp "$ENVROOT/VORBIS_Android.mk" "$ENVROOT/build/gemrb/jni/libvorbis/Android.mk" &&
  cp "$ENVROOT/OPENAL_Android.mk" "$ENVROOT/build/gemrb/jni/openal/Android.mk" &&
  cp "$ENVROOT/PNG_Android.mk" "$ENVROOT/build/gemrb/jni/libpng/Android.mk" &&

  echo -en "Copying prebuilt libraries and linking header directories...\n" &&

  # libogg
  cp "$ENVROOT/libogg-vorbis-android/main/libs/armeabi/libogg.so" "$ENVROOT/build/gemrb/jni/libogg/" &&
  ln -sf "$ENVROOT/libogg-vorbis-android/main/jni/include/" "$ENVROOT/build/gemrb/jni/libogg/include" &&

  # vorbis
  cp "$ENVROOT/libogg-vorbis-android/main/libs/armeabi/libvorbis.so" "$ENVROOT/build/gemrb/jni/libvorbis/" &&
  ln -sf "$ENVROOT/libogg-vorbis-android/main/jni/include/" "$ENVROOT/build/gemrb/jni/libvorbis/include" &&
  # those two are a little bit messy, because they both need their include directory
  # this is because they can't both be defined as prebuilt libraries in the same makefile and directory,
  # because that messes with makefile variables for some reason

  # png
  cp "$ENVROOT/libpng-android/obj/local/armeabi/libpng.a" "$ENVROOT/build/gemrb/jni/libpng/" &&
  ln -sf "$ENVROOT/libpng-android/jni/" "$ENVROOT/build/gemrb/jni/libpng/include" &&

  # openal
  cp "$ENVROOT/openal/android/libs/armeabi/libopenal.so" "$ENVROOT/build/gemrb/jni/openal/" &&
  ln -sf "$ENVROOT/openal/include" "$ENVROOT/build/gemrb/jni/openal/include" &&

  # python
  libpython="libpython-py4a.tar.bz2"
  if [[ ! -f $libpython ]]; then
    wget "http://sourceforge.net/projects/gemrb/files/Other%20Binaries/android/$libpython" -P "$ENVROOT"
  fi &&
  tar -xf "$ENVROOT/$libpython" -C "$ENVROOT/build/gemrb/jni/" &&

  echo -en "Done.\n"
}

function prepare_config {
  template=$1
  out=$2

  cp -f "$template" "$out" &&
  # change/enable/override some defaults
  sed -i 's,^#UseSoftKeyboard,UseSoftKeyboard,' "$out" &&
  sed -i 's,^Bpp=.*,Bpp=16,' "$out" &&
  sed -i 's,^#\?AudioDriver.*,AudioDriver = openal,' "$out" &&
  sed -i 's,^Bpp=.*,Bpp=16,' "$out" &&
  sed -i 's,^#MouseFeedback=.*,MouseFeedback=3,' "$out" &&
  # unclear why these default clearings are needed
  # currently the activity doesn't do anything with them
  sed -i 's,@DATA_DIR@,,' "$out" &&
  sed -i 's,GameOverridePath=.*,GameOverridePath=,' "$out" &&
  sed -i 's,GameDataPath=.*,GameDataPath=,' "$out" &&
  # convenience and better defaults
  sed -i 's,SavePath=.*,SavePath=/sdcard/gemrb/bg2/,' "$out" &&
  sed -i 's,CachePath=.*,CachePath=/sdcard/gemrb/bg2/cache,' "$out" &&
  # replace the whole block
  sed -i '/^GamePath/,/^CD5/ c\
#GamePath=/storage/emulated/0/Android/data/net.sourceforge.gemrb/files/bg2/\
#CD1=/storage/emulated/0/Android/data/net.sourceforge.gemrb/files/bg2/data\
GamePath=/sdcard/gemrb/bg2\
CD1=/sdcard/gemrb/bg2/data\
# CD2=<CD2_PLACEHOLDER>\
# CD3=<CD3_PLACEHOLDER>\
# CD4=<CD4_PLACEHOLDER>\
# CD5=<CD5_PLACEHOLDER>\
' "$out"
}

function move_and_edit_projectfiles {
  echo -en "Copying and editing files..."
  mkdir -p "$ENVROOT/build/gemrb/src/net/sourceforge/gemrb/" &&

  # copy the gemrb activity
  cp "$ENVROOT/GemRB.java" "$ENVROOT/build/gemrb/src/net/sourceforge/gemrb/" &&

  # prepare and copy the config file
  mkdir -p "$ENVROOT/build/gemrb/assets" &&
  prepare_config "$GEMRB_GIT_PATH/gemrb/GemRB.cfg.sample.in" "$ENVROOT/packaged.GemRB.cfg" &&
  mv "$ENVROOT/packaged.GemRB.cfg" "$ENVROOT/build/gemrb/assets" &&

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

  sed -i -e 's,SDL_app,GemRB,' "$ENVROOT/build/gemrb/jni/SDL/src/main/android/SDL_android_main.c" &&
  sed -i -e 's,//exit,exit,' "$ENVROOT/build/gemrb/jni/SDL/src/main/android/SDL_android_main.c" &&

  # change activity class and application name, as well as enable debuggable
  sed -i -e s,org.libsdl.app,net.sourceforge.gemrb, "$ENVROOT/build/gemrb/AndroidManifest.xml" &&
  sed -i -e s,SDLActivity,GemRB, "$ENVROOT/build/gemrb/AndroidManifest.xml" &&
  sed -i -e 's/android:name="GemRB"\s*$/& android:screenOrientation="landscape"/' "$ENVROOT/build/gemrb/AndroidManifest.xml" &&
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
  popd # back from $ENVROOT
  local build_path=${ENVROOT##$PWD/}
  echo -en "That should be it, provided all the commands ran succesfully.\n\n" # TODO: Error checking beyond $1
  echo -en "To build:\n"
  echo -en "  cd $build_path/build/gemrb\n"
  echo -en "  ndk-build && ant debug\n\n"
  echo -en "alternatively, for ndk-gdb debuggable builds: \n"
  echo -en "  cd $build_path/build/gemrb\n"
  echo -en "  ndk-build NDK_DEBUG=1 && ant debug\n\n"
  echo -en "The finished apk will be $ENVROOT/build/gemrb/bin/SDLActivity-debug.apk\n\n"
}

# Flow control starts here

if [ -z "$1" ]
then
  echo -en "Error: No argument supplied.\n
Usage:
  $0 /absolute/path/to/gemrb/git (optional path to build dir)\n"
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
