/************************************************************************
**
**
** This file is property of and copyright by the Experimental Nuclear 
** Physics Group, Dep. of Physics and Technology
** University of Bergen, Norway, 2007
** This file has been written by Dominik Fehlker
** Please report bugs to dfehlker@htwm.de
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

#ifdef RCU


#include <cstring>
#include <cerrno>
#include <cstdio>
#include "dev_actel.hpp"
#include "ce_base.h"
#include "dcscMsgBufferInterface.h"
#include "selectmapInterface.h"
#include "errno.h"
#include "rcu_issue.h"
#include "time.h"

using namespace std;

#define TRUE 1
#define FALSE 0

/**** SERVICE HANDLER CALLBACKS     ********************************************/

int updateActelRegister(TceServiceData* pData, int address, int type, void* parameter)
{
  int iResult=0;
  CEactel* pActel=(CEactel*)parameter;
  if (pActel) {
    iResult=pActel->UpdateActelRegister(pData, address, type);
  }
  return iResult;    
}

/**** END OF SERVICE HANDLER CALLBACKS     *************************************/

ActelStateMapper g_ActelStateMapper;

//Constructor
CEactel::CEactel()
  :
  CEDimDevice("ACTEL"),
  CEIssueHandler(){


  SetTranslationScheme(&g_ActelStateMapper);

  //Define transitions for the USER states
  CETransition* pReadback=new CETransition(eGoReadback, eStateUser0, "Readback", eStateOn);
  CETransition* pQuitReadback=new CETransition(eQuitReadback, eStateOn, "Quit_Readback", eStateUser0);
  CETransition* pScrubbing=new CETransition(eGoScrubbing, eStateUser1, "Scrubbing", eStateOn);
  CETransition* pQuitScrubbing=new CETransition(eQuitScrubbing, eStateOn, "Quit_Scrubbing", eStateUser1);
  AddTransition(pReadback);
  AddTransition(pQuitReadback);
  AddTransition(pScrubbing);
  AddTransition(pQuitScrubbing);

  
}


//Destructor
CEactel::~CEactel(){
}


extern int actionHandler(CETransitionId transition);

int CEactel::GetPayloadSize(unsigned int a, unsigned int b){
  int iResult = 0;

  return iResult;
}



CEState CEactel::EvaluateHardware(){
  int iResult = 0;
  CEState state = eStateUnknown;

  state = GetCurrentState();
  if(state == eStateUnknown){
    //iResult = Abort();
    //iResult = XilinxAbort();
    state = eStateOn;
    CE_Info("Setting State to State ON\n");
  }

  return state;
}

//Entry function for the Statemachine
//Hardware situation should be found out here, and the proper state entered, Services registered
int CEactel::ArmorDevice(){
  int iResult = 0;
  

  //Register the Services
  string name=GetServiceBaseName();
  name+="_FwVersion";
  RegisterService(eDataTypeInt, name.c_str(), 0.0, updateActelRegister, NULL, ACTELFwVersion, eDataTypeInt, this);
  //  name=GetServiceBaseName();
  //name+="_StatusRegister";
  //RegisterService(eDataTypeInt, name.c_str(), 0.0, updateActelRegister, NULL, ACTELStatusReg, eDataTypeInt, this);
  //name=GetServiceBaseName();
  //name+="_ErrorRegister";
  //RegisterService(eDataTypeInt, name.c_str(), 0.0, updateActelRegister, NULL, ACTELErrorReg, eDataTypeInt, this);
  name=GetServiceBaseName();
  name+="_CycleCounter";
  RegisterService(eDataTypeInt, name.c_str(), 0.0, updateActelRegister, NULL, ACTELReadbackCycleCounter, eDataTypeInt, this);
  name=GetServiceBaseName();
  name+="_FrameErrorCount";
  RegisterService(eDataTypeInt, name.c_str(), 0.0, updateActelRegister, NULL, ACTELReadbackErrorCount, eDataTypeInt, this);
  return iResult;
}

