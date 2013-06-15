// $Id: statemachine.cpp,v 1.15 2007/09/03 22:57:45 richter Exp $

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
#include <cmath>
#include <cstring>
#include <cstdio>
#include "statemachine.hpp"
#include "device.hpp"
#include "ce_base.h"
#include "strings.h"

#define UNNAMED_STATE_NAME "UNNAMED"

using namespace std;

CETransition CEStateMachine::fDefTrans[eLastDefaultTransition]={
  CETransition(), // eTransitionUnknown
  CETransition(eSwitchOn,     eStateOn,          "switchon", eStateOff),
  CETransition(eShutdown,     eStateOff,         "shutdown", eStateOn, eStateError, eStateFailure, eStateConfigured),
  CETransition(eConfigure,    eStateConfiguring, "configure",eStateOn, eStateConfigured),
  // configure done is for internal use and has no corresponding action
  // later this must be an available command in order to allow external configuration
  CETransition(eConfigureDone,eStateConfigured,"",     eStateConfiguring),
  CETransition(eReset,        eStateOn,          "reset",    eStateConfigured, eStateConfiguring, eStateError, eStateFailure),
  CETransition(eStart,        eStateRunning,     "start",    eStateConfigured),
  CETransition(eStop,         eStateConfigured,  "stop",     eStateRunning),
  CETransition(eNotify,       eStateUnknown,     "notify",   eStateFailure)
};

CEStateMachine::CEStateMachine()
  :
  fpDevice(NULL),
  fState(eStateUnknown),
  fName(),
  fpMapper(NULL),
  fLogLevel(eCEDebug)
{
}

CEStateMachine::~CEStateMachine()
{
  CleanupTransitionList();
}

int CEStateMachine::Armor()
{
  int iResult=0;
  CreateStateChannel();
  if (fpDevice) {
    iResult=fpDevice->ArmorDevice();
    if (iResult<0) {
      CE_Error("can not armor device %s (%p), switch to error state\n", fpDevice->GetName(), fpDevice);
      ChangeState(eStateError);
    }
  }
  return iResult;
}

int CEStateMachine::ChangeState(CEState state) {
  int iResult=0;
  CELockGuard l(this);
  CEState current=GetCurrentState();
  if (current!=state) {
    if (state!=eStateUnknown && state!=current) {
      if ((iResult=StateLeaveDispatcher(current))<0)
	CE_Warning("exit handler for state %s returned %d\n", GetStateName(current), iResult);
      iResult=StateEnterDispatcher(state);
      fState=state;
      const char* name=GetName();
      if (name) ceUpdateServices(name, 0);
    }
  } else {
    CE_Debug("device %s already in state %s, ignored\n", GetName(), GetStateName(current)); 
  }
  return iResult;
}

int CEStateMachine::SetErrorState()
{
  if (Check(eStateError)) return 0;
  return ChangeState(eStateError);
}

int CEStateMachine::SetFailureState()
{
  CEState states[]={eStateFailure, eStateError, eStateInvalid};
  if (Check(states)) return 0;
  return ChangeState(eStateFailure);
}

int CEStateMachine::SetIntermediateState(CEState state)
{
  // TODO: check here if we are processing a transition and allow
  // to set only in this case
  fState=state;
  const char* name=GetName();
  if (name) ceUpdateServices(name, 0);
  return 0;
}

const char* CEStateMachine::GetName() {
  return fName.c_str();
}

int CEStateMachine::Check(CEState state)
{
  return state==fState;
}

int CEStateMachine::Check(CEState states[])
{
  if (states) {
    CEState* element=states;
    while (*element!=eStateUnknown) {
      if (*element==fState) return 1;
      element++;
    }
  }
  return 0;
}

CEState CEStateMachine::GetCurrentState()
{
  return fState;
}

int CEStateMachine::GetTranslatedState(CEState state) {
  CEState current=state;
  if (state==eStateUnknown)
    current=GetCurrentState();
  if (fpMapper)
    return fpMapper->GetMappedState(current);
  return current;
}

int CEStateMachine::SetTranslationScheme(CEStateMapper* pMapper) {
  int iResult=0;
  if (pMapper) {
    if (fpMapper && fpMapper!=pMapper) {
      CE_Warning("overriding previously initialized mapper instance\n");
    }
    fpMapper=pMapper;
  } else {
    iResult=-EINVAL;
  }
  return iResult;
}

