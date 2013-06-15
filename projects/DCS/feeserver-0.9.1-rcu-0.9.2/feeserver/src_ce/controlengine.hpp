// $Id: controlengine.hpp,v 1.8 2007/08/22 22:26:26 richter Exp $

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

#ifndef __CONTROLENGINE_HPP
#define __CONTROLENGINE_HPP

#include "dimdevice.hpp"
#include "issuehandler.hpp"
#include "lockguard.hpp"

/**
 * @class ControlEngine
 * CE++ interface and base class for all ControlEngines.
 * This is the main interface of the C++ CE framework to
 * @ref feesrv_ceapi. The object oriented CE framework allows to implement
 * a control engine by creating a new class derived from ControlEngine. The
 * base class handles all entry points of the CE API. The ControlEngine is
 * a CEDimDevice, meaning that it automatically contains a state machine
 * and corresponding STATE and STATENAME channels. It controls the service
 * update and provides an entrypoint for CEIssueHandler command handlers.
 *
 * @section controlengine_statemachine State machine
 * The CE automatically initializes a state machine. The internal state ids
 * are according to the definition @ref CEState und the default state diagram is
 * according to the default CEStateMachine implementation.
 *
 * A few features of the state machine base class which allow to implement
 * customized devices/CEs: 
 * - The CEStateMachine base class has 5 <i>user defined</i> states. Those
 *   can be implemented in order to serve addidtional states.
 * - The default state diagram can be simply changed by defining the allowed
 *   transitions of a state, final states of transitions are though fixed
 * - The names and ids of states can be customized by providing a 
 *   CEStateMapper object.
 * - The names of actions/transitions can be changed by overloading a virtual
 *   function of the CEStatemachine
 *
 * The @ref CEDevice::EvaluateHardware() function \b must be implemented to
 * allow synchronization of the state machine with eventually underlying
 * hardware. At startup, @ref CEDevice::EvaluateHardware() is requested to
 * return a proper state to specify a starting point.
 *
 * Transitions between the different states are triggered by \em Actions.
 * Actions can be send to the main device by sending a string via the command
 * channel
 * <pre>
 * <action>MY_ACTION</action>
 * e.g. <action>START</action>
 * </pre>
 *
 * In general, \em Actions can be send to devices and sub-devices by
 * specification of the target.
 * <pre>
 * <action>MY_DEVICE MY_ACTION</action>
 * e.g. <action>00 SwitchOn</action>
 * </pre>
 * In the example the FEC #00 will be switched on.<br>
 * \b Note: Sub-devices might be locked, e.g. FECs can not be switched
 * individually if the MAIN device is in states CONFIGURED or RUNNING.
 *
 * @section controlengine_services Service handling
 * The ControlEngine automatically registers two services, <tt>MAIN_STATE</tt>
 * and <tt>MAIN_STATENAME</tt>, publishing its state.
 *
 * Additional service channels can be registered from e.g. the @ref InitCE()
 * function via the @ref rcu_ce_base_services. The registration requires
 * \em callback functions for the update. Additional custom devices can be
 * added as sub-devices of the CE and can register service channels in their
 * implementation of @ref CEDevice::ArmorDevice().
 *
 * The ControlEngine base class imlements the update loop, which deploys the
 * update \em callback functions.
 *
 * @section controlengine_commands Command handling
 * All command handling (the @ref issue entry of the @ref feesrv_ceapi) is
 * generalized to use CEIssueHandler command handlers. Such objects are
 * registered when they are created and are able to handle a certain group
 * of commands according of the implemented functionality. The CE loops over
 * all registered handlers.
 *
 * @section controlengine_customization Customization
 * A child class derived from ControlEngine describes a new CE. A dedicated
 * CEagent class and one global instance of it has to be added to handle the
 * creation of the customized CE.
 *
 * The child can implement a couple of virtual functions in order to
 * customize the CE:
 * - @ref  InitCE()
 *   The initialization function of the CE implementation. It can be 
 *   overloaded by the child to perform custom initializations, e.g. to add
 *   sub-devices.
 * - @ref ArmorDevice()
 *   Is the device specific handler function called at startup of th state
 *   machine.
 * - @ref DeinitCE()
 *   The cleanup function of the CE implementation. It can be overloaded
 *   by the child to perform custom clean up.
 * - @ref PreUpdate()
 *   Handler called before the service update loop.
 * - @ref PostUpdate()
 *   Handler called after the service update loop.
 *
 * See @ref ce_trd for a simple example.
 */
class ControlEngine : public CEDimDevice {
public:
  /**
   * Constructor
   * @param name           name of the device
   */
  ControlEngine(std::string name="MAIN");

  /**
   * D'tor
   */
  ~ControlEngine();

  /**
   * Main entry point of the CE.
   * Switch to the @ref RunCE of the instance.
   */
  static int Run();

  /**
   * Terminate the CE.
   * Switch to the @ref TerminateCE of the instance.
   */
  static int Terminate();

  /**
   * The command handling function.
   * The instance previously set will be used.
   */
  static int Issue(char* command, char** result, int* size);

  /**
   * Set the update rate.
   * Switch to the @ref SetUpdateRate of the instance.
   */
  static int SetUpdateRate(unsigned short millisec);
  
  /**
   * Main handler of actions received by the commend channel.
   * @param  pAction        string specifying the action
   * @return                neg. error code if failed
   */
  static int ActionHandler(const char* pAction);

  /**
   * action handler used internally.
   * @param  transition     transition id
   * @return                neg. error code if failed
   */
  static int ActionHandler(CETransitionId transition);

