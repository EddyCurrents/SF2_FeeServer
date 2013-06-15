// $Id: device.cpp,v 1.18 2007/09/01 22:49:58 richter Exp $

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

#include <vector>
#include <cerrno>
#include <cstring>
#include <cstdio>
#include "device.hpp"
#include "ce_base.h"
#include "rcu_issue.h"

using namespace std;

CEDevice::CEDevice(std::string name, int id,  CEDevice* pParent, std::string arguments) 
{
  string devname=name;
  string format=name;
  // look for a format specifier
  int fkey=name.find('%', 0);

  // check whether the service basename should contain the device name, if preceeded by
  // '*' the part between between '*' and '%' is used as device name.
  if (name[0]=='*') {
    int num=name.length()-1;
    if (fkey!=string::npos) num=fkey-1;
    devname.clear();
    if (devname.capacity()<num) devname.resize(num+1, 0);
    name.copy((char*)&devname[0], num, 1);
  }
  if (fkey!=string::npos && id>=0) {
    if (name[0]=='*') {
      int num=name.length()-fkey;
      format.clear();
      if (format.capacity()<num) format.resize(num+1, 0);
      name.copy((char*)&format[0], num, fkey);
    }
    int size=format.length();
    int written=0;
    do {
      fServiceBaseName.clear();
      fServiceBaseName.resize(size);
      size=snprintf((char*)&fServiceBaseName[0], fServiceBaseName.capacity(), format.c_str(), id);
    } while (++size>fServiceBaseName.capacity());
  }

  InitDevice(this, devname);
  fId=id;
  fpParent=pParent;
  fArguments=arguments;
  fCurrentIdx=-1;
  fSubDevices.clear();

  fpCH=new DeviceCommandHandler(this);
}

CEDevice::~CEDevice() {
  if (fpCH) delete fpCH;
  fpCH=NULL;
}

int CEDevice::SetDeviceId(int id)
{
  if (fId>=0) {
    CE_Warning("Overriding device Id %d with %d for device %p (%s)\n", fId, id, this, GetName());
  }
  fId=id;
  return 0;
}

int CEDevice::SetParentDevice(CEDevice* pParent)
{
  if (fpParent!=0) {
    CE_Warning("Overriding parent %p with %p for device %p (%s)\n", fpParent, pParent, this, GetName());
  }
  fpParent=pParent;
  return 0;
}

int CEDevice::SetArguments(std::string arguments)
{
  fArguments=arguments;
  return 0;
}

int CEDevice::ArmorDevice() {
  int iResult=0;
  return iResult;
}

CEState CEDevice::EvaluateHardware() {
  CEState state=eStateUnknown;
  return state;
}

int CEDevice::SynchronizeSubDevices() {
  int iResult=0;
  CEDevice* pDevice=GetFirstSubDevice();
  while (pDevice) {
    int iLocal=pDevice->SynchronizeAll();
    if (iResult>=0) iResult=iLocal;
    pDevice=GetNextSubDevice();
  }
  return iResult;
}

int CEDevice::SynchronizeAll() {
  int iSub=SynchronizeSubDevices();
  int iResult=Synchronize();
  if (iResult==0) iResult=iSub;
  return iResult;
}

int  CEDevice::EnterStateOFF() {
  int iResult=0;
  return iResult;
}

int  CEDevice::LeaveStateOFF() {
  int iResult=0;
  return iResult;
}

int  CEDevice::EnterStateON() {
  int iResult=0;
  return iResult;
}

int  CEDevice::LeaveStateON() {
  int iResult=0;
  return iResult;
}

int  CEDevice::EnterStateCONFIGURED() {
  int iResult=0;
  return iResult;
}

int  CEDevice::LeaveStateCONFIGURED() {
  int iResult=0;
  return iResult;
}

int  CEDevice::EnterStateCONFIGURING() {
  int iResult=0;
  return iResult;
}

int  CEDevice::LeaveStateCONFIGURING() {
  int iResult=0;
  return iResult;
}

int  CEDevice::EnterStateRUNNING() {
  int iResult=0;
  return iResult;
}

int  CEDevice::LeaveStateRUNNING() {
  int iResult=0;
  return iResult;
}

int CEDevice::EnterStateERROR() {
  int iResult=0;
  return iResult;
}

int CEDevice::SwitchOn(int iParam, void* pParam) {
  int iResult=0;
  return iResult;
} 

int CEDevice::Shutdown(int iParam, void* pParam) {
  int iResult=0;
  return iResult;
} 

int CEDevice::Configure(int iParam, void* pParam) {
  int iResult=0;
  return iResult;
} 

int CEDevice::Reset(int iParam, void* pParam) {
  int iResult=0;
  return iResult;
} 

int CEDevice::Start(int iParam, void* pParam) {
  int iResult=0;
  return iResult;
} 

