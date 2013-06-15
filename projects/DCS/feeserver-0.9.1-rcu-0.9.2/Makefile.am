# $Id: Makefile.am,v 1.8 2007/06/15 15:01:48 richter Exp $
# Makefile template for the FeeServer with RCU support

EXTRA_DIST		= configure-arm feeserver.spec

if NEED_DIM
EXTRA_SUBDIRS		= dim
else
DIM_EXTRA		= dim/Release-Notes	\
			  dim/dim		\
			  dim/src		\
			  dim/Makefile.am	\
			  dim/Makefile.in	\
			  dim/configure		\
			  dim/configure.ac	\
			  dim/aclocal.m4	\
			  dim/acinclude.m4	
endif
EXTRA_DIST		+= $(DIM_EXTRA)
SUBDIRS 		= $(EXTRA_SUBDIRS) feeserver rcu-ce-cmds scripts doc
DIST_SUBDIRS		= $(SUBDIRS)


rpm: dist
	rpmbuild -ta $$flags $(distdir).tar.gz


#dist_noinst_SCRIPTS	= autogen.sh
#
# EOF
#
