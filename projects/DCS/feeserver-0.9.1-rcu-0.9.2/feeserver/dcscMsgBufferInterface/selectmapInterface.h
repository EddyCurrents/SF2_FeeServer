// $Id: selectmapInterface.h,v 1.2 2007/03/17 00:01:15 richter Exp $

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

#ifndef __SELECTMAP_ACCESS_H
#define __SELECTMAP_ACCESS_H

#ifdef __cplusplus
extern "C" {
#endif
#include <linux/types.h> // for the __u32 type

/**
 * @defgroup selectmap_access Selectmap Interface
 * This module implements the basic access function to the Selectmap interface
 * of the Xilinx FPGA on the RCU.
 */

/**
 * @name API methods: Initialization and general methods.
 * @ingroup selectmap_access
 */

/**
 * Initialize the interface.
 * The device will be opened and some other other internal states initialized.
 * @param      pDeviceName: name of the device node, if NULL: /dev/rcu/virtex as default
 * @return     neg. error code if failed<br>
 *   -ENOSPC : can not get the size of the interface buffers, or buffers too small<br>
 *   -ENOENT : can not open device
 * @ingroup selectmap_access
 */
int initSmAccess(const char* pDeviceName);

/** 
 * Close the device and release internal data structures.
 * @return neg. error code if failed <br>
 *   -ENXIO : no device open
 * @ingroup selectmap_access
 */
int releaseSmAccess();

/************************************************************************************************************/

/**
 * @name Read/write access
 * @ingroup selectmap_access
 */

/**
 * Generates a Type 1 Header Package to send to the selectmap interface
 * @param rdwr		read (1) or write (0)
 * @param address	address of the register
 * @param words		#of words to read or write (max (2^11)-1)
 * @return
 * @ingroup selectmap_access
 */
__u32 smMakeT1Header(__u8 rdwr, __u16 address, __u16 words);

/**
 * Write one word to a register of the selectmap interface.
 * @param address   register number
 * @param data      data word
 * @return
 * @ingroup selectmap_access
 */
int smRegisterWrite(__u32 address, __u32 data);

/**
 * Read one word in a register of the selectmap interface.
 * @param address   register number
 * @param pData     buffer to receive the data
 * @return
 * @ingroup selectmap_access
 */
int smRegisterRead(__u32 address, __u32* pData);


#ifdef __cplusplus
}
#endif
#endif //__SELECTMAP_ACCESS_H
