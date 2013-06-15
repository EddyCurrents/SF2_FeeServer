// $Id: ce_command.c,v 1.46 2007/06/26 14:01:04 richter Exp $

/************************************************************************
**
**
** This file is property of and copyright by the Experimental Nuclear 
** Physics Group, Dep. of Physics and Technology
** University of Bergen, Norway, 2004
** This file has been written by Matthias Richter and Dag T Larsen
** Please report bugs to Matthias.Richter@ift.uib.no, dagtl@ift.uib.no
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
//#include <unistd.h>        // sleep
#include <dim/dim_common.h>    // dim_sleep
#include <stdio.h>
#include <errno.h>
#include "ce_command.h"
#include "fee_errors.h"
#include "fee_defines.h"
#include "ce_base.h"
//#include "rcu_service.h"   // handling of the service API methods for rcu-like CEs
#include "rcu_issue.h"     // handling of the issue API method for rcu-like CEs
#include <dim/dis.h>	  // dimserver library

// entry point for the ControlEngine instance
extern int ControlEngine_Run();
extern int ControlEngine_Terminate();
extern int ControlEngine_SetUpdateRate(unsigned short millisec);
extern int ControlEngine_Issue(char* command, char** result, int* size);

/***************************************************************************
 * API functions to the feeserver core
 */
int issue(char* command, char** result, int* size) {
  ControlEngine_Issue(command, result, size);
  return CE_OK;
}

void cleanUpCE() {
  ControlEngine_Terminate();
  CE_Info("CE terminated\n");
  return;
}

void initializeCE() {
  int ceState=CE_OK;
  int nRet=CE_OK;
  printf("\ninitializing ControlEngine ...\n");
#ifdef PACKAGE_STRING
  printf("-----   %s   ----\n\n", PACKAGE_STRING);
#endif
  void feeserverCompileInfo( char** date, char** time);
  char* date=NULL;
  char* time=NULL;
  feeserverCompileInfo(&date, &time);
  if (date!=NULL && time!=NULL) {
    printf("-----   compiled on %s %s   ----\n", date, time);
  }

#ifdef ENABLE_ANCIENT_05
  printf("compiled for the RCU firmware of May05\n"); 
#endif //ENABLE_ANCIENT_05
#ifdef RCUDUMMY
  printf("This is an RCU dummy version\n"); 
#endif //RCUDUMMY
#ifdef FECSIM
  printf("Front-end card simulation is on\n"); 
#endif //FECSIM
#ifdef DISABLE_SERVICES
  printf("CE info:  services disabled\n");
#endif // !DISABLE_SERVICES
  ControlEngine_Run();  
  return;
}

void signalFeePropertyChanged(FeeProperty* prop) {
  if (prop) {
    if ((prop->flag|PROPERTY_UPDATE_RATE)!=0) {
      // it isnt clear from the documentation whether to value is ment to present
      // the update rate in seconds, milli or microseconds
      // it seems to be milli seconds
      ControlEngine_SetUpdateRate(prop->uShortVal);
      CE_Info("got property change:PROPERTY_UPDATE_RATE %d\n", prop->uShortVal);  
    }
  }
  return;
}

/**
 * A simple wrapper function to dis_add_service.
 * The wrapper is needed to use the DIM C-code without changes in a C++
 * application.
 */
unsigned int ce_dis_add_service(char* service, char* type, void* buffer, int size, ceDisServiceCallback cb, long int tag)
{
  return dis_add_service(service, type, buffer, size, cb, tag);
}

/**
 * A simple wrapper function to dis_update_service.
 * The wrapper is needed to use the DIM C-code without changes in a C++
 * application.
 */
int ce_dis_update_service(unsigned int id)
{
  return dis_update_service(id);
}

void ce_ready(int iResult) {
  signalCEready(iResult);
}

void ce_sleep(int sec)
{
  sleep(sec); // sleep is redirected to dtq_sleep in dim_common.h
}

void ce_usleep(int usec)
{
  dim_usleep(usec);
}

/**
 * Force update of a DIM service channel.
 * This is a wrapper function to @ref feesrv_ceapi.
 */
int UpdateFeeService(const char* serviceName)
{
  int iResult=updateFeeService((char*)serviceName);
  if (iResult<0) {
    switch (iResult) {
    case FEE_WRONG_STATE:
      iResult=-EACCES;
      break;
    case FEE_NULLPOINTER:
      iResult=-EINVAL;
      break;
    case FEE_INVALID_PARAM:
      iResult=-ENOENT;
      break;
    default:
      iResult=-EFAULT;
    }
  }
  return iResult;
}
