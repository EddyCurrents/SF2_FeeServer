dnl -*- mode: autoconf -*- 
dnl
dnl $Id: configure.ac,v 1.36.2.2 2007/09/11 23:25:56 richter Exp $
dnl template for the configuration script for the FeeServer
dnl with RCU-like ControlEngines
dnl 
dnl ------------------------------------------------------------------
AC_INIT([FeeServer for RCU] , [0.9.1-rcu-0.9.2], [Matthias.Richter@ift.uib.no], feeserver)
AC_CANONICAL_SYSTEM
AC_PREFIX_DEFAULT(${PWD})
AC_CONFIG_SRCDIR(feeserver/src/feeserver.c)
AM_INIT_AUTOMAKE
AC_PROG_CC
AC_PROG_CXX
AC_PROG_LIBTOOL
AC_PROG_RANLIB

AC_DEBUG
AC_OPTIMIZATION

dnl the ARM_COMPILING switch is used in the feeserver/Makefile.am
dnl to set the linkink option to libtool (-all-static)
dnl so far I couldn't find out how to do this on the configure 
dnl command line. Although this flag should be set according to
dnl users choice of --anable/disable-shared
if test "x$cross_compiling" = "xyes" ; then
 case "${host}" in
 arm*)arm_compiling=yes ;;
 *)   arm_compiling=no ;; 
 esac
