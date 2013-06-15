// $Id: dev_rcu.cpp,v 1.39 2007/09/07 06:57:21 richter Exp $

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

#include <cstdlib>
#include <cstring>
#include <cstdio>
#include "dev_rcu.hpp"
#include "rcu_issue.h"
#include "dev_fec.hpp"
#include "rcu_service.h"
#include "dev_msgbuffer.hpp"
#include "RCU_ControlEngine.hpp" // for the PVSSStateMapper ids

using namespace std;

extern "C" void ce_sleep(int sec);
extern "C" void ce_usleep(int usec);

#ifdef RCU

#define SERVICE_SIM_HYST 5

/**** SERVICE HANDLER CALLBACKS     ********************************************/

/**
 * Update the active FEC list from the hardware (service handler function).
 * This function is registerd during service registration in
 * @ref CErcu::PublishRegs.
 * @param pData      int value data to be updated
 * @param FEC        FEC number
 * @param reg        register number
 * @param parameter  not yet used (handle to instance of CErcu)
 * @internal
 * @ingroup rcu_service
 */
int updateRcuRegister(TceServiceData* pData, int address, int type, void* parameter)
{
  int iResult=0;
  CErcu* pRcu=(CErcu*)parameter;
  if (pRcu) {
    iResult=pRcu->UpdateRcuRegister(pData, address, type);
  }
  return iResult;    
}

/**** END OF SERVICE HANDLER CALLBACKS     *************************************/

CErcu::CErcu()
  : 
  CEDimDevice("RCU"), 
  CEIssueHandler(),
  fOptions(RCU_DEFAULT_OPTIONS),
  fAFL(0),
  fAFLmask(0),
  fpMsgBuffer(NULL),
  fUpdateTempDisable(0)
{
  fFwVersion=-1;
  fFecIds.resize(FECActiveList_WIDTH, -1);
  fMaxFecB=FECActiveList_WIDTH/2;
  fMaxFecA=FECActiveList_WIDTH-fMaxFecB;  
  time(&fAflAccess);
}

CErcu::~CErcu() 
{
  if (fpMsgBuffer) DCSCMsgBuffer::ReleaseInstance(fpMsgBuffer);
  fpMsgBuffer==NULL;
}

CE_Mutex CErcu::fMutex;

CEState CErcu::EvaluateHardware() 
{
  int iResult=0;
  CEState state=eStateUnknown;
  if (fpMsgBuffer==NULL || fpMsgBuffer->CheckResult()<0) {
    return eStateError;
  }

  if (GetCurrentState()==eStateUnknown) {
    if (fFwVersion==0) {
      /* this is the case for invalid FW version number and no access to
       * MSM registers while the DDL SIU is Altro Bus Master (ABM)
       * -> go directly to state CONFIGURED with DCS board as ABM
       */
      state=eStateOn;
    } else if (fFwVersion>=1) {
      /* all other cases where the DDL SIU is the default ABM
       */
      state=eStateOff;
    } else {
      CE_Info("RCU hardware access not established, setting to state OFF\n");
      state=eStateOff;
    }
  }
  return state;
}

int CErcu::ArmorDevice()
{
  int iResult=0;
  // init the msg buffer interface
  fpMsgBuffer=DCSCMsgBuffer::GetInstance();

  iResult=EvaluateFirmwareVersion();
  if (iResult>=0) {
    fFwVersion=iResult;
  } else {
    // the state machine should not go to ERROR state, better to OFF during
    // EvaluateHardware
    iResult=0;
  }
  SingleRead(FECActiveList, &fAFL);
  ReadFECActiveList();

  // don't publish the rest og the services in order to make the log better
  // understandable
  if (fpMsgBuffer && fpMsgBuffer->Check(eStateError)) return -EACCES;

  RcuBranchLayout* pLayout=fpBranchLayout;
  if (pLayout) {
    int count=0;
    int* layoutArray=NULL;

    // try to get fixed numbers per branch
    int nofA=pLayout->GetNofFecBranchA();
    int nofB=pLayout->GetNofFecBranchB();
    if (nofA>fMaxFecA) {
      nofA=fMaxFecA;
      CE_Error("no of FECs in branch A defined by branch layout exceeds maximum of %d\n", fMaxFecA);      
    }
    if (nofB>fMaxFecB) {
      nofB=fMaxFecB;
      CE_Error("no of FECs in branch B defined by branch layout exceeds maximum of %d\n", fMaxFecB);      
    }

    // if no numbers where defined, try to get a layout array
    if (nofA<0 && nofB<0) {
      count=pLayout->GetBranchLayout(layoutArray);
    } else {
      if (nofA<0) nofA=0;
      if (nofB<0) nofB=0;
      count=nofA+nofB;
    }
    if (count>fFecIds.size()) {
      count=fFecIds.size();
      CE_Error("no of FECs defined by branch layout exceeds maximum of %d\n", FECActiveList_WIDTH);
    }

    // fill the mapping between FEC IDs and HW addresses
    for (int i=0; i<count; i++) {
      if (layoutArray) {
	if (layoutArray[i]>=0) fFecIds[i]=layoutArray[i];
	else fFecIds[i]=-1;
      } else {
	int offset=0;
	if (i>=nofA) offset=fMaxFecA-nofA;
	fFecIds[i+offset]=i;
      }
    }

#ifdef ENABLE_AUTO_DETECTION
    // run autodetection if no FEC were defined
    if (count==0) {
      DetectFECs();
      //checkValidFECs();
    }
#endif //ENABLE_AUTO_DETECTION

    // print layout
    char* strTable=NULL;
    int strSize=GetFECTableString(&strTable);
    if (strTable!=NULL) {
      if (strSize>0)
	CE_Info("\nvalid Frontend cards:\n%s\n\n", strTable);
      else 
	CE_Error("string pointer received but size zero\n"); 
      free(strTable);
    }

    // now create the devices
    vector<int>::iterator element=fFecIds.begin();
    int pos=0;
    while (element!=fFecIds.end()) {
      if (*element>=0) {
	CEfec* fec=pLayout->CreateFEC(*element, this);
	if (fec) {
	  AddSubDevice(fec);
	  fec->Armor();
	  fec->Synchronize();
	  fAFLmask|=0x1<<pos;
	}
      }
      element++;
      pos++;
    }
    delete pLayout;
  }

  string name=GetServiceBaseName();
  name+="_AFL";
  RegisterService(eDataTypeInt, name.c_str(), 0.0, updateRcuRegister, NULL, FECActiveList, eDataTypeInt, this);

  name=GetServiceBaseName();
  name+="_ALTRO_ERRST";
  RegisterService(eDataTypeInt, name.c_str(), 0.0, updateRcuRegister, NULL, AltroErrSt, eDataTypeInt, this);

  return iResult;
}

int CErcu::SetBranchLayout(RcuBranchLayout* pLayout)
{
  int iResult=0;
  if (pLayout!=NULL && fpBranchLayout!=NULL && pLayout!=fpBranchLayout) {
    CE_Warning("Overriding prviously set instance (%p) of branchlayout with %p\n", fpBranchLayout, pLayout);
  }
  fpBranchLayout=pLayout;
  return iResult;
}

// int CErcu::IsAllowedTransition(CETransition* pT) 
// {
//  int iResult=0;
//  return iResult;
// }

int CErcu::EnterStateOFF()
{
  int iResult=0;
  // think about turning the voltage regulator off and disable
  // msgBuffer (rcu bus)
  iResult=SetAltroBusMaster(eAbmDDLsiu);
  return iResult;
}

int CErcu::LeaveStateOFF() 
{
  int iResult=0;
  // think about turning the voltage regulator on and enable
  // msgBuffer (rcu bus)
  return iResult;
}

int CErcu::EnterStateON() 
{
  int iResult=0;
  CE_Debug("enter state ON for CErcu, setting DDL SIU as Altro Bus Master\n");
  iResult=SetAltroBusMaster(eAbmDCSboard);
  return iResult;
}

int CErcu::LeaveStateON() 
{
  int iResult=0;
  return iResult;
}

int CErcu::EnterStateCONFIGURED() 
{
  int iResult=0;
  CE_Debug("enter state CONFIGURED for CErcu, setting DCS board as Altro Bus Master\n");
  iResult=SetAltroBusMaster(eAbmDCSboard);
  return iResult;
}

int CErcu::LeaveStateCONFIGURED() 
{
  int iResult=0;
  return iResult;
}

int CErcu::EnterStateRUNNING() 
{
  int iResult=0;
  CE_Debug("enter state RUNNING for CErcu, setting DDL SIU as Altro Bus Master\n");
  iResult=SetAltroBusMaster(eAbmDDLsiu);
  return iResult;
}

