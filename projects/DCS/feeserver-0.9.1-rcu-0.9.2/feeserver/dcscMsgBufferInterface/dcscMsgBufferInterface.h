// $Id: dcscMsgBufferInterface.h,v 1.22 2006/05/26 12:06:08 richter Exp $

/************************************************************************
**
**
** This file is property of and copyright by the Experimental Nuclear 
** Physics Group, Dep. of Physics and Technology
** University of Bergen, Norway, 2004
** This file has been written by Matthias Richter,
** Matthias.Richter@ift.uib.no
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

#ifndef __DCSC_RCU_ACCESS_H
#define __DCSC_RCU_ACCESS_H

#ifdef __cplusplus
extern "C" {
#endif
/** @file   dcscMsgBufferInterface.h
    @author Matthias Richter
    @date   
    @brief  The Message Buffer Interface. 
    This file implements the @ref dcsc_msg_buffer_access.
*/

/**
 * @defgroup dcsc_msg_buffer_access Message Buffer Encoding
 * This module implements the basic access function to DCS board 
 * @ref message_buffer_interface via the @ref rcubus_driver. The interface consists 
 * of three buffers: the message in buffer (MIB), the message out or result buffer 
 * (MRB) and the register buffer (REG). The driver treats this buffers as  ordinary
 * memory in the address space of the arm linux. It exports the three buffers in a 
 * row,  no matter where they are physically located in the address space. The size
 * of each of the three buffers can be requested by the ioctl function (see below). 
 * <br>
 * <pre>
 * format of the driver access space:
 * ==================================
 *      address        buffer
 * 0                 : MIB
 * size of MIB       : MRB
 * size of MIB + MRB : REG
 * </pre>
 * The  dcscMsgBufferInterface interface query each buffer size at initialization
 * and adapts its access to that values. The size is always in bytes, nevertheless
 * the MIB and MRB are treated as 32 bit memory and the REG as 8 bit memory.
 * (Note: there were some problems with register access concerning this 32/8 bit 
 * question, but this affects only the registers other than 0).<br>
 * 
 * By now only the first register is used:
 * - bit 7: execute flag
 * - bit 6: multiplexer switch for the MIB re-read
 * - bit 0: command ready (foreseen, but not yet implemented)
 * 
 * refer to the dcscMsgBufferInterface.h file for the defines. <br>
 * 
 * 
 * <b>General access sequence</b><br>
 * <!------------------------------->
 * To access memory location inside the RCU the following process is invoked:<br>
 * 1. a sequence of command blocks is written to the MIB<br>
 * 2. the 'execute' bit in the control register is set<br>
 * 3. the dcs message buffer interface interprets and executes the command sequence<br>
 * 4. the result (containing status word and data) is placed in the MRB<br>
 * 5. the 'execute' bit is cleared<br>
 * 6. result is interpreted<br>
 * - step 1,2 and 6 is done by the dcscMsgBufferInterface interface and the dcsdriver
 * - step 3-5 is done by the arm processor firmware
 */

/**
 * @name Format of the command sequence (32 bit words).
 * @brief
 * The general format consists of a <i>header word</i>, the payload, a <i>block marker</i>
 * and an <i>end marker</i>. The command can contain more than one block, each of them
 * terminated by the <i>block marker</i>.
 * <!--
 * 1. : information word
 * 2. : data word #0
 * n+1: data word #n
 * n+2: block marker (bit 16-31: AA55) and check sum (not yet implemented)
 *       :
 * x  : end marker (bit 16-31: DD3)
 * -->
 * <img src="pic_MsgBufferFormat.gif" border="0" alt="">
 * Words 1 (information word) to n+2 (block marker) are considered to be programming blocks. A sequence 
 * of an arbitrary number of programming blocks is terminated by the end marker word. This is only 
 * limited by the size of the MIB. By now the dcscMsgBufferInterface interface uses only one programming block 
 * per sequence.
 *
 * <b>Header Word Version 2:</b> This is the current format
 * - Bit 0-5  : command id
 * - Bit 6-15 : number of words(excluding the information and marker words)
 * - Bit 16-23: block number
 * - Bit 24-25: data format
 * - Bit 26   : flash mode if set
 * 
 * The firmware has an inbuilt check for Bit 28-31, only 0x0 or 0x8 to 0xf are allowed 
 * for the 4 MSBs. Check the section 'Message Buffer Header Format Version' below.
 *
 * <b>Header Word Version 1:</b> this format is specified for backward compatibility only
 * - Bit 0 - 15: number of words in the programming block including both information and marker word
 * - Bit 28- 31: command id
 *
 * <b>Command IDs used by the interface:</b>
 * @ingroup dcsc_msg_buffer_access
 */

