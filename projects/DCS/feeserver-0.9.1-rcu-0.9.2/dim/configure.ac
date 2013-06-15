dnl
dnl
dnl
dnl ------------------------------------------------------------------
AC_DEFUN([AC_DEBUG],
[
  AC_REQUIRE([AC_PROG_CC])
  AC_MSG_CHECKING(whether to make debug objects)
  AC_ARG_ENABLE(debug,
    [AC_HELP_STRING([--enable-debug],[Enable debugging symbols in objects])],
    [],[enable_debug=yes])
  if test "x$enable_debug" = "xno" ; then
    CFLAGS=`echo $CFLAGS | sed 's,-g,,'`
  else
    case $CFLAGS in
    *-g*) ;;
    *)    CFLAGS="$CFLAGS -g" ;;
    esac
  fi
  AC_MSG_RESULT($enable_debug 'CFLAGS=$CFLAGS')
])

dnl ------------------------------------------------------------------
AC_DEFUN([AC_OPTIMIZATION],
[
  AC_REQUIRE([AC_PROG_CC])
  AC_ARG_ENABLE(optimization,
    [AC_HELP_STRING([--enable-optimization],[Enable optimization of objects])],
    [],[enable_optimization=yes])
  AC_MSG_CHECKING(for optimiztion level)
  changequote(<<, >>)dnl
  if test "x$enable_optimization" = "xno" ; then
    CFLAGS=`echo   $CFLAGS   | sed 's,-O\([0-9][0-9]*\|\),,'`
  elif test "x$enable_optimization" = "xyes" ; then
    case $CFLAGS in
    *-O*) ;;
    *)    CFLAGS="$CXXFLAGS -O2" ;;
    esac
  else
    CFLAGS=`echo $CFLAGS | sed "s,-O\([0-9][0-9]*\|\),-O$enable_optimization,"`
  fi
  changequote([, ])dnl
  AC_MSG_RESULT($enable_optimization 'CFLAGS=$CFLAGS')
])

dnl __________________________________________________________________
AC_INIT([Distributed Information Management System], [16.09], [Clara.Gaspar@cern.ch], dim)
AC_PREREQ(2.50)
AC_PREFIX_DEFAULT(${HOME})
AC_CANONICAL_TARGET
AM_INIT_AUTOMAKE([foreign])

dnl
dnl __________________________________________________________________
AC_PROG_CC
AC_PROG_CXX
AC_PROG_LIBTOOL

dnl ------------------------------------------------------------------
AH_TEMPLATE(MIPSEB, [Big-endian machine])
AH_TEMPLATE(MIPSEL, [Little-endian machine])
AC_C_BIGENDIAN([AC_DEFINE([MIPSEB])],[AC_DEFINE([MIPSEL])])
AC_DEFINE([PROTOCOL],[1])
case $host_os:$host_cpu in
sun*)		AC_DEFINE([sunos])		;;
solaris*)       AC_DEFINE([solaris])	
		LIBS="$LIBS -lsocket -lnsl"	;;
hp-ux*)		AC_DEFINE([hpux])		;;
osf*)		AC_DEFINE([osf])		;;
aix*)		AC_DEFINE([aix])	
		AC_DEFINE([unix])
		AC_DEFINE([_BSD])		;;
lynxos*:rs6000)	AC_DEFINE([LYNXOS])	
		AC_DEFINE([RAID])
		AC_DEFINE([unix])		
		CPPFLAGS="$CPPFLAGS -I/usr/include/bsd -I/usr/include/posix"
		LDFLAGS="$LDFLAGS -L/usr/posix/usr/lib"
		LIBS="$LIBS -lbsd"		;;
lynxos*:*86*)	AC_DEFINE([LYNXOS])	
		AC_DEFINE([unix])		
		LIBS="$LIBS -lbsd -llynx"	;;
lynxos*)	AC_DEFINE([LYNXOS])	
		AC_DEFINE([unix])		
		LIBS="$LIBS -lbsd"		;;
linux*)		AC_DEFINE([linux])		
		AC_DEFINE([unix])		;;
esac

dnl
dnl __________________________________________________________________
AC_ARG_ENABLE([cxx-api],
              [AC_HELP_STRING([--enable-cxx-api],
                              [Also generate the C++ API])])
AC_MSG_CHECKING([whether to enable C++ API])
if test ! "x$enable_cxx_api" = "xno" ; then
   enable_cxx_api=yes
fi
AC_MSG_RESULT([$enable_cxx_api])
AM_CONDITIONAL([CXXDIM],test "x$enable_cxx_api" = "xyes")
AC_SUBST(enable_cxx_api)

dnl __________________________________________________________________
AC_ARG_ENABLE([threads],
              [AC_HELP_STRING([--enable-threads],
                              [Make the library thread-safe])],
			      [], [enable_threads=no])
AC_MSG_CHECKING([whether make the library thread safe])
if test ! "x$enable_threads" = "xno" ; then
   enable_threads=yes
   case $host_os:$host_cpu in
   solaris*|sun*)	   CFLAGS="$CFLAGS -mt"
			   LIBS="$LIBS -lposix4"	;;
   hp-ux*|osf*|aix*)	   enable_threads=no		;;
   # The DIM libary should not be threaded on an ARM chip, even though
   # FeeServer uses threads and links against the thread library
   # The dim library must be compiled with the NOTHREADS flag to avoid
   # deadlocks
   linux*:arm*)		   enable_threads=no            ;;
   linux*)		   LIBS="$LIBS -pthread"	;;
   lynxos*:rs6000)	   CFLAGS="$CFLAGS -mthreads"	;;
   *)			   LIBS="$LIBS -lpthread"	;;
   esac
fi
if test "x$enable_threads" = "xno" ; then
   AC_DEFINE([NOTHREADS])
fi
AC_MSG_RESULT([$enable_threads])

dnl
dnl __________________________________________________________________
AC_DEBUG
AC_OPTIMIZATION

dnl __________________________________________________________________
AC_CONFIG_FILES([Makefile])

AC_OUTPUT
dnl
dnl EOF
dnl

