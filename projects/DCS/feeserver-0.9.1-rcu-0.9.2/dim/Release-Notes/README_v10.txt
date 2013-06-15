
                    DIM version 10.5 Release Notes

Notes 1 and 2 for Unix Users only
NOTE 1: In order to "make" DIM two environment variables should be set:
	OS = one of {HP-UX, AIX, OSF1, Solaris, SunOS, LynxOS, Linux}
	DIMDIR = the path name of DIM's top level directory
	The user should then go to DIM's top level directory and do:
	> source .setup
	> gmake all
	Or, if there is no support for C++ on the machine:
	> gmake CPP=no all

NOTE 2: The Name Server (Dns), DID, servers and clients (if running in 
	background) should be started whith the output redirected to a 
	logfile ex:
		Dns </dev/null >& dns.log &

NOTE 3: The Version Number service provided by servers is now set to 1005
	(version 10.05).

25/4/2002
Changes for version 10.0:
      - All source files are now common to Windows and Unix flavours 
	(Linux included). Directories src/win and src/unix no longer 
	necessary.
      - Fixed hopefully all compiler warnings (especially on Solaris 8).
      - In order to avoid potential deadlocks all tcpip writes (dna_write)
	are done by a separate thread (the timer thread, via a special 
	"immediate" queue). Except service updates and sending commands
	(dna_write_nowait) since they are not blocking and to preserve
	backward behaviour compatibility.
      - Optimized servers, clients and the name servers for large number
	of services
      - Modified error messages to be more explicit

01/5/2002
Changes/Bug Fixes for Version 10.1:
      - Fixed the DimRpc class, it would hang sometimes.
      - Fixed a problem in the "immediate" timer handler (too slow)
      - Added "const" to service names in DimInfo and DimService methods
      - changed print_date_time to dim_print_date_time and made it 
	available to users
      - Open_dns didn't always return the correct value (DID wouldn't
	reconnect to Dns on restart)
      - Did (on Linux) now shows services in alphabetical order

06/5/2002 
Changes/Bug Fixes for Version 10.2:
      - Fixed dtq.c and tcpip.c for Linux, dim_wait() would not always
	return when required
      - The distribution kit now also contains the shareable version of
	the DIM library for Linux - libdim.so
	The makefiles use the shareable version for creating Dns, Did 
	and the examples (.setup adds dim/linux to LD_LIBRARY_PATH)  

27/5/2002 
Changes/Bug Fixes for Version 10.3:
      - Changed dim include files not to include "windows.h" under
	windows. This was causing a conflict with Gaudi.
	(Had to change "DIM semaphores" from macros to subroutines)
  
18/7/2002 
Changes/Bug Fixes for Version 10.4:
      - Two consecutive client requests for the same service would not 
	implement the "stamped" flag properly (dic.c)
      - The DimBrowser class would not retreive service names containing
	the character "@". Fixed.
      - Re-fixed a bug that would make servers crash when clients exited
	while servers were updating a service (dis.c).
      - dtq_start_timer() did not always wait the requested amount of time.
      - Did (Linux version) now also prints timestamp and quality flag
	when Viewing service contents.
  
19/8/2002
Changes/Bug Fixes for Version 10.5:
      - Fixed DID for Solaris (had stopped working, Motif changed?) 
      - Fixed a nasty bug that must have been there forever:
	Several DIM processes (clients and Dns) execute at regular 
	intervals a "test write" on open connections to find out the status
	of the connection. These test writes where never cancelled when the
	connection was closed. (noticed on Solaris/BaBar).

Please check the Manual for more information at:
    http://www.cern.ch/dim



