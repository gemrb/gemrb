# -*-autoconf-*-
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

###################################################
dnl Configure paths for SDL
dnl Edheldil & Subvertir - 12/09/03
dnl Sam Lantinga 9/21/99
dnl stolen from Manish Singh
dnl stolen back from Frank Belew
dnl stolen from Manish Singh
dnl Shamelessly stolen from Owen Taylor

dnl AM_PATH_SDL([MINIMUM-VERSION, [ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]]])
dnl Test for SDL, and define SDL_CFLAGS and SDL_LIBS
dnl

AC_DEFUN([AM_PATH_SDL],
[dnl 
dnl Get the cflags and libraries from the sdl-config script
dnl
AC_ARG_WITH(sdl-prefix,[  --with-sdl-prefix=PFX   Prefix where SDL is installed (optional)],
            sdl_prefix="$withval", sdl_prefix="")
AC_ARG_WITH(sdl-exec-prefix,[  --with-sdl-exec-prefix=PFX Exec prefix where SDL is installed (optional)],
            sdl_exec_prefix="$withval", sdl_exec_prefix="")
AC_ARG_ENABLE(sdltest, [  --disable-sdltest       Do not try to compile and run a test SDL program],
		    , enable_sdltest=yes)

  if test x$sdl_exec_prefix != x ; then
     sdl_args="$sdl_args --exec-prefix=$sdl_exec_prefix"
     if test x${SDL_CONFIG+set} != xset ; then
        SDL_CONFIG=$sdl_exec_prefix/bin/sdl-config
     fi
  fi
  if test x$sdl_prefix != x ; then
     sdl_args="$sdl_args --prefix=$sdl_prefix"
     if test x${SDL_CONFIG+set} != xset ; then
        SDL_CONFIG=$sdl_prefix/bin/sdl-config
     fi
  fi

  AC_REQUIRE([AC_CANONICAL_TARGET])
  PATH="$prefix/bin:$prefix/usr/bin:$PATH"
  AC_PATH_PROG(SDL_CONFIG, sdl-config, no, [$PATH])
  min_sdl_version=ifelse([$1], ,0.11.0,$1)
  AC_MSG_CHECKING(for SDL - version >= $min_sdl_version)
  no_sdl=""
  if test "$SDL_CONFIG" = "no" ; then
    no_sdl=yes
  else
    SDL_CFLAGS=`$SDL_CONFIG $sdlconf_args --cflags`
    SDL_LIBS=`$SDL_CONFIG $sdlconf_args --libs`

    sdl_major_version=`$SDL_CONFIG $sdl_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\1/'`
    sdl_minor_version=`$SDL_CONFIG $sdl_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\2/'`
    sdl_micro_version=`$SDL_CONFIG $sdl_config_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\3/'`
    if test "x$enable_sdltest" = "xyes" ; then
      ac_save_CFLAGS="$CFLAGS"
      ac_save_LIBS="$LIBS"
      CFLAGS="$CFLAGS $SDL_CFLAGS"
      CXXFLAGS="$CXXFLAGS $SDL_CFLAGS"
      LIBS="$LIBS $SDL_LIBS"
dnl
dnl Now check if the installed SDL is sufficiently new. (Also sanity
dnl checks the results of sdl-config to some extent
dnl
      rm -f conf.sdltest
      AC_TRY_RUN([
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "SDL.h"

char*
my_strdup (char *str)
{
  char *new_str;
  
  if (str)
    {
      new_str = (char *)malloc ((strlen (str) + 1) * sizeof(char));
      strcpy (new_str, str);
    }
  else
    new_str = NULL;
  
  return new_str;
}

int main (int argc, char *argv[])
{
  int major, minor, micro;
  char *tmp_version;

  /* This hangs on some systems (?)
  system ("touch conf.sdltest");
  */
  { FILE *fp = fopen("conf.sdltest", "a"); if ( fp ) fclose(fp); }

  /* HP/UX 9 (%@#!) writes to sscanf strings */
  tmp_version = my_strdup("$min_sdl_version");
  if (sscanf(tmp_version, "%d.%d.%d", &major, &minor, &micro) != 3) {
     printf("%s, bad version string\n", "$min_sdl_version");
     exit(1);
   }

   if (($sdl_major_version > major) ||
      (($sdl_major_version == major) && ($sdl_minor_version > minor)) ||
      (($sdl_major_version == major) && ($sdl_minor_version == minor) && ($sdl_micro_version >= micro)))
    {
      return 0;
    }
  else
    {
      printf("\n*** 'sdl-config --version' returned %d.%d.%d, but the minimum version\n", $sdl_major_version, $sdl_minor_version, $sdl_micro_version);
      printf("*** of SDL required is %d.%d.%d. If sdl-config is correct, then it is\n", major, minor, micro);
      printf("*** best to upgrade to the required version.\n");
      printf("*** If sdl-config was wrong, set the environment variable SDL_CONFIG\n");
      printf("*** to point to the correct copy of sdl-config, and remove the file\n");
      printf("*** config.cache before re-running configure\n");
      return 1;
    }
}

],, no_sdl=yes,[echo $ac_n "cross compiling; assumed OK... $ac_c"])
       CFLAGS="$ac_save_CFLAGS"
       LIBS="$ac_save_LIBS"
     fi
  fi
  if test "x$no_sdl" = x ; then
     AC_MSG_RESULT(yes)
     ifelse([$2], , :, [$2])     
  else
     AC_MSG_RESULT(no)
     if test "$SDL_CONFIG" = "no" ; then
       echo "*** The sdl-config script installed by SDL could not be found"
       echo "*** If SDL was installed in PREFIX, make sure PREFIX/bin is in"
       echo "*** your path, or set the SDL_CONFIG environment variable to the"
       echo "*** full path to sdl-config."
     else
       if test -f conf.sdltest ; then
        :
       else
          echo "*** Could not run SDL test program, checking why..."
          CFLAGS="$CFLAGS $SDL_CFLAGS"
          LIBS="$LIBS $SDL_LIBS"
          AC_TRY_LINK([
#include <stdio.h>
#include "SDL.h"

int main(int argc, char *argv[])
{ return 0; }
#undef  main
#define main K_and_R_C_main
],      [ return 0; ],
        [ echo "*** The test program compiled, but did not run. This usually means"
          echo "*** that the run-time linker is not finding SDL or finding the wrong"
          echo "*** version of SDL. If it is not finding SDL, you'll need to set your"
          echo "*** LD_LIBRARY_PATH environment variable, or edit /etc/ld.so.conf to point"
          echo "*** to the installed location  Also, make sure you have run ldconfig if that"
          echo "*** is required on your system"
	  echo "***"
          echo "*** If you have an old version installed, it is best to remove it, although"
          echo "*** you may also be able to get things to work by modifying LD_LIBRARY_PATH"],
        [ echo "*** The test program failed to compile or link. See the file config.log for the"
          echo "*** exact error that occured. This usually means SDL was incorrectly installed"
          echo "*** or that you have moved SDL since it was installed. In the latter case, you"
          echo "*** may want to edit the sdl-config script: $SDL_CONFIG" ])
          CFLAGS="$ac_save_CFLAGS"
          LIBS="$ac_save_LIBS"
       fi
     fi
     SDL_CFLAGS=""
     SDL_LIBS=""
     ifelse([$3], , :, [$3])
  fi
  AC_SUBST(SDL_CFLAGS)
  AC_SUBST(SDL_LIBS)
  rm -f conf.sdltest
])

###################################################
dnl Configure paths for OPENAL
dnl AM_PATH_OPENAL([ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]])
dnl Test for OPENAL, and define OPENAL_CFLAGS and OPENAL_LIBS
dnl

