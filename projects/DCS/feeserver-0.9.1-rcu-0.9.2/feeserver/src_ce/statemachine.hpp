// $Id: statemachine.hpp,v 1.14 2007/09/01 17:00:02 richter Exp $

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

#ifndef __RCU_STATEMACHINE_HPP
#define __RCU_STATEMACHINE_HPP

#include <string>
#include <vector>
#include "threadmanager.hpp"

/** @defgroup rcu_ce_base_states The CE State Machine Framework
 * The State Machine Framework consists of a CEStateMachine base class
 * and derived CEDevice base classes. For the sake of simplicity, the
 * handling of states is 'hardwired'. The SM comes with a few default
 * states and a couple of user defined states, see @ref CEState.
 * Transitions are defined to switch from one state into another.<br>
 *
 * While all the basic functionality is implemented in CEStateMachine, the
 * handler functions are defined by CEDevice. All handler functions are
 * virtual and can be overloaded by the device implementation. 
 * The CEDevice provides an \em EnterState and a \em LeaveState handler
 * for each state. In addition there is a handler for each transition.
 * - LeaveStateXXX: is called each time when the state is left, before
 *                  the invocation of the transition handler. 
 *                  e.g. @ref CEDevice::LeaveStateOFF
 * - Transition handler are called in between the state handlers
 *                  e.g. @ref CEDevice::SwitchOn, @ref CEDevice::Start
 * - EnterStateXXX: is called each time when the state is entered after
 *                  the transition handler. 
 *                  e.g. @ref CEDevice::EnterStateON
 *
 * All default States and Transitions have names and defined behavior. The
 * final state of a transition is fixed, but not the initial states. A
 * couple of virtual functions of CEStateMachine define the behavior and
 * can be overloaded for customization:
 * - @ref CEStateMachine::IsAllowedTransition
 * - @ref CEStateMachine::GetDefaultTransitionName
 *
 * In addition to the default ones, transitions can be added to a state
 * machine with the @ref CEStateMachine::AddTransition() function. 
 *\b Note: The id of a new transition must be outside the range of the
 * default transition (see @ref CETransitionId).
 *
 * Names and Ids of states can be customized by a translation scheme,
 * see CEStateMapper.
 * @ingroup rcu_ce_base
 */

class CEDevice;
class CETransition;
class CEStateMachine;
class CEStateMapper;

/**
 * the default states of the device.
 * @ingroup rcu_ce_base_states
 */
typedef enum {
  /** the current state is unknown */
  eStateUnknown     = -1,
  /** special id for termination of arrays */
  eStateInvalid     = -1,
  /** device is off */
  eStateOff         = 0,
  /** error state, left via <tt>shutdown</tt> or <tt>reset</tt> */
  eStateError       = 0x1,
  /** soft error or error in sub-device, left via <tt>notify</tt> into the previous state*/
  eStateFailure     = 0x2,
  /** device is on */
  eStateOn          = 0x10,
  /** device is configuring */
  eStateConfiguring = 0x20,
  /** device is configured */
  eStateConfigured  = 0x40,
  /** device is running */
  eStateRunning     = 0x80,
  /** user defined state 1 */
  eStateUser0       = 0x100,
  /** user defined state 2 */
  eStateUser1       = 0x110,
  /** user defined state 3 */
  eStateUser2       = 0x120,
  /** user defined state 4 */
  eStateUser3       = 0x130,
  /** user defined state 5 */
  eStateUser4       = 0x140,
} CEState;

/**
 * the default transitions of the device.
 * @ingroup rcu_ce_base_states
 */
typedef enum {
  /** transition is unknown */
  eTransitionUnknown     = 0,
  /** switch the device on */
  eSwitchOn,
  /** switch device off */
  eShutdown,
  /** configure the device */
  eConfigure,
  /** signal configuration done */
  eConfigureDone,
  /** rest the device */
  eReset,
  /** start device */
  eStart,
  /** stop the device */
  eStop,
  /** notify action to confirm the FAILURE state */
  eNotify,
  /** last default transition id */
  eLastDefaultTransition
} CETransitionId;