int CErcu::LeaveStateRUNNING() 
{
  int iResult=0;
  return iResult;
}

int CErcu::SwitchOn(int iParam, void* pParam) 
{
  int iResult=0;
  return iResult;
} 

int CErcu::Shutdown(int iParam, void* pParam) 
{
  int iResult=0;
  // switch off the FECs
  RampAFL(0);
  ReadFECActiveList();
  return iResult;
} 

int CErcu::Configure(int iParam, void* pParam) 
{
  int iResult=0;
  CE_Debug("configure CErcu, setting DCS board as Altro Bus Master\n");
  iResult=SetAltroBusMaster(eAbmDCSboard);
  return iResult;
} 

int CErcu::Reset(int iParam, void* pParam) 
{
  int iResult=0;
  return iResult;
} 

int CErcu::Start(int iParam, void* pParam) 
{
  int iResult=0;
  return iResult;
} 

int CErcu::Stop(int iParam, void* pParam) 
{
  int iResult=0;
  return iResult;
}

int CErcu::SetAltroBusMaster(abm_t master) 
{
  int iResult=0;
  if (master==eAbmDDLsiu || master==eAbmDCSboard) {
    if ((iResult=SingleWrite(master, 0x0))<0) {
      CE_Error("can not set Altro Bus Master: error %d\n", iResult);
    }
  } else {
    CE_Error("ivalid parameter, I don't know about Altro Bus Master %#x\n", master);
    iResult=-EINVAL;
  }
  return iResult;
}

int CErcu::UpdateFecService(int &data, int FECid, int reg)
{
  int iResult=0;
  data=0;
  CE_LockGuard g(CErcu::fMutex);
  CEState states[]={eStateOff, eStateOn, eStateConfiguring, eStateConfigured, eStateRunning, eStateInvalid};
  if (Check(states)) {
    CE_LockGuard g(CErcu::fMutex);
    int fecPos=FindFecPosition(FECid);
    if (IsFECposActive(fecPos)) {
      // later we switch between the two versions of the SlowControl
      iResult=ReadFecRegister5(data, fecPos, reg);
    } else {
      iResult=-EACCES;
      CE_Warning("update for FEC id %d called, but seems to be not active\n", FECid);
    }
  } else {
    iResult=-ENOLINK;
  }
  return iResult;
}

int CErcu::ReadFecRegister5(int &data, int pos, int reg)
{
  // This is a special workaround for an RCU firmware bug. Memory access via the DDL SIU
  // interferes with the access of the MSM registers (which is supposed to be possible)
  // -> invalid value in the MSM error register. See below 
  if (fUpdateTempDisable) return 0;
  int iResult=-EIO;
  int nRet=0;
  unsigned short base     = FECCommands;
  unsigned short rnw      = 0x0001;
  unsigned short bcast    = 0x0000;
  unsigned short branch   = pos>=16;
  unsigned short FECAdr   = pos-16*branch;
  unsigned short BCRegAdr = reg;
  __u32 writeAddress = (base) | (rnw<<MSMCommand_rnw) | (bcast<<MSMCommand_bcast) | (branch<<MSMCommand_branch) | (FECAdr<<MSMCommand_FECAdr) | (BCRegAdr<<MSMCommand_BCRegAdr);

  __u32 u32RawData;
  if ((nRet=SingleWrite(FECResetErrReg, 0x0))<0) {
    CE_Warning("can not reset RCU SC Error register: SingleWrite failed (%d)\n", nRet);
  } else if ((nRet=SingleWrite(writeAddress, 0x0))<0) {
    CE_Warning("can not write RCU SC access command: SingleWrite failed (%d)\n", nRet);
  } else {
#ifndef RCUDUMMY
    nRet=SingleRead(FECErrReg, &u32RawData);
#else
    nRet=SingleRead(FECActiveList, &u32RawData);
    if (u32RawData&(0x1<<pos)==0) {
      CE_Warning("FecAccess5: instruction to not active FEC (position %d)\n", pos);
    }
    u32RawData=0;
#endif
    if(nRet<0){
      CE_Warning("FecAccess5: SingleRead failed\n");
    } else if (u32RawData & ~((0x1<<(FECErrReg_WIDTH))-1) ) {
      // invalid value of the error register indicating malfunction of firmware,
      // disabled update for that cycle in order to avoid repeated warnings
      CE_Warning("FecAccess5: invalid error register value %#x; interference from DDL SIU?\n", u32RawData);
      fUpdateTempDisable=1;
    } else if (u32RawData&(0x1<<0)) {
      CE_Warning("FecAccess5: instruction to not active FEC (position %d) error %#x\n", pos, u32RawData);
    } else if (u32RawData&(0x1<<1)) {
      CE_Warning("FecAccess5: no acknowledge from FEC position %d error %#x\n", pos, u32RawData);
    } else if ((nRet=SingleRead(FECResultREG, &u32RawData))<0) {
      CE_Warning("can not read RCU SC result register: SingleRead failed\n", nRet);
    } else {
      iResult=0;
      data=(u32RawData & 0xffff);
      int returnedFecNo=u32RawData>>16;
#ifndef RCUDUMMY
      if(returnedFecNo!=pos){
	CE_Warning("FecAccess5: FEC mismatch, addressed %d, got back %d\n", pos, returnedFecNo);
	iResult=-EBADF;
      }
      //CE_Debug("CErcu::ReadFecRegister5 FEC position %#x register reg %#x value %#x\n", pos, reg, data);
#endif
    }
  }
  return iResult;
}

int CErcu::ReadFecRegister8(int &data, int FEC, int reg)
{
  int nRet=0;
  unsigned short rnw         = 0x0001;
  unsigned short bcast       = 0x0000;
  unsigned short switchLines = 0x0001;
//  unsigned short branch  = pos>=16;
//  unsigned short FECAdr  = pos-16*branch;
//  __u32 SCaddReg  = (reg<<0) | (FECAdr<<8) | (branch<<12) | (bcast<<13) | (rnw<<14);
  __u32 SCaddReg  = (reg<<0) | (FEC<<8) | (bcast<<13) | (rnw<<14);
  __u32 rawData   = 0;
  data=0;
  nRet=SingleWrite(0x8009, switchLines);
  if (nRet<0) {
    CE_Warning("can not write RCU switchLines: SingleWrite failed (%d)\n", nRet);
    return -EBADF;
  }
  nRet=SingleWrite(0x8002, SCaddReg);
  if (nRet<0) {
    CE_Warning("can not write RCU SCaddReg: SingleWrite failed (%d)\n", nRet);
    return -EBADF;
  }
  nRet=SingleWrite(0x8003, data);
  if (nRet<0) {
    CE_Warning("can not write RCU SCdataReg: SingleWrite failed (%d)\n", nRet);
    return -EBADF;
  }
  nRet=SingleWrite(0x8011, 0x0);
  if (nRet<0) {
    CE_Warning("can not write RCU rst_ERRORreg: SingleWrite failed (%d)\n", nRet);
    return -EBADF;
  }
  nRet=SingleWrite(0x8012, 0x0);
  if (nRet<0) {
    CE_Warning("can not write RCU rst_RESULTreg: SingleWrite failed (%d)\n", nRet);
    return -EBADF;
  }
  nRet=SingleWrite(0x8010, 0x0);
  if (nRet<0) {
    CE_Warning("can not write RCU SCDcmdReg: SingleWrite failed (%d)\n", nRet);
    return -EBADF;
  }
#ifndef RCUDUMMY
  nRet=SingleRead(0x8005, &rawData);
  if (nRet<0) {
    CE_Warning("can not read RCU ERROR: SingleRead failed (%d)\n", nRet);
    return -EBADF;
  }
  if (rawData&(0x1<<0)) {
    CE_Warning("instruction to not active FEC (position %d)\n", FEC);
    return -EBADF;
  }
  if (rawData&(0x1<<1)) {
    CE_Warning("no acknowledge from FEC %d\n", FEC);
    return -EBADF;
  }
#else
  nRet=SingleRead(FECActiveList, &rawData);
  if (!rawData&(0x1<<FEC)) {
    CE_Warning("instruction to not active FEC (position %d)\n", FEC);
    return -EBADF;
  }
#endif
  rawData=0;
  nRet=SingleRead(0x8004, &rawData);
  if(nRet) {
    CE_Warning("can not read RCU SC RESULT: SingleRead failed\n", nRet);
    return -EBADF;
  }
  data=(rawData & 0xffff);
#ifndef RCUDUMMY
  if((rawData>>16)!=FEC){
    CE_Warning("FEC mismatch, addressed %d, got back %d\n", FEC, (rawData>>16));
    return -EBADF;
  }
#endif
  return 0;
}

