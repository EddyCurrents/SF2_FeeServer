/************************************************************************
 * **
 * **
 * ** This file is property of and copyright by the Department of Physics
 * ** Institute for Physic and Technology, University of Bergen,
 * ** Bergen, Norway.
 * ** In cooperation with Center for Technology Transfer and 
 * ** Telecommunication (ZTT), University of Applied Sciences Worms
 * ** Worms, Germany.
 * **
 * ** This file has been written by Sebastian Bablok,
 * ** Sebastian.Bablok@uib.no
 * **
 * ** Important: This file is provided without any warranty, including
 * ** fitness for any particular purpose. Further distribution of this file,
 * ** even with changes in the code, is only allowed, when this copyright
 * ** and warranty paragraph is kept unchanged and included to the sources. 
 * **
 * **
 * *************************************************************************/

#ifndef FEE_FUNCTIONS_H
#define FEE_FUNCTIONS_H

#include <pthread.h>
#include <stdbool.h>

#include "fee_types.h"

//-----------------------------------------------------------------------------
// Definition of functions for the feeserver, control engine, tests etc.
//
// @date        2003-05-06
// @author      Christian Kofler, Sebastian Bablok
//-----------------------------------------------------------------------------

/**
 * This function initializes the FeeServer and initializes and starts a part of
 * the CE - thread (control engine). Afterwards the DIM-framework is started.
 * @ingroup feesrv_core
 */
void initialize();

/**
 * CommandHandler used by DIM to execute an incoming command.
 * It is called every time the server receives new command data. Here, the
 * FeePacket, containing the command is unmarshalled and the data analyzed.
 * Commands for the FeeServer itself are executed immediately, commands for
 * the CE are delivered further (this is handled in an own thread, and controlled
 * by a watchdog). The whole procedure is secured with mutexes.
 *
 * @param tag pointer to the commandID (used by the DIM-framework).
 * @param address pointer to the command-data
 * @param size pointer to the size of command-data
 * @ingroup feesrv_core
 */
void command_handler(int* tag, char* address, int* size);

/**
 * Called when acknowledge-data of a command has to be sent back.
 * This function provides the data and size of it to the DIM-library,
 * so it is possible to change dynamically the size of the data.
 *
 * @param tag pointer to the serviceID (used by the DIM-framework)
 * @param address pointer to the result data to be send
 * @param size pointer to the size of the data
 * @ingroup feesrv_core
 */
void ack_service(int* tag, char** address, int* size);

/**
 * Function to catch interrupt SIGINT and perform a proper cleanup before
 * exit. This has to be registered during initialisation of FeeServer with
 * singal(...) (see man pages for usage). Depending on the FeeServer state,
 * different exit procedures are performed.
 *
 * @param sig the signal that is catched.
 * @ingroup feesrv_core
 */
void interrupt_handler(int sig);

/**
 * FeeServer exit handler, called by the FeeServer itself, when exit has been
 * signaled. A cleanUp is permformed before leaving the FeeServer application.
 *
 * @param state exit state
 * @ingroup feesrv_core
 */
void fee_exit_handler(unsigned int state);

/**
 * Exit handler called by the DIM framework. In most cases, this dummy exit
 * handler is used to disable the DIM exit command used by an ambitious user.
 * This function is also called by the framework, if a service of this FeeServer
 * already exists in the DIM_DNS, when it registers its services. In this case,
 * a specified exit state is provided to the fee_exit_handler, which exits the
 * FeeServer.
 *
 * @param bufp exit state
 * @ingroup feesrv_core
 */
void dim_dummy_exit_handler(int* bufp);

/**
 * Dim error handler to catch messages from the DIM framework.
 * With registering this function to the DIM framework, the FeeServer gets
 * all messages created by the DIM framework (they are no longer written to
 * stdout/stderr by the framework - if compiled as __DEBUG this function does
 * it). The FeeServer is now responsible to take according actions.
 * The retrieved messages will be send further, if possible (FeeServer in
 * running or error state).
 *
 * @param severity defines the log level of the recieved message
 * @param error_code represents the DIM internal error code (not used now)
 * @param msg the message provided by the framework
 * @ingroup feesrv_core
 */
