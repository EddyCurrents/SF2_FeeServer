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

#ifndef FEE_TYPES_H
#define FEE_TYPES_H

#include <stdbool.h>

#include "fee_loglevels.h"
//-----------------------------------------------------------------------------
// Definition of datatypes for the feeserver, control engine, tests etc.
//
// @date	2003-05-06
// @author	Christian Kofler, Sebastian Bablok
//-----------------------------------------------------------------------------


/**
 * Typedef Item.
 * Item is a struct, with a pointer to the item value (volatile double* location)
 * and the item name (char* name). This represents the core of an item. In
 * addition a default dead band is provided.
 * @ingroup feesrv_core
 */
typedef struct {
	/** struct value location -> pointer to the actual value of the item.*/
	volatile float* location;
	/** struct value name -> name of item.*/
	char* name;
	/** struct value defaultDeadband -> default deadband of this item */
	float defaultDeadband;
} Item; /**< The item which represents a service. */


/**
 * Typedef FeeProperty.
 * This struct is used to inform the CE about a property change in the
 * FeeServer. It provides a flag, identifying, which kind of property and the
 * property value itself. 
 * For now, only the update rate of the deadband check
 * is forseen to be signaled (old version before 0.9.1).
 * NEW with version 0.9.1:
 * unsigned int and unsigned long property values included. They will be used
 * for allowing the CE to change: 
 * <pre>
 * - logWatchDogTimeout (timeout for collecting duplicated log messages)
 * - updateRate (update rate, which is used to check for value change, s.a.)
 * - issueTimeout (sets the time which the CE has to remain in the issue call)
 * - logLevel (allows the CE to overwrite the default loglevel during init)
 * </pre>
 *
 * @ingroup feesrv_core
 */
typedef struct {
	/** 
	 * struct value flag -> the flag identifying the property.
	 * Possible flags are (defined in fee_defines.h):
	 * <pre>
	 * - PROPERTY_UPDATE_RATE 1 
	 *   -> corresponding value: uShortVal, valid range: >0;
	 *
	 * - PROPERTY_LOGWATCHDOG_TIMEOUT 2
	 *   -> corresponding value: uIntVal, valid range: >0;
	 *
	 * - PROPERTY_ISSUE_TIMEOUT 3
	 *   -> corresponding value: uLongVal, valid range: >0;
	 *
	 * - PROPERTY_LOGLEVEL 4 (@see fee_loglevels.h for details)
	 *   -> corresponding value: uIntVal, valid range: >0 and <= MSG_MAX_VAL;
	 * </pre>
	 */
	unsigned short flag;
	/**
	* struct value uShortVal -> stores the value of an unsigned short
	* property.
	*/
	unsigned short uShortVal;

/// --- NEW Member since version 0.9.1 [2007-08-17] --- ///
	/**
	 * struct value uIntVal -> stores the value of an unsigned integer
	 * property.
	 */
	unsigned int uIntVal;
	/**
	 * struct value uLongVal -> stores the value of an unsigned long
	 * property.
	 */
	unsigned long uLongVal;

} FeeProperty; /**< The FeeProperty, to signal a value change.*/


/**
 * Typedef ItemNode.
 * ItemNode is a struct for building a doubly linked list of Items and
 * corresponding information.
 * @ingroup feesrv_core
 */
typedef struct Node{
	/** struct value prev -> pointer to previous ItemNode.*/
	struct Node* prev;
	/** struct value next -> pointer to next ItemNode.*/
	struct Node* next;
	/** struct value id -> DIM-ServiceID (used by the DIM-framework).*/
	unsigned int id;
	/** struct value item -> pointer to the Item.*/
	Item* item;

	/**
	 * struct value lastTransmittedValue -> contains the last sent value of this
	 * node's item.
	 */
	float lastTransmittedValue;
	/**
	 * struct value threshold -> contains the deadband size for this item
	 * divided by 2. The dead band is around the lastTransmittedValue, the
	 * threshold can be used, because it is equal to both sides.
	 */
	float threshold;

	/**
	 * This is a backup of the location (address) for the item of this node.
	 */
	volatile float* locBackup;
	 /**
	  * Checksum of the location, to detect bit flips in "location".
	  */
	unsigned int checksum;
	/**
	 * Checksum backup to be able to verify original checksum.
	 */
	unsigned int checksumBackup;

	//AlarmCond*
} ItemNode; /**< ItemNode is a node of the local doubly linked list. */


