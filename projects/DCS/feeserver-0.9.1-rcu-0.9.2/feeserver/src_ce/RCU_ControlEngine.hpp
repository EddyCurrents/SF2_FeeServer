// $Id: RCU_ControlEngine.hpp,v 1.18 2007/09/01 12:40:38 richter Exp $

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

#ifndef __RCU_CONTROLENGINE_HPP
#define __RCU_CONTROLENGINE_HPP

// the CEfec class is only used in conjunction with the CErcu device
#ifdef RCU

#include "controlengine.hpp"
#include "issuehandler.hpp"
#include "dev_actel.hpp"
#include <vector>

class RcuBranchLayout;
/**
 * @page sec_rcuce The ControlEngine for the RCU
 * This sections describes the behavior of the RCU ControlEngine.
 * The sections is intended to be a reference manual for the user, not the
 * developer. The RCU CE implements the complete device (say RCU) dependend
 * access code of the FeeServer. <br>
 *
 * The CE consists of 3 functional parts:
 * -# @ref rcuce_services
 * -# @ref rcuce_commands
 * -# @ref rcuce_sm
 *
 * @section rcuce_services Services
 *
 * @section rcuce_commands Commands
 * The FeeServer supports one command channel in order to receive commands
 * from other applications via this DIM channel. The format follows the 
 * FEE API. The command is received by the FeeServer core, stripped by the
 * FEE Header and sent to the ControlEngine through the issue method.
 *
 *
 *
 * @see @ref feesrv_ceapi
 *
 * @section rcuce_sm State Machines
 * The RCU and the set of FECs make a very complex device. The state of the
 * different sub devices is controlled by a couple of internal state machines,
 * e.g. for the RCU as a hardware device (see CErcu) and for each FEC (see
 * CEfec).
 *
 * The internal state machines are hidden from the user and the SCADA system.
 * For simplicity, there is \b ONE main state machine implemented by the
 * RCUControlEngine class. The state machine represents a 'collective' state of
 * the whole hardware entity. 
 *
 * \b NOTE: \em States and \em Actions of the main state machine are adapted to
 * the states of the SCADA system. Both names and identifiers of states/actions
 * are different from the standard CEStateMachine. The translation is
 * implemented in the PVSSStateMapper.
 *
 * @see RCUControlEngine, PVSSStateMapper.
 */

// forward declarations
class CErcu;

/**
 * @class RCUControlEngine
 * The main class for the RCU ControlEngine.
 *
 * \b TOC
 * - @ref RCUControlEngine_states
 * - @ref RCUControlEngine_transitions
 * - @ref RCUControlEngine_customization
 *
 * This is the main device for the RCU FeeServer. It is a custom implementation
 * of the ControlEngine. Customization for the RCU includes
 * - the CErcu sub-device with CEfec devices
 * - open definition of the branch layout (FEC connected to the RCU buses)
 * - command set for RCU access (@ref FEESVR_CMD_RCU)
 * - states according to the PVSS SCANDA system for TPC Front-end Electronics 
 *
 * The RCUControlEngine is a plain software
 * device meaning there is no direct hardware underlying. It controles hardware
 * devices like the CErcu with CEfec sub-devices. The states and
 * transitions of this devices are known to the control system in the upper
 * layers. The states are published via a DIM channel and mirrord by a state
 * machine in the SCADA system.
 * 
 * @section RCUControlEngine_states States
 * <b>IDLE (33)</b>
 * - FECs off
 * - DDL SIU is default AltroBus Master (ABM)
 * - ABM can be set by the @ref RCU_SET_ABM command
 * - Monitoring off 
 * - Msg Buffer Driver unlocked (other application can access the hardware and
 *   change the status via the msgBufferInterface)
 * <br>
 *
 * <b>STANDBY (5)</b>
 * - state of FECs unchanged
 * - DDL SIU is AltroBus Master (ABM)
 * - ABM can be set by the @ref RCU_SET_ABM command 
 * - Monitoring on
 * - Msg Buffer Driver unlocked (other application can access the hardware and
 *   change the status via the msgBufferInterface)
 * <br>
 *
 * <b>DOWNLOADING (21)</b>
 * - state of FECs will be changed according to configuration
 * - reset of FECs is part of the configuration
 * - DCS board is AltroBus Master (ABM)
 * - Monitoring on
 * - Msg Buffer Driver locked (other application dont't have access to the hardware
 * <br>
 *
 * <b>STBY_CONFIGURED (3)</b>
 * - state of FECs according to configuration
 * - DDL SIU is AltroBus Master (ABM)
 * - Monitoring on
 * - Msg Buffer Driver locked (other application dont't have access to the hardware
 * <br>
 *
 * <b>RUNNING (34)</b>
 * - state of FECs according to configuration
 * - DDL SIU is AltroBus Master (ABM)
 * - Msg Buffer Driver locked (other application dont't have access to the hardware
 * - Monitoring on
 * - Trigger enabled
 *
 * <b>Note:</b> The FeeServer evaluates the version of the RCU firmware. If the version
 * does not support access to the MSM registers while the DDL SIU is ABM, the DCS board
 * is set as ABM
 * <pre>
 *
 *    ----------
 *   | IDLE (33)| <----------------------------------------- 
 *    ----------      |                |                    |
 *        |           ^                |                    |
 *        |           | GO_IDLE        |                    |
 *        |           |                |                    |
 *        |    ------------            |                 -----------
 *        |   | STANDBY (5)| <---------+-<- GO_STANDBY -| ERROR (13)|
 *        |    ------------    |       |                 -----------
 *        |           |        |       |                can be reached by any state
 *        | CONFIGURE |        |       |                or result of transition	      
 *        |           |        |       |                left by action 'go_standby'
 *      -----------------      |       |                or 'go_idle'
 *     | DOWNLOADING (21)|     ^       ^   
 *      -----------------      |       |   
 *          ^       |     GO_STANDBY   | 
 *          |       |          |       |   
 *      CONFIGURE   |          |    GO_IDLE 
 *          |       |          |       |  
 *          |  --------------------    |
 *           -| STBY_CONFIGURED (3)|---
 *             -------------------- 
 *                   | |         
 *            START  | ^  STOP   
 *                   | |         
 *              --------------      
 *             | RUNNING (34) |
 *              --------------
 * </pre>
 * @see PVSSStateMapper for State and Action IDs
 *
 * @section RCUControlEngine_transitions Transitions
 * <b>GO_IDLE</b>
 * - switch FECs off
 *
 * <b>GO_STANDBY</b>
 *
 * <b>CONFIGURE</b>
 *
 * <b>START</b>
 *
 * <b>STOP</b>
 *
 * @section RCUControlEngine_customization Customization
 *
 * @ingroup rcuce
 */