int CErcu::WriteFecRegister(int data, int FECid, int reg)
{
  int iResult=0;
  CE_LockGuard g(CErcu::fMutex);
  // later we have to switch between the different versions of the SlowControl
  int fecPos=FindFecPosition(FECid);
  if (fecPos>=0) iResult=WriteFecRegister5(data, fecPos, reg);
  return iResult;
}

int CErcu::WriteFecRegister5(int data, int FEC, int reg)
{
  int iResult=-EIO;
  int nRet=0;
  unsigned short base     = FECCommands;
  unsigned short rnw      = 0x0000;
  unsigned short bcast    = 0x0000;
  unsigned short branch   = FEC>=16;
  unsigned short FECAdr   = FEC-16*branch;
  unsigned short BCRegAdr = reg;
  __u32 writeAddress = (base) | (rnw<<MSMCommand_rnw) | (bcast<<MSMCommand_bcast) | (branch<<MSMCommand_branch) | (FECAdr<<MSMCommand_FECAdr) | (BCRegAdr<<MSMCommand_BCRegAdr);
  //CE_Debug("write FEC %d register %#x command %#x data %#x\n", FEC, reg, writeAddress, data);
  if ((nRet=SingleWrite(writeAddress, data))<0) {
    CE_Warning("can not write RCU SC access command: SingleWrite failed (%d)\n", nRet);
  } else {
    iResult=0;
  }
}

int CErcu::WriteFecRegister8(int data, int FEC, int reg)
{
  int nRet=-0;
  unsigned short rnw         = 0x0000;
  unsigned short bcast       = 0x0000;
  unsigned short switchLines = 0x0001;
//  unsigned short branch  = FEC>=16;
//  unsigned short FECAdr  = FEC-16*branch;
//  __u32 SCAddReg  = (reg<<0) | (FECAdr<<8) | (branch<<12) | (bcast<<13) | (rnw<<14);
  __u32 SCaddReg  = (reg<<0) | (FEC<<8) | (bcast<<13) | (rnw<<14);
  nRet=SingleWrite(0x8009, switchLines);
  if (nRet<0) {
    CE_Warning("can not write RCU switchLines: SingleWrite failed (%d)\n", nRet);
    return -EBADF;
  }
  nRet=SingleWrite(0x8002, SCaddReg);
  if (nRet<0) {
    CE_Warning("can not write RCU SCaddReg: SingleWrite failed (%d)\n", nRet);
    return -EBADF;
  }
  nRet=SingleWrite(0x8003, data);
  if (nRet<0) {
    CE_Warning("can not write RCU SCdataReg: SingleWrite failed (%d)\n", nRet);
    return -EBADF;
  }
  nRet=SingleWrite(0x8010, 0x0);
  if (nRet<0) {
    CE_Warning("can not write RCU SCDcmdReg: SingleWrite failed (%d)\n", nRet);
    return -EBADF;
  }
  return 0;
}

int CErcu::UpdateRcuRegister(TceServiceData* pData, int address, int type)
{
  int iResult=0;
  if (pData) {
    __u32 data=0;
    iResult=SingleRead(address, &data);
    if (type==eDataTypeInt) {
      pData->iVal=data;
    } else if (type==eDataTypeFloat) {
      pData->fVal=data;
    }
  } else {
    iResult=-EINVAL;
  }
  return iResult;
}

int CErcu::MaxFecsInBranch(unsigned short branch)
{
  unsigned maxFECAdr=0;
  maxFECAdr = (branch == 0x0) ? fMaxFecA : fMaxFecB;
  return maxFECAdr;
}

int CErcu::GetBranch(int index)
{
  unsigned short branch=0x000;
  (index < MAX_BRANCH_A_FEC) ? (branch = 0x000) : (branch = 0x001);
  return branch;
}

int CErcu::FindFecPosition(int id)
{
  int iResult=-ENOENT;
  for (int i=0; i<fFecIds.size() && iResult==-ENOENT; i++) {
    if (fFecIds[i]==id) {
      iResult=i;
      break;
    }
  }
  return iResult;
}

int CErcu::GetFecPosition(int id, int &branch, int &addr)
{
  int iResult=-ENODEV;
  for (int i=0; i<fFecIds.size(); i++) {
    if (fFecIds[i]==id) {
      branch=GetBranch(i);
      addr=i;
      if (branch>0) 
	addr-=MaxFecsInBranch(i);
      iResult=0;
      break;
    }
  }
  return iResult;
}

