/************************************************************************
 **
 **
 ** This file is property of and copyright by the Department of Physics
 ** Institute for Physic and Technology, University of Bergen,
 ** Bergen, Norway.
 ** In cooperation with Center for Technology Transfer and 
 ** Telecommunication (ZTT), University of Applied Sciences Worms
 ** Worms, Germany.
 **
 ** This file has been written by Sebastian Bablok,
 ** Sebastian.Bablok@uib.no
 **
 ** Important: This file is provided without any warranty, including
 ** fitness for any particular purpose. Further distribution of this file,
 ** even with changes in the code, is only allowed, when this copyright
 ** and warranty paragraph is kept unchanged and included to the sources. 
 **
 **
 *************************************************************************/

#ifndef FEE_DEFINES_H
#define FEE_DEFINES_H

#include "fee_loglevels.h"

//-----------------------------------------------------------------------------
// Definition of globals for the feeserver, control engine, tests etc.
//
// @date        2003-05-06
// @author      Christian Kofler, Sebastian Bablok
//-----------------------------------------------------------------------------

/**
 * version of the current FeeServer
 * @ingroup feesrv_core
 */
#define FEESERVER_VERSION "0.9.1"

/**
 * state of the FeeServer: collecting service-items
 * @ingroup feesrv_core
 */
#define COLLECTING 1

/**
 * Define for an alias of COLLECTING, which is the same like INITIALIZING.
 * @ingroup feesrv_core
 */
#define INITIALIZING COLLECTING

/**
 * state of the FeeServer: server is running
 * @ingroup feesrv_core
 */
#define RUNNING 2

/**
 * state of FeeServer is running, but CE failed to initialize. Now it is only
 * accepting commands for the FeeServer itself, no monitoring.
 * @ingroup feesrv_core
 */
#define ERROR_STATE 3

/**
 * TAG define, identifying the ACK service.
 * @ingroup feesrv_core
 */
#define ACK_SERVICE_TAG 321

/**
 * Defines the exit value to trigger a normal restart of the FeeServer
 *
 * @ingroup feesrv_core
 */
#define FEE_EXITVAL_RESTART 2

/**
 * Defines the exit value to trigger a restart of the FeeServer to try another
 * approach for initializing the ControlEngine
 *
 * @ingroup feesrv_core
 */
#define FEE_EXITVAL_TRY_INIT_RESTART 4

/**
 * Timeout, the FeeServer is waiting to initialize the CE (in sec).
 * NOTE: This time spread is added to the value of TIMEOUT_INIT_CE_MSEC !
 * This value has to be lower than 4294 to prevent buffer overflows.
 * @ingroup feesrv_core
 */
#define TIMEOUT_INIT_CE_SEC 30

/**
 * Timeout, the FeeServer is waiting to initialize the CE (in ms).
 * NOTE: This time spread is added to the value of TIMEOUT_INIT_CE_SEC !
 * This value has to be lower than 4294 to prevent buffer overflows.
 * Best keep it below 1000 (some machines have problems with larger numbers).
 * @ingroup feesrv_core
 */
#define TIMEOUT_INIT_CE_MSEC 100

/**
 * Timeout; the amount of milliseconds, the FEE-server is waiting for the return
 * of the issue-function (in ms). This value has to be lower than
 * MAX_ISSUE_TIMEOUT (4294967) !
 * @ingroup feesrv_core
 */
#define DEFAULT_ISSUE_TIMEOUT 60000

/**
 * Initial Timeout for the watchdog of replicated log messages. After this time
 * the hold back message will be sent for sure, if no other type of message has
 * triggered its sending before (replicated messages are collected until
 * either this timeout occurs or a different message is triggered. In the later
 * case the hold back message is sent first).
 * The value is given in ms and has to be lower than MAX_TIMEOUT (4294967)
 *
 * @ingroup feesrv_core
 */
#define DEFAULT_LOG_WATCHDOG_TIMEOUT 10000

/**
 * Maximum issue timeout value to prevent buffer overflows.
 * @ingroup feesrv_core
 */
#define MAX_ISSUE_TIMEOUT 4294967

/**
 * Maximum timeout in ms. Can be set to MAX_ISSUE_TIMEOUT.
 * @ingroup feesrv_core
 */
#define MAX_TIMEOUT MAX_ISSUE_TIMEOUT

