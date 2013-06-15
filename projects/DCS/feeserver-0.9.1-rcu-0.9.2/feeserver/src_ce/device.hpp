// $Id: device.hpp,v 1.12 2007/09/01 17:06:39 richter Exp $

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

#ifndef __DEVICE_HPP
#define __DEVICE_HPP

#include <vector>
#include <cerrno>
#include <linux/types.h>   // for the __u32 type
#include "statemachine.hpp"
#include "issuehandler.hpp"

class DeviceCommandHandler;

/**
 * @class CEDevice
 * Base class for devices. It inherits a state machine
 * from @ref CEStateMachine and provides default handlers for transitions and 
 * enter/leave-state events. Those handlers esentially doesn't do enything and can be
 * overloaded by the specific implementation. E.g. the @ref EnterStateOFF function
 * is called whenever the state OFF is entered, but after the specific transition has
 * been executed. The @ref LeaveStateOFF method is called when leaving state OFF before the 
 * transition handler @ref SwitchOn is executed.<br>
 * Refer to @ref CEState and @ref CETransitionId for states and transitions and to @class
 * CEStateMachine for a diagram of the default behavior. Note that the states and 
 * transitions can get other names by overloading @ref CEStateMachine::GetDefaultStateName
 * and @ref CEStateMachine::GetDefaultTransitionName.
 * @ingroup rcu_ce_base
 */
class CEDevice : public CEStateMachine {
public:
  /** constructor */
  CEDevice(std::string name="", int id=-1, CEDevice* pParent=NULL, std::string arguments="");

  /** destructor */
  ~CEDevice();

  /**
   * Set the device id.
   */
  int SetDeviceId(int id);

  /**
   * Get the Id of the device.
   * The id is usually used for faster indexing of sub-devices
   * within the parent device.
   * @return id of the device
   */
  int GetDeviceId() {return fId;}

  /**
   * Set the parent device.
   */
  int SetParentDevice(CEDevice* pParent);

  /**
   * Set the external arguments.
   */
  int SetArguments(std::string arguments);

  /**
   * Synchronize all sub-devices.
   * Loops over all sub-devices and calls the @ref CEStateMachine::Synchronize function for
   * each device.
   * @return             >=0 success, neg error code if failed
   */
  int SynchronizeSubDevices();

  /**
   * Synchronize all sub-devices and the device itself afterwards.
   * @return             >=0 success, neg error code if failed
   */
  int SynchronizeAll();

  /**
   * Send an ACTION to a sub-device.
   * The function searches for the target in the list of sub-devices
   * and sends the action if it is found.
   * @return >=0 if successful
   *         -ENOENT  if target not found
   *         -EACCESS if the sub-devices are locked 
   */
  int SendActionToSubDevice(const char* target, const char* action);

  /**
   * Get the common base for all services.
   * @return pointer to string
   */
  const char* GetServiceBaseName();

protected:
  /**
   * Get the parent device.
   * @return pointer to parent device
   */
  CEDevice* GetParentDevice() {return fpParent;}


  /**
   * Get the initialization arguments.
   */
  std::string& GetArguments() {return fArguments;};

  /**
   * Internal function called during the @ref CEStateMachine::Armor procedure.
   * The function is called from the @ref CEStateMachine base class. It is pure virtual
   * and must be implemented by the derived device to carry out specific start-up tasks.
   */
  virtual int ArmorDevice()=0;

  /**
   * Evaluate the state of the hardware.
   * The function does not change any internal data, it performes a number of evaluation
   * operations.
   * @return             state
   */
  virtual CEState EvaluateHardware();

  /**
   * Handler called when state machine changes to state OFF
   * The function can be implemented to execute specific functions when entering
   * state OFF.
   * @return            neg. error code if failed
   */
  virtual int EnterStateOFF();

  /**
   * Handler called when state machine leaves state OFF
   * The function can be implemented to execute specific functions when leaving
   * state OFF.
   * @return            neg. error code if failed
   */
  virtual int LeaveStateOFF();

  /**
   * Handler called when state machine changes to state ON
   * The function can be implemented to execute specific functions when entering
   * state ON.
   * @return            neg. error code if failed
   */
  virtual int EnterStateON();

  /**
   * Handler called when state machine leaves state ON
   * The function can be implemented to execute specific functions when leaving
   * state ON.
   * @return            neg. error code if failed
   */
  virtual int LeaveStateON();

  /**
   * Handler called when state machine changes to state CONFIGURED
   * The function can be implemented to execute specific functions when entering
   * state CONFIGURED.
   * @return            neg. error code if failed
   */
  virtual int EnterStateCONFIGURED();

