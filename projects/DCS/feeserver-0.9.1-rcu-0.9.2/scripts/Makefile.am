#
#
#
#if EN_FMD
FMDEXTRA	= mac2num feeserver
FMDSCRIPTS	= mac2num
FMDDATA	= feeserver
FMDINITSCRIPTS	= S90feeserver
#endif

EXTRA_DIST	= $(FMDEXTRA) 
bin_SCRIPTS	= startFeeServer.sh $(FMDSCRIPTS)
defdir		= $(sysconfdir)/sysconfig
def_DATA	= $(FMDDATA)
initdir		= $(sysconfdir)/init.d
init_SCRIPTS	= $(FMDINITSCRIPTS)

#
#
#
