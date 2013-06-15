// $Id: branchlayout.hpp,v 1.4 2007/07/09 16:36:47 richter Exp $

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

#ifndef __RCUBRANCHLAYOUT_HPP
#define __RCUBRANCHLAYOUT_HPP

// the CEfec class is only used in conjunction with the CErcu device
#ifdef RCU

#include "vector"

class CEDevice;
class CEfec;

/**
 * @class RcuBranchLayout
 * This class defines the branch layout/ FEC slot occupation of a certain
 * setup. The branch layout can be set to the @ref CErcu in order to
 * create the FECs appropriately.
 */
class RcuBranchLayout {
public:
  RcuBranchLayout();
  virtual ~RcuBranchLayout();
  
  /**
   * Get the number of FECs in branch A.
   * @return no of FECs
   */
  virtual int GetNofFecBranchA();

  /**
   * Get the number of FECs in branch B.
   * @return no of FECs
   */
  virtual int GetNofFecBranchB();

  /**
   * Get the branch layout.
   * This function allows to define a complex layout of the branch
   * slot occupation. It is called if both @ref GetNofFecBranchA and
   * @ref GetNofFecBranchB return -1 (default implementation).<br>
   *
   * The implementation of the base class reads the FEC configuration from
   * the system variable FEESERVER_FEC_MONITOR which is interpreted as a
   * string of subsequent '0' and '1' each indicating a not valid/valid card,
   * missing digets at the and are treated as '0'.
   * @param array     target to receive the array
   * @return size of the array
   */
  virtual int GetBranchLayout(int* &array);

  /**
   * Create a FEC.
   * This function creates the desired type of a FEC.
   * @param id        id of the sub device
   * @param pParent   parent device
   */
  virtual CEfec* CreateFEC(int id, CEDevice* pParent)=0;

private:
  /** layout array, read from FEESERVER_FEC_MONITOR */
  std::vector<int> fLayout;
};
#endif //RCU

#endif //__RCUBRANCHLAYOUT_HPP
