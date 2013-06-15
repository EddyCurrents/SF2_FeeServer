# $Id: Makefile,v 1.3 2006/05/11 12:12:31 richter Exp $
# Makefile template for the FeeServer with RCU support
#
# remarks:
# - if static is used the arm compiler can not sorrectly link the dim
#   and pthread libraries if they appears befor the object files,
#   thats why the -ldim and -lpthread flags have been added to the
#   local flags feeserver_LDFLAGS  

feeserver_SOURCES	+= src/feeserver.c 		
noinst_HEADERS		+= src/fee_defines.h 		\
			   src/fee_errors.h 		\
			   src/fee_functions.h 		\
                           src/fee_loglevels.h 		\
			   src/feepacket_flags.h 	\
			   src/fee_types.h		\
			   src/fee_utest.h

if TEST
TEST_SRC		=  src/fee_utest.c
endif
feeserver_SOURCES	+= $(TEST_SRC)

#
# EOF
#
