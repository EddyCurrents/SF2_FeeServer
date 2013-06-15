// $Id: dcs_driver.h,v 1.8 2006/05/26 12:09:07 richter Exp $

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

#ifndef __DCS_DRIVER_H
#define __DCS_DRIVER_H
/** @file   dcs_driver.h
    @author Matthias Richter
    @date   
    @brief  The @ref rcubus_driver.
*/

/**
 * @defgroup rcubus_driver RCU bus driver
 * The RCU bus driver is a container for several sub-drivers:
 *
 * - the Message Buffer Driver provides access to a memory mapped interface used
 * for communication between the DCS board and the RCU.
 * - the Selectmap driver provides access to the Selectmap(TM) interface of the
 * Xilinx FPGA
 *
 * More specifications will follow.
 */

// ioctl commands
#define IOCTL_TEST_MSGBUF_IN      0x001 // not yet implemented
#define IOCTL_TEST_MSGBUF_OUT     0x002 // not yet implemented
#define IOCTL_GET_MSGBUF_IN_SIZE  0x003 // return the size of the message in buffer in bytes, argument is __u32*
#define IOCTL_GET_MSGBUF_OUT_SIZE 0x004 // return the size of the message out buffer in bytes, argument is __u32*
#define IOCTL_GET_REGFILE_SIZE    0x005 // return the number of the registers, argument is __u32*
#define IOCTL_GET_VERSION_V02     0x006 // the old IOCTL_GET_VERSION id, for backward compatibility
#define IOCTL_GET_VERSION         0x010 // driver major version no, upper 16 bit of the int, minor no 16 lower bits
                                        // arg treated as target string pointer from version 0.3 on
                                        // up to version 0.2 id was 0x006
#define IOCTL_GET_VERS_STR_SIZE   0x011 // length of the version string, implemented from version 0.3 on
#define IOCTL_SET_DEBUG_LEVEL     0x018 // not yet implemented
#define IOCTL_SET_LOG_FILE        0x019 // not yet implemented
#define IOCTL_LOCK_DRIVER         0x020 // lock the driver for a sequence of commands
#define IOCTL_UNLOCK_DRIVER       0x021 // unlock the driver
#define IOCTL_RESET_LOCK          0x022 // reset the lock and release all waiting processes
#define IOCTL_ACTIVATE_LOCK       0x023 // activate the lock after reset
#define IOCTL_SEIZE_DRIVER        0x024 // bind the driver for a certain application
#define IOCTL_RELEASE_DRIVER      0x025 // release the driver by the occupying application
#define IOCTL_READ_REG            0x080 // command = IOCTL_READ_REG + register address
#define IOCTL_WRITE_REG           0x0c0 // command = IOCTL_READ_REG + register address

// access mode
#define ACCESS_IN_BUFFER      0x01
#define ACCESS_OUT_BUFFER     0x02
#define ACCESS_REGFILE        0x04
// read/write access to all the buffers, depending on offset the buffer is choosen, IN->OUT->REG
#define ACCESS_ALL            (ACCESS_IN_BUFFER|ACCESS_OUT_BUFFER|ACCESS_REGFILE)
// read access to msg out buffer and write access to msg in buffer, reg access via ioctl commands
#define ACCESS_REGIONS        0x08

// version string
#define VERSION_STRING_SIZE 20
#ifdef NODEBUG
#define RELEASETYPE "release"
#else //NODEBUG
#define RELEASETYPE "debug"
#endif //NODEBUG

#endif //__DCS_DRIVER_H