class RCUControlEngine : public ControlEngine, public CEIssueHandler {
public:
#ifdef RCU
  RCUControlEngine();
  ~RCUControlEngine();

  /**
   * Virtual method for returning the ID of the Command-group. 
   * This function will be overwritten by the different classes for the command-groups.
   * @return             GroupId, -1 if id not valid
   */
  int GetGroupId();

  /**
   * Enhanced selector method.
   * The function can be overwritten in case the group Id is not sufficient to select
   * a certain handler.
   * @return          1 if the handler can process the cmd with the parameter
   */
  int CheckCommand(__u32 cmd, __u32 parameter);
  
  /**
   * Command handling for the RCUControlEngine.
   * @see CEIssueHandler::issue for parameters and return values.
   */
  int issue(__u32 cmd, __u32 parameter, const char* pData, int iDataSize, CEResultBuffer& rb);
  
  /**
   * Get the expected size of the payload for a command.
   * @param cmd       command id stripped py the parameter
   * @param parameter the parameter (16 lsb of the command header)
   * @return number of expected 32 bit words
   */
  int GetPayloadSize(__u32 cmd, __u32 parameter);

  enum {
    eGoConfDDL = eLastDefaultTransition+1,
    eCloseDLL
  };

  /**
   * Helper struct to keep control of open @ref FEE_CONFIGURE.
   * It holds the hardware address and the decoded parameters.
   */
  struct FeeConfigureDesc {
    __u32 hwAddress;
    __u32 device;
    __u32 fec;
    __u32 altro;
    __u32 channel;
  };

  /**
   * Evaluate a part of the @ref FEE_CONFIGURE hardware address.
   * The funtion masks and shifts the hwAddress according to the mask
   * parameter. The result is right shifted to the first non-zero bit
   * of the mask.
   * @param hwAddress  hardware address
   * @param mask       bit mask
   * @return fraction of the hardware address
   */
  __u32 EvaluateHwAddress(__u32 hwAddress, __u32 mask) const;

protected:
  /**
   * Set the RCU branch layout.
   * The method can be used by RCUControlEngine child classes in order to
   * customize the branch layout.
   */
  int SetRcuBranchLayout(RcuBranchLayout* pLayout);
  
private:
  /**
   * Device ID's for the sub-devices of the RCUControlEngine
   */
  enum {
    eRCUDevice=0,
    eACTELDevice
  };

  /**
   * Check whether the sub devices are locked.
   * @return 1 devices locked, 0 not locked
   */
  virtual int SubDevicesLocked();

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
   * get the name of a default transition.
   * Can be overridden in order to change the default name. The name of the transition corresponds
   * one to one to the name of the <i>Action</i> which triggers the transition.
   * @param transition   the id of the transition to check
   * @return             pointer to name
   */
  const char* GetDefaultTransitionName(CETransition* pTransition);

