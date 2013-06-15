// $Id: dev_fec.hpp,v 1.7 2007/07/25 13:17:40 richter Exp $

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

#ifndef __DEV_FEC_HPP
#define __DEV_FEC_HPP

// the CEfec class is only used in conjunction with the CErcu device
#ifdef RCU

#include <vector>
#include <ctime>
#include "dimdevice.hpp"
#include "lockguard.hpp"
#include "ce_base.h"

/**
 * @class CEfec
 * Device implementation base class for FECs.
 * This is the base class for FEC devices. It controls the status of the FEC.
 * <br>
 * 
 * @section ce_fec_states States
 * Strictly, there is no need for a sophisticated state machine, but the
 * device framework provides it anyhow.
 * The scheme follows the standard States and Transitions of the @ref
 * CEStateEngine, with the following relevant states<br>
 * <b>OFF:</b>
 * <br>
 * - switching the device off means also physical power down of the FEC
 *
 * <b>ON:</b>
 * <br>
 *
 * <b>CONFIGURING:</b>
 * <br>
 *
 * <b>CONFIGURED:</b>
 * - not yet used
 * <br>
 *
 * <b>ERROR:</b>
 * <br>
 * - FEC's are turned to state \b ERROR if a value exceeds the specified range
 * - the FEC is powered down
 *
 * <b>FAILURE:</b>
 * <br>
 * - FEC's are turned to state \b FAILURE if there was a repeated access
 * error
 * - the FEC is not powered down
 *
 * @section ce_fec_services Services
 * The CEfec allows handling of standard services by just defining a service description
 * containing the relevant information (@ref CEfec::service_t)
 * - the BordController register
 * - a default deadband
 * - a conversion factor
 * - the allowed range of values
 *
 * The service description is defined by an overloaded virtual function @ref
 * GetServiceDescription. <br>
 * The services depend on the BC implementation and vary for the different
 * detectors. See CEfecTPC for an example.
 *
 * @ingroup rcu_ce
 */
class CEfec : public CEDimDevice {
public:
  CEfec(int id=-1, CEDevice* pParent=NULL, std::string arguments="", std::string name="*FEC%02d");
  virtual ~CEfec();

  /**
   * Update a service.
   * This function is called from the update callback.
   * @param pData
   * @param reg
   * @param type
   */
  int UpdateFecService(TceServiceData* pData, int id, int reg);
private:
  /**
   * Evaluate the state of the hardware.
   * The function does not change any internal data, it performes a number of evaluation
   * operations.
   * @return             state
   */
  CEState EvaluateHardware();

protected:
  /**
   * Internal function called during the @ref CEStateMachine::Armor procedure.
   * The function is called from the @ref CEStateMachine base class and adds the
   * default services defined by the specific @ref GetServiceDescription function.
   * The method can be overloaded to customize the services, if default services
   * are desired call @ref CEfec::ArmorDevice() from the overriding function.
   */
  virtual int ArmorDevice();

private:
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
   * Handler called when state machine changes to state RUNNING
   * The function can be implemented to execute specific functions when entering
   * state ERROR.
   * @return            neg. error code if failed
   */
  int EnterStateERROR();

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
   * Service descriptor for FEC services.
   * This structure describes the monitoring services of a FEC. The data points are
   * accessed via the RCU Slow Control module interface, which adapts transparently
   * to either the 5 or 8 bit version.<br>
   * The member \em conversionFactor determines whether the data point should be
   * published as float or int (=0.0).<br>
   * A range of valid values can be specified with the members \em max and \em min.
   * As soon as the value exceeds the range, the FEC is turned to error state and
   * physically switched off. No check is applied if \em min > \em max.
   */
  struct service_t {
    /** name */
    const char* name;
    /** register no */
    int regNo;
    /** conversion factor for float channels, int channel if 0 */
    float conversionFactor;
    /** deadband for float channels */
    float deadband;
    /** default value */
    float defaultVal;
    /** max value */
    float max;
    /** min value */
    float min;
  };

protected:
  /**
   * Get Service Description for FEC.
   * @param pArray       target variable to get the pointer to description array
   * @return number of services in array
   */
  virtual int GetServiceDescription(const CEfec::service_t*  &pArray) const;

  /**
   * Init additional services beyond the default ones.
   * The function can be overloaded in order to add more services to the FEC
   * which are not covered by the standard access function. Services can be
   * added by using @ref RegisterService @ref RegisterServiceGroup.
   * @return  neg. error code if failed
   */
  virtual int InitServices();
private:
  /** service list */
  std::vector<service_t> fListServices;

  /** register update error  */
  int fUpdateError;

  /** register update error counter  */
  int fUpdateErrorCount;

  /** time of last access failure */
  time_t fTimeAccessFailure;

  /** time of first occurrence of range excess */
  std::vector<time_t> fTimesRangeExcess;
};

#endif //RCU

#endif //__DEV_FEC_HPP
