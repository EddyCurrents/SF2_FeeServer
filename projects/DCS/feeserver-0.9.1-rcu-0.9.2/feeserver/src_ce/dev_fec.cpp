// $Id: dev_fec.cpp,v 1.16 2007/09/11 22:33:48 richter Exp $

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

// the CEfec class is only used in conjunction with the CErcu device
#ifdef RCU

#include <cstring>
#include "dev_fec.hpp"
#include "dev_rcu.hpp"

#define SERVICE_SIM_HYST 5
using namespace std;

extern "C" void ce_usleep(int sec);

int updateFecService(TceServiceData* pData, int id, int reg, void* parameter) 
{
  int iResult=0;
  CEfec* pFEC=(CEfec*)parameter;
  if (pFEC) {
    iResult=pFEC->UpdateFecService(pData, id, reg);
  }
  return iResult;
}

CEfec::CEfec(int id, CEDevice* pParent, std::string arguments, std::string name)
  : CEDimDevice(name, id, pParent, arguments),
    fUpdateError(-1),
    fUpdateErrorCount(0),
    fTimeAccessFailure(0),
    fTimesRangeExcess()
{
}

CEfec::~CEfec()
{
}

CEState CEfec::EvaluateHardware()
{
  int iResult=0;
  CEState state=eStateOff;
  CErcu* rcu=(CErcu*)GetParentDevice();
  if (rcu) {
    if (rcu->IsFECactive(GetDeviceId())) state=eStateOn;
    else state=eStateOff;
  }
  return state;
}

int CEfec::ArmorDevice()
{
  int iResult=0;
  const service_t* pArray;
  int iNofServices=GetServiceDescription(pArray);
  int iInitial=fListServices.size();
  int fecId=GetDeviceId();
  for (int i=0; i<iNofServices; i++) {
    string name=GetServiceBaseName();
    name+="_";
    name+=pArray[i].name;
    RegisterService(eDataTypeFloat, name.c_str(), pArray[i].deadband, updateFecService, NULL, fecId, i, this);
    fListServices.push_back(pArray[i]);
    fTimesRangeExcess.push_back(0); // reset the time for this service
  }
  return iResult;
}

int CEfec::EnterStateOFF()
{
  int iResult=0;
  CErcu* rcu=(CErcu*)GetParentDevice();
  if (rcu) {
    rcu->SwitchAFL(GetDeviceId(), 0);
  }
  return iResult;
}

int CEfec::LeaveStateOFF()
{
  int iResult=0;
  return iResult;
}

int CEfec::EnterStateON()
{
  int iResult=0;
  CErcu* rcu=(CErcu*)GetParentDevice();
  if (rcu) {
    if ((iResult=rcu->SwitchAFL(GetDeviceId(), 1))>=0) {
      // wait a while if something has been switched
      if (iResult>0) {
	ce_usleep(500000);
      }
#ifndef  FECSIM
      /**
       * TPC test June 2007
       * According to Roberto and some specifications of the I2C bus, the bus has to
       * be set into a correct state. This is done with a few dummy readout operations
       */
      int data=0;
      for (int i=0; i<5; i++) {
	rcu->UpdateFecService(data, GetDeviceId(), FECCSR0);
      }
#endif

      /**
       * Enable the Slow Control measurements.
       * The <i>Monitoring and Safety</i> module of the RCU firmware is set up to start
       * continuous monitoring. CSR0 of the BC set to 0x7FF.<br>
       */
      iResult=rcu->WriteFecRegister(0x7FF, GetDeviceId(), FECCSR0);
    }
  }
  return iResult;
}

int CEfec::LeaveStateON()
{
  int iResult=0;
  return iResult;
}

int CEfec::EnterStateCONFIGURED()
{
  int iResult=0;
  return iResult;
}

int CEfec::LeaveStateCONFIGURED()
{
  int iResult=0;
  return iResult;
}

int CEfec::EnterStateRUNNING()
{
  int iResult=0;
  return iResult;
}

int CEfec::LeaveStateRUNNING()
{
  int iResult=0;
  return iResult;
}

int CEfec::EnterStateERROR()
{
  CErcu* rcu=(CErcu*)GetParentDevice();
  if (rcu) {
    rcu->SwitchAFL(GetDeviceId(), 0);
  }
}

int CEfec::SwitchOn(int iParam, void* pParam)
{
  int iResult=0;
  return iResult;
} 

int CEfec::Shutdown(int iParam, void* pParam)
{
  int iResult=0;
  fUpdateError=-1;
  fUpdateErrorCount=0;
  return iResult;
} 

int CEfec::Configure(int iParam, void* pParam)
{
  int iResult=0;
  return iResult;
} 

int CEfec::Reset(int iParam, void* pParam)
{
  int iResult=0;
  fUpdateError=-1;
  fUpdateErrorCount=0;
  return iResult;
} 

int CEfec::Start(int iParam, void* pParam)
{
  int iResult=0;
  return iResult;
} 

int CEfec::Stop(int iParam, void* pParam)
{
  int iResult=0;
  return iResult;
}

int CEfec::GetServiceDescription(const CEfec::service_t*& /*pArray*/) const
{
  // default function of the base class
  return 0;
}

int CEfec::InitServices()
{
  // default function of the base class
  return 0;
}

