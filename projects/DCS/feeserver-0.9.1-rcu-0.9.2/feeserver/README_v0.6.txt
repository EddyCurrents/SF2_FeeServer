---------------------------------------------------------------------------------
                FeeServer version 0.6.8 release notes
---------------------------------------------------------------------------------

Compilation:
------------
To compile the FeeServer please follow these steps:
  - change to the dim folder (feeserver/dim)
  - run ". setup.sh" or "source setup.sh" to prepare your shell (bash) 
  - if you compile FeeServer for the first time on your system:
    - run "make -f makefile_dim.arm" to compile the required DIM Library
  - run "make -f makefile_feeserver.arm" to finally compile FeeServer
  - NOTE: It is important, that the DIM lib is compiled with the flag
    NOTHREADS !!! Else the communication can get stuck. The makefiles set this
    as standart. 
    (NOTE: The formerly working feature "restart FeeServer" and "update 
    FeeServer" does not work with this current configuration. Solution 
    will be presented soon!)
  
  if you want to clean up your system:
    - run "make realclean -f makefile_dim.arm" to clean up DIM stuff
    - run "make cleanserver -f makefile_feeserver.arm" to clean up

Preparation:
------------
You now compiled the version for ArmLinux, so put the binary wherever you use
to execute applications on your board.
You must prepare your envirnonment with some variables to run FeeServer.
You can either adapt the "setDim.sh" in the scripts folder or you can create
these variables by hand everytime:
  - DIM_DNS_NODE for the dns for DIM
  - DIM_SERVER_NAME for the name of the feeserver (as seen in DIM)


Execution:
----------
To execute FeeServer properly please follow these steps:
  - make sure that the dns for DIM is running on the machine you specified
  - finally run feeserver
  

Important Notes:
----------------
  - make sure that the setup.sh is executed only ONCE per shell!
  

---------------------------------------------------------------------------------

State for version 0.6.4:
------------------------
Besides the basic functionality there are several features implemented: 
  - logging via message service, also possible for controlEngine
  - asynchronous check for changes of itemValues via own monitor thread:
    It checks with a configurable update rate for changes of 
    item values that exceed their deadband.


Changes for version 0.6.5:
--------------------------
2004-04-30 and before:
  - adapted date format in all log messages
  - adapted creation of date entries

Changes for version 0.6.6:
--------------------------
2004-05-11:
  - fixed bug in monitor thread
  - implemented safety check for location in monitor thread
  - fixed bug in message channel 

Changes for version 0.6.7:
--------------------------
2004-07-06:
  - some clean up of implementation
  - moved initCE thread functionality (cancel type, cancel state) to FeeServer
    itself: "initializeCE()" is now called from thread-function 
    "threadInitializeCE()".

Changes for version 0.6.8:
--------------------------
2004-07-21:
  - new DIM version 14.04 included
  - exit handler refactored

__________________________  
2004-11-18:
  - minor changes, but no new version number!