/**
 * @class CEStateMachine
 * The State Machine class.
 * The State Machine class provides default states and default transitions. The
 * names of the default states and transitions can be adapted by defining a
 * CEStateMapper scheme and/or overriding the function @ref
 * GetDefaultTransitionName.<br>
 * \b Note: The method @ref GetDefaultStateName is depricated, a CEStateMapper
 * should be used instead. 
 * <br>
 * The default behaviour is like:
 * <pre>
 *              -----
 *             | OFF | <-------------------------------
 *              -----                                  |
 *               | |                                   |
 *    switchon   | ^  shutdown                         ^  shutdown
 *               | |                                   |
 *              ----                                -------
 *             | ON | <-----------------<- reset --| ERROR |
 *              ----    |             |             -------
 *               |      |             |              can be reached by any state or
 *    configure  |    reset           ^              result of transition	      
 *               |      |             |              left by action 'reset' or
 *     -------------    |             |              'shutdown'
 *    | CONFIGURING |   ^             |
 *     -------------    |             |                    ^
 *               |      |             |-<- reset ----      |
 *               |      |             |              |  shutdown
 *               |      |             ^              |     |
 *               |      |             |              |     |
 *            ------------            \             ---------
 *           | CONFIGURED | <--------- notify -----| FAILURE |
 *            ------------            /             ---------
 *               | |                  |              can be reached by any state or 
 *        start  | ^  stop            |              result of transition
 *               | |                  |              left by action 'notify' to the
 *            ---------               |              previous state, or 'reset' and
 *           | RUNNING | <------------               'shutdown' like the ERROR state
 *            ---------
 * </pre>
 * The CEDevice class implements handlers for the entering and leaving a state
 * and the transition handlers for default transitions. The
 * @ref DefaultTransitionDispatcher is a switch for the default transitions. The
 * concept of default states and transitions was introduced to simplify
 * implementation of devices and to save resources. It is foreseen to extend the
 * default states and transition dynamically. 
 * <b>Note:</b> The FAILURE state and <i>notify</i> handling is forseen but not
 * implemented since it is not needed in the beginning.
 *
 * @ingroup rcu_ce_base_states
 */
class CEStateMachine : public CEThreadManager {
public:
  CEStateMachine();
  ~CEStateMachine();

  /**
   * This is the external signal to start the state engine.
   * 
   */
  int Armor();

  /**
   * Get the name of the device/state machine
   * @return name as const char array
   */
  const char* GetName();

  /**
   * Check if the device is in a certain state
   * @param state        state to check
   * @return             1 if yes, 0 if not
   */
  int Check(CEState state);

  /**
   * Check if the device is in one of a list of state
   * @param states        array of states to check, terminated by eStateUnknown
   *                      element
   * @return             1 if yes, 0 if not
   */
  int Check(CEState states[]);

  /**
   * Get the current state.
   */
  CEState GetCurrentState();

  /**
   * Get the current state in a translated encoding.
   * A translation scheme can be defined by the @ref SetTranslationScheme
   * method. If no state is specified, the current state will be returned.
   * @param state        state to query
   * @return             translated state if scheme was defined <br>
   *                     @ref GetCurrentName if no scheme defined
   */
  int GetTranslatedState(CEState state=eStateUnknown);

  /**
   * Set a translation scheme to present states in an alternative encoding.
   * An object of type CEStateMapper defines the translation/mapping between
   * internal states and published state, including both ids and names
   * @param pMapper      a CEStateMapper object defining the translation
   * @return             neg. error code if failed
   */
  int SetTranslationScheme(CEStateMapper* pMapper);

  /**
   * Get the translation scheme.
   * @param pMapper      a CEStateMapper object defining the translation
   * @return             neg. error code if failed
   */
  CEStateMapper* GetTranslationScheme();

