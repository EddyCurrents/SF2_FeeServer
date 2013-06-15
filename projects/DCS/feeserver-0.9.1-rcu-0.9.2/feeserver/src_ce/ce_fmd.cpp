// $Id: ce_fmd.cpp,v 1.1 2007/06/12 09:00:29 richter Exp $

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

/***************************************************************************
 * ce_fmd.c
 * this file implements the specific methods for the FMD
 */

#ifdef FMD
#include <cerrno>
#include "ce_fmd.hpp"
#include "ce_base.h"
#include "dev_rcu.hpp"

using namespace std;

/*******************************************************************************
 *
 *                           service callbacks
 *
 ******************************************************************************/


/*******************************************************************************
 *
 *                           FMDagent
 *
 ******************************************************************************/

const char* FMDagent::GetName()
{
  return "FMD ControlEngine";
}

ControlEngine* FMDagent::CreateCE()
{
  return new FMDControlEngine;
}

/** the global control engine instance */
FMDagent g_FMDagent;

FMDControlEngine::FMDControlEngine()
{
  fpBranchLayout=NULL;
}

/*******************************************************************************
 *
 *                           FMDControlEngine implementation
 *
 ******************************************************************************/

FMDControlEngine::~FMDControlEngine()
{
}

int FMDControlEngine::InitCE()
{
  int iResult=0;
  fpBranchLayout=new FMDBranchLayout;
  if (fpBranchLayout) {
    SetRcuBranchLayout(fpBranchLayout);
  } else {
    iResult=-ENOMEM;
  }
  return iResult;
}

int FMDControlEngine::DeinitCE()
{
  int iResult=0;
  if (fpBranchLayout) {
    SetRcuBranchLayout(NULL);
    delete fpBranchLayout;
    fpBranchLayout=NULL;
  }
  return iResult;
}

/*******************************************************************************
 *
 *                           CEfecFMD implementation
 *
 ******************************************************************************/

CEfecFMD::CEfecFMD(int id, CEDevice* pParent, std::string arguments, std::string name)
  : CEfec(id, pParent, arguments, name)
{
}

CEfecFMD::~CEfecFMD()
{
}

const CEfec::service_t CEfecFMD::fServiceDesc[]={
  // add services here, see CEfecTPC::fServiceDesc in ce_tpc.cpp
  {"TEMP",    6, 0.25  , 0.5  , 40  ,  50, 0}
};

int CEfecFMD::ArmorDevice()
{
  int iResult=0;
  // add more service registrations here if the default handling is
  // not sufficient. Otherwise simply delete the whole method from the
  // class
  iResult=CEfec::ArmorDevice();
  return iResult;
}

int CEfecFMD::GetServiceDescription(const CEfec::service_t*  &pArray) const
{
  pArray=fServiceDesc;
  return sizeof(fServiceDesc)/sizeof(CEfec::service_t);
}

/*******************************************************************************
 *
 *                           FMDBranchLayout
 *
 ******************************************************************************/

FMDBranchLayout::FMDBranchLayout()
{
}

FMDBranchLayout::~FMDBranchLayout()
{
}

int FMDBranchLayout::GetNofFecBranchA()
{
  return 0;
}

int FMDBranchLayout::GetNofFecBranchB()
{
  return 0;
}

CEfec* FMDBranchLayout::CreateFEC(int id, CEDevice* pParent)
{
  return new CEfecFMD(id, pParent);
}

#endif //FMD
