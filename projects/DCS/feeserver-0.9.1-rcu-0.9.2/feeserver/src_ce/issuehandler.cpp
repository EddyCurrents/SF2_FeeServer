#include "issuehandler.hpp"
#include <cerrno>
//#include "device.hpp"
#include <iostream>


using namespace std;
        
        
CEIssueHandler::CEIssueHandler()
  :
  fpNext(NULL)
{
  RegisterIssuehandler(this);
}

CEIssueHandler::~CEIssueHandler(){
  UnregisterIssuehandler(this);
}

CEIssueHandler* CEIssueHandler::fCurrent=NULL;
CEIssueHandler* CEIssueHandler::fAnchor=NULL;
int CEIssueHandler::fCount=0;

int CEIssueHandler::RegisterIssuehandler(CEIssueHandler *ph){
  if (fAnchor==NULL) {
    fAnchor=ph;
  } else {
    ph->fpNext=fAnchor;
    fAnchor=ph;
  }
  fCount++;
  return 0;	
}

int CEIssueHandler::UnregisterIssuehandler(CEIssueHandler *ph){
  fCurrent=NULL;
  CEIssueHandler* prev=NULL;
  CEIssueHandler* handler=fAnchor;
  while (handler!=NULL && handler!=ph) {
    prev=handler;
    handler=handler->fpNext;
  }
  if (handler) {
    if (prev==NULL) {
      fAnchor=handler->fpNext;
    } else {
      prev->fpNext=handler->fpNext;
    }
    fCount--;
  }
  return 0;
}

CEIssueHandler* CEIssueHandler::getFirstIH(){
  
  if(fAnchor==0){
    return 0;
  }
  fCurrent=fAnchor;
  return fAnchor;
}

CEIssueHandler* CEIssueHandler::getNextIH(){
  if (fCurrent!=NULL) fCurrent=fCurrent->fpNext;
  return fCurrent;
}

int CEIssueHandler::CheckCommand(__u32 cmd, __u32 parameter)
{
  return 0;
}

int CEIssueHandler::HighLevelHandler(const char* pCommand, CEResultBuffer& rb)
{
  return -ENOSYS;
}