fi
AC_MSG_CHECKING(whether we're compiling for arm-linux)
AM_CONDITIONAL(ARM_COMPILING, test x$arm_compiling = xyes )
AC_MSG_RESULT($arm_compiling)

dnl ------------------------------------------------------------------
AC_MSG_CHECKING([whether to enable DCSC simulation])
AH_TEMPLATE([DCSC_SIMULATION],[Simulated DCS])
AC_ARG_ENABLE(dcscsim,
  [AC_HELP_STRING([--enable-dcscsim],
      [Turn on register simulation feature for Msg Buffer Interface ])],
  [],[enable_dcscsim=no])
if test "x$enable_dcscsim" = "xyes" ; then 
  AC_DEFINE(DCSC_SIMULATION)
fi
# AM_CONDITIONAL(DCSCSIM, test x$dcscsim = xtrue)
AC_MSG_RESULT([$enable_dcscsim])

dnl ------------------------------------------------------------------
dnl enable benchmark code
AC_MSG_CHECKING([whether to enable benchmarks])
AH_TEMPLATE([__BENCHMARK],[benchmarking code])
AC_ARG_ENABLE(benchmark,
  [AC_HELP_STRING([--enable-benchmark],[Enable benchmarking code])],
  [],[enable_benchmark=no])
if test "x$enable_benchmark" = "xyes" ; then
  AC_DEFINE(__BENCHMARK)
fi
AC_MSG_RESULT([$enable_benchmark])

dnl ------------------------------------------------------------------
dnl be careful with the TEST defines, there are two different defines
dnl one for the core and one for the Msg Buffer Interface
AC_MSG_CHECKING([whether to enable FeeServer core test mode])
AH_TEMPLATE([TEST],[FeeServer core test mode])
AC_ARG_ENABLE(test,
  [AC_HELP_STRING([--enable-test],[Enable test version (feeserver core)])],
  [],[enable_test=no])
if test "x$enable_test" = "xyes" ; then
  AC_DEFINE(TEST)
fi
AM_CONDITIONAL(TEST, test x$enable_test = xyes)
AC_MSG_RESULT([$enable_test])

dnl ------------------------------------------------------------------
dnl AC_MSG_CHECKING([whether to enable DCSC test mode])
dnl this should be merged somehow with the rcudummy option
dnl for now, DCSC_TEST is set in the feeserver/Makefile.am
dnl AH_TEMPLATE([DCSC_TEST],[Test mode])
dnl AC_ARG_ENABLE(hw,
dnl   [AC_HELP_STRING([--enable-hw],[disable hardware access and use a dummy device])],
dnl   [],[test "x$cross_compiling" = "xyes" && enable_hw=yes || enable_hw=no])
dnl if test "x$enable_hw" = "xno" ; then
dnl   AC_DEFINE(DCSC_TEST)
dnl fi
dnl AC_MSG_RESULT([$enable_hw])

dnl ------------------------------------------------------------------
AC_MSG_CHECKING([whether to enable DIM services])
AH_TEMPLATE([DISABLE_SERVICES, disable DIM services])
AC_ARG_ENABLE(services,
  [AC_HELP_STRING([--disable-services],[Disable dim services])],
  [],[enable_services=yes])
if test "x$enable_services" = "xyes" ; then 
  :
else
  AC_DEFINE(DISABLE_SERVICES)
fi  
# AM_CONDITIONAL(EN_SERVICES, test x$services = xtrue)
AC_MSG_RESULT([$enable_services])

dnl ------------------------------------------------------------------
suppress_rcu=no
dnl ------------------------------------------------------------------
AC_MSG_CHECKING([whether to enable TRD functionality])
AH_TEMPLATE([TRD], [Include TRD specific code])
AC_ARG_ENABLE(trd,
  [AC_HELP_STRING([--enable-trd],[Include TRD functionality])],
  [],[enable_trd=no])
if test "x$enable_trd" = "xyes" ; then 
  AC_DEFINE(TRD)
  suppress_rcu=yes
fi
AM_CONDITIONAL(EN_TRD, test x$enable_trd = xyes)
AC_MSG_RESULT([$enable_trd])

dnl ------------------------------------------------------------------
AC_MSG_CHECKING([whether to enable BusyBox functionality])
AH_TEMPLATE([BUSYBOX], [Include BusyBox specific code])
AC_ARG_ENABLE(busybox,
  [AC_HELP_STRING([--enable-busybox],[Include BusyBox functionality])],
  [],[enable_busybox=no])
if test "x$enable_busybox" = "xyes" ; then 
  AC_DEFINE(BUSYBOX)
  suppress_rcu=yes
fi
AM_CONDITIONAL(EN_BUSYBOX, test x$enable_busybox = xyes)
AC_MSG_RESULT([$enable_busybox])

dnl ------------------------------------------------------------------
AC_MSG_CHECKING([whether to enable RCU functionality])
AH_TEMPLATE([RCU], [Enable RCU functionality])
AC_ARG_ENABLE(rcu,
  [AC_HELP_STRING([--disable-rcu],[Exclude RCU funtionality])],
  [],[if test x$suppress_rcu = xyes ; then enable_rcu=no ; else enable_rcu=yes ; fi])
if test "x$enable_rcu" = "xyes" ; then 
  AC_DEFINE(RCU)
fi
AM_CONDITIONAL(EN_RCU, test x$enable_rcu = xyes)
AC_MSG_RESULT([$enable_rcu])

dnl ------------------------------------------------------------------
AC_MSG_CHECKING([whether to enable RCU dummy mode])
AH_TEMPLATE([RCUDUMMY], [Enable RCU dummy mode])
AC_ARG_ENABLE(rcudummy,
  [AC_HELP_STRING([--enable-rcudummy],
                  [Use RCU dummy instead of real hardware])],
  [],[enable_rcudummy=no])
if test "x$enable_rcudummy" = "xyes" ; then 
  AC_DEFINE(RCUDUMMY)
fi
AC_MSG_RESULT([$enable_rcudummy])

dnl ------------------------------------------------------------------
AC_MSG_CHECKING([whether to enable FEC simulation])
AH_TEMPLATE([FECSIM], [Enable FEC simulation])
AC_ARG_ENABLE(fecsim,
  [AC_HELP_STRING([--enable-fecsim],
                  [Simulate to response of the FECs])],
  [],[enable_fecsim=no])
if test "x$enable_fecsim" = "xyes" ; then 
  AC_DEFINE(FECSIM)
fi
AC_MSG_RESULT([$enable_fecsim])

dnl ------------------------------------------------------------------
AC_MSG_CHECKING([whether to enable TPC functionality])
AH_TEMPLATE([TPC], [Include TPC specific code])
AC_ARG_ENABLE(tpc,
  [AC_HELP_STRING([--enable-tpc],[Include TPC functionality])],
  [],[enable_tpc=no])
if test "x$enable_tpc" = "xyes" ; then
  if test x$enable_rcu != xyes ; then AC_ERROR([the TPC module needs the RCU]) ; fi
  AC_DEFINE(TPC)
fi
AM_CONDITIONAL(EN_TPC, test x$enable_tpc = xyes)
AC_MSG_RESULT([$enable_tpc])

dnl ------------------------------------------------------------------
AC_MSG_CHECKING([whether to enable PHOS functionality])
AH_TEMPLATE([PHOS], [Include PHOS specific code])
AC_ARG_ENABLE(phos,
  [AC_HELP_STRING([--enable-phos],[Include PHOS functionality])],
  [],[enable_phos=no])
if test "x$enable_phos" = "xyes" ; then 
  if test x$enable_rcu != xyes ; then AC_ERROR([the PHOS module needs the RCU]) ; fi
  AC_DEFINE(PHOS)
fi
AM_CONDITIONAL(EN_PHOS, test x$enable_phos = xyes)
AC_MSG_RESULT([$enable_phos])

dnl ------------------------------------------------------------------
AC_MSG_CHECKING([whether to enable FMD functionality])
AH_TEMPLATE([FMD], [Include FMD specific code])
AC_ARG_ENABLE(fmd,
  [AC_HELP_STRING([--enable-fmd],[Include FMD functionality])],
  [],[enable_fmd=no])
if test "x$enable_fmd" = "xyes" ; then 
  if test x$enable_rcu != xyes ; then AC_ERROR([the FMD module needs the RCU]) ; fi
  AC_DEFINE(FMD)
fi
AM_CONDITIONAL(EN_FMD, test x$enable_fmd = xyes)
AC_MSG_RESULT([$enable_fmd])

dnl ------------------------------------------------------------------
AC_MSG_CHECKING([whether to enable fixes for RCU firmware of May 05])
AH_TEMPLATE([ENABLE_ANCIENT_05], [Fixes for May 05 RCU firmware])
AC_ARG_ENABLE(ancient05,
  [AC_HELP_STRING([--enable-ancient05],[Fixes for May 05 RCU firmware])],
  [],[enable_ancient05=no])
if test "x$enable_ancient05" = "xyes" ; then 
  AC_DEFINE(ENABLE_ANCIENT_05)
fi
AC_MSG_RESULT([$enable_ancient05])

dnl ------------------------------------------------------------------
AC_MSG_CHECKING([whether to enable probing for RCU firmware features and 
                 Front-End Cards])
AH_TEMPLATE([ENABLE_AUTO_DETECTION], [Auto-detection of FW features and FECs])
AC_ARG_ENABLE(auto-detection,
  [AC_HELP_STRING([--enable-auto-detection],[Auto-detection of FW features and FECs])],
  [],[enable_auto_detection=no])
if test "x$enable_auto_detection" = "xyes" ; then 
  AC_DEFINE(ENABLE_AUTO_DETECTION)
fi
AC_MSG_RESULT([$enable_auto_detection])

dnl ------------------------------------------------------------------
AC_MSG_CHECKING([whether to enable master mode])
AH_TEMPLATE([ENABLE_MASTERMODE], [Enable master mode])
AC_ARG_ENABLE(mastermode,
  [AC_HELP_STRING([--enable-mastermode],
                  [Enable features which can cause security lacks])],
  [],[enable_mastermode=no])
AC_MSG_RESULT([$enable_mastermode])
if test "x$enable_mastermode" = "xyes" ; then 
  AC_DEFINE(ENABLE_MASTERMODE)
  AC_MSG_WARN(
[
You are about to switch on features which can cause security lacks. The 
FeeServer will exept commands for download and/or execution of programs/
scripts. Since DIM is an unsecure network, abuse is possible although
very unlikely. This is NOT an issue in the final DCS of ALICE as well 
as other CLOSED networks.])
fi
# AM_CONDITIONAL(EN_MASTERMODE, test x$mastermode = xtrue)

dnl ------------------------------------------------------------------
dnl The dim library for the FeeServer must be compiled with the flag
dnl NOTHREADS in order to avoid a dead-lock. So we can not rely on a 
dnl dim library in the system and have to always use the builin library
#AC_DIM
have_dim=no
AC_CONFIG_SUBDIRS(dim)
AM_CONDITIONAL(NEED_DIM, test "x$have_dim" = "xno")


dnl ------------------------------------------------------------------
dnl thread flags for the feeserver
AC_THREAD_FLAGS

dnl ------------------------------------------------------------------
dnl
dnl Documentation
dnl
AC_ARG_VAR(DOXYGEN, The Documentation Generator)
AC_PATH_PROG(PERL, perl)
AC_PATH_PROG(DOXYGEN, doxygen)
AM_CONDITIONAL(HAVE_DOXYGEN, test ! "x$DOXYGEN" = "x")
HAVE_DOT=NO
DOT_PATH=
AC_PATH_PROG(DOT, dot)
if ! test "x$DOT" = "x" ; then
   HAVE_DOT=YES
   DOT_PATH=`dirname $DOT`
fi
AC_SUBST([HAVE_DOT])
AC_SUBST([DOT_PATH])

dnl ------------------------------------------------------------------
dnl
dnl RPM stuff 
dnl
RPM_VERSION=`echo $VERSION | tr '-' '_'`

RPM_BUILD_ARGS=
for arg in `echo $ac_configure_args | tr \' ' ' `; do 
 case $arg in
   --prefix=*)  ;;
   --host=* | \
   CC=* | \
   CXX=* | \
   LD=* | \
   --*) RPM_BUILD_ARGS="$RPM_BUILD_ARGS $arg" ;;
 esac
done
if test $prefix = NONE ; then
   RPM_PREFIX=`pwd`
else
   RPM_PREFIX=$prefix
fi
AC_SUBST(RPM_VERSION)
AC_SUBST(RPM_PREFIX)
AC_SUBST(RPM_BUILD_ARGS)

dnl ------------------------------------------------------------------
AC_CONFIG_FILES([Makefile 
		 feeserver/Makefile
		 feeserver/dcscMsgBufferInterface/Makefile
		 feeserver.spec
		 scripts/Makefile
		 scripts/startFeeServer.sh
		 scripts/S90feeserver
		 doc/Makefile
		 doc/doxygen.conf
		 rcu-ce-cmds/Makefile])

AC_OUTPUT
dnl
dnl EOF
dnl

