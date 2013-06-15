/************************************************************************
**
**
** This file is property of and copyright by the Experimental Nuclear 
** Physics Group, Dep. of Physics and Technology
** University of Bergen, Norway, 2007
** This file has been written by Dominik Fehlker
** Please report bugs to dfehlker@htwm.de
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
#ifndef __DEV_ACTEL_HPP
#define __DEV_ACTEL_HPP


#ifdef RCU

//Beschreibung für das Device
//states, transitions

#include "dimdevice.hpp"
#include "codebook_rcu.h"
#include "issuehandler.hpp"
#include "controlengine.hpp"
#include "errno.h"
#include "ce_base.h"


/**
 * @class CEactel
 *
 * \b TOC
 * - @ref CEactel_states
 * - @ref CEactel_transitions
 *
 * @section CEactel_states States
 * <br>
 * <b> SCRUBBING </b>
 * - performs continous scrubbing (overwriting of all frames, no matter if the contain errors or not)
 * - Number of cycles can be monitored with the cyclecounter
 * 
 * 
 * <br>
 *
 * <b> READBACK </b>
 * - executes continous Frame by Frame Readback and Verification 
 * - number of cycles can be monitored with the CycleCounter
 * - number of occurred errors can be monitored with the FrameErrorCount
 *
 *
 * <br>
 *
 * <b> OFF / idle </b>
 * - No checks done
 *
 * <br>
 *
 * <b> ON / standby</b>
 * - Checks successfully performed, everything ready
 * 
 * <br>
 *
 *
 * <pre>
 *
 *
 *              +-------->---------+---------------->----------------------------+
 *              |                  |                                             |
 *              ^                  ^                                             |
 *              |                  |                                             V
 *       -------------       ------------                                     -----------
 *      |  SCRUBBING  |     |  READBACK  |            +- GO_STANDBY/GO_IDLE -| ERROR (13)|
 *       -------------       ------------             |                          -----------
 *         |        |          |       |              |           can be reached by any state
 *         |        ^          |       |              |           or result of transition	      
 *         |        |          |       |              |           left by action 'go_standby'
 *         |    GO_SCRUBBING   |       |              |           or 'go idle'
 *         V        |          ^       |              V
 *         |        |          |       V              |
 *         |        |     GO_READBACK  |              |
 *         |        |          |       |              |
 *         |        |          |       |              |
 *         |        |          |       |              |
 *         |   ------------------      |              |
 *         +->| ON / configured  |<----+--GO_STANDBY--+
 *             ------------------                     |
 *                   | |                              |
 *                   ^ |                              |
 *                   | |                              |
 *            START  | v  STOP                        |
 *                   | |                              |
 *              -------------                         |
 *             | OFF / idle |<------GO_IDLE-----------+
 *              -------------
 * </pre>
 * @see ActelStateMapper for Statenames
 *
 * @section CEactel_transitions Transitions
 * <b>GO_IDLE</b>
 *
 * <b>GO_STANDBY</b>
 *
 * <b>CONFIGURE</b>
 *
 * <b>START</b>
 *
 * <b>STOP</b>
 *
 * <b>GO_READBACK</b>
 *
 * <b>QUIT_READBACK</b>
 *
 * <b>GO_SCRUBBING</b>
 *
 * <b>QUIT_SCRUBBING</b>
 *
 * @ingroup rcuce
 */
class CEactel : public CEDimDevice, public CEIssueHandler {
public:
  CEactel();
  ~CEactel();

public:
  
  /**
   * Get the expected size of the payload for a command.
   * @param cmd       command id stripped py the parameter
   * @param parameter the parameter (16 lsb of the command header)
   * @return number of expected 32 bit words
   */
  int GetPayloadSize(unsigned int a, unsigned int b);

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
   * 
   *@return result value
   */
  int ArmorDevice();


  /**
   * get the name of a default transition.
   * Can be overridden in order to change the default name. The name of the transition corresponds
   * one to one to the name of the <i>Action</i> which triggers the transition.
   * @param transition   the id of the transition to check
   * @return             pointer to name
   */
  const char* GetDefaultTransitionName(CETransition* pTransition);

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
   * Handler called when state machine laves state OFF
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
   * Handler called when state machine laves state ON
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
  //int EnterStateCONFIGURED();
  
