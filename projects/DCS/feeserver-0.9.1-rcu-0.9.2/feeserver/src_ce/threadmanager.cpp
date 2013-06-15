// $Id: threadmanager.cpp,v 1.1 2006/04/13 10:32:08 richter Exp $

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

#include "threadmanager.hpp"
#include "ce_base.h"

CEThreadManager::CEThreadManager() {
}

CEThreadManager::~CEThreadManager() {
}

int CEThreadManager::Lock() {
  int iResult=0;
  return iResult;
}

int CEThreadManager::Unlock() {
  int iResult=0;
  return iResult;
}

CELockGuard::CELockGuard(CEThreadManager* thrM)
{
  fpManager=thrM;
  if (fpManager) {
    fpManager->Lock();
  } else {
    CE_Fatal("invalid parameter to lock guard %p\n", this);
  }
}

CELockGuard::~CELockGuard()
{
  if (fpManager) {
    fpManager->Unlock();
  } else {
    CE_Fatal("uninitialized lock guard %p\n", this);
  }
}