/**
 * Default update rate for check of Item list in ms.
 * @ingroup feesrv_core
 */
#define DEFAULT_UPDATE_RATE 1000

/**
 * This value multiplied with the deadband checker updateRate and the amount
 * of nodes in the service list defines the time amount, after that each
 * service is at least updated once. This multiplier is used to enlargen the
 * time interval, if needed.
 * @ingroup feesrv_core
 */
#define TIME_INTERVAL_MULTIPLIER 25
//don't use large multipliers (between 1-100 is ok)


/**
 * Default deadband size for monitored values.
 * !!! has to be defined, check it with TPC and TRD !!!
 * @ingroup feesrv_core
 */
//#define DEFAULT_DEADBAND 6.0  // is now set for each item individually

/**
 * The default LogLevel of the FeeServer set on start up (MSG_ALARM has to be
 * set always!).
 * @ingroup feesrv_core
 */
#define DEFAULT_LOGLEVEL (MSG_WARNING + MSG_ERROR + MSG_ALARM + MSG_SUCCESS_AUDIT)
//#define DEFAULT_LOGLEVEL (MSG_WARNING + MSG_ERROR + MSG_ALARM + MSG_INFO + MSG_SUCCESS_AUDIT)

/**
 * This define sets the flag of a FeeProperty to inform about (/ set) a new  
 * update rate.
 * @ingroup feesrv_core
 */
#define PROPERTY_UPDATE_RATE 1

/**
 * This define sets the flag of a FeeProperty to set a new logwatchdog timeout.
 * @ingroup feesrv_core
 */
#define PROPERTY_LOGWATCHDOG_TIMEOUT 2

/**
 * This define sets the flag of a FeeProperty to set a new issue timeout.
 * @ingroup feesrv_core
 */
#define PROPERTY_ISSUE_TIMEOUT 3

/**
 * This define sets the flag of a FeeProperty to set a new loglevel.
 * @ingroup feesrv_core
 */
#define PROPERTY_LOGLEVEL 4


/**
 * FeePacket header size in bytes
 * @ingroup feesrv_core
 */
#define HEADER_SIZE 12

/**
 * The size of the ID - field in the command header. (in bytes)
 * @ingroup feesrv_core
 */
#define HEADER_SIZE_ID 4

/**
 * The size of the ErrorCode - field in the command header. (in bytes)
 * @ingroup feesrv_core
 */
#define HEADER_SIZE_ERROR_CODE 2

/**
 * The size of the Flags - field in the command header. (in bytes)
 * @ingroup feesrv_core
 */
#define HEADER_SIZE_FLAGS 2

/**
 * The size of the Checksum - field in the command header. (in bytes)
 * @ingroup feesrv_core
 */
#define HEADER_SIZE_CHECKSUM 4

/**
 * The offset - size of the ID - field in the command header. (in bytes)
 * @ingroup feesrv_core
 */
#define HEADER_OFFSET_ID 4

/**
 * The offset - size of (ID - field + errorCode - field) in the command header.
 * (in bytes)
 * @ingroup feesrv_core
 */
#define HEADER_OFFSET_ERROR_CODE 6

/**
 * The offset - size of (ID + errorCode + reserved + flags - field) in
 * the command header. (in bytes)
 * @ingroup feesrv_core
 */
#define HEADER_OFFSET_FLAGS 8

/**
 * The offset - size of (ID + errorCode + reserved + flags + checksum - field)
 * in the command header. (in bytes)
 * @ingroup feesrv_core
 */
#define HEADER_OFFSET_CHECKSUM 12

/**
 * Define for the Adler base; largest prime number smaller than 2^16 (65536).
 * Used by the checksum algorithm.
 * @ingroup feesrv_core
 */
#define ADLER_BASE 65521

/**
 * Define for the local detector (TPC, TRD, PHOS, FMD ...)
 * ZTT is the debug and "testing detector"
 * @ingroup feesrv_core
 */
#ifdef TPC
#define LOCAL_DETECTOR "tpc\0"
#else
#ifdef TRD
#define LOCAL_DETECTOR "trd\0"
#else
#ifdef PHOS
#define LOCAL_DETECTOR "pho\0"
#else
#ifdef FMD
#define LOCAL_DETECTOR "fmd\0"
#else
#define LOCAL_DETECTOR "ZTT\0"
#endif
#endif
#endif
#endif

// end of header file
#endif