int CErcu::TranslateRcuCommand(__u32 cmd, __u32 parameter, const char* pData, int iDataSize, CEResultBuffer& rb) {
  int iResult=0;
  int iBitWidth=8;
  CE_Debug("CErcu::TranslateRcuCommand cmd=%#x parameter=%#x datasize=%d\n", cmd, parameter, iDataSize);
  switch (cmd) {
  case RCU_WRITE_INSTRUCTION:
  case RCU_EXEC_INSTRUCTION:
    iResult=writeDataBlockToRCUMem(pData, iDataSize, parameter, 32, 
				   ALTROInstMEM, ALTROInstMEM_SIZE);
    if (iResult>=0) {
      if (cmd==RCU_EXEC_INSTRUCTION) {
	iResult=sendRCUExecCommand(0);
      }
    }
    break;
  case RCU_WRITE_PATTERN32:
    iBitWidth*=2;
    // fall through intended
  case RCU_WRITE_PATTERN16:
    iBitWidth*=2;
    // fall through intended
  case RCU_WRITE_PATTERN8:
    iResult=writeDataBlockToRCUMem(pData, iDataSize, parameter, iBitWidth, 
				   ALTROPatternMEM, ALTROPatternMEM_SIZE);
    break;
  case RCU_WRITE_PATTERN10:
    iResult=writeDataBlockToRCUMem(pData, iDataSize, parameter, 10, 
				   ALTROPatternMEM, ALTROPatternMEM_SIZE);
    break;
  case RCU_EXEC:
    // TODO: in conjunction with the pre-scanning to be implemented there has to be 
    // a version check of the command set
    iResult=sendRCUExecCommand(parameter);
    if (iResult>=0) iResult=0;
    break;
  case RCU_STOP_EXEC:
    iResult=sendStopRCUExec();
    if (iResult>=0) iResult=0;
    break;
  case RCU_READ_INSTRUCTION:
    {
      iResult=readRcuMemoryIntoResultBuffer(ALTROInstMEM, parameter, ALTROInstMEM_SIZE, rb);
      if (iResult>=0) iResult=0;
    }
    break;
  case RCU_READ_PATTERN:
    {
      iResult=readRcuMemoryIntoResultBuffer(ALTROPatternMEM, parameter, ALTROPatternMEM_SIZE, rb);
      if (iResult>=0) iResult=0;
    }
    break;
  case RCU_READ_MEMORY:
    iResult=readRcuMemoryIntoResultBuffer(parameter, 1, 0, rb);
    if (iResult>=0) iResult=0;
    break;
  case RCU_WRITE_MEMORY:
    iResult=writeDataBlockToRCUMem(pData, iDataSize, 1, 32, 
				   parameter, 0);
    break;
  case RCU_WRITE_RESULT:
    iResult=writeDataBlockToRCUMem(pData, iDataSize, parameter, 32, 
				   ALTROResultMEM, ALTROResultMEM_SIZE);
    break;
  case RCU_READ_RESULT:
    {
      iResult=readRcuMemoryIntoResultBuffer(ALTROResultMEM, parameter, ALTROResultMEM_SIZE, rb);
      if (iResult>=0) iResult=0;
    }
    break;
  case RCU_WRITE_MEMBLOCK: // write block to rcu memory {parameter+1}
    // the first 32 bit word of the data is the address to write to, the data is following
    if ((parameter+1)*sizeof(__u32)<=iDataSize) {
      __u32 u32Address=*((__u32*)pData);
      if (u32Address<0x10000 && parameter<0x10000) {
	if (u32Address+parameter>0xffff) {
	  CE_Error("data buffer (%d) exceeds size of rcu address range\n", parameter);
	} else {
	  iResult=writeDataBlockToRCUMem(pData+sizeof(__u32), iDataSize, parameter, 32, 
					 u32Address, 0);
	  if (iResult>=0) iResult+=sizeof(__u32); // for the address
	}
      } else {
	CE_Error("address/count exceeds rcu address range\n");
	iResult=-EINVAL;
      }
    } else {
      CE_Error("data size missmatch, %d byte available in buffer but %d requested by command (%#x)\n", iDataSize, (parameter+1)*sizeof(__u32), cmd);
      iResult=-EINVAL;
    }
    break;
  case RCU_READ_MEMBLOCK: // read block from rcu memory {1}
    // number of data words to read specified by parameter, address is the 32 bit word following the command id
    if (iDataSize>=sizeof(__u32)) {
      __u32 address=*((__u32*)pData);
      iResult=readRcuMemoryIntoResultBuffer(address, parameter, 0, rb);
      if (iResult>=0) iResult=sizeof(__u32); // the address word in the payload
    }
    break;
  case RCU_CHECK_ERROR:
    rb.resize(sizeof(__u32)/sizeof(CEResultBuffer::value_type), 0);
    rb[0]=checkRCUError();
    iResult=0;
    break;
  case RCU_EN_AUTO_CHECK:
    if (parameter!=0) {
      iResult=SetOptionFlag(RCU_OPT_AUTO_CHECK);
      CE_Info("switch on automatic check of rcu sequencer (%0x)\n", iResult);
    } else {
      iResult=ClearOptionFlag(RCU_OPT_AUTO_CHECK);
      CE_Info("switch off automatic check of rcu sequencer (%0x)\n", iResult);
    }
    iResult=0;
    break;
  case RCU_READ_ERRST:
    iResult=readRcuMemoryIntoResultBuffer(AltroErrSt, 1, AltroErrSt_SIZE, rb);
    if (iResult>=0) iResult=0;
    break;
  case RCU_WRITE_TRGCFG:
    iResult=writeDataBlockToRCUMem(pData, iDataSize, 1, 32, 
				   AltroTrCfg, AltroTrCfg_SIZE);
    break;
  case RCU_READ_TRGCFG:
    iResult=readRcuMemoryIntoResultBuffer(AltroTrCfg, 1, AltroTrCfg_SIZE, rb);
    if (iResult>=0) iResult=0;
    break;
  case RCU_WRITE_PMCFG:
    iResult=writeDataBlockToRCUMem(pData, iDataSize, 1, 32, 
				   AltroPmCfg, AltroPmCfg_SIZE);
    break;
  case RCU_READ_PMCFG:
    iResult=readRcuMemoryIntoResultBuffer(AltroPmCfg, 1, AltroPmCfg_SIZE, rb);
    if (iResult>=0) iResult=0;
    break;
  case RCU_WRITE_AFL:
    {
      __u32* pAFL=(__u32*)pData;
      if (*pAFL!=0 && fAFLmask==0) {
	CE_Warning("RCU_WRITE_AFL ignored, no FECs defined\n");
      }
      *pAFL&=fAFLmask;
      iResult=writeDataBlockToRCUMem(pData, iDataSize, 1, 32, 
				     FECActiveList, FECActiveList_SIZE);
    }
    break;
  case RCU_READ_AFL:
    iResult=readRcuMemoryIntoResultBuffer(FECActiveList, 1, FECActiveList_SIZE, rb);
    if (iResult>=0) iResult=0;
    break;
  case RCU_WRITE_ACL:
    iResult=writeDataBlockToRCUMem(pData, iDataSize, parameter, 32, 
				   ALTROACL, ALTROACL_SIZE);
    break;
  case RCU_READ_ACL:
    iResult=readRcuMemoryIntoResultBuffer(ALTROACL, parameter, ALTROACL_SIZE, rb);
    if (iResult>=0) iResult=0;
    break;
  case RCU_WRITE_HEADER:
    iResult=writeDataBlockToRCUMem(pData, iDataSize, parameter, 32, 
				   DataHEADER, DataHEADER_SIZE);
    break;
  case RCU_READ_HEADER:
    iResult=readRcuMemoryIntoResultBuffer(DataHEADER, parameter, DataHEADER_SIZE, rb);
    if (iResult>=0) iResult=0;
    break;
  case RCU_RESET:
    {
      __u32 dummy=0;
      __u32 cmd=CMDRESET;
      switch (parameter) {
      case 1: cmd=CMDRESETFEC; break;
      case 2: cmd=CMDRESETRCU; break;
      }
      writeDataBlockToRCUMem((const char*)&dummy, sizeof(__u32), 1, 32, cmd, 1);
      iResult=0;
    }
    break;
  case RCU_L1TRG_SELECT:
    {
      __u32 dummy=0;
      __u32 cmd=CMDL1CMD;
      switch (parameter) {
      case 1: cmd=CMDL1TTC; break;
      case 2: cmd=CMDL1I2C; break;
      }
      writeDataBlockToRCUMem((const char*)&dummy, sizeof(__u32), 1, 32, cmd, 1);
      iResult=0;
    }
    break;
  case RCU_SEND_L1_TRIGGER:
    {
      __u32 dummy=0;
      __u32 cmd=CMDL1;
      writeDataBlockToRCUMem((const char*)&dummy, sizeof(__u32), 1, 32, cmd, 1);
      iResult=0;
    }
    break;
  case RCU_READ_FW_VERSION:
    rb.resize(sizeof(__u32)/sizeof(CEResultBuffer::value_type), 0);
    *((__u32*)&rb[0])=fFwVersion;
    break;
  default:
    CE_Warning("unrecognized command id (%#x)\n", cmd);
    iResult=-ENOSYS;
  }
  return iResult;
}

int CErcu::SingleWrite(__u32 address, __u32 data)
{
  if (fpMsgBuffer)
    return fpMsgBuffer->SingleWrite(address, data);
  return -ENODEV;
}

int CErcu::SingleRead(__u32 address, __u32* pData)
{
  if (fpMsgBuffer)
    return fpMsgBuffer->SingleRead(address, pData);
  return -ENODEV;
}

int CErcu::MultipleWrite(__u32 address, __u32* pData, int iSize, int iDataSize)
{
  if (fpMsgBuffer)
    return fpMsgBuffer->MultipleWrite(address, pData, iSize, iDataSize);
  return -ENODEV;
}

int CErcu::MultipleRead(__u32 address, int iSize,__u32* pData)
{
  if (fpMsgBuffer)
    return fpMsgBuffer->MultipleRead(address, iSize,pData);
  return -ENODEV;
}

int CErcu::checkRCUError()
{
  int iResult=0;
  __u32 reg=0;
  int busy=0;
  const int nofBusyLoops=3;
  //CE_LockGuard g(fMutex);
  do {
    iResult=SingleRead(AltroErrSt, &reg);
    if (iResult>=0) {
      reg&=~RCU_HWADDR_MASK;
      if (reg>0) {
	if (reg&RCU_SEQ_BUSY) {
	  if (++busy < nofBusyLoops) {
	    continue;
	  } else if (busy == nofBusyLoops) {
	    usleep(10);
	    continue;
	  } else {
	    CE_Error("RCU sequencer remains in busy state\n");
	    iResult=-EBUSY;
	  }
	} else if (reg&SEQ_ABORT) {
	  CE_Warning("RCU sequencer reports SEQ_ABORT \n");
	  iResult=-EINTR;
	} else if (reg&FEC_TIMEOUT) {
	  CE_Error("RCU sequencer reports error FEC_TIMEOUT \n");
	  iResult=-ETIMEDOUT;
	} else if (reg&ALTRO_ERROR) {
	  CE_Error("RCU sequencer reports error ALTRO_ERROR \n");
	  iResult=-EIO;
	} else if (reg&ALTRO_HWADD_ERROR) {
	  CE_Error("RCU sequencer reports error ALTRO_HWADD_ERROR \n");
	  iResult=-EIO;
	} else if (reg&PATTERN_MISSMATCH) {
	  CE_Warning("RCU sequencer reports error PATTERN missmatch \n");
	  iResult=-EBADF;
	}
      }
    } else {
      CE_Error("failed to read from RCU memory\n");
    }
  } while (iResult>=0 && reg>0);
  return iResult;
}

int CErcu::sendRCUExecCommand(unsigned short start)
{
  int iResult=0;
  CE_LockGuard g(fMutex);
  SingleWrite(CMDResAltroErrSt, 0);
  if ((iResult=SingleWrite(CMDExecALTRO, start))>=0) {
    CE_Debug("instruction buffer executed\n");
    if (CheckOptionFlag(RCU_OPT_AUTO_CHECK)) {
      iResult=checkRCUError();
    }
  } else {
    CE_Error("altro exec command failed, error=%d", iResult);
  }
  return iResult;
}

int CErcu::sendStopRCUExec()
{
  int iResult=0;
  CE_LockGuard g(fMutex);
  if ((iResult=SingleWrite(CMDAbortALTRO, 0))<0) {
    CE_Error("attemp to stop execution failed with error %d\n", iResult);
  }
  return iResult;
}

