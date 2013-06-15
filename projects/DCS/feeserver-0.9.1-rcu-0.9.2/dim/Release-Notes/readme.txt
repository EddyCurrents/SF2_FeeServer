
                    DIM version 9.8 Release Notes

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

NOTE 3: The Version Number service provided by servers is now set to 908
	(version 9.8).
  
Changes for version 9.0:

   In order to increase the compatibility between Windows and Linux and
   to respect CVS rules the following modifications have been done:

      - C++ files have been renamed from .cc to .cxx and include files from
	.hh to .hxx

      - Include files are in "./dim" directory
        (Source files are now in the "./src" directory)

      - Windows executables and libraries are in "./bin"
      - Linux executables and libraries are "./linux"

      - Windows developper studio setting are in "./Visual"
      - Linux Makefiles are in the top directory

Changes for version 9.1:

    Fixed some "Harp" problems:
      - Fixed a bug causing a loop in the timer handling 
      - Optimized dns connection handling for many services
      - Fixed a re-connection bug for clients 
      
Changes for version 9.2:

      - Created the static methods:
		DimServer::autoStartOff();
		DimServer::autoStartOn();
	Which prevents/allows DimServices to be declared to the Name Server as
	soon as they are created. if "autoStart" is set Off the user has to call
	DimServer::start(char *serverName) when all services have been declared.
	By default autoStart is On

Changes for version 9.3:

      - Created a new Utility: DimBridge - It forwards DIM Services (and Commands)
	from one Name Server to another (to bypass firewalls)
      - Fixed a bug in tcpip.c - which prevents a DIM Server to send test messages to
	himself (related to the above utility).

Changes for version 9.4:

      - Merged DID and XDID (DID should now work on all UNIX flavours)
      - Allow users to select the ethernet interface (or to specify a complete 
        ipname, with the domain, when not available by default):
		setenv DIM_HOST_NODE <ipname>
	Before starting up a DIM server 
 
Changes for version 9.5:

      - Added an environment variable DIM_DNS_PORT allowing users to specify a
	different port number (default is 2505) for the DNS. This allows 
	starting more than one DIM Name Servers (DNSs) on the same machine.
      - Accomodated for a Solaris "feature": ioctl FIONREAD which should 
        return the number of bytes waiting to be read on a socket sometimes 
	returns '0' when there are bytes to read. This provoked undesired 
        "disconnections" in BaBar.
      - Fixed a bug related to the padding of structures (characters following
        an odd number of shorts)
      - Fixed a bug in the retry mechanism when writting to a full socket. The
        problem appeared when the connection was killed while retrying.
      - Did (Unix/Linux version) now allows sending formatted commands (i.e. 
        structures) to a server. Also services are now visualised in a 
	formatted manner.

Changes for version 9.6:
      - Fixed DID: it crashed when a server name was very big (Motif) and
        it didn't remove servers that died while being in error (red).
      - Fixed a bug in the client library: sometimes services where requested
	from the name server more than once unnecessarily.
      - Fixed a bug in the server library: Sometimes the server crashed if it 
	was updating a service when the client was killed or died.

Changes for version 9.7
      - Fixed a bug introduced in version 9.6 (related to the last point).
        Sometimes servers would leave some connections open and started using
        all the CPU (involves dis.c and tcpip.c).
      - Upgraded DID to support very long server names.

Changes for version 9.8
      - Fixed a bug in DID: it crashed when a service name to be viewed was
        typed in by hand
      - Fixed a bug in dis.c: Servers would not register their services with 
        the DNS if the number of services was a multiple of 100.
 
Please check the Manual for more information at:
    http://www.cern.ch/dim



