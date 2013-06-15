// $Id: ce_fmd.hpp,v 1.1 2007/06/12 09:00:29 richter Exp $

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
#ifndef __CE_FMD_H
#define __CE_FMD_H

#include "dev_fec.hpp"
#include "branchlayout.hpp"
#include "RCU_ControlEngine.hpp"


/**
 * @defgroup rcu_ce_fmd FMD specific modules of the CE
 * The group is intended to contain all FMD specific implementations. 
 * The FMD module must be enabled by the configure option <b>--enable-fmd</b>.
 *
 * The FMD customization includes
 * - the FMDagent defines the CE to be run by the framework
 * - the FMDControlEngine is a modified RCUControlEngine which sets the
 * - FMDBranchLayout, the definition of the layout
 * - the CEfecFMD, the customized CEfec device
 *
 * @ingroup feesrv_ce
 */

#ifdef FMD
/**
 * @class FMDagent
 * This is the imlementation of the agent for the FMD ControlEngine.
 * It creates a CE of type FMDControlEngine.
 *
 * @ingroup rcu_ce_fmd
 */
class FMDagent : public CEagent
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
 * @class FMDControlEngine
 * The FMD specific customization of the RCUControlEngine.
 * The main pupose is to set the correct branch layout for FMD through
 * an object of FMDBranchLayout.
 *
 * @ingroup rcu_ce_fmd
 */
class FMDControlEngine : public RCUControlEngine 
{
 public:
  FMDControlEngine();
  ~FMDControlEngine();

 private:
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
  
  /** instance of the branch layout */
  RcuBranchLayout* fpBranchLayout;
};

/**
 * @class CEfecFMD
 * The FMD specific customization of the CEfec.
 * The class defines the default services for the FEC.
 *
 * @ingroup rcu_ce_fmd
 */
class CEfecFMD : public CEfec
{
 public:
  CEfecFMD(int id=-1, CEDevice* pParent=NULL, std::string arguments="", std::string name="*FEC%02d");
  ~CEfecFMD();

  /**
   * Get service description for FMD FECs.
   * @param pArray       target variable to get the pointer to description array
   * @return number of service descriptions
   */
  int GetServiceDescription(const CEfec::service_t*  &pArray) const;

  /**
   * Internal function called during the @ref CEStateMachine::Armor procedure.
   * Specific implemetation of the @ref CEfec::ArmorDevice() function in order
   * to add more services.
   */
  int ArmorDevice();

  /**
   * <i>Set</i> function for APD channels.
   * The function handler is called, whenever a 
   * @ref FEESVR_SET_FERO_DATA command for the service is received, 
   * @see ceSetFeeValue
   * @see rcu_ce_base_services
   * @ingroup rcu_ce_fmd
   */
  int SetAPD(float value, int channelNo);

  /**
   * <i>Update</i> function for APD channels.
   * @ingroup rcu_ce_fmd
   */
  int UpdateAPD(float &value, int channelNo);

 private:
  /** array of service descriptions */
  static const CEfec::service_t fServiceDesc[];
};

/**
 * @class FMDBranchLayout
 * The FMD specific branch layout CEfec.
 * The branch layout specifies currently 0 FECs in both branches, this has
 * to be implemented by the detector people.
 *
 * @ingroup rcu_ce_fmd
 */
class FMDBranchLayout : public RcuBranchLayout {
public:
  FMDBranchLayout();
  ~FMDBranchLayout();
  
  /**
   * Get the number of FECs in branch A.
   * @return no of FECs
   */
  int GetNofFecBranchA();

  /**
   * Get the number of FECs in branch B.
   * @return no of FECs
   */
  int GetNofFecBranchB();

  /**
   * Create a FMD FEC
   */
  CEfec* CreateFEC(int id, CEDevice* pParent);

 private:

};
#endif //FMD

#endif //__CE_FMD_H