int CErcu::writeCommandBufferToRCUMem(const char* pBuffer, int iCount, int iWordSize, __u32 u32BaseAddress)
{
  int iResult=0;
  const char* pSrc=pBuffer;
  __u32 u32Address=u32BaseAddress;
  __u32 data=0;
  int i=0;
  CE_LockGuard g(fMutex);
  if (iWordSize==4 || iWordSize==2) {
    if (ceCheckOptionFlag(DEBUG_USE_SINGLE_WRITE)) {
      for (i=0;i<iCount && iResult>=0;i++) {
	data=*((__u32*)pSrc);
	if (iWordSize==2)
	  data&=0x0000ffff;
	else if (iWordSize!=4) {
	  CE_Error("bit width %d not supported by the debug version\n", iWordSize);
	  iResult=-ENOSYS;
	  break;
	}
	if (u32Address==FECActiveList) {
	  CEState whiteList[]={eStateOn, eStateConfiguring, eStateInvalid};
	  if (Check(whiteList)) {
	    iResult=RampAFL(data);
	    ReadFECActiveList();
	  } else {
	    CE_Warning("switching of FECs locked for state %s", GetCurrentStateName());
	  }
	} else
	  iResult=SingleWrite(u32Address, data);
	pSrc+=iWordSize;
	u32Address++;
      }
    } else {
      CE_Debug("writeCommandBufferToRCUMem base address %#x\n", u32Address);
      const char* pWork=pBuffer;
      int iWorkCount=iCount;
      if (u32BaseAddress<=FECActiveList && u32BaseAddress+iCount>FECActiveList) {
	int before=FECActiveList-u32BaseAddress;
	if (before>0) {
	  iResult=MultipleWrite(u32BaseAddress, (__u32*)pWork, before, iWordSize);
	  pWork+=before*sizeof(__u32);
	  iWorkCount-=before;
	  u32Address+=before;
	}
	if (iResult>=0) {
	  CEState whiteList[]={eStateOn, eStateConfiguring, eStateInvalid};
	  if (Check(whiteList)) {
	    iResult=RampAFL(*((__u32*)pWork));
	    ReadFECActiveList();
	  } else {
	    CE_Warning("switching of FECs locked for state %s\n", GetCurrentStateName());
	  }
	  pWork+=sizeof(__u32);
	  iWorkCount++;
	  u32Address++;
	}
      }
      if (iResult>=0 && iWorkCount>0)
	iResult=MultipleWrite(u32Address, (__u32*)pWork, iCount, iWordSize);
    }
    if (iResult>=0) {
      CE_Debug("wrote %d words to rcu buffer 0x%08x with result %d\n", iCount, u32BaseAddress, iResult);
      iResult=iCount;
    } else {
      CE_Error("writing to rcu buffer 0x%08x aborted after %d words, result %d\n", u32BaseAddress, i, iResult);
    }
  } else {
    CE_Error("writeCommandBufferToRCUMem: invalid word size\n");
  }
  return iResult;
}

int CErcu::writeDataBlockToRCUMem(const char* pBuffer, int iSize, int iNofWords, int iWordSize,
				  __u32 tgtAddress, int partSize)
{
  int iResult=0;
  // check the width of the data words
  int iWordsPer32=0;
  if (iWordSize==8 || iWordSize==16 || iWordSize==32 || iWordSize==10) {
    iWordsPer32=32/iWordSize;
  } else if (iWordSize==0) {
    iWordsPer32=1;
  } else {
    CE_Error("bit width %d not supported\n", iWordSize);
    iResult=-ENOSYS;
  }

  // write the data
  if (iResult>=0 && iWordsPer32>0) {
    // align to 32 bit
    int iMinBufferSize=(((iNofWords-1)/iWordsPer32)+1)*sizeof(__u32);
    if (iMinBufferSize<=iSize){
      if (partSize>0 && iNofWords>partSize) {
	CE_Error("buffer (size %d) exceeds size of rcu memory partition %#x (%d)\n", iNofWords, tgtAddress, partSize);
      } else {
	/* for some historical reason, the 'align' parameter of the rcuMultipleWrite method
	   of the dcscMsgBuffer interface has the following meaning:
	   1 = 8 bit, 2 = 16 bit, 4 = 32 bit and 3 = 10 bit (although the last one is not 
	   logical)
	*/
	int iAlign=0;
	switch (iWordsPer32) {
	case 1: iAlign=4; break;
	case 4: iAlign=1; break;
	default: iAlign=iWordsPer32;
	}
	if ((iResult=writeCommandBufferToRCUMem(pBuffer, iNofWords, iAlign, tgtAddress))>=0) {
	  iResult=iMinBufferSize;
	  CE_Debug("data block of %d words written to rcu memory at %#x\n", iNofWords, tgtAddress);
	}
      }
    } else {
      CE_Error("too little data (%d byte(s)) to write %d %d-bit word(s)\n", iSize, iNofWords, iWordSize);
      iResult=-ENODATA;
    }
  }
  return iResult;
}

int CErcu::readRcuMemoryIntoResultBuffer(__u32 address, int size, int partSize,
					 CEResultBuffer& rb)
{
  int iResult=0;
  if (address>0xffff) {
    CE_Error("address %#x exceeds RCU memory space\n", address);
    return -EINVAL;
  }
  if (partSize>0 && size>partSize) {
    size=partSize;
    CE_Warning("requested size exceeds size of partition %#x, truncated\n", address);
  }
  if (address+size>0xffff) {
    size=0x10000-address;
    CE_Warning("requested size exceeds size of RCU memory space, truncated\n");    
  }
  int iCurrent=rb.size();
  rb.resize(iCurrent+size, 0);
  CE_LockGuard g(fMutex);
  int i=0;
    if (ceCheckOptionFlag(DEBUG_USE_SINGLE_WRITE)) {
      __u32 addr=address;
      for (i=0; addr<address+size && iResult>=0; addr++, i++) {
	iResult=SingleRead(addr, &rb[iCurrent+i]);
      }
      if (iResult>=0) iResult=size;
    } else {
      iResult=MultipleRead(address, size, &rb[iCurrent]);
    }
    if (iResult<size) {
      // shrink the array if there were less words read
      rb.resize(iCurrent+(iResult<0?0:iResult));
    } else if (iResult>size) {
      CE_Warning("result missmatch: %d words read, but %d requested to read\n", iResult, size);
      iResult=size;
    }
  return iResult;
}

int CErcu::PublishServices()
{
  return 0;
}

int CErcu::PublishRegs()
{
  return 0;
}

int CErcu::PublishReg(int regNum)
{
  return 0;
}

int CErcu::ReadFECActiveList()
{
  int iResult=0;
  CEState states[]={eStateOff, eStateOn, eStateConfiguring, eStateConfigured, eStateRunning, eStateInvalid};
  if (Check(states)) {
    // enable the update
    // ReadFECActiveList is called at the beginning of each update cycle. If there was a MSM acces
    // failure (invalid value of the error register indicating malfunction of firmware), the update
    // had been disabled for that cycle in order to avoid repeated warnings
    fUpdateTempDisable=0;
    CE_LockGuard g(CErcu::fMutex);
    __u32 afl=0;
    if (SingleRead(FECActiveList, &afl)>=0) {
      //CE_Debug("afl %#x fAFL %#x\n", afl, fAFL);
      if (1/*afl!=fAFL*/) {
	//CE_Debug("afl=%#x fAFL=%#x\n", afl, fAFL);
	// We call the switch on handler for the FEC below, there is another cross check
	// in SwitchAFL, therefore the fAFL backup has to be set already here and a 
	// copy is used further down. 
	fAFL=afl;
	int bSleep=1;
	for (int i=0; i<8*sizeof(__u32) && i<fFecIds.size(); i++){
	  if (fFecIds[i]>=0) {
	    CEDevice* pFEC=FindDevice(fFecIds[i]);
	    if (pFEC) {
	      CEState offStates[]={eStateOff, eStateError, eStateInvalid};
	      int switchFec=(afl&((__u32)0x1<<i))==0 ^ pFEC->Check(offStates);
	      int switchOn=(afl&((__u32)0x1<<i))!=0;
	      if (switchFec) {
		CE_Debug("FEC position %d changed\n", i);
		if (switchOn!=0 && bSleep==1) {
		  // sleep if the first FEC to be switched on is encountered
		  ce_usleep(500000);
		  bSleep=0;
		}
		const char* transition="SwitchOn";
		// The AFL has been changed, if the FEC is not off, switch it off
		// later we have to check if the FEC was turned off automatically
		// and have to go to error.
		if (switchOn==0) {
		  transition="Shutdown";
		} else if (pFEC->Check(eStateError) || pFEC->Check(eStateFailure)) {
		  transition="Reset";
		}
		CE_Info("FEC position %d , %s (AFL %#x)\n", i, transition, afl);
		pFEC->TriggerTransition(transition);
	      }
	    }
	  }
	}
      }
    }
  }
  return iResult;;
}