  /**
   * Handler called when state machine laves state CONFIGURED
   * The function can be implemented to execute specific functions when leaving
   * state CONFIGURED.
   * @return            neg. error code if failed
   */
  //int LeaveStateCONFIGURED();
  

  /**
   * Handler called when state machine changes to state RUNNING
   * The function can be implemented to execute specific functions when entering
   * state RUNNING.
   * @return            neg. error code if failed
   */
  int EnterStateRUNNING();

  /**
   * Handler called when state machine laves state RUNNING
   * The function can be implemented to execute specific functions when leaving
   * state RUNNING.
   * @return            neg. error code if failed
   */
  int LeaveStateRUNNING();


  /**
   * Handler called when state machine changes to state USER0
   * The function can be implemented to execute specific functions when entering
   * state USER0.
   * @return            neg. error code if failed
   */
  int EnterStateUSER0();
  
  /**
   * Handler called when state machine laves state USER0
   * The function can be implemented to execute specific functions when leaving
   * state USER0.
   * @return            neg. error code if failed
   */
  int LeaveStateUSER0();

  /**
   * Handler called when state machine changes to state USER1
   * The function can be implemented to execute specific functions when entering
   * state USER1.
   * @return            neg. error code if failed
   */
  int EnterStateUSER1();  

  /**
   * Handler called when state machine laves state USER1
   * The function can be implemented to execute specific functions when leaving
   * state USER1.
   * @return            neg. error code if failed
   */
  int LeaveStateUSER1();
  
  /**
   * Handler called when state machine changes to state USER2
   * The function can be implemented to execute specific functions when entering
   * state USER2.
   * @return            neg. error code if failed
   */
  //int EnterStateUSER2();  

  /**
   * Handler called when state machine laves state USER2
   * The function can be implemented to execute specific functions when leaving
   * state USER2.
   * @return            neg. error code if failed
   */
  //int LeaveStateUSER2();

  /**
   * The handler for the <i>switchon</i> action.
   * @param iParam       integer parameter passed from the caller of the action
   * @param pParam       void pointer passed from the caller of the action
   * @return             >=0 success, neg error code if failed
   */
  int SwitchOn(int iParam, void* pParam);

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
  
  /**
   * The handler for the <i>shutdown</i> action.
   * @param iParam       integer parameter passed from the caller of the action
   * @param pParam       void pointer passed from the caller of the action
   * @return             >=0 success, neg error code if failed
   */
  int ShutDown(int iParam, void* pParam);


  enum {
    eGoReadback = eLastDefaultTransition+1,
    eGoScrubbing,
    eQuitReadback,
    eQuitScrubbing
  };
  
public:

  /**
   * Command handling for the Actel command group.
   * @return          number of bytes of the payload buffer which have been processed
   *                  neg. error code if failed
   */
  int TranslateActelCommand(__u32 cmd, __u32 parameter, const char* pData, int iDataSize, CEResultBuffer& rb);
  
/*********************************
 ***   Internal functions     ***
 ********************************/  

  /**
   * Checks for a correct state of the hardware
   *
   * @return negative error code if failed
   **/
  int Checking();


  /**
   * Does a init procedure of the Xilinx from the Flash,
   * which will erase the configuration memory.
   *
   * @return
   **/
  int XilinxInit();
  
  /**
   * Does a startup procedure of the Xilinx
   *
   * @return
   **/
  int XilinxStartup();

  /**
   * Issues an Abort sequence on the Xilinx 
   *
   * @return
   **/
  int XilinxAbort();

  /**
   * Reads the last active code from the status
   * register and returns it in this manner:
   * 
   * 1:Initial Configuration, 2: Scrubbing, 3: Scrubbing continously
   * 4: Readback Verification, 5: Readback Verification continously
   * 
   * @return last active command
   **/
  int LastActiveCommand();

  /**
   * Reads the status register and returns codes for
   * occured errors
   *
   * -1: Smap Interface busy, -4: Configuration controller is busy
   * -8: Flash contains no pointer value
   *
   * @return occured errors
   **/
  int CheckStatusReg();