/**
 * Code for the 'single read' command.
 * payload:<br>
 * - 32 bit address word
 * @ingroup dcscMsgBufferAccess
 */
#define SINGLE_READ	0x1
/**
 * Code for the 'single write' command.
 * payload:<br>
 * - 32 bit address word
 * - 32 bit data word
 * @ingroup dcscMsgBufferAccess
 */
#define SINGLE_WRITE	0x2
/**
 * Code for the 'multiple read' command.
 * payload:<br>
 * - 32 bit address word
 * - 32 bit number of words
 * @ingroup dcscMsgBufferAccess
 */
#define MULTI_READ	0x3
/**
 * Code for the 'multiple write' command.
 * payload:<br>
 * - 32 bit address word
 * - 32 bit number of words
 * - 32 bit data words
 * @ingroup dcscMsgBufferAccess
 */
#define MULTI_WRITE	0x4
/**
 * Code for the 'random read' command.
 * payload:<br>
 * @ingroup dcscMsgBufferAccess
 */
#define RANDOM_READ	0x5 // not yet implemented in dcscMsgBufferInterface but provided by the firmware
/**
 * Code for the 'random write' command.
 * payload:<br>
 * @ingroup dcscMsgBufferAccess
 */
#define RANDOM_WRITE	0x6 // not yet implemented in dcscMsgBufferInterface but provided by the firmware
/**
 * Erase the flash completely.
 * The command is specific to the rcu flash access, available since
 * version 2.2. of the DCSboard firmware.
 * payload:<br>
 * no data words
 * @ingroup dcscMsgBufferAccess
 */
#define FLASH_ERASEALL    0x21
/**
 * Erase one sector of the flash
 * The command is specific to the rcu flash access, available since
 * version 2.2. of the DCSboard firmware.
 * payload:<br>
 * 1 data word: sector address 
 * @ingroup dcscMsgBufferAccess
 */
#define FLASH_ERASE_SEC   0x22
/**
 * Erase multiple sectors of the flash.
 * The command is specific to the rcu flash access, available since
 * version 2.2. of the DCSboard firmware.
 * payload:<br>
 * 2 data words: sector address and count
 * @ingroup dcscMsgBufferAccess
 */
#define FLASH_MULTI_ERASE 0x24
/**
 * Read the ID of the flash.
 * The command is specific to the rcu flash access, available since
 * version 2.2. of the DCSboard firmware.
 * payload:<br>
 * 1 data word: 0 manufacturer id, 1 device id
 * @ingroup dcscMsgBufferAccess
 */
#define FLASH_READID      0x28 
/**
 * Code for the flash reset command.
 * The command is specific to the rcu flash access, available since
 * version 2.2. of the DCSboard firmware.
 * payload:<br>
 * no data words
 * @ingroup dcscMsgBufferAccess
 */
#define FLASH_RESET       0x30

/**
 * @name Message Buffer Header Format Version.
 * @brief Version handling has been introduced in firmware version 2.
 * <pre>
 * Bit 28-31 of message buffer header
 * 0x0     : not used
 * 0x1-0x7 : version 1 (bit 31==0, other bits arbitrary)
 * 0xa     : version 2
 * 0xf     : Feeserver CE command
 * </pre>
 * 
 * <b>Some helper defines</b> for the version decoding:
 * @ingroup dcsc_msg_buffer_access
 */

/**
 * Mask to extract Message Buffer Version 1 command ids. Since the commands didn't
 * use Bit 31, this bit is now used to indicate a Version 2 or higher format. 
 * @ingroup dcsc_msg_buffer_access
 */
#define MSGBUF_VERSION_1_MASK 0x70000000
/**
 * Message Buffer Version 2.0 id.
 * Version 2.0 represented the same functionality as version 1 but with a modified
 * header word format.
 * @ingroup dcsc_msg_buffer_access
 */
#define MSGBUF_VERSION_2      0xa0000000
/**
 * Message Buffer Version 2.2 id.
 * In version 2.2 to flash access commands had been introduced. 
 * @ingroup dcsc_msg_buffer_access
 */
