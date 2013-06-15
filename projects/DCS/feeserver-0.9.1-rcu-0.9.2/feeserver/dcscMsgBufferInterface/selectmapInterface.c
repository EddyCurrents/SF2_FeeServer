// $Id: selectmapInterface.c,v 1.2 2007/03/17 00:01:15 richter Exp $

/************************************************************************
**
**
** This file is property of and copyright by the Experimental Nuclear 
** Physics Group, Dep. of Physics and Technology
** University of Bergen, Norway, 2006
** This file has been written by Stian Skjerveggen, Jan Martin Langeland
** and Jon Kristian Bernhardsen
** h106353@stud.hib.no h107995@stud.hib.no h106384@stud.hib.no
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

#include <stdio.h> /* printf */
#include <string.h> /*string */
#include <fcntl.h> /* O_CREAT etc */
#include <linux/errno.h> /* error codes */
#include <sys/ioctl.h> /*ioctl() */
#include <sys/stat.h> /* S_IWUSR etc */
#include "selectmapInterface.h"
#include "virtex_io.h" /* ioctl commands */

#define SM_REG_WRITE 0
#define SM_REG_READ 1

static int g_SmFile;

#ifndef DCSC_TEST
const char* g_pDefaultSmDevice="/dev/virtex";
#else //DCSC_TEST
const char* g_pDefaultSmDevice="/tmp/virtex";
#endif //DCSC_TEST

int initSmAccess(const char* pDeviceName)
{
	int iResult=0;
	const char* pDevice;
	int flags = O_RDWR;
	int mode = 0;

	// open default or specified device 
	if (pDeviceName)
		pDevice=pDeviceName;
	else
		pDevice=g_pDefaultSmDevice;
	/* checking for actual device or pseudo-device (file) */
	if (strncmp(pDevice, "/dev/", 4 != 0)) {
		flags |= O_CREAT; //create the file
		mode = S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP; //setting file mode
	}		
	fprintf(stderr, "initialize selectmap access interface: device %s\n", pDevice);
	// opening..
	g_SmFile = open(pDevice, flags, mode);

	if (g_SmFile > 0)
		iResult = 0;
	else
		iResult = -ENOENT;
	
	return iResult;
}

int releaseSmAccess()
{
	int iResult=0;
	fprintf(stderr, "release selectmap access interface\n");
	iResult = close(g_SmFile);
	return iResult;
}

__u32 smMakeT1Header(__u8 rdwr, __u16 address, __u16 words)
{
	/* checking type, either 0 (write) or 1 (read) */
	if (rdwr > 1)
		return -EINVAL; //Invalid argument
	/* checking address, max 14 bits */
	if (address&(0x3<<14))
		return -EINVAL;
	/* checking words, max 2^11-1 (2047) */
	if (words > 2047)
		return -EINVAL;

	/* putting together header */
	unsigned long int hdr = 0;
	hdr |= (1<<29); /* type 1*/
	hdr |= (1<<(rdwr?27:28)); /* if rd;27.bit, wr;28.bit */
	hdr |= (address<<13);
	hdr |= words;
	return hdr;
}


__u32 smMakeT2Header(__u8 rdwr, __u32 words)
{
	if (rdwr > 1)
		return 0;
	
	/* max 2^27-1 words */
	if (words > 134217727)
		return 0;

	/* putting together header */	
	__u32 hdr = 0;
	hdr |= (2<<29); /* type 2 */
	hdr |= (1<<(rdwr?27:28));
	hdr |= words;	
	return hdr;
}

__u32 smMakeFacHeader(__u8 ba, __u8 mja, __u8 mna)
{
	/* checking type, either 0 (write) or 1 (read) */
	if (ba > 2)
		return 0;
	
	/* putting together header */
	__u32 hdr = 0;
	hdr |= (ba << 25);
	hdr |= (mja << 17);
	hdr |= (mna << 9);	
	return hdr;
}



int smRegisterWrite(__u32 address, __u32 pData)
{
	int iResult=0;
	address = smMakeT1Header(SM_REG_WRITE, address, 1);
	if (iResult < 0) {
		fprintf(stderr, "invalid type 1 packet header, aborting write.\n");
		return iResult;
	}
	iResult = ioctl(g_SmFile, VIRTEX_SET_REG, address);
	iResult = ioctl(g_SmFile, VIRTEX_WRITE_REG, pData); //have to send the value here, error if just the pointer is sent, unknown why.
	return iResult;
}

int smRegisterRead(__u32 address, __u32 *pData)
{
	int iResult=0;
	*pData = smMakeT1Header(SM_REG_READ, address, 1);

	iResult = ioctl(g_SmFile, VIRTEX_READ_REG, pData);
	iResult = 1;
	
	return iResult;
}


