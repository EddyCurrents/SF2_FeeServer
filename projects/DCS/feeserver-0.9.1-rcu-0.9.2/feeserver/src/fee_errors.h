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

#ifndef FEE_ERRORS_H
#define FEE_ERRORS_H

/**
 * @defgroup errval Error codes
 * Here are some error codes and condition defines used in the feeserver.
 * There are different areas for every party on this project: <pre>
 * -1		to	-19	belongs to FEE-Communications (ZTT) [starts with FEE_...]
 * -20		to	-39	belongs to TPC [starts with TPC_...]
 * ...
 * -60      to  -79 belongs to InterCom - Layer [starts with INTERCOM_...]
 *
 * RESERVED IS:
 *  -99	for FEE_UNKNOWN_RETVAL
 *    0	for FEE_OK
 *  99		for FEE_MAX_RETVAL
 * <pre>
 *
 * The common used rule is: <pre>
 * value <  0	means error code or failure condition
 * value == 0	means execution was successful
 * value > 0 	means some addtional information is provided
 * </pre>
 *
 * @author Christian Kofler, Sebastian Bablok
 * @ingroup feesrv_core
 */

/**
 * Unexpected return value encountered
 * @ingroup errval
 */
#define FEE_UNKNOWN_RETVAL -99

/**
 * Define of return val for published item name already exists
 * @ingroup errval
 */
#define FEE_ITEM_NAME_EXISTS -11

/**
 * Define for error concering Threads (-initialization)
 * @ingroup errval
 */
#define FEE_THREAD_ERROR -10

/**
 * Define for insufficient memory -> error
 * @ingroup errval
 */
#define FEE_INSUFFICIENT_MEMORY -9

/**
 * Monitoring thread initialising failed.
 * @ingroup errval
 */
#define FEE_MONITORING_FAILED -8

/**
 * Command data corrupted, wrong checksum.
 * @ingroup errval
 */
#define FEE_CHECKSUM_FAILED -7

/**
 * Initialisation of CE failed
 * @ingroup errval
 */
#define FEE_CE_NOTINIT -6

/**
 * Execution simply failed
 * @ingroup errval
 */
#define FEE_FAILED -5

/**
 * Operation was called in wrong state
 * @ingroup errval
 */
#define FEE_WRONG_STATE -4

/**
 * One of incoming parameters is invalid
 * @ingroup errval
 */
#define FEE_INVALID_PARAM -3

/**
 * Nullpointer exception encountered
 * @ingroup errval
 */
#define FEE_NULLPOINTER -2

/**
 * A timeout occured
 * @ingroup errval
 */
#define FEE_TIMEOUT -1

/**
 * Execution was successful, no error
 * @ingroup errval
 */
#define FEE_OK 0

/**
 * Maximum value of Feeserver return value. (only two letters are allowed)
 * @ingroup errval
 */
#define FEE_MAX_RETVAL 99

/**
 * Define for CE completely and successfully initalized
 * @ingroup errval
 */
#define CE_OK FEE_OK

/**
 * Define for CE has not sufficient memory to complete init procedure
 * @ingroup errval
 */
#define CE_INSUFFICIENT_MEMORY FEE_INSUFFICIENT_MEMORY


#endif