AC_DEFUN([AM_PATH_OPENAL],
[dnl 
dnl Get the cflags and libraries from the openal-config script
dnl
AC_ARG_WITH(openal-prefix,[  --with-openal-prefix=PFX   Prefix where OPENAL is installed (optional)],
            openal_prefix="$withval", openal_prefix="")
AC_ARG_WITH(openal-exec-prefix,[  --with-openal-exec-prefix=PFX Exec prefix where OPENAL is installed (optional)],
            openal_exec_prefix="$withval", openal_exec_prefix="")
AC_ARG_ENABLE(openaltest, [  --disable-openaltest       Do not try to compile and run a test OPENAL program],
		    , enable_openaltest=yes)

  if test x$openal_exec_prefix != x ; then
     openal_args="$openal_args --exec-prefix=$openal_exec_prefix"
     if test x${OPENAL_CONFIG+set} != xset ; then
        OPENAL_CONFIG=$openal_exec_prefix/bin/openal-config
     fi
  fi
  if test x$openal_prefix != x ; then
     openal_args="$openal_args --prefix=$openal_prefix"
     if test x${OPENAL_CONFIG+set} != xset ; then
        OPENAL_CONFIG=$openal_prefix/bin/openal-config
     fi
  fi

  AC_REQUIRE([AC_CANONICAL_TARGET])
  PATH="$prefix/bin:$prefix/usr/bin:$PATH"
  AC_PATH_PROG(OPENAL_CONFIG, openal-config, no, [$PATH])
  AC_PATH_PROG(PKGCONFIG, pkg-config, no, [$PATH])
  AC_MSG_CHECKING(for OPENAL library)
  no_openal=""
  if test "$OPENAL_CONFIG" = "no" && test "$PKGCONFIG" = "no" ; then
    no_openal=yes
  else
    if test "$OPENAL_CONFIG" = "no" ; then
      OPENAL_CFLAGS=`$PKGCONFIG openal --cflags`
      OPENAL_LIBS="`$PKGCONFIG openal --libs` $LIBPTHREAD"
    else
      OPENAL_CFLAGS=`$OPENAL_CONFIG $openalconf_args --cflags`
      OPENAL_LIBS="`$OPENAL_CONFIG $openalconf_args --libs` $LIBPTHREAD"
    fi

    if test "x$enable_openaltest" = "xyes" ; then
      ac_save_CFLAGS="$CFLAGS"
      ac_save_LIBS="$LIBS"
      CFLAGS="$CFLAGS $OPENAL_CFLAGS"
      CXXFLAGS="$CXXFLAGS $OPENAL_CFLAGS"
      LIBS="$LIBS $OPENAL_LIBS"
dnl
dnl Now check if the installed OPENAL is sufficiently new.
dnl
      rm -f conf.openaltest
      AC_TRY_LINK([
#include "AL/al.h"
],[ return alGetError(); ],no_openal="",no_openal=yes)
       CFLAGS="$ac_save_CFLAGS"
       LIBS="$ac_save_LIBS"
     fi
  fi
  if test "x$no_openal" = x ; then
     AC_MSG_RESULT(yes)
     ifelse([$1], , :, [$1])     
  else
     AC_MSG_RESULT(no)
     echo "*** The test program failed to compile or link. See the file config.log for the"
     echo "*** exact error that occured. This usually means OPENAL is not installed,"
     echo "*** that it's installed incorrectly or that it has been moved since"
     echo "*** installation. In the latter case, you may want to edit the "
     echo "*** openal-config script: $OPENAL_CONFIG"
     OPENAL_CFLAGS=""
     OPENAL_LIBS=""
     ifelse([$2], , :, [$2])
  fi
  AC_SUBST(OPENAL_CFLAGS)
  AC_SUBST(OPENAL_LIBS)
  rm -f conf.openaltest
])



