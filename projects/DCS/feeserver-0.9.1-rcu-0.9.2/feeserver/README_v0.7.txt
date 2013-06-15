---------------------------------------------------------------------------------
                FeeServer version 0.7.0 release notes
---------------------------------------------------------------------------------

Compilation:
------------
To compile the FeeServer please follow these steps:
  - change to the dim folder (feeserver/dim)
  - run ". setup.sh" or "source setup.sh" to prepare your shell (bash) 
  - NEW FEATURE: the FeeServer now allows to print out benchmark timestamps.
    To enable this feature, you have to comment in the "benchmark = yes" line
	in the makefile_feeserver.arm (makefile_feeserver.linux). If you specify
	the environmental variable FEE_BENCHMARK_FILENAME, the timestamps are 
	written to this file, else the benchmark output is sent as log message
	in the log level MSG_SUCCESS_AUDIT. 
	To use the normal FeeServer make sure this line is outcommented.
  - if you compile FeeServer for the first time on your system:
    - run "make -f makefile_dim.arm" to compile the required DIM Library
  - run "make -f makefile_feeserver.arm" to finally compile FeeServer
  - NOTE: It is important, that the DIM lib is compiled with the flag
    NOTHREADS !!! Else the communication can get stuck. The makefiles set this
    as standart. 
  - to install the feeserver in the run folder, type 
    "make install -f makefile_feeserver.arm". This copies the binary to the run
    folder, where the start script is located.
  
  if you want to clean up your system:
    - run "make realclean -f makefile_dim.arm" to clean up DIM stuff
    - run "make clean -f makefile_feeserver.arm" to clean up
    
  - to compile everything for standard-linux use "*.linux" makefiles 
    instead of "*.arm"

Preparation:
------------
You now compiled the version for ArmLinux, so put the binary wherever you use
to execute applications on your board.
You must prepare your envirnonment with some variables to run FeeServer.
You can either adapt the "setDim.sh" in the dim/run folder (new position,
formerly scripts folder!) or you can create these variables by hand:
  - DIM_DNS_NODE for the dns for DIM
  - FEE_SERVER_NAME for the name of the feeserver (as seen in DIM)

Since version 0.7.7 a new environmental variable has been added:
  - FEE_LOGWATCHDOG_TIMEOUT variable to set the timeout for the watchdog, 
    checking for replicated log messages (details see below: Changes 0.7.7).
    If not set, a default value is used.
	
  !!! IMPORTANT !!! NOTE:
  -----------------------
  Since version 0.7.0:
  The environmental variable for the FeeServer name has changed to
  FEE_SERVER_NAME.


Execution:
----------
To execute FeeServer properly please follow these steps:
  - make sure that the dns for DIM is running on the machine you specified
  - !!! NEW !!! since version 0.7.0:
    finally run "sh startFeeServer.sh"
    This script starts the FeeServer and takes care of various features.
  

Important Notes:
----------------
  - make sure that the setup.sh is executed only ONCE per shell!
  - start the FeeServer only with the startFeeServer.sh script.
  - interface of FeeServer and CE changed in version 0.7.0  
  - The service name scheme for CE service has changed:
	old style: "servername/servicename" -> new style: "servername_servicename"  
	These changes for ACK-, MSG- and Command-channel will follow.


