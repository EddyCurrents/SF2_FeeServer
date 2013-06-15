// $Id: rcu_issue.cpp,v 1.39 2007/07/17 15:41:59 richter Exp $

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
 * rcu_issue.c
 * this file implements the handling of 'issue' call of the CE API
 */

#include <errno.h>
#include <stdlib.h>       // malloc, free,
#include <cstring>         // memcpy
#include <vector>         // vector arrays
#include <unistd.h>       // usleep
#include "fee_errors.h"
#include "ce_command.h"   // CE API
#include "ce_base.h"      // CE primitives
#include "device.hpp"     // CEResultBuffer
#include "dcscMsgBufferInterface.h" // access library to the dcs board message buffer interface
#include "codebook_rcu.h" // the mapping of the rcu memory
#include "rcu_service.h"
#include "rcu_issue.h"
#include "dev_rcu.hpp"
#include "controlengine.hpp"
#include "issuehandler.hpp"

using namespace std;

/*******************************************************************************/
/**
 * Global switch to the corresponding handlers.
 * The function checks the incoming command block for CE commands, encoded
 * Msg Buffer blocks end high level commands and calls the corresponding
 * handler to process the command. CE commands can appear in a sequence. The
 * handler functions must return the number of bytes they have processed in
 * the incoming buffer. There is no scanning functionality and decomposition
 * forseen in the switch, it is a sequential processing. Note that some of the
 * commands can not appear in a command sequence (@ref translateShellCommand).
 * @param buffer      the incoming command buffer
 * @param size        size of the incoming command buffer in byte
 * @param rb          result buffer to receive the command result
 * @param bSingleCmd  execute just the first command of a sequence
 * @return            number of processed bytes of the buffer, neg error code if
 *                    failed
 */
int translateCommand(char* buffer, int size, CEResultBuffer& rb, int bSingleCmd)
{
  int iResult=0;
  CE_Debug("translateCommand size=%d\n", size);
  int iProcessed=0;
  char* pData=buffer;
  int iNofTrailerBytes=0;
  char* searchKey=NULL;
  char* highlevelKey=NULL;
  int highlevelCmd=0;
  if (size>=2*sizeof(__u32) 
      && (*((__u32*)(pData+size-sizeof(__u32)))&CE_CMD_EM_MASK) 
         == CE_CMD_ENDMARKER) {
    // if the last word of the command block is an end-marker, this is exluded
    // from the payload sent to the indivifual handlers, this is due to 
    // commands which depent on the size of the payload and thus can not be
    // part of a command sequence
    iNofTrailerBytes=sizeof(__u32);
  }
  do {
    if (size<sizeof(__u32)) {
      CE_Error("unknown header format (less than 32bit)\n");
      iResult=-EPROTO;
    } else {
      if (pData!=buffer+iProcessed) {
	CE_Fatal("internal variable missmatch\n");
	iResult=-EFAULT;
	break;
      }
      __u32 u32header=0;
      if (size-iProcessed>=sizeof(__u32)) {
	u32header=*((__u32*)pData);
	CE_Debug("checking command header %#x\n", u32header);
	iResult=checkFeeServerCommand(u32header);
      } else {
	CE_Warning("truncated command header (less than 32bit)\n");
	iResult=0;
      }
      if (iResult==1) { // this is a FeeServer command
	// the complete command code
	__u32 cmd=u32header&(FEESERVER_CMD_MASK|FEESERVER_CMD_ID_MASK|FEESERVER_CMD_SUB_MASK);
	// truncated command code without sub id
	__u32 cmdId=u32header&(FEESERVER_CMD_MASK|FEESERVER_CMD_ID_MASK);
	// the parameter extracted from the header
	__u32 parameter=u32header&FEESERVER_CMD_PARAM_MASK;
	iProcessed+=sizeof(__u32);
	pData+=sizeof(__u32);
	
	
	CEIssueHandler* pH = CEIssueHandler::getFirstIH();
        iResult=0;
	
        while(pH!=NULL && iResult>=0){
	  int groupId=-1;
	  if((groupId=pH->GetGroupId())==cmdId ||
	     (groupId==-1 && pH->CheckCommand(cmd, parameter))){
	    iResult=pH->issue(cmd, parameter, pData, size-iProcessed-iNofTrailerBytes, rb);
	    break;
	  }
	  pH=CEIssueHandler::getNextIH();
	} 
	
	if(pH==NULL){
	  switch (cmdId) {
	  case FEESVR_CMD_DATA_RO:
	    // this was once defined but never imlemented
	    CE_Error("function not implemented\n");
	    iResult=-ENOSYS;
	    break;
	  default:
	    CE_Warning("unrecognized command id (%#x)\n", cmdId);
	    iResult=-ENOSYS;
	  }//end switch
	}//end if(pHProcessed)
	if (iResult>size-iProcessed-iNofTrailerBytes) {
	  CE_Error("command handler offset %d out of command block size %d\n", iResult, size-iProcessed);
	  iResult=-EPROTO;
	} else if (iResult>=0) {
	  CE_Debug("command %#x finished with result %d\n", u32header, iResult);
	  iProcessed+=iResult;
	  pData+=iResult;
	  iResult=0;
	  // check for the end-marker
	  __u32 u32tailer=*((__u32*)(pData));
	  if ((u32tailer&CE_CMD_EM_MASK) != CE_CMD_ENDMARKER) {
	    if (ceCheckOptionFlag(DEBUG_RELAX_CMD_CHECK)==0) {
	      CE_Error("missing end marker - got %x=(%x|%x) expected %x\n", 
		       u32tailer, u32tailer, CE_CMD_EM_MASK, CE_CMD_ENDMARKER);
	      iResult=-EPROTO;
	    }
	  } else {
	    iProcessed+=sizeof(__u32);
	    pData+=sizeof(__u32);
	  }
	}
      } else if (iResult==0 && (iResult=checkMsgBufferCommand(u32header))==1) {
	// this is an encoded buffer
	CE_Warning("handling of encoded message buffer data blocks not yet implemented\n");
	pData+=size-iProcessed;
	iProcessed=size;
      } else if (u32header==0) {
	// try the next word in the buffer
	pData+=sizeof(__u32);
	iProcessed+=sizeof(__u32);
      } else if (iResult==0) {
	CE_Error("unknown command header: %#x\n", u32header);
	iResult=-EPROTO;
      }
    }
  } while (iResult>=0 && size>iProcessed && bSingleCmd==0);
  if (iResult>=0) iResult=iProcessed;
  return iResult;
}