//only with default transitions
const char* CEactel::GetDefaultTransitionName(CETransition* pTransition)
{
  const char* name="unnamed";
  if (pTransition) {
     if (pTransition->IsDefaultTransition()) {
      switch (pTransition->GetID()) {
      case eShutdown:
	name="Go_shutdown";
	break;
      case eReset:
	name="Go_Standby";
	break;
      case eGoReadback:
	name="Go_Readback";
	break;
      case eQuitReadback:
	name="Quit_Readback";
	break;
      case eGoScrubbing:
	name="Go_Scrubbing";
	break;
      case eQuitScrubbing:
	name="Quit_Scrubbing";
	break;
      default:
	name=pTransition->GetName();
      }
    } else {
      // CE_Warning("transition with id %d is not a default one\n", pTransition->GetID());
       CE_Info("transition with id %d is not a default one\n", pTransition->GetID());
    }
  }  else {
    name="invalid";
    //CE_Error("invalid argument\n");
    CE_Error("invalid argument\n");
  }
  return name;
}


//Off = Idle
int CEactel::EnterStateOFF(){
  int iResult = 0;

  return iResult;
}

//Off = Idle
int CEactel::LeaveStateOFF(){
  int iResult = 0;
  unsigned short FwVersion = 0;
  
  //When we leave state off, we should
  //evaluate the hardware that we are dealing with

  FwVersion = EvaluateFirmwareVersion();
  if(FwVersion != 0x0103){
    CE_Warning("Actel Device: Unknown Firmware Version: %#x\n", FwVersion);        
  }
  

  return iResult;
}

//ON = configured
int CEactel::EnterStateON(){
  int iResult = 0;
  
    
  return iResult;
}

//ON = configured
int CEactel::LeaveStateON(){
  int iResult = 0;

  return iResult;
}

int CEactel::EnterStateRUNNING(){
  int iResult = 0;

  //iResult = Readback();

  return iResult;
}

int CEactel::LeaveStateRUNNING(){
  int iResult = 0;

  iResult = Abort();

  return iResult;
}

/**
int CEactel::EnterStateCONFIGURED(){
  int iResult = 0;

  return iResult;
}

int CEactel::LeaveStateCONFIGURED(){
  int iResult = 0;

  return iResult;
}
**/

//USER1 = Scrubbing
int CEactel::EnterStateUSER1(){
  int iResult = 0;

  //Do a continously scrubbing
  iResult = Scrub();
  
  return iResult;
}

//USER1  = Scrubbing
int CEactel::LeaveStateUSER1(){
  int iResult = 0;

  iResult = Abort();

   return iResult;
}

//USER0 = Readback Verification
int CEactel::EnterStateUSER0(){
  int iResult = 0;

  iResult = Readback();
  
  return iResult;
}

//USER0 = Readback Verification
int CEactel::LeaveStateUSER0(){
  int iResult = 0;
  int iNrOfFrameErrors = 0;

  //Here we can check for the number of occurred errors
  //during readback:

  /** Stop the Readback **/
  iResult = Abort();
  iNrOfFrameErrors = NumberOfFrameErrors();

  if(iNrOfFrameErrors > 0){
    //Here we can log/publish the number of occurred errors
    CE_Warning("Actel Device: Found %d Errors in the Configuration Memory\n", iNrOfFrameErrors);    
  }

  return iResult;
}


int CEactel::SwitchOn(int iParam, void* pParam){
  int iResult = 0;

  return iResult;
}

int CEactel::ShutDown(int iParam, void* pParam){
  int iResult = 0;

  return iResult;
}

int CEactel::Configure(int iParam, void* pParam){
  int iResult = 0;

  return iResult;
}

int CEactel::Reset(int iParam, void* pParam){
  int iResult = 0;

  return iResult;
}