#define MSGBUF_VERSION_2_2    0xb0000000
/**
 * FeeServer command id.
 * This bit pattern is not really used by the Message Buffer Interface, its just
 * reserved for the FeeServer.
 * @ingroup dcsc_msg_buffer_access
 */
#define FEESERVER_CMD         0xf0000000

/**
 * @name Format of the command result (32 bit words).
 * @brief The result given in the MRB has the format:
 * <pre>
 * ============================================
 * 1.information word (same format as above)
 * 2.status word
 * 3.- n: the data words follow
 * Bit 0-15 of the information word contain the number of words in the buffer
 *          including status and information word. There is no end marker
 * 
 * Status word: = 0 if no error
 * Bit 15: set if any error
 * </pre>
 * The interface uses the following defines to interprete the error bits:
 * @ingroup dcsc_msg_buffer_access
 */

/**
 * Status Word Bit 0: missing marker
 * @ingroup dcsc_msg_buffer_access
 */
#define MSGBUF_STATUS_NOMARKER  0x01
/**
 * Status Word Bit 1: missing end marker
 * @ingroup dcsc_msg_buffer_access
 */
#define MSGBUF_STATUS_NOENDMRK  0x02
/**
 * Status Word Bit 2: no target answer (something wrong with the RCU or not connected)
 * @ingroup dcsc_msg_buffer_access
 */
#define MSGBUF_STATUS_NOTGTASW  0x04
/**
 * Status Word Bit 3: no bus grant (no access to the bus on dcs board)
 * @ingroup dcsc_msg_buffer_access
 */
#define MSGBUF_STATUS_NOBUSGRT  0x08
/**
 * Status Word Bit 5: old message buffer format (prior to v2 with rcu-sh version v1.0)
 * @ingroup dcsc_msg_buffer_access
 */
#define MSGBUF_STATUS_OLDFORMAT 0X10

#include <linux/types.h> // for the __u32 type

/************************************************************************************************************/

/**
 * @name API methods: Initialization and general methods.
 */

/**
 * @struct dcscInitArguments_t
 * Argument struct for extended initialization
 * <!-- @ingroup dcsc_msg_buffer_access -->
 */
struct dcscInitArguments_t {
  unsigned int flags;  
  int iVerbosity;
  int iMIBSize;      // size of the MIB
};

/**
 * Type definition for extended initialization parameters.
 * @ingroup dcsc_msg_buffer_access
 */
typedef struct  dcscInitArguments_t TdcscInitArguments;

  /** 
   * Initialization flag: force message buffer format version v1
   * This sets the encoding to version 1 fixed rather than trying
   * to get the correct firmware version.
   * @ingroup dcsc_msg_buffer_access
   */
#define DCSC_INIT_FORCE_V1 0x0001
  /** 
   * Initialization flag: force message buffer format version v2.
   * This sets the encoding to version 2 fixed rather than trying
   * to get the correct firmware version.
   * @ingroup dcsc_msg_buffer_access
   */
#define DCSC_INIT_FORCE_V2 0x0002
  /** 
   * Initialization flag:  write block to device/file but do not execute.
   * The interface incodes the message buffer and writes it to the device/
   * file but does not execute the command.
   * @ingroup dcsc_msg_buffer_access
   */
#define DCSC_INIT_ENCODE   0x0100
  /** 
   * Initialization flag:  Append to file.
   * When in 'encoding' mode, the data is appended to the file. 
   * @ingroup dcsc_msg_buffer_access
   */
#define DCSC_INIT_APPEND   0x0200
  /** 
   * Initialization flag: suppress stderr output.
   * The stderr output is completely suppressed.
   * @ingroup dcsc_msg_buffer_access
   */
#define DCSC_SUPPR_STDERR  0x0400
  /** 
   * Initialization flag: skip automatic adaption to driver properties. 
   * By default the interfaces ties to fetch the charactaristics of the driver
   * during initialization (MIB size, version , ...). This flag causes the
   * interface to skip the adaption.
   * @ingroup dcsc_msg_buffer_access
   */
#define DCSC_SKIP_DRV_ADPT 0x1000

/**
 * Initialize the interface.
 * The device will be opened and some other other internal states initialized.
 * @param      pDeviceName: name of the device node, if NULL: /dev/dcsc as default
 * @return     neg. error code if failed<br>
 *   -ENOSPC : can not get the size of the interface buffers, or buffers too small<br>
 *   -ENOENT : can not open device
 * @ingroup dcsc_msg_buffer_access
 */
