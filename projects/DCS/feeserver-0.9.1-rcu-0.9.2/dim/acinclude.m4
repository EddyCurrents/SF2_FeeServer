dnl
dnl $Id: acinclude.m4,v 1.4 2006/07/11 21:29:01 richter Exp $
dnl
dnl  Copyright (C) 2002 Christian Holm Christensen <cholm@nbi.dk>
dnl
dnl  This library is free software; you can redistribute it and/or
dnl  modify it under the terms of the GNU Lesser General Public License
dnl  as published by the Free Software Foundation; either version 2.1
dnl  of the License, or (at your option) any later version.
dnl
dnl  This library is distributed in the hope that it will be useful,
dnl  but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
dnl  Lesser General Public License for more details.
dnl
dnl  You should have received a copy of the GNU Lesser General Public
dnl  License along with this library; if not, write to the Free
dnl  Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
dnl  02111-1307 USA
dnl
dnl ------------------------------------------------------------------
AC_DEFUN([AC_THREAD_FLAGS],
[
  dnl ------------------------------------------------------------------
  dnl Thread flags to use 
  case $host_os:$host_cpu in
  solaris*|sun*)	  CFLAGS="$CFLAGS -mt"
			  LIBS="$LIBS -lposix4"		;;
  hp-ux*|osf*|aix*)					;;
  # The DIM libary should not be threaded on an ARM chip, but the
  # FeeServer  assumes that it is so we take the next line out and
  # default to normal Linux
  # linux*:arm*)					;;
  linux*)		  LIBS="$LIBS -pthread"		;;
  lynxos*:rs6000)	  CFLAGS="$CFLAGS -mthreads"	;;
  *)			  LIBS="$LIBS -lpthread"	;;
  esac
])

AC_DEFUN([AC_DIM_ARCH],
[
  AC_REQUIRE([AC_PROG_CC])
  dnl ------------------------------------------------------------------
  dnl Byte order 
  AH_TEMPLATE(MIPSEB, [Big-endian machine])
  AH_TEMPLATE(MIPSEL, [Little-endian machine])
  AC_C_BIGENDIAN([AC_DEFINE([MIPSEB])],[AC_DEFINE([MIPSEL])])
  AC_DEFINE([PROTOCOL],[1])

  AC_REQUIRE([AC_THREAD_FLAGS])
  dnl ------------------------------------------------------------------
  dnl Misc flags per host OS/CPU
  case $host_os:$host_cpu in
  sun*)		  AC_DEFINE([sunos])		;;
  solaris*)	  AC_DEFINE([solaris])	
		  LIBS="$LIBS -lsocket -lnsl"	;;
  hp-ux*)	  AC_DEFINE([hpux])		;;
  osf*)		  AC_DEFINE([osf])		;;
  aix*)		  AC_DEFINE([aix])	
		  AC_DEFINE([unix])
		  AC_DEFINE([_BSD])		;;
  lynxos*:rs6000) AC_DEFINE([LYNXOS])	
		  AC_DEFINE([RAID])
		  AC_DEFINE([unix])		
		  CPPFLAGS="$CPPFLAGS -I/usr/include/bsd -I/usr/include/posix"
		  LDFLAGS="$LDFLAGS -L/usr/posix/usr/lib"
		  LIBS="$LIBS -lbsd"		;;
  lynxos*:*86*)	  AC_DEFINE([LYNXOS])	
		  AC_DEFINE([unix])		
		  LIBS="$LIBS -lbsd -llynx"	;;
  lynxos*)	  AC_DEFINE([LYNXOS])	
		  AC_DEFINE([unix])		
		  LIBS="$LIBS -lbsd"		;;
  linux*)	  AC_DEFINE([linux])		
		  AC_DEFINE([unix])		;;
  esac
])

dnl ------------------------------------------------------------------
AC_DEFUN([AC_DIM],
[
  AC_REQUIRE([AC_DIM_ARCH])
  dnl ------------------------------------------------------------------
  AC_ARG_ENABLE([builtin-dim],
	        [AC_HELP_STRING([--enable-builtin-dim],
	                        [Use shipped DIM, not systems])])
  have_dim=no
  AC_LANG_PUSH(C++)
  AC_CHECK_LIB(dim, [dic_get_server],
	       [AC_CHECK_HEADERS([dim/dim.hxx],[have_dim=yes])])
  AC_LANG_POP(C++)
  if test "x$enable_builtin_dim" = "xyes" ; then 
     have_dim=no
  fi
  AC_MSG_CHECKING(whether to use possible system DIM installed)
  if test "x$have_dim" = "xno" ; then 
     AC_CONFIG_SUBDIRS(dim)
  fi
  AC_MSG_RESULT($have_dim)
  AM_CONDITIONAL(NEED_DIM, test "x$have_dim" = "xno")
])