int CEactel::Start(int iParam, void* pParam){
  int iResult = 0;
   
  CE_Debug("ACTELControlEngine::Start called\n");
  //  if (fpACTEL) {
  //  iResult=fpACTEL->TriggerTransition(eStart);
  //}
  
  
  return iResult;
}

int CEactel::Stop(int iParam, void* pParam){
  int iResult = 0;

  return iResult;
}

int CEactel::TranslateActelCommand(__u32 cmd, __u32 parameter, const char* pData, int iDataSize, CEResultBuffer& rb){
  int iResult = 0;
  int iProcessed = 0;
  switch (cmd) {
  case RCU_ACTEL_READBACK:
    //iResult=TriggerTransition((CETransitionId)eGoReadback);
    break;
  case RCU_READ_FLASH:
    break;
  case RCU_WRITE_FLASH:
    //program the flash memory
    break;
    //erase the Flash Memory
  case RCU_ERASE_FLASH:
    EraseFlash();
    CE_Info("Actel Device: received BOB command\n");
    break;
    //do one initial configuration from the passed data
  case RCU_INIT_CONF:
    break;
    //do one scrubbing cycle with the passed scrubbing data
  case RCU_SCRUBBING:
    break;
  case RCU_READ_FPGA_CONF:
    break; 
  case RCU_WRITE_FPGA_CONF:
    break;
  default:
    CE_Warning("unrecognized command id (%#x)\n", cmd);
    iResult=-ENOSYS;
  }
  
  return iResult;
}

/***********************************************
 ********   internal functions   ***************
 **********************************************/




int CEactel::UpdateActelRegister(TceServiceData* pData, int address, int type)
{
  int iResult=0;
#ifndef RCUDUMMY
  if (pData) {
    __u32 data=0;
    iResult=rcuSingleRead(address, &data);    
    
    /**
     * Cut out the 16 used bits to get
     * rid of eventual garbage on the bus...
     * Just don't forget it in case of reading
     * more then 16 bits.
     **/
    data &= 0x0000FFFF;
      
    if (type==eDataTypeInt) {
      pData->iVal=data;
    } else if (type==eDataTypeFloat) {
      pData->fVal=data;
    }
  } else {
    iResult=-EINVAL;
  }
#endif
  return iResult;
}


//check all the error and other registers to
//see if something fishy is going on...
int CEactel::Checking(){
  int iResult = 0;
  __u32 u32rawData;
  
  //First try to read the Firmware register
  iResult = rcuSingleRead(ACTELFwVersion, &u32rawData);
  if(iResult<0){
    CE_Warning("Actel Device: rcuSingleRead failed\n");
    iResult = EIO;
    return iResult;
  }
  CE_Info("Actel Device: Firware Version Register: %#x", u32rawData);
  
  

  if(iResult = CheckStatusReg() < 0){
    //if there is a nullpointer in the flash
    if(iResult = -8 | -12 | -13){
      /**
       * 
       **/
    }

  }

  if(CheckErrorReg() < 0){
    //something wrong in the error register

  }

  return iResult;
}

/** Do init procedure of the Xilinx **/
int CEactel::XilinxInit(){
  int iResult = 0;
  __u32 u32rawData;
  u32rawData = 0x1;

  iResult = rcuSingleWrite(ACTELSmCommandReg, u32rawData);
  if(iResult<0){
    CE_Warning("Actel Device: rcuSingleWrite failed\n");
    iResult = EIO;
    return iResult;
  }  
  
  return iResult;
}

/** Do startup procedure of the Xilinx **/
int CEactel::XilinxStartup(){
  int iResult = 0;
  __u32 u32rawData;
  u32rawData = 0x2;

  iResult = rcuSingleWrite(ACTELSmCommandReg, u32rawData);
  if(iResult<0){
    CE_Warning("Actel Device: rcuSingleWrite failed\n");
    iResult = EIO;
    return iResult;
  }  
  
  return iResult;
}