int CErcu::SwitchAFL(int fecid, int on)
{
  int iResult=0;
  CE_LockGuard g(CErcu::fMutex);
  int pos=FindFecPosition(fecid);
  if (pos>=0) {
    __u32 afl=0;
    if ((iResult=SingleRead(FECActiveList, &afl))>=0) {
      CE_Debug("afl=%#x fAFL=%#x %s\n", afl, fAFL, on==0?"Shutdown":"SwitchOn");
      if (((afl&(0x1<<pos))!=0) ^ (on!=0)) {
	// not synchronous
	if (on==0) {
	  // switch off
	  CE_Debug("switch FEC id %d pos %#x off\n", fecid, pos);
	  afl&=~(((__u32)0x1)<<pos);
	} else {
	  // switch on
	  CE_Debug("switch FEC id %d pos %#x on\n", fecid, pos);
	  afl|=((__u32)0x1)<<pos;
	}
	// FEC power up and down has to be ramped, wait if last access was
	// less than one second ago
	time_t current;
	time(&current);
	double diff=difftime(current, fAflAccess);
	CE_Debug("last AFL access %f seconds ago\n", diff);
	if (diff<1.0) ce_sleep(1);
	if ((iResult=SingleWrite(FECActiveList, afl))>=0) {
	  //CE_Debug("wrote AFL %#x\n", afl);
	  iResult=1; // indicate that we switched something
	}
	time(&fAflAccess);
      }
      fAFL=afl;
    }
  }
  return iResult;
}

int CErcu::RampAFL(__u32 setAfl)
{
  int iResult=0;
  __u32 afl=0;
  __u32 changedBits=0;
  CE_LockGuard g(CErcu::fMutex);
  if ((iResult=SingleRead(FECActiveList, &afl))>=0 &&
      (changedBits=afl^setAfl)>0) {
    CEState tmpState=eStateUnknown;
    CEDevice* pParent=GetParentDevice();
    if (pParent) {
      CEStateMapper* pMapper=dynamic_cast<PVSSStateMapper*>(pParent->GetTranslationScheme());
      if (pMapper) {
	if ((changedBits&setAfl)==0) {
	  // all bits which we change are ZERO -> RAMPING_DOWN
	  tmpState=pMapper->GetStateFromMappedState(PVSSStateMapper::kStateRampingDown);
	} else {
	  // there is at least one FEC to switch on -> RAMPING_UP
	  tmpState=pMapper->GetStateFromMappedState(PVSSStateMapper::kStateRampingUp);
	}
      }
    }
    StateSwitchGuard g(pParent, tmpState);
    CE_Debug("Ramp AFL from %#x to %#x\n", afl, setAfl);
    time_t current;
    time(&current);
    double diff=difftime(current, fAflAccess);
    CE_Debug("last AFL access %f seconds ago\n", diff);
    int wait=0;
    if (diff<1.0) wait=1;
    for (int i=0; i<FECActiveList_WIDTH; i++) {
      if ((afl&(0x1<<i))^(setAfl&(0x1<<i))) {
	if (wait>0) ce_sleep(1);
	wait=1;
	afl^=0x1<<i;
	CE_Debug("write afl %#x\n", afl);
	if ((iResult=SingleWrite(FECActiveList, afl))>=0) {
	  iResult=1; // indicate that we switched something
	}
	fAFL=afl;
	CEDevice* pFEC=FindDevice(fFecIds[i]);
	if (pFEC) {
	  const char* transition="SwitchOn";
	  if ((afl&(0x1<<i))==0) {
	    transition="Shutdown";
	  } else if (pFEC->Check(eStateError) || pFEC->Check(eStateFailure)) {
	    transition="Reset";
	  }
	  CE_Info("FEC %d (position %d) %s\n", pFEC->GetDeviceId(), i, transition);
	  pFEC->TriggerTransition(transition);
	  ceUpdateServices(pFEC->GetServiceBaseName(), 0);
	}
      }
    }
    time(&fAflAccess);
  }
  return iResult;
}

int CErcu::GetFECTableString(char** ppTgt)
{
  int iResult=0;
  if (fFecIds.size()>0) {
    if (ppTgt) {
      int cards_per_branch[NOF_BRANCHES];
      int branch_rows[NOF_BRANCHES];
      const char* branch_name[NOF_BRANCHES];
      int branch_card_offset[NOF_BRANCHES];
      cards_per_branch[0]=fMaxFecA;
      cards_per_branch[1]=fMaxFecB;
      branch_rows[0]=0;
      branch_rows[1]=0;
      branch_name[0]="A";
      branch_name[1]="B";
      branch_card_offset[0]=FEC_BRANCH_A_TO_GLOBAL(0);
      branch_card_offset[1]=FEC_BRANCH_B_TO_GLOBAL(0);
      int branch=0;
      int i=0;
      int row=0;
      int cardsPerRow=10;
      int nofRows=0;
      for (branch=0; branch<NOF_BRANCHES; branch++) {
	if (cards_per_branch[branch]>0)
	  branch_rows[branch]=1+(cards_per_branch[branch]-1)/cardsPerRow;
	nofRows+=branch_rows[branch];
      }
      char* strTable=NULL;
      const char* blank=" ";
      int iSizeTableString=(cardsPerRow*3 + 12)*3*nofRows+nofRows;
      strTable=(char*)malloc(iSizeTableString);
      if (strTable) {
	memset(strTable, 0, iSizeTableString);
	branch=0;
	for (branch=0; branch<NOF_BRANCHES; branch++) {
	  for (row=0;  row<branch_rows[branch]; row++) {
	    // header line
	    if (strlen(strTable)>0)
	      sprintf(strTable, "%s\n\nBRANCH %s #:", strTable, branch_name[branch]);
	    else
	      sprintf(strTable, "BRANCH %s #:", branch_name[branch]);
	    for (i=row*cardsPerRow; i<(row+1)*cardsPerRow && i<cards_per_branch[branch]; i++) {
	      if (i<10) blank=" ";
	      else blank="";
	      sprintf(strTable, "%s %s%d", strTable, blank, i);
	    }
	    sprintf(strTable, "%s\n===========", strTable);
	    for (i=row*cardsPerRow; i<(row+1)*cardsPerRow && i<cards_per_branch[branch]; i++) {
	      sprintf(strTable, "%s===", strTable);
	    }
	    sprintf(strTable, "%s\n           ", strTable);
	    for (i=row*cardsPerRow; i<(row+1)*cardsPerRow && i<cards_per_branch[branch]; i++) {
	      sprintf(strTable, "%s  %d", strTable, fFecIds[branch_card_offset[branch]+i]>=0?1:0);
	    }
	  }
	}
      }
      *ppTgt=strTable;
      iResult=iSizeTableString;
    } else {
      iResult=-EINVAL;
    }
  } else {
    const char* message="no Front-End Cards defined";
    iResult=strlen(message);
    *ppTgt=(char*)malloc(iResult+1);
    if (*ppTgt) strcpy(*ppTgt, message);
    else iResult=-ENOMEM;
  }
  return iResult;
}

int CErcu::IsFECvalid(int i) {
  int iResult=0;
  iResult=FindDevice(i)!=NULL;
  return iResult;
}

int CErcu::IsFECactive(int id) {
  int iResult=0;
  int pos=FindFecPosition(id);
  if (pos>=0) 
    iResult=IsFECposActive(pos);
  return iResult;
}

int CErcu::IsFECposActive(int i) {
  int iResult=0;
  if (i>=0 && i<8*sizeof(fAFL)) {
    iResult=(fAFL&(0x1<<i))!=0;
  } else {
    CE_Error("CErcu::IsFECposActive pos %d out of range [0,%d]\n", i, 8*sizeof(fAFL));
  }
  return iResult;
}

