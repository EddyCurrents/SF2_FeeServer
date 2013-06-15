// $Id: controlengine.cpp,v 1.18 2007/09/11 22:34:14 richter Exp $

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
#include <cstdlib>
#include "ce_base.h"
#include "controlengine.hpp"
#include "rcu_issue.h" // temporary, for RCUce_Issue
#include "ce_command.h"
#include "fee_errors.h"

// externally defined functions (ce_command.c)
// for compatibility with C header files
extern "C" void ce_ready(int iResult);
extern "C" void ce_sleep(int sec);
extern "C" void ce_usleep(int usec);

using namespace std;

ControlEngine::ControlEngine(std::string name)
  : 
  CEDimDevice(name),
  fSleepSec(1),
  fSleepUsec(0),
  fProcFlags(0)
{
  SetInstance(this);
}

ControlEngine::~ControlEngine() {
  ResetInstance(this);
}

ControlEngine* ControlEngine::fpInstance=NULL;
CE_Mutex ControlEngine::fMutex;

int ControlEngine::Run()
{
  int iResult=0;
  if (fpInstance) {
    iResult=fpInstance->RunCE();
  } else {
    iResult=-ENOENT;
  }
  return iResult;
}

int ControlEngine::RunCE()
{
  int iResult=0;
  // this is a quick fix to prevent that messages are sent via the message channel before
  // the DIM server was started, the update function is first called when the server is
  // started. 
  // has to be replaced by a proper state handling of the CE
  int bRunning=0;
  ceClearOptionFlag(ENABLE_CE_LOGGING_CHANNEL);
  ceSetOptionFlag(DEBUG_DISABLE_SRV_UPDT);
  if ((iResult=InitCE())>=0) {
    if ((iResult=Armor())>=0) {
      if ((iResult=Synchronize())>=0) {
	ce_ready(iResult);
	bRunning=1;
	sleep(1);
	ceSetLogLevel(eCEWarning);
	ceSetOptionFlag(ENABLE_CE_LOGGING_CHANNEL);
	ceClearOptionFlag(DEBUG_DISABLE_SRV_UPDT);
	time_t fLastTimestamp;
	time(&fLastTimestamp);
	while ((fProcFlags&eTerminate)==0) {
	  { // dont remove, its part of the lock guard
	    CE_LockGuard g(ControlEngine::fMutex);
	    // skip updating in this loop when other processes are active
	    if (fProcFlags!=0) {
	      CE_Info("active command interpretation, skip update\n");
	      continue;
	    }
	    fProcFlags|=eUpdate;
	  }
	  // dont leave the function according to the specifications
#ifndef DISABLE_SERVICES
	  if (ceCheckOptionFlag(DEBUG_DISABLE_SRV_UPDT)==0) {
	    PreUpdate();
	    ceUpdateServices(NULL, 0);
	    PostUpdate();
	  }
#endif //!DISABLE_SERVICES
	  fProcFlags&=~eUpdate;
	  if (fSleepSec>0) ce_sleep(fSleepSec);
	  else if (fSleepUsec) ce_usleep(fSleepUsec);
	  else ce_sleep(1);
	  if (difftime(time(NULL),fLastTimestamp)>=10) {
	    time(&fLastTimestamp);
	    string timeString=ctime(&fLastTimestamp);
	    //printf("set time stamp %s", timeString.c_str());
	    ceSetTimestamp(timeString.c_str());
	  }
	}
      }
    }
  }
  if (bRunning) {
    CE_Debug("ControlEngine::RunCE terminated (%d)\n", iResult);
  } else {
    ce_ready(FEE_FAILED);
  }
  fProcFlags|=eUpdateTerminated;
  return iResult;
}

int ControlEngine::Terminate()
{
  int iResult=0;
  if (fpInstance) {
    iResult=fpInstance->TerminateCE();
  } else {
    iResult=-ENOENT;
  }
  return iResult;
}

