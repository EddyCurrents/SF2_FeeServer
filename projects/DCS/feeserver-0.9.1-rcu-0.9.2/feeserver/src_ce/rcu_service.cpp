// $Id: rcu_service.cpp,v 1.29 2007/08/22 22:25:01 richter Exp $

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

#include <stdlib.h>        // malloc, free,
#include <stdio.h>         // snprintf
#include <string.h>
//#include <math.h>
#include <errno.h>
#include "ce_command.h"
#include "fee_errors.h"
#include "dcscMsgBufferInterface.h" // access library to the dcs board message buffer interface
#include "codebook_rcu.h"  // the mapping of the rcu memory
#include "ce_base.h" // common service handling
#ifdef TPC
#include "ce_tpc.h"
#endif //TPC
#ifdef PHOS
#include "ce_phos.h"
#endif //PHOS
#include "rcu_service.h"     // handling of the service API methods for rcu-like CEs
#include "rcu_issue.h"     // handling of the issue API method for rcu-like CEs
#include <unistd.h>	//for usleep
#include "dev_rcu.hpp"

/**
 * @file rcu_service.c
 * Service handling of the RCU, belongs to @ref rcu_service.
 */

/***************************************************************************
 * handling of services
 */
int     g_numberOfAdrs=0;

Item**     g_pRegItem=0;
__u32*   g_regItemAdr=0;
char** g_pRegItemName=0;
float*     g_deadband=0;
float* g_regItemValue=0;

int updateReg(TceServiceData* pData, int regNum, int dummy, void* parameter){
  int ceState=CE_OK;
#ifdef RCU
  int nRet=CE_OK;
  CErcu* pRCU=(CErcu*)parameter;
  CEState states[]={eStateOn, eStateConfiguring, eStateConfigured, eStateRunning, eStateInvalid};
  if (pData && pRCU && pRCU->Check(states)) {
#ifndef RCUDUMMY
    __u32 u32RawData;
    nRet=rcuSingleRead(g_regItemAdr[regNum],&u32RawData);
    if(nRet<0){
      pData->fVal=CE_FSRV_NOLINK;
      CE_Warning("updateReg: rcuSingleRead failed\n");
      ceState=FEE_FAILED;
      return ceState;
    }
    pData->fVal=u32RawData;
#endif // RCUDUMMY
  } else {
    CE_Error("updateReg: invalid location\n");
  }
#endif //RCU
  return ceState;
}

__u32* cnvAllExtEnvI(char** values){
  int i = 0;
  int arrSize = 0;
  char* pEnd=0;
  __u32* retVal=0;
  if(values==NULL){
    return NULL;
  }
  arrSize = (sizeof values/sizeof values[0]);
  retVal = (__u32*)malloc(arrSize*sizeof(__u32*));
  if (retVal == NULL) {
    CE_Error("no memory available\n");
    fflush(stdout);
    return NULL;
  }
  for(i=0; 0<arrSize; i++){
    if(values[i]!=NULL){
      retVal[i] = strtoul(values[i],&pEnd,0);
    }
    else{
      retVal[i]=0;
    }
  }
  free(pEnd);
  return retVal;
}

float* cnvAllExtEnvF(char** values){
  int i=0;
  int arrSize=0;
  float* retVal=0;
  if(values==NULL){
    return NULL;
  }
  arrSize = (sizeof values/sizeof values[0]);
  retVal = (float*)malloc(arrSize*sizeof(float*));
  if (retVal == NULL) {
    CE_Error("no memory available\n");
    fflush(stdout);
    return NULL;
  }
  for(i=0; 0<arrSize; i++){
    if(values[i]!=NULL){
      retVal[i] = atof(values[i]);
    }
    else{
      retVal[i]=0;
    }
  }
  return retVal;
}

//__u32 cnvExtEnv(char* var){
//  __u32 retVal = 0;
//  unsigned char i=0;
//  while(var!= 0 && var[i]!='\0' && i<g_nofFECs && (var[i]=='0' || var[i]=='1')){
//    retVal += var[i]*pow(2,i);
//    i++;
//  }
//  return retVal;
//}

/**
char** splitString(char* string){
  char** retVal = 0;
  __u32 nextPos = 0;
  __u32 curPos  = 0;
  unsigned short int nSeg = 1;
  unsigned char i = 0;
  if(string==NULL){
    return retVal;
  }
  while (string[i]!='\0'){
    if(string[i]==','){
      nSeg++;
    }
    i++;
  }
  retVal = (char**) malloc(nSeg*sizeof(char*));
  for(i=0;i<nSeg;i++){
    curPos = nextPos;
    if(i==nSeg-1){
      nextPos = (__u32)strchr(string+curPos, '\0');
    }
    else{
      nextPos = (__u32)strchr(string+curPos, ',');
    }
    retVal[i] = (char*) malloc((nextPos-curPos)*sizeof(char));
    strncpy (retVal[i],string+curPos+1,nextPos-curPos-1);
    retVal[i][nextPos-curPos]='\0';
  }
  return retVal;
}
*/