###################################################
dnl Configure paths for python
dnl Shamelessly ripped from dia 0.92
dnl From Andrew Dalke
dnl Updated by James Henstridge

AC_DEFUN([AM_PATH_PYTHON],
 [
  dnl Find a version of Python.  I could check for python versions 1.4
  dnl or earlier, but the default installation locations changed from
  dnl $prefix/lib/site-python in 1.4 to $prefix/lib/python1.5/site-packages
  dnl in 1.5, and I don't want to maintain that logic.

  if test -z "$PYTHON"; then
     AC_PATH_PROGS(PYTHON, python python2.3 python2.2 python2.1 python2.0 python1.6 python1.5)
  fi

  dnl should we do the version check?
  ifelse([$1],[],,[
    AC_MSG_CHECKING(if Python version >= $1)
    changequote(<<, >>)dnl
    prog="
import sys, string
minver = '$1'
pyver = string.split(sys.version)[0]  # first word is version string
# split strings by '.' and convert to numeric
minver = map(string.atoi, string.split(minver, '.'))
if hasattr(sys, 'version_info'):
    pyver = sys.version_info[:3]
else:
    pyver = map(string.atoi, string.split(pyver, '.'))
# we can now do comparisons on the two lists:
if tuple(pyver) >= tuple(minver):
        sys.exit(0)
else:
        sys.exit(1)"
    changequote([, ])dnl
    if $PYTHON -c "$prog" 1>&AC_FD_CC 2>&AC_FD_CC
    then
      AC_MSG_RESULT(okay)
      $2
    else
      AC_MSG_RESULT(too old)
      $3
    fi
  ])

  AC_MSG_CHECKING([local Python configuration])
  echo

  dnl Query Python for its version number.  Getting [:3] seems to be
  dnl the best way to do this; it's what "site.py" does in the standard
  dnl library.  Need to change quote character because of [:3]

  AC_SUBST(PYTHON_VERSION)
  changequote(<<, >>)dnl
  PYTHON_VERSION=`$PYTHON -c "import sys; print sys.version[:3]"`
  changequote([, ])dnl


  dnl Use the values of $prefix and $exec_prefix for the corresponding
  dnl values of PYTHON_PREFIX and PYTHON_EXEC_PREFIX.  These are made
  dnl distinct variables so they can be overridden if need be.  However,
  dnl general consensus is that you shouldn't need this ability.

  AC_SUBST(PYTHON_PREFIX)
  PYTHON_PREFIX='${prefix}'

  AC_SUBST(PYTHON_EXEC_PREFIX)
  PYTHON_EXEC_PREFIX='${exec_prefix}'


])