void dim_error_msg_handler(int severity, int error_code, char* msg);

/**
 * Starts the serving functionality of the DIM-framework and the monitoring thread.
 * Before starting, a command for the server, a service for the acknowledge and
 * the message service are added to the server. The server name is taken from
 * the enviromental variable FEE_SERVER_NAME. In case of errors while starting
 * the serving functionality of the framework, the above mentioned channels are
 * unregistered before leaving this function. If the CE has not been successfully
 * initialized or the monitoring thread could not be started, the FeeServer runs
 * limited functionality (commands to the CE are not delivered further, no
 * monitoring is provided); an apropriated error message is offered for this case.
 * No services are added, when the server is already in the state RUNNING!
 *
 * @param initState the initial state of the ACK - service
 *
 * @return FEE_OK, if starting was successful or server is already running, else
 *                      the error code FEE_FAILED is given back.
 * @ingroup feesrv_core
 */
int start(int initState);

/**
 * This function initializes and starts the thread, which takes care of the
 * monitoring of the published values. Should be called AFTER the DIM-server
 * has been started.
 *
 * @return FEE_OK, if thread has been successfully started,
 *				 else FEE_MONITORING_FAILED
 * @ingroup feesrv_core
 */
int startMonitorThread();

/**
 * This function initializes and starts the LogWatchDog thread, which takes
 * care of publishing "hold back replicated log messages" after a given
 * timeout. The amount of how often this message has been hold back as well as
 * the description of this message is send after the timeout and the counter
 * "replicatedMsgCount" is set back. If the starting of the tread fails, the
 * messenger can just run without it. Then no replicated MSGs are held back.
 *
 * @return FEE_OK, if thread has been successfully started,
 *               else FEE_THREAD_ERROR
 * @ingroup feesrv_core
 */
int startLogWatchDogThread();

/**
 * Function, which should run in an own thread and monitors the published
 * values. The values are periodically checked, if they exceed a given deadband
 * around the lastTransmittedValue. A value, which exceeds the deadband is
 * updated via DIM and the new lastTransmittedValue is stored. Each round has in
 * sum a timewait of "updateRate", which can be set via a FeeServer command. The
 * sleep in between each item is the "updateRate" divided by the number of
 * itemNodes.
 * @ingroup feesrv_core
 */
void monitorValues();

/**
 * This function takes care of publishing "hold back replicated log messages".
 * After a given timeout this function updates the message channel.
 * The amount of how often this message has been hold back as well as
 * the description of this message are sent after the timeout and the counter
 * "replicatedMsgCount" is set back. The timeout is given by the global variable
 * "logWatchDogTimeout". A default value is provided by the FeeServer, changes
 * can be applied before starting the FeeServer via setting the environmental
 * variable "FEE_LOGWATCHDOG_TIMEOUT".
 * The function should run in its own thread.
 *
 * @ingroup feesrv_core
 */
void runLogWatchDog();

/**
 * Function to check if replicated log messages are pending. If so,
 * the amount of how often this message has been hold back as well as
 * the description of this message are sent. Afterwards the counter is set
 * back to zero. Don't call createLogMessage(..) inside this function,
 * it is executed after the logger mutex is locked!
 *
 * @return true, if a replicated log message has been pending, else false
 *
 * @ingroup feesrv_core
 */
bool checkReplicatedLogMessage();

/**
 * Adds an item to the doubly linked list.
 * Function to add an item for monitoring to a local doubly linked list.
 *
 * @param _id DIM ServiceID of the item
 * @param _item pointer to the item-struct; contains value-location, value-name,
 *					and the default dead band of the item.
 * @ingroup feesrv_core
 */
void add_item_node(unsigned int _id, Item* _item);

/**
 * Searches for an item specified by its name.
 *
 * @param name the name of the desired item .
 *
 * @return the itemNode, if item is in the list, else 0.
 * @ingroup feesrv_core
 */