/**
 * Typedef IssueStruct.
 * IssueStruct contains pointer to the datatypes used by the issue-function.
 * This struct is mainly used for data-exchange between command handler and
 * issue-thread.
 *
 * @see issue()
 * @ingroup feesrv_core
 */
typedef struct {
	/**
	 * struct value nRet -> contains the return value of the issue-function.
	 * (FEE_OK or error code).
	 */
	int nRet;
	/** struct value command -> pointer to the command-data (see issue-function). */
	char* command;
	/** struct value result -> pointer to the result-data (see issue-function). */
	char* result;
	/**
	 * struct value size -> the size of command-data for input and the size of the
	 * result-data for ouput (see issue-function).
	 */
	int size;
} IssueStruct;

/**
 * Typedef MessageStruct.
 * MessageStruct contains the data fields provided by the message service.
 *
 * @see sendMessage()
 * @ingroup feesrv_core
 */
typedef struct {
	/**
	 * struct value eventType -> contains the (log) level type of the current
	 * message. (info, warning, error , ...)
	 *
	 * @see fee_loglevels.h for more details.
	 */
	unsigned int eventType;
	/** struct value detector -> the detector, which triggered this message. */
	char detector[MSG_DETECTOR_SIZE];
	/**
	 * struct value source -> the source (area, eg: FeeServer or CE), where the
	 * event occured.
	 */
	char source[MSG_SOURCE_SIZE];
	/** struct value description -> the actual message of the event. */
	char description[MSG_DESCRIPTION_SIZE];
	/** struct value date -> the date, when the event occured. */
	char date[MSG_DATE_SIZE];
} MessageStruct;

/**
 * Typedef FlagBits
 * 2 byte field for flag bits in FeePacket header.
 * @ingroup feesrv_core
 */
typedef unsigned short FlagBits;

/**
 * Typedef CommandHeader.
 * CommandHeader contains all elements of a FeePacket header as members of the
 * struct.
 * @ingroup feesrv_core
 */
typedef struct {
	/** identification number of the command (used in the FeePacket header) */
	unsigned int id;
	/** the error code - see fee_errors.h for details! */
	short errorCode;
	/**
	 * The flags of the command, such as "HuffmannEncoded" and "CheckSum" and
	 * all other commands for the FeeServer itself.
	 */
	FlagBits flags;
	/** the checksum of the command */
	int checksum;
} CommandHeader;


////   ---- NEW FEATURE SINCE VERSION 0.8.1 (2007-06-12) ----- /////

/**
 * Typedef IntItem.
 * IntItem is a struct, with a pointer to the integer item value
 * (volatile int* location)
 * and the IntItem name (char* name). This represents the core of an IntItem.
 * In addition a default dead band is provided.
 * @ingroup feesrv_core
 */
typedef struct {
	/** struct value location -> pointer to the actual value of the item.*/
	volatile int* location;
	/** struct value name -> name of item.*/
	char* name;
	/** struct value defaultDeadband -> default deadband of this item */
	int defaultDeadband;
} IntItem; /**< The item which represents a service. */

/**
 * Typedef IntItemNode.
 * IntItemNode is a struct for building a doubly linked list of IntItems and
 * corresponding information.
 * @ingroup feesrv_core
 */
