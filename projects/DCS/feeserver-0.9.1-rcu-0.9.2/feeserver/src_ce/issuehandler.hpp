#include <sys/types.h>
#include <linux/types.h> // for the __u32 type
#include <vector>

#ifndef __ISSUEHANDLER_HPP
#define __ISSUEHANDLER_HPP

/**
 * The result buffer for the @ref issue handling.
 * @ingroup rcu_ce_base_issue
 */
typedef std::vector<__u32> CEResultBuffer; 


/**
 * @class CEIssueHandler
 * @brief helper class for the handling of the Command interpretation for the RCU FeeServer
 *
 * This class implements a framework for handling Command interpretation in the RCU FeeServer
 * Here we can register and unregister the different commands in a global list. The list of the 
 * commandos will be processed in the FeeServer
 *
 * @ingroup rcu_ce
 */

class CEIssueHandler {
  
public:
  
  /**
   * standard constructor. The object is automatically registered in the
   * global CEIssueHandler-list
   */
  CEIssueHandler();
  
  /**
   * standard destructor. The object is automatically unregistered from the global
   * CEIssueHandler-list
   */
  virtual ~CEIssueHandler();
  
  /**
   * Virtual method for returning the ID of the Command-group. 
   * This function will be overwritten by the different classes for the command-groups.
   * @return             GroupId, -1 if id not valid
   */
  virtual int GetGroupId()=0;

  /**
   * Enhanced selector method.
   * The function can be overwritten in case the group Id is not sufficient to select
   * a certain handler.
   * @return          1 if the handler can process the cmd with the parameter
   */
  virtual int CheckCommand(__u32 cmd, __u32 parameter);
  
  /**
   * Virtual method for handling commands in FeeServer
   * This function will be overwritten by the different classes for the command-groups.
   * @param cmd       complete command id (upper 16 bit of 4 byte header)
   * @param parameter command parameter extracted from the 4 byte header (lower 16 bit)
   * @param pData     pointer to the data following the header
   * @param iDataSize size of the data in bytes
   * @return          number of bytes of the payload buffer which have been processed
   *                  neg. error code if failed
   *                  - -ENOSYS  unknown command
   */
  virtual int issue(__u32 cmd, __u32 parameter, const char* pData, int iDataSize, CEResultBuffer& rb)=0;

  /**
   * Handler for high level commands.
   * The handler gets a single string containing the command. The general form of
   * a high level command is:
   * <pre>
   *    \<fee\> command \</fee\>
   * </pre>
   * The header and trailer keywords are cur out by the framework.
   * @param command   command string
   * @return >0 if processed, 0 if not
   *         - -EEXIST   command available, but target/device wrong
   *         - -ENOSYS   handler not implemented
   */
  virtual int HighLevelHandler(const char* pCommand, CEResultBuffer& rb);
  
  /**
   * Get the expected size of the payload for a command.
   * @param cmd       command id stripped py the parameter
   * @param parameter the parameter (16 lsb of the command header)
   * @return number of expected 32 bit words
   */
  virtual int GetPayloadSize(__u32 cmd, __u32 parameter)=0;
  
  /**
   * Get the first CEIssueHandler in the list
   * @return  pointer to first agent in the list, NULL if empty
   */
  static CEIssueHandler* getFirstIH();
  
  /**
   * Get the next CEIssueHandler in the list
   * @return  pointer to next agent in the list, NULL if end of list
   */
  static CEIssueHandler* getNextIH();
  
private:
  
  /**
   * Register CEIssueHandler in the global list.
   * 
   */
  static int RegisterIssuehandler(CEIssueHandler* pHandler);
  
  /**
   * Unregister agent in the global list.
   * 
   */
  static int UnregisterIssuehandler(CEIssueHandler* pHandler);
  
  /** the current object link (list position) */
  static CEIssueHandler* fCurrent;
  
  /** the link to the anchor object (list position*/
  static CEIssueHandler* fAnchor;
  
  /** the link to the next object in list*/
  CEIssueHandler* fpNext;
  
  /** number of objects in list*/ 
  static int fCount;
  
};
#endif //ISSUEHANDLER