int ControlEngine::TerminateCE()
{
  CE_LockGuard g(ControlEngine::fMutex);
  int iResult=0;
  CE_Debug("ControlEngine::TerminateCE %p\n", this);
  fProcFlags|=eTerminate;
  int count=0;
  int sleeptime=fSleepSec>0?fSleepSec:1;
  // wait until the update and issue threads are finnished
  while (fProcFlags&(eUpdate|eIssue) || (fProcFlags&eUpdateTerminated)==0) {
    CE_Debug("thraeds active (%#x), waiting ...\n", fProcFlags&(eUpdate|eIssue));
    // wait at least two update periods but maximum 60s
    if (count>1 && count*sleeptime>60) {
      iResult=-ETIMEDOUT;
      break;
    }
    ce_sleep(sleeptime);
    count ++;
  }
  return iResult;
}

int ControlEngine::Issue(char* command, char** result, int* size)
{
  int iResult=0;
  int iPrintSize=0;
  if (command!=NULL && result!=NULL && size!=NULL) {
    int iBufferSize=*size;
    *result=NULL;
    *size=0;
    if (fpInstance) {
      CE_LockGuard g(ControlEngine::fMutex);
      if ((fpInstance->fProcFlags&eTerminate)==0) {
	ceAbortUpdate();
	while (fpInstance->fProcFlags&eUpdate) {
	  ce_usleep(200);
	}

	fpInstance->fProcFlags|=eIssue;
	char* pData=command;
	int iProcessed=0;
	CEResultBuffer rb;
	rb.clear();
	if ((iResult=fpInstance->PreIssue(pData+iProcessed, iBufferSize-iProcessed, rb))>=0)
	  iProcessed+=iResult;
       
	if (iResult>=0 && iProcessed<iBufferSize) {
	  if ((iResult=fpInstance->ScanHighLevelCommands(pData+iProcessed, iBufferSize-iProcessed, rb))>=0)
	    iProcessed+=iResult;
	}
	if (iResult>=0 && iBufferSize-iProcessed>3) {
	  // TODO convert to issuehandler loop when all command have been 
	  // converted
	  pData+=iProcessed;
	  *size=iBufferSize-iProcessed;
	  if ((iResult=RCUce_Issue(pData, result, size))>=0) {
	    if (*size>0 && *result!=NULL) {
	      // temporary, append to the result buffer
	      int currSize=rb.size();
	      int grow=(*size)/sizeof(CEResultBuffer::value_type);
	      if (grow*sizeof(CEResultBuffer::value_type)!=*size) {
		CE_Warning("alignment missmatch in result buffer\n");
		grow+=1;
	      }
	      rb.resize(currSize+grow);
	      memcpy(reinterpret_cast<char*>(&rb[currSize]), *result, *size);
	      free(*result);
	      *result=NULL;
	      *size=0;
	    }
	  }
	}
	if (iResult>=0) {
	  *size=rb.size()*sizeof(CEResultBuffer::value_type);
	  if (*size>0) {
	    void* pTarget=NULL;
	    if ((allocateMemory(*size, 'c', "ControlEngine", 'A', &pTarget))==0 && pTarget!=NULL) {
	      *result=static_cast<char*>(pTarget);
	      //CE_Info("result buffer of size %d\n", *size);
	      memcpy(*result, reinterpret_cast<char*>(&rb[0]), *size);
// 	      iPrintSize=*size;
// 	      if (*size>g_iMaxBufferPrintSize)
// 		iPrintSize=g_iMaxBufferPrintSize;
// 	      if (iPrintSize>0)
// 		printBufferHex((unsigned char*)*result, iPrintSize, 4, "CE Debug: issue result buffer");
	    } else {
	      CE_Error("can not allocate result memory\n");
	    }
	  }
	}
	fpInstance->fProcFlags&=~eIssue;
      }
    } else {
      iResult=-ENOENT;
    }
  } else {
    CE_Fatal("Inavlid parameter in ControlEngine::Issue command=%p result=%p size=%p\n",
	     command, result, size);
  }
  return iResult;
}

int ControlEngine::PreIssue(char* command, int size, CEResultBuffer& rb)
{
  int iResult=0;
  return iResult;
}

