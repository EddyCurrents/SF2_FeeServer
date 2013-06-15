
                    DIM version 12.11 Release Notes

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

NOTE 3: The Version Number service provided by servers is now set to 1211
	(version 12.11).

13/2/2003
Changes for version 12.0:
      - Included Java support in the same distribution kit and updated
	the documentation on the WEB.
	In order to make a dim shareable library to be used from java
	on linux:
		setenv JDK_INCLUDE <your jdk include directory>
		gmake JDIM=yes all
	The libraries are distributed for windows and linux 7.3
	The java part is in jdim to run some examples:
		java -classpath .../jdim/classes dim.test.TestServer
		java -classpath .../jdim/classes dim.test.TestClient
20/3/2003
Changes for version 12.1:
      - Removed all references to iostream.h in DIM include files and
	source files in order to support Linux RedHat 8.0 and gcc 3.2

19/6/2003
Changes for version 12.2:
      - Fixed a bug in the DimTimer class
      - Added a missing "destructor" for DimCommand in Java.

23/6/2003
Changes for version 12.2-1:
      - Changed the directory structure under jdim to respect the Java 
	conventions

02/7/2003
Changes for version 12.3:
      - Fixed a bug that made clients crash when a server released a 
	command service.
	Added the possibility of sending mixed services in the Java
	implementation.

04/7/2003
Changes for version 12.3-1:
      - The Java version was not correct in the previous ZIP 
	(the loading of jdim.dll was not done properly)

24/7/2003
Changes for version 12.4:
      - Fixed a bug in Did related to the size of displayed services
      - Fixed a memory leak when servers received commands

01/8/2003
Changes for version 12.5:
      - Clients would sometimes crash when receiving data for a service 
	which they had just released, fixed.
      - Clients could also potencially crash when failing to write to a 
	server that had just disconnected, fixed.

20/8/2003
Changes for version 12.6:
      - Clients would crash when releasing the same service twice, 
	fixed.
      - Exported through JNI the possibility of protecting DIM critical 
	sections, so that dim_lock and dim_unlock can be used from Java.

26/8/2003
Changes for version 12.7:
      - A client would sometimes crash when releasing the service inside
	the service's callback. Fixed.
      - A client would also sometimes crash if it releasead a service while
	still connecting to this service. I.e. if a dic_info_service was 
	immediately followed by a dis_release_service. Fixed.
      - The server could also crash or hang is still connecting to a client 
	when the service was removed. Fixed
      - Found and fixed a small memory lead (allocating ids)
	
01/9/2003
Changes for version 12.8:
      - Implemented "short" and "longlong" (64 bit integer) in the C++ version
	of both, servers and clients.
      - Implemented boolean, byte, short and long in the Java version. 
	
03/9/2003
Changes for version 12.9:
      - Protected dis_set_quality and dis_set_timestamp against bad service_ids.
      - Alowed dis_stop_serving() to be called before any dis_start_serving(),
	before it would crash. 

05/9/2003
Changes for version 12.10:
      - dis_stop_serving would sometimes crash when called from dis_remove_service,
	fixed. 

25/9/2003
Changes for version 12.11:
      - dic_command_callback() would sometimes not send the command if the 
	callback reception immediatelly terminated the program. I.e. 
	dim_send_command might not actually send the command. Fixed.

Please check the Manual for more information at:
    http://www.cern.ch/dim