  /**
   * Handler called when state machine leaves state CONFIGURED
   * The function can be implemented to execute specific functions when leaving
   * state CONFIGURED.
   * @return            neg. error code if failed
   */
  virtual int LeaveStateCONFIGURED();

  /**
   * Handler called when state machine changes to state CONFIGURING
   * The function can be implemented to execute specific functions when entering
   * state CONFIGURING.
   * @return            neg. error code if failed
   */
  virtual int EnterStateCONFIGURING();

  /**
   * Handler called when state machine leaves state CONFIGURING
   * The function can be implemented to execute specific functions when leaving
   * state CONFIGURING.
   * @return            neg. error code if failed
   */
  virtual int LeaveStateCONFIGURING();

  /**
   * Handler called when state machine changes to state RUNNING
   * The function can be implemented to execute specific functions when entering
   * state RUNNING.
   * @return            neg. error code if failed
   */
  virtual int EnterStateRUNNING();

  /**
   * Handler called when state machine leaves state RUNNING
   * The function can be implemented to execute specific functions when leaving
   * state RUNNING.
   * @return            neg. error code if failed
   */
  virtual int LeaveStateRUNNING();

  /**
   * Handler called when state machine changes to state RUNNING
   * The function can be implemented to execute specific functions when entering
   * state ERROR.
   * @return            neg. error code if failed
   */
  virtual int EnterStateERROR();

  /**
   * Handler called when state machine enters user defined state 0.
   * The function can be implemented to execute specific functions when entering
   * user defined state 0.
   * @return            neg. error code if failed
   */
  virtual int EnterStateUSER0() {return 0;}

  /**
   * Handler called when state machine enters user defined state 1.
   * The function can be implemented to execute specific functions when entering
   * user defined state 1.
   * @return            neg. error code if failed
   */
  virtual int EnterStateUSER1() {return 0;}

  /**
   * Handler called when state machine enters user defined state 2.
   * The function can be implemented to execute specific functions when entering
   * user defined state 2.
   * @return            neg. error code if failed
   */
  virtual int EnterStateUSER2() {return 0;}

  /**
   * Handler called when state machine enters user defined state 3.
   * The function can be implemented to execute specific functions when entering
   * user defined state 3.
   * @return            neg. error code if failed
   */
  virtual int EnterStateUSER3() {return 0;}

  /**
   * Handler called when state machine enters user defined state 4.
   * The function can be implemented to execute specific functions when entering
   * user defined state 4.
   * @return            neg. error code if failed
   */
  virtual int EnterStateUSER4() {return 0;}

  /**
   * Handler called when state machine leaves user defined state 0.
   * The function can be implemented to execute specific functions when leaving
   * user defined state 0.
   * @return            neg. error code if failed
   */
  virtual int LeaveStateUSER0() {return 0;}

  /**
   * Handler called when state machine leaves user defined state 1.
   * The function can be implemented to execute specific functions when leaving
   * user defined state 1.
   * @return            neg. error code if failed
   */
  virtual int LeaveStateUSER1() {return 0;}

  /**
   * Handler called when state machine leaves user defined state 2.
   * The function can be implemented to execute specific functions when leaving
   * user defined state 2.
   * @return            neg. error code if failed
   */
  virtual int LeaveStateUSER2() {return 0;}

  /**
   * Handler called when state machine leaves user defined state 3.
   * The function can be implemented to execute specific functions when leaving
   * user defined state 3.
   * @return            neg. error code if failed
   */
  virtual int LeaveStateUSER3() {return 0;}

  /**
   * Handler called when state machine leaves user defined state 4.
   * The function can be implemented to execute specific functions when leaving
   * user defined state 4.
   * @return            neg. error code if failed
   */
  virtual int LeaveStateUSER4() {return 0;}

  /**
   * The handler for the <i>switchon</i> action.
   * @param iParam       integer parameter passed from the caller of the action
   * @param pParam       void pointer passed from the caller of the action
   * @return             >=0 success, neg error code if failed
   */
  virtual int SwitchOn(int iParam, void* pParam); 

  /**
   * The handler for the <i>shutdown</i> action.
   * @param iParam       integer parameter passed from the caller of the action
   * @param pParam       void pointer passed from the caller of the action
   * @return             >=0 success, neg error code if failed
   */
  virtual int Shutdown(int iParam, void* pParam); 

  /**
   * The handler for the <i>configure</i> action.
   * @param iParam       integer parameter passed from the caller of the action
   * @param pParam       void pointer passed from the caller of the action
   * @return             >=0 success, neg error code if failed
   */
  virtual int Configure(int iParam, void* pParam); 