int CErcu::MakeRegItemLists()
{
  int ceState = 0;
#ifdef OLD_SERVICE_HANDLING
  char* serviceAdr  = NULL;
  char* serviceName = NULL;
  char* serviceDead = NULL;
  serviceAdr  = getenv("FEESERVER_SERVICE_ADR" );
  serviceName = getenv("FEESERVER_SERVICE_NAME");
  serviceDead = getenv("FEESERVER_SERVICE_DEAD");
  g_regItemAdr   = cnvAllExtEnvI(splitString(serviceAdr ));
  g_pRegItemName =               splitString(serviceName);
  g_deadband     = cnvAllExtEnvF(splitString(serviceDead));
  if(g_regItemAdr!=NULL && g_pRegItemName!=NULL && g_deadband!=NULL){
    if((g_regItemAdr!=NULL && (g_pRegItemName==NULL || g_deadband==NULL)) || (g_pRegItemName!=NULL && (g_regItemAdr==NULL || g_deadband==NULL)) || (g_deadband!=NULL && (g_pRegItemName==NULL || g_regItemAdr==NULL))){
      ceState=FEE_FAILED;
      CE_Error("Environment variables FEESERVER_SERVICE_ADR, FEESERVER_SERVICE_DEAD  and FEESERVER_SERVICE_NAME do not match\n");
      fflush(stdout);
      return ceState;
    }
    g_numberOfAdrs = sizeof(g_regItemAdr)/sizeof(g_regItemAdr[0]);
    if(g_numberOfAdrs!=(sizeof(g_pRegItemName)/sizeof(g_pRegItemName[0])) || g_numberOfAdrs!=(sizeof(g_deadband)/sizeof(g_deadband[0]))){
      ceState=FEE_FAILED;
      CE_Error("Environment variables FEESERVER_SERVICE_NAME and FEESERVER_SERVICE_ADR do not match\n");
      fflush(stdout);
      return ceState;
    }
    g_pRegItem     = (Item**) malloc(g_numberOfAdrs*sizeof(Item*));
    g_regItemValue = (float*) malloc(g_numberOfAdrs*sizeof(float));
    if (g_pRegItem==0 || g_deadband==0 || g_regItemValue==0){
      ceState=FEE_INSUFFICIENT_MEMORY;
      CE_Error("no memory available\n");
      fflush(stdout);
      return ceState;
    }
  }
#endif //OLD_SERVICE_HANDLING
  return 0;
}

int CErcu::EvaluateFirmwareVersion() 
{
  int nRet=0;
  int i=0;
  __u32 abState=CErcu::eAbmDDLsiu;
  __u32 fwVersion=0;

  if (SingleWrite(abState, 0x0)<0) {
    CE_Warning("no access to RCU\n");
    return -EIO;
  }

  nRet=SingleRead(RCUFwVersion, &fwVersion);
  if(nRet<0){
    CE_Error("reading of FW version register %#x failed\n", RCUFwVersion);
  }

  if((nRet<0) || (fwVersion==0)){
    CE_Info("could not get FW version ... "
	    "try to read FW version register again with DCS board as bus master\n");
    nRet=SingleWrite(CErcu::eAbmDCSboard, 0x0);
    nRet=SingleRead(RCUFwVersion, &fwVersion);
    if(nRet<0){
      CE_Warning("second trial to read FW version failed\n");
    }
  }

  if((nRet<0) || (fwVersion==0)){
    CE_Info("could not get FW version from %x ... "
	    "try to read old FW version register %x\n",
	    RCUFwVersion, RCUFwVersionOld);
    nRet=SingleRead(RCUFwVersionOld, &fwVersion);
    if(nRet<0){
      CE_Warning("second trial to read FW version failed\n");
    }
  }

  if (fwVersion>0) {
    CE_Info("RCU firmware version is %#x\n", fwVersion);
    CE_Info("DDL set to default Altro Bus master\n");
    abState=CErcu::eAbmDDLsiu;
    nRet=SingleWrite(abState, 0x0);
    if(nRet<0){
      CE_Warning("no access to RCU\n");
      return -EIO;
    }
    return fwVersion;
  }

#ifdef ENABLE_AUTO_DETECTION
  CE_Info("RCU firmware without version register; probing MSM access\n");
  __u32 aflBackup=0;
  nRet=SingleRead(FECActiveList, &aflBackup);
  if(nRet<0){
    CE_Error("reading Active FEC List failed\n");
    return -EIO;
  }

  abState=CErcu::eAbmDDLsiu;
  nRet=SingleWrite(abState, 0x0);
  if(nRet<0){
    CE_Warning("no access to RCU\n");
    return FEE_FAILED;
  }

  for (i=0; i < 2; i++) {
    __u32 testPattern=~aflBackup;
    __u32 readback=0;
    nRet=SingleWrite(FECActiveList, testPattern);
    if(nRet<0){
      CE_Warning("can not write test pattern to AFL\n");
      return FEE_FAILED;
    }
    nRet=SingleRead(FECActiveList, &readback);
    if(nRet<0){
      CE_Warning("reading of test pattern from AFL failed\n");
      return FEE_FAILED;
    }
    if (readback==testPattern) {
      if (abState==CErcu::eAbmDDLsiu) {
	CE_Info("this RCU firmware versions seems to allow MSM access while the DDL is Altro Bus master\n");
	CE_Info("DDL set to default Altro Bus master\n");
      } else {
	CE_Info("this RCU firmware versions does not allow MSM access while the DDL is Altro Bus master\n");
	CE_Info("DCS board set to default Altro Bus master\n");
      }
      break;
    } else if (abState==CErcu::eAbmDDLsiu) {
      // next trial with DCS as Altro Bus master
      abState=CErcu::eAbmDCSboard;
      nRet=SingleWrite(abState, 0x0);
      if(nRet<0){
	CE_Warning("no access to RCU\n");
	return FEE_FAILED;
      }
    }
  }
  
  nRet=SingleWrite(FECActiveList, aflBackup);
  if(nRet<0){
    CE_Warning("can not restore AFL\n");
    return FEE_FAILED;
  }
  nRet=abState==CErcu::eAbmDDLsiu;
#else //!ENABLE_AUTO_DETECTION
  CE_Info("RCU firmware without version register;"
	  "RCU firmware probing is disabled\n");
  CE_Info("DDL set to default Altro Bus master\n");
  abState=CErcu::eAbmDDLsiu;
  nRet=SingleWrite(abState, 0x0);
  if(nRet<0){
    CE_Warning("no access to RCU\n");
    return -EIO;
  }
  nRet=1;
#endif //ENABLE_AUTO_DETECTION
  return nRet;
}

__u32 CErcu::EncodeSlowControl5Cmd(unsigned short rnw, unsigned short bcast, unsigned short branch, unsigned short FECAdr, unsigned short BCRegAdr)
{
  unsigned short unused = 0x000;
  unsigned short base = 0x00C;

  __u32 writeAddress = (unused<<16) | (base<<MSMCommand_base) | (rnw<<MSMCommand_rnw) | (bcast<<MSMCommand_bcast) | (branch<<MSMCommand_branch) | (FECAdr<<MSMCommand_FECAdr) | (BCRegAdr<<MSMCommand_BCRegAdr);

  return writeAddress;
}

int CErcu::CheckSingleFEC(__u32 writeAddress)
{
  __u32 u32RawData=0;
  int result=0;

  CE_LockGuard g(CErcu::fMutex);
  //clear Error Register	
  if (SingleWrite(FECResetErrReg, 0x0)<0) {
    CE_Warning("CErcu::CheckSingleFEC: SingleWrite failed\n");
    return -EIO;
  }

  // write register
  if (SingleWrite(writeAddress, 0x0)<0) {
    CE_Warning("CErcu::CheckSingleFEC: SingleWrite failed\n");
    return -EIO;
  }

  // check error register
  if (SingleRead(FECErrReg,&u32RawData)<0) {
    CE_Warning("CErcu::CheckSingleFEC: SingleRead failed\n");
    return -EIO;
  }
  
  // Instruction to non active FEC
  if(u32RawData&(0x1<<0)){
    result = -EBADF;
  }

  // No ack from FEC
  if(u32RawData&(0x1<<1)){
    result = -EACCES;
  }

  // ok
  if(u32RawData==(0x00)){
    result = 0;
  }
  return result;
}

int CErcu::CheckValidFECs()
{
  int iResult=0;
  unsigned short rnw      = 0x001;   //write
  unsigned short bcast    = 0x000;   //dont broadcast
  unsigned short branch   = 0x000;   //start at Branch A (0)
  unsigned short FECAdr   = 0x000;   //address of FEC to cycle
  unsigned short BCRegAdr = 0x006;  //read Temperature register
  __u32 SlowControlCMD=0;
  __u32 u32AFL=0;
  unsigned i=0;
  unsigned cardsactive = 0;
  unsigned cardsvalid = 0;
  int answer = 0;
  unsigned match = 0;


  if (SingleRead(FECActiveList,&u32AFL)<0) {
    CE_Warning("detectFECs: SingleRead failed\n");
    return -EIO;
  }
    
  for(i=0; i < fFecIds.size(); i++){
    if (fFecIds[i]>=0) {
      //Card is defined
       cardsvalid++;
      
      branch = (i < fMaxFecA) ? 0x000 : 0x001;
      FECAdr = (branch == 0x0) ? i : i%fMaxFecA;

      SlowControlCMD = EncodeSlowControl5Cmd(rnw, bcast, branch, FECAdr, BCRegAdr);
      answer = CheckSingleFEC(SlowControlCMD);
      
      switch(answer){
	case -EACCES: //no ack from FEC
		if(~u32AFL&(0x1<<i)){
		  match++;
		}
		if(u32AFL&(0x1<<i)){
		  CE_Warning("FEC %d was turned on but received no Ack from FEC\n",i );
		}	
		break;
	case -EBADF: //instruction to not active card
		if(u32AFL&(0x1<<i)){
		  CE_Error("FEC %d was turned on but received 'Instruction to not active FEC' Error\n",i );
		}
		if(~u32AFL&(0x1<<i)){
		  match++;
		}
		break;
	case 0:  // Card answered
		if(u32AFL&(0x1<<i)){
		  match++; cardsactive++;
		}
		if(~u32AFL&(0x1<<i)){
		  CE_Error("Card %d was turned off, but received ack from this FEC.\n",i);
		}
		break;
      }
      CE_Debug("wAdr: 0x%x, brnch: 0x%x , FECAdr: 0x%x, Pos: %d, Ans: %d\n",SlowControlCMD, branch, FECAdr, i, answer);
    }
  }
  if(match !=  cardsvalid) {
    CE_Debug("Found %d mismatches.\n", (cardsvalid - match));
    iResult=-ENODEV;
  } else {
    iResult=match;
  }
  return iResult;
}

