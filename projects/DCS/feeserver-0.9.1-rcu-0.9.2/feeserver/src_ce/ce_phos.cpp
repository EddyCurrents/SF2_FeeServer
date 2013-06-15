// $Id: ce_phos.cpp,v 1.16 2007/09/01 23:27:42 richter Exp $

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
 * ce_phos.c
 * this file implements the specific methods for the PHOS
 */

#ifdef PHOS
#include <cerrno>
#include <cstring>
#include <cstdio>
#include "ce_phos.h"
#include "rcu_service.h"
#include "ce_base.h"
#include "dev_rcu.hpp"
#include "rcu_issue.h"

using namespace std;

/*******************************************************************************
 *
 *                           service callbacks
 *
 ******************************************************************************/

/**
 * <i>Set</i> function for APD channels.
 * The 'set' function is of type
 * int (*ceSetFeeValue)(TceServiceData* pData, int major, int minor, void* parameter);
 *
 * The function pointer is passed to the CE service framework during service
 * registration. The function handler is called, Whenever a 
 * @ref FEESVR_SET_FERO_DATA command for the service is received, 
 * @see ceSetFeeValue
 * @see rcu_ce_base_services
 * @ingroup rcu_ce_phos
 */
int PHOSsetAPD(TceServiceData* pData, int major, int minor, void* parameter) 
{
  int iResult=0;
  if (pData) {
    CEfecPHOS* pFec=(CEfecPHOS*)parameter;
    if (pFec) {
      pFec->SetAPD(pData->fVal, major);
    }
  } else {
    iResult=-EINVAL;
  }
  return iResult;
}

/**
 * <i>Update</i> function for APD channels.
 * The 'update' function is of type 
 * int (*ceUpdateService)(TceServiceData* pData, int major, int minor, void* parameter)
 *
 * The function pointer is passed to the CE service framework during service
 * registration. The function handler is called, whenever an update of the value of the
 * published data is requested. This is done periodically by the framework.
 * @see ceSetFeeValue
 * @see rcu_ce_base_services
 * @ingroup rcu_ce_phos
 */
int PHOSupdateAPD(TceServiceData* pData, int major, int minor, void* parameter) 
{
  int iResult=0;
  if (pData) {
    CEfecPHOS* pFec=(CEfecPHOS*)parameter;
    if (pFec) {
      pFec->UpdateAPD(pData->fVal, major);
    }
  } else {
    iResult=-EINVAL;
  }
  return iResult;
}

/*******************************************************************************
 *
 *                           PHOSagent
 *
 ******************************************************************************/

const char* PHOSagent::GetName()
{
  return "PHOS ControlEngine";
}

ControlEngine* PHOSagent::CreateCE()
{
  return new PHOSControlEngine;
}

/** the global control engine instance */
PHOSagent g_PHOSagent;

/*******************************************************************************
 *
 *                           PHOSControlEngine implementation
 *
 ******************************************************************************/

PHOSControlEngine::PHOSControlEngine()
  :
  fpBranchLayout(NULL),
  fpCH(NULL)
{
  fpCH=new PHOSCommandHandler;
}

PHOSControlEngine::~PHOSControlEngine()
{
  if (fpCH) delete fpCH;
  fpCH=NULL;
}

int PHOSControlEngine::InitCE()
{
  int iResult=0;
  fpBranchLayout=new PHOSBranchLayout;
  if (fpBranchLayout) {
    SetRcuBranchLayout(fpBranchLayout);
  } else {
    iResult=-ENOMEM;
  }
  return iResult;
}

int PHOSControlEngine::DeinitCE()
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
 *                           PHOSCommandHandler implementation
 *
 ******************************************************************************/

PHOSCommandHandler::PHOSCommandHandler(PHOSControlEngine* pInstance)
  :
  fInstance(pInstance)
{
}

PHOSCommandHandler::~PHOSCommandHandler()
{
}

int PHOSCommandHandler::GetGroupId()
{
  return FEESVR_CMD_PHOS;
}

int PHOSCommandHandler::GetPayloadSize(__u32 cmd, __u32 parameter)
{
  return 0;
}

int PHOSCommandHandler::HighLevelHandler(const char* pCommand, CEResultBuffer& rb)
{
  int iResult=-ENOSYS;
  CE_Warning("PHOSCommandHandler::HighLevelHandler not yet implemented");
  return iResult;
}