int initRcuAccess(const char* pDeviceName);
  
/** 
 * Extended initialization.
 * The function allows beside the device name a couple of other parameters
 * to adjust the interface behavior.
 * @param  pDeviceName  name of the device node, if NULL: /dev/dcsc as default
 * @param  flags        logical or of the following flags
 * @param  iMIBSize     size of the MIB, used for encoding of message blocks
 * @return neg. error code if failed<br>
 *   -ENOSPC : can not get the size of the interface buffers, or buffers too small<br>
 *   -ENOENT : can not open device
 * @ingroup dcsc_msg_buffer_access
 */
int initRcuAccessExt(const char* pDeviceName, TdcscInitArguments* pArg);

/** 
 * Close the device and release internal data structures.
 * @return neg. error code if failed <br>
 *   -ENXIO : no device open
 * @ingroup dcsc_msg_buffer_access
 */
int releaseRcuAccess();

/** 
 * Get driver info from the driver and print it.
 * @param  iVerbosity     1 full text, 0 just a few messages
 * @ingroup dcsc_msg_buffer_access
 */
void printDriverInfo(int iVerbosity);

/**
 * Start register simulation.
 * The interface implements simulation of the firmware response for debugging purpose. 
 * Instead reading the control register (reg 0), a text file will be read. This helps
 * when testing the software on another machine than the dcs board or when firmware is
 * under development a comprehensive description will follow later.<br>
 * <b>Note:</b> The simulation feature has to be turned on at configure time with the
 * option --enable-dcscsim
 * @ingroup dcsc_msg_buffer_access
 */
void startSimulation();

/**
 * Stop the simulation.
 * Cleanup function for the simulation feature.
 * @ingroup dcsc_msg_buffer_access
 */
void stopSimulation();

/**
 * Reset the simulation.
 * Reset all the internal variables of the register simulation and seek to beginning 
 * of the register files.
 */
void resetSimulation();

/************************************************************************************************************/

/**
 * @name Read/write access
 * @ingroup dcsc_msg_buffer_access
 */

/**
 * Write a single location (32bit word)
 * @param address   16 bit address in RCU memory space
 * @param data      data word
 * @return
 * @ingroup dcsc_msg_buffer_access
 */
int rcuSingleWrite(__u32 address, __u32 data);

/**
 * Read a single location (32bit word)
 * @param address   16 bit address in RCU memory space
 * @param pData     buffer to receive the data
 * @return
 * @ingroup dcsc_msg_buffer_access
 */
int rcuSingleRead(__u32 address, __u32* pData);

/**
 * Write a number of 32bit words beginning at a location.
 * The function takes care for the size of the MIB and splits the operation if
 * the amount of data to write exceeds the MIB size.
 * The function expects data in little endian byte order
 * @param address   16 bit address in RCU memory space
 * @param pData     buffer containing the data
 * @param iSize     number of words to write
 * @param iDataSize size of one word in bytes, allowed 1 (8 bit), 2 (16 bit),3 (compressed 10 bit) 
 *                  or 4 (32 bit) -2 and -4 denote 16/32 bit words which get swapped
 * @return
 * @ingroup dcsc_msg_buffer_access
 */
int rcuMultipleWrite(__u32 address, __u32* pData, int iSize, int iDataSize);

/**
 * Read a number of 32bit words beginning at a location.
 * @param address   16 bit address in RCU memory space
 * @param iSize     number of words to write
 * @param pData     buffer to receive the data, the function expect it to be of suitable size
 *                  (i.e. iSize x wordsize)
 * @return          number of 32bit words which have been read, neg. error code if failed
 * @ingroup dcsc_msg_buffer_access
 */
int rcuMultipleRead(__u32 address, int iSize,__u32* pData);

/**
 * Provide the message buffer for direct access.
 * Creation and destruction of the buffer is handled by the interface internally
 * <b>DO NOT DELETE THIS BUFFER</b>.<br>
 * <b>Note:</b> This function is forseen but not yet implemented
 * @param  ppBuffer    target to receive the buffer pointer
 * @param  pSize       target to receive the buffer size
 * @return pointer to buffer, the caller is supposed to encode the complete message buffer,
 *         including the information word, the markers and data as well as check sum.
 *         The interface does not alter it.
 * @ingroup dcsc_msg_buffer_access
 */
