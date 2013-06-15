// $Id: dev_msgbuffer.hpp,v 1.4 2007/09/01 13:00:05 richter Exp $

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

#ifndef __DEV_MSGBUFFER_HPP
#define __DEV_MSGBUFFER_HPP

#include <vector>
#include <time.h>
#include <pthread.h>
#include <memory>
#include "issuehandler.hpp"
#include "dimdevice.hpp"
#include "lockguard.hpp"
#include "ce_base.h"

// forward declarations
class MsgBufferStateMapper;

/**
 * @class DCSCMsgBuffer
 * Implementation of the DCS board message buffer interface.
 * This class encapsulates the @ref dcsc_msg_buffer_access interface. The
 * MsgBuffer interface is a memory mapped interface between the DCS board and
 * originally the RCU. Meanwhile, a few other device use the same aproach
 * (driver and tools).<br>
 * The interface can be used to access the memory space inside the FPGA (see
 * @ref DCSCMsgBuffer_memory_access), the Flash memory on the RCU (see @ref
 * DCSCMsgBuffer_flash_access), and the logic of the FPGA via the SelectMap
 * interface (Xilinx specific, see @ref DCSCMsgBuffer_selectmap_access). The
 * current mode is represented by 3 states:
 *
 * @section DCSCMsgBuffer_states States
 * <b>MSGBUFFER (16)</b>
 *  - normal mode of the device
 *  - access to the (RCU) memory space
 * <br>
 *
 * <b>FLASH (256)</b>
 *  - access to the flash memory
 * <br>
 *
 * <b>SELECTMAP (272)</b>
 *  - acces to the FPGA via the SelectMap interface
 * <br>
 *
 * \b Note: there are no transitions externally available. All transitions
 * are handled internally The states are exported for information. 
 *
 * @section DCSCMsgBuffer_usage Usage
 * It is not allowed the create and delete objects of this class, instead,
 * one global instance is handled and a pointer to the instance can be obtained
 * by @ref GetInstance(). The instance must be released by calling @ref
 * ReleaseInstance().
 *
 * A child class DCSCMsgBufferSim exists to simulate the memory access and to
 * support development of the code without presence of hardware. The simulation
 * is activated with the <tt> --enable-rcudummy </tt> switch during configuration
 * (see @ref readme).
 *
 * \b IMPORTANT: The instance returned by @ref GetInstance() must not be added
 * as a sub-device of another device.
 *
 * @section DCSCMsgBuffer_memory_access Memory access
 * - @ref SingleWrite
 * - @ref MultipleWrite
 * - @ref SingleRead
 * - @ref MultipleRead
 *
 * @section DCSCMsgBuffer_flash_access Flash access
 * soon to be implemented
 *
 * @section DCSCMsgBuffer_selectmap_access Selectmap access
 * soon to be implemented
 *
 * @section DCSCMsgBuffer_commands Commands
 * Implements a CEIssueHandler to handle the commands concerning the message
 * buffer access library.
 * See @ref FEESVR_DCSC_CMD command group.
 *
 * The following high-level commands are implemented:
 * - DCSC_SET_OPTION_FLAG 0x<hex>
 * - DCSC_CLEAR_OPTION_FLAG 0x<hex>
 */
class DCSCMsgBuffer : public CEDimDevice, public CEIssueHandler {
protected:
  /** constructor prohibited (but available for childs) */
  DCSCMsgBuffer();
  /** destructor prohibited for external use */
  ~DCSCMsgBuffer();
private:
  /** copy constructor prohibited */
  DCSCMsgBuffer(DCSCMsgBuffer&);
  /** copy operator prohibited */
  DCSCMsgBuffer& operator=(DCSCMsgBuffer&);

public:
  /**
   * Return the result of the global initialization.
   */
  int CheckResult();

  /**
   * Get an instance.
   * This function creates and/or returns the global instance of the interface.
   * It decides whether to create the simulation class or the normal interface.
   */
  static DCSCMsgBuffer* GetInstance();

  /**
   * Release an instance.
   */
  static int ReleaseInstance(DCSCMsgBuffer* instance);

  /**
   * Write a single location (32bit word).
   * Wrapper to dcscMsgBuffer method.
   * Overloaded by the simulation class.
   * @param address   16 bit address in memory space
   * @param data      data word
   * @return
   */
  virtual int SingleWrite(__u32 address, __u32 data);

  /**
   * Read a single location (32bit word).
   * Wrapper to dcscMsgBuffer method.
   * Overloaded by the simulation class.
   * @param address   16 bit address in memory space
   * @param pData     buffer to receive the data
   * @return
   */
  virtual int SingleRead(__u32 address, __u32* pData);

  /**
   * Write a number of 32bit words beginning at a location.
   * The function takes care for the size of the MIB and splits the operation if
   * the amount of data to write exceeds the MIB size.
   * The function expects data in little endian byte order.
   * Wrapper to dcscMsgBuffer method.
   * Overloaded by the simulation class.
   * @param address   16 bit address in memory space
   * @param pData     buffer containing the data
   * @param iSize     number of words to write
   * @param iDataSize size of one word in bytes, allowed 1 (8 bit), 2 (16 bit),3 (compressed 10 bit) 
   *                  or 4 (32 bit) -2 and -4 denote 16/32 bit words which get swapped
   * @return
   */
  virtual int MultipleWrite(__u32 address, __u32* pData, int iSize, int iDataSize);

  /**
   * Read a number of 32bit words beginning at a location.
   * Wrapper to dcscMsgBuffer method.
   * Overloaded by the simulation class.
   * @param address   16 bit address in memory space
   * @param iSize     number of words to read
   * @param pData     buffer to receive the data, the function expect it to be of suitable size
   *                  (i.e. iSize x wordsize)
   * @return          number of 32bit words which have been read, neg. error code if failed
   */
  virtual int MultipleRead(__u32 address, int iSize,__u32* pData);

protected:
  /**
   * Internal function called during the @ref CEStateMachine::Armor procedure.
   * See @ref CEDevice::ArmorDevice()
   */
  int ArmorDevice();