CEStateMapper* CEStateMachine::GetTranslationScheme() {
  return fpMapper;
}

const char* CEStateMachine::GetCurrentStateName()
{
  return GetStateName(fState);
}

int CEStateMachine::TriggerTransition(const char* action, int iMode, int iParam, void* pParam)
{
  int iResult=0; 
  if (action) {
    CETransition* pTrans=FindTransition(action);
    if (pTrans) {
      iResult=TriggerTransition(pTrans, iMode, iParam, pParam);
    } else {
      CE_Error("no transition defined for action %s\n", action);
      iResult=-ENOENT;
    }
  } else {
    CE_Error("invalid pointer\n");
    iResult=-EINVAL;
  }
  return iResult;
}

int CEStateMachine::TriggerTransition(CETransitionId transition, int iMode, int iParam, void* pParam)
{
  int iResult=0; 
  CETransition* pTrans=FindTransition(transition);
  if (pTrans) {
    iResult=TriggerTransition(pTrans, iMode, iParam, pParam);
  } else {
    CE_Error("no transition defined for id %s\n", transition);
    iResult=-ENOENT;
  }
  return iResult;
}

int CEStateMachine::TriggerTransition(CETransition* pTrans, int iMode, int iParam, void* pParam)
{
  int iResult=0; 
  if (pTrans) {
    CELockGuard l(this);
    CEState current=GetCurrentState();
    if (current==pTrans->GetFinalState()) {
      CE_Log(fLogLevel, "device already in state %s: transition \'%s\' ignored\n", GetCurrentStateName(), GetTransitionName(pTrans));
    } else if (IsAllowedTransition(pTrans)) {

      // 1. call the LeaveStateXXXX handler
      iResult=StateLeaveDispatcher(current);
      if (iResult<0) {
	ChangeState(eStateError);
	return iResult;
      }

      // 2. Call the transition handler
      CEState final=TransitionDispatcher(pTrans, fpDevice, iParam, pParam);
      
      if (final==eStateError) {
	iResult=-EFAULT;
	ChangeState(eStateError);
      } else if (IsValidState(final)) {
	// 3. Call the EnterStateXXX handler
	iResult=StateEnterDispatcher(final);
	if (iResult<0) {
	  ChangeState(eStateError);
	  return iResult;
	}
	fState=final;
	const char* name=GetName();
	if (name) ceUpdateServices(name, 0);
	CE_Log(fLogLevel, "device state changed to %s\n", GetCurrentStateName());
      } else {
	CE_Error("dispatcher for transition %d (%s) returned invalid state %d\n", pTrans->GetID(), 
		 GetTransitionName(pTrans), final);
	ChangeState(eStateError);
      }
    } else {
      CE_Log(fLogLevel, "device in state %s: transition \'%s\' not allowed\n", GetCurrentStateName(), GetTransitionName(pTrans));
    }
  } else {
    CE_Error("invalid pointer\n");
    iResult=-EINVAL;
  }
  return iResult;
}

int CEStateMachine::Synchronize()
{
  int iResult=0; 
  if (fpDevice) {
    CELockGuard l(this);
    CEState state=fpDevice->EvaluateHardware();
    if (IsValidState(state)) {
      ChangeState(state);
    } else {
      CE_Error("device evaluate returned invalid state %d\n", state);
      fState=eStateError;
    }
  } else {
    CE_Fatal("state machine not initialized\n");
    iResult=-EFAULT;
  }
  return iResult;
}

int CEStateMachine::InitDevice(CEDevice* pDevice, string name)
{
  int iResult=0; 
  if (pDevice) {
    if (fpDevice) {
      CE_Warning("overriding device pointer %p with %p\n", fpDevice, pDevice);
    }
    fpDevice=pDevice;
    fName=name;
    CreateStateChannel();
  } else {
    CE_Error("invalid pointer\n");
    iResult=-EINVAL;
  }
  return iResult;
}

int CEStateMachine::UpdateStateChannel() {
  return -ENOSYS;
}

int CEStateMachine::CreateStateChannel() {
  return -ENOSYS;
}

int CEStateMachine::CreateAlarmChannel() {
  return -ENOSYS;
}

int CEStateMachine::SendAlarm(int alarm) {
  return 0;
}

