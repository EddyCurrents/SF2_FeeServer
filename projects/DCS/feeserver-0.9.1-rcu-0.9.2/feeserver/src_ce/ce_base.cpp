// $Id: ce_base.cpp,v 1.26 2007/09/11 22:33:11 richter Exp $

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

#include <errno.h>
#include <stdlib.h>        // malloc, free,
#include <stdio.h>
#include <stdarg.h>        // varargs
#include <string.h>
#include <stdio.h>
#include <sys/types.h>    // exec
#include <unistd.h>       // fork/exec
#include <sys/wait.h>     // wait command
#include "ce_command.h"
#include "fee_errors.h"
#include "ce_base.h"
#include "rcu_issue.h"
#include "device.hpp"     // CEResultBuffer
#include <cstring>
#include "issuehandler.hpp"
#include "rcu_issue.h"

using namespace std;

/*******************************************************************************************
 * global options
 */
#define CE_DEFAULT_OPTIONS DEBUG_RELAX_CMD_CHECK
//int g_ceoptions=CE_DEFAULT_OPTIONS;
int g_ceoptions=0;
int ceSetOptions(int options)
{
  g_ceoptions=options;
  return g_ceoptions;
}

int ceSetOptionFlag(int of)
{
  g_ceoptions|=of;
  return g_ceoptions;
}

int ceClearOptionFlag(int of)
{
  g_ceoptions&=~of;
  return g_ceoptions;
}

int ceCheckOptionFlag(int of)
{
  return (g_ceoptions&of)!=0;
}

int g_iMaxBufferPrintSize=4;
//int g_printError=0;

/******************************************************************************************/

/**
 * @class CEPropertiesCommandHandler
 * CEIssueHandler for commands concerning and explicitly handled by the CE.
 * Binary commands:
 *  - @ref CEDBG_SET_BUF_PRINT_SIZE
 *  - @ref CEDBG_EN_SERVICE_UPDATE
 *  - @ref CEDBG_SET_SERVICE_VALUE
 *  - @ref CE_SET_LOGGING_LEVEL
 *  - @ref CE_RELAX_CMD_VERS_CHECK
 * 
 * High-Level commands
 * <pre>
 *  CEDBG_SET_BUF_PRINT_SIZE <num>
 *  CEDBG_EN_SERVICE_UPDATE  <0,1>
 *  CE_SET_LOGGING_LEVEL     <level>
 *  CE_RELAX_CMD_VERS_CHECK
 * </pre>
 *
 * @ingroup rcu_ce_base_services
 */
class CEPropertiesCommandHandler : public CEIssueHandler
{
 public:
  /** constructor */
  CEPropertiesCommandHandler() {}

  /** destructor */
  ~CEPropertiesCommandHandler(){}

  /**
   * Get group id.
   * Invalid group Id in order to call @ref CEPropertiesCommandHandler::CheckCommand
   */
  int GetGroupId() {return -1;}

  /**
   * Enhanced selector method.
   * The handler is for the FEESVR_SET_FERO_DATA group and the 
   * CE_FORCE_CH_UPDATE command.
   * @return          1 if the handler can process the cmd with the parameter
   */
  int CheckCommand(__u32 cmd, __u32 parameter);

  /**
   * Get the expected size of the payload for a command.
   * @param cmd       command id stripped py the parameter
   * @param parameter the parameter (16 lsb of the command header)
   * @return number of expected 32 bit words
   */
  int GetPayloadSize(__u32 cmd, __u32 parameter);

  /**
   * Handler for binary commands.
   * @see CEIssueHandler::issue for parameters and return values.
   */
  int issue(__u32 cmd, __u32 parameter, const char* pData, int iDataSize, CEResultBuffer& rb);

  /**
   * Handler for high level commands.
   * @see CEIssueHandler::HighLevelHandler for parameters and return values.
   */
  int HighLevelHandler(const char* pCommand, CEResultBuffer& rb);
};

int CEPropertiesCommandHandler::CheckCommand(__u32 cmd, __u32 parameter)
{
  switch (cmd) {
  case CEDBG_SET_BUF_PRINT_SIZE:
  case CEDBG_EN_SERVICE_UPDATE:
  case CEDBG_SET_SERVICE_VALUE:
  case CE_RELAX_CMD_VERS_CHECK:
  case CE_SET_LOGGING_LEVEL:
  case CE_GET_HIGHLEVEL_CMDS:
    return 1;
  }
  return 0;
}

int CEPropertiesCommandHandler::GetPayloadSize(__u32 cmd, __u32 parameter)
{
  return 0;
}

int CEPropertiesCommandHandler::issue(__u32 cmd, __u32 parameter, const char* pData, int iDataSize, CEResultBuffer& rb)
{
  int iResult=0;
  int iProcessed=0;
  CE_Debug("translateCeCommand cmd=%#x parameter=%#x\n", cmd, parameter);
  switch (cmd) {
  case CEDBG_SET_BUF_PRINT_SIZE:
    g_iMaxBufferPrintSize=parameter;
    CE_Info("printing maximum %d bytes of incoming buffer\n", g_iMaxBufferPrintSize);
    break;
  case CEDBG_EN_SERVICE_UPDATE:
    if (parameter==0)
      ceSetOptionFlag(DEBUG_DISABLE_SRV_UPDT);
    else
      ceClearOptionFlag(DEBUG_DISABLE_SRV_UPDT);
    break;
  case CEDBG_SET_SERVICE_VALUE:
    if (pData && iDataSize>0) {
      iProcessed=parameter+sizeof(__u32);
      if (iDataSize>=iProcessed) {
	__u32 value=*((__u32*)pData);
	const char* name=pData+sizeof(__u32);
	if (*(name+parameter)==0) {
	  iResult=ceWriteService(name, value);
	} else {
	  CE_Error("service name is not zero terminated\n");
	}
      } else {
	CE_Error("data size missmatch, %d byte(s) available, but %d expected\n", iDataSize, parameter+sizeof(__u32));
	iProcessed=0;
      }
    } else {
      CE_Error("write service value: no data avaiable\n");
    }
    break;
  case CE_RELAX_CMD_VERS_CHECK:
    iResult=ceSetOptionFlag(DEBUG_RELAX_CMD_CHECK);
    CE_Info("relax command end marker and version check\n");
    break;
  case CE_SET_LOGGING_LEVEL:
    iResult=ceSetLogLevel(parameter);
    CE_Info("set logging level to %d\n", iResult);
    break;
  case CE_GET_HIGHLEVEL_CMDS:
    if (pData && parameter<=iDataSize) {
      CE_Debug("CE_GET_HIGHLEVEL_CMDS %s\n", pData);
    }
    iProcessed=parameter;
    break;
  default:
    CE_Warning("unrecognized command id (%#x)\n", cmd);
    iResult=-ENOSYS;
  }

  if (iResult>=0) iResult=iProcessed;
  return iResult;
}

int CEPropertiesCommandHandler::HighLevelHandler(const char* pCommand, CEResultBuffer& rb)
{
  int iResult=0;
  __u32 cmd=0;
  int keySize=0;
  int len=strlen(pCommand);
  const char* pBuffer=pCommand;
  if (pCommand) {
    //CE_Debug("CEPropertiesCommandHandler checks command %s\n", pCommand);
    if (strncmp(pCommand, "CEDBG_SET_BUF_PRINT_SIZE", keySize=strlen("CEDBG_SET_BUF_PRINT_SIZE"))==0)
      cmd=CEDBG_SET_BUF_PRINT_SIZE;
    else if (strncmp(pCommand, "CEDBG_EN_SERVICE_UPDATE", keySize=strlen("CEDBG_EN_SERVICE_UPDATE"))==0)
      cmd=CEDBG_EN_SERVICE_UPDATE;
    else if (strncmp(pCommand, "CEDBG_SET_SERVICE_VALUE", keySize=strlen("CEDBG_SET_SERVICE_VALUE"))==0)
      cmd=CEDBG_SET_SERVICE_VALUE;
    else if (strncmp(pCommand, "CE_SET_LOGGING_LEVEL", keySize=strlen("CE_SET_LOGGING_LEVEL"))==0)
      cmd=CE_SET_LOGGING_LEVEL;
    else if (strncmp(pCommand, "CE_RELAX_CMD_VERS_CHECK", keySize=strlen("CE_RELAX_CMD_VERS_CHECK"))==0)
      cmd=CE_RELAX_CMD_VERS_CHECK;

    if (cmd>0 && keySize>0) {
      pBuffer+=keySize;
      len-=keySize;
      while (*pBuffer==' ' && *pBuffer!=0 && len>0) {pBuffer++; len--;} // skip all blanks
      __u16 data=0;
      switch (cmd) {
      case CEDBG_SET_BUF_PRINT_SIZE:
      case CEDBG_EN_SERVICE_UPDATE:
	//case CEDBG_SET_SERVICE_VALUE:
      case CE_SET_LOGGING_LEVEL:
	if (sscanf(pBuffer, "%d", &data)>0) {
	} else {
	  CE_Error("error scanning high-level command %s\n", pCommand);
	  iResult=-EPROTO;
	}
	break;
      }
      if (iResult>=0) {
	if ((iResult=issue(cmd, data, NULL, 0, rb))>=0) {
	  iResult=1;
	}
      }
    }
  } else {
    iResult=-EINVAL;
  }
  return iResult;
}

