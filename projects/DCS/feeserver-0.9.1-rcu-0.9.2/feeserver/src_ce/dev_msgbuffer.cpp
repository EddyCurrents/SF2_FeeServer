// $Id: dev_msgbuffer.cpp,v 1.4 2007/09/03 20:01:48 richter Exp $

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

#include "dev_msgbuffer.hpp"
#include "dcscMsgBufferInterface.h"
#include "rcu_issue.h"
#include <cerrno>
#include <cstring>
#include <cstdio>

using namespace std;

extern "C" void ce_sleep(int sec);
extern "C" void ce_usleep(int usec);

void* threadCheckDriver(void* param) 
{
  __u32 data=0;
  rcuSingleRead(0, &data); // just a dummy check
  if (param) *(static_cast<int*>(param))=1;
  pthread_exit(NULL);
  return NULL;
}

DCSCMsgBuffer* DCSCMsgBuffer::fpInstance=NULL;
int DCSCMsgBuffer::fInstances=0;
int DCSCMsgBuffer::fResult=0;

DCSCMsgBuffer::DCSCMsgBuffer()
  : 
  CEDimDevice("MSGBUFFER"), 
  CEIssueHandler(),
  fpStateMapper(new MsgBufferStateMapper)
{
  // define additional transitions
  CETransition* pActivateFlash=new CETransition(eActivateFlash, eStateUser0, "", eStateUser1, eStateOn);
  CETransition* pActivateSM=new CETransition(eActivateSelectmap, eStateUser1, "", eStateUser0, eStateOn);
  AddTransition(pActivateFlash);
  AddTransition(pActivateSM);

  // set translation scheme
  SetTranslationScheme(fpStateMapper.get());
}

DCSCMsgBuffer::~DCSCMsgBuffer() 
{
  if (--fInstances==0 && fResult>=0) {
    Release();
  }
}

DCSCMsgBuffer* DCSCMsgBuffer::GetInstance()
{
  if (fpInstance==0) {
#ifndef RCUDUMMY
    fpInstance=new DCSCMsgBuffer;
#else
    CE_Info("using simulation of message buffer\n");
    fpInstance=new DCSCMsgBufferSim;
#endif
    if (fpInstance) {
      CE_Info("\n");
      if (fpInstance->Armor()>=0) {
	CE_Info("\n");
	fpInstance->Synchronize();
      }
    }
  }
  if (fpInstance) {
    fInstances++;
  }

  return fpInstance;
}

int DCSCMsgBuffer::ReleaseInstance(DCSCMsgBuffer* instance)
{
  int iResult=0;
  if (instance!=NULL) {
    if (instance==fpInstance) {
      if (--fInstances==0) {
	fpInstance=NULL;
	delete instance;
      }
    } else {
      iResult=-EBADFD;
    }
  } else {
    iResult=-EINVAL;
  }
}

int DCSCMsgBuffer::Release()
{
  int iResult=releaseRcuAccess();
  CE_Info("releaseRcuAccess finished with error code %d\n", iResult);
  return iResult;
}

int DCSCMsgBuffer::CheckResult()
{
  return fResult;
}

int DCSCMsgBuffer::CheckDriver()
{
  int iResult;
  pthread_t thread_check;
  int iCheckResult=0;
  if ((iResult=pthread_create(&thread_check, NULL, &threadCheckDriver, &iCheckResult))==0) {
    for (int count=0; count<10; count++) {
      ce_usleep(500000);
      if (iCheckResult!=0)
	break;

    }
    if (iCheckResult==0)
      iResult=-EACCES;
  } else {
    CE_Warning("can not initialize thread (%d)\n", iResult);
    iResult=-EFAULT;
  }
  return iResult;
}

//CE_Mutex DCSCMsgBuffer::fMutex;

int DCSCMsgBuffer::SingleWrite(__u32 address, __u32 data)
{
  // don't know yet if we should take it away for performance reasons
  CEState blackList[]={eStateError, eStateFailure, eStateInvalid};
  if (Check(blackList)) {
    return 0;
  }
  return rcuSingleWrite(address, data);
}

int DCSCMsgBuffer::SingleRead(__u32 address, __u32* pData)
{
  // don't know yet if we should take it away for performance reasons
  CEState blackList[]={eStateError, eStateFailure, eStateInvalid};
  if (Check(blackList)) {
    *pData=~((__u32)0);
    return 0;
  }
  return rcuSingleRead(address, pData);
}

int DCSCMsgBuffer::MultipleWrite(__u32 address, __u32* pData, int iSize, int iDataSize)
{
  // don't know yet if we should take it away for performance reasons
  CEState blackList[]={eStateError, eStateFailure, eStateInvalid};
  if (Check(blackList)) {
    return 0;
  }
  return rcuMultipleWrite(address, pData, iSize, iDataSize);
}