int CEStateMachine::IsDefaultState(CEState state)
{
  int iResult=0; 
  switch (state) {
  case eStateUnknown:
  case eStateOff:         
  case eStateError:       
  case eStateFailure:     
  case eStateOn:          
  case eStateConfiguring: 
  case eStateConfigured:  
  case eStateRunning:     
  case eStateUser0:
  case eStateUser1:
  case eStateUser2:
  case eStateUser3:
  case eStateUser4:
    iResult=1;
  }
  return iResult;
}

int CEStateMachine::IsValidState(CEState state) {
  // handling of additional states is currently not implemented
  // we forward this function simply
  return IsDefaultState(state);
}

int CEStateMachine::IsDefaultTransition(CETransitionId id)
{
  int iResult=0; 
  iResult=id>eTransitionUnknown && id<eLastDefaultTransition;
  return iResult;
}

CETransition* CEStateMachine::FindTransition(const char* action) {
  CETransition* pTransition=NULL;

  // check default transitions
  for (int id=eTransitionUnknown+1; id<eLastDefaultTransition; id++) {
    const char* tn=GetDefaultTransitionName(&fDefTrans[id]);
    if (strlen(tn)!=0 && strcasecmp(tn, action)==0) {
      pTransition=&fDefTrans[id];
      break;
    }
  }

  // check dynamic transitions
  if (pTransition==NULL) {
    CE_Debug("checking dynamic transitions\n");
    vector<CETransition*>::iterator element=fTransitions.begin();
    while (pTransition==NULL && element!=fTransitions.end()) {
      CE_Debug("checking dynamic transition %d %s\n", (*element)->GetID(), (*element)->GetName());
      if ((*element)!=NULL && (**element)==action) {
	pTransition=*element;
      }
      element++;
    }
  }
  return pTransition;
}

CETransition* CEStateMachine::FindTransition(CETransitionId id) 
{
  CETransition* pTransition=NULL;
  if (id<eLastDefaultTransition) {
    pTransition=&fDefTrans[id];
  } else {
    vector<CETransition*>::iterator element=fTransitions.begin();
    while (pTransition==NULL && element!=fTransitions.end()) {
      if ((*element!=NULL) && (**element)==id) {
	pTransition=*element;
      }
      element++;
    }
  }
  return pTransition;
}

CETransition* CEStateMachine::GetNextTransition(CETransitionId id)
{
  CETransition* pTransition=NULL;
  int ID=id;
  if (++ID<eLastDefaultTransition) {
    pTransition=&fDefTrans[ID];
  } else {
    vector<CETransition*>::iterator element=fTransitions.begin();
    while (pTransition==NULL && element!=fTransitions.end()) {
      if ((*element!=NULL) && (**element)>=ID) {
	pTransition=*element;
      }
      element++;
    }
  }
  return pTransition;
}

std::string CEStateMachine::GetTransitions(int bAll)
{
  string out;
  CETransition* pT=GetNextTransition();
  while (pT) {
    if (bAll || IsAllowedTransition(pT)) {
      string name=GetTransitionName(pT);
      if (name.length()>0) {
	if (out.length()>0) out+=" ";
	out+=name;
      }
    }
    pT=GetNextTransition(pT->GetID());
  }
  return out;
}

std::string CEStateMachine::GetStates()
{
  string out;
  CEState states[]={
    eStateOff,
    eStateError,
    eStateFailure,
    eStateOn,
    eStateConfiguring,
    eStateConfigured,
    eStateRunning,
    eStateUser0,
    eStateUser1,
    eStateUser2,
    eStateUser3,
    eStateUser4,
    eStateUnknown
  };
  
  CEState* element=states;
  const int maxDigits=4;
  int maxId=(int)pow(10.0, 4)-1;
  char id[maxDigits];
  while (*element!=eStateUnknown) {
    string name=GetStateName(*element);
    if (name.length()>0 && name.compare(UNNAMED_STATE_NAME)!=0) {
      if (out.length()>0) out+=" ";
      out+=name;
      if (*element<maxId) {
	sprintf(id, "%d", GetTranslatedState(*element));
	out+="("; out+=id; out+=")";
      }
    }
    element++;
  }

  return out;
}

int CEStateMachine::IsAllowedTransition(CETransition* pT) {
  int iResult=0;
  if (pT) {
    iResult=pT->IsInitialState(GetCurrentState());
  } else {
    iResult=-EINVAL;
  }
  return iResult;
}