CEPropertiesCommandHandler g_PropertyCommandHandler;

/*******************************************************************************************
 * list handling
 */
TceServiceDesc* g_anchor=NULL; // list of services
int ceInsertService(TceServiceDesc* pEntry) {
  int iResult=0;
  if (pEntry) {
    if (g_anchor!=NULL) {
      pEntry->pNext=g_anchor;
    }
    g_anchor=pEntry;
  } else {
    iResult=-EINVAL;
  }
  return iResult;
}

TceServiceDesc* ceFindService(const char* name) {
  TceServiceDesc* pDesc=NULL;
  if (name) {
    pDesc=g_anchor;
    for (pDesc=g_anchor; pDesc!=NULL; pDesc=pDesc->pNext) {
      if (pDesc->pName) {
	string* strName=(string*)pDesc->pName;
	if (strName->compare(name)==0) {
	  break;
	}
      } else {
	CE_Error("uninitialized item for entry %p\n", pDesc);
      }
    }
  }
  return pDesc;
}

int ceCleanupServices() {
  int iResult=0;
  TceServiceDesc* pDesc=g_anchor;
  g_anchor=NULL;
  while (pDesc!=NULL) {
    TceServiceDesc* pNext=pDesc->pNext;
    // the item and the name is released by the feeserver core
    free(pDesc);
    pDesc=pNext;
  }
  return iResult;
}

// forward declaration of the wrapper function implemented in ce_command.c
extern "C" int ce_dis_update_service(unsigned int id);
extern "C" unsigned int ce_dis_add_service(char* service, char* type, void* buffer, int size, ceDisServiceCallback cb, long int tag);
extern "C" int UpdateFeeService(const char* serviceName);

int g_abort=0;

void ceAbortUpdate()
{
  g_abort=1;
}

int ceUpdateServices(const char* pattern, int bForce) {
  int iResult=0;
  if (ceCheckOptionFlag(DEBUG_DISABLE_SRV_UPDT)==0) {
    TceServiceDesc* pDesc=g_anchor;
    g_abort=0;
    for (pDesc=g_anchor; pDesc!=NULL; pDesc=pDesc->pNext) {
      if (g_abort) {
	CE_Info("abort of update loop requested, terminate\n");
	break;
      }
      string name("unknown");
      if (pDesc->pName) name=*((string*)pDesc->pName);
      //CE_Debug("updating service %s\n", name.c_str());
      if (pDesc->pFctUpdate && (pattern==NULL ||
				name.compare(0, strlen(pattern), pattern)==0)) {
	string strBackup("");
	int iBackup=0;
	switch (pDesc->datatype) {
	case eDataTypeInt: iBackup=pDesc->data.iVal; break;
	case eDataTypeString: 
	  if (pDesc->data.strVal) {
	    strBackup=*((string*)pDesc->data.strVal);
	  }
	  break;
	}
	int iTempres=0;
	if ((iTempres=(*pDesc->pFctUpdate)(&pDesc->data, pDesc->major, pDesc->minor, pDesc->parameter))<0) {
	  CE_Warning("update for entry %p (%s) returned %d\n", pDesc, name.c_str(), iTempres);
	  iResult=-EREMOTEIO;
	} else {
	  int doUpdate=bForce;
	  switch (pDesc->datatype) {
	  case eDataTypeInt: doUpdate=(iBackup!=pDesc->data.iVal); break;
	  case eDataTypeString:
	    if (pDesc->data.strVal) {
	      doUpdate|=strBackup.compare(*((string*)pDesc->data.strVal));
	    }
	    break;
	  }
	  if (doUpdate) {
	    int count=0;
	    if (pDesc->dimid>0) {
	      count=ce_dis_update_service(pDesc->dimid);
	    } else {
	      count=UpdateFeeService(name.c_str());      
	    }
	    if (count>=0 && bForce) {
	      CE_Info("service %s updated to %d client(s)\n", name.c_str(), count);
	    }
	  }
	}
      }
    }
  }
  return iResult;
}

int ceSetValue(const char* name, float value) {
  int iResult=0;
  if (name) {
    TceServiceDesc* pDesc=ceFindService(name);
    if (pDesc) {
      if (pDesc->pFctSet) {
	TceServiceData data;
	data.fVal=value;
	iResult=(*pDesc->pFctSet)(&data, pDesc->major, pDesc->minor, pDesc->parameter);
      } else {
	CE_Warning("service %s has no set function\n", name);
	iResult=-ENOLINK;
      }
    } else {
      CE_Warning("service %s not found\n", name);
      iResult=-ENOENT;
    }
  } else {
    CE_Error("invalid argument\n");
    iResult=-EINVAL;
  }
  return iResult;
}

int ceWriteService(const char* name, float value)
{
  int ceState=0;
  if (name) {
    if (g_ceoptions&DEBUG_DISABLE_SRV_UPDT) {
      TceServiceDesc* pDesc=ceFindService(name);
      if (pDesc) {
	pDesc->data.fVal=value;
      } else {
	createLogMessage(MSG_ERROR, "invalid service ", "CE::ceWriteService");
	ceState=-ENOENT;
      }
    } else {
      createLogMessage(MSG_WARNING, "set value is only possible if service update is deactivated", "CE::ceWriteService");
    }
  } else {
    createLogMessage(MSG_ERROR, "invalid parameter ", "CE::ceWriteService");
    ceState=-EINVAL;
  }
  return ceState;
}

void updateStringService(void* tag, void** buffer, int* size, int* firstTime) {
  if (tag!=NULL && buffer!=NULL && size!=NULL) {
    *buffer=NULL;
    *size=0;    
    long int param=*((long int*)tag);
    string* data=(string*)param;
    if (data) {
      *buffer=(char*)data->c_str();
      *size=data->length()+1;
    }
  }
}