ItemNode* findItem(char* name);

/**
 * Prepares a proper exit of the commandHandler in case of an error during
 * execution of the current command. This includes sending a log message which
 * indicates the reason for exiting, sending the appropriated error code back
 * and releasing occupied ressources.
 *
 * @param id packet ID for the creation of the ACK packet
 * @param errorCode for the creation of the ACK packet
 * @param msgType event type for the created log message
 * @param message description of the reason for leaving the commandHandler
 * @ingroup feesrv_core
 */
void leaveCommandHandler(unsigned int id, short errorCode,
			unsigned int msgType, char* message);

/**
 * Unlocks mutex for watchdog of issue thread and takes appropriate actions
 * on eventually occuring errors. If the unlock fails, the state is changed to
 * ERROR because the CE would hang up next time a command should be executed!
 * @ingroup feesrv_core
 */
void unlockIssueMutex();

/**
 * Function to call issue in seperate thread. Here a command to the CE is
 * delivered further and, when the execution has been finished, a signal is sent
 * to the watch dog, that controls this thread.
 *
 * @param threadParam pointer to IssueStruct, used for access by the CE.
 *
 * @return pointer to data, returned by thread; in this case always 0.
 * @ingroup feesrv_core
 */
void* threadIssue(void* threadParam);

/**
 * Function intializes the thread for CE start up and executes initializeCE().
 *
 * @ingroup feesrv_core
 */
void threadInitializeCE();

/**
 * Function creates the FEE-header for an ACK packet with the listed elements.
 *
 * @param id the id of the packet
 * @param errorCode the error code, provided by this ACK
 * @param huffmanFlag indicates if the payload is huffman encoded
 * @param checksumFlag indicates if a checksum is calculated
 * @param checksum the checksum of the packet / payload
 *
 * @return pointer to the header data.
 * @ingroup feesrv_core
 */
char* createHeader(unsigned int id, short errorCode, bool huffmanFlag,
					bool checksumFlag, int checksum);

/**
 * Function to marshall the header for an ACK packet out of a given CommandHeader.
 *
 * @param pHeader pointer to a header struct, containing the needed values
 *
 * @return the marshalled header
 * @ingroup feesrv_core
 */
char* marshallHeader(CommandHeader* pHeader);

/**
 * Function to test the submitted checksum of an incomming FeePacket against the
 * calculated checksum of the payload.
 *
 * @param payload the command-payload, from which the checksum is calculated.
 *				(NOTE: use in the corresponding InterComLayer
 *				"unsigned char*" for the checksum calculation; [signed] char*
 *				calculates a different checksum !)
 * @param size the size of the payload in bytes.
 * @param checksum the checksum of FeePacket to compare with freshly calculated.
 *
 * @return true, if both checksums are equal, else false.
 * @ingroup feesrv_core
 */
bool checkCommand(char* payload, int size, unsigned int checksum);

/**
 * Function to calculate a checksum with the adler32 algorithm.
 *
 * @param buffer data to calculate the checksum of. (NOTE: It is important to
 *				use unsigned char* to be compatible to different plattforms.)
 * @param size the size of the data.
 *
 * @return the calculated checksum.
 * @ingroup feesrv_core
 */
unsigned int calculateChecksum(unsigned char* buffer, int size);

/**
 * This function checks the address of the location of a monitored FLOAT value
 * for bitflips and tries to repair the location, if possible.
 * To fullfill this task, the function has access to an address backup and two
 * checksum values.
 * The check: First the location and the location backup are compared, if
 * unequal, the location is checked versus the first checksum. If this fails,
 * the location backup is checked versus the first checksum. And so on with
 * the second checksum, if something fails, or checksums are unidentical.
 * If the right location can be determined, the other variables will be fixed.
 *
 * @param node the item node (float value item) containing the needed data.
 *
 * @return true, if check or repair try has been successful, else false.
 * @ingroup feesrv_core
 */
bool checkLocation(ItemNode* node);

