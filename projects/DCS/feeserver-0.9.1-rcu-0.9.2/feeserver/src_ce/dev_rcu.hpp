// $Id: dev_rcu.hpp,v 1.21 2007/09/03 07:32:16 richter Exp $

/************************************************************************
**
**
** This file is property of and copyright by the Experimental Nuclear 
** Physics Group, Dep. of Physics and Technology
** University of Bergen, Norway, 2006
** This file has been written by Matthias Richter
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

#ifndef __DEV_RCU_HPP
#define __DEV_RCU_HPP

#ifdef RCU

#include <vector>
#include <time.h>
#include "issuehandler.hpp"
#include "dimdevice.hpp"
#include "codebook_rcu.h"
#include "lockguard.hpp"
#include "ce_base.h"
#include "branchlayout.hpp"

class CEfec;
class RcuBranchLayout;
class DcscCommandHandler;
class DCSCMsgBuffer;

/**
 * @class CErcu
 * Device implementation of the RCU.
 * This is the main device for the RCU. It controls the status of the RCU, the
 * Altro Bus Master setting, ...
 * <br>
 *
 * @section CErcu_states States
 * The device follows the standard States and Transitions of the @ref CEStateEngine:<br>
 * <b>OFF:</b>
 * - no access to the RCU
 * - RCU bus disabled
 * <br>
 *
 * <b>ON:</b>
 * - DDL SIU is AltroBus Master (ABM)
 * <br>
 *
 * <b>CONFIGURING:</b>
 * - not yet decided whether this is needed or not
 * - DCS board is AltroBus Master (ABM)
 * - configuration of RCU behavior in progress
 * <br>
 *
 * <b>CONFIGURED:</b>
 * - DCS board is AltroBus Master (ABM)
 * <br>
 *
 * <b>RUNNING:</b>
 * - start the RCU sequencer and wait until it is finished
 * - timeout @ref RCU_SEQUENCER_TIMEOUT in usec
 *
 * @section CErcu_highlevel High-Level commands
 * See @ref CErcu::HighLevelHandler
 * @ingroup rcu_ce
 */
class CErcu : public CEDimDevice, virtual public CEIssueHandler {
public:
  CErcu();
  ~CErcu();

private:
  /**
   * Evaluate the state of the hardware.
   * The function does not change any internal data, it performes a number of evaluation
   * operations.
   * @return             state
   */
  CEState EvaluateHardware();

  /**
   * Internal function called during the @ref CEStateMachine::Armor procedure.
   * The function is called from the @ref CEStateMachine base class to carry out
   * device specific start-up tasks.
   */
  int ArmorDevice();

  /**
   * Check if the transition is allowed for the current state.
   * This overrides the @ref CEStateMachine::IsAllowedTransition method in order
   * to change the default behavior.
   * @param transition   pointer to transition descriptor
   * @return             1 if yes, 0 if not, neg. error code if failed
   */
  //int IsAllowedTransition(CETransition* pT);

  /******************************************************************************
   ********************                                     *********************
   ********************    state and transition handlers    *********************
   ********************                                     *********************
   *****************************************************************************/

  /**
   * Handler called when state machine changes to state OFF
   * The function can be implemented to execute specific functions when entering
   * state OFF.
   * @return            neg. error code if failed
   */

  int EnterStateOFF();

  /**
   * Handler called when state machine leaves state OFF
   * The function can be implemented to execute specific functions when leaving
   * state OFF.
   * @return            neg. error code if failed
   */
  int LeaveStateOFF();

  /**
   * Handler called when state machine changes to state ON
   * The function can be implemented to execute specific functions when entering
   * state ON.
   * @return            neg. error code if failed
   */
  int EnterStateON();

  /**
   * Handler called when state machine leaves state ON
   * The function can be implemented to execute specific functions when leaving
   * state ON.
   * @return            neg. error code if failed
   */
  int LeaveStateON();

  /**
   * Handler called when state machine changes to state CONFIGURED
   * The function can be implemented to execute specific functions when entering
   * state CONFIGURED.
   * @return            neg. error code if failed
   */
  int EnterStateCONFIGURED();

  /**
   * Handler called when state machine leaves state CONFIGURED
   * The function can be implemented to execute specific functions when leaving
   * state CONFIGURED.
   * @return            neg. error code if failed
   */
  int LeaveStateCONFIGURED();

  /**
   * Handler called when state machine changes to state RUNNING
   * The function can be implemented to execute specific functions when entering
   * state RUNNING.
   * @return            neg. error code if failed
   */
  int EnterStateRUNNING();

