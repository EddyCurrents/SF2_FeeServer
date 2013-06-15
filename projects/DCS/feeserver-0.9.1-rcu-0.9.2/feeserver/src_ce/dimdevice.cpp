// $Id: dimdevice.cpp,v 1.7 2007/09/01 16:58:27 richter Exp $

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

#include <cerrno>
#include <cstring>
#include "dimdevice.hpp"
#include "ce_base.h"

using namespace std;

/**
 * Update the value of the 'STATE' service for the specific @ref CEDimDevice.
 * This function is registerd during service registration in
 * @ref CEDimDevice::CreateStateChannel.
 * @param pF         location to update
 * @param major      not used
 * @param minor      not used
 * @param parameter  pointer to device instance
 * @internal
 * @ingroup rcu_ce_base
 */
int updateDimDeviceState(TceServiceData* pData, int major, int minor, void* parameter) {
  int iResult=0;
  if (pData) {
    if (parameter) {
      switch (minor) {
      case eDataTypeFloat:
	pData->fVal=(float)((CEStateMachine*)parameter)->GetTranslatedState();
	break;
      case eDataTypeInt:
	pData->iVal=(int)((CEStateMachine*)parameter)->GetTranslatedState();
	break;
      case eDataTypeString:
	{
	  string* data=(string*)pData->strVal;
	  if (data) {
	    const char* name=((CEStateMachine*)parameter)->GetCurrentStateName();
	    if (name)
	      *data=name;
	    else
	      *data="INVALID";
	  }
	}
	break;
      }
    } else {
      CE_Error("missing parameter for function updateMainState, check service registration\n");
      iResult=-EFAULT;
    }
  } else {
    CE_Error("invalid location\n");
    iResult=-EINVAL;
  }
  return iResult;
}

int setDimDeviceState(TceServiceData* pData, int major, int minor, void* parameter)
{
  int iResult=0;
  if (pData) {
    if (parameter) {
      int switchOn=0;
      switch (minor) {
      case eDataTypeFloat:
	switchOn=pData->fVal>0.0;
	break;
      case eDataTypeInt:
	switchOn=pData->iVal>0;
	break;
      }
      CEStateMachine *sm=(CEStateMachine*)parameter;
      if (switchOn==0) {
	sm->TriggerTransition(eShutdown);
      } else if (sm->Check(eStateOff)) {
	sm->TriggerTransition(eSwitchOn);
      }
    } else {
      CE_Error("missing parameter for function updateMainState, check service registration\n");
      iResult=-EFAULT;
    }
  } else {
    CE_Error("invalid location\n");
    iResult=-EINVAL;
  }
  return iResult;
}


/**
 * Update the value of the 'ALARM' service for the @ref CEDimDevice.
 * This function is registerd during service registration in
 * @ref CEDimDevice::CreateAlarmChannel.
 * @param pF         location to update
 * @param major      not used
 * @param minor      not used
 * @param parameter  pointer to device instance
 * @internal
 * @ingroup rcu_ce_base
 */
int updateDimDeviceAlarm(TceServiceData* pData, int major, int minor, void* parameter) {
  int iResult=0;
  if (parameter) {
    iResult=((CEDimDevice*)parameter)->SendNextAlarm(pData);
  } else {
    CE_Error("missing parameter for function updateMainState, check service registration\n");
    iResult=-EFAULT;
  }
  return iResult;
}

CEDimDevice::CEDimDevice(std::string name, int id, CEDevice* pParent, std::string arguments) 
  : CEDevice(name, id, pParent, arguments)
{
  fLastSentAlarm=0;
}

CEDimDevice::~CEDimDevice() {
}

int CEDimDevice::SendNextAlarm(TceServiceData* pData) {
  int iResult=0;
  if (pData) {
    // TODO: alarm list handling
  } else {
    CE_Error("invalid location\n");
    iResult=-EINVAL;
  }
  return iResult;
}

int CEDimDevice::CreateStateChannel() {
  int iResult=0;
  string name=GetServiceBaseName();
  if (name.length()>0) name+="_";
  name+="STATE";
  iResult=RegisterService(eDataTypeFloat, (const char*)&name[0], 0.5, updateDimDeviceState, setDimDeviceState, 0, eDataTypeFloat, this);
  name=GetServiceBaseName();
  if (name.length()>0) name+="_";
  name+="STATENAME";
  iResult=RegisterService(eDataTypeString, (const char*)&name[0], 0.0, updateDimDeviceState, NULL, 0, eDataTypeString, this);
  return iResult;
}

int CEDimDevice::CreateAlarmChannel() {
  string name=GetServiceBaseName();
  if (name.length()>0) name+="_";
  name+="ALARM";
  return RegisterService(eDataTypeInt, (const char*)&name[0], 0.0, updateDimDeviceAlarm, NULL, 0, 0, this);
}

int CEDimDevice::SendAlarm(int alarm) {
  return 0;
}

