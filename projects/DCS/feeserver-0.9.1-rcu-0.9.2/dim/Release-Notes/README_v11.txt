
                    DIM version 11.7 Release Notes

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

NOTE 3: The Version Number service provided by servers is now set to 1107
	(version 11.07).

23/8/2002
Changes for version 11.0:
      - Made Did (Solaris and Linux) run single threaded (to fix some
	strange behaviour of Motif) 
      - Created an "util" directory under "src". To keep DIM utilities.
	Modified all makefiles and Visual Studio accordingly

08/9/2002
Changes for version 11.1:
      - Fixed dnsExists on Solaris. Now dtq.c uses select instead of 
	usleep (usleep is not thread safe). 

12/9/2002
Changes for version 11.2:
      - dim_send_command and dim_get_service utilities where hanging on 
	windows - fixed.  

22/10/2002
Changes for version 11.3:
      - Replaced usleep by select in Did (linux), it was creating a 
	deadlock with signals. (Did is not multithreaded because of 
	Motif)
      - Fixed two bugs in Dns - one is a design flaw, it was creating 
	too many timer entries unnecessarily and using a lot of CPU 
	(just to update did).
	The second happened when a server tried to declare an existing 
	service. The Dns correctly tried to kill the server, but if the 
	server didn't die than the Dns would keep the connection busy 
	instead of disconnecting it.
      - The feature whereby a server disconnects after a timeout from a 
	hanging (not consuming the data from the socket) client, in order 
	to avoid the server hanging himself, had been commented out by
	mistake. It's now back. 

23/10/2002
Changes for version 11.4:
      - Related to the disconnection feature above. Replaced usleep by
	select (with a timeout) when writing to a client. The default
	timeout for a server to disconnect from a hanging client is now 
	5 seconds. But it can be changed (or checked) by using:
		- void dim_set_write_timeout(int secs)
		- int dim_get_write_timeout()
		or
		- DimServer::setWriteTimeout(int secs);
		- int DimServer::getWriteTimeout();
 
31/10/2002
Changes for version 11.5:
      - Allowed tcpip writes to proceed in parallel with reads (in
	different threads) in order to avoid deadlocks (this was 
	previously protected by a semaphore).
      - Removed old commented out code from all source files
      (Note: These changes are not included in CVS tag v11r6)

06/11/2002
Changes for version 11.6:
      - The Dns "forgot" to stop the test_write timer when a server
	went into error. Fixed.

25/11/2002
Changes for version 11.7:
      - Fixed some bugs in the RPC client implementation
      - Created (and exported) a dim_usleep() routine based on select
      - Implemented a dis_get_timeout(int service_id, int client_id) 
	and a DimService::getTimeout(int client_id) which allows
	servers to find out the timeout requested by a client
      - When a server stopped serving a command using dis_remove_service
	the client was not informed and continued using the "cached"
	address instead of re-asking the name server. Fixed. 

Please check the Manual for more information at:
    http://www.cern.ch/dim