  /**
   * Handler called when state machine leaves state RUNNING
   * The function can be implemented to execute specific functions when leaving
   * state RUNNING.
   * @return            neg. error code if failed
   */
  int LeaveStateRUNNING();

  /**
   * The handler for the <i>switchon</i> action.
   * @param iParam       integer parameter passed from the caller of the action
   * @param pParam       void pointer passed from the caller of the action
   * @return             >=0 success, neg error code if failed
   */
  int SwitchOn(int iParam, void* pParam); 

  /**
   * The handler for the <i>shutdown</i> action.
   * @param iParam       integer parameter passed from the caller of the action
   * @param pParam       void pointer passed from the caller of the action
   * @return             >=0 success, neg error code if failed
   */
  int Shutdown(int iParam, void* pParam); 

  /**
   * The handler for the <i>configure</i> action.
   * @param iParam       integer parameter passed from the caller of the action
   * @param pParam       void pointer passed from the caller of the action
   * @return             >=0 success, neg error code if failed
   */
  int Configure(int iParam, void* pParam); 

  /**
   * The handler for the <i>reset</i> action.
   * @param iParam       integer parameter passed from the caller of the action
   * @param pParam       void pointer passed from the caller of the action
   * @return             >=0 success, neg error code if failed
   */
  int Reset(int iParam, void* pParam); 

  /**
   * The handler for the <i>start</i> action.
   * @param iParam       integer parameter passed from the caller of the action
   * @param pParam       void pointer passed from the caller of the action
   * @return             >=0 success, neg error code if failed
   */
  int Start(int iParam, void* pParam); 

  /**
   * The handler for the <i>stop</i> action.
   * @param iParam       integer parameter passed from the caller of the action
   * @param pParam       void pointer passed from the caller of the action
   * @return             >=0 success, neg error code if failed
   */
  int Stop(int iParam, void* pParam);

public:
  /**
   * Update service value for a FEC.
   * This functions checks for the state of the RCU and the FEC to access
   * and reads a 16 bit value from a BC register.
   * Switch to @ref FecAccess5 or @ref FecAccess8 depending on the RCU
   * firmware version.<br>
   * \em Note: The Id of a FEC is not necessarily equal to the position. The
   * layout if defined by the implementation of the active RcuBranchLayout
   * instance.
   * @param data         return target (16 bit)
   * @param FECid        FEC id
   * @param reg          register no
   * @return neg error code if failed
   */
  int UpdateFecService(int &data, int FECid, int reg);

  /**
   * 5 bit version of the SlowControl read access.
   * This version of the SlowControl encodes the address of a BC register
   * together with the FEC address and the command into a single address.
   * @param data         return target (16 bit)
   * @param FEC          FEC number
   * @param reg          register no
   * @return neg error code if failed
   */
  int ReadFecRegister5(int &data, int FEC, int reg);

  /**
   * 8 bit version of the SlowControl read access.
   * @param data         return target (16 bit)
   * @param FEC          FEC number
   * @param reg          register no
   * @return neg error code if failed
   */
  int ReadFecRegister8(int &data, int FEC, int reg);

  /**
   * Write a BC register of a FEC.
   * Switches to @ref WriteFecRegister5 or @ref WriteFecRegister8 depending
   * on the firmware version.<br>
   * \em Note: The Id of a FEC is not necessarily equal to the position. The
   * layout if defined by the implementation of the active RcuBranchLayout
   * instance.
   * @param data         data to write
   * @param FECid        FEC id
   * @param reg          register no
   * @return neg error code if failed
   */
  int WriteFecRegister(int data, int FECid, int reg);

  /**
   * 5 bit version of the SlowControl write access.
   * This version of the SlowControl encodes the address of a BC register
   * together with the FEC address and the command into a single address.
   * @param data         data to write
   * @param FEC          FEC number
   * @param reg          register no
   * @return neg error code if failed
   */
  int WriteFecRegister5(int data, int FEC, int reg);

  /**
   * 8 bit version of the SlowControl read access.
   * @param data         data to write
   * @param FEC          FEC number
   * @param reg          register no
   * @return neg error code if failed
   */
  int WriteFecRegister8(int data, int FEC, int reg);

  /**
   * Update handler for RCU registers.
   * @param              value target
   * @param              address in RCU memory space
   * @param              data type of the service
   */
  int UpdateRcuRegister(TceServiceData* pData, int address, int type);

