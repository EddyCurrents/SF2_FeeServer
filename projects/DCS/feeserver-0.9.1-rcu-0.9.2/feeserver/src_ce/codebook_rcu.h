// $Id: codebook_rcu.h,v 1.19 2007/09/03 07:32:16 richter Exp $

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

#ifndef __CODEBOOK_RCU_H
#define __CODEBOOK_RCU_H
/* the memory locations on the rcu card
 */

// lsbs of memory locations with a smaller bit width than the system are ignored 
#define ALTROResultMEM        0x6000    // 20bit x 128
#define ALTROResultMEM_WIDTH  20
#define ALTROResultMEM_SIZE   128

#define ALTROACL              0x6400    // 16bit x 256
#define ALTROACL_WIDTH        16
#define ALTROACL_SIZE         256

#define ALTROPatternMEM       0x6800    // 10bit x 1024
#define ALTROPatternMEM_WIDTH 10
#define ALTROPatternMEM_SIZE  1024

#define ALTROInstMEM          0x7000    // 23bit x 256
#define ALTROInstMEM_WIDTH    23
#define ALTROInstMEM_SIZE     256

#define DataHEADER            0x4001    // 32bit x 6
#define DataHEADER_WIDTH      32
#define DataHEADER_SIZE       6

/* new define further down */
/* #define ALTROStatusREG        0x7800    // {16'h(fec_acl),12'h0,4'h(status)}	 */
/* #define ALTROStatusREG_WIDTH  32 */
/* #define ALTROStatusREG_SIZE   1 */

/* these registers never got used (SC design of 2004) */ 
/* #define SCResultMEM           0x8000    // 16bit x 16 */
/* #define SCResultMEM_WIDTH     16 */
/* #define SCResultMEM_SIZE      16 */

/* #define SCInstMEM             0x9000    // 11bit x 256 */
/* #define SCInstMEM_WIDTH       11 */
/* #define SCInstMEM_SIZE        256 */

/* #define DataAssemblerDataMEM        0x4000    // 32bit x 1024 */
/* #define DataAssemblerDataMEM_WIDTH  32 */
/* #define DataAssemblerDataMEM_SIZE   1024 */

#define StatusReg             0xA000    // {busySC,errorSC,busyAM,errorAM}	
#define StatusReg_WIDTH       32
#define StatusReg_SIZE        1

#define FECActiveList         0x8000
#define FECActiveList_WIDTH   32
#define FECActiveList_SIZE    1

#define FECRDOList            0x8001
#define FECRDOList_WIDTH      32
#define FECRDOList_SIZE       1

#define FECResultREG          0x8002
#define FECResultREG_WIDTH    21
#define FECResultREG_SIZE     1

#define FECErrReg             0x8003
#define FECErrReg_WIDTH       2
#define FECErrReg_SIZE        1

#define FECINTmode            0x8004
#define FECINTmode_WIDTH      2
#define FECINTmode_SIZE       1

#define RCUFwVersion          0x8008
#define RCUFwVersion_WIDTH    24
#define RCUFwVersion_SIZE     1

/* a few FW version from May 06 use 0x8006 as FW version register */
#define RCUFwVersionOld       0x8006
#define RCUFwVersionOld_WIDTH 24
#define RCUFwVersionOld_SIZE  1

#define FECStatusMemory       0x8100
#define FECStatusMemory_WIDTH 16
#define FECStatusMemory_Size  32

#define FECCommands           0xC000
#define FECCommands_WIDTH     32
#define FECCommands_SIZE      4096

#ifdef  ENABLE_ANCIENT_05
#define FECResetErrReg        0x8012
#else   // !ENABLE_ANCIENT_05
#define FECResetErrReg        0x8010
#endif  // ENABLE_ANCIENT_05
#define FECResetErrReg_WIDTH  1
#define FECResetErrReg_SIZE   1

/** Altro ERRST register */
#define AltroErrSt            0x7800
/** bit width of Altro ERRST register */
#define AltroErrSt_WIDTH      32
/** size of Altro ERRST register block */
#define AltroErrSt_SIZE       1 
/** error in pattern memory comparission */
#define PATTERN_MISSMATCH     0x01
/** the rcu sequencer was aborted */
#define SEQ_ABORT             0x02
/** timeout during FEC access */
#define FEC_TIMEOUT           0x04
/** error bit of the Altro bus set during operation */
#define ALTRO_ERROR           0x08
/** channel address missmatch */
#define ALTRO_HWADD_ERROR     0x10
/** 
 * RCU Hardware address mask
 * the RCU Hardware address bits are RW and encode sector [8:3] and partition [2:0]
 */
#define RCU_HWADDR_MASK       0x001ff000
/** RCU Hardware address partition mask [2:0] */
#define RCU_HWADDR_PART_MASK  0x00007000
/** RCU Hardware address partition bit width */
#define RCU_HWADDR_PART_WIDTH 3
/** RCU Hardware address sector mask [8:3] */
#define RCU_HWADDR_SEC_MASK   0x001f8000
/** RCU Hardware address sector bit width */
#define RCU_HWADDR_SEC_WIDTH  6
/** RCU Hardware address width */
#define RCU_HWADDR_WIDTH      RCU_HWADDR_SEC_WIDTH+RCU_HWADDR_PART_WIDTH
/** the sequencer is busy */
#define RCU_SEQ_BUSY          0x80000000