int dcscProvideMessageBuffer(__u32** ppBuffer, int* pSize);

/**
 * Prepare the message buffer for a specific command.
 * Creation and destruction of the buffer is handled by the interface internally
 * <b>DO NOT DELETE THIS BUFFER</b>.<br>
 * <b>Note:</b> This function is forseen but not yet implemented
 * @param  ppBuffer    target to receive the buffer pointer
 * @param  pSize       target to receive the buffer size
 * @param  cmdID       command ID to prepare the buffer for
 * @param  flags       for future extensions (e.g. preparation for compressed pedestal data)
 * @return pointer to buffer where the data words can directly be stored to, the information
 *         word and the markers and check sum are written by the interface.
 * @ingroup dcsc_msg_buffer_access
 */
int dcscPrepareMessageBuffer(__u32** ppBuffer, int* pSize, unsigned int cmdID, unsigned int flags);

/**
 * Execute the message buffer command. 
 * Pointer and size are required in order to cross check the buffer arguments.<br>
 * This tool is going to be used also for the encoding of configuration data, 
 * it might be usefull to write the ready prepared command buffer to a file or
 * pipe. In that sense no execution and result interpretation is needed, this
 * will be controlled by the operation flags.<br>
 * <b>Note:</b> This function is forseen but not yet implemented
 * @param  pCommand      pointer to command buffer to execute
 * @param  iSize         size of the command buffer
 * @param  ppTarget      target to receive the result buffer (e.g. for read operations)
 * @param  ppTargetSize  target to receive the size of the result buffer
 * @param  operation     flags for operation control (for future extension) 
 * result:
 *   result buffer if available
 * @ingroup dcsc_msg_buffer_access
 */
int dcscExecuteCommand(__u32* pBuffer, int iSize, __u32** ppTarget, int* ppTargetSize, unsigned int operation);


/**
 * Operation specifier for the @ref dcscGetHeaderAttribute function.
 * @ingroup dcsc_msg_buffer_access
 */
enum {
  eUnknownSpec = 0,
  eVersion,
  eCommand,
  eNofWords,
  ePacked,
};

/**
 * Extract a part of the header word according to the specifier.
 * @param header      the header word
 * @param iSpecifier  specifier for the part to extract
 * @return extracted part, neg. error code if failed
 * @ingroup dcsc_msg_buffer_access
 */
int dcscGetHeaderAttribute(__u32 header, int iSpecifier);

/**
 * Check a message block for correct format.
 * @param  pBuffer    pointer to message block
 * @param  iSize      size of the block in 32bit words
 * @param  iVerbosity 0: no messages, 1: warnings for format missmatch, 2: debug messages
 * @return >0 check successful, return block length in 32bit words<br>
 *         =0 unknown format<br>
 *         <0 other errors
 * @ingroup dcsc_msg_buffer_access
 */
int dcscCheckMsgBlock(__u32* pBuffer, int iSize, int iVerbosity);

/************************************************************************************************************/

/**
 * @name Driver control functions
 * @ingroup dcsc_msg_buffer_access
 */

/**
 * Operation ids for the @ref dcscLockCtrl function.
 * @ingroup dcsc_msg_buffer_access
 */
enum {
  eUnknownDriverCmd=0,
  /** lock the driver */
  eLock,
  /** unlock the driver */
  eUnlock,
  /**
   * bind driver to the application, access denied for all other
   * applications. 
   */ 
  eSeize,
  /** release after bind */
  eRelease,
  /** deactivate and reset the driver lock functionality */
  eDeactivateLock,
  /** activate the driver lock functionality */
  eActivateLock,
};

/**
 * Lock the driver
 * @param cmd   operation id   
 * @ingroup dcsc_msg_buffer_access
 */
int dcscLockCtrl(int cmd);

/**
 * Set debug flags for the driver
 * @param flags driver debug message id
 * @ingroup dcsc_msg_buffer_access
 */
int dcscDriverDebug(unsigned int flags);

/************************************************************************************************************/

/**
 * @name Bus control functions
 * @ingroup dcsc_msg_buffer_access
 */

/**
 * Command ids to the @ref rcuBusControlCmd function.
 * @ingroup dcsc_msg_buffer_access
 */