extern int g_iMaxBufferPrintSize;
int RCUce_Issue(char* command, char** result, int* size) {
  int iResult=0;
  if (command && size && *size!=0) {
    int iPrintSize=*size;
    if (*size>g_iMaxBufferPrintSize)
      iPrintSize=g_iMaxBufferPrintSize;
    if (iPrintSize>0) {
      printBufferHex((unsigned char*)command, iPrintSize, 4, "CE Debug: issue command buffer");
/*       char command_print[4]; */
/*       int i=0; */
/*       for (i=0; i<4; i++) { */
/* 	if (i<*size && command[i]>=0x20) command_print[i]=command[i]; */
/* 	else command_print[i]='.'; */
/*       } */
/*       CE_Debug("issue command buffer\n  %x %x %x %x    :  %c %c %c %c\n" */
/* 	       , command[0], command[1], command[2], command[3] */
/* 	       , command_print[0], command_print[1], command_print[2], command_print[3]); */
    }
    CEResultBuffer rb;
    rb.clear();
    iResult=translateCommand(command, *size, rb, 0);
    *size=0;
    *result=NULL;
    if (iResult>=0) {
      *size=rb.size()*sizeof(CEResultBuffer::value_type);
      if (*size>0) {
	*result=(char*)malloc(*size);
	if (*result) {
	  //CE_Info("result buffer of size %d\n", *size);
	  memcpy(*result, &(rb[0]), *size);
	  iPrintSize=*size;
	  if (*size>g_iMaxBufferPrintSize)
	    iPrintSize=g_iMaxBufferPrintSize;
	  if (iPrintSize>0)
	    printBufferHex((unsigned char*)*result, iPrintSize, 4, "CE Debug: issue result buffer");
	} else {
	  CE_Error("can not allocate result memory\n");
	}
      }
    } else {
    }
  } else {
  }
  return 0;
}

/*******************************************************************************
 * helper functions
 */

int MASK_SHIFT(int val, int mask) {
  int iResult=val&mask;
  int shift=0;
  while (((mask>>shift)&0x1)==0 && (mask>>shift)>0) shift++;
  iResult>>=shift;
  return iResult;
}

/* check if the provided header represents FeeServer command or not
 * result: 1 yes, 0 no
 */
int checkFeeServerCommand(__u32 header) {
  if ((header&FEESERVER_CMD_MASK)==FEESERVER_CMD) return 1;
  return 0;
}

/* check if the provided header represents a Msg Buffer command or not
 * result: 1 yes, 0 no
 */
int checkMsgBufferCommand(__u32 header) {
  __u32 masked=header&FEESERVER_CMD_MASK;
  switch (masked) {
    // all previous firmware versions are not supported as direct commands
  case MSGBUF_VERSION_2:    
  case MSGBUF_VERSION_2_2:
    return 1;
  }
  return 0;
}
/* extract command id from the provided header
 * result: 0 - 15 command id
 *         -1 no FeeServer command
 */
int extractCmdId(__u32 header) {
  if (checkFeeServerCommand(header)) return -1;
  return ((header&FEESERVER_CMD_ID_MASK)>>FEESERVER_CMD_ID_BITSHIFT);
}

/* extract command sub id from the provided header
 * result: 0 - 255 command id
 *         -1 no FeeServer command
 */
int extractCmdSubId(__u32 header) {
  if (checkFeeServerCommand(header)) return -1;
  return ((header&FEESERVER_CMD_SUB_MASK)>>FEESERVER_CMD_SUB_BITSHIFT);
}