CE-issues:
---------
Following some notes, which should be take account of. 
  - We included a demo function for initializing an "Item" in our dummy CE.
    When writing an own CE, it is best copying and using this function in
    order to get a proper initialized "Item".
  - Don't use static created "Items", especially don't use static allocated
    memory for the "Item" names. This would case the clean up to hang and a 
    proper exit would be no longer possible.
  - Please obey the DCS naming and numbering convention, when naming an "Item".
    more under: http://alicedcs.web.cern.ch/AliceDCS/
  - Provide the "Items" with a default deadband. (Else the deadband will be 0,
    and the update rate would get to high -> to much traffic). 
	To clarify that, the deadband must not be zero and should be set by the CE.
  - After the CE has been initialized, let this thread sleep for approx. 
    2-5 seconds (this gives the FeeServer time start the serving functionality 
    inside the DIM framework).
  - In "cleanUpCE()", the CE don't need to clean and/or free "Items" and its
    members. (In fact it must not do it, this would crash the FeeServer during
    its cleanup.)
  - Don't call the createLogMessage() function during initializing the CE.
    (after the above mentioned sleep is fine). This service is only available
    after the DIM serving functionality has been started. 
  - Be careful with sending log messages, especially in rapidly repeated
    checks/functions, this can pile up your log file extremely. Even consider 
    carefully the specified log level; MSG_ALARM should be only for events, 
    that could damage or destroy the system/hardware.
  - If, while testing, certain log messages of the CE don't appear, check the
    set log levels in FeeServer and InterComLayer. They may have filtered
    these messages. To set the default log level of the FeeServer, you can 
    also use the environmental variable FEE_LOG_LEVEL before starting the 
    FeeServer.
  - The CE can now also call a function, which allows for writting out 
	benchmark timestamps. If the environmental variable FEE_BENCHMARK_FILENAME
	is set, the output goes to this file, else the benchmark timestamp is sent
	via the log message channel.
	The signature of the function looks like that:
	void createBenchmark(char* msg); 
	where msg is a self-choosen text, which should determine the location of 
	the benchmark timestamp. It is prefixed to the timestamp automatically.


--------------------------------------------------------------------------------

Changes for version 0.7.0:
--------------------------
2004-11-30 and before:
  - environmental variable for the FeeServer name changed to FEE_SERVER_NAME
  - implemented a starting script for the FeeServer (startFeeServer.sh)
    [includes the features of restart- and update- FeeServer commands]
  - time on board now set in starting script
  - log messages now also possible from CE
  - improved failure tolerance: backup solution for watchdogs, handling failures
    on unexpected behaviour in several functions, improved thread handling,
    secured logging mechanism, additional minor changes to improve stability
  - increased amount and details of log messages in all areas
  - introduced helper function to create an empty Item 
    see mockup CE for details! please use before assigning values and publishing
  - changes on function "signalCEready()":
    CE can now inform FeeServer about errors that occured during initialization
    via parameter which provides "OK" or an error code: 
    signalCEready(int ceState)
    see mockup CE for usage and fee_errors.h for error codes



