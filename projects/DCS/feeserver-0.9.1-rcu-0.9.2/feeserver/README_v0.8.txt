---------------------------------------------------------------------------------
                FeeServer version 0.8 release notes
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
formerly scripts folder!) or you can set these variables by hand (export XY):
  - DIM_DNS_NODE for the dns for DIM
  - FEE_SERVER_NAME for the name of the feeserver (as seen in DIM)
  - FEE_LOG_LEVEL if you want to set a different log level on start up. The
    loglevel is an integer value and can be any combination of the following:
    (MSG_INFO=1; MSG_WARNING=2; MSG_ERROR=4; MSG_FAILURE_AUDIT=8;
    MSG_SUCCESS_AUDIT=16; MSG_DEBUG=32; MSG_ALARM=64) - MSG_ALARM is always on
    

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
    NOTE: To have full functionality of the FeeServer you have to use this 
    script!!!


Important Notes:
----------------
  - make sure that the setup.sh is executed only ONCE per shell!
  - start the FeeServer only with the startFeeServer.sh script.
  - interface of FeeServer and CE changed in version 0.7.0
  - The service name scheme for CE service has changed:
    old style: "servername/servicename" -> new style: "servername_servicename"
    These changes for ACK-, MSG- and Command-channel will follow.
  - you can extract the documentation of the FeeServer with doxygen, which 
    also gives an detailed description of the interface between FeeServer and 
    CE.

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

Changes for version 0.8.0:
--------------------------
(Note: this includes also changes for version 0.7.7, since this was not an
official release)
  - Watchdog for replicated log messages implemented. When this watch dog is
    running, replicated messages are hold back until the watch dog detects a
    timeout or a different message is issued. In this case the number of hold
    back log messages and the description of the replicated message are sent.
    To set the timeout use the environmental variable FEE_LOGWATCHDOG_TIMEOUT,
    its value is set as timeout in milliseconds. If not set, a default value
    of 10000 ms is used. This feature should mainly reduce replicated log
    messages. (This watch dog is running in its own thread.)
  - The environmental variable FEE_LOGWATCHDOG_TIMEOUT can also be set via the
    start script, when providing it as second parameter (in ms).
    (startFeeServer.sh <FeeServer Name> <Log WatchDog Timeout> )
    Note: you don't need to provided these as parameters to the start script.	
  - Bugfix: The FeeServer produced a segmentation fault in the clean up, when
    no item has been published (affected stop and restart of FeeServer). Fixed!
  - Automatic restart of the FeeServer in case of failed initialisation of the 
    ControlEngine during start up. This is tried three times (can be adjusted 
    in the start script startFeeServer.sh: just set the value of counterInitVal
    'counterInitVal' to the number of retries you want to perform), log 
    messages are provided when performing a retry. If the init fails still 
    after the retries, the FeeServer runs without the ControlEngine.
    
    
Changes for version 0.8.1:
--------------------------
  - start script checks now for availability of rdate and can bin called from 
    any other directory (you don't need to be in the "run" - dir anymore)
  - lowered define for time triggered update of monitored values. For changes 
    see "TIME_INTERVAL_MULTIPLIER" in fee_defines.h (too high value resulted
    in too long time interval between time triggered value updates).
  - NEW INTERFACE functions: 
      The CE is now able to publish INTEGER values as well:
      /**
       * Publishes integer items.
       * Function, called by the Control Engine for each INTEGER service the
       * FeeServer should offer. This must be done before the server starts 
       * serving start()). (is implemented by the FeeServer).
       * NOTE: THIS FUNCTION IS IN ADDITION TO PUBLISH FLOAT VALUES:
       * int publish(Item* item);
       * THE "IntItem" is different to "Item"!
       *
       * @param intItem pointer to the IntItem-struct; contains the inmteger
       *                value-location, the value-name and the default dead 
       *                band for this IntItem.
       *
       * @return FEE_OK, if publishing to server was successful, else an error
       *                code possible errors are:
       *                FEE_WRONG_STATE -> FeeServer is not in state running
       *                FEE_NULLPOINTER -> given Item contains null pointer, 
       *                    IntItem is not published and not stored in list 
       *                    (but CE can continue)
       *                FEE_INSUFFICIENT_MEMORY -> insufficient memory on board
       *                FEE_ITEM_NAME_EXISTS -> item name already exists, item
       *                    is not published and not stored in list 
       *                    (but CE can continue)
       */
      int publishInt(IntItem* intItem);
      
      This function has to be called for every Integer value, that shall be 
      published and monitored. It can only be called during CE-initialization.
      
      The "IntItem" is similar to the "Item", but is taking care of integer
      values:
      /**
       * Typedef IntItem.
       * IntItem is a struct, with a pointer to the integer item value
       * (volatile int* location)
       * and the IntItem name (char* name). This represents the core of an 
       * IntItem. In addition a default dead band is provided.
       * @ingroup feesrv_core
       */
      typedef struct {
        /** 
         * struct value location -> pointer to the actual value of the 
         * IntItem.
         */
        volatile int* location;
        /** struct value name -> name of item.*/
        char* name;
        /** struct value defaultDeadband -> default deadband of this item */
        int defaultDeadband;
      } IntItem; /**< The item which represents a service. */
      
      An "IntItem" wraps the published integer value. There has to be an 
      "IntItem" for each pushlied int value. The name has to be unique, even 
      unique against the float value list. The handling of an "IntItem" is 
      similar to the "Item" used for float values. To initialize an "IntItem"
      an interface function is offered, which basically sets initial "0" to 
      the members:
      /**
       * Function creates an empty, but initialized IntItem.
       * All "members" are set to 0. For further use it is essential, that 
       * these members are filled with the appropriate values, especially the 
       * memory for the '\0'-terminated name has to be still allocated!
       *
       * @return pointer to the new IntItem, if enough memory is available, 
       *                 else NULL
       */
      IntItem* createIntItem();
      
      The integer values are monitored like the float values. An update is 
      trigger when the given deadband is exceeded and after certain time
      intervals. All this is done by the FeeServer framework. Updates are 
      decoupled, so there is always a small time delay between the check/update
      of two values. Monitoring of float and integer values is independent.
      
      Most of the changes in "feeserver.c" are done due to this new feature (
      including a new monitor thread)
  - Bug fix in automatic restart in case if CE init timeout.



--------------------------------------------------------------------------------

For questions and remarks please contact:
-----------------------------------------
Christian Kofler 
Sebastian Bablok <bablok@ift.uib.no>
Bejamin Schockert <schockert@ztt.fh-worms.de>
http://www.ztt.fh-worms.de
(download at https://www.ztt.fh-worms.de/download/alice/)

--------------------------------------------------------------------------------