int CEfec::UpdateFecService(TceServiceData* pData, int id, int reg)
{
  int iResult=0;
  if (pData) {
    if (GetDeviceId()==id) {
      if (reg<fListServices.size()) {
	int type=fListServices[reg].conversionFactor<0?eDataTypeInt:eDataTypeFloat;
	CEState states[]={eStateOn, eStateConfiguring, eStateConfigured, eStateRunning, eStateInvalid};
	if (Check(states)) {
	  const int gMonitoringDisable=30;
	  const int gMaxRangeExcess=15;
	  const int gMaxFailureCount=5;
	  // In order to suppress log messages the following filter applies:
	  // 1. if one register returns failure this is memorized
	  // 2. update of all other registers are skipped if there was a memorized failure
	  // 3. if the failure repeats during the next update loop, update is disabled for the period
	  //    of gMonitoringDisable seconds
	  // 4. if the number of failures for that register exceeds gMaxFailureCount the FEC is switched
	  //    to state failure
	  if ((fTimeAccessFailure==0 || difftime(time(NULL),fTimeAccessFailure)>gMonitoringDisable) &&
	      (fUpdateError<0 || fUpdateError==reg)) {
	  CErcu* rcu=(CErcu*)GetParentDevice();
	  if (rcu) {
	    float value=0;
#ifdef  FECSIM
	    if (eDataTypeFloat) {
	      /* simulate the FEC services */
	      if (pData->fVal==CE_FSRV_NOLINK) pData->fVal=fListServices[reg].defaultVal;
	      else if (pData->fVal<fListServices[reg].defaultVal+SERVICE_SIM_HYST*fListServices[reg].deadband) pData->fVal+=fListServices[reg].deadband;
	      else pData->fVal=fListServices[reg].defaultVal-SERVICE_SIM_HYST*fListServices[reg].deadband;
	      value=pData->fVal;
	    }
#else  // !FECSIM
	    int data=0;
	    /** this is a temporary fix for the TPC test Jun 07
	     * set to continous measurement each time
	     */
	    rcu->WriteFecRegister(0x7FF, GetDeviceId(), FECCSR0);
	    if ((iResult=rcu->UpdateFecService(data, id, fListServices[reg].regNo))>=0) {
	      switch (type) {
	      case eDataTypeInt:
		pData->iVal=(int)data;
		value=pData->iVal;
		break;
	      case eDataTypeFloat:
		pData->fVal=data*fListServices[reg].conversionFactor;
		value=pData->fVal;
		break;
	      default:
		CE_Error("invalid data point type %d\n", type);
		iResult=-EFAULT;
	      }
	      if (fUpdateError==reg) {
		// reset the error memory
		fUpdateError=-1;
		fUpdateErrorCount=0;
		fTimeAccessFailure=0;
	      }
	    } else if (iResult==-ENOLINK) {
	      // handled below
	    } else {
	      if (fTimeAccessFailure>0) {
		if (fUpdateError==reg && ++fUpdateErrorCount>=gMaxFailureCount) {
		  // repeated error in the same register, switch to FAILURE state and
		  // disable monitoring
		  CE_Warning("access failure to FEC %d, action 3: switch to FAILURE state\n", id);
		  SetFailureState();
		}
	      } else {
		if (fUpdateError==reg) {
		  CE_Warning("access failure to FEC %d, action 2: disable monitoring for %d seconds\n", id, gMonitoringDisable);
		  time(&fTimeAccessFailure);
		} else if (fUpdateError<0) {
		  // memorize the error
		  CE_Warning("access failure to FEC %d reg %d, action 1: memorize\n", id, reg);
		  fUpdateError=reg;
		  fUpdateErrorCount=0;
		}
	      }
	      iResult=-EACCES;
	    }
#endif // FECSIM
	    if (iResult>=0 && fListServices[reg].max > fListServices[reg].min &&
		(fListServices[reg].max < value ||
		 fListServices[reg].min > value)) {
	      if (fTimesRangeExcess[reg]==0) {
		CE_Warning("FEC %d service %s: %f exceeds range [%f,%f],tagged for supervision\n",
			   id, fListServices[reg].name, value, fListServices[reg].min, fListServices[reg].max);
		time(&fTimesRangeExcess[reg]);
	      } else if (difftime(time(NULL), fTimesRangeExcess[reg])>gMaxRangeExcess) {
		CE_Error("FEC %d service %s: %f exceeds range [%f,%f] for %d seconds, FEC switched to failure state\n", 
			 id, fListServices[reg].name, value, fListServices[reg].min, fListServices[reg].max,
			 gMaxRangeExcess);
		// TODO: send an alarm
		//SetErrorState();
		SetFailureState();
	      }
	    }
	  } else {
	    CE_Error("parent RCU device not available\n");
	    iResult=-EFAULT;
	  }
	  }
	} else {
	  iResult=-ENOLINK;
	}
	if (iResult==-ENOLINK) {
	  switch (type) {
	  case eDataTypeInt:
	    pData->iVal=CE_ISRV_NOLINK;
	    break;
	  case eDataTypeFloat:
	    pData->fVal=CE_FSRV_NOLINK;
	    break;
	  default:
	    CE_Error("invalid data point type %d\n", type);
	    iResult=-EFAULT;
	  }
	  iResult=0;
	}
      }
    } else {
      CE_Error("id missmatch: FEC instance %d, required id %d\n", GetDeviceId(), id);
      iResult=-ENODEV;
    }
    if (iResult<0 && iResult!=-EACCES) {
      CE_Warning("access failure to FEC id %d error %d, switch to FAILURE state\n", id, iResult);
      SetFailureState();
    }
    // all failures have been handled, reset error in order to avoid further warning messages
    iResult=0;
  }
  return iResult;
}

#endif //RCU
