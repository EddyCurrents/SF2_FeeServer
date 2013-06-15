// $Id: ce_phos.h,v 1.10 2007/06/26 14:01:04 richter Exp $

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
#ifndef __CE_PHOS_H
#define __CE_PHOS_H

#include "dev_fec.hpp"
#include "branchlayout.hpp"
#include "RCU_ControlEngine.hpp"
#include "issuehandler.hpp"

class PHOSCommandHandler;

/**
 * @defgroup rcu_ce_phos PHOS specific modules of the CE
 * The group is intended to contain all PHOS specific implementations. 
 * The PHOS module must be enabled by the configure option <b>--enable-phos</b>.
 *
 * The PHOS customization includes
 * - the PHOSagent defines the CE to be run by the framework
 * - the PHOSControlEngine is a modified RCUControlEngine which sets the
 * - PHOSBranchLayout, the definition of the layout
 * - the CEfecPHOS, the customized CEfec device
 *
 * @ingroup feesrv_ce
 */

#ifdef PHOS
/**
 * Number of APD channels per FEC
 * @ingroup rcu_ce_phos
 */
#define numberOfAPDchannels 32

/**
 * @class PHOSagent
 * This is the imlementation of the agent for the PHOS ControlEngine.
 * It creates a CE of type PHOSControlEngine.
 *
 * @ingroup rcu_ce_phos
 */
class PHOSagent : public CEagent
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
 * @class PHOSControlEngine
 * The PHOS specific customization of the RCUControlEngine.
 * The main pupose is to set the correct branch layout for PHOS through
 * an object of PHOSBranchLayout.
 *
 * @ingroup rcu_ce_phos
 */
class PHOSControlEngine : public RCUControlEngine 
{
 public:
  PHOSControlEngine();
  ~PHOSControlEngine();

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

  /** instance of the command handler */
  PHOSCommandHandler* fpCH;
};

/**
 * @class PHOSCommandHandler
 * This is a CEIssueHandler for the PHOS detector. It is automatically
 * registered and the issue request is redirected to the
 * PHOSControlEngine.
 *
 * @ingroup rcu_ce_phos
 */
class PHOSCommandHandler : public CEIssueHandler
{
 public:
  /** constructor */
  PHOSCommandHandler(PHOSControlEngine* pInstance=NULL);

  /** destructor */
  ~PHOSCommandHandler();

  /**
   * Get group id.
   */
  int GetGroupId();

  /**
   * Get the expected size of the payload for a command.
   * @param cmd       command id stripped py the parameter
   * @param parameter the parameter (16 lsb of the command header)
   * @return number of expected 32 bit words
   */
  int GetPayloadSize(__u32 cmd, __u32 parameter);

  /**
   * Issue handler for PHOS commands.
   * specific command translation for the PHOS Control Engine
   * @see CEIssueHandler::issue for parameters and return values.
   * @see rcu_issue.h for command ids and for further information
   */
  int issue(__u32 cmd, __u32 parameter, const char* pData, int iDataSize, CEResultBuffer& rb);

  /**
   * Handler for PHOS high level commands.
   * @see CEIssueHandler::HighLevelHandler for parameters and return values.
   */
  int HighLevelHandler(const char* pCommand, CEResultBuffer& rb);

private:
  /** instance of the device */
  PHOSControlEngine* fInstance;
};

/**
 * @class CEfecPHOS
 * The PHOS specific customization of the CEfec.
 * The class defines the default services for the FEC and adds in addition
 * services for all APDs. APD services are registered with both \em update
 * and \em set callback functions. This allows to set the value of an APD
 * via the @ref FEESVR_SET_FERO_DATA cammnd.
 *
 * @ingroup rcu_ce_phos
 */
class CEfecPHOS : public CEfec
{
 public:
  CEfecPHOS(int id=-1, CEDevice* pParent=NULL, std::string arguments="", std::string name="*FEC%02d");
  ~CEfecPHOS();

  /**
   * Get service description for PHOS FECs.
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
   * @ingroup rcu_ce_phos
   */
  int SetAPD(float value, int channelNo);

  /**
   * <i>Update</i> function for APD channels.
   * @ingroup rcu_ce_phos
   */
  int UpdateAPD(float &value, int channelNo);

 private:
  /** array of service descriptions */
  static const CEfec::service_t fServiceDesc[];
};

/**
 * @class PHOSBranchLayout
 * The PHOS specific branch layout CEfec.
 * The class sets 14 FECs in each branch and creates FEC devices of type
 * CEfecPHOS.<br>
 * Naming of the FEC devices (also the base name of all services) follows
 * the convention:<br>
 * <tt>FEC_\<branch\>\<position_in_branch\></tt> e.g. <tt>FEC_A01, FEC_B13</tt><br>
 * Service channels get names like <tt>FEC_A01_STATE, FEC_A01_APD_31</tt>
 *
 * @ingroup rcu_ce_phos
 */
class PHOSBranchLayout : public RcuBranchLayout {
public:
  PHOSBranchLayout();
  ~PHOSBranchLayout();
  
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
   * Create a PHOS FEC
   */
  CEfec* CreateFEC(int id, CEDevice* pParent);

 private:

};
#endif //PHOS

#endif //__CE_PHOS_H
