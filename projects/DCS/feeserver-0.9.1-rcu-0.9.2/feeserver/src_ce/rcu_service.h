// $Id: rcu_service.h,v 1.15 2007/06/08 14:42:10 richter Exp $

/************************************************************************
**
**
** This file is property of and copyright by the Experimental Nuclear 
** Physics Group, Dep. of Physics and Technology
** University of Bergen, Norway, 2004
** This file has been written by Matthias Richter and Dag T Larsen
** Please report bugs to Matthias.Richter@ift.uib.no, dagtl@ift.uib.no
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

#ifndef __RCU_SERVICE_H
#define __RCU_SERVICE_H

#include <linux/types.h>   // for the __u32 type

struct ceServiceData_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup rcu_service Service handling for the RCU FeeServer
 * This group describes service handling methods of the RCU FeeServer.
 * Standard services for the RCU and the attached FECs are defined in rcu_service.h,   
 * implemented in rcu_service.c. <br>
 * The RCU provides access to the attached Front-end cards and the control engine publishes
 * the corresponding services. The access is carried out by the Monitoring and Safety Module
 * of the RCU firmware.<br>
 *
 * Services:
 * <pre>
 * The standard services per FEC are:
 * =============================================================================================
 *
 * def.conf    name      description                    deadband  bitwidth conversion  unit
 * ---------------------------------------------------------------------------------------------
 *   off     "T_TH"    temperature threshold              0.5        10      0.25       oC
 *   off     "AV_TH"   analog voltage threshold           0.5        10      0.0043     V
 *   off     "AC_TH"   analog current threshold           0.5        10      0.017      A
 *   off     "DV_TH"   digital voltage threshold          0.5        10      0.0043     V
 *   off     "DC_TH"   digital current threshold          0.5        10      0.03       A
 *   on      "TEMP"    temperature                        0.5        10      0.25       oC
 *   on      "AV"      analog voltage                     0.5        10      0.0043     V
 *   on      "AC"      analog current                     0.5        10      0.017      A
 *   on      "DV"      digital voltage                    0.5        10      0.0043     V
 *   on      "DC"      digital current                    0.5        10      0.03       A
 *   off     "L1CNT"   level 1 trigger count              0.5        16      1.0
 *   off     "L2CNT"   level 2 trigger count              0.5        16      1.0
 *   off     "SCLKCNT"                                    0.5        16      1.0
 *   off     "DSTBCNT"                                    0.5         8      1.0
 *   off     "TSMWORD"                                    0.5         9      1.0
 *   off     "USRATIO"                                    0.5        16      1.0
 *   off     "CSR0"    BC control & status register 0     0.5        11      1.0
 *   off     "CSR1"    BC control & status register 1     0.5        14      1.0
 *   off     "CSR2"    BC control & status register 2     0.5        16      1.0
 *   off     "CSR3"    BC control & status register 3     0.5        16      1.0
 * </pre>
 *
 * A service with state 'no link' (FEC inactive, no access) is set to -2000.<br>
 * 
 * The name of the service is derived from the number of the FEC and the service base name,
 * like 05_TEMP 
 *
 * @author Matthias Richter
 * @ingroup feesrv_ce
*/

/**
 * @defgroup rcuce_service_internal Internal defines and methods of the RCU CE service handling
 * Internal defines and methods of the RCU CE service handling.
 * @internal
 * @ingroup rcu_service
 */

/**
 * the maximum length of a service name.
 * The define denotes the default length of service names. Used inside rcu_service, there is
 * no restriction within the @ref rcu_ce_base_services module.
 * @internal
 * @ingroup rcuce_service_internal
 */
#  define MAX_SERVICE_NAME_LEN 15

/**
 * @name Branch layout
 * @internal
 * @ingroup rcuce_service_internal
 */

/**
 * An RCU can serve at max 32 FECs grouped into two branches
 * @internal
 * @ingroup rcuce_service_internal
 */
#  define NOF_BRANCHES     2
/**
 * The maximum number of FECs in branch A. The actual number is defined by 
 * @ref NOF_FEC_BRANCH_A in the detector specific CE header file. 
 * @see ce_tpc.h, ce_phos.h
 * @internal
 * @ingroup rcuce_service_internal
 */
#  define MAX_BRANCH_A_FEC 16
/**
 * The maximum number of FECs in branch B. The actual number is defined by 
 * @ref NOF_FEC_BRANCH_B in the detector specific CE header file. 
 * @see ce_tpc.h, ce_phos.h
 * @internal
 * @ingroup rcuce_service_internal
 */
#  define MAX_BRANCH_B_FEC 16
/**
 * The maximum total number of FECs.
 * @internal
 * @ingroup rcuce_service_internal
 */
#  define MAX_NOF_FEC      MAX_BRANCH_A_FEC+MAX_BRANCH_B_FEC