  /**
   * Get name of the current state.
   * If a @ref CEStateMapper is defined, this will determine the name of the state.
   */
  const char* GetCurrentStateName();

  /**
   * Start a transition.
   * If the device is in the right state for the desired action, the transition
   * dispatcher is called.
   * @param action       char string specifying the action
   * @param iMode        mode flags
   * @param iParam       arbitrary integer parameter passed to dispatcher/handler
   * @param pParam       arbitrary void pointer passed to dispatcher/handler
   * @return             >=0 success, neg error code if failed
   *   -EPERM            wrong state, transition not permittded
   *   -ENOENT           transition not found
   *   -EBUSY            device is busy
   */
  int TriggerTransition(const char* action, int iMode=0, int iParam=0, void* pParam=NULL);

  /**
   * Start a transition.
   * If the device is in the right state for the desired action, the transition
   * dispatcher is called.
   * @param transition   id for the transition to be executed
   * @param iMode        mode flags
   * @param iParam       arbitrary integer parameter passed to dispatcher/handler
   * @param pParam       arbitrary void pointer passed to dispatcher/handler
   * @return             >=0 success, neg error code if failed
   *   -EPERM            wrong state, transition not permittded
   *   -ENOENT           transition not found
   *   -EBUSY            device is busy
   */
  int TriggerTransition(CETransitionId transition, int iMode=0, int iParam=0, void* pParam=NULL);

  /**
   * Start a transition.
   * If the device is in the right state for the desired action, the transition
   * dispatcher is called.
   * @param pTrans       transition to be executed
   * @param iMode        mode flags
   * @param iParam       arbitrary integer parameter passed to dispatcher/handler
   * @param pParam       arbitrary void pointer passed to dispatcher/handler
   * @return             >=0 success, neg error code if failed
   *   -EPERM            wrong state, transition not permittded
   *   -ENOENT           transition not found
   *   -EBUSY            device is busy
   */
  int TriggerTransition(CETransition* pTrans, int iMode=0, int iParam=0, void* pParam=NULL);

  /**
   * Synchronize state machine with the hardware
   * Performe a number of evaluation functions to dtermine the ste of the
   * hardware device and set the state machine according to that.
   * @return             >=0 success, neg error code if failed
   */
  int Synchronize();

  /**
   * Update the state channel created by @ref CreateStateChannel.
   */
  virtual int UpdateStateChannel();

  /**
   * Get the first available transition after specified id.
   * The function returns the next transition after the transition
   * with the specified id.
   * @param id           id of preceeding transition
   * @return pointer to transition instance
   */
  CETransition* GetNextTransition(CETransitionId id=eTransitionUnknown);

  /**
   * Get available transitions.
   * Return a string with the available transitions for the current state
   * or all transitions.
   * @param bAll         all transitions
   * @return string object
   */
  std::string GetTransitions(int bAll=0);
 
  /**
   * Get states.
   * Return a string with the states.
   * @return string object
   */
  std::string GetStates();

  /**
   * Set log level.
   * This is a log level for notifications on state changes, by default
   * it is eDebug and those messages are suppressed.
   */
  void SetLogLevel(int level) {fLogLevel=level;}

protected:
  /** 
   * init the device instance for this state machine.
   * The device instance is initialized from the constructor of the CEDevice
   * class and is used to call handler methods for default transitions.
   * @param pDevice      pointer to device 
   * @return             0 if succeeded, neg error code if failed
   */
  int InitDevice(CEDevice* pDevice, std::string name="");

  /** 
   * create a state channel for this state machine.
   * The function will be called at initialisation and
   * can be overloaded in order to create a 'channel' (whatever it might be),
   * to publish the state trough it. E.g. a DIM channel. 
   * The default implementation does not forsee a channel.
   */
  virtual int CreateStateChannel();