/** Issue abort sequence **/
int CEactel::XilinxAbort(){
  int iResult = 0;
  __u32 u32rawData;
  u32rawData = 0x10;

  iResult = rcuSingleWrite(ACTELSmCommandReg, u32rawData);
  if(iResult<0){
    CE_Warning("Actel Device: rcuSingleWrite failed\n");
    iResult = EIO;
    return iResult;
  }  
  
  return iResult;
}


int CEactel::CheckErrorReg(){
  int iResult = 0;
  __u32 u32rawData;
  
  //And then we check the Error Register
  iResult = rcuSingleRead(ACTELErrorReg, &u32rawData);
  if(iResult<0){
    CE_Warning("Actel Device: rcuSingleRead failed\n");
    iResult = EIO;
    return iResult;
  }
  if((u32rawData & 0x0080) != 0){
    CE_Warning("Actel Device: Parity Error in Selectmap RAM detected: %#x\n", u32rawData);
    iResult -= 64;
  }
  if((u32rawData & 0x0040) != 0){
    CE_Warning("Actel Device: Parity Error in Flash RAM detected: %#x\n", u32rawData);
    iResult -= 128;
  }

  return iResult;
}


//return the last active command from the Actel
int CEactel::LastActiveCommand(){
  int iResult = 0;
  __u32 u32rawData;
  
  //Then we evaluate the status register
  iResult = rcuSingleRead(ACTELStatusReg, &u32rawData);
  if(iResult<0){
    CE_Warning("Actel Device: rcuSingleRead failed\n");
    iResult = EIO;
    return iResult;
  }

  switch(u32rawData & 0xF){
  case 0xA:     //Initial Configuration
    iResult = 1;
    break;
  case 0xB:     //Scrubbing
    iResult = 2;
    break;
  case 0xC:     //Scrubbing continously
    iResult = 3;
    break;
  case 0xD:     //Readback and Verification
    iResult = 4;
    break;
  case 0xE:     //Readback and Verification continously
    iResult = 5;
    break;
  default://should not happen
    break;  
  }  

  return iResult;
}

int CEactel::CheckStatusReg(){
  int iResult = 0;
  __u32 u32rawData;

  /** Then we evaluate the status register **/
  iResult = rcuSingleRead(ACTELStatusReg, &u32rawData);
  if(iResult<0){
    CE_Warning("Actel Device: rcuSingleRead failed\n");
    iResult = EIO;
    return iResult;
  }
  /**
   * Now we check the status register for
   * - Null pointer in flash: Bit 7
   **/
  if((u32rawData & 0x0080) != 0){
    iResult -= 8;
    CE_Warning("Actel Device: Flash contains no pointer value: %#x\n", u32rawData);
  }
  /** - busy flag: Bit 6 **/
  if((u32rawData & 0x0040) != 0){
    iResult -= 4;
    CE_Warning("Actel Device: Configuration controller is busy with ongoing task: %#x\n", u32rawData);
  }
  /** - smap busy: Bit 4 **/
  if((u32rawData & 0x0010) != 0){
    iResult -= 1;
    CE_Warning("Actel Device: Selectmap Interface is busy: %#x\n", u32rawData);
  }

  return iResult;
}


/**************************************************
 * Yet to come
 ***************************************************/
/**
 * In case the Xilinx is not/ not properly configured
 * try to reconfigure, this tests should be done before turning on
 **/
int CEactel::TryUpdateConfiguration(){
  int iResult = 0;

  DisableAutomaticConfiguration();
  
  InitialConfiguration();

  

  return iResult;
}

/**
 * If the fash is faulty (test with checksum?) then try
 * to upload a new version of the configuration.
 **/
int CEactel::TryUpdateFlash(){
  int iResult = 0;

  //maybe skip this?
    
  return iResult;
}




/**
 * If there was something strange seen in the (error) registers,
 * try to clear the registers and try again.
 **/