/**
 * Function checks an event against the current log level.
 * If it is not included, the function returns false, else true.
 *
 * @param event the event type of the current event.
 *
 * @return true, if event type is included in the current log level, else false.
 * @ingroup feesrv_core
 */
bool checkLogLevel(int event);

/**
 * FeeServer command to update and restart itself. The function retrieves the
 * memory, containing the binary, from the issueStruct and stores it to its
 * filesystem. Afterwards the FeeServer exits with a specified exit state, which
 * can be used by the starting script to trigger further actions.
 *
 * @param issueParam pointer to the struct containing length and data for the
 *				new binary of the FeeServer.
 * @ingroup feesrv_core
 */
void updateFeeServer(IssueStruct* issueParam);

/**
 * Calls the triggerRestart(int exitVal) function with FEE_EXITVAL_RESTART as
 * exit value. FEE_EXITVAL_RESTART should be "2".
 *
 * @see triggerRestart
 *
 * @ingroup feesrv_core
 */
void restartFeeServer();

/**
 * A log message is generated and the cleanUp - function called.
 * Afterwards, the FeeServer exits with a specified exit state.
 * The start script can analyze this state and then trigger a restart of the
 * FeeServer. This function is also used for trying another CE init.
 *
 * @param exitVal defines the exit value that the FeeServer - executable
 *          returns when the restart is triggered.
 *
 * @ingroup feesrv_core
 */
void triggerRestart(int exitVal);

/**
 * Function sets the deadband for a specified item.
 *
 * @param issueParam pointer to issueParam struct containing the item name and
 *				the new dead band.
 *
 * @return FEE_OK, if setting was successful, else an error value (see fee_errors.h)
 * @ingroup feesrv_core
 */
int setDeadband(IssueStruct* issueParam);

/**
 * Function sets the deadband for a multiple items.
 *
 * @param name (with wildcard) for the broadcast of the new deadband.
 * @param newDeadbandBC the new deadband (is divided by 2 to fit to the threshold)
 *
 * @return the number of items, which have been changed
 * @ingroup feesrv_core
 */
unsigned int setDeadbandBroadcast(char* name, float newDeadbandBC);

/**
 * Function to get the deadband for a specified item.
 *
 * @param issueParam pointer to issueParam struct containing the instruction data,
 *				the deadband is written in the issue - struct (result).
 *
 * @return FEE_OK, if getting was successful, else an error value (see fee_errors.h)
 * @ingroup feesrv_core
 */
int getDeadband(IssueStruct* issueParam);

/**
 * Function to set a new timeout value for watchdog of issue - thread.
 *
 * @param issueParam pointer to issueParam struct containing the new timeout.
 *
 * @return FEE_OK, if setting was successful, else an error value (see fee_errors.h)
 * @ingroup feesrv_core
 */
int setIssueTimeout(IssueStruct* issueParam);

/**
 * Function to get the current timeout value for watchdog of issue - thread.
 *
 * @param issueParam pointer to issueParam struct containing the instruction data,
 *				the timeout is written in the issue - struct (result).
 *
 * @return FEE_OK, if getting was successful, else an error value (see fee_errors.h)
 * @ingroup feesrv_core
 */
int getIssueTimeout(IssueStruct* issueParam);

/**
 * Function to set a new value for the update rate (used by the monitor thread)
 *
 * @param issueParam pointer to issueParam struct containing the new update rate.
 *
 * @return FEE_OK, if setting was successful, else an error value (see fee_errors.h)
 * @ingroup feesrv_core
 */
int setUpdateRate(IssueStruct* issueParam);

/**
 * Function to get the current update rate (used by the monitor thread)
 *
 * @param issueParam pointer to issueParam struct containing the instruction data,
 *				the update rate is written in the issue - struct (result).
 *
 * @return FEE_OK, if getting was successful, else an error value (see fee_errors.h)
 * @ingroup feesrv_core
 */
int getUpdateRate(IssueStruct* issueParam);

