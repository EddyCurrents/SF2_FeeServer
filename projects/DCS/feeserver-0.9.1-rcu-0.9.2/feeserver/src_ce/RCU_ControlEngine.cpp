// $Id: RCU_ControlEngine.cpp,v 1.19 2007/09/01 12:40:38 richter Exp $

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
#include "ce_base.h"
#include "RCU_ControlEngine.hpp"
#include "dev_rcu.hpp"
#include "dcscMsgBufferInterface.h" 
#include "rcu_issue.h" // for FEE_CONFIGURE commands

using namespace std;

#ifdef RCU

/******************************************************************/

DefaultRCUagent g_DefaultRCUagent;

const char* DefaultRCUagent::GetName()
{
  return "RCU ControlEngine";
}

int DefaultRCUagent::IsDefaultAgent()
{
  return 1;
}

ControlEngine* DefaultRCUagent::CreateCE()
{
  return new RCUControlEngine;
}

/** global instance of the PVSS state mapper */
PVSSStateMapper  g_PVSSStateMapper;

extern int actionHandler(CETransitionId transition);
int translateCommand(char* buffer, int size, CEResultBuffer& rb, int bSingleCmd);

RCUControlEngine::RCUControlEngine()
  :
  fpRCU(new CErcu),
  fpACTEL(new CEactel),
  fIndent(0)
{
  SetTranslationScheme(&g_PVSSStateMapper);
  if (fpRCU) {
    fpRCU->SetDeviceId(eRCUDevice);
    fpRCU->SetParentDevice(this);
  }

  // define additional transitions
  CETransition* pConfDDL=new CETransition(eGoConfDDL, eStateUser0, "START_DAQ_CONF", eStateConfiguring, eStateOn, eStateOff);
  CETransition* pCloseDDL=new CETransition(eCloseDLL, eStateConfigured, "END_DAQ_CONF", eStateUser0);
  AddTransition(pConfDDL);
  AddTransition(pCloseDDL);

  if (fpACTEL) {
    fpACTEL->SetDeviceId(eACTELDevice);
    fpACTEL->SetParentDevice(this);
  }
}

RCUControlEngine::~RCUControlEngine()
{
  int iResult=0;
  fpRCU=NULL; // deleted in sub-device handling
}

CEState RCUControlEngine::EvaluateHardware() {
  CEState state=eStateUnknown;
  if (GetCurrentState()==eStateUnknown) state=eStateOff; // we start in state IDLE
  if (fpRCU) {
    fpRCU->Synchronize();
    if (fpRCU->GetCurrentState()==eStateError) {
      CE_Error("sub-device %s is in ERROR state, switching to ERROR\n", fpRCU->GetName());
      state=eStateError;
    }
  } else {
    CE_Error("I need an RCU sub-device! Seems to be uninitialized\n");
    state=eStateError;
  }
  return state;
}

extern CErcu g_RCU; // dev_rcu.cpp
int RCUControlEngine::ArmorDevice()
{
 int iResult=0;
 // init the rcu sub-device, later this has to be added to the list
 // of sub-devices, but this functionality has to be implemented
 // in the CEdevice class first
 if (fpRCU) {
   AddSubDevice(fpRCU);
   fpRCU->Armor();
 } else {
   iResult=-ENOMEM;
 }

 if(fpACTEL) {
   AddSubDevice(fpACTEL);
   fpACTEL->Armor();
 } else {
   iResult=-ENOMEM;
 }
 //CE_Info("ACTEL sub-device is in state %s (%d)\n", fpACTEL->GetCurrentStateName(), GetCurrentState());

 return iResult;
}

const char* RCUControlEngine::GetDefaultTransitionName(CETransition* pTransition)
{
  const char* name="unnamed";
  if (pTransition) {
    if (pTransition->IsDefaultTransition()) {
      /* some of the transitions must be renamed in order to have the same
       * name as in the mirroring SCADA implementation.
       */
      switch (pTransition->GetID()) {
      case eShutdown:
	name="Go_Idle";
	break;
      case eReset:
	name="Go_Standby";
	break;
      default:
	name=pTransition->GetName();
      }
    } else {
      CE_Warning("transition with id %d is not a default one\n", pTransition->GetID());
    }
  } else {
    name="invalid";
    CE_Error("invalid argument\n");
  }
  return name;
}