int CEactel::TryUpdateValidation(){
  int iResult = 0;
  
  iResult = Abort();
  //Clear the status and error registers and try again
  iResult = ClearErrorReg();
  iResult = ClearStatusReg();

  return iResult;
}


/**************************************************
 * /Yet to come
 ***************************************************/
//call from evaluate hardware

//find out which version of the firmware we are dealing with
int CEactel::EvaluateFirmwareVersion(){
  int iResult = 0;
  __u32 u32rawData;
  iResult = rcuSingleRead(ACTELFwVersion, &u32rawData);
  if(iResult<0){
    CE_Warning("Actel Device: rcuSingleRead failed\n");
    iResult = EIO;
    return iResult;
  }
  
  return iResult;
}

//some smaller helper functions here
//**********************************

//Disable the Automatic Configuration
int CEactel::DisableAutomaticConfiguration(){
  int iResult = 0;
  __u32 u32rawData;
  u32rawData = 0xa;

  iResult = rcuSingleWrite(ACTELStopPowerUpCodeReg, u32rawData);
  if(iResult<0){
    CE_Warning("Actel Device: rcuSingleWrite failed\n");
    iResult = EIO;
    return iResult;
  }
  
  return iResult;
}

int CEactel::ClearErrorReg(){
  int iResult = 0;
  __u32 u32rawData;
  u32rawData = 0x1;

  iResult = rcuSingleWrite(ACTELCommandReg, u32rawData);
  if(iResult<0){
    CE_Warning("Actel Device: rcuSingleWrite failed\n");
    iResult = EIO;
    return iResult;
  }
  
  return iResult;
}

int CEactel::ClearStatusReg(){
  int iResult = 0;
  __u32 u32rawData;
  u32rawData = 0x2;

  iResult = rcuSingleWrite(ACTELCommandReg, u32rawData);
  if(iResult<0){
    CE_Warning("Actel Device: rcuSingleWrite failed\n");
    iResult = EIO;
    return iResult;
  }
  
  return iResult;
}

//Do one initial Configuration of the Xilinx from the Flash
int CEactel::InitialConfiguration(){
  int iResult = 0;
  __u32 u32rawData;
  u32rawData = 0x4;

  iResult = rcuSingleWrite(ACTELCommandReg, u32rawData);
  if(iResult<0){
    CE_Warning("Actel Device: rcuSingleWrite failed\n");
    iResult = EIO;
    return iResult;
  }
  
  return iResult;
}

/**
 * Do scrubbing one time
 **/
int CEactel::Scrub(){
  int iResult = 0;
  __u32 u32rawData;
  u32rawData = 0x10;

  iResult = rcuSingleWrite(ACTELCommandReg, u32rawData);
  if(iResult<0){
    CE_Warning("Actel Device: rcuSingleWrite failed\n");
    iResult = EIO;
    return iResult;
  }
  
  return iResult;
}

/**
 * Scrub for more then one time, 0 for continously
 **/
int CEactel::Scrub(int times){
  int iResult = 0;
  __u32 u32rawData;
  u32rawData = (__u32)times;

  //write how many times scrubbing should be done to the data register
  iResult = rcuSingleWrite(ACTELDataReg, u32rawData);
  if(iResult<0){
    CE_Warning("Actel Device: rcuSingleWrite failed\n");
    iResult = EIO;
    return iResult;
  }  
  
  u32rawData = 0x20;  
  
  //execute scrubbing
  iResult = rcuSingleWrite(ACTELCommandReg, u32rawData);
  if(iResult<0){
    CE_Warning("Actel Device: rcuSingleWrite failed\n");
    iResult = EIO;
    return iResult;
  }
  
  return iResult;
}

/**
 * readback all frames one time
 **/