/**
 * Function to set a new logLevel for the FeeServer.
 * (can be a cumulation of different levels. NOTE: MSG_ALARM is always set!).
 *
 * @param issueParam pointer to issueParam struct containing the new log level.
 *
 * @return FEE_OK, if setting was successful, else an error value (see fee_errors.h)
 * @ingroup feesrv_core
 */
int setLogLevel(IssueStruct* issueParam);

/**
 * Function to get the current logLevel for the FeeServer
 *
 * @param issueParam pointer to issueParam struct containing the instruction data,
 *				the loglevel is written in the issue - struct (result).
 *
 * @return FEE_OK, if getting was successful, else an error value (see fee_errors.h)
 * @ingroup feesrv_core
 */
int getLogLevel(IssueStruct* issueParam);

/**
 * This function checks conditions of update rate, and informs, if necessary,
 * the CE about its value. Therefore, a FeeProperty, containing the update rate,
 * is created. NOTE: only if it makes sense, the CE is informed.
 * @ingroup feesrv_core
 */
void provideUpdateRate();

/**
 * Copies a given message into a new Message Struct.
 * All values are copied.
 *
 * @param orgMsg pointer to the struct, whose values should be copied to the
 *		new MessageStruct
 *
 * @return the copied MessageStruct
 * @ingroup feesrv_core
 */
//MessageStruct copyMessage(const MessageStruct* const orgMsg);


/**
 * This function cleans up everything when finishing the server.
 * It also calls a cleanUp  for the control engine.
 * @ingroup feesrv_core
 */
void cleanUp();

/**
 * Deletes the complete item list.
 *
 * @return FEE_OK, if removing was successful, else FEE_FAILED
 * @ingroup feesrv_core
 */
int deleteItemList();

/**
 * Function initializes each member of a given issueStruct with 0.
 *
 * @param issueStr pointer to the issueStruct to initialize.
 * @ingroup feesrv_core
 */
void initIssueStruct(IssueStruct* issueStr);

/**
 * Function initializes each member of a given ItemNode with 0.
 *
 * @param iNode pointer to the ItemNode to initialize.
 * @ingroup feesrv_core
 */
void initItemNode(ItemNode* iNode);

/**
 * Function to initialize the message struct with the first event:
 * Info of starting the FeeServer. In addition the back Message struct for
 * storing the last message is also initiated containing a different
 * description compared to the original message struct.
 *
 * @ingroup feesrv_core
 */
void initMessageStruct();

/**
 * Function removes the service of all Items in the ItemList from the
 * DIM framework. This is the exact opposite to all publish calls, where
 * items are added as services to the DIM framework.
 * It is essential to call this function in case of a CE_ERROR state before
 * start() is called in order to survive!
 * @ingroup feesrv_core
 */
void unpublishItemList();


////   --------- NEW FEATURE SINCE VERSION 0.8.1 (2007-06-12) ---------- /////

/**
 * Function, which should run in an own thread and monitors the published
 * integer values. The values are periodically checked, if they exceed a given
 * deadband around the lastTransmittedIntValue. A value, which exceeds the
 * deadband is updated via DIM and the new lastTransmittedIntValue is stored.
 * Each round has in sum a timewait of "updateRate", which can be set via a
 * FeeServer command. The sleep in between each IntItem is the "updateRate"
 * divided by the number of IntItemNodes.
 * @ingroup feesrv_core
 */
void monitorIntValues();

/**
 * This function checks the address of the location of a monitored INTEGER
 * value for bitflips and tries to repair the location, if possible.
 * To fullfill this task, the function has access to an address backup and two
 * checksum values.
 * The check: First the location and the location backup are compared, if
 * unequal, the location is checked versus the first checksum. If this fails,
 * the location backup is checked versus the first checksum. And so on with
 * the second checksum, if something fails, or checksums are unidentical.
 * If the right location can be determined, the other variables will be fixed.
 *
 * @param node the IntItem node containing the needed data.
 *
 * @return true, if check or repair try has been successful, else false.
 * @ingroup feesrv_core
 */
bool checkIntLocation(IntItemNode* node);

