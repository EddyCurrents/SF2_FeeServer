// $Id: ce_base.h,v 1.16 2007/09/03 22:57:45 richter Exp $

/************************************************************************
**
**
** This file is property of and copyright by the Experimental Nuclear 
** Physics Group, Dep. of Physics and Technology
** University of Bergen, Norway, 2004
** This file has been written by Matthias Richter,
** Please report bugs to Matthias.Richter@ift.uib.no
**
** Permission to use, copy, modify and distribute this software and its  
** documentation strictly for non-commercial purposes is hereby granted  
** without fee, provided that the above copyright notice appears in all  
** copies and that both the copyright notice and this permission notice  
** appear in the supporting documentation. The authors make no claims    
** about the suitability of this software for any purpose. It is         
** provided "as is" without express or implied warranty.                 
**
*************************************************************************/
#ifndef __CE_BASE_H
#define __CE_BASE_H

#include "fee_types.h"
#include <linux/types.h>   // for the __u32 type

#ifdef __cplusplus
extern "C" {
#endif
/**
 * @defgroup rcu_ce_base Basic functions of the CE
 * Basic methods for the FeeServer ControlEngine, originating from the RCU
 * control engine.
 *
 * This module contains some functionality which is not part of the CE API, 
 * but might be useful for the sake of convenience.
 * - enhanced service registration<br>
 * The publish method of the CE API requires a lot of duplicated code like memory
 * allocation. The @ref rcu_ce_base_services module provides a simple registration
 * scheeme with automatic update and set functionality.
 * - enhanced logging<br>
 * Since the requirements for logging and debugging of the CE goes beyond the
 * features provided by the core a convenient and extended logging/filtering scheeme
 * was developed and is going to be extended.
 * - some global CE properties like disabling service update, relaxed format check of
 * command buffer are collected in the @ref rcu_ce_base_prop module.
 *
 * @author Matthias Richter
 * @ingroup feesrv_ce
 */

/**
 * @defgroup rcu_ce_base_services Basic service handling
 * The module provides two simple functions for service registration: @ref RegisterService
 * and @ref RegisterServiceGroup.
 * The registration is done by a simple call of one of those function. All memory allocation,
 * data structure initialization and the periodical update is carried out by the CE framework.
 * The user has to provide the service (base)name and pointers
 * to an <i>update</i> and (optional) <i>set</i> handler. The <i>update</i> handler ist called
 * periodically from the CE framework whenever update of the service is required.<br>
 *
 * In order to make the two function available <tt>ce_base.h</tt> must be included in the
 * source code.
 * <pre>
 *   #include "ce_base.h"
 * </pre>
 *
 * <b>Note:</b> In order to define terminology. <i>Update</i> is from the point of DIM and the 
 * outside application which subscribes to this services. The data flow is <b>from</b> the 
 * hardware <b>to</b> the Control System. <br>
 * <i>Set</i> is the opposite way, data is set <b>from</b> the Control System 
 * <b>to</b> the hardware. For the latter purpose the CE provides a general command group
 * @ref FEESVR_SET_FERO_DATA.
 * @author Matthias Richter
 * @ingroup rcu_ce_base
 */

/**
 * forward declaration
 */
typedef struct ceServiceDesc_t TceServiceDesc;

/**
 * forward declaration
 */
typedef struct ceServiceData_t TceServiceData;

/**
 * The service data field.
 */
struct ceServiceData_t {
  union {
    /** integer type */
    int iVal;
    /** float type */
    float fVal;
    /** string type, casted to string* before use */
    void* strVal;
  };
};

typedef void (*ceDisServiceCallback)(void* tag, void** buffer, int* size, int* firstTime);

/**
 * Funtion type definition for the update function which has to be implemented for a service. 
 * Several services can use the same function, for that purpose three parameters
 * are intented to differentiate between different services. The update function will be called
 * from the CE framwork whenever the value of this data point has to be updated.
 * @param pData      pointer to a float variable to receive the updated value
 * @param major     index for service identification by the update function
 * @param minor     sub-index for service identification by the update function
 * @param parameter pointer to additional data structure
 * @ingroup rcu_ce_base_services
 */
typedef int (*ceUpdateService)(TceServiceData* pData, int major, int minor, void* parameter);

/**
 * Function type definition for the set function which can be implemented for a service. 
 * The same as for the update function applies for the parameters.
 * The <i>set</i> function is called if the CE received a command of type @ref FEESVR_SET_FERO_DATA.
 * If no <i>set</i> function was registered the command is just ignored.
 * @param data      the value to be set to the hardware data point/register
 * @param major     index for service identification by the update function
 * @param minor     sub-index for service identification by the update function
 * @param parameter pointer to additional data structure
 * @ingroup rcu_ce_base_services
 */
typedef int (*ceSetFeeValue)(TceServiceData* pData, int major, int minor, void* parameter);

/**
 * Data type identifiers for services.
 * Currently, only <i>float</i> is supported.
 * @ingroup rcu_ce_base_services
 */
enum ceServiceDataType{
  /** initializer */
  eDataTypeUnknown = 0,
  /** Integer type*/
  eDataTypeInt,
  /** Float type */
  eDataTypeFloat,
  /** String type */
  eDataTypeString,
};

/**
 * service value for float services without connection to the Data Point.
 * This might happen if Front-end cards are switched off. The value is also
 * used as initializer.
 * @ingroup rcu_ce_base_services
 */
#define CE_FSRV_NOLINK -2000

/**
 * service value for int services without connection to the Data Point.
 * @ingroup rcu_ce_base_services
 */
#define CE_ISRV_NOLINK ~((int)0)

//struct Item;

/**
 * The service descriptor
 * The service descriptor are stored in a single linked list, every item contains
 * the pointer to the next element. The descriptor holds
 * - the @ref Item struct given to the core
 * - the data variable
 * - pointers to <i>update</i> and <i>set</i> functions as well as the parameters
 * which identify this service for those functions  
 * @ingroup rcu_ce_base_services
 */
struct ceServiceDesc_t {
  /** pointer to Item struct passed to the core via publish() */
  Item* pItem;
  /** pointer to next entry */
  TceServiceDesc* pNext;
  /** data type see @ref ceServiceDataType */
  int datatype;
  /** the data field */
  TceServiceData data;
  /** the data backup field */
  TceServiceData databackup;
  /** string type, casted to string* before use */
  void* pName;
  /** id of the dim channel for channels published directly by the CE */
  unsigned int dimid;
  /** pointer to <i>update</i> function */
  ceUpdateService pFctUpdate;
  /** pointer to <i>set</i> function */
  ceSetFeeValue pFctSet;
  /** index */
  int major;
  /** sub-index */
  int minor;
  /** pointer to user defined data */
  void* parameter;
};

/***************************************************************************/

/**
 * @defgroup rcu_ce_base_prop CE properties
 * Properties of the ControlEngine
 * @author Matthias Richter
 * @ingroup rcu_ce_base
 */

/**
 * @name ce option flags
 * @ingroup rcu_ce_base_prop
 */

/**
 * use Msg Buffer interface in single write/read mode
 * @ingroup rcu_ce_base_prop
 */
#define DEBUG_USE_SINGLE_WRITE 0x0001 // use single write and read instead of multiple w/r

/**
 * disable the service update
 * @ingroup rcu_ce_base_prop
 */
#define DEBUG_DISABLE_SRV_UPDT 0x0002 // disable the service update

/**
 * indicates relaxed command buffer format checking
 * @ingroup rcu_ce_base_prop
 */
#define DEBUG_RELAX_CMD_CHECK  0x0004 // relax the end marker and version check of an incoming command

/**
 * indicates whether or not to send log messages through FeeServer message channel.
 * @ingroup rcu_ce_base_prop
 */
#define ENABLE_CE_LOGGING_CHANNEL  0x0008

/**
 * @name API functions for CE properties
 * @ingroup rcu_ce_base_prop
 */

/**
 * set the options
 * @param  options options bitfield
 * @return the options bitfield
 * @ingroup rcu_ce_base_prop
 */
int ceSetOptions(int options);

/**
 * set option flags
 * all other flags remain unaltered
 * @param  of option flag(s)
 * @return the options bitfield
 * @ingroup rcu_ce_base_prop
 */
int ceSetOptionFlag(int of);

/**
 * clear an option flag
 * all other flags remain unaltered
 * @param  of option flag(s)
 * @return the options bitfield
 * @ingroup rcu_ce_base_prop
 */
int ceClearOptionFlag(int of);

/**
 * check an option flag
 * @param  of option flag
 * @return 1 if set, 0 if not
 * @ingroup rcu_ce_base_prop
 */
int ceCheckOptionFlag(int of);

/*******************************************************************************/

/**
 * @name internal functions for service registration
 * @ingroup rcu_ce_base_services
 */

/**
 * cleanup the service descriptors
 * @ingroup rcu_ce_base_services
 */
int ceCleanupServices();

/**
 * Abort the update loop before the next item.
 */
  void ceAbortUpdate();

/**
 * iterate over all services and call the update funtion 
 * @param pattern    pattern for services to update
 * @param bForce     force an update of the corresponding DIM channel
 * @ingroup rcu_ce_base_services
 */
int ceUpdateServices(const char* pattern, int bForce);

/**
 * call the set function for a service 
 * this sets the value for a data point in the FEE
 * @ingroup rcu_ce_base_services
 */
int ceSetValue(const char* name, float value);

/**
 * set the service value of a service (for debugging only)
 * this is only possible if the service update is disabled
 * @ingroup rcu_ce_base_services
 */
int ceWriteService(const char* name, float value);

/*******************************************************************************/

/**
 * @name API methods for service registration
 * @ingroup rcu_ce_base_services
 */

/**
 * Register a service.
 * @param type        type of the channel, see @ref ceServiceDataType
 * @param name        unique name of the service
 * @param defDeadband the default deadband 
 * @param pFctUpdate  pointer to <i>update</i> function
 * @param pFctSet     pointer to <i>set</i> function
 * @param major       optional parameters to be passed to the <i>update</i> and <i>set</i> function 
 * @param minor       optional parameters to be passed to the <i>update</i> and <i>set</i> function 
 * @param parameter   optional parameters to be passed to the <i>update</i> and <i>set</i> function 
 * @ingroup rcu_ce_base_services
 */
int RegisterService(enum ceServiceDataType type, const char* name, float defDeadband, ceUpdateService pFctUpdate, ceSetFeeValue pFctSet, int major, int minor, void* parameter);

/**
 * Register a group of services.
 * The names are built from the basename and the number of services.
 * @param type        type of the channel ("F" or "I")
 * @param basename    base name of the services, the name might contain a '%d' sequence which is then
 *                    replaced by the number, number is appended if no '%d' provided
 * @param count       number of services in this group, passed to the <i>update</i> and <i>set</i> function as parameter major
 * @param defDeadband the default deadband 
 * @param pFctUpdate  pointer to <i>update</i> function
 * @param pFctSet     pointer to <i>set</i> function
 * @param minor       optional parameters to be passed to the <i>update</i> and <i>set</i> function 
 * @param parameter   optional parameters to be passed to the <i>update</i> and <i>set</i> function 
 * @ingroup rcu_ce_base_services
 */
int RegisterServiceGroup(enum ceServiceDataType type, const char* basename, int count, float defDeadband, ceUpdateService pFctUpdate, ceSetFeeValue pFctSet, int minor, void* parameter);

/***************************************************************************/

/**
 * @defgroup rcu_ce_base_logging ControlEngine logging
 * Logging methods for the FeeServer ControlEngine 
 * @author Matthias Richter
 * @ingroup rcu_ce_base
 */

/**
 * @name logging levels of the CE
 * @ingroup rcu_ce_base_logging
 */
enum {
  /** */
  eCELogAll = 0,
  /** */
  eCEDebug,
  /** */
  eCEInfo,
  /** */
  eCEWarning,
  /** */
  eCEError,
  /** */
  eCEFatal
};

/**
 * @name logging macros
 * The macros provide a simple call like the printf function. The origin of the log message
 * will be added automatically. The log message will be send via the FeeServer message
 * channel and printed out to the terminal.
 * @ingroup rcu_ce_base_logging
 */

/**
 * general log message
 * @ingroup rcu_ce_base_logging
 */
#define CE_Log(level, format, ...)   ceLogMessage(__FILE__, __LINE__, level,   format, ##__VA_ARGS__)
/**
 * debug message
 * @ingroup rcu_ce_base_logging
 */
#define CE_Debug(format, ...)   ceLogMessage(__FILE__, __LINE__, eCEDebug,   format, ##__VA_ARGS__)
/**
 * info message
 * @ingroup rcu_ce_base_logging
 */
#define CE_Info(format, ...)    ceLogMessage(__FILE__, __LINE__, eCEInfo,    format, ##__VA_ARGS__)
/**
 * warning
 * @ingroup rcu_ce_base_logging
 */
#define CE_Warning(format, ...) ceLogMessage(__FILE__, __LINE__, eCEWarning, format, ##__VA_ARGS__)
/**
 * error message
 * @ingroup rcu_ce_base_logging
 */
#define CE_Error(format, ...)   ceLogMessage(__FILE__, __LINE__, eCEError,   format, ##__VA_ARGS__)
/**
 * severe error message
 * @ingroup rcu_ce_base_logging
 */
#define CE_Fatal(format, ...)   ceLogMessage(__FILE__, __LINE__, eCEFatal,   format, ##__VA_ARGS__)
/**
 * Send an alarm via the alarm channel of a @ref CEDevice and a warning via the
 * standard @ref rcu_ce_base_logging.
 * <b>Note:</b> The macro can only be used inside member functions of classes derived from 
 * @ref CEDevice.
 * @ingroup rcu_ce_base_logging
 */
#define CE_Alarm(alarm, format, ...) ceLogMessage(__FILE__, __LINE__, eCEWarning, format, ##__VA_ARGS__);SendAlarm(alarm);

/**
 * @name internal logging functions
 * @ingroup rcu_ce_base_logging
 */

/**
 * The logging function used by the macros
 * @ingroup rcu_ce_base_logging
 */
int ceLogMessage(const char* file, int lineNo, int loglevel, const char* format, ...);

/**
 * @name logging API functions
 * @ingroup rcu_ce_base_logging
 */

/**
 * set the logging level of the CE 
 * @ingroup rcu_ce_base_logging
 */
int ceSetLogLevel(int level);

/**
 * set the timestamp for logging
 * @ingroup rcu_ce_base_logging
 */
void ceSetTimestamp(const char* ts);

/*******************************************************************************************/

/**
 * @name Internal helper functions for the 'issue' handling
 * @ingroup rcu_issue
 */

/**
 * mask a value and shift it according to the mask
 * @param val  data word to mask and shift
 * @param mask data mask
 * @return the processed data word
 * @ingroup rcu_issue
 */
int MASK_SHIFT(int val, int mask);

/**
 * check if the provided header represents a FeeServer command or not
 * @param header the header word to check
 * @return 1 yes, 0 no
 * @ingroup rcu_issue
 */
int checkFeeServerCommand(__u32 header);

/**
 * check if the provided header represents a Msg Buffer command or not
 * @param header the header word to check
 * @return 1 yes, 0 no
 * @ingroup rcu_issue
 */
int checkMsgBufferCommand(__u32 header);

/**
 * extract command id from the provided header.
 * @param header
 * @return 0 - 15 command id, -1 no FeeServer command
 * @ingroup rcu_issue
 */
int extractCmdId(__u32 header);

/* extract command sub id from the provided header
 * @param header
 * @return 0 - 255 command id, -1 no FeeServer command
 * @ingroup rcu_issue
 */
int extractCmdSubId(__u32 header);

#ifdef __cplusplus
}
#endif
#endif //__CE_BASE_H