/**
 * The default number of FECs.
 * The number of Front-end cards is set by the detector specific CE with the defines
 * - @ref NOF_FEC_BRANCH_A
 * - @ref NOF_FEC_BRANCH_B
 *
 * @see ce_tpc.h, ce_phos.h, ... for the values
 *
 * If none of the above defines is set the default value of @ref NOF_FEC_DEFAULT is taken.<br>
 * @note Always make sure that the include file for the detector is placed before this file, otherwise 
 * you get a conflict at the customized detector definition. If no detector specific CE is enabled,
 * the fall back is zero FECs.
 * @internal
 * @ingroup rcuce_service_internal
 *
*/
#  define NOF_FEC_DEFAULT  0

/**
 * @name Helper macros to convert between local (within a branch) and global addressing of a FEC.
 * @internal
 * @ingroup rcuce_service_internal
 */

/**
 * Convert Branch A FEC address to global address
 * @internal
 * @ingroup rcuce_service_internal
 */
#  define FEC_BRANCH_A_TO_GLOBAL(x) x
/**
 * Convert Branch B FEC address to global address
 * @internal
 * @ingroup rcuce_service_internal
 */
#  define FEC_BRANCH_B_TO_GLOBAL(x) MAX_BRANCH_A_FEC + x
/**
 * Convert global address to Branch A FEC address
 * returns -1 if the global address is outside branch A
 * @internal
 * @ingroup rcuce_service_internal
 */
#  define FEC_GLOBAL_TO_BRANCH_A(x) (x>=0 && x<MAX_BRANCH_A_FEC)?x:-1
/**
 * Convert global address to Branch B FEC address
 * returns -1 if the global address is outside branch B
 * @internal
 * @ingroup rcuce_service_internal
 */
#  define FEC_GLOBAL_TO_BRANCH_B(x) (x>=MAX_BRANCH_A_FEC && x<MAX_NOF_FEC)?x-MAX_BRANCH_A_FEC:-1

/*   
   The actual configuration of services and FECs is defined by a couple of environment variables:
   FEESERVER_FEC_MONITOR   determines the valid FECs, its a string of '0' and '1', missing entries
                           are set to 0, e.g. '01001' puts FEC 2 and 5 into the configuration
   FEESERVER_FECSRV_ENABLE override the default FEC service configuration, comma separated list of names 
   FEESERVER_FECSRV_ADDEN  add FEC services to the default service configuration, comma separated list of names 
   FEESERVER_SERVICE_ADR   list of addresses in the RCU going to be published as services
   FEESERVER_SERVICE_NAME  list of the service names for the above RCU locations
   FEESERVER_SERVICE_DEAD  list of the default deadbands for the above RCU locations

   States:
   =============================================================================================
   The RCU publishes 1 channel for its own state and a state channel for each FEC
   The states are defined in rcu_state.h

   RCU_STATE name of the RCU channel
   xx_STATE  name of the FEC #xx channel
 */


/******************************************************************************************/

/**
 * @name Service publishing
 * @ingroup rcu_service
 */

/**
 * publish additional registers for the RCU itself (RCU memory locations).
 * the registers are defined by the three environment variables 
 * FEESERVER_SERVICE_ADR  list of addresses
 * FEESERVER_SERVICE_NAME list of the service names
 * FEESERVER_SERVICE_DEAD list of the default deadbands
 * the function makeRegItemLists() builds the internal arrays
 * @ingroup rcu_service
 */
int publishRegs();

/**
 * read the environment variables and build the internal arrays for the RCU services.
 * not yet implemented
 * @ingroup rcu_service
 */
int makeRegItemLists();

/* publish a certain register
 */
int publishReg(int regNum);



/******************************************************************************************/

/**
 * @name Service update
 * @ingroup rcu_service
 */

/**
 * read the AFL (active FEC list) from the RCU and set the corresponding flags
 * this function is called at the begin of each update cycle 
 * @ingroup rcu_service
 */

/**
 * the update function for standard RCU memory locations
 * this function is registered for the RCU services and is called each update loop
 * @param pF        pointer to the float variable to receive the updated data
 * @param regNo     index of the register
 * @param dummy     don't care, required by API but not used
 * @param parameter don't care, optional parameter required by API but not used
 * @ingroup rcu_service
 */
int updateReg(ceServiceData_t* pData, int regNum, int dummy, void* parameter);

/******************************************************************************************/

/**
 * @name Internal methods
 * @internal
 * @ingroup rcuce_service_internal
 */

/**
 * 
 * @internal
 * @ingroup rcuce_service_internal
 */
int readExtEnv();

/**
 * 
 * @internal
 * @ingroup rcuce_service_internal
 */
__u32* cnvAllExtEnvI(char** values);

/**
 * 
 * @internal
 * @ingroup rcuce_service_internal
 */
float* cnvAllExtEnvF(char** values);

/**
 * 
 * @internal
 * @ingroup rcuce_service_internal
 */
//__u32 cnvExtEnv(char* var);

/**
 * 
 * @internal
 * @ingroup rcuce_service_internal
 */
char** splitString(char* string);

#ifdef __cplusplus
}
#endif

#endif //__RCU_SERVICE_H
