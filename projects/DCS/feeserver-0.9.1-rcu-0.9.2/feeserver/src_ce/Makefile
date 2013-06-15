#
#
#
feeserver_SOURCES	+= $(srcdir)/src_ce/ce_command.c		\
			   $(srcdir)/src_ce/issuehandler.cpp 		\
			   $(srcdir)/src_ce/rcu_issue.cpp 		\
			   $(srcdir)/src_ce/statemachine.cpp 		\
			   $(srcdir)/src_ce/device.cpp 			\
			   $(srcdir)/src_ce/dimdevice.cpp 		\
			   $(srcdir)/src_ce/controlengine.cpp 		\
			   $(srcdir)/src_ce/threadmanager.cpp 		\
			   $(srcdir)/src_ce/lockguard.cpp 		\
			   $(srcdir)/src_ce/rcu_service.cpp 		\
			   $(srcdir)/src_ce/dev_msgbuffer.cpp 		\
			   $(srcdir)/src_ce/ce_base.cpp	

noinst_HEADERS		+= $(srcdir)/src_ce/ce_command.h 		\
			   $(srcdir)/src_ce/codebook_rcu.h 		\
			   $(srcdir)/src_ce/issuehandler.hpp		\
			   $(srcdir)/src_ce/rcu_issue.h 		\
			   $(srcdir)/src_ce/statemachine.hpp 		\
			   $(srcdir)/src_ce/device.hpp 			\
			   $(srcdir)/src_ce/dimdevice.hpp 		\
			   $(srcdir)/src_ce/controlengine.hpp 		\
			   $(srcdir)/src_ce/RCU_ControlEngine.hpp 	\
			   $(srcdir)/src_ce/dev_rcu.hpp 		\
			   $(srcdir)/src_ce/dev_actel.hpp 		\
			   $(srcdir)/src_ce/dev_msgbuffer.hpp 		\
			   $(srcdir)/src_ce/branchlayout.hpp 		\
			   $(srcdir)/src_ce/dev_fec.hpp 		\
			   $(srcdir)/src_ce/threadmanager.hpp 		\
			   $(srcdir)/src_ce/lockguard.hpp 		\
			   $(srcdir)/src_ce/rcu_service.h 		\
			   $(srcdir)/src_ce/ce_base.h			\
			   $(srcdir)/src_ce/ce_tpc.h			\
			   $(srcdir)/src_ce/ce_fmd.hpp			\
			   $(srcdir)/src_ce/ce_phos.h
if EN_RCU
# rcu specific source files
RCU_SRC			=  $(srcdir)/src_ce/dev_rcu.cpp 		\
			   $(srcdir)/src_ce/dev_fec.cpp 		\
			   $(srcdir)/src_ce/dev_actel.cpp 		\
			   $(srcdir)/src_ce/RCU_ControlEngine.cpp 	\
			   $(srcdir)/src_ce/branchlayout.cpp

if EN_TPC
# tpc specific source files
TPC_SRC			=  $(srcdir)/src_ce/ce_tpc.cpp
endif

if EN_PHOS
# phos specific source files
PHOS_SRC		=  $(srcdir)/src_ce/ce_phos.cpp
endif

if EN_FMD
# fmd specific source files
FMD_SRC		=  $(srcdir)/src_ce/ce_fmd.cpp
endif

endif
feeserver_SOURCES	+= $(RCU_SRC) $(TPC_SRC) $(PHOS_SRC) $(FMD_SRC)


#
# EOF
#