int RCUControlEngine::IsAllowedTransition(CETransition* pT) {
  int iResult=0;
  if (pT) {
    CEState current=GetCurrentState();
    int id=pT->GetID();
    switch (id) {
    case eConfigure:
      iResult=(current==eStateOff) || 
	(current==eStateOn) ||
	(current==eStateConfigured);
      break;
    case eSwitchOn:
      // SwitchOn is replaced by GoStandby
      iResult=0;
      break;
    case eReset:
      // add the GoStandby transition for the OFF and CONF_DDL states
      iResult|=(current==eStateOff);
      iResult|=(current==eStateUser0);
      iResult|=CEStateMachine::IsAllowedTransition(pT);
      break;
    case eStart:
    case eStop:
      // the state RUNNING has been dropped for the main CE
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

int RCUControlEngine::EnterStateOFF() {
  int iResult=0;
  if (fpRCU) {
  }
  // clear all pending configurations
  fConfigurations.clear();
  return iResult;
}

int RCUControlEngine::LeaveStateOFF() {
  int iResult=0;
  return iResult;
}

int RCUControlEngine::EnterStateON() {
  int iResult=0;
  CE_Debug("RCUControlEngine::EnterStateON called\n");
  // check whether the RCU sub-device is running
  if (fpRCU) {
    CEState blackList[]={eStateError, eStateOff, eStateFailure, eStateInvalid};
    if (fpRCU->Check(blackList)) {
      // try to switch the RCU on if it is in OFF state
      if (fpRCU->Check(eStateOff)) iResult=fpRCU->TriggerTransition(eSwitchOn);
      // try to reset to get the RCU sub-device to ON state
      else iResult=fpRCU->TriggerTransition(eReset);
      if (iResult>=0) {
      } else {
	CE_Error("RCU sub-device is in state %s (%d)\n", fpRCU->GetCurrentStateName(), GetCurrentState());
      }
    }
  }
  return iResult;
}

int RCUControlEngine::LeaveStateON() {
  int iResult=0;
  CE_Debug("RCUControlEngine::LeaveStateON called\n");
  return iResult;
}

int RCUControlEngine::EnterStateCONFIGURED() {
  int iResult=0;
  CE_Debug("RCUControlEngine::EnterStateCONFIGURED called\n");
  return iResult;
}

int RCUControlEngine::LeaveStateCONFIGURED() {
  int iResult=0;
  CE_Debug("RCUControlEngine::LeaveStateCONFIGURED called\n");
  return iResult;
}

int  RCUControlEngine::EnterStateCONFIGURING() {
  int iResult=0;
  return iResult;
}

int  RCUControlEngine::LeaveStateCONFIGURING() {
  int iResult=0;
  if (fpRCU) {
    iResult=fpRCU->TriggerTransition(eConfigureDone);
  }
  return iResult;
}

int RCUControlEngine::EnterStateRUNNING() {
  int iResult=0;
  CE_Debug("RCUControlEngine::EnterStateRUNNING called\n");
  return iResult;
}

int RCUControlEngine::LeaveStateRUNNING() {
  int iResult=0;
  CE_Debug("RCUControlEngine::LeaveStateRUNNING called\n");
  return iResult;
}

int RCUControlEngine::SwitchOn(int iParam, void* pParam) {
  int iResult=0;
  CE_Debug("RCUControlEngine::SwitchOn called\n");
  return iResult;
} 

int RCUControlEngine::Shutdown(int iParam, void* pParam) {
  int iResult=0;
  CE_Debug("RCUControlEngine::Shutdown called\n");
  SetIntermediateState(eStateUser1);
  if (fpRCU) {
    fpRCU->TriggerTransition(eShutdown);
  }
  return iResult;
} 

int RCUControlEngine::Configure(int iParam, void* pParam) {
  int iResult=0;
  CE_Debug("RCUControlEngine::Configure called\n");
  // check whether the RCU sub-device is running
  if (fpRCU) {
    // try to switch the RCU on if it is in OFF state
    if (fpRCU->Check(eStateOff)) iResult=fpRCU->TriggerTransition(eSwitchOn);
    CEState states[]={eStateOn, eStateConfigured};
    if (fpRCU->Check(states)) {
      // set the RCU to state CONFIGURING
      iResult=fpRCU->TriggerTransition(eConfigure);
    } else {
      CE_Error("RCU sub-device is in state %s (%d)\n", fpRCU->GetCurrentStateName(), fpRCU->GetCurrentState());
      iResult=-ECHILD;
    }
  }
  return iResult;
} 

int RCUControlEngine::Reset(int iParam, void* pParam) {
  int iResult=0;
  CE_Debug("RCUControlEngine::Reset called\n");
  // clear all pending configurations
  fConfigurations.clear();
  if (fpRCU) {
    fpRCU->TriggerTransition(eReset);
  }
  return iResult;
} 

int RCUControlEngine::Start(int iParam, void* pParam) {
  int iResult=0;
  CE_Debug("RCUControlEngine::Start called\n");
  if (fpRCU) {
    iResult=fpRCU->TriggerTransition(eStart);
  }
  return iResult;
} 

int RCUControlEngine::Stop(int iParam, void* pParam) {
  int iResult=0;
  CE_Debug("RCUControlEngine::Stop called\n");
  if (fpRCU) {
    iResult=fpRCU->TriggerTransition(eStop);
  }
  return iResult;
}

int RCUControlEngine::GetGroupId()
{
  // no unique group id, use CheckCommand
  return -1;
}

int RCUControlEngine::CheckCommand(__u32 cmd, __u32 parameter)
{
  int iResult=0;
  switch (cmd) {
  case FEE_CONFIGURE:
  case FEE_CONFIGURE_END:
  case FEE_VERIFICATION:
  case FEE_EXTERNAL_CONFIGURATION:
    iResult=1;
    break;
  default:
    iResult=0;
  }
  CE_Debug("RCUControlEngine::CheckCommand cmd %#x parameter %#x result %d\n", cmd, parameter, iResult);
  return iResult;
}

__u32 RCUControlEngine::EvaluateHwAddress(__u32 hwAddress, __u32 mask) const
{
  __u32 address=hwAddress;
  if (mask==0) return 0;
  while ((mask&0x1)==0) {mask>>=1; address>>=1;}
  address&=mask;
  return address;
}

int RCUControlEngine::FindFeeConfigureDesc(__u32 tag, int type)
{
  int iResult=fConfigurations.size();
  while (--iResult>=0) {
    if ((type<0  && fConfigurations[iResult].hwAddress==tag) ||
	(type==0 && fConfigurations[iResult].device==tag) ||
	(type==1 && fConfigurations[iResult].device==2 && fConfigurations[iResult].fec==tag))
      break;
  }
  if (iResult==-1) iResult=-ENOENT;
  return iResult;
}

RCUControlEngine::FeeConfigureDesc RCUControlEngine::CreateFeeConfigureDesc(__u32 hwAddress)
{
  FeeConfigureDesc desc;
  desc.hwAddress=hwAddress;
  desc.device=EvaluateHwAddress(hwAddress, FEE_CONFIGURE_HW_DEVICE);
  desc.fec=EvaluateHwAddress(hwAddress, FEE_CONFIGURE_HW_ALTRO_BRANCH|FEE_CONFIGURE_HW_ALTRO_FEC);
  desc.altro=EvaluateHwAddress(hwAddress, FEE_CONFIGURE_HW_ALTRO_NO);
  desc.channel=EvaluateHwAddress(hwAddress, FEE_CONFIGURE_HW_ALTRO_CHANNEL);
  return desc;
}

int RCUControlEngine::ExecFeeConfigure(__u32 parameter, const char* pData, int iDataSize, CEResultBuffer& rb)
{
  int iResult=0;
  int iProcessed=0;
  if (iDataSize>=2*sizeof(__u32)) {
    __u32 hwAddress=*((__u32*)pData);
    __u32 checksum=*(((__u32*)pData)+1);
    int pendingConfigurations=fConfigurations.size();

    FeeConfigureDesc desc=CreateFeeConfigureDesc(hwAddress);

    fIndent++;
    string format="";
    for (int i=0; i<fIndent; i++) format+=" "; 
    format+="%s (%#x): device ";
    switch (desc.device) {
    case 0:
      format+="CE";
      break;
    case 1:
      format+="RCU";
      break;
    case 2:
      format+="FEC";
      format+=" (fec %d)";
      break;
    case 3:
      format+="ALTRO";
      format+=" (fec %d altro %d)";
      break;
    case 4:
      format+="CHANNEL";
      format+=" (fec %d altro %d channel %d)";
      break;
    default:
      format="unknown hardware address %#x";
    }
    format+="\n";
    // search for a pending FEE_CONFIGURE for the device
    int pos=FindFeeConfigureDesc(desc.device, 0);
    if (pos==-ENOENT) {
      if (!Check(eStateConfiguring))
	iResult=ActionHandler(eConfigure);
      if (iResult>=0 && Check(eStateConfiguring)) {
	fConfigurations.push_back(desc);
      }
      CE_Info(format.c_str(), "FEE_CONFIGURE    ", hwAddress, desc.fec, desc.altro, desc.channel);
    } else {
      CE_Info(format.c_str(), "found pending FEE_CONFIGURE", hwAddress, desc.fec, desc.altro, desc.channel);
    }

    int i=0;
    int iCmdWords=0; // counter for processed words
    if (iResult>=0 && Check(eStateConfiguring)) {
      iProcessed=2*sizeof(__u32);
      for (i=0; iCmdWords<parameter && iResult>=0; i++) {
	if ((iResult=translateCommand((char*)pData+iProcessed, iDataSize-iProcessed, rb, 1))>=0) {
	  iCmdWords+=iResult/sizeof(__u32);
	  iProcessed+=iResult; // this is the byte counter
	}
      }
    } else {
      CE_Error("can not switch main state machine to configuration mode, error %d\n", iResult);
    }
    CE_Debug("end of FEE_CONFIGURE sequence with result %d (%d commands, %d words)\n", iResult<0?iResult:iProcessed, i, iCmdWords);
    fIndent--;
  } else {
    CE_Error("missing payload for FEE_CONFIGURE\n");
    iResult=-EPROTO;
  }
  if (iResult>=0) iResult=iProcessed;
  return iResult;
}

int RCUControlEngine::issue(__u32 cmd, __u32 parameter, const char* pData, int iDataSize, CEResultBuffer& rb)
{
  int iResult=0;
  int iProcessed=0;
  switch (cmd) {
  case FEE_CONFIGURE:
    if ((iResult=ExecFeeConfigure(parameter, pData, iDataSize, rb))>=0) {
      iProcessed+=iResult;
      CE_Debug("%d %d\n", iResult, iProcessed);
    }
    break;
  case FEE_CONFIGURE_END:
    if (iDataSize>=sizeof(__u32)) {
      __u32 hwAddress=*((__u32*)pData);
      int pos=FindFeeConfigureDesc(hwAddress);
      if (pos>=0) {
	string format="%s%s (%#x): device ";
	const char* blanks="      ";
	int nofblanks=strlen(blanks)-fIndent;
	if (nofblanks>0) blanks+=nofblanks;
	switch (fConfigurations[pos].device) {
	case 0:
	  format+="CE";
	  break;
	case 1:
	  format+="RCU";
	  break;
	case 2:
	  format+="FEC";
	  format+=" (fec %d)";
	  break;
	case 3:
	  format+="ALTRO";
	  format+=" (fec %d altro %d)";
	  break;
	case 4:
	  format+="CHANNEL";
	  format+=" (fec %d altro %d channel %d)";
	  break;
	default:
	  format="unknown hardware address %#x";
	}
	format+="\n";
	CE_Info(format.c_str(), blanks, "FEE_CONFIGURE_END", fConfigurations[pos].hwAddress, fConfigurations[pos].fec, fConfigurations[pos].altro, fConfigurations[pos].channel);
	if (pos!=fConfigurations.size()-1) {
	  CE_Warning("found open FEE_CONFIGURE operations, force termination\n");
	  while (pos!=fConfigurations.size()-1) {
	    CE_Warning("    forced termination of FEE_CONFIGURE %#x\n", fConfigurations.back().hwAddress);
	    fConfigurations.pop_back();
	  }
	}
	fConfigurations.pop_back();
      } else {
	CE_Warning("no active FEE_CONFIGURE for hardware address %#x, FEE_CONFIGURE_END ignored\n", hwAddress);
      }

      if (fConfigurations.size()==0 && !Check(eStateUser0)/* not in CONF_DDL*/) {
	iResult=ActionHandler(eConfigureDone);
      }
      if (iResult>=0) {
      } else {
	CE_Error("state machine termination of configuration mode, error %d\n", iResult);
      }
      iProcessed=sizeof(__u32);
    } else {
      CE_Error("missing hardware address for FEE_CONFIGURE_END\n");
      iResult=-EPROTO;
    }
    break;
  case FEE_VERIFICATION:
    if (parameter>0) {
      CE_Error("non-empty FEE_VERIFICATION command currently not available, use empty one\n");
      iResult=-ENOSYS;
    }
    break;
  case FEE_EXTERNAL_CONFIGURATION:
    CE_Debug("got FEE_EXTERNAL_CONFIGURATION hw address %#x\n", parameter);
    iResult=ActionHandler((CETransitionId)eGoConfDDL);
    break;
  default:
    CE_Warning("unrecognized command id (%#x)\n", cmd);
    iResult=-ENOSYS;
  }

  if (iResult>=0) iResult=iProcessed;
  else SetErrorState();
  return iResult;
}

int RCUControlEngine::GetPayloadSize(__u32 cmd, __u32 parameter)
{
  CE_Error("RCUControlEngine::GetPayloadSize needs to be imlemented\n");
  return 0;
}

int RCUControlEngine::PreUpdate()
{
  int iResult=0;
  if (fpRCU) {
    fpRCU->ReadFECActiveList();
  }
  return iResult;
}

int RCUControlEngine::SetRcuBranchLayout(RcuBranchLayout* pLayout)
{
  int iResult=0;
  if (fpRCU) {
    iResult=fpRCU->SetBranchLayout(pLayout);
  } else {
    iResult=-ENODEV;
  }
  return iResult;
}

int RCUControlEngine::SubDevicesLocked()
{
  CEState WhiteList[]={eStateOn, eStateInvalid};
  return Check(WhiteList)==0;
}

#endif //RCU

PVSSStateMapper::PVSSStateMapper() {
}

PVSSStateMapper::~PVSSStateMapper() {
}

int PVSSStateMapper::GetMappedState(CEState state) {
  switch (state) {
    case eStateConfiguring: return kStateDownloading;     // DOWNLOADING
    case eStateError:       return kStateError;           // ERROR
    case eStateRunning:     return kStateRunning;         // RUNNING 
    case eStateOn:          return kStateStandby;         // STANDBY
    case eStateOff:         return kStateIdle;            // IDLE
    case eStateConfigured:  return kStateStbyConfigured;  // STBY_CONFIGURED
    case eStateUser0:       return kStateConfDdl;         // CONF_DDL
    case eStateUser1:       return kStateRampingDown;     // RAMPING_DOWN
    case eStateUser2:       return kStateRampingUp;       // RAMPING_UP

  }
  CE_Warning("can not translate state %d to PVSS state coding\n", state);
  return kStateElse;
}

CEState PVSSStateMapper::GetStateFromMappedState(int state) {
  switch (state) {
    case kStateDownloading:    return eStateConfiguring; // DOWNLOADING
    case kStateError:          return eStateError;       // ERROR	
    case kStateRunning:        return eStateRunning;     // RUNNING 	
    case kStateStandby:        return eStateOn;          // STANDBY	
    case kStateIdle:           return eStateOff;         // IDLE	
    case kStateStbyConfigured: return eStateConfigured;  // STBY_CONFIGURED
    case kStateConfDdl:        return eStateUser0;       // CONF_DDL
    case kStateRampingDown:    return eStateUser1;       // RAMPING_DOWN
    case kStateRampingUp:      return eStateUser2;       // RAMPING_UP
  }
  CE_Warning("can not translate PVSS state %d to internal state coding\n", state);
  return eStateUnknown;
}

const char* PVSSStateMapper::GetMappedStateName(CEState state)
{
  const char* name=NULL; 
  switch (state) {
  case eStateUnknown:     name="UNKNOWN";         break;
  case eStateOff:         name="IDLE";            break;
  case eStateOn:          name="STANDBY";         break;
  case eStateConfiguring: name="DOWNLOADING";     break;
  case eStateConfigured:  name="STBY_CONFIGURED"; break;
  case eStateRunning:     name="";                break;
  case eStateUser0:       name="CONF_DDL";        break;
  case eStateUser1:       name="RAMPING_DOWN";    break;
  case eStateUser2:       name="RAMPING_UP";      break;
  }
  return name;
}
