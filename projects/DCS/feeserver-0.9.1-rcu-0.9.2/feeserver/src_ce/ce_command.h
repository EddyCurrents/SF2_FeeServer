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

#ifndef CE_COMMAND_H
#define CE_COMMAND_H

#include "fee_types.h"

#ifdef __cplusplus
extern "C" {
#endif
/** @defgroup feesrv_ce The FeeServer ControlEngine (CE)
 * The ControlEngine is the abstraction layer for the actual hardware access.
 */

/** @defgroup feesrv_ceapi The ControlEngine API
 * Definition of the API between FeeServer core and ControlEngine
 * @ingroup feesrv_ce
 */

/**
 * Issue function to give a command to the control engine and to receive the
 * result of this command. This must be implemented by the CE, and will be
 * called by the FeeServer. The memory of the command is allocated by the
 * FeeServer and also freed by it, the memory for the result is allocated by
 * the CE, and will be freed by the FeeServer. The SIZE represents, when
 * called, the size of the command; when returning, it should be the size of
 * the result data (both in bytes). As return value an errorCode is offered,
 * 0 stands for everything went all right (FEE_OK).
 *
 * @see fee_errors.h for more details on the error code.
 *
 * @param command pointer to the memory of the command for the CE (allocated
 *			and freed by FeeServer).
 * @param result pointer to a pointer for the CE to tell the FeeServer where
 *			it can find the result data (allocated by CE, freed by FeeServer).
 * @param size when calling it keeps the size of the command (in bytes), when
 *			returning, it should contain the size of the result (in bytes).
 *
 * @return an errorCode is returned, 0 if the call was successful (FEE_OK). More
 *			details in fee_errors.h .
 * @ingroup feesrv_ceapi
 */
int issue(char* command, char** result, int* size);

/**
 * Function to clean up the CE and its components. This function is implemnted
 * by the CE.
 * @ingroup feesrv_ceapi
 */
void cleanUpCE();

/**
 * Creates a log message.
 * Via this function both, the FeeServer and the ControlEngine, can send messages
 * to the upper layers. Depending on the current LogLevel of the FeeServer the
 * provided message will be sent or not. (is implemented by the FeeServer)
 *
 * @param type the event type of this message - the priority / severity
 *				(see fee_loglevels.h for more)
 * @param description the message body HAS TO BE TERMINATED wiht NULL!
 * @param origin who created this message (most likely "FeeServer" or "CE") HAS
 *				TO BE TERMINATED with NULL!
 * @ingroup feesrv_ceapi
 */
void createLogMessage(unsigned int type, char* description, char* origin);

/**
 * Function, that initalizes the control engine and its components.
 * Should be called within a dedicated thread. If items are published, this
 * function is reponsible for updating the corresponding values regularly.
 * This function is implemented by the CE.
 * @ingroup feesrv_ceapi
 */
void initializeCE();

/**
 * Publishes items.
 * Function, called by the Control Engine for each service the DIM-Server
 * should offer. This must be done before the server starts serving (start()).
 * (is implemented by the FeeServer).
 *
 * @param item pointer to the item-struct; contains the value-location, the
 *			value-name and the default dead band for this item.
 *
 * @return FEE_OK, if publishing to server was successful, else an error code
 *				possible errors are:
 *				FEE_WRONG_STATE -> FeeServer is not in state running
 *				FEE_NULLPOINTER -> given Item contains null pointer, item is
 *                  not published and not stored in list (but CE can continue)
 *				FEE_INSUFFICIENT_MEMORY -> insufficient memory on board
 *				FEE_ITEM_NAME_EXISTS -> item name already exists, item is
 *					not published and not stored in list (but CE can continue)
 * @ingroup feesrv_ceapi
 */
int publish(Item* item);

/**
 * Sends a signal to the calling thread, that the initialisation of the CE is finished.
 * The thread can/must stay alive afterwards to do other stuff, if needed.
 * (When Items are published, it has to stay alive).
 * (is implemented by the FeeServer)
 *
 * @param ceState tells the FeeServer, if CE has been completely and successfully
 *				initialized, else an errorCode is provided.
 *
 * @see fee_errors.h
 * @ingroup feesrv_ceapi
 */
void signalCEready(int ceState);

/**
 * Function to signal the CE, that a property of the FeeServer has changed, that
 * might be interessting to the ControlEngine. For now, it is only forseen, that
 * the update rate of deadband checks is propagated to the CE. This function
 * provides a FeeProperty, which consists of a flag, telling what kind of
 * property is signaled, and the value itself.
 *
 * @param prop pointer to the FeeProperty, providing flag and property value
 * @ingroup feesrv_ceapi
 */
void signalFeePropertyChanged(FeeProperty* prop);

/**
 * Triggers an update of the service given by serviceName. This function can
 * be called by the CE and is executed by the FeeServer. This function is only
 * executed, when the FeeServer is in the state RUNNING, else an error is
 * returned.
 *
 * @param serviceName the name of the service, which shall be updated.
 *
 * @return number of updated clients, if service is available and has been
 * 			updated, else an error code (negative value).
 *
 * @see fee_errors.h
 * @ingroup feesrv_ceapi
 */
int updateFeeService(char* serviceName);

/**
 * Function to write out a benchmark timestamp
 * If the environmental variable "FEE_BENCHMARK_FILENAME" is specified, the
 * printout is written to this file, if not a log message with the same content
 * is generated, event level MSG_SUCCESS_AUDIT.
 * This function is only available, if the FeeServer is compiled
 * with the flag __BENCHMARK.
 *
 * @param msg text containing the location, where the benchmark timestamp is
 *				generated. This text is prefixed to the benchmark timestamp.
 *				NOTE: The string has to be NULL terminated.
 * @ingroup feesrv_ceapi
 */
void createBenchmark(char* msg);


////   ---- NEW FEATURE SINCE VERSION 0.8.1 (2007-06-12) ----- /////

/**
 * Publishes integer items.
 * Function, called by the Control Engine for each INTEGER service the
 * FeeServer should offer. This must be done before the server starts serving
 * start()). (is implemented by the FeeServer).
 * NOTE: THIS FUNCTION IS IN ADDITION TO PUBLISH FLOAT VALUES:
 * int publish(Item* item);
 * THE "IntItem" is different to "Item"!
 *
 * @param intItem pointer to the IntItem-struct; contains the inmteger
 * value-location, the value-name and the default dead band for this IntItem.
 *
 * @return FEE_OK, if publishing to server was successful, else an error code
 *				possible errors are:
 *				FEE_WRONG_STATE -> FeeServer is not in state running
 *				FEE_NULLPOINTER -> given Item contains null pointer, item is
 *                  not published and not stored in list (but CE can continue)
 *				FEE_INSUFFICIENT_MEMORY -> insufficient memory on board
 *				FEE_ITEM_NAME_EXISTS -> item name already exists, item is
 *					not published and not stored in list (but CE can continue)
 * @ingroup feesrv_ceapi
 */
int publishInt(IntItem* intItem);

/**
 * Function creates an empty, but initialized Item.
 * All "members" are set to 0. For further use it is essential, that these
 * members are filled with the appropriate values, especially the memory
 * for the '\0'-terminated name has to be still allocated!
 *
 * @return pointer to the new Item, if enough memory is available, else NULL.
 * @ingroup feesrv_ceapi
 */
Item* createItem();

/**
 * Function creates an empty, but initialized IntItem.
 * All "members" are set to 0. For further use it is essential, that these
 * members are filled with the appropriate values, especially the memory
 * for the '\0'-terminated name has to be still allocated!
 *
 * @return pointer to the new IntItem, if enough memory is available, else NULL
 * @ingroup feesrv_ceapi
 */
IntItem* createIntItem();

/**
 * Function creates an initialized and filled Item.
 * All "members" are set according to the input parameters and a pointer to
 * the new created Item is returned.
 *
 * @param floatLocation pointer to the actual float value that shall be
 *            published. This location must NOT change after publishing.
 * @param itemName desired name of the corresponding service (Null terminated)
 * @param defDeadband chosen default deadband of the service
 *
 * @return pointer to the new Item, if enough memory is available, else NULL.
 * @ingroup feesrv_ceapi
 */
Item* fillItem(float* floatLocation, char* itemName, float defDeadband);

/**
 * Function creates an initialized and filled IntItem.
 * All "members" are set according to the input parameters and a pointer to
 * the new created IntItem is returned.
 *
 * @param intLocation pointer to the actual int value that shall be
 *            published. This location must NOT change after publishing.
 * @param itemName desired name of the corresponding service (Null terminated)
 * @param defDeadband chosen default deadband of the service
 *
 * @return pointer to the new IntItem, if enough memory is available, else NULL
 * @ingroup feesrv_ceapi
 */
IntItem* fillIntItem(int* intLocation, char* itemName, int defDeadband); 


// ---------- used for new memory management ------- //

/**
 * Interface function to allocate memory. The CE can make the FeeServer
 * allocated and administrade memory for the CE with this function. If the
 * memory will be lateron used for the acknowledge data, the CE can indicate
 * that, so the memory already contains a prefix memory block for the
 * corresponding protocol header.
 *
 * @param size the size of the desired memory in bytes
 * @param type the data type for which memory will be used.
 *			's', 'i', 'l' - for short, integer, long
 *			'f', 'd' - for float and double
 *			'c', 's' - for char and char array (string)
 *			'x' - for complex
 *			Capital letters are used for unsigned versions.
 *			(in most case setting to 'x' = complex is sufficient).
 * @param module the name of the module that acquires the memory (max. 29 chars)
 * @param prefixPurpose defines the purpose of the memory: if no prefix memory
 *			block is required set to '0', if the memory will be used for the
 *			acknoledge data of an issue-call set to 'A'. (more might follow)
 * @param ptr out pointer, which will contain the allocted memory after the
 *			function returns (NOTE: the pointer points to the memory, which can
 *			be used by the calling module, it does NOT point to any possible
 *			memory block for headers or so which might prefixe the memory).
 *
 * @return FEE_OK on success, else an error code is returned:
 *			- FEE_INVALID_PARAM: invalid parameter for size or prefixPurpose
 *			- FEE_FAILED: Unable to create memory block (in most cases
 *				insufficient memory). 
 * @ingroup feesrv_ceapi
 */
int allocateMemory(unsigned int size, char type, char* module,
		char prefixPurpose, void** ptr);

/**
 * Interface function to free memory allocated by the FeeServer with the call
 * of allocateMemory(...). With this function the CE can release memory that is
 * under the management of the FeeServer.
 *
 * @param addr the (identifying) address of the memory that shall be freed;
 *			this must be the address returned by 'ptr' of the allocateMemory()
 *			function.
 *
 * @return FEE_OK on success, else an error code is returned:
 *			- FEE_NULLPOINTER: addr is a NULL pointer
 *			- FEE_INVALID_PARAM: addr is not in list of managed memory blocks
 * @ingroup feesrv_ceapi
 */
int freeMemory(void* addr);



/// --- NEW FEATURE SINCE VERSION 0.8.2 [Wrapper funcs] (2007-07-27) --- ///

// Wrap for Float Service values
/**
 * Wrapper function for publish() to indicate that a float value is published.
 * This function just calls publish().
 * 
 * @param floatItem the float item to be published 
 *				(is passed further to publish(Item* item); 
 *				FloatItem is a typedef to Item)
 *
 * @return FEE_OK, if publishing to server was successful, else an error code
 *				possible errors are:
 *				FEE_WRONG_STATE -> FeeServer is not in state running
 *				FEE_NULLPOINTER -> given Item contains null pointer, item is
 *                  not published and not stored in list (but CE can continue)
 *				FEE_INSUFFICIENT_MEMORY -> insufficient memory on board
 *				FEE_ITEM_NAME_EXISTS -> item name already exists, item is
 *					not published and not stored in list (but CE can continue)
 * @ingroup feesrv_ceapi
 */
int publishFloat(FloatItem* floatItem);

/**
 * Wrapper function for createItem() to indicate that a float value is created.
 * This function just calls createItem().
 *
 * @return pointer to the new FloatItem, if enough memory is available, else 
 *			NULL. FloatItem is a typedef to Item.
 * @ingroup feesrv_ceapi
 */
FloatItem* createFloatItem();

/**
 * Wrapper function for fillItem() to indicate that a float value is filled.
 * FloatItem is a typedef for Item.
 * Function creates an initialized and filled Item.
 * All "members" are set according to the input parameters and a pointer to
 * the new created Item is returned.
 *
 * @param floatLocation pointer to the actual float value that shall be
 *            published. This location must NOT change after publishing.
 * @param itemName desired name of the corresponding service (Null terminated)
 * @param defDeadband chosen default deadband of the service
 *
 * @return pointer to the new Item, if enough memory is available, else NULL.
 * @ingroup feesrv_ceapi
 */
FloatItem* fillFloatItem(float* floatLocation, char* itemName, float defDeadband);


/// --- NEW FEATURE SINCE VERSION 0.8.2b [Char Channel] (2007-07-28) --- ///

/**
 * Publishes char items.
 * Function, called by the Control Engine for each char service the
 * FeeServer should offer. This must be done before the server starts serving
 * (call of start()). (is implemented by the FeeServer).
 * NOTE: THIS FUNCTION IS IN ADDITION TO PUBLISH FLOAT / INT VALUES.
 * CharItem is very different to Item and IntItem. Instead of a location
 * pointer, the CharItem owns a function pointer to the corresponding callback
 * routine: <pre>
 *
 * void user_routine(long* tag, int** address, int* size);
 *
 * This function will be called, when an update of the char service is
 * triggered. This function has to be implemented by the CE.
 *
 * - param tag - The parameter that identifies the service, the tag is given
 * in the publishChar() call via the CharItem (Set by CE - member of CharItem).
 *
 * - param address - Should return the address of the data to be sent to the
 * FeeClient.
 *
 * - param size - Should return the size in Bytes of the data to be sent to the
 * FeeClient
 *
 * </pre>
 *
 * @param charItem pointer to the CharItem-struct,  containing the name of the
 *				Service, an identifying tag and a pointer to a callback routine
 *				which is called by the DIM framework, when the corresponding
 *				service shall be updated.
 *
 * @return FEE_OK, if publishing to server was successful, else an error code
 *				possible errors are:
 *				FEE_WRONG_STATE -> FeeServer is not in state running
 *				FEE_NULLPOINTER -> given CharItem contains null pointer,
 *				charItem is not published and not stored in list
 *				(but CE can continue).
 *				FEE_INSUFFICIENT_MEMORY -> insufficient memory on board
 *				FEE_ITEM_NAME_EXISTS -> item name already exists, item is
 *				not published and not stored in list (but CE can continue)
 *				Note: The name must also be unique against the int and float
 *				items.
 * @ingroup feesrv_ceapi
 */
int publishChar(CharItem* charItem);

/**
 * Function creates an empty, but initialized CharItem.
 * All "members" are set to 0. For further use it is essential, that these
 * members are filled with the appropriate values, especially the memory
 * for the '\0'-terminated name has to be still allocated!
 *
 * @return pointer to the new CharItem, if enough memory is available, else NULL
 * @ingroup feesrv_ceapi
 */
CharItem* createCharItem();

/**
 * Function creates an initialized and filled CharItem.
 * All "members" are set according to the input parameters and a pointer to
 * the new created CharItem is returned.
 *
 * @param funcPointer pointer to the callback routine, that shall be called 
 *			by the DIM framework, when the corresponding FeeService is updated.
 *			The callback routine has to be implemented by the CE.
 *			The signature of the callback routine looks like:
 *			void user_routine(long* tag, int** address, int* size);
 *				[tag - The parameter that identifies the service. The tag is 
 *				given to the publishChar() call via the CharItem.]
 *				[address - Should return the address of the (char) data to be
 *				sent to the FeeClient.] 
 *				[size - Should return the size of the data to be sent to the 
 *				FeeClient in bytes.] 
 * @param itemName desired name of the corresponding service (Null terminated)
 * @param tag the above mentioned tag to identify the service in the callback
 *			routine.
 *
 * @return pointer to the new CharItem, if enough memory is available, else NULL
 * @ingroup feesrv_ceapi
 */
CharItem* fillCharItem(void (*funcPointer)(long*, int**, int*), char* itemName,
        long tag);
//CharItem* fillCharItem(void* funcPointer, char* itemName, long tag);


/// -- NEW FEATURE V. 0.9.1 [Set FeeServer Properies in CE] (2007-08-17) -- ///

/**
 * This function allows the CE to set some of the FeeServer properties during
 * the initializing phase. If the new value is in the accepted range, the new
 * value is set. (@see FeeProperty for details on valid ranges)
 * NOTE: The CE is only allowed to change the properties during the call of 
 * initializeCE() - the FeeServer will accept it only in the INITIALIZING state
 *
 * @param prop the FeeProperty, containing information which property shall be
 * 			changed and its new value (@see FeeProperty).
 *
 * @return true if setting was successful, else false (if called in FeeServer
 * 			states other than INITIALIZING, it is always false).
 * @ingroup feesrv_ceapi
 */
bool setFeeProperty(FeeProperty* prop);

#ifdef __cplusplus
}
#endif

#endif