int RegisterService(enum ceServiceDataType type, const char* name, float defDeadband, ceUpdateService pFctUpdate, ceSetFeeValue pFctSet, int major, int minor, void* parameter) {
  int iResult=0;
  TceServiceDesc* pEntry=NULL;
  if (name != NULL) {
    if (pFctUpdate==NULL && pFctSet==NULL) {
      CE_Warning("neither update nor set function available for service %s\n", name);
    }
    pEntry=(TceServiceDesc*)malloc(sizeof(TceServiceDesc));
    if (pEntry) {
      static string serverName;
      const char* envname=getenv("FEE_SERVER_NAME");
      if (envname) serverName=envname;
      memset(pEntry, 0, sizeof(TceServiceDesc));
      string* strName=new string(name);
      pEntry->pName=(void*)strName;
      switch (type) {
      case eDataTypeFloat:
	{
	  // allocate the original Item struct for float channels
	  // the struct is freed by the core
	  Item* pItem=(Item*)malloc(sizeof(Item));
	  if (pItem) {
	    pEntry->pItem=pItem;
	    memset(pItem, 0, sizeof(Item));
	    // allocate space for the name, freed by the core
	    pItem->name=(char*)malloc(strName->size()+1);
	    if (pItem->name) {
	      strcpy(pItem->name, strName->c_str());
	    }
	    pItem->location=&pEntry->data.fVal;
	    pEntry->data.fVal=CE_FSRV_NOLINK;
	    pItem->defaultDeadband=defDeadband;
	    if ((iResult=publish(pEntry->pItem))==CE_OK) {
	    } else {
	      iResult=-EFAULT;
	    }
	  } else {
	    iResult=-ENOMEM;
	  }
	}
	break;
      case eDataTypeInt:
	{
	  string serviceName=serverName + "_" + *strName;
	  char* sn=(char*)serviceName.c_str();
	  pEntry->dimid=ce_dis_add_service(sn, "I", 
					   &pEntry->data.iVal, sizeof(int), NULL, 0);
	  pEntry->data.iVal=CE_ISRV_NOLINK;
	  pEntry->databackup.iVal=~CE_ISRV_NOLINK; // publish at least once
	}
	break;
      case eDataTypeString:
	{
	  string serviceName=serverName + "_" + *strName;
	  char* sn=(char*)serviceName.c_str();
	  string* pStrData=new string("");
	  if (pStrData) {
	    string* pStrBackup=new string("backup"); // publish at least once
	    if (pStrBackup) {
	      pEntry->data.strVal=pStrData;
	      pEntry->databackup.strVal=pStrBackup;
	      pEntry->dimid=ce_dis_add_service(sn, "C", 0, 0, 
					       updateStringService, (long int)pEntry->data.strVal);
	    } else {
	      delete pStrData;
	      iResult=-ENOMEM;
	    }
	  } else {
	    iResult=-ENOMEM;
	  }
	}
	break;
      default:
	iResult=-EINVAL;
      }
      if (iResult>=0) {
	CE_Info("service %s of type %d registered\n", name, type);
	pEntry->datatype=type;
	pEntry->pFctUpdate=pFctUpdate;
	pEntry->pFctSet=pFctSet;
	pEntry->major=major;
	pEntry->minor=minor;
	pEntry->parameter=parameter;
	ceInsertService(pEntry);
      } else {
	if (pEntry) free(pEntry);
      }
    } else {
      iResult=-ENOMEM;
    }
  } else {
    CE_Error("invalid parameter\n");
    iResult=-EINVAL;
  }
  return iResult;
}

int RegisterServiceGroup(enum ceServiceDataType type, const char* basename, int count, float defDeadband, ceUpdateService pFctUpdate, ceSetFeeValue pFctSet, int minor, void* parameter) {
  int iResult=0;
  if (basename && count>0) {
    int digits=1;
    int i=count;
    while ((i/=10)>0) digits++;
    if (digits<9) {
      //CE_Debug("basename=%s count=%d digits=%d\n", basename, count, digits);
      int namelen=strlen(basename)+2+digits;
      char* name=(char*)malloc(namelen);
      char* format=(char*)malloc(namelen); // this has actually only indirect to do with namelen but its appropriate 
      if (name && format) {
	const char* key=strchr(basename, '%');
	strcpy(format, basename);
	if (key) {
	  int iPos=(key-basename)+1;
	  if (key[1]=='d') {
	    sprintf(format+iPos, "0*d");
	    iPos+=3;
	  } else {
	    *(format+iPos++)='%';
	    *(format+iPos++)=key[1];
	  }
	  strcpy(format+iPos, &key[2]);
	} else {
	  sprintf(format+strlen(basename), "_%%0*d");
	}
	//fprintf(stderr, "%s\n", format);
	for (i=0; i<count && iResult>=0; i++) {
	  sprintf(name, format, digits, i);
	  name[namelen-1]=0;
	  iResult=RegisterService(type, name, defDeadband, pFctUpdate, pFctSet, i, minor, parameter);
	}
      } else {
	iResult=-ENOMEM;
      }
      if (name) free(name);
      if (format) free(format);
    }
  } else {
    CE_Error("invalid parameter\n");
    iResult=-EINVAL;
  }
  return iResult;
}

/**
 * @class CEServiceCommandHandler
 * CEIssueHandler for the DIM services.
 * The @ref FEESVR_SET_FERO_DATA command group provides setting of values (write
 * operation) for data points in the Front-end electronics.
 * Binary commands:
 *  - @ref FEESVR_SET_FERO_DATA32
 *  - @ref FEESVR_SET_FERO_DATA16
 *  - @ref FEESVR_SET_FERO_DATA8
 *  - @ref CE_FORCE_CH_UPDATE
 * 
 * High-Level commands
 * <pre>
 *  FEESVR_SET_FERO_DATA32 <value>
 *  FEESVR_SET_FERO_DATA16 <value>
 *  FEESVR_SET_FERO_DATA8  <value>
 *  CE_FORCE_CH_UPDATE
 * </pre>
 *
 * @ingroup rcu_ce_base_services
 */
class CEServiceCommandHandler : public CEIssueHandler
{
 public:
  /** constructor */
  CEServiceCommandHandler() {}

  /** destructor */
  ~CEServiceCommandHandler(){}

  /**
   * Get group id.
   * Invalid group Id in order to call @ref CEServiceCommandHandler::CheckCommand
   */
  int GetGroupId() {return -1;}

  /**
   * Enhanced selector method.
   * The handler is for the FEESVR_SET_FERO_DATA group and the 
   * CE_FORCE_CH_UPDATE command.
   * @return          1 if the handler can process the cmd with the parameter
   */
  int CheckCommand(__u32 cmd, __u32 parameter);

  /**
   * Get the expected size of the payload for a command.
   * @param cmd       command id stripped py the parameter
   * @param parameter the parameter (16 lsb of the command header)
   * @return number of expected 32 bit words
   */
  int GetPayloadSize(__u32 cmd, __u32 parameter);

  /**
   * Issue handler basic service commands.
   * @see CEIssueHandler::issue for parameters and return values.
   */
  int issue(__u32 cmd, __u32 parameter, const char* pData, int iDataSize, CEResultBuffer& rb);

  /**
   * Handler for high level commands.
   * @see CEIssueHandler::HighLevelHandler for parameters and return values.
   */
  int HighLevelHandler(const char* pCommand, CEResultBuffer& rb);
};

int CEServiceCommandHandler::CheckCommand(__u32 cmd, __u32 parameter)
{
  if (cmd==CE_FORCE_CH_UPDATE) return 1;
  if ((cmd&(FEESERVER_CMD_ID_MASK|FEESERVER_CMD_MASK))==FEESVR_SET_FERO_DATA) return 1;
  return 0;
}

int CEServiceCommandHandler::GetPayloadSize(__u32 cmd, __u32 parameter)
{
  int iWordSize=0;
  switch (cmd) {
  case FEESVR_SET_FERO_DFLOAT:
    iWordSize=sizeof(float)/sizeof(__u32);
    //fall through
  case FEESVR_SET_FERO_DATA32:
    iWordSize+=2;
    //fall through
  case FEESVR_SET_FERO_DATA16:
    iWordSize+=1;
    //fall through
  case FEESVR_SET_FERO_DATA8:
    iWordSize+=1;
    //fall through
  case CE_FORCE_CH_UPDATE:
  default:
    // do nothing
    ;
  }
  return parameter+iWordSize;
}