int PHOSCommandHandler::issue(__u32 cmd, __u32 parameter, const char* pData, int iDataSize, CEResultBuffer& rb)
{
  int iResult=0;
  // check arguments
  if (pData==NULL || iDataSize<parameter) return 0;

  // check instance
  if (fInstance==NULL) return 0;

  CE_Warning("PHOSCommandHandler::issue not yet implemented");
  return iResult;
}

/*******************************************************************************
 *
 *                           CEfecPHOS implementation
 *
 ******************************************************************************/

CEfecPHOS::CEfecPHOS(int id, CEDevice* pParent, std::string arguments, std::string name)
  : CEfec(id, pParent, arguments, name)
{
}

CEfecPHOS::~CEfecPHOS()
{
}

const CEfec::service_t CEfecPHOS::fServiceDesc[]={
  // add services here, see CEfecTPC::fServiceDesc in ce_tpc.cpp
  {"TEMP",    6, 0.25  , 0.5  , 40  ,  50, 0}
};

int CEfecPHOS::ArmorDevice()
{
  int iResult=0;

  // !!! adjust the default dead band
  float defaultDeadband=100.0;

  string name=GetServiceBaseName();
  name+="_APD";
  /* registration of the FECs is done separately for the two branches in order
   * to set appropriate service names
   */
  iResult=RegisterServiceGroup(eDataTypeFloat, name.c_str(), numberOfAPDchannels, defaultDeadband, PHOSupdateAPD, PHOSsetAPD, 0, this);
  iResult=CEfec::ArmorDevice();
  return iResult;
}

int CEfecPHOS::SetAPD(float value, int channelNo)
{
  // implement the hardware access here
  CE_Info("set APD value for FEC %d, channel %d: %f\n", GetDeviceId(), channelNo, value);
  CE_Warning("hardware access not yet implemented\n");
  return 0;
}

int CEfecPHOS::UpdateAPD(float& value, int channelNo)
{
  CEState states[]={eStateOn, eStateConfiguring, eStateConfigured, eStateRunning, eStateInvalid};
  if (Check(states)) {
    // implement the hardware access here, remember to use the device id
    // GetDeviceId() whenever accessing registers through the RCU parent device
    // The Ids might be mixed and not in line with the FEC position in the
    // backplanes. This depends on the implemented branch layout.
    if (value>1000 || value<0) value=channelNo; 
    else value+=1;
  } else {
    // non active services have to be set to -2000
    value=CE_FSRV_NOLINK;
  }
  return 0;
}

int CEfecPHOS::GetServiceDescription(const CEfec::service_t*  &pArray) const
{
  pArray=fServiceDesc;
  return sizeof(fServiceDesc)/sizeof(CEfec::service_t);
}

/*******************************************************************************
 *
 *                           PHOSBranchLayout
 *
 ******************************************************************************/

PHOSBranchLayout::PHOSBranchLayout()
{
}

PHOSBranchLayout::~PHOSBranchLayout()
{
}

int PHOSBranchLayout::GetNofFecBranchA()
{
  return 14;
}

int PHOSBranchLayout::GetNofFecBranchB()
{
  return 14;
}

CEfec* PHOSBranchLayout::CreateFEC(int id, CEDevice* pParent)
{
  // The naming convention for PHOS FEC services was discussed with Per Thomas
  // in 2006, don't know if it is still valid. These lines build the device 
  // names for the FECs according to their id
  // The device ids count from 0 to NofFecBranchA-1 in branch A and
  // NofFecBranchA to NofFecBranchB-1 in branch B.
  // mysterious crash on the dcs board: The original idea was to
  // use "FEC_" as prefix, but this leads somehow to a crash of
  // the feeserver on the DCS board. On a normal PC it works
  // DETAILED INVESTIGATION NECESSARY
  string name="";
  int pos=0;
  if (id<GetNofFecBranchA()) {
    name+="A";
    pos=id;
  } else if (id<GetNofFecBranchA()+GetNofFecBranchB()) {
    name+="B";
    pos=id-GetNofFecBranchA();
  } else {
    CE_Error("invalid FEC id\n");
    return NULL;
  }
  if (pos>=100) {
    CE_Error("invalid FEC position (string border override)\n");
    return NULL;
  }
  int len=name.length();
  name+="__"; // two dummy characters
  sprintf((char*)&name[len], "%02d", pos);
  return new CEfecPHOS(id, pParent, "", name.c_str());
}

#endif //PHOS