  /**
   * Find out how much FECs can be attached to a branch
   * 
   * @param branch	for now either 0x0 for Branch A or 0x1 for branch B
   * @return 		number of FECs that can max. be attached
   */
  int MaxFecsInBranch(unsigned short branch);

  /**
   * Find out which branch we are in
   * 
   * @param index       the address of the fec from which we want to know to know the branch
   * @return		0x000 for branch A or 0x001 for branch B
   */
  int GetBranch(int index);

  /**
   * Find the hardware address of a FEC
   * FECs can have ids different from the position in the actual hardware setup
   * which corresponds to a bit in the RCU AFL. The mapping is defined by a
   * custom @ref branchlayout class implementation.
   * @param id          id of the FEC
   * @return hardware address if found, -ENOENT if not found
   */
  int FindFecPosition(int id);

  /**
   * Find the position of a FEC with certain id.
   * 
   * @param branch      target to receive branch no
   * @param addr        target to receive address in branch
   * @return 0 if FEC id is found, -ENODEV
   */
  int GetFecPosition(int id, int &branch, int &addr);

  /******************************************************************************
   ********************                                     *********************
   ********************             API functions           *********************
   ********************                                     *********************
   *****************************************************************************/

  /**
  * Handling of binary RCU commands.
  * @see CEIssueHandler::issue for parameters and return values.
  *
  * @ingroup rcu_issue
  */ 
  int issue(__u32 cmd, __u32 parameter, const char* pData, int iDataSize, CEResultBuffer& rb);	
  
  /**
   * Get id of the RCU command group
   * @return          the command id (@ref FEESVR_CMD_RCU)
   */
  int GetGroupId();
  
  /**
   * Handler for high level commands.
   * The CErcu device provides the following high-level commands:
   * - RCU_WRITE_AFL
   * - RCU_READ_AFL
   * - RCU_WRITE_MEMORY
   * - RCU_READ_FW_VERSION
   * @see CEIssueHandler::HighLevelHandler.
   * @param command   command string
   * @param rb        result buffer
   * @return 1 if processed, 0 if not, neg. error code if failled 
   */
  int HighLevelHandler(const char* pCommand, CEResultBuffer& rb);

  /**
   * Command handling for the RCU command group.
   * @param cmd       command id stripped py the parameter
   * @param parameter the parameter (16 lsb of the command header)
   * @param pData     payload buffer excluding the header
   * @param iDataSize size of the payload buffer in bytes 
   * @param rb        reference to result buffer
   * @return          number of bytes of the payload buffer which have been processed
   *                  neg. error code if failed
   * @ingroup rcu_issue
   */
  int TranslateRcuCommand(__u32 cmd, __u32 parameter, const char* pData, int iDataSize, CEResultBuffer& rb);
  
  /**
   * Get the expected size of the payload for a command.
   * @param cmd       command id stripped py the parameter
   * @param parameter the parameter (16 lsb of the command header)
   * @return number of expected 32 bit words
   */
  int GetPayloadSize(__u32 cmd, __u32 parameter);

  /**
   * Update the active FEC list from the hardware.
   * Reads the corresponding RCU register and updates the internal array
   * the function also creates the global array when invoked for the first time 
   */
  int ReadFECActiveList();

  /**
   * Switch the AFL register in order to switch a FEC.
   * \em Note: The Id of a FEC is not necessarily equal to the position. The
   * layout if defined by the implementation of the active RcuBranchLayout
   * instance.
   * @param fecid      id of the FEC
   * @param on         1=on, 0=off
   * @return 1 if the AFL has been switched
   *         0 if the AFL was already according to the switch
   *         neg. error code if failed
   */
  int SwitchAFL(int fecid, int on);

  /**
   * Ramp the switching of the AFL bits.
   * The function changes the bits of the AFL one by one in order to realize a
   * ramped power up/down of the FECs.
   * @param setAfl     AFL target value
   */
  int RampAFL(__u32 setAfl);

  /**
   * Produce a text output from the list of valid FECs.
   * NOTE: there are two internal list for the status of the FECs
   *       the "vaild list"  denotes the cards which has been registered to the CE at startup
   *       the "active list" represents the hardware status 
   */
  int GetFECTableString(char** ppTgt);


  /**
   * Check whether a FEC is valid.
   * 'Valid' means a FEC is defined in the current system setup and services
   * are published.
   */
  int IsFECvalid(int i);

