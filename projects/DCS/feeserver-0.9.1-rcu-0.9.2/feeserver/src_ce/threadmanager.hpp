// $Id: threadmanager.hpp,v 1.1 2006/04/13 10:32:08 richter Exp $

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

#ifndef __RCU_THREADMANAGER_HPP
#define __RCU_THREADMANAGER_HPP

/**
 * @class CEThreadManager
 * @ingroup rcu_ce_base
 */
class CEThreadManager {
public:
  CEThreadManager();
  ~CEThreadManager();

  /**
   * Lock the manager in its current state
   */
  int Lock();

  /**
   * Unlock the manager
   */
  int Unlock();

private:

};

/**
 * @class CELockGuard
 * @ingroup rcu_ce_base
 */
class CELockGuard {
public:
  CELockGuard(CEThreadManager* thrM);
  ~CELockGuard();

private:
  CEThreadManager* fpManager;
};

#endif //__RCU_THREADMANAGER_HPP