enum {
  eUnknownBusCtrl=0,
  eEnableSelectmap,  // enable the select map mode, interface has to be in msg buffer mode
  eDisableSelectmap,
  eEnableFlash,      // enable the flash mode, interface has to be in msg buffer mode
  eDisableFlash,
  eEnableMsgBuf,     // disable both select map and flash mode
  eResetFirmware,    // trigger a firmware reset sequence
  eReadCtrlReg,      // read the value of the control register
  eCheckSelectmap,   // check whether in mode selectmap
  eCheckFlash,       // check whether in mode flash
  eCheckMsgBuf,      // check whether in mode msg buffer
  eResetFlash,       // send reset command to flash
  eFlashID,          // read the flash id
  eFlashCtrlDCS,     // all flash control done by the DCS board FW, since FW version 2.2
  eFlashCtrlActel,   // all flash control done by the RCU Actel FW
  eEnableCompression,// use compressed data in msg buffer whenever possible
  eDisableCompression
};

/**
 * Switch bits in the control register (firmware comstat).
 * @param iCmd   buffer control command, see enums above
 * @return  the (new) value of the control register, neg. error code if failed<br>
 *          -EBADFD if interface in wrong state to perform the command<br>
 *          -EIO    if internal error (bits can not be changed)<br>
 *          -EINVAL unknown command id
 * @ingroup dcsc_msg_buffer_access
 */
int rcuBusControlCmd(int iCmd);

/**
 * Read the value of a register from the register buffer.
 * Originally, registers were 8 bit wide, but in the implementation of
 * the message buffer interface in the DCS board firmware the addressing 
 * was 32 bit wide. Thats why register numbers correspond to register
 * addresses = 4*regNo.
 * @param reg    # of register
 * @return 8 bit value of the register
 */
int msgBufReadRegister(int reg);

/**
 * Write an 8 bit value to a register of the register buffer.
 * The same as in @ref msgBufReadRegister applies for the register
 * addressing.
 * @param reg    # of register
 * @param value  8 bit value to write
 * @return 8 bit value of the register after write
 */
int msgBufWriteRegister(int reg, unsigned char value);


/************************************************************************************************************/

/**
 * @name Flash control functions
 * @ingroup dcsc_msg_buffer_access
 */

/**
 * Write to the RCU flash. 
 * currently all communication is done through the message buffer
 * @param address    start location
 * @param pData      buffer containing the data
 * @param iSize      number of words to write
 * @param iDataSize  size of one word in bytes, allowed 1,2
 * @return
 * @ingroup dcsc_msg_buffer_access
 */
int rcuFlashWrite(__u32 address, __u32* pData, int iSize, int iDataSize);

/**
 * Read a number of 16bit words from the flash memory.
 * currently all communication is done through the message buffer
 * @param address    start location
 * @param iSize      number of words to read
 * @param pData      buffer to receive the data, the function expect it to be of suitable size 
 *                   (i.e. iSize x wordsize)
 * @return number of read 32bit words, neg. error code if failed
 * @ingroup dcsc_msg_buffer_access
 */
int rcuFlashRead(__u32 address, int iSize,__u32* pData);

/**
 * Erase sectors of the flash.
 * @param  firstSec   the first sector to erase; if -1 erase all
 * @param  lastSec    the last sector to erase
 * @return 0 success, neg. error code if failed
 * @ingroup dcsc_msg_buffer_access
 */
int rcuFlashErase(int startSec, int stopSec);

/************************************************************************************************************/

/**
 * @name Debug Message Control
 * @ingroup dcsc_msg_buffer_access
 */

/**
 * Set the debug options.
 * Refer to the help menu.
 * @param options     debug options 
 * @return current value of options
 * @ingroup dcsc_msg_buffer_access
 */
int setDebugOptions(int options);

/**
 * Set a debug option flag.
 * @param of          option flags
 * @return current value of options
 * @ingroup dcsc_msg_buffer_access
 */
int setDebugOptionFlag(int of);

/**
 * clear a debug option flag.
 * @param of          option flags
 * @return current value of options
 * @ingroup dcsc_msg_buffer_access
 */
int clearDebugOptionFlag(int of);

/**
 * Print content of a buffer hexadecimal.
 * The content is assumed to be little endian (LSB first).
 * @param  pBuffer      pointer to buffer
 * @param  iBufferSize  size of the buffer in byte
 * @param  iWordSize    number of bytes in one word (1,2 or 4)
 * @param  pMessage     an optional message
 * @ingroup dcsc_msg_buffer_access
 */