int CEServiceCommandHandler::issue(__u32 cmd, __u32 parameter, const char* pData, int iDataSize, CEResultBuffer& rb)
{
  int iResult=0;
  int iProcessed=0;
  CE_Debug("setFeroData cmd=%#x parameter=%#x datasize=%d\n", cmd, parameter, iDataSize);
  int iWordSize=0;
  switch (cmd) {
  case FEESVR_SET_FERO_DFLOAT:
    iWordSize=sizeof(float)/sizeof(__u32);
    //fall through
  case FEESVR_SET_FERO_DATA32:
    iWordSize+=2;
    //fall through
  case FEESVR_SET_FERO_DATA16:
    iWordSize+=1;
    //fall through
  case FEESVR_SET_FERO_DATA8:
    iWordSize+=1;
    //fall through
  case CE_FORCE_CH_UPDATE:
    if (pData && iDataSize>0) {
      if (iDataSize>=parameter+iWordSize) {
	iProcessed=parameter+iWordSize;
	const char* name=pData+iWordSize;
	CE_Debug("set fero data: %s\n", name);
	if (*(name+parameter-1)==0) {
	  if (cmd==CE_FORCE_CH_UPDATE) {
	    iResult=ceUpdateServices(name, 1);
	  } else {
	    float fVal=0;
	    switch (cmd) {
	    case FEESVR_SET_FERO_DFLOAT:
	      fVal=*((float*)pData);
	      break;
	    case FEESVR_SET_FERO_DATA32:
	      fVal=*((__u32*)pData);
	      break;
	    case FEESVR_SET_FERO_DATA16:
	      fVal=*((__u16*)pData);
	      break;
	    case FEESVR_SET_FERO_DATA8:
	      fVal=*((__u8*)pData);
	      break;
	    }
	    CE_Debug("set value for service \'%s\' (%f)\n", name, fVal);
	    iResult=ceSetValue(name, fVal);
	  }
	} else {
	  CE_Error("service name is not zero terminated\n");
	}
      } else {
	CE_Error("data size missmatch, %d byte(s) available, but %d expected\n", iDataSize, parameter+sizeof(__u32));
      }
    } else {
      CE_Error("write service value: no data avaiable\n");
    }
    break;
  default:
    CE_Warning("unknown command id (%#x)\n", cmd);
    iResult=-ENOSYS;
  }
  if (iResult>=0) iResult=iProcessed;
  return iResult;
}

int CEServiceCommandHandler::HighLevelHandler(const char* pCommand, CEResultBuffer& rb)
{
  int iResult=0;
  __u32 cmd=0;
  int keySize=0;
  int len=strlen(pCommand);
  const char* pBuffer=pCommand;
  if (pCommand) {
    //CE_Debug("CEServiceCommandHandler checks command %s\n", pCommand);
    if (strncmp(pCommand, "FEESVR_SET_FERO_DATA32", keySize=strlen("FEESVR_SET_FERO_DATA32"))==0)
      cmd=FEESVR_SET_FERO_DATA32;
    else if (strncmp(pCommand, "FEESVR_SET_FERO_DATA16", keySize=strlen("FEESVR_SET_FERO_DATA16"))==0)
      cmd=FEESVR_SET_FERO_DATA16;
    else if (strncmp(pCommand, "FEESVR_SET_FERO_DATA8", keySize=strlen("FEESVR_SET_FERO_DATA8"))==0)
      cmd=FEESVR_SET_FERO_DATA8;
    else if (strncmp(pCommand, "CE_FORCE_CH_UPDATE", keySize=strlen("CE_FORCE_CH_UPDATE"))==0)
      cmd=CE_FORCE_CH_UPDATE;

    if (cmd>0 && keySize>0) {
      pBuffer+=keySize;
      len-=keySize;
      while (*pBuffer==' ' && *pBuffer!=0 && len>0) {pBuffer++; len--;} // skip all blanks
      const char* pServiceName=pBuffer;
      int iNameLen=0;
      while (pServiceName[iNameLen]!=0 && pServiceName[iNameLen]!=' ') iNameLen++; // jump over the service name
      iNameLen++;
      if (iNameLen>1) {
	int iPos=0;
	__u32 data=0;
	switch (cmd) {
	case CE_FORCE_CH_UPDATE:
	  break;
	case FEESVR_SET_FERO_DATA32:
	  iPos+=2;
	  // fall through
	case FEESVR_SET_FERO_DATA16:
	  iPos+=1;
	  // fall through
	case FEESVR_SET_FERO_DATA8:
	  iPos+=1;
	  if (sscanf(&pServiceName[iNameLen], "%d", &data)>0) {
	  } else {
	    iResult=-EPROTO;
	  }
	  break;
	}
	int iBufferSize=((iNameLen+iPos+3)/sizeof(__u32));
	__u32* cmdBuffer=new __u32[iBufferSize];
	if (cmdBuffer) {
	  if (iResult>=0) {
	    if (iPos>0)
	      memcpy((char*)&cmdBuffer[0], (char*)&data, iPos);
	    cmdBuffer[iBufferSize-1]=0;
	    strncpy(((char*)&cmdBuffer[0])+iPos, pServiceName, iNameLen-1);
	    iResult=issue(cmd, iNameLen, (const char*)cmdBuffer, iBufferSize*sizeof(__u32), rb);
	  }
	  delete [] cmdBuffer;
	}
      } else {
	CE_Error("invalid service name in high-level command %s\n", pCommand);
	iResult=-EPROTO;
      }
    }
  } else {
    iResult=-EINVAL;
  }
  return iResult;
}

CEServiceCommandHandler g_ServiceCommandHandler;

/******************************************************************************/
/**
 * @defgroup CE_issue  The standard issue handling
 * @note Definition: 
 * @ingroup rcu_ce_base
 */

/**
 * Find the next end-marker in the data buffer
 * @param   pData      buffer containing the command payload
 * @param   iDataSize  size of the payload
 * @return  offset in byte, neg. error code if failed
 * @ingroup CE_issue
 */
int findNextEndMarker(const char* pData, int iDataSize)
{
  int iResult=0;
  if (pData) {
    //void* pMarker=memchr(pData, , iDataSize);
  } else {
    CE_Error("invalid argument\n");
    iResult=-EINVAL;
  }
  return iResult;
}

/******************************************************************************/
/**
 * @defgroup CE_shex  The shell execution command group
 * @note Definition: 
 * @ingroup rcu_ce_base
 */
//extern int RCUce_SetResult(char* buffer, int size);

#ifdef OLD_PIPE_READ

/* @struct  streamBuffer
 * @ingroup CE_shex
 * @brief   The stream buffer descriptor
 */
typedef struct streamBuffer_t TstreamBuffer;
struct streamBuffer_t {
  int iSize;
  char* pBuffer;
  TstreamBuffer* pNext;
};

/* cleanup a stream buffer list
 * the functions releases all buffers in the descriptors as well as all
 * descriptors except the anchor, the content of the anchor is set to zero
 * @param anchor  pointer to the first element of the list
 * @ingroup CE_shex
 */
int cleanStreamBuffers(TstreamBuffer* anchor)
{
  int iResult=0;
  if (anchor) {
    TstreamBuffer* iter=anchor;
    while (iter && iResult>=0) {
      if (iter->pBuffer) {
	free(iter->pBuffer);
      }
      TstreamBuffer* pCurr=iter;
      iter=iter->pNext;
      if (pCurr!=anchor)
	free(pCurr);
      else
	memset(pCurr,0,sizeof(TstreamBuffer));
    }
  } else {
    CE_Error("invalid parameter\n");
    iResult=-EINVAL;
  }
  return iResult;
}

/* read from an open stream
 * Buffers of the default size are allocated, filled placed in the buffer list.
 * The first element is the anchor of the list. As long as the 'pNext' member
 * is not NULL, there are subsequent buffers in the list.
 * The function returns the total size of the stream
 * @param fp                  stream file descriptor
 * @param iDefaultBufferSize  default size for buffer allocation
 * @param anchor              first element of the buffer list
 * @ingroup CE_shex
 */
int readStream(FILE* fp, int iDefaultBufferSize, TstreamBuffer* anchor)
{
  int iResult=0;
  if (fp && anchor) {
    int iBufferSize=iDefaultBufferSize;
    if (iBufferSize==0) iBufferSize=1024;
    TstreamBuffer** desc=&anchor;
    do {
      if (desc) {
	if (*desc==NULL) {
	  *desc=(TstreamBuffer*)malloc(sizeof(TstreamBuffer));
	}
	if (*desc) {
	  memset(*desc, 0, sizeof(TstreamBuffer));
	  (*desc)->pBuffer=(char*)malloc(iBufferSize);
	  if ((*desc)->pBuffer) {
	    (*desc)->iSize=fread((*desc)->pBuffer, 1, iBufferSize-1, fp);
	    if ((*desc)->iSize>0) {
	      memset((*desc)->pBuffer+(*desc)->iSize, 0, iBufferSize-(*desc)->iSize);
	      iResult+=(*desc)->iSize;
	    } else {
	      free((*desc)->pBuffer);
	      (*desc)->pBuffer=NULL;
	    }
	  } else {
	    CE_Error("no memory available, failed to allocate %d bytes\n", iBufferSize);
	    iResult=-ENOMEM;
	  }
	  if (ferror(fp)) {
	    iResult=-EIO;
	  }
	  if (iResult>=0)
	    CE_Debug("accumulated %d byte(s)\n", iResult);
	  else 
	    CE_Error("abort file descriptor reading with error code %d\n", iResult);
	  desc=&((*desc)->pNext);
	} else {
	  CE_Error("no memory available, failed to allocate %d bytes\n", iBufferSize);
	  iResult=-ENOMEM;
	}
      } else {
	CE_Error("fatal internal failure\n");
	iResult=-EFAULT;
      }
    } while (iResult>0 && !feof(fp));
    if (iResult<=0) {
      cleanStreamBuffers(anchor);
    }
  } else {
    CE_Error("invalid pointer\n");
    iResult=-EINVAL;
  }
  return iResult;
}

