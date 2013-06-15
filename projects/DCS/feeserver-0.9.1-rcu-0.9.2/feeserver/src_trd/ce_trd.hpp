// $Id: ce_trd.hpp,v 1.4 2007/08/20 21:53:45 richter Exp $

/************************************************************************
**
**
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
#ifndef __CE_TRD_H
#define __CE_TRD_H

#include "controlengine.hpp"

struct ceServiceData_t;

/**
 * @defgroup ce_trd TRD specific modules of the CE
 * The group is intended to contain all TRD specific implementations. 
 * Currently it serves as an example for the CE++ framework.
 *
 * The TRD module must be enabled by the configure option <b>--enable-trd</b>,
 * this switch automatically disables the RCUControlEngine.
 *
 * The TRD customization includes
 * - the TRDagent defines the CE to be run by the framework
 * - the TRDControlEngine derives from ControlEngine and is currently a sample
 *   customization
 *
 * @ingroup feesrv_ce
 */

#ifdef TRD

/**
 * @class TRDagent
 * This is the imlementation of the agent for the TRD ControlEngine.
 * It creates a CE of type TRDControlEngine.
 *
 * @ingroup ce_trd
 */
class TRDagent : public CEagent
{
private:
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
 * @class TRDControlEngine
 * The TRD specific customization of the ControlEngine.
 *
 * This is a sample implementation of a simple ControlEngine child, which has
 * currently not much to do with the TRD FeeServer. It's just an example.
 * 
 * It implements the following functions (in the sequence how they are called
 * by the framework):
 * - @ref InitCE() is the place to do customizations for the creation of the
 *   state machine 
 * - @ref ArmorDevice() is the device specific handler function called from
 * - @ref EvaluateHardware() must be implemented to tell the state machine
 *   where to start.
 *   @ref CEStateMachine::Armor() in order to start the state machine.
 * - @ref DeinitCE() is the place for clean-up
 *
 * Two sample services are registered in the @ref ArmorDevice() method, a float
 * and an int type service with the corresponding callback functions @ref
 * TRDsetService and @ref TRDupdateService. The 3 optional parameters are used
 * in the following way:
 * - <tt>major</tt>: service id
 * - <tt>minor</tt>: type of the service channel (int/float)
 * - <tt>parameter</tt>: pointer to the control engine instance
 *
 * The pointer to the instance is used to redirect to @ref WriteServiceValue
 * and @ref ReadServiceValue.
 *
 * @ingroup ce_trd
 */
class TRDControlEngine : public ControlEngine 
{
public:
  TRDControlEngine();
  ~TRDControlEngine();

  /**
   * Write a service value.
   * This sample function writes a log message and the value to the
   * internal variable.<br>
   * The \em set function can be invoked be sending a FEESVR_SET_FERO_DATA32
   * to the feeserver, e.g.
   * <pre>
   * echo '<fee>FEESVR_SET_FERO_DATA32 MAIN_SERVICE1 0</fee>' | bin/feeserver-ctrl --server server-name
   * </pre>
   */
  int WriteServiceValue(ceServiceData_t* pData, int major, int type);

  /**
   * Read a service value.
   * As an example, the function just increments the value of the service.
   */
  int ReadServiceValue(ceServiceData_t* pData, int major, int type);

protected:
  /**
   * Internal function called during the @ref CEStateMachine::Armor procedure.
   * The function is called from the @ref CEStateMachine base class. and carries
   * out specific start-up tasks.
   */
  int ArmorDevice();  

private:
  /**
   * Evaluate the state of the hardware.
   * The function does not change any internal data, it performes a number of evaluation
   * operations and mus return a proper state.
   * @return             state
   */
  CEState EvaluateHardware();

  /**
   * Init the ControlEngine.
   * The initialization function of the CE implementation. Called by the
   * framework at start of the CE.
   */
  int InitCE();

  /**
   * Deinit the ControlEngine.
   * The initialization function of the CE implementation.
   */
  int DeinitCE();

  /** value for the service 1 */
  float fService1;

  /** value for the service 2 */
  int fService2;
  
};
#endif //TRD

#endif //__CE_TRD_H