#define AltroTrCfg            0x7801
#define AltroTrCfg_WIDTH      32
#define AltroTrCfg_SIZE       1

#define AltroTrCnt            0x7802
#define AltroTrCnt_WIDTH      32
#define AltroTrCnt_SIZE       1

#define AltroLwAdd            0x7803
#define AltroLwAdd_WIDTH      18
#define AltroLwAdd_SIZE       1

#define AltroIrAdd            0x7804
#define AltroIrAdd_WIDTH      20
#define AltroIrAdd_SIZE       1

#define AltroIrDat            0x7805
#define AltroIrDat_WIDTH      20
#define AltroIrDat_SIZE       1

#define AltroPmCfg            0x7806
#define AltroPmCfg_WIDTH      20
#define AltroPmCfg_SIZE       1

#define AltroChAdd            0x7807
#define AltroChAdd_WIDTH      20
#define AltroChadd_SIZE       1

#define FECCSR0               0x11
#define FECCSR1               0x12
#define FECCSR2               0x13
#define FECCRR3               0x14

// the following commands are executed if there is an access to the specified address
#define CMDExecALTRO          0x0000    // 1bit x 1
#define CMDAbortALTRO         0x0800    // 1bit x 1
#define CMDExecSC             0x1000    // 1bit x 1
#define CMDAbortSC            0x1800    // 1bit x 1
#define CMDRESET              0x2000    // 1bit x 1
#define CMDRESETFEC           0x2001    // 1bit x 1
#define CMDRESETRCU           0x2002    // 1bit x 1
#define CMDResAltroErrSt      0x6c01    // 1bit x 1
#define CMDDCS_ON             0xE000    // 1bit x 1
#define CMDSIU_ON             0xF000    // 1bit x 1
#define CMDL1TTC              0xE800    // 1bit x 1
#define CMDL1I2C              0xF800    // 1bit x 1
#define CMDL1CMD              0xD800    // 1bit x 1
#define CMDL1                 0xD000    // 1bit x 1

// Defines for Bitshift commands to address BC registers
// for the 5 bit version of the slow control module
#define MSMCommand_base		12
#define MSMCommand_rnw		11
#define MSMCommand_bcast	10
#define MSMCommand_branch	9
#define MSMCommand_FECAdr	5
#define MSMCommand_BCRegAdr	0
		
		
//Defines for the Actel Device

/** Actel Command Register **/
#define ACTELCommandReg         0xB000
#define ACTELCommandReg_SIZE    1
#define ACTELCommandReg_WIDTH   32

/** Actel Selectmap Command Register **/
#define ACTELSmCommandReg       0xB002
#define ACTELSmCommandReg_SIZE  1
#define ACTELSmCommandReg_WIDTH 16

/** Actel Status Register **/
#define ACTELStatusReg          0xB100    
#define ACTELStatusReg_SIZE     1
#define ACTELStatusReg_WIDTH    12

/** Actel Error Register **/
#define ACTELErrorReg           0xB101
#define ACTELErrorReg_SIZE      1
#define ACTELErrorReg_WIDTH     8

/** Firmware Version Register**/
#define ACTELFwVersion          0xB102
#define ACTELFwVersion_SIZE     1
#define ACTELFwVersion_WIDTH    16

/** Actel Data Register **/
#define ACTELDataReg            0xB103    //
#define ACTELDataReg_SIZE       1
#define ACTELDataReg_WIDTH      32

/** Register holds the number of Errors encountered during RBAV **/
#define ACTELReadbackErrorCount          0xB104
#define ACTELReadbackErrorCount_SIZE     1
#define ACTELReadbackErrorCount_WIDTH    16

/** Register holds the number of the last frame with error **/
#define ACTELLastErrorFrameNumber        0xB105
#define ACTELLastErrorFrameNumber_SIZE   1
#define ACTELLastErrorFrameNumber_WIDTH  12

/** Number of Last frame beeing verified **/
#define ACTELLastFrameNumber             0xB106
#define ACTELLastFrameNumber_SIZE        1
#define ACTELLastFrameNumber_WIDTH       12

/** Number of times a complete readback and verification was done **/
#define ACTELReadbackCycleCounter        0xB107
#define ACTELReadbackCycleCounter_SIZE   1
#define ACTELReadbackCycleCounter_WIDTH  16

/** Register to stop automatic configuration **/
#define ACTELStopPowerUpCodeReg          0xB112
#define ACTELStopPowerUpCodeReg_SIZE     1
#define ACTELStopPowerUpCodeReg_WIDTH    32

/** Resets the RCU when written to **/
#define ACTELRCUReset            0x2000
#define ACTELRCUReset_SIZE       1
#define ACTELRCUReset_WIDTH      1

/** Resets the Actel when written to **/
#define ACTELReset               0x2003
#define ACTELReset_SIZE          1
#define ACTELReset_WIDTH         1


#endif //__CODEBOOK_RCU_H