/**
 * Adds an IntItem to the doubly linked list of IntItemNodes.
 * Function to add an IntItem for monitoring to a local doubly linked list.
 *
 * @param _id DIM ServiceID of the IntItem
 * @param _int_item pointer to the IntItem-struct; contains value-location,
 *				value-name and the default dead band of the IntItem.
 * @ingroup feesrv_core
 */
void add_int_item_node(unsigned int _id, IntItem* _int_item);

/**
 * Searches for an IntItem specified by its name.
 *
 * @param name the name of the desired IntItem .
 *
 * @return the IntItemNode, if item is in the list, else 0.
 * @ingroup feesrv_core
 */
IntItemNode* findIntItem(char* name);

/**
 * This function checks the address of the location of a monitored integer
 * value for bitflips and tries to repair the location, if possible.
 * To fullfill this task, the function has access to an address backup and two
 * checksum values.
 * The check: First the location and the location backup are compared, if
 * unequal, the location is checked versus the first checksum. If this fails,
 * the location backup is checked versus the first checksum. And so on with
 * the second checksum, if something fails, or checksums are unidentical.
 * If the right location can be determined, the other variables will be fixed.
 *
 * @param intNode the IntItem node containing the needed data.
 *
 * @return true, if check or repair try has been successful, else false.
 * @ingroup feesrv_core
 */
//bool checkLocation(IntItemNode* intNode);

/**
 * Deletes the complete IntItem list.
 *
 * @return FEE_OK, if removing was successful, else FEE_FAILED
 * @ingroup feesrv_core
 */
int deleteIntItemList();

/**
 * Function initializes each member of a given IntItemNode with 0.
 *
 * @param intItemNode pointer to the IntItemNode to initialize.
 * @ingroup feesrv_core
 */
void initIntItemNode(IntItemNode* intItemNode);

/**
 * Function removes the service of all IntItems in the IntItemList from the
 * DIM framework. This is the exact opposite to all publishInt calls, where
 * IntItems are added as services to the DIM framework.
 * It is essential to call this function in case of a CE_ERROR state before
 * start() is called in order to survive!
 * @ingroup feesrv_core
 */
void unpublishIntItemList();


// ------------ used for new memory managemnt ----- //

/**
 * Function to find a MemoryNode identifyied by the identity address of the
 * memory block
 *
 * @param addr the identifying address
 *
 * @return pointer to the desired MemoryNode; if no corresponding node is found
 *			the pointer is NULL.
 */
MemoryNode* findMemoryNode(void* addr);

/**
 * Function to free a MemoryNode (memory block and all its meta data) from a
 * given Memory Node.
 *
 * @param node pointer to the MemoryNode to free.
 */
void freeMemoryNode(MemoryNode* node);

/**
 * Function to create a MemoryNode: allocating the corresponding memory,
 * filling the meta data struct and adding the node to the list of MemoryNodes.
 * This also includes setting of the correct identifying address.
 *
 * @param size size of the desired memory in bytes
 * @param type the type for which the memory will be usedd later on
 *			(this is not really necessary, in most cases set to 'c')
 * @param module the module which is acquiring the memory (max 30 chars).
 * @param preSize the of memory, that shall be allocated as prefix block to the
 *			memory. This is used if later on a header shall be put in front.
 *
 * @return pointer to the new cerated and inserted MemoryNode.
 */
MemoryNode* createMemoryNode(unsigned int size, char type, char* module,
		unsigned int preSize);

/**
 * Function to clean up the whole MemoryNode list. Be sure to call this
 * function only during cleanup and AFTER all other modules are killed.
 */
void cleanupMemoryList();


/// --- New function for feature Char Channel (v.0.8.2b - 28.07.07) --- ///

/**
 * Adds an CharItem to the doubly linked list of CharItemNodes.
 *
 * @param _id DIM ServiceID of the CharItem
 * @param _char_item pointer to the CharItem-struct; contains the user_routine
 *			pointer (function pointer), the service-name and the tag, which  
 *			identifies this service in the callback user_routine.
 * @ingroup feesrv_core
 */