int CEDevice::Stop(int iParam, void* pParam) {
  int iResult=0;
  return iResult;
}

int CEDevice::AddSubDevice(CEDevice* pDevice) {
  int iResult=0;
  if (pDevice) {
    int id=pDevice->GetDeviceId();
    if (id>=0) {
      if (FindDevice(pDevice)==NULL &&
	  FindDevice(id)==NULL) {
	int oldSize=fSubDevices.size();
	if (fSubDevices.size()<=id) {
	  fSubDevices.resize(id+1, NULL);
	}
	if (fSubDevices[id]!=NULL && fSubDevices[id]!=pDevice) {
	  CE_Warning("overriding device %p (%s - %d) with %p (%s - %d)\n",
		     fSubDevices[id], fSubDevices[id]->GetName(), fSubDevices[id]->GetDeviceId(),
		     pDevice, pDevice->GetName(), id);
	}
	fSubDevices[id]=pDevice;
	CE_Debug("Device %p (%s : %d) added\n", pDevice, pDevice->GetName(), id);
      } else {
	CE_Error("Device %p (%s : %d) already exists\n", pDevice, pDevice->GetName(), id);
      }
    } else {
      CE_Error("sub device %p does not have an id\n", pDevice);
    }
  } else {
    iResult=-EINVAL;
  }
  return iResult;
}

int CEDevice::CleanupSubDevices() {
  int iResult=0;
  vector<CEDevice*>::iterator element=fSubDevices.begin();
  while (element!=fSubDevices.end()) {
    CEDevice* pDevice=*element;
    *element=NULL;
    CE_Debug("Device %p (%s : %d) deleted\n", pDevice, pDevice->GetName(), pDevice->GetDeviceId());
    try {
      delete pDevice;
    }
    catch (...) {
      CE_Error("could not delete sub device %p\n", pDevice);
    }
    element++;
  }
  return iResult;
}

CEDevice* CEDevice::FindDevice(CEDevice* pDevice)
{
  CEDevice* pFound=NULL;
  if (pDevice) {
    int id=pDevice->GetDeviceId();
    if (id>=0 && fSubDevices.size()>id) pFound=fSubDevices[id];
    if (pFound==NULL) {
      pFound=GetFirstSubDevice();
      while (pFound) {
	if (pFound==pDevice ||
	    pFound->GetDeviceId()==pDevice->GetDeviceId() && pDevice->GetDeviceId()>=0) break;
	pFound=GetNextSubDevice();
      }
    }
  }
  return pFound;
}

CEDevice* CEDevice::FindDevice(int id)
{
  CEDevice* pFound=NULL;
  if (id>=0 && fSubDevices.size()>id) pFound=fSubDevices[id];
  if (pFound==NULL) {
    pFound=GetFirstSubDevice();
    while (pFound) {
      if (pFound->GetDeviceId()==id) break;
      pFound=GetNextSubDevice();
    }
  }
  return pFound;
}

CEDevice* CEDevice::GetFirstSubDevice()
{
  fCurrentIdx=-1;
  return GetNextSubDevice();
}

CEDevice* CEDevice::GetNextSubDevice()
{
  CEDevice* pDevice=NULL;
  do {
    if (fSubDevices.size()>fCurrentIdx+1) {
      pDevice=fSubDevices[++fCurrentIdx];
    }
  } while (pDevice==NULL && fSubDevices.size()>fCurrentIdx+1);
  return pDevice;
}

const char* CEDevice::GetServiceBaseName()
{
  if (fServiceBaseName.length()>0) return fServiceBaseName.c_str();
  return GetName();
}

int CEDevice::SendActionToSubDevice(const char* target, const char* action)
{
  int iResult=-ENOENT;
  if (SubDevicesLocked()!=0) {
    CE_Warning("sub-devices locked for device %s\n", GetServiceBaseName());
    return -EACCES;
  }
  CEDevice* pDev=GetFirstSubDevice();
  while (pDev!=NULL && iResult==-ENOENT) {
    string name=pDev->GetServiceBaseName();
    //CE_Debug("compare device name \'%s\' with target \'%s\'\n", name.c_str(), target);
    iResult=name.compare(target)==0;
    if (iResult!=1) {
      // this is for backwad compatibility, we allow the send actions
      // to the STATE channel
      name+="_STATE";
      iResult=name.compare(target)==0;
    }
    if (iResult==1) {
      // TriggerTransition delivers -ENOENT if the transition was not found
      // think about this later
      /*iResult=*/pDev->TriggerTransition(action);
      break;
    }
    if ((iResult=pDev->SendActionToSubDevice(target, action))>=0) {
      break;
    }
    iResult=-ENOENT;
    pDev=GetNextSubDevice();
  }
  return iResult;
}