int DCSCMsgBuffer::MultipleRead(__u32 address, int iSize,__u32* pData)
{
  // don't know yet if we should take it away for performance reasons
  CEState blackList[]={eStateError, eStateFailure, eStateInvalid};
  if (Check(blackList)) {
    if (pData) {
      memset(pData, 0xff, iSize*sizeof(__u32));
    }
    return 0;
  }
  return rcuMultipleRead(address, iSize,pData);
}

/************************************************************************************
 *
 *    state machine related methods
 *
 ************************************************************************************/
int DCSCMsgBuffer::ArmorDevice()
{
  if ((fResult=initRcuAccess(NULL))>=0) {
    unsigned int dcscDbgOptions=0;
    dcscDbgOptions|=PRINT_RESULT_HUMAN_READABLE;
    setDebugOptions(dcscDbgOptions);
    CE_Info("checking driver\n");
    if ((fResult=fpInstance->CheckDriver())<0) {
      if (fResult==-EACCES) {
	CE_Warning("access failure, driver possibly locked, reseting and trying again\n");
	dcscLockCtrl(eDeactivateLock);
	fResult=fpInstance->CheckDriver();
      }
    }
    if (fResult<0) {
      CE_Error("message buffer interface not accessible\n");
      fpInstance->Release();
    }
  } else {
    CE_Error("initRcuAccess finished with error code %d\n", fResult);
    fResult=-EACCES;
  }
  return fResult;
}

CEState DCSCMsgBuffer::EvaluateHardware()
{
  CEState state=GetCurrentState();
  if (state==eStateUnknown) {
    if (rcuBusControlCmd(eCheckMsgBuf)) {
      state=eStateOn;    // MSGBUFFER
    } else if (rcuBusControlCmd(eCheckFlash)) {
      state=eStateUser0; // FLASH
    } else if (rcuBusControlCmd(eCheckSelectmap)) {
      state=eStateUser1; // SELECTMAP
    } else {
      CE_Warning("MsgBuffer device in unknown state, switch to mode 'msg buffer'\n");
      rcuBusControlCmd(eEnableMsgBuf);
      state=eStateOn;    // MSGBUFFER
    }
  }
  return state;
}

const char* DCSCMsgBuffer::GetDefaultTransitionName(CETransition* pTransition)
{
  // disable all default names except the ones handled below explicitely
  const char* name="";
  if (pTransition) {
    if (pTransition->IsDefaultTransition()) {
      switch (pTransition->GetID()) {
      case eSwitchOn:
	// if we want to have the transition to be externally available we have to
	// give it a name. Currently transition of this class are only for internal
	// use.
	//name="ActivateMsgBuffer";
	break;
      case eReset:
	name=pTransition->GetName();
	break;
      }
    }
  } else {
    name="invalid";
  }
  return name;
}

int DCSCMsgBuffer::IsAllowedTransition(CETransition* pT) {
  int iResult=0;
  if (pT) {
    CEState current=GetCurrentState();
    int id=pT->GetID();
    switch (id) {
    case eConfigure:
    case eReset:
    case eStart:
    case eStop:
      // none of them allowed
      break;
    case eSwitchOn:
      // the ActivateMsgBuffer transition 
      iResult|=(current==eStateUser0); // FLASH
      iResult|=(current==eStateUser1); // SELECTMAP
      break;
    default:
      // forward to the default method
      iResult=CEStateMachine::IsAllowedTransition(pT);
    }
    CE_Debug("checking whether transition %d (%s) is allowed for state %d (%s): %s\n", id, GetTransitionName(pT), current, GetCurrentStateName(), iResult==1?"yes":"no");
  } else {
    iResult=-EINVAL;
  }
  return iResult;
}

const char* MsgBufferStateMapper::GetMappedStateName(CEState state)
{
  const char* name=NULL;
  // most og the default states for this device are disbled and do not get a name
  switch (state) {
  case eStateOff:         name="";                break;
  case eStateOn:          name="MSGBUFFER";       break;
  case eStateConfiguring: name="";                break;
  case eStateConfigured:  name="";                break;
  case eStateRunning:     name="";                break;
  case eStateUser0:       name="FLASH";           break;
  case eStateUser1:       name="SELECTMAP";       break;
  }
  return name;
}

/************************************************************************************
 *
 *    issue handler related methods
 *
 ************************************************************************************/
int DCSCMsgBuffer::GetGroupId() 
{
  return FEESVR_DCSC_CMD;
}

int DCSCMsgBuffer::issue(__u32 cmd, __u32 parameter, const char* pData, int iDataSize, CEResultBuffer& rb)
{
  int iResult=0;
  int iProcessed=0;
  CE_Debug("translateDcscCommand cmd=%#x parameter=%#x\n", cmd, parameter);
  switch (cmd) {
  case DCSC_SET_OPTION_FLAG:
    iResult=setDebugOptionFlag(parameter);
    CE_Info("dcscRCUaccess debug options: %#x\n", iResult);
    break;
  case DCSC_CLEAR_OPTION_FLAG:
    iResult=clearDebugOptionFlag(parameter);
    CE_Info("dcscRCUaccess debug options: %#x\n", iResult);
    break;
  case DCSC_USE_SINGLE_WRITE:
    if (parameter!=0) {
      iResult=ceSetOptionFlag(DEBUG_USE_SINGLE_WRITE);
      CE_Info("use single write (%0x)\n", iResult);
    } else {
      iResult=ceClearOptionFlag(DEBUG_USE_SINGLE_WRITE);
      CE_Info("use multiple write (%0x)\n", iResult);
    }
    break;
  default:
    CE_Warning("unrecognized command id (%#x)\n", cmd);
    iResult=-ENOSYS;
  }
  if (iResult>=0) iResult=iProcessed;
  return iResult;
}