void add_char_item_node(unsigned int _id, CharItem* _char_item);

/**
 * Searches for an CharItem specified by its name.
 *
 * @param name the name of the desired CharItem .
 *
 * @return pointer to the CharItemNode, if item is in the list, else 0.
 * @ingroup feesrv_core
 */
CharItemNode* findCharItem(char* name);

/**
 * Deletes the complete CharItem list.
 *
 * @return FEE_OK, if removing was successful, else FEE_FAILED
 * @ingroup feesrv_core
 */
int deleteCharItemList();

/**
 * Function initializes each member of a given CharItemNode with 0.
 *
 * @param charItemNode pointer to the CharItemNode to initialize.
 * @ingroup feesrv_core
 */
void initCharItemNode(CharItemNode* charItemNode);

/**
 * Function removes the services of all CharItems in the CharItemList from the
 * DIM framework. This is the exact opposite to all publishChar calls, where
 * CharItems are added as services to the DIM framework.
 * It is essential to call this function in case of a CE_ERROR state before
 * start() is called in order to survive!
 * @ingroup feesrv_core
 */
void unpublishCharItemList();
 


//--------------------------------- Debug Methods -----------------------------

#ifdef __DEBUG
/**
 * Wrapper method to print the data on screen for debugging purpose.
 *
 * @param data the data to print
 * @param start the start point, from where to print the data
 * @param size the size of the data
 * @ingroup feesrv_core
 */
void printData(char* data, int start, int size);
#endif


//--------------------------------- Unit Test convenience methods -------------

#ifdef __UTEST
/**
 * Returns the current state of the feeserver
 * possible statees are:
 * - COLLECTING if the server is willing to get Items from the Control Engine
 * - RUNNING if the server is providing DIM Services / takes DIM Commands
 *
 * @return the state of the feeserver
 * @ingroup feesrv_core
 */
const int getState();

/**
 * Offers the pointer to the first node of itemlist to the test-cases.
 *
 * @return pointer to first node of itemlist
 * @ingroup feesrv_core
 */
const ItemNode* getFirstNode();

/**
 * Offers the pointer to the last node of itemlist to the test-cases.
 *
 * @return pointer to last node of itemlist
 * @ingroup feesrv_core
 */
const ItemNode* getLastNode();

/**
 * Gives bakc the size of the doubly-linked-list.
 *
 * @return size of list
 * @ingroup feesrv_core
 */
int listSize();

/**
 * Offers pointer to the acknowledge data of a command to the test-cases.
 *
 * @return pointer to ACK-Data
 * @ingroup feesrv_core
 */
const char* getCmndACK();

/**
 * Allows the test-cases to modify state.
 * Allowed states are <i> COLLECTING </i> for collecting services,<br>
 * and <i> RUNNING </i> to signal server is serving.
 *
 * @param newState value, representing new state
 * @ingroup feesrv_core
 */
void setState(int newState);

/**
 * Allows the test-cases to change the result data of a command.
 *
 * @param newData pointer the new result data.
 * @ingroup feesrv_core
 */
void setCmndACK(char* newData);

/**
 * Allows the test-cases to set the size of the result data of a command.
 *
 * @param size of the result data.
 * @ingroup feesrv_core
 */
void setCmndACKSize(int size);

/**
 * Function to set the server name
 *
 * @param name the servername
 * @ingroup feesrv_core
 */
void setServerName(char* name);

/**
 * Function to clear server name.
 * The memory for servername is freed.
 * @ingroup feesrv_core
 */
void clearServerName();

/**
 * Function to tell the DIM-Framework to stop server.
 * @ingroup feesrv_core
 */
void stopServer();

/**
 * Function to pass the pointer of the global variable, which is used to
 * send as signal when the CE is ready.
 * @ingroup feesrv_core
 */
pthread_cond_t* getInitCondPtr();

/**
 * Function to pass the pointer of the global variable, which is used to
 * lock inside the thread.
 * @ingroup feesrv_core
 */
pthread_mutex_t* getWaitInitMutPtr();

#endif

#endif