typedef struct IntNode {
	/** struct value prev -> pointer to previous ItemNode.*/
	struct IntNode* prev;
	/** struct value next -> pointer to next ItemNode.*/
	struct IntNode* next;
	/** struct value id -> DIM-ServiceID (used by the DIM-framework).*/
	unsigned int id;
	/** struct value intItem -> pointer to the IntItem.*/
	IntItem* intItem;

	/**
	 * struct value lastTransmittedIntValue -> contains the last sent value of this
	 * node's IntItem.
	 */
	int lastTransmittedIntValue;
	/**
	 * struct value threshold -> contains the deadband size for this IntItem
	 * divided by 2. The dead band is around the lastTransmittedIntValue, the
	 * threshold can be used, because it is equal to both sides.
	 */
	float threshold;

	/**
	 * This is a backup of the location (address) for the IntItem of this node.
	 */
	volatile int* locBackup;
	 /**
	  * Checksum of the location, to detect bit flips in "location".
	  */
	unsigned int checksum;
	/**
	 * Checksum backup to be able to verify original checksum.
	 */
	unsigned int checksumBackup;

	//AlarmCond*
} IntItemNode; /**< IntItemNode is a node of the local doubly linked list. */

// Wrapper for FloatItems

/**
 * Typedef of a FloatItem to Item. Used as Wrapper.
 */
typedef Item FloatItem;


// ------ new memory management -------------------- //

/**
 * Typedef for MemoryMetaData
 * The MemoryMetaData describes the features of an allocated memory block like
 * size, type, calling module, ... .
 * @ingroup feesrv_core
 */
typedef struct {
	/** Size of the allocated memory */
	unsigned int memSize;
	/**
	 * Type of the memory  (used for int = 'i', short = 's', long = 'l',
	 * float = 'f', double = 'd', char = 'c', bool = 'b', string = 's',
	 * complex = 'c'). The unsinged versions are represented by the corresponding
	 * capital letter.
	 */
	char memType;
	/** The module acquiring te memory block. */
	char memDest[30];
	/**
	 * Indicates if the memory is prefixed by some memory, e.g. used for some
	 * header, etc. (false = no prefix)
	 */
	bool prefixed;
	/** size of the prefix memory, no prefix => size = 0. */
	unsigned int prefixSize;

} MemoryMetaData;

/**
 * Typedef for MemoryNode
 * These memory nodes are collected in a doubly link list and contain pointer
 * to allocated memory and describing meta data (MemoryMetaData).
 * @ingroup feesrv_core
 */
typedef struct MemNode {
	/** Pointer to the previous MemoryItem */
	struct MemNode* prev;
	/** Pointer to he next MemoryItem */
	struct MemNode* next;
	/**
	 * Base pointer to the allocated memory.
	 * This can differ from the identifying address, in cases, where the
	 * identifying address is prefixed by some memory for header, etc.
	 */
	void* ptr;
	/** Identifying address of the MemoryItem. */
	void* identityAddr;
	/** MemoryMetaData object describing the allocated memory (size, type, ...) */
	MemoryMetaData mmData;

} MemoryNode;


///   --- NEW FEATURE SINCE VERSION 0.8.2b [Char Channel] (2007-07-28) --- ///

/**
 * Typedef CharItem.
 * CharItem is a struct, with a function pointer to a callback routine, that
 * provides the data for the corresponding FeeService; the FeeService name,
 * and a tag which identifies the FeeService.
 * @ingroup feesrv_core
 */
typedef struct {
	/**
	 *  struct function pointer -> user_routine for callback in case of
	 * FeeService update. Called by the DIM framework.
	 */
	void (*user_routine)(long* tag, int** address, int* size);
	/** struct value name -> name of char item (service name). */
	char* name;
	/**
	 * struct value tag -> identifies the FeeService; handed to DIM when
	 * registering the service and return by the framework as parameter of
	 * the callback routine.
	 */
	long tag;
} CharItem;


/**
 * Typedef CharItemNode.
 * CharItemNode is a struct for building a doubly linked list of CharItems and
 * corresponding information.
 * @ingroup feesrv_core
 */
typedef struct CharNode {
	/** struct value prev -> pointer to previous CharNode.*/
	struct CharNode* prev;
	/** struct value next -> pointer to next CharNode.*/
	struct CharNode* next;
	/** struct value id -> DIM-ServiceID (used by the DIM-framework).*/
	unsigned int id;
	/** struct value charItem -> pointer to the CharItem.*/
	CharItem* charItem;

} CharItemNode; /**< CharItemNode is a node of the local doubly linked list. */



#endif