  /** 
   * create an alarm channel for this state machine.
   * The function will be called at initialisation and
   * can be overloaded in order to create a 'channel' (whatever it might be),
   * to publish the alarm trough it. E.g. a DIM channel. 
   * The default implementation does not forsee a channel.
   */
  virtual int CreateAlarmChannel();

  /** 
   * Send an alarm.
   * @param alarm        alarm id
   */
  virtual int SendAlarm(int alarm);

  /**
   * check if the state is one of the default states.
   * @param state        the state to check
   * @return             1 if default state, 0 if not
   */
  static int IsDefaultState(CEState state);

  /**
   * check if the state is a valid state.
   * @param state        the state to check
   * @return             1 if default state, 0 if not
   */
  int IsValidState(CEState state);

  /**
   * check if the transition is one of the default transitions.
   * @param transition   the id of the transition to check
   * @return             1 if default transition, 0 if not
   */
  int IsDefaultTransition(CETransitionId id);
  
  /**
   * Find the transition descriptor for an action.
   * @param action       the name of the action to find a transition for
   * @return             pointer to transition descriptor
   */
  CETransition* FindTransition(const char* action);

  /**
   * Find the transition descriptor for an id.
   * @param id           the id to find a transition for
   * @return             pointer to transition descriptor
   */
  CETransition* FindTransition(CETransitionId id);

  /**
   * Check if the transition is allowed for the current state.
   * See definition of @ref CEStateMachine::fDefTrans for the description of
   * the default transitions.
   * @param transition   pointer to transition descriptor
   * @return             1 if yes, 0 if not, neg. error code if failed
   */
  virtual int IsAllowedTransition(CETransition* pT);

  /**
   * dispatch default transitions to the handler functions.
   * All handler functions for default transitions are virtual functions of the
   * class @ref CEDevice which can be overloaded.
   * @param transition   the transition to be called
   * @param iParam       integer parameter passed from the caller of the action
   * @param pParam       void pointer passed from the caller of the action
   * @return             state after the transition
   */
  CEState DefaultTransitionDispatcher(CETransition* transition, int iParam, void* pParam);

  /**
   * dispatch <i>enter-state</i> events to the handler functions.
   * @param state        state for which the handler has to be called
   * @return             neg. error code is failed
   */
  int StateEnterDispatcher(CEState state);

  /**
   * dispatch <i>enter-state</i> events to the handler functions of default
   * states.
   * @param state        state for which the handler has to be called
   * @return             neg. error code is failed
   */
  int DefaultStateEnterDispatcher(CEState state);

  /**
   * dispatch <i>leave-state</i> events to the handler functions.
   * @param state        state for which the handler has to be called
   * @return             neg. error code is failed
   */
  int StateLeaveDispatcher(CEState state);

  /**
   * dispatch <i>leave-state</i> events to the handler functions for default
   * states.
   * @param state        state for which the handler has to be called
   * @return             neg. error code is failed
   */
  int DefaultStateLeaveDispatcher(CEState state);

  /**
   * dispatch transitions to the handler functions.
   * This function must be overloaded if additional transitions have been added.
   * <b>Note:</b>For future extension.
   * @param transition   the transition to be called
   * @param pDevice      pointer to device instance for this state machine
   * @param iParam       integer parameter passed from the caller of the action
   * @param pParam       void pointer passed from the caller of the action
   * @return             state after the transition
   */
  virtual CEState TransitionDispatcher(CETransition* transition, CEDevice* pDevice, int iParam, void* pParam);

  /**
   * Get the name of a state.
   * The function is a switch between the default states and the dynamic state
   * handling.
   * @param state        the state to check
   * @return             pointer to name
   */
  const char* GetStateName(CEState state);

  /**
   * get the name of a default state.
   * Can be overridden in order to change the default name.
   * @param state        the state to check
   * @return             pointer to name
   */
  virtual const char* GetDefaultStateName(CEState state);