CEState CEStateMachine::DefaultTransitionDispatcher(CETransition* transition, int iParam, void* pParam)
{
  CEState state=eStateUnknown;
  int iResult=0;
  if (transition) {
    if (fpDevice) {
      switch (transition->GetID()) {
      case eSwitchOn:
	iResult=fpDevice->SwitchOn(iParam, pParam);
	break;
      case eShutdown:
	iResult=fpDevice->Shutdown(iParam, pParam); 
	break;
      case eConfigure:
	iResult=fpDevice->Configure(iParam, pParam); 
	break;
      case eConfigureDone:
	// in principle there is no extra handler needed, so we just skip this
	//iResult=fpDevice->(iParam, pParam); 
	state=eStateConfigured;
	break;
      case eReset:
	iResult=fpDevice->Reset(iParam, pParam); 
	break;
      case eStart:
	iResult=fpDevice->Start(iParam, pParam); 
	break;
      case eStop:
	iResult=fpDevice->Stop(iParam, pParam); 
	break;
      case eNotify:
	//iResult=fpDevice->Notify(iParam, pParam); 
	CE_Error("action \'notify\' not yet supported\n");
	iResult=-ENOSYS;
	break;
      default:
	CE_Error("transition %d (%s) is not a default transition\n", transition->GetID(), transition->GetName());
	iResult=-EFAULT;
      }
      if (iResult>=0) state=transition->GetFinalState();
      else state=eStateError;
    } else {
      CE_Fatal("severe internal inconsistancy, device pointer missing\n");
    }
  } else {
    CE_Fatal("severe internal inconsistancy\n");
  }
  return state;
}

int CEStateMachine::StateEnterDispatcher(CEState state)
{
  int iResult=0;
  if (IsDefaultState(state)) {
    iResult=DefaultStateEnterDispatcher(state);
  } else {
    CE_Error("inconsistancy: currently there are no non-default states\n");
    fState=eStateError;
    return -EFAULT;
  }
  return iResult;
}

int CEStateMachine::DefaultStateEnterDispatcher(CEState state)
{
  int iResult=0;
  if (fpDevice) {
    switch (state) {
    case eStateOff:
      iResult=fpDevice->EnterStateOFF();
      break;
    case eStateOn:
      iResult=fpDevice->EnterStateON();
      break;
    case eStateConfiguring:
      //iResult=fpDevice->EnterStateCONFIGURING();
      //CE_Warning("no enter-state handler implemented for state CONFIGURING");
      break;
    case eStateConfigured:
      iResult=fpDevice->EnterStateCONFIGURED();
      break;
    case eStateRunning:
      iResult=fpDevice->EnterStateRUNNING();
      break;
    case eStateError:
      iResult=fpDevice->EnterStateERROR();
      break;
    case eStateUser0:
      iResult=fpDevice->EnterStateUSER0();
      break;
    case eStateUser1:
      iResult=fpDevice->EnterStateUSER1();
      break;
    case eStateUser2:
      iResult=fpDevice->EnterStateUSER2();
      break;
    case eStateUser3:
      iResult=fpDevice->EnterStateUSER3();
      break;
    case eStateUser4:
      iResult=fpDevice->EnterStateUSER4();
      break;
    case eStateUnknown:
    case eStateFailure:
      // these states do not have a handler function
      break;
    default:
      CE_Error("state #%d (%s) is not a default state\n", state, GetStateName(state));
    }
  } else {
    CE_Fatal("severe internal inconsistancy, device pointer missing\n");
  }
  return iResult;
}

int CEStateMachine::StateLeaveDispatcher(CEState state)
{
  int iResult=0;
  if (IsDefaultState(state)) {
    iResult=DefaultStateLeaveDispatcher(state);
  } else {
    CE_Error("inconsistancy: currently there are no non-default states\n");
    fState=eStateError;
    return -EFAULT;
  }
  return iResult;
}