int CEactel::Readback(){
  int iResult = 0;
  __u32 u32rawData;
  u32rawData = 0x100;
  
  iResult = rcuSingleWrite(ACTELCommandReg, u32rawData);
  if(iResult<0){
    CE_Warning("Actel Device: rcuSingleWrite failed\n");
    iResult = EIO;
    return iResult;
  }

  return iResult;
}

/**
 * readback a given number of times, 0 for continously
 **/
int CEactel::Readback(int times){
  int iResult = 0;
  __u32 u32rawData;
  u32rawData = (__u32)times;
  
  //write how many times a Readback should be done to the data register
  //  iResult = rcuSingleWrite(ACTELDataReg, u32rawData);
  iResult = rcuSingleWrite(ACTELDataReg, 0x0);
  if(iResult<0){
    CE_Warning("Actel Device: rcuSingleWrite failed\n");
    iResult = EIO;
    return iResult;
  }

  u32rawData = 0x200;
  
  //execute Readback
  iResult = rcuSingleWrite(ACTELCommandReg, u32rawData);
  if(iResult<0){
    CE_Warning("Actel Device: rcuSingleWrite failed\n");
    iResult = EIO;
    return iResult;
  }

  return iResult;
}

/**
 * Abort any ongoing operation on the Actel
 **/
int CEactel::Abort(){
  int iResult = 0;
  __u32 u32rawData;
  u32rawData = 0x8000;
 
 iResult = rcuSingleWrite(ACTELCommandReg, u32rawData);
  if(iResult<0){
    CE_Warning("Actel Device: rcuSingleWrite failed\n");
    iResult = EIO;
    return iResult;
  }

  return iResult;
}
 
/**
 * Read out the number of errors that were found during Readback
 **/
unsigned short CEactel::NumberOfFrameErrors(){
  int iResult = 0;
  unsigned short frameerrors = 0;
  __u32 u32rawData = 0;
  
  iResult = rcuSingleRead(ACTELReadbackErrorCount, &u32rawData);
  if(iResult<0){
    CE_Warning("Actel Device: rcuSingleRead failed\n");
    iResult = EIO;
    return iResult;
  }
  frameerrors = u32rawData;

  return frameerrors;  
}

/**
 * Read out how many complete cycles a frame by frame readback has been done
 **/
unsigned short CEactel::NumberOfCycles(){
  int iResult = 0;
  unsigned short cycles = 0;
  __u32 u32rawData = 0;

  iResult = rcuSingleRead(ACTELReadbackCycleCounter, &u32rawData);
  if(iResult<0){
    CE_Warning("Actel Device: rcuSingleRead failed\n");
    iResult = EIO;
    return iResult;
  }
  cycles = u32rawData;

  return cycles;
}

/*********************************
 *** IssueHandler functions    ***
 ********************************/

int CEactel::GetGroupId(){
  return FEESVR_CMD_RCUCONF;
  
}

/**
 * The commands (e.g. sent by feeserver-ctrl) ment for the Actel device end up here 
 * we pass them on to the TranslateActelCommand
 **/
int CEactel::issue(__u32 cmd, __u32 parameter, const char* pData, int iDataSize, CEResultBuffer& rb){
  TranslateActelCommand(cmd, parameter, pData, iDataSize, rb);
}

int CEactel::CheckCommand(__u32 cmd, __u32 parameter)
{
  int iResult=0;
  switch (cmd) {
    case RCU_READ_FLASH:
    case RCU_WRITE_FLASH:
    case RCU_READ_FPGA_CONF:
    case RCU_WRITE_FPGA_CONF:
    case RCU_SCRUBBING:
    case RCU_INIT_CONF:
    case RCU_ACTEL_READBACK:
    case RCU_ERASE_FLASH:
    iResult=1;
    break;
  default:
    iResult=0;
  }
  CE_Debug("ActelDevice::CheckCommand cmd %#x parameter %#x result %d\n", cmd, parameter, iResult);
  return iResult;
}



/**********************************************
 ***** DcscMsgBufferInterface Functions  ******
 *********************************************/