int ControlEngine::ScanHighLevelCommands(char* command, int size, CEResultBuffer& rb)
{
  //CE_Debug("buffer %p size %d\n", command, size);
  int iResult=0;
  char* pData=command;
  int iProcessed=0;
  char* searchKey=NULL;
  char* highlevelKey=NULL;
  int highlevelCmd=0;
  if (iResult==0 
      /* note: the assignment of searchKey is by purpose! */
      && (((highlevelKey=strstr(pData, "<fee>"))!=NULL && (highlevelCmd=1) && (searchKey="</fee>")) ||
	  ((highlevelKey=strstr(pData, "<action>"))!=NULL && (highlevelCmd=2) && (searchKey="</action>"))
	  )
      ) {
    char* pEndKey=NULL;
    if ((pEndKey=strstr(pData, searchKey))!=NULL) {
      iProcessed=int(pEndKey-pData)+strlen(pEndKey);
      // jump to the end of the key
      highlevelKey=strchr(highlevelKey, '>');
      // remove all heading blanks
      while (*(++highlevelKey)==' ' && (highlevelKey<(pData+size-iProcessed)));
      // remove all trailing blanks
      char* pEndCmd=pEndKey;
      *pEndKey=0;
      while (--pEndCmd>highlevelKey && (*pEndCmd==' ' || *pEndCmd==0)) *pEndCmd=0;
      if (strlen(highlevelKey)>0) {
	if (highlevelCmd==2) {
	  CE_Info("got action: %s\n", highlevelKey);
	  extern int actionHandler(const char* pAction);
	  iResult=ControlEngine::ActionHandler(highlevelKey);
	} else {
	  CE_Info("got highlevel command: %s\n", highlevelKey);
	  iResult=ControlEngine::HighLevelCommandHandler(highlevelKey, rb);
	}
      } else {
	CE_Warning("empty %s ignored\n", highlevelCmd==1?"high-level command":"action");
      }
      if (iResult>=0) {
	iResult=iProcessed;
      }
    } else {
      CE_Error("missing end-keyword for command buffer\n");
      iResult=-EPROTO;
    }
  }
  return iResult;
}

int ControlEngine::SetUpdateRate(unsigned short millisec)
{
  int iResult=0;
  if (fpInstance) {
    if (millisec>=1000) {
      fpInstance->SetUpdateRate(millisec/1000, 0);
    } else {
      fpInstance->SetUpdateRate(0, millisec*1000);
    }
  } else {
    iResult=-ENOENT;
  }
  return iResult;
}

int ControlEngine::SetUpdateRate(unsigned short sec, unsigned short usec)
{
  int iResult=0;
  fSleepSec=sec;
  fSleepUsec=usec;
  return iResult;
}

int ControlEngine::ActionHandler(const char* pAction) {
  int iResult=0;
  if (pAction) {
    if (fpInstance) {
      string action(pAction);
      string target("");
      while (action[0]==' ') action.erase(0,1);
      string::size_type blank = action.find( " " );
      if( blank != string::npos ) {
	target=action.substr(0, blank);
	action=action.substr(blank+1);
	while (action[0]==' ') action.erase(0,1);
	if (action.length()==0) {
	  action=target;
	  target="";
	}
      }
      //CE_Debug("compare device name \'%s\' with target \'%s\'\n", fpInstance->GetServiceBaseName(), target.c_str());
      if (target.length()>0 && target.compare(fpInstance->GetServiceBaseName())!=0) {
	if ((iResult=fpInstance->SendActionToSubDevice(target.c_str(), action.c_str()))>=0) {
	} else if (iResult==-ENOENT) {
	  CE_Warning("can not find sub-device %s\n", target.c_str());
	}
      } else {
	iResult=fpInstance->TriggerTransition(action.c_str());
      }
    }
  } else {
    iResult=-EINVAL;
  }
  return iResult;
}

int ControlEngine::ActionHandler(CETransitionId transition) {
  int iResult=0;
  if (fpInstance) 
    iResult=fpInstance->TriggerTransition(transition);
  return iResult;
}