  /**
   * The handler for the <i>reset</i> action.
   * @param iParam       integer parameter passed from the caller of the action
   * @param pParam       void pointer passed from the caller of the action
   * @return             >=0 success, neg error code if failed
   */
  virtual int Reset(int iParam, void* pParam); 

  /**
   * The handler for the <i>start</i> action.
   * @param iParam       integer parameter passed from the caller of the action
   * @param pParam       void pointer passed from the caller of the action
   * @return             >=0 success, neg error code if failed
   */
  virtual int Start(int iParam, void* pParam); 

  /**
   * The handler for the <i>stop</i> action.
   * @param iParam       integer parameter passed from the caller of the action
   * @param pParam       void pointer passed from the caller of the action
   * @return             >=0 success, neg error code if failed
   */
  virtual int Stop(int iParam, void* pParam);

protected:
  /**
   * Add a sub-device to the list.
   * \em Note: The device must have an id.
   * @param pDevice      pointer to device object
   * @return             >=0 success, neg error code if failed
   */
  int AddSubDevice(CEDevice* pDevice);

  /**
   * Delete all sub devices.
   * By default are sub devices are deleted. The function can be overridden.
   * @return             >=0 success, neg error code if failed
   */
  virtual int CleanupSubDevices();

  /**
   * Find a sub device.
   * Looks for the value of the pointer and the id.
   * @param pDevice      device instance to find
   */
  CEDevice* FindDevice(CEDevice* pDevice);

  /**
   * Find a sub device by id.
   * @param id           device id
   */
  CEDevice* FindDevice(int id);

  /**
   * Get the first sub device
   */
  CEDevice* GetFirstSubDevice();

  /**
   * Get the next sub device
   */
  CEDevice* GetNextSubDevice();

private:
  /**
   * Check whether the sub devices are locked.
   * The function decides from the current state whether sub-devices
   * can be changed from the outside or not.
   * @return 1 devices locked, 0 not locked
   */
  virtual int SubDevicesLocked();

  /** 
   * Id of the device.
   * The id is used for faster device routing inside the parent device.
   */
  int fId;

  /** the parent device */
  CEDevice* fpParent;

  /** 'Command line' arguments to the device */
  std::string fArguments;

  /**
   * the device list.
   * Currently there is no protection of the list as we assume that there is no dynamic
   * change of the list during operation.
   */
  std::vector<CEDevice*> fSubDevices;

  /** Current position in the sub device list */
  int fCurrentIdx;

  /** The common base for all services*/
  std::string fServiceBaseName;

  /** handler for common device commands */
  DeviceCommandHandler* fpCH;
  friend class CEStateMachine;
};

/**
 * @class DeviceCommandHandler
 * Common command handler for State Machines.
 * Binary commands:
 *  - @ref CE_GET_STATES
 *  - @ref CE_GET_TRANSITIONS
 *  - @ref CE_GET_STATE
 *  - @ref CE_GET_STATENAME
 *  - @ref CE_TRIGGER_TRANSITION
 * 
 * High-Level commands
 * <pre>
 *  CE_GET_STATES         <target>
 *  CE_GET_TRANSITIONS    <target> [ALL]
 *  CE_GET_STATE          <target>
 *  CE_GET_STATENAME      <target>
 *  CE_TRIGGER_TRANSITION <target> <transition>
 * </pre>
 */
class DeviceCommandHandler : public CEIssueHandler {
public:
  /** constructor */
  DeviceCommandHandler(CEDevice* pInstance=NULL);

  /** destructor */
  ~DeviceCommandHandler();

  /**
   * Get group id.
   * Does nothing, implemented since pure virtual in the base class.
   */
  int GetGroupId();

  /**
   * Get the expected size of the payload for a command.
   * Does nothing, implemented since pure virtual in the base class.
   * @param cmd       command id stripped py the parameter
   * @param parameter the parameter (16 lsb of the command header)
   * @return number of expected 32 bit words
   */
  int GetPayloadSize(__u32 cmd, __u32 parameter);

  /**
   * Handler for binary commands for a SM.
   * @see CEIssueHandler::issue for parameters and return values.
   */
  int issue(__u32 cmd, __u32 parameter, const char* pData, int iDataSize, CEResultBuffer& rb);

  /**
   * Handler for high level commands for a SM.
   * @see CEIssueHandler::HighLevelHandler for parameters and return values.
   */
  int HighLevelHandler(const char* pCommand, CEResultBuffer& rb);
private:
  /** instance of the device */
  CEDevice* fDevice;
};

#endif //__DEVICE_HPP