  /**
   * Reads the error register and returns codes for
   * occured errors
   *
   * -64: Parity Error in Selectmap RAM detected
   * -128: Parity Error in Flash RAM detected
   *
   * @return occured errors
   **/
  int CheckErrorReg();
  

  /**
   * Try to update the Configuration memory of the Xilinx
   *
   * @return
   **/  
  int TryUpdateConfiguration();

  /**
   * Try to update the Flash memory
   *
   * @return 
   **/
  int TryUpdateFlash();
  
  /**
   * Try to update Registers necessary for Validating 
   *
   * @return
   **/
  int TryUpdateValidation();
  
  /**
   * Read the firmware register
   *
   * @return
   **/
  int EvaluateFirmwareVersion();

  /**
   * Disable the automatic configuration function,
   * Has to be disabled for deleting the configuration memory
   *
   * @return
   **/
  int DisableAutomaticConfiguration();
  
  /**
   * Clears the Error register
   *
   * @return 0 if success, EIO in case of failure
   **/
  int ClearErrorReg();

  /**
   * Clears the Error register
   *
   * @return 0 if success, EIO in case of failure
   **/
  int ClearStatusReg();

  /**
   * Trigger the Initial Configuration
   *
   * @return
   **/
  int InitialConfiguration();

  /**
   * Scrub one time
   *
   * @return
   **/
  int Scrub();
  
  /**
   * Scrub a given number of times, 0 for continously
   *
   * @return
   **/
  int Scrub(int times);
  
  /**
   * Readback and Verification one time
   *
   * @return
   **/
  int Readback();
  
  /**
   * Readback and Verification a given number of times, 0 for continously
   *
   * @return
   **/
  int Readback(int times);

  /**
   * Trigger an abort command on the Xilinx
   *
   * @return
   **/
  int Abort();

  /**
   * Reads the Number of faulty frames from the Hardwareregister
   *
   * @return number of faulty frames
   **/
  unsigned short NumberOfFrameErrors();

  /**
   * Reads the Number of times a complete cycle of readback and
   * verification has been done (all frames)
   *
   * @return number of cycles
   **/
  unsigned short NumberOfCycles();

  /**
   * Read one Actel register
   * 
   **/
  int UpdateActelRegister(TceServiceData* pData, int address, int type);


/*********************************
 ***Issuehandler functions     ***
 ********************************/
  
  /**
   * Here we can check for the commands that we can serve
   * 
   * 
   **/
  int CheckCommand(__u32 command, __u32 parameter);

  /**
   * The commands (e.g. from the feeserver-ctrl arrive
   * here in this function and pass them on to the TranslateActelCommand
   **/
  int issue(__u32 cmd, __u32 parameter, const char* pData, int iDataSize, CEResultBuffer& rb);

  /**
   * Get the Group ID for the commands that we serve here
   * 
   * 
   **/
  int GetGroupId();
  
  
  /******************************************
   ***** Functions Operating on the Flash ***
   *****************************************/
  
  int EraseFlash();
  

  
  /**********************************************
   ***** DcscMsgBufferInterface Functions  ******
   *********************************************/
  
  /**
   * Find the bus state (Flash/msgbuffer/selectmap) and return it
   * @return the bus state
   **/
  int getBusState();
  
  /**
   * switch the bus to flash state
   * @return 
   **/
  int enterFlashState(int oldstate);
  
  /**
   * Restore the bus state given in the argument
   * @return 
   **/
  int restoreBusState(int oldstate);
  
  
};


/*********************************
 ***  Statemapper stuff ***
 ********************************/

/**
 * @class ActelStateMapper
 * A mapper class to translate names and identifiers for States and Actions to
 * the Actel state encoding.
 * The RCUControlEngine is derived from CEStateMachine and inherits the state
 * machine functionality. Standard Names/Identifiers are translated by the
 * ActelStateMapper.
 * @ingroup rcuce
 */
class ActelStateMapper : public CEStateMapper {
public:
  ActelStateMapper();
  ~ActelStateMapper();


  /**
   * Get name of the state according to translation scheme.
   * @return translated name
   */
  const char* GetMappedStateName(CEState state);

};

#endif //actel

#endif //__DEV_ACTEL_HPP