###################################################
dnl Macro to check for availability of python headers
dnl  AM_CHECK_PYTHON_HEADERS([ACTION-IF-POSSIBLE], [ACTION-IF-NOT-POSSIBLE])
dnl function also defines PYTHON_INCLUDES

AC_DEFUN([AM_CHECK_PYTHON_HEADERS],
[AC_REQUIRE([AM_PATH_PYTHON])
AC_MSG_CHECKING(for python headers)
dnl deduce PYTHON_INCLUDES
py_prefix=`$PYTHON -c "import sys; print sys.prefix"`
py_exec_prefix=`$PYTHON -c "import sys; print sys.exec_prefix"`
PYTHON_INCLUDES="-I${py_prefix}/include/python${PYTHON_VERSION}"
if test "$py_prefix" != "$py_exec_prefix"; then
  PYTHON_INCLUDES="$PYTHON_INCLUDES -I${py_exec_prefix}/include/python${PYTHON_VERSION}"
fi
AC_SUBST(PYTHON_INCLUDES)
dnl check if the headers exist:
save_CPPFLAGS="$CPPFLAGS"
CPPFLAGS="$CPPFLAGS $PYTHON_INCLUDES"
AC_TRY_CPP([#include <Python.h>],dnl
[AC_MSG_RESULT(found)
$1],dnl
[AC_MSG_RESULT(not found)
$2])
CPPFLAGS="$save_CPPFLAGS"
])

###################################################
dnl Macro to check for availability of python libraries
dnl  AM_CHECK_PYTHON_LIBS([ACTION-IF-POSSIBLE], [ACTION-IF-NOT-POSSIBLE])
dnl function also defines PYTHON_LIBS

AC_DEFUN([AM_CHECK_PYTHON_LIBS],
[AC_REQUIRE([AM_PATH_PYTHON])
AC_MSG_CHECKING(for python libraries)
dnl deduce PYTHON_LIBS
py_prefix=`$PYTHON -c "import sys; print sys.prefix"`
py_exec_prefix=`$PYTHON -c "import sys; print sys.exec_prefix"`
PYTHON_LIBS="-L${py_prefix}/lib -lpython${PYTHON_VERSION}"
if test "$py_prefix" != "$py_exec_prefix"; then
  PYTHON_LIBS="$PYTHON_LIBS -L${py_exec_prefix}/lib -lpython${PYTHON_VERSION}"
fi
AC_SUBST(PYTHON_LIBS)
dnl check if the lib links:
save_CPPFLAGS="$CPPFLAGS"
CPPFLAGS="$CPPFLAGS $PYTHON_INCLUDES"
save_LIBS="$LIBS"
LIBS="$LIBS $PYTHON_LIBS $LIBPTHREAD"
AC_TRY_LINK([#include <Python.h>],[
Py_Initialize();
],dnl
[AC_MSG_RESULT(found)
$1],dnl
[AC_MSG_RESULT(not found)
$2])
LIBS="$save_LIBS"
CPPFLAGS="$save_CPPFLAGS"
])


###################################################
dnl Check for ZLib (gzip compression) library
dnl Available from the GNU Autoconf Macro Archive at:
dnl http://www.gnu.org/software/ac-archive/htmldoc/check_zlib.html
dnl

AC_DEFUN([CHECK_ZLIB],
#
# Handle user hints
#
[AC_MSG_CHECKING(if zlib is wanted)
AC_ARG_WITH(zlib,
[  --with-zlib=DIR root directory path of zlib installation [defaults to
                    /usr/local or /usr if not found in /usr/local]
  --without-zlib to disable zlib usage completely],
[if test "$withval" != no ; then
  AC_MSG_RESULT(yes)
  ZLIB_HOME="$withval"
else
  AC_MSG_RESULT(no)
fi], [
AC_MSG_RESULT(yes)
ZLIB_HOME=/usr/local
if test ! -f "${ZLIB_HOME}/include/zlib.h"
then
        ZLIB_HOME=/usr
fi
])

#
# Locate zlib, if wanted
#
if test -n "${ZLIB_HOME}"
then
        ZLIB_OLD_LDFLAGS=$LDFLAGS
        ZLIB_OLD_CPPFLAGS=$LDFLAGS
        LDFLAGS="$LDFLAGS -L${ZLIB_HOME}/lib"
        CPPFLAGS="$CPPFLAGS -I${ZLIB_HOME}/include"
        AC_LANG_SAVE
        AC_LANG_C
        AC_CHECK_LIB(z, inflateEnd, [zlib_cv_libz=yes], [zlib_cv_libz=no])
        AC_CHECK_HEADER(zlib.h, [zlib_cv_zlib_h=yes], [zlib_cvs_zlib_h=no])
        AC_LANG_RESTORE
        if test "$zlib_cv_libz" = "yes" -a "$zlib_cv_zlib_h" = "yes"
        then
                #
                # If both library and header were found, use them
                #
                AC_CHECK_LIB(z, inflateEnd)
                AC_MSG_CHECKING(zlib in ${ZLIB_HOME})
                AC_MSG_RESULT(ok)
        else
                #
                # If either header or library was not found, revert and bomb
                #
                AC_MSG_CHECKING(zlib in ${ZLIB_HOME})
                LDFLAGS="$ZLIB_OLD_LDFLAGS"
                CPPFLAGS="$ZLIB_OLD_CPPFLAGS"
                AC_MSG_RESULT(failed)
                AC_MSG_ERROR(either specify a valid zlib installation with --with-zlib=DIR or disable zlib usage with --without-zlib)
        fi
fi

])


###################################################
dnl Test whether STL library defines method container::at().
dnl Older versions (e.g. 2.95.x on Debian) don't and newer (3.x) do
dnl Syntax: AC_CHECK_STL_CONTAINER_AT([ACTION-IF-YES], [ACTION-IF-NO])

AC_DEFUN([AC_CHECK_STL_CONTAINER_AT],
[
AC_MSG_CHECKING(for container::at)
AC_TRY_COMPILE(
[
#include <vector>
#include <deque>
#include <string>

using namespace std;
],
[
deque<int> test_deque(3);
test_deque.at(2);
vector<int> test_vector(2);
test_vector.at(1);
string test_string("test_string");
test_string.at(3);
],
[AC_MSG_RESULT(yes)
dnl AC_DEFINE(HAVE_CONTAINER_AT)
$1],
[AC_MSG_RESULT(no)
$2])
])

###################################################
dnl Test whether the compiler permits casting from pointer-to-object 
dnl to pointer-to-function (forbidden in GCC v4 and ISO C++).
dnl If the cast is forbidden, define HAVE_FORBIDDEN_OBJECT_TO_FUNCTION_CAST.
dnl Syntax: AC_CHECK_OBJECT_TO_FUNCTION_CAST()

AC_DEFUN([AC_CHECK_OBJECT_TO_FUNCTION_CAST],
[
SAVE_CXXFLAGS="$CXXFLAGS"
CXXFLAGS="$CXXFLAGS -Werror"
AC_MSG_CHECKING(whether compiler permits casting between ptr-to-object and ptr-to-function)
AC_TRY_COMPILE(
[
typedef void *(* voidvoid)(void);
],
[
void *object = 0;
voidvoid function;
function = (voidvoid) object;
],
[AC_MSG_RESULT(yes)
],
[AC_MSG_RESULT(no)
AC_DEFINE(HAVE_FORBIDDEN_OBJECT_TO_FUNCTION_CAST, 1, [Define to 1 if compiler forbids casting between pointer-to-function and pointer-to-object])
])
CXXFLAGS="$SAVE_CXXFLAGS"
])


###################################################
dnl Check for the name of Posix threads library.
dnl Ripped from XMMS by Peter Alm & co and modified to integrate w/ GemRB

AC_DEFUN([AC_CHECK_PTHREADS],
[
LIBPTHREAD=error
AC_MSG_CHECKING(for old style FreeBSD -pthread flag)
AC_EGREP_CPP(yes,
        [#if (defined(__FreeBSD_cc_version) && __FreeBSD_cc_version <= 500001) || defined(__OpenBSD__)
          yes
        #endif
        ], AC_MSG_RESULT(yes)
        CXXFLAGS="$CXXFLAGS -D_THREAD_SAFE"
        LIBPTHREAD="-pthread",
        AC_MSG_RESULT(no))
if test "x$LIBPTHREAD" = xerror; then
        AC_CHECK_LIB(pthread, pthread_attr_init,
                LIBPTHREAD="-lpthread")
fi
if test "x$LIBPTHREAD" = xerror; then
        AC_CHECK_LIB(pthreads, pthread_attr_init,
                LIBPTHREAD="-lpthreads")
fi
if test "x$LIBPTHREAD" = xerror; then
        AC_CHECK_LIB(c_r, pthread_attr_init,
                LIBPTHREAD="-lc_r")
fi
if test "x$LIBPTHREAD" = xerror; then
        AC_CHECK_FUNC(pthread_attr_init, LIBPTHREAD="")
fi
if test "x$LIBPTHREAD" = xerror; then
        AC_MSG_ERROR(*** Unable to locate working posix thread library)
fi
AC_SUBST(LIBPTHREAD)
])