int CEDevice::SubDevicesLocked()
{
  return 0;
}

DeviceCommandHandler::DeviceCommandHandler(CEDevice* pInstance)
  :
  fDevice(pInstance)
{
}

DeviceCommandHandler::~DeviceCommandHandler()
{
}

int DeviceCommandHandler::GetGroupId()
{
  return -1;
}

int DeviceCommandHandler::GetPayloadSize(__u32 cmd, __u32 parameter)
{
  return 0;
}

int DeviceCommandHandler::HighLevelHandler(const char* pCommand, CEResultBuffer& rb)
{
  int iResult=0;
  __u32 cmd=0;
  int keySize=0;
  int len=strlen(pCommand);
  const char* pBuffer=pCommand;
  if (pCommand && fDevice) {
    //CE_Debug("device %s checks command %s\n", fDevice->GetServiceBaseName(), pCommand);
    if (strncmp(pCommand, "CE_GET_TRANSITIONS", keySize=strlen("CE_GET_TRANSITIONS"))==0)
      cmd=CE_GET_TRANSITIONS;
    else if (strncmp(pCommand, "CE_GET_STATENAME", keySize=strlen("CE_GET_STATENAME"))==0)
      cmd=CE_GET_STATENAME;
    else if (strncmp(pCommand, "CE_GET_STATES", keySize=strlen("CE_GET_STATES"))==0)
      cmd=CE_GET_STATES;
    else if (strncmp(pCommand, "CE_GET_STATE", keySize=strlen("CE_GET_STATE"))==0)
      cmd=CE_GET_STATE;
    else if (strncmp(pCommand, "CE_TRIGGER_TRANSITION", keySize=strlen("CE_TRIGGER_TRANSITION"))==0)
      cmd=CE_TRIGGER_TRANSITION;

    if (cmd>0 && keySize>0) {
      pBuffer+=keySize;
      len-=keySize;
      while (*pBuffer==' ' && *pBuffer!=0 && len>0) {pBuffer++; len--;}
      iResult=(issue(cmd,  len, pBuffer, len, rb))>0;
      if (iResult>0) iResult=1;
      else iResult=-EEXIST;
    }
  }
  return iResult;
}

int DeviceCommandHandler::issue(__u32 cmd, __u32 parameter, const char* pData, int iDataSize, CEResultBuffer& rb)
{
  int iResult=0;
  // check arguments
  if (pData==NULL || iDataSize<parameter) return 0;

  // check instance
  if (fDevice==NULL) return 0;

  // check target
  string arguments;
  string target(pData, parameter);
  int blank=target.find(' ');
  if (blank!=string::npos) {
    arguments.assign(target, blank+1, parameter-blank-1);
    target.assign(pData, blank);
  }
  
  CE_Debug("device %s: checking target %s\n", fDevice->GetServiceBaseName(), target.c_str());

  if (target.compare(fDevice->GetServiceBaseName())!=0) return 0;

  switch (cmd) {
  case CE_GET_STATES:
    {
      string states=fDevice->GetStates();
      int len=states.length()+1;
      int size=len/sizeof(CEResultBuffer::value_type);
	if (len%sizeof(CEResultBuffer::value_type)>0) size++;
      rb.resize(size);
      strcpy((char*)&rb[0], states.c_str());
    }
    break;
  case CE_GET_TRANSITIONS:
    {
      string transitions=fDevice->GetTransitions(arguments.compare("ALL")==0);
      int len=transitions.length()+1;
      int size=len/sizeof(CEResultBuffer::value_type);
	if (len%sizeof(CEResultBuffer::value_type)>0) size++;
      rb.resize(size);
      strcpy((char*)&rb[0], transitions.c_str());
    }
    break;
  case CE_GET_STATE:
    {
      rb.resize(sizeof(__u32)/sizeof(CEResultBuffer::value_type));
      if (rb.size()==0) rb.resize(sizeof(__u32)/sizeof(CEResultBuffer::value_type));
      *((__u32*)&rb[0])=(__u32)fDevice->GetTranslatedState();
    }
    break;
  case CE_GET_STATENAME:
    {
      string name=fDevice->GetCurrentStateName();
      int len=name.length()+1;
      int size=len/sizeof(CEResultBuffer::value_type);
	if (len%sizeof(CEResultBuffer::value_type)>0) size++;
      rb.resize(size);
      strcpy((char*)&rb[0], name.c_str());
    }
    break;
  case CE_TRIGGER_TRANSITION:
    if ((iResult=fDevice->TriggerTransition(arguments.c_str())>=0))
      iResult=parameter;
    break;
  default:
    CE_Warning("unknown command id %#x\n", cmd);
    iResult=-ENOSYS;
  }
  if (iResult>=0)
    iResult=parameter;
  return iResult;
}