void printBufferHex(unsigned char *pBuffer, int iBufferSize, int wordSize, const char* pMessage);

/**
 * Print content of a buffer hexadecimal formated with the address.
 * The function formats the buffer to hexadecimal ascii with a dedicated number
 * of words per row. Each row is preceeded by the address.
 * The content is assumed to be little endian (LSB first)
 * @param  pBuffer         pointer to buffer
 * @param  iBufferSize     size of the buffer in byte
 * @param  iWordSize       number of bytes in one word (1,2 or 4)
 * @param  iWordsPerRow    number of words in one row
 * @param  iStartAddress   start address for the output
 * @param  pMessage        an optional message
 * @ingroup dcsc_msg_buffer_access
 */
void printBufferHexFormatted(unsigned char *pBuffer, int iBufferSize, int iWordSize, int iWordsPerRow, int iStartAddress, const char* pMessage);

/**
 * @name Interface debug options
 * @brief The debug output of the interface can be adjusted by a couple of
 * flags. Flags can be changed by the @ref setDebugOptionFlag, @ref
 * clearDebugOptionFlag and @ref setDebugOptions functions. 
 * @ingroup dcsc_msg_buffer_access
 */

/**
 * Print the command buffer before writing to the MIB.
 * @ingroup dcsc_msg_buffer_access
 */
#define PRINT_COMMAND_BUFFER        0x01
/**
 * Print the MRB after executing the command.
 * @ingroup dcsc_msg_buffer_access
 */
#define PRINT_RESULT_BUFFER         0x02
/**
 * Test the MIB after writing the command buffer to it.
 * @ingroup dcsc_msg_buffer_access
 */
#define CHECK_COMMAND_BUFFER        0x04
/**
 * Ignore the result of the MIB read back and check.
 * @ingroup dcsc_msg_buffer_access
 */
#define IGNORE_BUFFER_CHECK         0x08
/**
 * Print access to the control registers.
 * @ingroup dcsc_msg_buffer_access
 */
#define PRINT_REGISTER_ACCESS       0x10
/**
 * Print the result of the command.
 * @ingroup dcsc_msg_buffer_access
 */
#define PRINT_COMMAND_RESULT        0x20
/**
 * Print split info for multiple operations.
 * @ingroup dcsc_msg_buffer_access
 */
#define PRINT_SPLIT_DEBUG           0x40
/**
 * Print debug information on the file conversion.
 * @ingroup dcsc_msg_buffer_access
 */
#define DBG_FILE_CONVERT            0x80
/**
 * Print the status bits human readable.
 * @ingroup dcsc_msg_buffer_access
 */
#define PRINT_RESULT_HUMAN_READABLE 0x100
/**
 * Print debug information concerning the wait ('check') command.
 * @ingroup dcsc_msg_buffer_access
 */
#define DBG_CHECK_COMMAND           0x400

/**
 * The default debug flags.
 * @ingroup dcsc_msg_buffer_access
 */
#define DBG_DEFAULT                 PRINT_RESULT_HUMAN_READABLE

/************************************************************************************************************/

/**
 * @name Interface error codes.
 * @brief The interface uses in general the error codes in errno.h.
 * In addition to that a few more are defined which reflect the
 * error returned in the MRB status word.
 * @ingroup dcsc_msg_buffer_access
 */

/**
 * Error indicates a missing marker in the message buffer command.
 * @ingroup dcsc_msg_buffer_access
 */
#define EMISS_MARKER         0x1001
/**
 * Error indicates a missing end marker at the and of the message
 * buffer command block.
 * @ingroup dcsc_msg_buffer_access
 */
#define EMISS_ENDMARKER      0x1002
/**
 * Error in the communication between DCS board and RCU motherboard,
 * no target answer.
 * @ingroup dcsc_msg_buffer_access
 */
#define ENOTARGET            0x1003
/**
 * Error on the DCS board internally, no bus grant.
 * @ingroup dcsc_msg_buffer_access
 */
#define ENOBUSGRANT          0x1004
/**
 * Error indicates an old message buffer v1 format.
 * The error can be thrown by firmware version v2 and higher.
 * @ingroup dcsc_msg_buffer_access
 */
#define EOLDFORMAT           0x1005

#ifdef __cplusplus
}
#endif
#endif //__DCSC_RCU_ACCESS_H
