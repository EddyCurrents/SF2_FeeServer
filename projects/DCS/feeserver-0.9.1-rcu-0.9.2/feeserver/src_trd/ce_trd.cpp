// $Id: ce_trd.cpp,v 1.3 2007/08/20 21:53:45 richter Exp $

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
 * ce_trd.c
 * this file implements the specific methods for the TRD
 */

#ifdef TRD
#include <cerrno>
#include <string>
#include "ce_trd.hpp"
#include "ce_base.h"

using namespace std;

/*******************************************************************************
 *
 *                           service callbacks
 *
 ******************************************************************************/

/**
 * A sample <i>Set</i> callback function.
 * The 'set' function is of type
 * int (*ceSetFeeValue)(TceServiceData* pData, int major, int minor, void* parameter);
 *
 * The function pointer is passed to the CE service framework during service
 * registration. The function handler is called, Whenever a 
 * @ref FEESVR_SET_FERO_DATA command for the service is received, 
 * @see ceSetFeeValue
 * @see rcu_ce_base_services
 * @ingroup ce_trd
 */
int TRDsetService(TceServiceData* pData, int major, int type, void* parameter) 
{
  int iResult=0;
  if (pData) {
    TRDControlEngine* pCE=(TRDControlEngine*)parameter;
    if (pCE) {
      pCE->WriteServiceValue(pData, major, type);
    }
  } else {
    iResult=-EINVAL;
  }
  return iResult;
}

/**
 * <i>Update</i> callback function TRD CE services.
 * The 'update' function is of type 
 * <pre>
 * int (*ceUpdateService)(TceServiceData* pData, int major, int minor, void* parameter)
 * </pre>
 *
 * The function pointer is passed to the CE service framework during service
 * registration. The function handler is called, whenever an update of the value of the
 * published data is requested. This is done periodically by the framework.
 * @see ceSetFeeValue
 * @see rcu_ce_base_services
 * @ingroup ce_trd
 */
int TRDupdateService(TceServiceData* pData, int major, int type, void* parameter) 
{
  int iResult=0;
  if (pData) {
    TRDControlEngine* pCE=(TRDControlEngine*)parameter;
    if (pCE) {
      switch (type) {
      case eDataTypeFloat:
	pCE->ReadServiceValue(pData, major, type);
	break;
      case eDataTypeInt:
	pCE->ReadServiceValue(pData, major, type);
	break;
      }
    }
  } else {
    iResult=-EINVAL;
  }
  return iResult;
}

/*******************************************************************************
 *
 *                           TRDagent
 *
 ******************************************************************************/

const char* TRDagent::GetName()
{
  return "TRD ControlEngine";
}

ControlEngine* TRDagent::CreateCE()
{
  return new TRDControlEngine;
}

/** the global agent instance */
TRDagent g_TRDagent;


/*******************************************************************************
 *
 *                           TRDControlEngine implementation
 *
 ******************************************************************************/

TRDControlEngine::TRDControlEngine()
  :
  fService1(CE_FSRV_NOLINK),
  fService2(CE_ISRV_NOLINK)
{
}

TRDControlEngine::~TRDControlEngine()
{
}

int TRDControlEngine::InitCE()
{
  int iResult=0;
  return iResult;
}

int TRDControlEngine::ArmorDevice()
{
  int iResult=0;

  // add some additional dummy services
  int serviceID=0;

  string name=GetServiceBaseName();
  name+="_SERVICE1";
  RegisterService(eDataTypeFloat, name.c_str(), 50.0, TRDupdateService, TRDsetService, serviceID++, eDataTypeFloat, this);

  name=GetServiceBaseName();
  name+="_SERVICE2";
  RegisterService(eDataTypeInt, name.c_str(), 0.0, TRDupdateService, TRDsetService, serviceID++, eDataTypeInt, this);

  return iResult;
}

CEState TRDControlEngine::EvaluateHardware() {
  CEState state=GetCurrentState();
  // we start in state OFF
  // The check can also involve evaluation of sub-devices
  // and whatever this state might depend on
  if (state==eStateUnknown) state=eStateOff;
  return state;
}

int TRDControlEngine::DeinitCE()
{
  int iResult=0;
  // add cleanup here
  return iResult;
}

int TRDControlEngine::WriteServiceValue(TceServiceData* pData, int major, int type)
{
  // implement the hardware access here
  if (pData) {
    switch (type) {
    case eDataTypeFloat:
      CE_Info("set float service value for TRD CE: service %d value %f\n", major, pData->fVal);
      if (major==0) fService1=pData->fVal;
      break;
    case eDataTypeInt:
      CE_Info("set int service value for TRD CE: service %d value %#x\n", major, pData->iVal);
      // Note: the frame work currently passes the value to set only in the fVal member
      // regardless of the actual type. This will be corrected in the future
      if (major==1) fService2=(int)pData->fVal;
      break;
    }
    CE_Warning("hardware access not yet implemented\n");
  }
  return 0;
}

int TRDControlEngine::ReadServiceValue(TceServiceData* pData, int major, int type)
{
  CEState states[]={eStateOn, eStateConfiguring, eStateConfigured, eStateRunning, eStateUnknown};
  // remove the comment in the if statement if you want to update depending
  // on the state
  if (1/*Check(states)*/) {
    // implement the hardware access here, this is just some dummy
    // update.
    switch (type) {
    case eDataTypeFloat:
      {
	// if it is service 1 (major 0) we want to use the internal variable
	// otherwize we increment the value directly
	float* pRef=&(pData->fVal); // use a reference
	if (major==0 /* service 1 */) pRef=&fService1;
	if (*pRef>1000 || *pRef<0) *pRef=major; 
	else *pRef+=1;
	CE_Debug("major %d *pRef=%f pData->fVal=%f fService1=%f\n", major, *pRef, pData->fVal, fService1);
	pData->fVal=*pRef;
      }
      break;
    case eDataTypeInt:
      {
	// if it is service 2 (major 1) we want to use the internal variable
	// otherwize we increment the value directly
	int* pRef=&(pData->iVal); // use a reference
	if (major==1 /* service 2 */) pRef=&fService2;
	if (*pRef>1000 || *pRef<0) *pRef=major; 
	else *pRef+=1;
	pData->iVal=*pRef;
      }
      break;
    }
  } else {
    switch (type) {
    case eDataTypeFloat:
      // non active float services are set to -2000
      pData->fVal=CE_FSRV_NOLINK;
      break;
    case eDataTypeInt:
      // non active int services are set to 0xffffffff
      pData->iVal=CE_ISRV_NOLINK;
      break;
    }
  }
  return 0;
}

#endif //TRD