int CEactel:: getBusState(){
  int state, iResult;
  state = 0;
  iResult = 0;
  
  iResult = rcuBusControlCmd(eCheckSelectmap);
  if(iResult == 1)
    state = 1;
  iResult = 0;
  iResult = rcuBusControlCmd(eCheckFlash);
  if(iResult == 1)
    state = 2;
  iResult = 0;
  iResult = rcuBusControlCmd(eCheckMsgBuf);
  if(iResult == 1)
    state = 3;
  iResult = 0;
  
  return state;
}



int CEactel:: enterFlashState(int oldstate){
  int iResult;
  
  switch(oldstate){
  case 2: //already in Flash State
    break;
  case 1: //state was Selectmap
    iResult = rcuBusControlCmd(eEnableMsgBuf);
    if(iResult < 0){
      CE_Warning("Actel Device: Error entering Msg Mode: %d\n", iResult);
    }
    //Fall through intended, mode has to be in msgbuffer mode before
  case 3:
    //State was MsgBuf or SelectMap - switching to Flash
    iResult = rcuBusControlCmd(eEnableFlash);
    if(iResult < 0){
      CE_Warning("Actel Device: Error entering Flash Mode: %d\n", iResult);
    }
    break;
  default:
    iResult = -1;
    CE_Warning("Actel Device: Error entering Flash Mode: %d\n", iResult);
    break;
  }
  
return iResult;
}

int CEactel:: restoreBusState(int oldstate){
  int iResult = 0;
  
  switch(oldstate){
  case 2: // was already in Flash State
    break;
  case 1:
    //State was MsgBuf or SelectMap - switching to Flash
    iResult = rcuBusControlCmd(eEnableMsgBuf);
    if(iResult < 0){
      CE_Warning("Actel Device: Error entering MsgBuf Mode: %d\n", iResult);
    }
    iResult = rcuBusControlCmd(eEnableSelectmap);
    if(iResult < 0){
      CE_Warning("Actel Device: Error entering SelectMap Mode: %d\n", iResult);
    }
    break;
  case 3:
    //State was MsgBuf or SelectMap - switching to Flash
    iResult = rcuBusControlCmd(eEnableMsgBuf);
    if(iResult < 0){
      CE_Warning("Actel Device: Error entering MsgBuf Mode: %d\n", iResult);
    }
    break;
  default:
    iResult = -1;
    CE_Warning("Actel Device: Error while restoring the original bus state\n");
    break;
  }
  return iResult;
}

/******************************************
 ***** Functions Operating on the Flash ***
 *****************************************/

int CEactel:: EraseFlash(){
  int iOldState, iResult = 0;
  
  iOldState = getBusState();
  
  iResult = enterFlashState(iOldState);
  
  iResult = rcuFlashErase(-1, 0);
  if(iResult < 0){
    printf("Error erasing the Flash: %d", iResult);
  }
  
  iResult = restoreBusState(iOldState);

  return iResult;
}


/*********************************
 ***  Statemapper stuff        ***
 ********************************/

ActelStateMapper::ActelStateMapper() {
}

ActelStateMapper::~ActelStateMapper() {
}

const char* ActelStateMapper::GetMappedStateName(CEState state)
{
  const char* name=NULL; 
  switch (state) {
  case eStateUnknown:     name="UNKNOWN";         break;
  case eStateOff:         name="IDLE";            break;
  case eStateError:       name="ERROR";           break;
  case eStateFailure:     name="FAILURE";         break;
  case eStateOn:          name="STANDBY";         break;
  case eStateConfiguring: name="DOWNLOADING";     break;
  case eStateConfigured:  name="STBY_CONFIGURED"; break;
  case eStateRunning:     name="RUNNING";         break;
  case eStateUser0:       name="READBACK";        break;
  case eStateUser1:       name="SCRUBBING";       break;
  }
  return name;
}

#endif //RCU

