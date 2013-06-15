// $Id: ce_tpc.cpp,v 1.15 2007/08/28 14:53:24 richter Exp $

/************************************************************************
**
**
** This file is property of and copyright by the Experimental Nuclear 
** Physics Group, Dep. of Physics and Technology
** University of Bergen, Norway, 2004
** This file has been written by Matthias Richter,
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

/***************************************************************************
 * ce_tpc.c
 * this file implements the specific methods for the TPC
 */

#include <errno.h>
#include <stdlib.h>       // malloc, free,
#include <string.h>       // memcpy
#include "fee_errors.h"
#include "ce_command.h"   // CE API
#include "codebook_rcu.h" // the mapping of the rcu memory
#include "dcscMsgBufferInterface.h" // access library to the dcs board message buffer interface
#include "ce_base.h"
#include "ce_tpc.h"
#include "device.hpp"     // CEResultBuffer

using namespace std;

#ifdef TPC

/*******************************************************************************
 *
 *                           TPCagent
 *
 ******************************************************************************/

const char* TPCagent::GetName()
{
  return "TPC ControlEngine";
}

ControlEngine* TPCagent::CreateCE()
{
  return new TPCControlEngine;
}

/** the global control engine instance */
TPCagent g_TPCagent;

/*******************************************************************************
 *
 *                           TPCControlEngine implementation
 *
 ******************************************************************************/

TPCControlEngine::TPCControlEngine()
{
  fpBranchLayout=NULL;
}

TPCControlEngine::~TPCControlEngine()
{
}

int TPCControlEngine::InitCE()
{
  int iResult=0;
  fpBranchLayout=new TPCBranchLayout;
  if (fpBranchLayout) {
    SetRcuBranchLayout(fpBranchLayout);
  } else {
    iResult=-ENOMEM;
  }
  return iResult;
}

int TPCControlEngine::DeinitCE()
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
 *                           CEfecTPC implementation
 *
 ******************************************************************************/

CEfecTPC::CEfecTPC(int id, CEDevice* pParent, std::string arguments)
  : CEfec(id, pParent, arguments)
{
}

CEfecTPC::~CEfecTPC()
{
}

const CEfec::service_t CEfecTPC::fServiceDesc[]={
  // optional services (remove the comment if desired)
//   {"T_TH",    1, 0.25  , 0.01 , 40  , -1, 0},
//   {"AV_TH",   2, 0.0043, 0.01 , 3.61, -1, 0}, 
//   {"AC_TH",   3, 0.017 , 0.01 , 0.75, -1, 0}, 
//   {"DV_TH",   4, 0.0043, 0.01 , 2.83, -1, 0}, 
//   {"DC_TH",   5, 0.03  , 0.01 , 1.92, -1, 0},
//   {"L1CNT",  11,   1   , 0.0  , 0   , -1, 0},
//   {"L2CNT",  12,   1   , 0.0  , 0   , -1, 0},
//   {"SCLKCNT",13,   1   , 0.0  , 0   , -1, 0},
//   {"DSTBCNT",14,   1   , 0.0  , 0   , -1, 0},
//   {"TSMWORD",15,   1   , 0.0  , 0   , -1, 0},
//   {"USRATIO",16,   1   , 0.0  , 0   , -1, 0},
  // the default services
  {"TEMP",    6, 0.25  , 1.0  , 40  ,  50, 0},
  {"AV",      7, 0.0043, 0.05 , 3.61, -1, 0},
  {"AC",      8, 0.017 , 0.5 , 0.75, -1, 0},
  {"DV",      9, 0.0043, 0.05 , 2.83, -1, 0},
  {"DC",     10, 0.03  , 0.5 , 1.92, -1, 0}
};

int CEfecTPC::GetServiceDescription(const CEfec::service_t*  &pArray) const
{
  pArray=fServiceDesc;
  return sizeof(fServiceDesc)/sizeof(CEfec::service_t);
}

/*******************************************************************************
 *
 *                           TPCBranchLayout
 *
 ******************************************************************************/

TPCBranchLayout::TPCBranchLayout()
  :
  fNofA(-2),
  fNofB(-2),
  fLayoutSize(0),
  fLayout(NULL)
{
}

TPCBranchLayout::~TPCBranchLayout()
{
  if (fLayout) delete [] fLayout;
  fLayout=NULL;
  fLayoutSize=0;
}

int TPCBranchLayout::GetNofFecBranchA()
{
  if (fNofA<-1) SetBranchLayoutFromServerName();
  return fNofA;
}

int TPCBranchLayout::GetNofFecBranchB()
{
  if (fNofB<-1) SetBranchLayoutFromServerName();
  return fNofB;
}

CEfec* TPCBranchLayout::CreateFEC(int id, CEDevice* pParent)
{
  return new CEfecTPC(id, pParent);
}

int TPCBranchLayout::SetBranchLayoutFromServerName()
{
  const char* serverName=getenv("FEE_SERVER_NAME");
  const char* key="TPC-FEE_";
  int side=-1;
  int sec=-1;
  int part=-1;
  if (serverName!=NULL && strncmp(serverName, key, strlen(key))==0) {
    string strServerName=serverName;
    strServerName.erase(0, strlen(key));
    const char* pBuffer=strServerName.c_str();
    if (sscanf(pBuffer, "%d", &side)>0 &&
	sscanf(pBuffer+2, "%d", &sec)>0 &&
	sscanf(pBuffer+5, "%d", &part)>0) {
      CE_Info("setting up FEC layout for side/sector/partition %d/%d/%d\n", side, sec, part);
      int arraySize=32;
      fLayout=new int[arraySize];
      if (fLayout) {
	fLayoutSize=arraySize;
	memset(fLayout, 0xff, fLayoutSize*sizeof(int)); // set all to -1
	int i=0;
	int fec=0;
	int fecsA=9;
	int fecsB=9;
	if (part>2) {fecsA++; fecsB++;}
	if (part==1) {fecsA=13; fecsB=12;}
	// FEC naming convention fixed in July 2007: identical on both
	// sides
// 	if (side==0) {
	  for (i=fecsA-1; i>=0; i--) fLayout[i]=fec++;
	  for (i=0; i<fecsB; i++) fLayout[i+16]=fec++;
// 	} else {
// 	  for (i=fecsB-1; i>=0; i--) fLayout[i+16]=fec++;
// 	  for (i=0; i<fecsA; i++) fLayout[i]=fec++;
// 	}	
      }
    }
  }
  if (fLayout==NULL) {
    CE_Info("side/sector/partition %d/%d/%d\n", side, sec, part);
  }
  fNofA=-1; fNofB=-1; // indicate that we have processed evaluation
  return 0;
}

int TPCBranchLayout::GetBranchLayout(int* &array)
{
  if (fLayoutSize>0) {
    array=fLayout;
    return fLayoutSize;
  }
  return RcuBranchLayout::GetBranchLayout(array);
}

#endif //TPC