  /**
   * Check whether a FEC of specified ID is active.
   * 'Active' means the FEC is powered and has top be monitored. 'Valid' means
   * it is defined in the current system setup and services are published.<br>
   * \em Note: The Id of a FEC is not necessarily equal to the position. The
   * layout if defined by the implementation of the active RcuBranchLayout
   * instance.
   * @param id      id of the FEC
   * @return   1 if active, 0 if not
   */
  int IsFECactive(int id);

  /**
   * Check whether a FEC at specified position is active.
   * 'Active' means the FEC is powered and has top be monitored. 'Valid' means
   * it is defined in the current system setup and services are published.
   * @param pos     position in the branches (= bit in the AFL register)
   * @return   1 if active, 0 if not
   */
  int IsFECposActive(int pos);

  /**
   * Build an address for writing to or reading from a BC register.
   * 
   * @param rnw		ReadNotWrite: set 0 for a write to a BC register, 1 for a read of an register
   * @param bcast		Broadcast: set to 1 for an boradcast
   * @param branch	Branch: set 0 for Branch A or 1 for Branch B
   * @param FECAdr	FrontEndCard address, specifies the FEC, 0x000 is the first one
   * @param BCRegAdr	Bordcontroller registeraddress: the address of the register you want to talk to
   * @return 		the address to the BC register you want to write to
   * 
   * For further information see the RCU Firmware: Registers and Commands, chapter 3.3
   */
  __u32 EncodeSlowControl5Cmd(unsigned short rnw, unsigned short bcast, unsigned short branch, unsigned short FECAdr, unsigned short BCRegAdr);

  /**
   * Tries to read a register of the given BC address and evaluates the
   * Error register
   * @param address	writeaddress: the address that should be checked
   * @return		returns 0 if no error occurred,
   *                    -EIO    if RCU acces fails 
   *                    -EBADF  if card was not active, 
   *                    -EACCES if no ack was received from card
   */
  int CheckSingleFEC(__u32 writeAddress);

  /**
   * Checks if the validList matches the hardware.
   * The function checks the valid entry for all FECs
   * which are enabled in the FECActiveList register.
   * @return		the errorcode
   */
  int CheckValidFECs();
  
  /**
   * Detect all working FECs by turning all the cards on, and then trying to read
   * a register from each card. The list of valid cards is created according to 
   * result. The status of the AFL register (RCU) is restored after the test.
   * @return	nRet, the errorcode
   */
  int DetectFECs(); 

  /**
   * Set the branch layout.
   * The method can be used by RCUControlEngine child classes in order to
   * customize the branch layout.
   */
  int SetBranchLayout(RcuBranchLayout* pLayout);

private:
  /******************************************************************************
   ********************                                     *********************
   ********************          internal functions         *********************
   ********************                                     *********************
   *****************************************************************************/

  /**
   * Write a single location (32bit word).
   * Wrapper to dcscMsgBuffer method.
   * Overloaded by the RCU simulation class.
   * @param address   16 bit address in RCU memory space
   * @param data      data word
   * @return
   */
  virtual int SingleWrite(__u32 address, __u32 data);

  /**
   * Read a single location (32bit word).
   * Wrapper to dcscMsgBuffer method.
   * Overloaded by the RCU simulation class.
   * @param address   16 bit address in RCU memory space
   * @param pData     buffer to receive the data
   * @return
   */
  virtual int SingleRead(__u32 address, __u32* pData);

  /**
   * Write a number of 32bit words beginning at a location.
   * The function takes care for the size of the MIB and splits the operation if
   * the amount of data to write exceeds the MIB size.
   * The function expects data in little endian byte order.
   * Wrapper to dcscMsgBuffer method.
   * Overloaded by the RCU simulation class.
   * @param address   16 bit address in RCU memory space
   * @param pData     buffer containing the data
   * @param iSize     number of words to write
   * @param iDataSize size of one word in bytes, allowed 1 (8 bit), 2 (16 bit),3 (compressed 10 bit) 
   *                  or 4 (32 bit) -2 and -4 denote 16/32 bit words which get swapped
   * @return
   */
  virtual int MultipleWrite(__u32 address, __u32* pData, int iSize, int iDataSize);

  /**
   * Read a number of 32bit words beginning at a location.
   * Wrapper to dcscMsgBuffer method.
   * Overloaded by the RCU simulation class.
   * @param address   16 bit address in RCU memory space
   * @param iSize     number of words to write
   * @param pData     buffer to receive the data, the function expect it to be of suitable size
   *                  (i.e. iSize x wordsize)
   * @return          number of 32bit words which have been read, neg. error code if failed
   */
  virtual int MultipleRead(__u32 address, int iSize,__u32* pData);