Changes for version 0.7.1:
--------------------------
  - new DIM framework included
  - inserted checks against buffer overflow for timeouts and update rate
  - timeout for init CE has new structure: now two defines, one for seconds,
	and one for milliseconds.
  - changed scheme of servicenames (CE), 
	new structure: "servername_servicename" !!!
  - now the FeeServer updates each value also after certain time intervals
    (the time interval, by which all services are updated, is: (updateRate [of 
    the deadband checker]) * (amount of published services) * 
    (TIME_INTERVAL_MULTIPLIER [to modify the time interval by default, change
    this value in fee_defines.h before compiling]).
  - the start up log level can be set from outside before calling the start 
    script by setting the environmental variable FEE_LOG_LEVEL (obey the 
    possible log levels; NOTE: MSG_ALARM is always set). If FEE_LOG_LEVEL is
    not provided, the default log level is used.
  - the starting script now restarts the FeeServer in case of an exit due to
    an unknown error. So, exit the FeeServer via the exit command of the 
    InterComLayer or the newly introduced killFeeServer.sh script, which first
    kill the script before killing the FeeServer itself.
  - Added the "CE issues" in this ReadMe, should help to avoid errors while
    writing and testing own CE's. (more see above)


Changes for version 0.7.2:
--------------------------
  - FeeServer now catches the (error) messages created by the DIM framework
    and maps them to its own message system, sending if possible. They are only
    printed to command line, if the FeeServer is compiled as __DEBUG.


Changes for version 0.7.3:
--------------------------
  - New FeeServer - CE interface function added: The FeeServer notifies the CE
    about changes in certain properties. For now, only a change in the update
    rate of a monitored item is propagated to the CE.
    Definition:

    Function to signal the CE, that a property of the FeeServer has changed,
    that might be interessting to the ControlEngine. For now, it is only 
    forseen, that the update rate of deadband checks is propagated to the CE.
    This function provides a FeeProperty, which consists of a flag, telling 
    what kind of property is signaled, and the value itself.
    @param prop pointer to the FeeProperty, providing flag and property value
 
          void signalFeePropertyChanged(FeeProperty* prop);

    This function is called by the FeeServer (core) and should be implemented
    by the ControlEngine (CE). 

   
    The FeeProperty looks like this:

    Typedef FeeProperty.
    This struct is used to inform the CE about a property change in the
    FeeServer. It provides a flag, identifying, which kind of property and the
    property value itself. For now, only the update rate of the deadband check
    is forseen to be signaled.
 
    typedef struct {
	    /** struct value flag -> the flag identifying the property.*/
	    unsigned short flag;
	    /**
	     * struct value uShortVal -> stores the value of an unsigned short
	     * property.
	     */
	    unsigned short uShortVal;
    } FeeProperty;

  - Message, Acknowledge and Command channel names are now using the "_" 
    instead of the "/" inside their names.
  - The FeeServer now checks during the publish call, if the item name already
    exists. If yes, the item is discarded and FEE_ITEM_NAME_EXISTS returned by
    the publish function. The CE should check for all different return values
    of the publish function, some errors are not critical. 
    (see publish() documentation in ce_command.h for more details)


Changes for version 0.7.4:
--------------------------
  - The FeeServer now only updates the values, if the values exceeds the 
    deadband and (in case of deadband = 0) is definitely different from the
    last transmitted one. This does not affect the normal update after a 
    certain time amount.
  - Increased value for normal update of service value (nit exceeding the 
    deadband) to 10000 (former value was 10).
  - Setting of deadbands now allows for broadcasts: providing a service name
    for the update of a deadband as "*_Temp" means, that all services with
    "_Temp" in the name will get this new deadband. The wildcard "*" has to
    be the first character of the service name.


Changes for version 0.7.5:
--------------------------
  - New interface function between FeeServer and ControlEngine introduced:
		int updateFeeService(char* serviceName);
    This function allows the CE to trigger an update of a given service, it is
    implemented by the FeeServer and can be called by the ControlEngine.
    For more details see the interface header file "ce_command.h". 

Changes for version 0.7.6:
--------------------------
  - Benchmarking feature implemented:
	The FeeServer writes out a benchmark timestamp, when it receives a command
	and when an ACK is sent. The environmental variable FEE_BENCHMARK_FILENAME
	provides the filename for the output (if empty, the benchmark is sent as
	log message, event level: MSG_SUCCESS_AUDIT [you have to switch on this 
	log level as well]). 
  - Function to allow benchmarking for the CE included in interface to CE:
	void createBenchmark(char* msg);
  - To compile for benchmarking comment in the benchmark flag in the makefile
	makefile_feeserver.arm (makefile_feeserver.linux).

Changes for version 0.7.7:
--------------------------
  - Watchdog for replicated log messages implemented. When this watch dog is
    running, replicated messages are hold back until the watch dog detects a 
    timeout or a different message is issued. In this case the number of hold
    back log messages and the description of the replicated message are sent. 
    To set the timeout use the environmental variable FEE_LOGWATCHDOG_TIMEOUT,
    its value is set as timeout in milliseconds. If not set, a default value
    of 10000 ms is used. This feature should mainly reduce replicated log 
    messages. (This watch dog is running in its own thread.) 
  - Bugfix: The FeeServer produced a segmentation fault in the clean up, when
    no item has been published (affected stop and restart of FeeServer). Fixed!     


--------------------------------------------------------------------------------

For questions and remarks please contact:
-----------------------------------------
Christian Kofler <kofler@ztt.fh-worms.de>
Sebastian Bablok <bablok@ift.uib.no>
Bejamin Schockert <schockert@ztt.fh-worms.de>
http://www.ztt.fh-worms.de

--------------------------------------------------------------------------------

