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

#ifndef FEE_LOGLEVELS_H
#define FEE_LOGLEVELS_H

/**
 * @defgroup feesrv_logging Logging and logging levels
 * The logging levels of the FeeServer
 * <b>Note:</b>The ControlEngine might use other/additional logging filters
 * @see feesrv_ce for more information.
 *
 * @ingroup feesrv_core
 */


/**
 * Define for the event - type "INFO" .
 * @ingroup feesrv_logging
 */
#define MSG_INFO    1

/**
 * Define for the event - type "WARNING" .
 * @ingroup feesrv_logging
 */
#define MSG_WARNING 2

/**
 * Define for the event - type "ERROR" .
 * @ingroup feesrv_logging
 */
#define MSG_ERROR   4

/**
 * Define for the event - type "FAILURE_AUDIT" (currently not used).
 * @ingroup feesrv_logging
 */
#define MSG_FAILURE_AUDIT 8

/**
 * Define for the event - type "SUCCESS_AUDIT" (currently not used).
 * @ingroup feesrv_logging
 */
#define MSG_SUCCESS_AUDIT 16

/**
 * Define for the event - type "DEBUG" .
 * @ingroup feesrv_logging
 */
#define MSG_DEBUG   32

/**
 * Define for the event - type "ALARM" .
 * @ingroup feesrv_logging
 */
#define MSG_ALARM   64

/**
 * Defines the maximum value of cumulated event - types.
 * @ingroup feesrv_logging
 */
#define MSG_MAX_VAL 127

/**
 * Size of the detector field in messages (in byte).
 * @ingroup feesrv_logging
 */
#define MSG_DETECTOR_SIZE       4

/**
 * Size of the source field in messages (in byte).
 * @ingroup feesrv_logging
 */
#define MSG_SOURCE_SIZE         256

/**
 * Size of the description field in messages (in byte).
 * @ingroup feesrv_logging
 */
#define MSG_DESCRIPTION_SIZE    256

/**
 * Size of the date field in messages (in byte), format: YYYY-MM-DD hh:mm:ss .
 * @ingroup feesrv_logging
 */
#define MSG_DATE_SIZE           20

#endif