/* flush the given stream buffers
 * the content of the stream buffer list is flushed until to last newline and printed 
 * either to the provided file or via the standard log channel as info
 * The characters after the last newline in the pipe are kept if bComplete is false.
 * All flushed buffers are rearranged and released, eventually remaining characters are
 * placed in anchor.
 * @param anchor              first element of the buffer list
 * @param fpOutput            stream file descriptor
 * @param bComplete           flash completely
 * @param pMsg                message header for the standard log methods
 */
int flushStreamBuffer(TstreamBuffer* anchor, FILE* fpOutput, int bComplete, const char* pMsg)
{
  int iResult=0;
  if (anchor) {
    TstreamBuffer* iter=anchor;
    TstreamBuffer* pLastIter=NULL;
    char* pRemaining=NULL; 
    while (iter && iResult>=0) {
      //CE_Debug("iterator=%p last iterator=%p\n", iter, pLastIter);
      if (iter->pBuffer && iter->iSize>0) {
	//CE_Debug("processing stream buffer %p\n", iter->pBuffer);
	if (fpOutput) {
	  // the simple case just writes the stream buffer as they are to the file
	  fwrite(iter->pBuffer, 1, iter->iSize, fpOutput);
	  free(iter->pBuffer);
	  iter->pBuffer=NULL;
	  iter->iSize=0;
	} else {
	  // using the CE logging method is more complicated, print only full lines
	  // unless bComplete==true
	  char* pStart=iter->pBuffer;
	  char* pNewline=NULL;
	  while (pStart!=NULL && iResult>=0 && 
		 ((pNewline=strchr(pStart, '\n'))!=NULL           // print until the next newline 
		  || (bComplete && (pStart<(iter->pBuffer+iter->iSize))) // or the remaining content even if there was no newline
		  || pRemaining!=NULL    // or force a newline since there is already sth left from the prev buffer
		  )
		 ) {
	    //CE_Debug("stepping into print loop\n");
	    if (pNewline) {
	      //CE_Debug("found newline at %p\n", pNewline);
	      if (pNewline<(iter->pBuffer+iter->iSize)) {
		*pNewline=0; // terminate the string at the position of the newline
	      } else {
		CE_Error("unterminated stream buffer\n");
		iResult=-EFAULT;
		break;
	      }
	    }
	    char* pTemp=pRemaining;    // placeholder for either the remaining of the previous buffer
	    if (pTemp==NULL) pTemp=""; // or an empty string
	    const char* pHeader=pMsg;
	    if (pHeader==NULL) pHeader="message from program stderr channel";
	    CE_Info("%s:\n%s%s\n", pHeader, pTemp, pStart);
	    if (pRemaining!=NULL && pLastIter!=NULL) {
	      // delete the last stream buffer
	      free(pLastIter->pBuffer);
	      pLastIter->pBuffer=NULL;
	      pLastIter->iSize=0;
	      // delete the last stream buffer descriptor if it is not the anchor
	      if (pLastIter!=anchor) {
		//free(pLastIter);
		pLastIter=NULL;
	      }
	    }
	    pRemaining=NULL; // whatever was the content of the remaining buffer, it has been printed
	    if (pNewline && (pNewline+1)<(iter->pBuffer+iter->iSize)) pStart=pNewline+1; 
	    else pStart=NULL;
	  }
	  if (pStart==NULL) {
	    // the buffer has completely been flushed
	    free(iter->pBuffer);
	    iter->pBuffer=NULL;
	    iter->iSize=0;
	  } else {
	    pRemaining=pStart;
	  }
	  //CE_Debug("leaving print loop: iResult=%d pStart=%p pRemaining=%p\n", iResult, pStart, pRemaining);
	}
      }
      pLastIter=iter;
      iter=iter->pNext;
      if (pLastIter->pBuffer==NULL) {
	if (pRemaining==NULL) {
	  if (pLastIter!=anchor) free(pLastIter);
	  pLastIter=NULL;
	} else {
	  // something severe wrong: pRemaining points into the universe
	  CE_Fatal("severe internal error\n");
	  iResult=-EFAULT;
	  break;
	}
      }
    }
    if (pRemaining!=NULL) {
      if (pLastIter->pBuffer!=NULL) {
	// copy the remaining data into the anchor
	int iCopyLen=strlen(pRemaining);
	if (iCopyLen==0) {
	  CE_Warning("empty buffer should be released at that point, possible memory leak\n");
	}
	if (pRemaining+iCopyLen>pLastIter->pBuffer+pLastIter->iSize) {
	  // string not terminated
	  CE_Warning("the stream buffer seams to be unterminated\n");
	  iCopyLen=((pLastIter->pBuffer+pLastIter->iSize)-pRemaining)-1;
	}
	if (anchor->pBuffer!=pLastIter->pBuffer) free(anchor->pBuffer);
	anchor->pBuffer=(char*)malloc(iCopyLen+1);
	if (anchor->pBuffer) {
	  memcpy(anchor->pBuffer, pRemaining, iCopyLen);
	  anchor->pBuffer[iCopyLen]=0;
	  anchor->iSize=iCopyLen; // information about original length of the buffer is lost, but it will be free'd anyway
	} else {
	}
      } else {
	// something severe wrong: pRemaining points to non existent data buffer
	CE_Fatal("severe internal error\n");
	iResult=-EFAULT;
      }
    } else {
      if (anchor->pBuffer!=NULL) {
	CE_Fatal("possible memory leak detected\n");
      }
      anchor->pBuffer=NULL;
      anchor->iSize=0;
    }
    anchor->pNext=NULL; // the contact to the next element is most likely already lost
    //if (pLastIter!=anchor) free(pLastIter);
    pLastIter=NULL;
  } else {
    CE_Error("invalid pointer\n");
    iResult=-EINVAL;
  }
  return iResult=0;
}
#endif //OLD_PIPE_READ

/* Flush a result buffer
 * The content of the result buffer is flushed until to last newline and printed 
 * either to the provided file or via the standard log channel as info
 * The characters after the last newline in the pipe are kept if bComplete is false.
 * In the latter case the remaining data is arranged at the beginning of the buffer.
 * @param pRB                 the result buffer
 * @param fpOutput            stream file descriptor
 * @param bComplete           flash completely
 * @param pMsg                message header for the standard log methods
 */