int CErcu::DetectFECs()
{
  int iResult=0;
  unsigned short rnw      = 0x001;   //write
  unsigned short bcast    = 0x000;   //dont broadcast
  unsigned short branch   = 0x000;   //start at Branch A (0)
  unsigned short FECAdr   = 0x000;   //address of FEC to cycle
  unsigned short BCRegAdr = 0x006;  //write Temperature register
  __u32 SlowControlCMD;
  unsigned maxFECAdr = 0;
  unsigned position = 0;
  int answer, i;
  
  CE_Info("running detectFECs\n");

  //turn on all cards
  __u32 aflBackup=0;
  if (SingleRead(FECActiveList, &aflBackup)<0) {
    CE_Warning("detectFECs: SingleRead failed can not read AFL\n");
    return -EIO;
  }

  if (SingleWrite(FECActiveList, (0xFFFFFFFF))<0) {
    CE_Warning("detectFECs: SingleWrite failed\n");
    return -EIO;
  }

  //Wait for the Boardcontrollers to come up
  usleep(300000);

  for(branch = 0x000; branch < NOF_BRANCHES; branch++){

    maxFECAdr = (branch == 0x0) ? fMaxFecA : fMaxFecB;
    //Loop through the FEC addresses
    for(FECAdr = 0x000; FECAdr < maxFECAdr; FECAdr++){
		
	//what position are we in the AFL?
	position = (branch == 0x0) ? FECAdr : FECAdr + fMaxFecA;

      	SlowControlCMD = EncodeSlowControl5Cmd(rnw, bcast, branch, FECAdr, BCRegAdr);

        answer = CheckSingleFEC(SlowControlCMD);
	
	if (position<fFecIds.size()) {	
	  switch(answer){
	  case -2://no ack from FEC
		  //CE_Debug("Case -2\n");
		  fFecIds[position] = -1;
		  break;
	  case -1://command to not active FEC
		  //CE_Debug("Case -1\n");
		  fFecIds[position] = -1;
		  break;
	  case 0://no error
		  //CE_Debug("Case 0\n");
		  fFecIds[position] = position;
		  break;
	  }
	  CE_Debug("wAdr: 0x%x, brnch: 0x%x , FECAdr: 0x%x, Pos: %d, Ans: %d\n",SlowControlCMD, branch, FECAdr, position, answer);
	} else {
	  CE_Error("position %d out of valid list array (size %d)\n", position, fFecIds.size());
	}
    }//FECAdr
  }//Branches

  // restore the original value of the AFL
  if (SingleWrite(FECActiveList, aflBackup)<0) {
    CE_Warning("detectFECs: can not restore AFL\n");
    return -EIO;
  }

  return iResult;
}

int CErcu::SetOptions(int options)
{
  fOptions=options;
  return fOptions;
}

int CErcu::SetOptionFlag(int of)
{
  fOptions|=of;
  return fOptions;
}

int CErcu::ClearOptionFlag(int of)
{
  fOptions&=~of;
  return fOptions;
}

int CErcu::CheckOptionFlag(int of)
{
  return (fOptions&of)!=0;
}

int CErcu::issue(__u32 cmd, __u32 parameter, const char* pData, int iDataSize, CEResultBuffer& rb)
{
  TranslateRcuCommand(cmd, parameter, pData, iDataSize, rb);
}

int CErcu::GetGroupId()
{
  return FEESVR_CMD_RCU;
}

int CErcu::HighLevelHandler(const char* pCommand, CEResultBuffer& rb)
{
  int iResult=0;
  __u32 cmd=0;
  int keySize=0;
  int len=strlen(pCommand);
  const char* pBuffer=pCommand;
  if (pCommand) {
    //CE_Debug("CErcu checks command %s\n", pCommand);
    if (strncmp(pCommand, "RCU_WRITE_AFL", keySize=strlen("RCU_WRITE_AFL"))==0)
      cmd=RCU_WRITE_AFL;
    else if (strncmp(pCommand, "RCU_READ_AFL", keySize=strlen("RCU_READ_AFL"))==0)
      cmd=RCU_READ_AFL;
    else if (strncmp(pCommand, "RCU_WRITE_MEMORY", keySize=strlen("RCU_WRITE_MEMORY"))==0)
      cmd=RCU_WRITE_MEMORY;
    else if (strncmp(pCommand, "RCU_READ_FW_VERSION", keySize=strlen("RCU_READ_FW_VERSION"))==0)
      cmd=RCU_READ_FW_VERSION;

    if (cmd>0 && keySize>0) {
      pBuffer+=keySize;
      len-=keySize;
      while (*pBuffer==' ' && *pBuffer!=0 && len>0) {pBuffer++; len--;} // skip all blanks
      __u32 parameter=0;
      __u32 data=0;
      const char* pPayload=NULL;
      int iPayloadSize=0;
      switch (cmd) {
      case RCU_WRITE_AFL:
	if (sscanf(pBuffer, "0x%x", &data)>0) {
	  pPayload=(const char*)&data;
	  iPayloadSize=sizeof(data);
	} else {
	  CE_Error("error scanning high-level command %s\n", pCommand);
	  iResult=-EPROTO;
	}
	break;
      case RCU_WRITE_MEMORY:
	{
	  const char* pBlank=NULL;
	  if (sscanf(pBuffer, "0x%x", &parameter)>0 &&
	      (pBlank=strchr(pBuffer, ' '))!=NULL &&
	      sscanf(pBlank, " 0x%x", &data)>0) {
	    pPayload=(const char*)&data;
	    iPayloadSize=sizeof(data);
	  } else {
	    CE_Error("error scanning high-level command %s\n", pCommand);
	    iResult=-EPROTO;
	  }
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

int CErcu::GetPayloadSize(__u32 cmd, __u32 parameter){
  int iResult=0;
  /* 
     switch (cmd) {
     
     case RCU_EXEC:  
     case RCU_READ_PATTERN:
     case RCU_READ_MEMORY:   
     case RCU_READ_RESULT:  
     case RCU_READ_ERRST:  
     case RCU_READ_TRGCFG: 
     case RCU_READ_PMCFG:
     case RCU_READ_AFL:
     case RCU_READ_ACL:
     case RCU_READ_HEADER:
     case RCU_READ_MEMBLOCK:
     iResult = 0;
     break;
     
     case RCU_WRITE_INSTRUCTION:
     case RCU_EXEC_INSTRUCTION:
     case RCU_WRITE_PATTERN32:
     case RCU_WRITE_MEMORY:
     iResult = (int)parameter;
     break;
     
     case RCU_WRITE_PATTERN16:
     iResult = (int)parameter +1;
     
     case RCU_WRITE_PATTERN8:
     iResult = (int)parameter + 3;
     break;
     
     case RCU_WRITE_PATTERN10: 
     
     case RCU_WRITE_RESULT:
     case RCU_WRITE_MEMBLOCK:   
     case RCU_CHECK_ERROR: 
     case RCU_EN_AUTO_CHECK:   
     case RCU_WRITE_TRGCFG: 
     case RCU_WRITE_PMCFG:  
     case RCU_WRITE_AFL:    
     case RCU_WRITE_ACL:   
     case RCU_WRITE_HEADER:    
     case RCU_RESET: 
     case RCU_L1TRG_SELECT: 
     case RCU_SEND_L1_TRIGGER:
     
     default:
     CE_Warning("unrecognized command id (%#x)\n", cmd);
     iResult=-ENOSYS;
     }
  */
  return iResult;
}
#endif //RCU