  /**
   * check the rcu error register
   * @result neg. error code if failed
   *         -BUSY        busy bit remains active
   *         -ETIMEDOUT   timeout during FEC access
   *         -EBADF       pattern memory missmatch
   *         -EIO         Altro Bus in error state of Channel HW address missmatch
   *         -EINTR       Sequencer aborted
   */
  int checkRCUError();

  /**
   * start the sequencer to interprete the sequence written to rcu instruction memory
   * @param start       8 bit start address in the RCU instruction memory
   */
  int sendRCUExecCommand(unsigned short start);

  /**
   * send a stop command to the rcu sequencer
   */
  int sendStopRCUExec();

  /**
   * write data to rcu memory
   * @param pBuffer     data buffer
   * @param iCount      # of words to write
   * @param iWordSize   size of word in byte
   * @param u32BaseAddress address in RCU memory space
   */
  int writeCommandBufferToRCUMem(const char* pBuffer, int iCount, int iWordSize, __u32 u32BaseAddress);

  /**
   * write a data block to the rcu instruction memory
   * the address of the instruction memory is determined by the ALTROInstMEM identifier in codebook_rcu.h
   * the function expects 32 bit data
   * parameters:
   * @param pBuffer     data Buffer
   * @param iSize       size of data buffer in byte
   * @return            # of words written, neg. error code if failed
   */
  //int writeCommandBufferToRCUInstructionMem(const char* pBuffer, int iSize);

  /**
   * write a data block to the rcu pattern memory
   * the address of the pattern memory is determined by the ALTROPatternMEM identifier in codebook_rcu.h
   * the function can handle 8, 16 and 32 bit data, current constraint: the data block must be aligned 
   * to 32 bit, otherwize the last bytes are skipped 
   * handling of 10bit compressed data forseen
   * parameters:
   * @param pBuffer     data Buffer
   * @param iSize       size of data buffer in byte
   * @param iWordSize   bit width of the data
   * @return            # of words written, neg. error code if failed
   */
  //int writeDataBlockToRCUPatternMem(const char* pBuffer, int iSize, int iWordSize);

  /**
   * Write a data block to RCU memory
   * @param pBuffer      data buffer to write
   * @param iSize        size of the buffer in byte
   * @param iNofWords    number of words to write
   * @param iWordSize    size of one word in bit
   * @param tgtAddress   address in RCU memory space
   * @param partSize     size of the corresponding partition in RCU memory space
   * @return number of processed bytes; neg. error code if failed 
   *
   */
  int writeDataBlockToRCUMem(const char* pBuffer, int iSize, int iNofWords, int iWordSize,
				    __u32 tgtAddress, int partSize);

  /**
   * Read from result buffer, allocate the target buffer and insert it as the result
   * the buffer will be automatically transferred as result of the issue method to the
   * FeeServer core, and will be relaesed by that.<br>
   * If the size of the partition is provided the function will check whether the partition
   * is large enough. If not, the output is truncated.
   * @param address  address in rcu memory space
   * @param size     # of 32 bit words to read from the memory
   * @param partSize size of the partition in RCU memory space
   * @return:        >0 no of words read, neg. error code if failed
   */
  int readRcuMemoryIntoResultBuffer(__u32 address, int size, int partSize, CEResultBuffer& rb);

  /*
   * Update the active FEC list from the hardware.
   * Reads the corresponding RCU register and updates the internal array
   * the function also creates the global array if needed.<br>
   * <b>Note:</b> There are two internal lists: the <i>valid</i> list which 
   * is build at startup of the server and indicates all FECs which are valid
   * and have corresponding service channels, and the <i>active</i> list which
   * indicates an <i>actice</i> FEC out of the <i>valid</i> ones. 
   */
  //int ReadFECActiveList();

  /**
   * Read the registers to publish from environment variables.
   * The function was raraly tested.
   */
  int MakeRegItemLists();

  /**
   * Publish services for the FECs.
   * The function is a loop over all valid FECs which calls @ref PublishService.
   */
  int PublishServices();

  /**
   * Publish registers of RCU memory.
   * The function is a loop over all valid FECs which calls @ref PublishReg.
   */
  int PublishRegs();

  /**
   * Publish a single register of the RCU
   */
  int PublishReg(int regNum);

  /**
   * Version ids for RCU firmware
   */
  enum rcu_fw_version {
    eRCUversionUnknown   = 0,
    eRCUversionAncient05,
    eRCUversionMar06,
    eRCUversionMay06
  };