  /**
   * Get the name of a state.
   * The function is a switch between the default transitions and the dynamic
   * transition handling.
   * @param pTrans       the transition to check
   * @return             pointer to name
   */
  const char* GetTransitionName(CETransition* pTrans);

  /**
   * Get the name of a default transition.
   * Can be overridden in order to change the default name. The name of the
   * transition corresponds one to one to the name of the <i>Action</i> which
   * triggers the transition.
   * @param transition   the id of the transition to check
   * @return             pointer to name
   */
  virtual const char* GetDefaultTransitionName(CETransition* pTransition);

  /**
   * Change the state to the ERROR state.
   */
  int SetErrorState();

  /**
   * Change the state to the FAILURE state.
   */
  int SetFailureState();

  /**
   * Set intermediate state.
   */
  int SetIntermediateState(CEState state);

  /** 
   * Add a transition to the list.
   * The transition object must be created dynamically with \em new. The
   * cleanup is done by the state machine. Example:
   * <pre>
   *  CETransition* t1=new CETransition(100, eStateUser0, "GO_USER0", eStateOn);
   *  AddTransition(t1);
   * </pre>
   * Creates a transition from @ref eStateOn to @ref eStateUser0 with name
   * \em GO_USER0.
   * \see CETransition for a parameter description.
   */
  int AddTransition(CETransition* pTransition);

  /**
   * Guard for temporary states.
   * The guard can be used to switch temporarily from one state to another
   * and back again when the processing is finnished and the guard gets
   * out of scope.
   *
   * \b IMPORTANT: Never create dynamic objects from the guard.
   *
   * Usage:
   * <pre>
   * MySmartFunction()
   * {
   *   StateSwitchGuard g(this, eStateUser3);
   *   // do my smart stuff
   * }
   * </pre>
   */
  class StateSwitchGuard {
  public:
    StateSwitchGuard(CEStateMachine* pInstance, CEState tmpState);
    ~StateSwitchGuard();
  private:
    /** constructor prohibited */
    StateSwitchGuard();
    /** copy constructor prohibited */
    StateSwitchGuard(const StateSwitchGuard&);
    /** copy operator prohibited */
    StateSwitchGuard& operator=(const StateSwitchGuard&);

    /** the original state */
    CEState fOriginal;

    /** instance of the state machine */
    CEStateMachine* fpInstance;
  };

private:
  /**
   * Change from to current state to another one.
   * The LeaveStateXXXX and EnterStateXXXX handlers are called.<br>
   * \b Note: No transition handler is called.
   * @param state        the state to change to
   */ 
  int ChangeState(CEState state);

  /** the device instance for this state machine */
  CEDevice* fpDevice;
  /** state variable */
  CEState fState;
  /** name of the state machine */
  std::string fName;
  /** the mapper to translate to another state encoding */
  CEStateMapper* fpMapper;

  /** the list of transitions */
  std::vector<CETransition*> fTransitions;
  /** delete all transition descriptors in the list */
  int CleanupTransitionList();

  /** default transition descriptors */
  static CETransition fDefTrans[eLastDefaultTransition];

  /** logging level for state notifications*/
  int fLogLevel;
};

/**
 * @class CETransition
 * The class hosts all parameters which are neccesary for a transition.
 * In order to simplify the handling of the default transitions and to safe
 * memory, there are static dummy descriptors defined in the @ref StateMachine
 * for the default transitions. Both naming and the function callbacks are
 * handled directly via member functions of the @ref CEDevice.<br>
 * The concept is intended to allow the definition of additional transitions.
 * This is currently not implemented.
 * @ingroup rcu_ce_base_states
 */
class CETransition : public CEThreadManager {
public:
  friend class CEStateMachine;

  /** constructor */
  CETransition();
  /** constructor with initialization */
  CETransition(CETransitionId id, 
	       CEState finalState=eStateUnknown, 
	       const char* name="", 
	       CEState initialState1=eStateUnknown, 
	       CEState initialState2=eStateUnknown, 
	       CEState initialState3=eStateUnknown, 
	       CEState initialState4=eStateUnknown, 
	       CEState initialState5=eStateUnknown);

