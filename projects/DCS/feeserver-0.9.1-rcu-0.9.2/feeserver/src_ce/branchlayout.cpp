// $Id: branchlayout.cpp,v 1.2 2007/06/04 22:35:12 richter Exp $

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
#include <cstdlib>
#include "ce_base.h"
#include "branchlayout.hpp"

RcuBranchLayout::RcuBranchLayout()
{
}

RcuBranchLayout::~RcuBranchLayout()
{
}

int RcuBranchLayout::GetNofFecBranchA()
{
  return -1;
}

int RcuBranchLayout::GetNofFecBranchB()
{
  return -1;
}

int RcuBranchLayout::GetBranchLayout(int* &array)
{
  int iResult=0;
  array=NULL;
  int i=0;
  char* FECValidList = getenv("FEESERVER_FEC_MONITOR");
  fLayout.clear();
  if(FECValidList!=NULL){
    for(i=0; FECValidList[i]!=0 && iResult>=0; i++) {
      int valid=-1;
      if(FECValidList[i]=='1'){
	valid = i;
      } else if (FECValidList[i]!='0'){
	iResult=-EINVAL;
	CE_Error("environment variable FEESERVER_FEC_MONITOR not set properly\n");
      }
      fLayout.push_back(valid);
    }
  }
  if (iResult>=0) {
    iResult=fLayout.size();
    array=&fLayout[0];
  } else {
    fLayout.clear();
  }
  return iResult;
}