int flushStreamBuffer(CEResultBuffer* pRB, FILE* fpOutput, int bComplete, const char* pMsg)
{
  int iResult=0;
  if (pRB) {
    int iSize=pRB->size()*sizeof(CEResultBuffer::value_type);
    if (iSize>0) {
      char* pBuffer=(char*)&(*pRB)[0];
      if (fpOutput) {
	// the simple case just writes the stream buffer as they are to the file
	fwrite(pBuffer, 1, iSize, fpOutput);
	pRB->resize(0);
      } else {
	// using the CE logging method is more complicated, print only full lines
	// unless bComplete==true
	char* pStart=pBuffer;
	char* pNewline=NULL;
	while (pStart!=NULL && iResult>=0 && 
	       ((pNewline=strchr(pStart, '\n'))!=NULL     // print until the next newline 
		|| (bComplete && (pStart<pBuffer+iSize)) // or the remaining content even if there was no newline
		)
		 ) {
	    //CE_Debug("stepping into print loop\n");
	    if (pNewline) {
	      //CE_Debug("found newline at %p\n", pNewline);
	      if (pNewline<pBuffer+iSize) {
		*pNewline=0; // terminate the string at the position of the newline
	      } else {
		CE_Error("unterminated stream buffer\n");
		iResult=-EFAULT;
		break;
	      }
	    }
	    const char* pHeader=pMsg;
	    if (pHeader==NULL) pHeader="message from program stderr channel";
	    CE_Info("%s:\n%s\n", pHeader, pStart);
	    if (pNewline && (pNewline+1)<pBuffer+iSize) pStart=pNewline+1; 
	    else pStart=NULL;
	  }
	if (pStart!=NULL && pStart<pBuffer+iSize) {
	  CE_Warning("partial flushing currentky not implemented, print out truncated\n");
	}
	pRB->resize(0);
      }
    }
  } else {
    CE_Error("invalid pointer\n");
    iResult=-EINVAL;
  }
  return iResult=0;
}

/* read from an open stream into a vector
 * @param fp                  stream file descriptor
 * @param iDefaultBufferSize  default size for buffer allocation
 * @param pRB                 pointer to issue result buffer
 * @ingroup CE_shex
 */
int readStreamIntoVector(FILE* fp, int iDefaultBufferSize, CEResultBuffer* pRB)
{
  int iResult=0;
  if (fp && pRB) {
    int iBufferSize=iDefaultBufferSize;
    if (iBufferSize==0) iBufferSize=1024;
    int offset=0;
    // this is just to keep the old code for reading of the childs
    // stdout channel. The loop was originally in spawnShellProgram
#ifdef OLD_PIPE_READ
    int bOldRead=0;
    if (bOldRead) {
      TstreamBuffer anchor;
      memset(&anchor, 0, sizeof(TstreamBuffer));
      // the readStream method is from the 'old' days of the CE when it was written in
      // plain C. Now the issue handlers use vectors to return the result data, so we
      // can do the same for the readout of the stdout and stderr channels of the 
      // child process ... if we find time.
      // Anyway, this is not time critical as it is just a debug feature of the CE.
      int resSize=readStream(fp, 0, &anchor);
      if (resSize>0) {
	// calculate the number of words, align to element size of the result buffer 
	resSize=(resSize-1)/sizeof(CEResultBuffer::value_type)+1;
	pRB->resize(resSize,0);
	int element=0;
	int offset=0;
	TstreamBuffer* iter=&anchor;
	while (iter && iResult>=0) {
	  if (iter->pBuffer && iter->iSize>0) {
	    if (offset+iter->iSize<=(resSize-element)*sizeof(CEResultBuffer::value_type)) {
	      memcpy(((char*)&(*pRB)[element])+offset, iter->pBuffer, iter->iSize);
	      element+=(offset+iter->iSize)/sizeof(CEResultBuffer::value_type);
	      offset=(offset+iter->iSize)%sizeof(CEResultBuffer::value_type);
	    } else {
	      CE_Error("stream buffer list mismatch, stream too big\n");
	      iResult=-EFAULT;
	    }
	  }
	  iter=iter->pNext;
	}
	if (iResult<0) element=0;
	else if (offset!=0) element++;
	if (element!=pRB->size()) pRB->resize(element);
      }

      cleanStreamBuffers(&anchor);
      if (iResult>=0)
	iResult=resSize;
    } else
#endif //OLD_PIPE_READ
      do {
	CE_Debug("resize result buffer to %d\n", offset+iBufferSize);
	pRB->resize(offset+iBufferSize, 0);
	int iRead=fread(&(*pRB)[offset], sizeof(CEResultBuffer::value_type), iBufferSize, fp);
	if (iRead<0) {
	  CE_Error("internal error, can not read from stream\n");
	  iResult=-EBADF;
	} else {
	  offset+=iRead;
	  if (iRead<iBufferSize) {
	    CE_Debug("%d bytes read from childs stdout channel, resize result buffer to %d\n", iRead*sizeof(CEResultBuffer::value_type), offset);
	    pRB->resize(offset);      
	  }
	}
      } while (iResult>=0 && !feof(fp));
    if (iResult>=0) iResult=pRB->size()*sizeof(CEResultBuffer::value_type);
  } else {
    CE_Error("invalid pointer\n");
    iResult=-EINVAL;
  }
  return iResult;
}

/* write a data buffer to a file
 * @param pFileName   name of the file
 * @param pData       data buffer
 * @param iDataSize   size of data buffer in byte
 * @ingroup CE_shex
 */
int writeBufferToFile(const char* pFileName, const char* pData, int iDataSize)
{
  int iResult=0;
  if (pFileName && pData) {
    FILE* fp=NULL;
    // the 'b' is meaningless in POSIX type systems, but one never knows if this code will be used
    // on other architectures, once I spent a while to find a bug of this type on a w... system
    fp=fopen(pFileName, "wb"); 
    if (fp) {
      int iWritten=fwrite(pData, 1, iDataSize, fp);
      if (iWritten!=iDataSize) {
	CE_Error("error writing file %s: %d byte(s) of %d written\n", pFileName, iWritten, iDataSize);
	iResult=-EIO;
      }
      fclose(fp);
      fp=NULL;
    } else {
      iResult=-EBADF;
      CE_Error("can not open file %s\n", pFileName);
    }
  } else {
    CE_Error("invalid pointer\n");
    iResult=-EINVAL;
  }
  return iResult;
}

/* spawn a child process and run a program as the child
 * @param  argv           program name and arguments 
 * @param  bNonBlocking   return immediately, not yet implemented
 * @param  pRB            pointer to issue result buffer
 * @param  iVerbosity     0: don't read the stderr channel of the child and neither print info messages
 * @ingroup CE_shex
 */
int spawnShellProgram(char const **argv, int bNonBlocking, CEResultBuffer* pRB, int iVerbosity)
{
  int iResult=0;
  if (argv) {
/*     const char** pArg=(const char**)argv; */
/*     while (pArg && *pArg) { */
/*       CE_Debug("argument: %s\n", *pArg); */
/*       pArg++; */
/*     } */
    pid_t child_pid=0;
    int result_pipe[2]; // childs stdout channel -> parents result buffer
    int error_pipe[2];  // childs stderr channel -> parents CE logging
    int abnorm_pipe[2]; // childs exit value in case of execution error 
    iResult=pipe(result_pipe);
    if (iResult<0) {
      CE_Error("can not create pipe: error %d\n", iResult);
      return iResult;
    }
    iResult=pipe(error_pipe);
    if (iResult<0)  {
      CE_Error("can not create pipe: error %d\n", iResult);
      return iResult;
    }
    iResult=pipe(abnorm_pipe);
    if (iResult<0)  {
      CE_Error("can not create pipe: error %d\n", iResult);
      return iResult;
    }
    child_pid=fork();
    if (child_pid==0) {
      // child process
      close(result_pipe[0]); // close read end of the pipe
      close(error_pipe[0]); // close read end of the pipe
      close(abnorm_pipe[0]); // close read end of the pipe
      dup2(result_pipe[1], STDOUT_FILENO);
      dup2(error_pipe[1], STDERR_FILENO);
      // write a value to the pipe in order to avoid blocking
      // when parent reads from the pipe
      FILE* abnormStream=fdopen(abnorm_pipe[1], "w");
      fwrite(&iResult, sizeof(int), 1, abnormStream);
      fclose(abnormStream);
      execvp(argv[0], (char* const*)argv);
      // here we should never get if the exec was succesfull
      fprintf(stderr, "error executing child process %s\n", argv[0]);
      iResult=-EIO;
      abnormStream=fdopen(abnorm_pipe[1], "w");
      fwrite(&iResult, sizeof(int), 1, abnormStream);
      fclose(abnormStream);
      exit(iResult);
    } else {
      // parent process
      close(result_pipe[1]); // close write end of the pipe
      close(error_pipe[1]); // close write end of the pipe
      close(abnorm_pipe[1]); // close write end of the pipe
      FILE* resStream=fdopen(result_pipe[0], "r");
      FILE* errStream=fdopen(error_pipe[0], "r");
      // check the childs system status, but the child returns properly
      // even if the execution failed. So the direct pipe for the 
      // announcement of the failure was created
      int child_status=0;
      wait(&child_status);
      // check the direct pipe for the return value
      FILE* abnormStream=fdopen(abnorm_pipe[0], "r");
      if (abnormStream) {
	fread(&iResult, sizeof(int), 1, abnormStream);
      } else {
	CE_Error("severe internal system error, can not get file descriptor\n");
      }
      fclose(abnormStream);
      if(iResult>=0 && WIFEXITED(child_status)) {
	char retCode=WEXITSTATUS(child_status);
	if (iVerbosity>0) CE_Info("program %s finished with exit code %d\n", argv[0], retCode);
	if (pRB) {
	  // read the stdout of the child
	  iResult=readStreamIntoVector(resStream, 1024, pRB);
	}
      } else {
	CE_Error("program %s exited abnormally\n", argv[0]);
      }
	if (iVerbosity>0) {
	  // read the stderr of the child
#ifdef OLD_PIPE_READ
	  TstreamBuffer anchor;
	  memset(&anchor, 0, sizeof(TstreamBuffer));
	  int errSize=readStream(errStream, 0, &anchor);
	  if (errSize>0) {
	    flushStreamBuffer(&anchor, NULL, 1, NULL);
	  }
	  cleanStreamBuffers(&anchor);
#else // OLD_PIPE_READ
	  CEResultBuffer errChannel;
	  if (readStreamIntoVector(errStream, 1024, &errChannel)>0) {
	    flushStreamBuffer(&errChannel, NULL, 1, NULL);
	  }
#endif //OLD_PIPE_READ
	}
	if (resStream) fclose(resStream);
	if (errStream) fclose(errStream);
    }
  } else {
    CE_Error("invalid pointer\n");
    iResult=-EINVAL;
  }
  //CE_Debug("result=%d\n", iResult);
  return iResult;
}