int DCSCMsgBuffer::HighLevelHandler(const char* pCommand, CEResultBuffer& rb)
{
  int iResult=0;
  __u32 cmd=0;
  int keySize=0;
  int len=strlen(pCommand);
  const char* pBuffer=pCommand;
  if (pCommand) {
    if (strncmp(pCommand, "DCSC_SET_OPTION_FLAG", keySize=strlen("DCSC_SET_OPTION_FLAG"))==0)
      cmd=DCSC_SET_OPTION_FLAG;
    else if (strncmp(pCommand, "DCSC_CLEAR_OPTION_FLAG", keySize=strlen("DCSC_CLEAR_OPTION_FLAG"))==0)
      cmd=DCSC_CLEAR_OPTION_FLAG;

    if (cmd>0 && keySize>0) {
      pBuffer+=keySize;
      len-=keySize;
      while (*pBuffer==' ' && *pBuffer!=0 && len>0) {pBuffer++; len--;} // skip all blanks
      __u32 parameter=0;
      __u32 data=0;
      const char* pPayload=NULL;
      int iPayloadSize=0;
      switch (cmd) {
      case DCSC_SET_OPTION_FLAG:
      case DCSC_CLEAR_OPTION_FLAG:
	if (sscanf(pBuffer, "0x%x", &parameter)>0) {
	} else {
	  CE_Error("error scanning high-level command %s\n", pCommand);
	  iResult=-EPROTO;
	}
	break;
      }
      if (iResult>=0) {
	if ((iResult=issue(cmd, parameter, pPayload, iPayloadSize, rb))>=0) {
	  iResult=1;
	}
      }
    }
  } else {
    iResult=-EINVAL;
  }
  return iResult;  
}

#ifdef RCUDUMMY
int DCSCMsgBufferSim::fMemSize=0x10000;

DCSCMsgBufferSim::DCSCMsgBufferSim()
{
  fMemory.resize(fMemSize, 0);
}

DCSCMsgBufferSim::~DCSCMsgBufferSim()
{
  fMemory.clear();
}

int DCSCMsgBufferSim::SingleWrite(__u32 address, __u32 data)
{
  //CE_Debug("DCSCMsgBufferSim::SingleWrite address %#x size %#x\n", address, data);
  if (address<fMemory.size()) fMemory[address]=data;
  return 0;
}

int DCSCMsgBufferSim::SingleRead(__u32 address, __u32* pData)
{
  if (pData && address<fMemory.size()) *pData=fMemory[address];
  //CE_Debug("DCSCMsgBufferSim::SingleRead address %#x data %#x\n", address, *pData);
  return 0;
}

int DCSCMsgBufferSim::MultipleWrite(__u32 address, __u32* pData, int iSize, int iDataSize)
{
  int iResult=0;
  int size=0;
  if (pData) {
    //CE_Debug("DCSCMsgBufferSim::MultipleWrite address %#x size %#x data %#x\n", address, iSize, *pData);
    if (address<fMemory.size()) {
      if ((int)address + iSize < fMemory.size()) size=iSize;
      else size=fMemory.size()-address;
      if (iDataSize==4) {
	memcpy(&fMemory[address], pData, size*sizeof(__u32));
	iResult=size;
      } else if (iDataSize==1 || iDataSize==2) {
	unsigned char* pSrc=(unsigned char*)pData;
	for (int i=0; i<size; i++) {
	  if (iDataSize==1) {
	    fMemory[i]=*pSrc;
	  } else {
	    fMemory[i]=*((unsigned short*)pSrc);
	    pSrc++;
	  }
	  pSrc++;
	}
      } else if (iDataSize==3) {
	CE_Info("so far rcu dummy 10bit write just ignores the data\n"); 
      } else {
	CE_Error("invalid data specification %d\n", iDataSize); 
	iResult=-EINVAL;
      }
    }
  } else {
    size=-EINVAL;
  }
  return iResult;
}

int DCSCMsgBufferSim::MultipleRead(__u32 address, int iSize,__u32* pData)
{
  int size=0;
  if (pData) {
    if (address<fMemory.size()) {
      if ((int)address + iSize < fMemory.size()) size=iSize;
      else size=fMemory.size()-address;
      memcpy(pData, &fMemory[address], size*sizeof(__u32));
    }
  } else {
    size=-EINVAL;
  }
  return size;
}

#endif //RCUDUMMY