  /**
   * Read the version register of the RCU firmware/ evaluate the version.
   * The functin Reads the version register of the RCU firmware which is available 
   * in newer fw versions. Otherwise it tries to evaluate the version by probing
   * properties. 
   * The register table is loaded according to the fw version. If version avaluation
   * is not possible, always the latest layout is assumed but a warning is given.
   * By this policy, this function adopts to older fw versions if they are known.<br>
   * Currently, the function sets also the altro bus master, this is due to the code
   * conversion policy, we just re-use the old enableControl function. 
   *
   * The functions reads the RCU firmware version register and probes for supported
   * FW functionality if server was compiled with --enable-auto-detection.
   * Newer firmware versions provide access to the Slow control registers even 
   * when the DDL has access to the Altro Bus. In that case the control of the Altro 
   * Bus is given to the DDL. Older firmware versions require that the DCS board has 
   * access to the Altro Bus. The RCU firmware does not provide any state register
   * about the Altro Bus switch and others, so we have to probe and be lucky.<br>
   * The following scheme applies:
   * 1. The version register 0x8008 is checked for a valid RCU FW version number. <br>
   *  if no valid version number found <br>
   * 2. The version register 0x8006 is checked for a valid RCU FW version number. A
   *    few FW version from May 06 use 0x8006 as version register. This was before the
   *    final address map was fixed. <br>
   *  if no valid version number found and compiled with --enable-auto-detection <br>
   * 3. The FW is probed for features
   *
   * The function is overloaded by the CErcuSim simulattion class.
   * @return   0 DCS board is ABM <br>
   *           1 no RCU firmware version number found but MSM registers accessible
   *           while DDL SIU is ABM <br>
   *           >1 RCU firmware version number: DDL SIU is ABM <br>
   *           neg. error code if failed
   */
  virtual int EvaluateFirmwareVersion();

public:
  /**
   * Ids for the Altro Bus Master settings
   */
  enum abm_t {
    eAbmUnknown  = 0,
    eAbmDCSboard = CMDDCS_ON,
    eAbmDDLsiu   = CMDSIU_ON
  };

  /** the mutex */
  /** public until all access methods have been incorporated into rcu device */
  static CE_Mutex fMutex;

private:
  /**
   * Set the Altro Bus Master
   * The two mezzanine cards of the RCU have both a connection to the Altro Bus, but
   * only one at a time can control it. The DCS board can switch the multiplexer,
   * after Power On the DDL-SIU has by default the access.
   * Older RCU firmware versions does not allow the DCS board te access the registers
   * of the Monitoring and Safety Module (MSM) of the RCU. From May06 this was fixed,
   * and will later be extended to the Sequencer memories as well together with a
   * sequencer scheduler. <br>
   * This method is called according to the state @ref eStateOn and 
   * @ref eStateConfigured.
   * @return @ref abm_t, neg. error code if failed
   */
  int SetAltroBusMaster(abm_t master);

  /**
   * Set the options
   */
  int SetOptions(int options);

  /**
   * Set a option flag(s)
   */
  int SetOptionFlag(int of);

  /**
   * Clear a option flag(s)
   */
  int ClearOptionFlag(int of);

  /**
   * Check a single option flag
   */
  int CheckOptionFlag(int of);

  /** firmware version */
  int fFwVersion;

  /** mapping of FEC IDs and hardware addresses */
  std::vector<int> fFecIds;

  /** maximum no of FECs in branch A */
  int fMaxFecA;

  /** maximum no of FECs in branch B */
  int fMaxFecB;

  /** a copy of the RCU AFL (active FEC list) */
  __u32 fAFL;

  /** mask of the RCU AFL (valid FEC list) */
  __u32 fAFLmask;

  /** the active branch layout */
  RcuBranchLayout* fpBranchLayout;

  /** option flags */
  int fOptions;

  /** last access to the AFL */
  time_t fAflAccess;

  /** the instance of the message buffer interface */
  DCSCMsgBuffer* fpMsgBuffer;

  /** workaround for firmware bug (version 190606) 
   * if there was one access failure due to interference with memory access
   * via the DDL SIU, update is disabled for the current cycle
   */
  int fUpdateTempDisable;
};

/**
 * automatically check error status after RCU sequencer is finnished
 */
#define RCU_OPT_AUTO_CHECK       0x0001

/**
 * the default options
 */
#define RCU_DEFAULT_OPTIONS      RCU_OPT_AUTO_CHECK

#endif //RCU

#endif //__DEV_RCU_HPP