  /**
   * Main handler for high level commands.
   * Tries to find a CEIssueHandler which can handle the command.
   * @param pCommand        char buffer with command
   * @return >0 processed, 0 not processed, neg. error code if failed
   */
  static int HighLevelCommandHandler(const char* pCommand, CEResultBuffer& rb);

protected:
  /**
   * Set the instance of the active ControlEngine.
   * This function is called from the constructor. CEs can be instantiated
   * by adding a global object to the code which is than created 
   * automatically.
   * @param pInstance    pointer to ControlEngine
   */
  int SetInstance(ControlEngine* pInstance);

  /**
   * Reset the active instance if it is equal to the argument.
   * @param pInstance    pointer to ControlEngine
   */
  int ResetInstance(ControlEngine* pInstance);

private:
  /**
   * Main entry point of the CE.
   * 
   */
  int RunCE();

  /**
   * Terminate the CE.
   * The terminate handler sets the eTerminate processing flag in order to
   * indicate other threads (update and issue handlers) the pending
   * terminate action.
   * The instance previously set will be used.
   */
  int TerminateCE();

  /**
   * Set the update rate.
   */
  int SetUpdateRate(unsigned short sec, unsigned short usec);

  /**
   * Init the ControlEngine.
   * The initialization function of the CE implementation.
   */
  virtual int InitCE();

  /**
   * Deinit the ControlEngine.
   * The initialization function of the CE implementation.
   */
  virtual int DeinitCE();

  /**
   * Handler called before the service update loop.
   */
  virtual int PreUpdate();

  /**
   * Handler called after the service update loop.
   */
  virtual int PostUpdate();

  /**
   * Custom command handler.
   * The number of processed bytes is returned by the function in case of
   * success.<br>
   * If processing was successful but the processed size is smaller than the
   * incoming block, a CEIssueHandler is searched.
   * @return nof processed bytes if success, neg. error code if failure
   */
  virtual int PreIssue(char* command, int size, CEResultBuffer& rb);

  /**
   * High-level command handler.
   * The function scans the command block for high-level commands and actions.
   * The number of processed bytes is returned by the function in case of
   * success.
   * @return nof processed bytes if success, neg. error code if failure
   */
  int ScanHighLevelCommands(char* command, int size, CEResultBuffer& rb);

  /** the instance of the active ControlEngine (singleton)*/
  static ControlEngine* fpInstance;

  /**
   * Sleeping time in seconds.
   * The CE update loop sleeps a certain time between the service update function. 
   * The time is adapted to the FeeServer update rate in the 
   * @ref signalFeePropertyChanged API function. 
   * @ingroup feesrv_ce
   */
  short fSleepSec;

  /**
   * Sleeping time in micro seconds.
   * If the sleeping time is smaller than a second, this variable is used for a
   * sleep in micro seconds, ignored otherwise. <br>
   * The time is adapted to the FeeServer update rate in the 
   * @ref signalFeePropertyChanged API function. 
   * @ingroup feesrv_ce
   */
  short fSleepUsec;

  /**
   * Flags to indicate different stages of the processing.
   */
  enum ProcFlags {
    /** Update loop running */
    eUpdate = 0x1,
    /** Issue handler running */
    eIssue = 0x2,
    /** Terminate sequence pending */
    eTerminate = 0x4,
    /** Update thread is terminated */
    eUpdateTerminated = 0x8
  };

  /** the processing flags for thread sync */
  short fProcFlags;

  /** mutex */
  static CE_Mutex fMutex;
};

/**
 * @class CEagent
 * The CEagent manages the active instance of the ControlEngine.
 * This is a very simple and slim class which provides a method to create
 * a CE. An agent provides a name and a function to create an instance
 * of a ControlEngine child. The utilization of agents allows fully
 * modular development, there is no necessity to change or add a single
 * line in the CE framework in order to get a custom CE running<br>
 * Implementation of a custom CE requires to implement a CE class 
 * derived from ControlEngine and an agent class derived from CEagent.
 * In order to automatically register the agent, <b>one global object<b>
 * has to be added to the source code. <br>
 * It is possible to define default agents and non-default agents. If e.g.
 * a default implemetation should be always available as fall back, this
 * can be defined as \em default. If at the same time a \em non-default
 * agent is present, this overrides the default one. If only the two
 * required virtual methods are implemented by the child agent, it is 
 * automatically \em non-default.
 */
class CEagent
{
public:
  CEagent();
  ~CEagent();

  /**
   * Create the instance of the Control engine from the active agent
   * instance.
   */
  static ControlEngine* Create();

  /**
   * Get name of the active agent 
   */
  static const char* GetInstanceName();

protected:
  /**
   * Set the instance of the active ControlEngine.
   * This function is called from the constructor. CEs can be instantiated
   * by adding a global object to the code which is than created 
   * automatically.
   * @param pInstance    pointer to ControlEngine
   */
  int SetInstance(CEagent* pInstance);

  /**
   * Reset the active instance if it is equal to the argument.
   * @param pInstance    pointer to ControlEngine
   */
  int ResetInstance(CEagent* pInstance);

private:
  /**
   * Get the name of the agent.
   */
  virtual const char* GetName()=0;

  /**
   * Check whether this Agent makes a default CE.
   * The function returns 0 by default and can be overloaded by default
   * Agents. Non-default Agents are preferred over default ones, the first
   * non-default overrides the current default one.
   * @return 1 if default CE
   */
  virtual int IsDefaultAgent();

  /**
   * Create the instance of the Control engine.
   */
  virtual ControlEngine* CreateCE()=0;

  /** the active agent */
  static CEagent* fpInstance;
};

#endif //__CONTROLENGINE_HPP