  /**
   * Check if the transition is allowed for the current state.
   * This overrides the @ref CEStateMachine::IsAllowedTransition method in order
   * to change the default behavior.
   * @param transition   pointer to transition descriptor
   * @return             1 if yes, 0 if not, neg. error code if failed
   */
  int IsAllowedTransition(CETransition* pT);

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
   * Handler called when state machine changes to state CONFIGURING
   * The function can be implemented to execute specific functions when entering
   * state CONFIGURING.
   * @return            neg. error code if failed
   */
  int EnterStateCONFIGURING();

  /**
   * Handler called when state machine leaves state CONFIGURING
   * The function can be implemented to execute specific functions when leaving
   * state CONFIGURING.
   * @return            neg. error code if failed
   */
  int LeaveStateCONFIGURING();

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

  /**
   * Handler called before the service udate loop.
   */
  int PreUpdate();

  /**
   * Find a configuration descriptor.
   * @param tag          search tag
   * @param type         <0 hwAddress                                   <br>
   *                     =0 device                                      <br>
   *                     =1 fec
   * @return position in the array, -ENOENT if not found
   */
  int FindFeeConfigureDesc(__u32 tag, int type=-1);

  /**
   * Create a configuration descriptor from a hardware address
   * @param hwAddress
   * @return configure descriptor
   */
  FeeConfigureDesc CreateFeeConfigureDesc(__u32 hwAddress);

  /**
   * Execute the @ref FEE_CONFIGURE command.
   * @see CEIssueHandler::issue for parameters and return values.
   */
  int ExecFeeConfigure(__u32 parameter, const char* pData, int iDataSize, CEResultBuffer& rb);


  /** the RCU sub device */
  CErcu* fpRCU;

  /** The ACTEL sub device */
  CEactel* fpACTEL;

  /** list of open @ref FEE_CONFIGURE */
  std::vector<FeeConfigureDesc> fConfigurations;

  /** debug printout indent*/
  int fIndent;

#endif //RCU
};

class DefaultRCUagent : public CEagent
{
private:
  /**
   * Indicate a default Agent.
   * This function is implemented to indicate a default Agent. The agent
   * as lower priotity than non-default agents.
   */
  int IsDefaultAgent();

  /**
   * Get the name of the agent.
   */
  const char* GetName();

  /**
   * Create the instance of the Control engine.
   */
  ControlEngine* CreateCE();
};

/**
 * @class PVSSStateMapper
 * A mapper class to translate names and identifiers for States and Actions to
 * the PVSS state encoding.
 * The RCUControlEngine is derived from CEStateMachine and inherits the state
 * machine functionality. Standard Names/Identifiers are translated by the
 * PVSSStateMapper.
 * @ingroup rcuce
 */
class PVSSStateMapper : public CEStateMapper {
public:
  PVSSStateMapper();
  ~PVSSStateMapper();

  /**
   * Action definitions of the PVSS mirror SM
   */
  enum {
    /** START */
    kActionStart         = 16,
    /** CONFIGURE */
    kActionConfigure     =  6,
    /** STOP */
    kActionStop          =  3,
    /** GO_STANDBY */
    kActionGoStandby     =  2,
    /** GO_IDLE */
    kActionGoOff         =  1
  };

  /**
   * State definitions of the PVSS mirror SM
   */
  enum {
    /** RUNNING */
    kStateRunning        = 34,
    /** IDLE */
    kStateIdle           = 33,
    /** CONF_DDL */
    kStateConfDdl        = 32,
    /** CONFIGURING */
    kStateDownloading    = 21,
    /** ERROR */
    kStateError          = 13,
    /** RAMPING_UP */
    kStateRampingUp      =  8,
    /** RAMPING_DOWN */
    kStateRampingDown    =  7,
    /** STANDBY */
    kStateStandby        =  5,
    /** OFF (for completeness, not used by the MAIN state machine) */
    kStateOff            =  4,
    /** STBYCONFIGURED */
    kStateStbyConfigured =  3,
    /** UNKNOWN */
    kStateElse           =  0
  };

  /**
   * Get the mapped state from the CEState.
   * @param state         CEState
   * @return              translated state
   */
  int GetMappedState(CEState state);

  /**
   * Get the CEState from the mapped state.
   * @param state         CEState
   * @return              translated state
   */
  CEState GetStateFromMappedState(int state);

  /**
   * Get name of the state according to translation scheme.
   * @return translated name
   */
  const char* GetMappedStateName(CEState state);

};

#endif //RCU

#endif //__RCU_CONTROLENGINE_HPP