dnl ------------------------------------------------------------------
AC_DEFUN([AC_DEBUG],
[
  AC_REQUIRE([AC_PROG_CC])
  AC_REQUIRE([AC_PROG_CXX])
  AC_MSG_CHECKING(whether to make debug objects)
  AC_ARG_ENABLE(debug,
    [AC_HELP_STRING([--enable-debug],[Enable debugging symbols in objects])],
    [],[enable_debug=yes])
  if test "x$enable_debug" = "xno" ; then
    CFLAGS=`echo $CFLAGS | sed 's,-g,,'`
    CXXFLAGS=`echo $CXXFLAGS | sed 's,-g,,'`
  else
    AC_DEFINE(__DEBUG)
    case $CXXFLAGS in
    *-g*) ;;
    *)    CXXFLAGS="$CXXFLAGS -g" ;;
    esac
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
  AC_REQUIRE([AC_PROG_CXX])

  AC_ARG_ENABLE(optimization,
    [AC_HELP_STRING([--enable-optimization],[Enable optimization of objects])],
    [],[enable_optimization=yes])

  AC_MSG_CHECKING(for optimiztion level)

  changequote(<<, >>)dnl
  if test "x$enable_optimization" = "xno" ; then
    CFLAGS=`echo   $CFLAGS   | sed 's,-O\([0-9][0-9]*\|\),,'`
    CXXFLAGS=`echo $CXXFLAGS | sed 's,-O\([0-9][0-9]*\|\),,'`
  elif test "x$enable_optimization" = "xyes" ; then
    case $CXXFLAGS in
    *-O*) ;;
    *)    CXXFLAGS="$CXXFLAGS -O2" ;;
    esac
    case $CFLAGS in
    *-O*) ;;
    *)    CFLAGS="$CXXFLAGS -O2" ;;
    esac
  else
    CFLAGS=`echo   $CFLAGS   | sed "s,-O\([0-9][0-9]*\|\),-O$enable_optimization,"`
    CXXFLAGS=`echo $CXXFLAGS | sed "s,-O\([0-9][0-9]*\|\),-O$enable_optimization,"`
  fi
  changequote([, ])dnl
  AC_MSG_RESULT($enable_optimization 'CFLAGS=$CFLAGS')
])

dnl ------------------------------------------------------------------

dnl
dnl Autoconf macro to check for existence or ROOT on the system
dnl Synopsis:
dnl
dnl  ROOT_PATH([MINIMUM-VERSION, [ACTION-IF-FOUND, [ACTION-IF-NOT-FOUND]]])
dnl
dnl Some examples: 
dnl 
dnl    ROOT_PATH(3.03/05, , AC_MSG_ERROR(Your ROOT version is too old))
dnl    ROOT_PATH(, AC_DEFINE([HAVE_ROOT]))
dnl 
dnl The macro defines the following substitution variables
dnl
dnl    ROOTCONF           full path to root-config
dnl    ROOTEXEC           full path to root
dnl    ROOTCINT           full path to rootcint
dnl    ROOTLIBDIR         Where the ROOT libraries are 
dnl    ROOTINCDIR         Where the ROOT headers are 
dnl    ROOTCFLAGS         Extra compiler flags
dnl    ROOTLIBS           ROOT basic libraries 
dnl    ROOTGLIBS          ROOT basic + GUI libraries
dnl    ROOTAUXLIBS        Auxilary libraries and linker flags for ROOT
dnl    ROOTAUXCFLAGS      Auxilary compiler flags 
dnl    ROOTRPATH          Same as ROOTLIBDIR
dnl
dnl The macro will fail if root-config and rootcint isn't found.
dnl
dnl Christian Holm Christensen <cholm@nbi.dk>
dnl
AC_DEFUN([ROOT_PATH],
[
  AC_ARG_WITH(rootsys,
  [  --with-rootsys          top of the ROOT installation directory],
    user_rootsys=$withval,
    user_rootsys="none")
  if test ! x"$user_rootsys" = xnone; then
    rootbin="$user_rootsys/bin"
  elif test ! x"$ROOTSYS" = x ; then 
    rootbin="$ROOTSYS/bin"
  else 
   rootbin=$PATH
  fi
  AC_PATH_PROG(ROOTCONF, root-config , no, $rootbin)
  AC_PATH_PROG(ROOTEXEC, root , no, $rootbin)
  AC_PATH_PROG(ROOTCINT, rootcint , no, $rootbin)
	
  if test ! x"$ROOTCONF" = "xno" && \
     test ! x"$ROOTCINT" = "xno" ; then 

    # define some variables 
    ROOTLIBDIR=`$ROOTCONF --libdir`
    ROOTINCDIR=`$ROOTCONF --incdir`
    ROOTCFLAGS=`$ROOTCONF --noauxcflags --cflags` 
    ROOTLIBS=`$ROOTCONF --noauxlibs --noldflags --libs`
    ROOTGLIBS=`$ROOTCONF --noauxlibs --noldflags --glibs`
    ROOTAUXCFLAGS=`$ROOTCONF --auxcflags`
    ROOTAUXLIBS=`$ROOTCONF --auxlibs`
    ROOTRPATH=$ROOTLIBDIR
	
    if test $1 ; then 
      AC_MSG_CHECKING(wether ROOT version >= [$1])
      vers=`$ROOTCONF --version | tr './' ' ' | awk 'BEGIN { FS = " "; } { printf "%d", ($''1 * 1000 + $''2) * 1000 + $''3;}'`
      requ=`echo $1 | tr './' ' ' | awk 'BEGIN { FS = " "; } { printf "%d", ($''1 * 1000 + $''2) * 1000 + $''3;}'`
      if test $vers -lt $requ ; then 
        AC_MSG_RESULT(no)
	no_root="yes"
      else 
        AC_MSG_RESULT(yes)
      fi
    fi
  else
    # otherwise, we say no_root
    no_root="yes"
  fi

  AC_SUBST(ROOTLIBDIR)
  AC_SUBST(ROOTINCDIR)
  AC_SUBST(ROOTCFLAGS)
  AC_SUBST(ROOTLIBS)
  AC_SUBST(ROOTGLIBS) 
  AC_SUBST(ROOTAUXLIBS)
  AC_SUBST(ROOTAUXCFLAGS)
  AC_SUBST(ROOTRPATH)

  if test "x$no_root" = "x" ; then 
    ifelse([$2], , :, [$2])     
  else 
    ifelse([$3], , :, [$3])     
  fi
])


#
# EOF
#