/* run a shell program
 * @param  pHeadingArg  placed as the first element of the argument array if not NULL 
 * @param  pCmdString   zero terminated string with command and arguments
 * @param  pTrailingArg placed as the last element of the argument array if not NULL 
 * @param  pRB          pointer to issue result buffer
 * @param  iVerbosity   0: don't read the stderr channel of the child and neither print info messages
 * @ingroup CE_shex
 */
int callShellProgram(const char* pHeadingArg, const char* pCmdString, int iStringSize, 
		     const char** arrayTrailingArg, int iNofTrailingArgs, CEResultBuffer* pRB, 
		     int iVerbosity)
{
  int iResult=0;
  int argc=0;
  int iLen=0;
  char* pBuffer=NULL;
  if (pCmdString) while (iLen<iStringSize && pCmdString[iLen]!=0 && pCmdString[iLen]!='\n') iLen++;
  if (iLen>0) {
    int i=0;
    int bQuote=0;
    int bDblQuote=0;
    int bSlash=0;
    int iLastBlank=0;
    pBuffer=(char*)malloc(iLen+1);
    if (pBuffer) {
      // make a working copy of the string 
      memset(pBuffer, 0, iLen+1);
      strncpy(pBuffer, pCmdString, iLen);
      // scan the string and replace blanks to isolate arguments
      for (i=0; i<=iLen; i++) {
	switch (pBuffer[i]) {
	case ' ':
	  if (!bQuote && !bDblQuote) {
	    pBuffer[i]=0;
	    if (i-iLastBlank>1) argc++;
	    iLastBlank=i;
	  }
	  break;
	case '\'':
	  if (!bSlash) {
	    if (bQuote) {
	      bQuote=0;
	      if (i-iLastBlank>1) argc++;
	    } else {
	      bQuote=1;
	    }
	    pBuffer[i]=0;
	    iLastBlank=i;
	  }
	  break;
	case '\"':
	  if (!bSlash) {
	    if (bDblQuote) {
	      bDblQuote=0;
	      if (i-iLastBlank>1) argc++;
	    } else {
	      bDblQuote=1;
	    }
	    pBuffer[i]=0;
	    iLastBlank=i;
	  }
	  break;
	}
	if (pBuffer[i]=='\\' && !bSlash) bSlash=1;
	bSlash=0;
      }
      if (i-iLastBlank>1) argc++;
    } else {
      CE_Error("no memory available, failed to allocate %d bytes\n", iLen+1);
      iResult=-ENOMEM;
    }
  }

  if (iResult>=0) {
    if (argc>0) {
      //CE_Debug("found %d arguments in \'%s\'\n", argc, pCmdString);
    }
    if (pHeadingArg) argc++; // space for the first argument
    if (arrayTrailingArg && iNofTrailingArgs>0) argc+=iNofTrailingArgs; // space for the last argument
    if (argc>0) {
      argc++; // one additional NULL element to terminate the arguments
      const char** argv=(const char**)malloc(argc*sizeof(char*));
      if (argv) {
	memset(argv, 0, argc*sizeof(char*));
	int j=0;
	int i=0;
	if (pHeadingArg) argv[j++]=pHeadingArg;
	char* pNewArg=NULL;
	for (i=0;iLen>0 && i<=iLen && pBuffer!=NULL && j<argc-1; i++) {
	  //CE_Debug("char # %d %c\n", i, pBuffer[i]);
	  if (pBuffer[i]!=0 && pNewArg==NULL) {
	    //CE_Debug("start argument %d: %s\n", j, &pBuffer[i]);
	    pNewArg=&pBuffer[i];
	  }else if (pBuffer[i]==0 && pNewArg!=NULL) {
	    //CE_Debug("set argument %d: %s\n", j, pNewArg);
	    argv[j++]=pNewArg;
	    pNewArg=NULL;
	  }
	}
	if (arrayTrailingArg) 
	  for (i=0; i<iNofTrailingArgs && j<argc-1; i++) argv[j++]=arrayTrailingArg[i]; 
	iResult=spawnShellProgram(argv, 0, pRB, iVerbosity);
	free(argv);
	argv=NULL;
      } else {
	CE_Error("no memory available, failed to allocate %d bytes\n", argc*sizeof(char*));
	iResult=-ENOMEM;
      }
    } else {
      CE_Warning("empty command\n");
    }
    if (pBuffer!=NULL) free(pBuffer);
    pBuffer=NULL;
  }
  return iResult;
}

/**
 * @class ShellCommandHandler
 * CEIssueHandler for the Shell command group.
 * <b>Note:</b> none of shell execution commands can appear in a command
 * sequence since they take all the payload as the script/programm and
 * don't care about end-markers.
 *
 * @ingroup rcu_ce_base_services
 */
class ShellCommandHandler : public CEIssueHandler
{
 public:
  /** constructor */
  ShellCommandHandler() {}

  /** destructor */
  ~ShellCommandHandler(){}

  /**
   * Get group id.
   */
  int GetGroupId() {return FEESVR_CMD_SHELL;}

  /**
   * Get the expected size of the payload for a command.
   * @param cmd       command id stripped py the parameter
   * @param parameter the parameter (16 lsb of the command header)
   * @return number of expected 32 bit words
   */
  int GetPayloadSize(__u32 cmd, __u32 parameter);

  /**
   * Issue handler for shell command group.
   * Main switch for the @ref FEESVR_CMD_SHELL shell command execution group.
   * <b>Note:</b> none of shell execution commands can appear in a command
   * sequence since they take all the payload as the script/programm and
   * don't care about end-markers.
   * @see CEIssueHandler::issue for parameters and return values
   */
  int issue(__u32 cmd, __u32 parameter, const char* pData, int iDataSize, CEResultBuffer& rb);
};

int ShellCommandHandler::GetPayloadSize(__u32 cmd, __u32 parameter)
{
  return parameter;
}

