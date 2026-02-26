AC_INIT(restream,0.01)
AM_INIT_AUTOMAKE([subdir-objects foreign])
CXXFLAGS="$CXXFLAGS -std=c++17"
AC_PROG_CC
AC_PROG_CXX
AC_CONFIG_HEADERS([config.hpp])
AC_CONFIG_SRCDIR([src/restream.cpp])
AC_CANONICAL_HOST
AC_CONFIG_MACRO_DIR([m4])
AC_LANG([C++])


##############################################################################
###  Check pkgconf  - Required.  Needed to get lib paths/info
##############################################################################
AC_CHECK_PROG([PKGCONFIG],[pkgconf],[yes],[no])
AS_IF([test "${PKGCONFIG}" = "no" ],[
    AC_MSG_ERROR([Required package 'pkgconf' not found.])
  ]
)

##############################################################################
###  Check pthread
##############################################################################
AC_CHECK_HEADERS(pthread.h,,AC_MSG_ERROR([pthread is required.]))
CPPFLAGS="${CPPFLAGS} -D_THREAD_SAFE "
LIBS="$LIBS -pthread "

##############################################################################
###  Check setting/getting thread names
##############################################################################
AC_CHECK_HEADERS(pthread_np.h,[PTHREAD_NP="yes"],[PTHREAD_NP="no"])

AC_MSG_CHECKING([for pthread_setname_np])
AC_LINK_IFELSE(
  [AC_LANG_PROGRAM([#include <pthread.h>], [pthread_setname_np(pthread_self(), "name")])
  ],[
    AC_DEFINE([HAVE_PTHREAD_SETNAME_NP], [1], [Define if you have pthread_setname_np function.])
    PTHREAD_SETNAME_NP="yes"
    AC_MSG_RESULT([yes])
  ],[
    PTHREAD_SETNAME_NP="no"
    AC_MSG_RESULT([no])
  ]
)

AC_MSG_CHECKING([for pthread_getname_np])
AC_LINK_IFELSE(
  [AC_LANG_PROGRAM([#include <pthread.h>], [pthread_getname_np(pthread_self(), NULL, 0)])
  ],[
    AC_DEFINE([HAVE_PTHREAD_GETNAME_NP], [1], [Define if you have pthread_getname_np function.])
    PTHREAD_GETNAME_NP="yes"
    AC_MSG_RESULT([yes])
  ],[
    PTHREAD_GETNAME_NP="no"
    AC_MSG_RESULT([no])
  ]
)

##############################################################################
###  Check libmicrohttpd - Required.  Needed for stream/webcontrol
##############################################################################
AS_IF([pkgconf libmicrohttpd ], [
    TEMP_CPPFLAGS="$TEMP_CPPFLAGS "`pkgconf --cflags libmicrohttpd`
    TEMP_LIBS="$TEMP_LIBS "`pkgconf --libs libmicrohttpd`
  ],[
    AC_MSG_ERROR([Required package libmicrohttpd-dev not found, please check and install necessary dependencies])
  ]
)
HOLD_CPPFLAGS="$CPPFLAGS"
CPPFLAGS="$CPPFLAGS $TEMP_CPPFLAGS"
AC_CHECK_HEADERS(microhttpd.h,[MHTTP="yes"],[MHTTP="no"])
AC_MSG_CHECKING(libmicrohttpd libraries)
AC_MSG_RESULT($MHTTP)
AS_IF([test "${MHTTP}" != "yes" ], [
    AC_MSG_ERROR([Required package libmicrohttpd-dev not found, please check and install necessary dependencies])
  ]
)
CPPFLAGS="$HOLD_CPPFLAGS"

##############################################################################
###  Check for ffmpeg
##############################################################################
FFMPEG_VER="--"
AC_ARG_WITH([ffmpeg],
  AS_HELP_STRING([--with-ffmpeg[=DIR]],[Build with FFMPEG support]),
  [FFMPEG=$withval],
  [FFMPEG="yes"]
)

AS_IF([test "${FFMPEG}" = "no"], [
    AC_MSG_CHECKING(for FFmpeg)
    AC_MSG_ERROR([Required ffmpeg packages were not found.])
  ],[
    AC_MSG_CHECKING(FFmpeg pkgconf path)
    TEMP_PATH=$PKG_CONFIG_PATH
    AS_IF([test "${FFMPEG}" != "yes"], [
        PKG_CONFIG_PATH=${FFMPEG}/lib/pkgconfig:$PKG_CONFIG_PATH
        FFMPEG="yes"
      ]
    )
    export PKG_CONFIG_PATH
    AC_MSG_RESULT($PKG_CONFIG_PATH)

    FFMPEG_DEPS="libavdevice libavformat libavcodec libswresample libswscale libavutil"
    AC_MSG_CHECKING(for FFmpeg)
    AS_IF([pkgconf $FFMPEG_DEPS], [
        FFMPEG_VER=`pkgconf --modversion libavformat`
        TEMP_CPPFLAGS=`pkgconf --cflags $FFMPEG_DEPS`" $TEMP_CPPFLAGS "
        TEMP_LIBS=`pkgconf --libs $FFMPEG_DEPS`" $TEMP_LIBS "
        AC_DEFINE([HAVE_FFMPEG], [1], [Define to 1 if FFMPEG is around])
        AC_MSG_RESULT(yes)
      ],[
        AC_MSG_RESULT(no)
        AC_MSG_ERROR([Required ffmpeg packages were not found.])
      ]
    )
    PKG_CONFIG_PATH=$TEMP_PATH
    export PKG_CONFIG_PATH
  ]
)

TEMP_CPPFLAGS="$TEMP_CPPFLAGS -W -Wall -Werror -Wextra -Wformat -Wshadow -Wpointer-arith -Wwrite-strings -Winline -Wredundant-decls -Wno-long-long -ggdb -g3"

CPPFLAGS="$CPPFLAGS $TEMP_CPPFLAGS"
LIBS="$LIBS $TEMP_LIBS"
LDFLAGS="$TEMP_LDFLAGS"

AC_CONFIG_FILES([
	Makefile
	src/Makefile
])

AC_OUTPUT

##############################################################################
###  Report results to user
##############################################################################
echo ""
echo "   **************************"
echo "      Configure status       "
echo "      ${PACKAGE_NAME} ${PACKAGE_VERSION}"
echo "   **************************"
echo
echo "CPPFLAGS: $CPPFLAGS"
echo
echo "LIBS: $LIBS"
echo
echo "LDFLAGS: $TEMP_LDFLAGS $LDFLAGS"
echo
echo  "Install prefix:       $prefix"
echo