  /** constructor with initialization */
  CETransition(int id, 
	       CEState finalState=eStateUnknown, 
	       const char* name="", 
	       CEState initialState1=eStateUnknown, 
	       CEState initialState2=eStateUnknown, 
	       CEState initialState3=eStateUnknown, 
	       CEState initialState4=eStateUnknown, 
	       CEState initialState5=eStateUnknown);

  /** copy contructor */
  CETransition(const CETransition& src);

  /** destructor */
  ~CETransition();

  /**
   * Get the transition id.
   * @return             id of the transition
   */
  CETransitionId GetID();

  /**
   * Get the transition name.
   * @return             name of the transition
   */
  const char* GetName();

  /**
   * check if the transition is one of the default transitions.
   * @param transition   the id of the transition to check
   * @return             1 if default transition, 0 if not
   */
  int IsDefaultTransition();

  /**
   * Check if the transition is allowd for a certain state.
   * The function checks if the specified state is one of the initial states.
   * If there were no initial states defined for the transition the return
   * value is 1.
   * @param state        the state to check   
   * @return             1 if yes, 0 if not, neg. error code if failed
   */
  virtual int IsInitialState(CEState state);

  /**
   * Get the final state of this transition
   * @return final state with successful transition
   */
  CEState GetFinalState();

  CETransition& operator=(CETransition& src);

  int operator==(const char* action);

  int operator==(CETransitionId id);

  int operator==(int id);

  int operator>(CETransitionId id);

  int operator>(int id);

  int operator>=(CETransitionId id);

  int operator>=(int id);

protected:
  /**
   * Handler function.
   * The method is called by the framework if the transition is executed.
   * The parameters are the same a specified to the @ref 
   * CEStateMachine::TriggerTransition method.
   * @param pDevice      device instance
   * @param iParam       general purpose int paramater
   * @param pParam       general purpose pointer paramater
   */
  virtual int Handler(CEDevice* pDevice, int iParam, void* pParam);

private:
  int SetAttributes(CETransitionId id, 
		    CEState finalState=eStateUnknown, 
		    const char* name="", 
		    CEState initialState1=eStateUnknown, 
		    CEState initialState2=eStateUnknown, 
		    CEState initialState3=eStateUnknown, 
		    CEState initialState4=eStateUnknown, 
		    CEState initialState5=eStateUnknown);

  /** id of this transition */
  CETransitionId fId;
  /** name of this transition */
  const char* fName;
  /** the final state of this transition */
  CEState fFinalState;
  /** the array of the states which the transition is allowed for */
  std::vector<CEState> fInitialStates;
};

/**
 * A simple base class to provide state code translation.
 * The mapper class can be used in two ways:
 * - if the state IDs of the state should be different from the default IDs
 *   specified in @ref CEState you can implement the @ref GetMappedState and
 *   @ref GetStateFromMappedState
 * - if the names of the states are changed then the @ref GetMappedStateName
 *   method can be overloaded.
 * Of course they can also be combined. The translation scheme for a state
 * machine can be set by @ref CEStateMachine::SetTranslationScheme.
 * @ingroup rcu_ce_base_states
 */
class CEStateMapper {
public:
  CEStateMapper() {};
  ~CEStateMapper() {};

  /**
   * Get the mapped state from the CEState.
   * @param state         CEState
   * @return              translated state
   */
  virtual int GetMappedState(CEState state) {return state;}

  /**
   * Get the mapped state name from the CEState.
   * @param state         CEState
   * @return              translated state
   */
  virtual const char* GetMappedStateName(CEState state) {return NULL;}

  /**
   * Get the CEState from the mapped state.
   * @param state         CEState
   * @return              translated state
   */
  virtual CEState GetStateFromMappedState(int state) {return (CEState)state;}
};

#endif //__RCU_STATEMACHINE_HPP