  /**
   * Evaluate the state of the device.
   * See @ref CEDevice::EvaluateHardware()
   * @return             state
   */
  CEState EvaluateHardware();

  /**
   * Get the name of a default transition.
   * See @ref CEStateMachine::GetDefaultTransitionName.
   * @param transition   the id of the transition to check
   * @return             pointer to name
   */
  const char* GetDefaultTransitionName(CETransition *pTransition);

  /**
   * Check if the transition is allowed for the current state.
   * See @ref CEStateMachine::IsAllowedTransition.
   * @param transition   pointer to transition descriptor
   * @return             1 if yes, 0 if not, neg. error code if failed
   */
  int IsAllowedTransition(CETransition *pT);

  enum {
    eActivateFlash = eLastDefaultTransition,
    eActivateSelectmap
  };

private:
  /**
   * Get group id.
   */
  int GetGroupId();

  /**
   * Get the expected size of the payload for a command.
   * @param cmd       command id stripped py the parameter
   * @param parameter the parameter (16 lsb of the command header)
   * @return number of expected 32 bit words
   */
  int GetPayloadSize(__u32 cmd, __u32 parameter) {return 0;}

  /**
   * Issue handler for DCSC MsgBuffer interface commands.
   * @param cmd       complete command id (upper 16 bit of 4 byte header)
   * @param parameter command parameter extracted from the 4 byte header (lower 16 bit)
   * @param pData     pointer to the data following the header
   * @param iDataSize size of the data in bytes
   * @return          number of bytes of the payload buffer which have been processed
   *                  neg. error code if failed
   *                  -ENOSYS  unknown command
   */
  int issue(__u32 cmd, __u32 parameter, const char* pData, int iDataSize, CEResultBuffer& rb);

  /**
   * Handler for DCSC MsgBuffer interface high level commands.
   * @see CEIssueHandler::HighLevelHandler.
   * @param command   command string
   * @return 1 if processed, 0 if not, neg. error code if failled 
   */
  int HighLevelHandler(const char* pCommand, CEResultBuffer& rb);

  /******************************************************************************
   ********************                                     *********************
   ********************          internal functions         *********************
   ********************                                     *********************
   *****************************************************************************/

  /**
   * Check whether the driver is accessible.
   * @return neg. error code if failed. -EACCES if not accessible
   */
  virtual int CheckDriver();

  /**
   * Release the dcscMsgBuffer interface.
   * Helper method to clean-up from constructor in case of error or destructor
   */
  int Release();


  /** mutex for thread protection */
  //CE_Mutex fMutex;

  /** the one and only global instance */
  static DCSCMsgBuffer* fpInstance;

  /** instance counter */
  static int fInstances;

  /** global result of the initialization */
  static int fResult;

  /** instance of the state mapper */
  std::auto_ptr<MsgBufferStateMapper> fpStateMapper;
};

class MsgBufferStateMapper: public CEStateMapper {
public:
  const char* GetMappedStateName(CEState state);
};

#ifdef RCUDUMMY
/**
 * @class DCSCMsgBufferSim
 * Simulated MSGBUFFER device.
 * The simulated device allocates memory space to simulate the memory
 * and overloads the corresponding access functions. 
 */
class DCSCMsgBufferSim : public DCSCMsgBuffer {
  friend class DCSCMsgBuffer;
private:
  /** constructor prohibited */
  DCSCMsgBufferSim();
  /** copy constructor prohibited */
  DCSCMsgBufferSim(DCSCMsgBufferSim&);
  /** copy operator prohibited */
  DCSCMsgBufferSim& operator=(DCSCMsgBufferSim&);
public:
  /** destructor */
  ~DCSCMsgBufferSim();

  /**
   * Write a single location (32bit word).
   * @param address   16 bit address in memory space
   * @param data      data word
   * @return
   */
  int SingleWrite(__u32 address, __u32 data);

  /**
   * Read a single location (32bit word).
   * @param address   16 bit address in memory space
   * @param pData     buffer to receive the data
   * @return
   */
  int SingleRead(__u32 address, __u32* pData);

  /**
   * Write a number of 32bit words beginning at a location.
   * The function takes care for the size of the MIB and splits the operation if
   * the amount of data to write exceeds the MIB size.
   * The function expects data in little endian byte order.
   * @param address   16 bit address in memory space
   * @param pData     buffer containing the data
   * @param iSize     number of words to write
   * @param iDataSize size of one word in bytes, allowed 1 (8 bit), 2 (16 bit),3 (compressed 10 bit) 
   *                  or 4 (32 bit) -2 and -4 denote 16/32 bit words which get swapped
   * @return
   */
  int MultipleWrite(__u32 address, __u32* pData, int iSize, int iDataSize);

  /**
   * Read a number of 32bit words beginning at a location.
   * @param address   16 bit address in memory space
   * @param iSize     number of words to write
   * @param pData     buffer to receive the data, the function expect it to be of suitable size
   *                  (i.e. iSize x wordsize)
   * @return          number of 32bit words which have been read, neg. error code if failed
   */
  int MultipleRead(__u32 address, int iSize,__u32* pData);

private:
  /**
   * Overload and always succeed.
   * @return neg. error code if failed. -EACCES if not accessible
   */
  int CheckDriver() {return 0;}

  /** size of the memory space */
  static int fMemSize;

  /** the memory space */
  std::vector<__u32> fMemory;
};
#endif //RCUDUMMY

#endif //__DEV_RCU_HPP