int ShellCommandHandler::issue(__u32 cmd, __u32 parameter, 
			  const char* pData, int iDataSize, 
			  CEResultBuffer& rb) {
  int iResult=0;
  CE_Debug("translateShellCommand cmd=%#x parameter=%#x\n", cmd, parameter);
#ifndef ENABLE_MASTERMODE
  if (cmd!=FEESRV_RCUSH_SCRIPT) {
    CE_Error("this version of the FeeServer is not authorized to execute shell programs, skip ...\n");
    iResult=-ENOSYS;
  }
#endif
  switch (cmd) {
  case FEESRV_EXECUTE_PGM:
# ifdef ENABLE_MASTERMODE
    iResult=callShellProgram(NULL, pData, iDataSize, NULL, 0, &rb, 1);
# endif //ENABLE_MASTERMODE
    break;
  case FEESRV_RCUSH_SCRIPT:
  case FEESRV_EXECUTE_SCRIPT:
  case FEESRV_BINARY_PGM:
      if (parameter==0 || parameter<iDataSize) {
	char tmpFileName[20];
	sprintf(tmpFileName, "/tmp/.fsvexec");
	iResult=writeBufferToFile(tmpFileName, pData+parameter, iDataSize-parameter);
	if (iResult>=0) {
	  switch (cmd) {
	  case FEESRV_RCUSH_SCRIPT:
	    {
	      const int iNofTrailingArgs=2;
	      const char* trailingArgs[iNofTrailingArgs];
	      trailingArgs[0]="b"; // batch command for rcu-sh
	      trailingArgs[1]=tmpFileName;
	      iResult=callShellProgram("rcu-sh", pData, parameter, trailingArgs, iNofTrailingArgs, &rb, 1);
	    }
	    break;
#         ifdef ENABLE_MASTERMODE
	  case FEESRV_EXECUTE_SCRIPT:
	    iResult=callShellProgram("sh", pData, parameter, (const char**)&tmpFileName, 1, &rb, 1);
	    break;
	  case FEESRV_BINARY_PGM:
	    if (callShellProgram("chmod", "a+x", 4, (const char**)&tmpFileName, 1, NULL, 0)>=0) {
	      iResult=callShellProgram(tmpFileName, pData, parameter, NULL, 0, &rb, 1);
	    } else {
	      CE_Error("can not change attributes for temporary program %s\n", tmpFileName);
	    }
	    break;
#         endif //ENABLE_MASTERMODE
	  }
	  // delete tmp file
	  {
	    const int iNofTrailingArgs=1;
	    const char* trailingArgs[iNofTrailingArgs];
	    trailingArgs[0]=tmpFileName;
	    callShellProgram("rm", NULL, 0, trailingArgs, iNofTrailingArgs, NULL, 0); 
	  }
	} else {
	  CE_Error("can not write temporary script file %s\n", tmpFileName);
	}
      } else {
	CE_Error("invalid parameter, rcu-sh cmd line options exceed total size of buffer\n");
	iResult=-EINVAL;
      }
    break;
  default:
    CE_Warning("unrecognized command id (%#x)\n", cmd);
    iResult=-ENOSYS;
  }
  if (iResult>=0) {
    // the main switch needs the number of processed bytes in the incoming
    // command buffer. In contrast to other commands the length of the 
    // commands of the shell execution group is not given by the parameter.
    // This is a simple approach which holds since shell execution commands
    // are debugging commands and does not appear in command sequences.  
    iResult=iDataSize;
  }

  return iResult;
}

ShellCommandHandler g_ShellCommandHandler;

/*******************************************************************************************
 * @defgroup CE_logging  The ControlEngine logging methods
 * @note Definition: 
 * 
 */

#define LOG_BUFFER_SIZE 600
char g_LogBuffer[LOG_BUFFER_SIZE+1]="";
/** the global logging level
 * Note: the level is changed for the normal operation, right after initialization
 * See calls of @ref ceSetLogLevel
 */
int g_LogLevel=eCEInfo;
string g_Timestamp="";

int ceSetLogLevel(int level) {
  g_LogLevel=level;
  return g_LogLevel;
}

void ceSetTimestamp(const char* ts) 
{
  if (ts) g_Timestamp=ts;
}

int ceLogMessage(const char* file, int lineNo, int loglevel, const char* format, ...)
{
  int iResult=0;

  // check the log level
  if (g_LogLevel!=eCELogAll && g_LogLevel>loglevel) return iResult;
  if (format==NULL) return 0;

  int tgtLen=0;
  int iBufferSize=LOG_BUFFER_SIZE;
  char* tgtBuffer=g_LogBuffer;
  memset(tgtBuffer, 0, LOG_BUFFER_SIZE+1);
  int iFeeLogLevel=0;

  const char* strLevel="";
  switch (loglevel) {
  case eCEDebug:
    iFeeLogLevel=MSG_DEBUG;
    strLevel="Debug:  ";
    break;
  case eCEInfo:
    iFeeLogLevel=MSG_INFO;
    strLevel="Info:   ";
    break;
  case eCEWarning:
    iFeeLogLevel=MSG_WARNING;
    strLevel="Warning:";
    break;
  case eCEError:
    iFeeLogLevel=MSG_ERROR;
    strLevel="Error:  ";
    break;
  case eCEFatal:
    iFeeLogLevel=MSG_ALARM;
    strLevel="Fatal:  ";
    break;
  }

  // filtering of message repetitions 
  /*
  static int lastLineNo=0;
  static const char* lastFile=NULL;
  static int messageRep=0;
  static int lastFeeLogLevel=0;
  if (lastFile!=NULL && lastLineNo==lineNo && lastFile==file) {
    messageRep++;
    return 0;
  } else if (messageRep>0) {
    snprintf(g_LogBuffer, iBufferSize, "Last message (%s line %d) repeated %d time(s)\n", lastFile, lastLineNo, messageRep);
    messageRep=0;
    printf(g_LogBuffer);
    fflush(stdout);
    if (ceCheckOptionFlag(ENABLE_CE_LOGGING_CHANNEL))
      createLogMessage(lastFeeLogLevel,g_LogBuffer,"CE");
  }
  lastLineNo=lineNo;
  lastFile=file;
  lastFeeLogLevel=iFeeLogLevel;

  memset(tgtBuffer, 0, LOG_BUFFER_SIZE+1);
  */

  int len=0;
  const char* fmtLevel="CE %s ";
  const char* fmtOrigin="(%s line %d) ";

  // print the debug level of the message
  if (strLevel[0]!=0 && strlen(strLevel)+strlen(fmtLevel)+tgtLen<iBufferSize) {
    len=snprintf(tgtBuffer, iBufferSize-tgtLen, fmtLevel, strLevel);
    if (len<iBufferSize-tgtLen) {
      tgtBuffer+=len;
      tgtLen+=len;
    }
  }

  // print the origin
  if (file!=NULL && strlen(file)+strlen(fmtOrigin)+5+tgtLen<iBufferSize) {
    len=snprintf(tgtBuffer, iBufferSize-tgtLen, fmtOrigin, file, lineNo);
    if (len<iBufferSize-tgtLen) {
      tgtBuffer+=len;
      tgtLen+=len;
    }
  }

  va_list args;
  va_start(args, format);

  // print the message to stdout
  if (tgtLen>0) {
    if (g_Timestamp.length()>0) {
      // print level, origin and timestamp
      printf("%s%s            ", g_LogBuffer, g_Timestamp.c_str());
    } else {
      // print level and origin
      printf("%s", g_LogBuffer);
    }
    // print the message
      vprintf(format, args);
    fflush(stdout);
  }

  // the following lines handle the logging to the DIM channel
  if (ceCheckOptionFlag(ENABLE_CE_LOGGING_CHANNEL)) {
    if (iBufferSize>tgtLen) {
      len=vsnprintf(tgtBuffer, iBufferSize-tgtLen, format, args);
      if (len>0 && len<iBufferSize-tgtLen) {
	tgtBuffer+=len;
	tgtLen+=len;
      } else {
	fprintf(stderr, "message buffer to small for log message \"%s\", file %s line %d\n", format, file, lineNo);
      }
    } else {
      fprintf(stderr, "message buffer to small for log message \"%s\", file %s line %d\n", format, file, lineNo);
    }
  } else {
    fprintf(stderr, "can not create logging message, invalid argument\n");
  }

  if (tgtLen>0) {
    // Note: the log channel blocks the command thread if DIM was compiled without NOTHREADS flag
    createLogMessage(iFeeLogLevel,g_LogBuffer,"CE");
  }
  return iResult;
}