int CEStateMachine::DefaultStateLeaveDispatcher(CEState state)
{
  int iResult=0;
  if (fpDevice) {
    switch (state) {
    case eStateOff:
      iResult=fpDevice->LeaveStateOFF();
      break;
    case eStateOn:
      iResult=fpDevice->LeaveStateON();
      break;
    case eStateConfiguring:
      iResult=fpDevice->LeaveStateCONFIGURING();
      break;
    case eStateConfigured:
      iResult=fpDevice->LeaveStateCONFIGURED();
      break;
    case eStateRunning:
      iResult=fpDevice->LeaveStateRUNNING();
      break;
    case eStateUser0:
      iResult=fpDevice->LeaveStateUSER0();
      break;
    case eStateUser1:
      iResult=fpDevice->LeaveStateUSER1();
      break;
    case eStateUser2:
      iResult=fpDevice->LeaveStateUSER2();
      break;
    case eStateUser3:
      iResult=fpDevice->LeaveStateUSER3();
      break;
    case eStateUser4:
      iResult=fpDevice->LeaveStateUSER4();
      break;
    case eStateUnknown:
    case eStateError:
    case eStateFailure:
      // these states do not have a handler function
      break;
    default:
      CE_Error("state #%d (%s) is not a default state\n", state, GetStateName(state));
    }
  } else {
    CE_Fatal("severe internal inconsistancy, device pointer missing\n");
  }
  return iResult;
}

CEState CEStateMachine::TransitionDispatcher(CETransition* transition, CEDevice* pDevice, int iParam, void* pParam)
{
  int iResult=0;
  CEState state=eStateUnknown;
  if (transition) {
    if (transition->IsDefaultTransition()) {
      state=DefaultTransitionDispatcher(transition, iParam, pParam);
    } else {
      iResult=transition->Handler(pDevice, iParam, pParam);
      if (iResult>=0) state=transition->GetFinalState();
      else state=eStateError;
    }
  } else {
    state=eStateError;
    CE_Fatal("internal missmatch: invalid parameter");
  }
  return state;
}

const char* CEStateMachine::GetStateName(CEState state) {
  const char* sn=NULL;
  if (fpMapper && (sn=fpMapper->GetMappedStateName(state))!=NULL)
    return sn;
  if (IsDefaultState(state)) {
    return GetDefaultStateName(state);
  }
  CE_Error("currently no other states than default ones allowed\n");
  return NULL;
}

const char* CEStateMachine::GetDefaultStateName(CEState state)
{
  const char* name=UNNAMED_STATE_NAME;
  switch (state) {
  case eStateUnknown:     name="UNKNOWN";     break;
  case eStateOff:         name="OFF";         break;
  case eStateError:       name="ERROR";       break;
  case eStateFailure:     name="FAILURE";     break;
  case eStateOn:          name="ON";          break;
  case eStateConfiguring: name="CONFIGURING"; break;
  case eStateConfigured:  name="CONFIGURED";  break;
  case eStateRunning:     name="RUNNING";     break;
  }
  return name;
}

const char* CEStateMachine::GetTransitionName(CETransition* pTransition)
{
  const char* name="unnamed";
  if (pTransition) {
    if (pTransition->IsDefaultTransition()) {
      name=GetDefaultTransitionName(pTransition);
    } else {
      name=pTransition->GetName();
    }
  } else {
    name="invalid";
    CE_Error("invalid argument\n");
  }
  return name;
}

const char* CEStateMachine::GetDefaultTransitionName(CETransition* pTransition)
{
  const char* name="unnamed";
  if (pTransition) {
    if (pTransition->IsDefaultTransition()) {
      name=pTransition->GetName();
    }
  } else {
    name="invalid";
    CE_Error("invalid argument\n");
  }
  return name;
}

int CEStateMachine::AddTransition(CETransition* pTransition)
{
  int iResult=0;
  if (pTransition) {
    if (FindTransition(pTransition->GetID())!=NULL) {
      CE_Warning("Transition of id %d already defined\n", pTransition->GetID());
    } else if (FindTransition(pTransition->GetName())!=NULL) {
      CE_Warning("Transition of name %s already defined\n", pTransition->GetName());
    }
    fTransitions.push_back(pTransition);
  } else {
    iResult=-EINVAL;
  }
  return iResult;
}

int CEStateMachine::CleanupTransitionList()
{
  int iResult=0;
  vector<CETransition*>::iterator element=fTransitions.begin();
  while (element!=fTransitions.end()) {
    if (*element) {
      CETransition* pTrans=*element;
      *element=NULL;
      delete pTrans;
    }
    element++;
  }
  return iResult;
}

