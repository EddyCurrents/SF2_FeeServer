// $Id: ce_tpc.h,v 1.13 2007/07/04 14:57:20 richter Exp $

/************************************************************************
**
**
** This file is property of and copyright by the Experimental Nuclear 
** Physics Group, Dep. of Physics and Technology
** University of Bergen, Norway, 2004
** This file has been written by Matthias Richter,
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
#ifndef __CE_TPC_H
#define __CE_TPC_H

#include "dev_fec.hpp"
#include "branchlayout.hpp"
#include "RCU_ControlEngine.hpp"

/**
 * @defgroup rcu_ce_tpc TPC specific modules of the CE
 * The group is intended to contain all TPC specific implementations.
 * The TPC module must be enabled by the configure option <b>--enable-tpc</b>. 
 * 
 * The TPC customization includes
 * - the TPCagent defines the CE to be run by the framework
 * - the TPCControlEngine is a modified RCUControlEngine which sets the
 * - TPCBranchLayout, the definition of the layout
 * - the CEfecTPC, the customized CEfec device
 *
 * @author Matthias Richter
 * @ingroup feesrv_ce
 */

#ifdef TPC
/**
 * @class TPCagent
 * This is the imlementation of the agent for the TPC ControlEngine.
 * It creates a CE of type TPCControlEngine.
 *
 * @ingroup rcu_ce_tpc
 */
class TPCagent : public CEagent
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
 * @class TPCControlEngine
 * The TPC specific customization of the RCUControlEngine.
 * The main pupose is to set the correct branch layout for TPC through
 * an object of TPCBranchLayout.
 *
 * @ingroup rcu_ce_tpc
 */
class TPCControlEngine : public RCUControlEngine 
{
 public:
  TPCControlEngine();
  ~TPCControlEngine();

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
 * @class CEfecTPC
 * The TPC specific customization of the CEfec.
 * The class defines the default services for the FEC through the @ref
 * CEfecTPC::fServiceDesc array.
 *
 * <pre>
 * The standard services per TPC FEC are:
 * =================================================================================================
 * 	     	     
 * reg def.conf    name      description                    deadband  bitwidth conversion  unit
 * -------------------------------------------------------------------------------------------------
 *  1    off     "T_TH"    temperature threshold              0.5        10      0.25       oC
 *  2    off     "AV_TH"   analog voltage threshold           0.5        10      0.0043     V
 *  3    off     "AC_TH"   analog current threshold           0.5        10      0.017      A
 *  4    off     "DV_TH"   digital voltage threshold          0.5        10      0.0043     V
 *  5    off     "DC_TH"   digital current threshold          0.5        10      0.03       A
 *  6    on      "TEMP"    temperature                        0.5        10      0.25       oC
 *  7    on      "AV"      analog voltage                     0.5        10      0.0043     V
 *  8    on      "AC"      analog current                     0.5        10      0.017      A
 *  9    on      "DV"      digital voltage                    0.5        10      0.0043     V
 * 10    on      "DC"      digital current                    0.5        10      0.03       A
 * 11    off     "L1CNT"   level 1 trigger count              0.5        16      1.0
 * 12    off     "L2CNT"   level 2 trigger count              0.5        16      1.0
 * 13    off     "SCLKCNT"                                    0.5        16      1.0
 * 14    off     "DSTBCNT"                                    0.5         8      1.0
 * 15    off     "TSMWORD"                                    0.5         9      1.0
 * 16    off     "USRATIO"                                    0.5        16      1.0
 * 17    off     "CSR0"    BC control & status register 0     0.5        11      1.0
 * 18    off     "CSR1"    BC control & status register 1     0.5        14      1.0
 * 19    off     "CSR2"    BC control & status register 2     0.5        16      1.0
 * 10    off     "CSR3"    BC control & status register 3     0.5        16      1.0
 * </pre>
 *
 * A service with state 'no link' (FEC inactive, no access) is set to -2000 (@ref
 * CE_FSRV_NOLINK).<br>
 *
 * @ingroup rcu_ce_tpc
 */
class CEfecTPC : public CEfec
{
 public:
  CEfecTPC(int id=-1, CEDevice* pParent=NULL, std::string arguments="");
  ~CEfecTPC();

  /**
   * Get service description for TPC FECs.
   * @param pArray       target variable to get the pointer to description array
   * @return number of service descriptions
   */
  int GetServiceDescription(const CEfec::service_t*  &pArray) const;
 private:
  /** array of service descriptions */
  static const CEfec::service_t fServiceDesc[];
};

/**
 * @class TPCBranchLayout
 * The TPC specific branch layout CEfec.
 * The class leaves the definition of the branch layout to the RcuBranchLayout
 * class which scans the environment variable \b FEESERVER_FEC_MONITOR. 
 * FECs devices are added in slots according to this variable.
 * The TPCBranchLayout creates FEC devices of type CEfecTPC.<br>
 * Naming of the FEC devices (also the base name of all services) follows
 * the standard convention <tt>FEC_\<id\></tt>, where the prefix <tt>FEC_</tt>
 * is skipped for all service base names.
 * Service channels get names like <tt>01_STATE, 05_TEMP</tt>
 *
 * @ingroup rcu_ce_tpc
 */
class TPCBranchLayout : public RcuBranchLayout {
public:
  TPCBranchLayout();
  ~TPCBranchLayout();
  
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
   * Get the branch layout.
   * @param array     target to receive the array
   * @return size of the array
   */
  int GetBranchLayout(int* &array);

  /**
   * Create a TPC FEC
   */
  CEfec* CreateFEC(int id, CEDevice* pParent);

 private:
  /**
   * Set branch layout from server name
   */
  int SetBranchLayoutFromServerName();
  
  /** no of FECs in branch A, evaluated from the server name */
  int fNofA;

  /** no of FECs in branch B, evaluated from the server name */
  int fNofB;

  /** size of layout definition array */
  int fLayoutSize;

  /** the complex FEC layout array */
  int* fLayout;
};

#endif //TPC

#endif //__CE_TPC_H
