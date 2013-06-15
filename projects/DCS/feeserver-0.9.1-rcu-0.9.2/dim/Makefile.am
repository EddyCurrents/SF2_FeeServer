#
#
#
AUTOMAKE_OPTIONS        =  foreign
ACLOCAL_AMFLAGS         =  -I .
AM_CPPFLAGS		= -I$(srcdir)/dim
EXTRA_DIST		=  Release-Notes/license.gpl	\
			   Release-Notes/readme.txt	\
			   Release-Notes/README_v9.txt	\
			   Release-Notes/README_v10.txt	\
			   Release-Notes/README_v11.txt	\
			   Release-Notes/README_v12.txt	      

lib_LTLIBRARIES		=  libdim.la

libdim_la_SOURCES	=  src/dic.c                	\
                           src/dis.c                	\
			   src/dna.c			\
			   src/sll.c			\
			   src/dll.c			\
			   src/hash.c			\
			   src/swap.c			\
			   src/copy_swap.c		\
			   src/open_dns.c		\
			   src/conn_handler.c		\
			   src/tcpip.c			\
			   src/dtq.c			\
			   src/dim_thr.c		\
			   src/utilities.c
if CXXDIM
CXXDIM_ADDON_SRC	=  src/diccpp.cxx 		\
			   src/discpp.cxx 		\
			   src/dimcpp.cxx 		\
			   src/tokenstring.cxx
endif
libdim_la_SOURCES	+= $(CXXDIM_ADDON_SRC)

pkginclude_HEADERS	=  dim/dic.h			\
			   dim/dim.h			\
			   dim/dim_common.h		\
			   dim/dis.h			
if CXXDIM
CXXDIM_ADDON_HDR	=  dim/dic.hxx 			\
			   dim/dim.hxx 			\
			   dim/dim_core.hxx 		\
			   dim/dis.hxx 			\
			   dim/dllist.hxx 		\
			   dim/sllist.hxx 		\
			   dim/tokenstring.hxx
endif
pkginclude_HEADERS	+= $(CXXDIM_ADDON_HDR)


#
# EOF
#