CEStateMachine::StateSwitchGuard::StateSwitchGuard(CEStateMachine* pInstance, CEState tmpState)
  :
  fOriginal(eStateUnknown),
  fpInstance(pInstance)
{
  fOriginal=fpInstance->GetCurrentState();
  if (fOriginal!=eStateUnknown &&
      fOriginal!=tmpState &&
      fOriginal!=eStateError &&
      fOriginal!=eStateFailure &&
      fpInstance!=NULL) {
    // not sure if this is the best approach. Maybe we schould call something
    // like a transition
    fpInstance->ChangeState(tmpState);
    const char* name=fpInstance->GetName();
    if (name) ceUpdateServices(name, 0);
  } else {
    fpInstance=NULL; // nothing to do in the destructor
  }
}

CEStateMachine::StateSwitchGuard::~StateSwitchGuard()
{
  if (fOriginal!=eStateUnknown && fpInstance!=NULL) {
    fpInstance->ChangeState(fOriginal);
    const char* name=fpInstance->GetName();
    if (name) ceUpdateServices(name, 0);
  }
}


CETransition::CETransition() {
  fId=eTransitionUnknown;
  fFinalState=eStateUnknown;
  fName="unknown";
}

CETransition::CETransition(CETransitionId id, 
			   CEState finalState, 
			   const char* name,
			   CEState initialState1, CEState initialState2, 
			   CEState initialState3, CEState initialState4, 
			   CEState initialState5) 
{
  SetAttributes(id, finalState, name, initialState1, 
		initialState2, initialState3, initialState4, initialState5);
}

CETransition::CETransition(int id,
			   CEState finalState, 
			   const char* name,
			   CEState initialState1, CEState initialState2, 
			   CEState initialState3, CEState initialState4, 
			   CEState initialState5) 
{
  SetAttributes((CETransitionId)id, finalState, name, initialState1, 
		initialState2, initialState3, initialState4, initialState5);
}

int CETransition::SetAttributes(CETransitionId id,
				CEState finalState, 
				const char* name,
				CEState initialState1, CEState initialState2, 
				CEState initialState3, CEState initialState4, 
				CEState initialState5) 
{
  fId=id;
  if (name) fName=name;
  else fName="unknown";
  fFinalState=finalState;
  if (initialState1!=eStateUnknown) fInitialStates.push_back(initialState1);
  if (initialState2!=eStateUnknown) fInitialStates.push_back(initialState2);
  if (initialState3!=eStateUnknown) fInitialStates.push_back(initialState3);
  if (initialState4!=eStateUnknown) fInitialStates.push_back(initialState4);
  if (initialState5!=eStateUnknown) fInitialStates.push_back(initialState5);
}

CETransition::~CETransition() {
}

CETransitionId CETransition::GetID() {
  return fId;
}

const char* CETransition::GetName() {
  return fName;
}

int CETransition::IsDefaultTransition() {
  int iResult=fId>eTransitionUnknown && fId<eLastDefaultTransition;
  return iResult;
}

int CETransition::IsInitialState(CEState state) {
  int iResult=0;
  if (fInitialStates.size()==0) iResult=1;
  else {
    vector<CEState>::iterator initialState=fInitialStates.begin();
    while (initialState!=fInitialStates.end()) {
      if ((iResult=(*initialState==state))==1) break;
      initialState++;
    }
  }
  return iResult;
}

CEState CETransition::GetFinalState() {
  return fFinalState;
}

int CETransition::Handler(CEDevice* pDevice, int iParam, void* pParam)
{
  return 0;
}

CETransition::CETransition(const CETransition& src)
  :
  fId(src.fId),
  fName(src.fName),
  fFinalState(src.fFinalState),
  fInitialStates(src.fInitialStates.begin(), src.fInitialStates.end())
{
}

CETransition& CETransition::operator=(CETransition& src)
{
  fId=src.fId;
  fName=src.fName;
  fFinalState=src.fFinalState;
  fInitialStates.assign(src.fInitialStates.begin(), src.fInitialStates.end());
  return *this;
}

int CETransition::operator==(const char* action)
{
  if (action!=NULL &&
      fName!=NULL &&
      strlen(fName)!=0)
    return strcasecmp(fName, action)==0;

  return 0;
}

int CETransition::operator==(CETransitionId id)
{
  return fId==id;
}

int CETransition::operator==(int id)
{
  return fId==(CETransitionId)id;
}

int CETransition::operator>(CETransitionId id)
{
  return fId>id;
}

int CETransition::operator>(int id)
{
  return (int)fId>id;
}

int CETransition::operator>=(CETransitionId id)
{
  return fId>=id;
}

int CETransition::operator>=(int id)
{
  return (int)fId>=id;
}