int ControlEngine::HighLevelCommandHandler(const char* pCommand, CEResultBuffer& rb) {
  int iResult=0;
  if (pCommand) {
    int canHandle=0;
    CEIssueHandler* pIH=CEIssueHandler::getFirstIH();
    while (pIH!=NULL && iResult==0) {
      iResult=pIH->HighLevelHandler(pCommand, rb);
      if (iResult!=-ENOSYS) {
	//CE_Debug("HighLevelHandler %p result %d\n", pIH, iResult);
	if (iResult>0) break;
	else if (iResult==-EEXIST) {
	  canHandle=1; 
	  iResult=0; // keep going
	}
      } else {
	iResult=0; // keep going
      }
      pIH=CEIssueHandler::getNextIH();
    }
    if (iResult==0) {
      if (canHandle>0) {
	CE_Warning("can not find device/handler/target for command %s\n", pCommand);
	iResult=-ENODEV;
      } else {
	CE_Warning("unknown high-level command %s\n", pCommand);	
	iResult=-ENOENT;
      }
    } else if (iResult==-EPROTO) {
      CE_Error("error scanning high-level command %s\n", pCommand);
    }
  } else {
    iResult=-EINVAL;
  }
  return iResult;
}

int ControlEngine::SetInstance(ControlEngine* pInstance)
{
  int iResult=0;
  CE_Debug("ControlEngine::SetInstance %p\n", pInstance);
  if (fpInstance==NULL || fpInstance==pInstance) {
    fpInstance=pInstance;
  } else {
    CE_Warning("CE instance %p ignored\n", pInstance);
  }
  return iResult;
}

int ControlEngine::ResetInstance(ControlEngine* pInstance)
{
  int iResult=0;
  if (fpInstance==pInstance) fpInstance=pInstance;
  return iResult;
}

int ControlEngine::InitCE()
{
  // default implementation, do nothing
  return 0;
}

int ControlEngine::DeinitCE()
{
  // default implementation, do nothing
  return 0;
}

int ControlEngine::PreUpdate()
{
  // default implementation, do nothing
  return 0;
}

int ControlEngine::PostUpdate()
{
  // default implementation, do nothing
  return 0;
}

extern "C" int ControlEngine_Run()
{
  ControlEngine* pCE=CEagent::Create();
  if (pCE) {
    pCE->SetLogLevel(eCEInfo);
    CE_Info("\n************************************************\n"
	    "\n   Running ControlEngine %s (%p)\n"
	    "\n   created from agent %s\n"
	    "\n************************************************\n"
	    , pCE->GetName(), pCE, CEagent::GetInstanceName());
    return ControlEngine::Run();
  }
  CE_Fatal("agent %s: can not create ControlEngine Instance\n", CEagent::GetInstanceName());
  ce_ready(-6/*FEE_CE_NOTINIT*/); // bad, I know
  return -EFAULT;
}

extern "C" int ControlEngine_Terminate()
{
  return ControlEngine::Terminate();
}

extern "C" int ControlEngine_SetUpdateRate(unsigned short millisec)
{
  return ControlEngine::SetUpdateRate(millisec);
}

extern "C" int ControlEngine_Issue(char* command, char** result, int* size)
{
  return ControlEngine::Issue(command, result, size);
}

CEagent::CEagent()
{
  SetInstance(this);
}

CEagent::~CEagent()
{
  ResetInstance(this);
}

CEagent* CEagent::fpInstance=NULL;

int CEagent::SetInstance(CEagent* pInstance)
{
  int iResult=0;
  CE_Debug("CEagent::SetInstance %p\n", pInstance);
  if (fpInstance==NULL || 
      (fpInstance!=pInstance && fpInstance->IsDefaultAgent()==1)) {
    fpInstance=pInstance;
  } else if (pInstance->IsDefaultAgent()==1) {
    CE_Warning("CE agent instance %p ignored\n", pInstance);
  }
  return iResult;
}

int CEagent::ResetInstance(CEagent* pInstance)
{
  int iResult=0;
  if (fpInstance==pInstance) fpInstance=pInstance;
  return iResult;
}

ControlEngine* CEagent::Create()
{
  ControlEngine* pCE=NULL;
  if (fpInstance) {
    pCE=fpInstance->CreateCE();
    CE_Debug("created CE %p instance from agent %s (%p)\n", pCE, fpInstance->GetName(), fpInstance);
  }
  return pCE;
}

const char* CEagent::GetInstanceName()
{
  if (fpInstance) return fpInstance->GetName();
  return "not available";
}

int CEagent::IsDefaultAgent()
{
  // default implementation, do nothing
  return 0;
}
