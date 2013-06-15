// $Id: dcscMsgBufferInterface.c,v 1.38 2007/05/15 14:35:02 richter Exp $

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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <linux/errno.h>
//#include <sys/types.h>
//#include <sys/stat.h>
//#include <sys/ioctl.h>
#include <fcntl.h>
//#include <unistd.h>
#include "dcs_driver.h"
#include "dcscMsgBufferInterface.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>

// format and location of firmware versions
// introduced May 2005, RCU4 card
// this comes along with the change of the format in the message buffer interface
// both rcu and dcscard version are located in the rcu memory space
#define DCSC_FIRMWARE_VERSION_ADDRESS        0// 0 switches off; proposed addresss is 0x1000, but not yet working
#define DCSC_FIRMWARE_VERSION_MINOR_SIZE     6    
#define DCSC_FIRMWARE_VERSION_MAJOR_SIZE     5
#define DCSC_FIRMWARE_VERSION_DAY_SIZE       5
#define DCSC_FIRMWARE_VERSION_MONTH_SIZE     4
#define DCSC_FIRMWARE_VERSION_YEAR_SIZE      12
#define DCSC_FIRMWARE_VERSION_MINOR_BITSHIFT 0
#define DCSC_FIRMWARE_VERSION_MAJOR_BITSHIFT DCSC_FIRMWARE_VERSION_MINOR_BITSHIFT+DCSC_FIRMWARE_VERSION_MINOR_SIZE
#define DCSC_FIRMWARE_VERSION_DAY_BITSHIFT   DCSC_FIRMWARE_VERSION_MAJOR_BITSHIFT+DCSC_FIRMWARE_VERSION_MAJOR_SIZE
#define DCSC_FIRMWARE_VERSION_MONTH_BITSHIFT DCSC_FIRMWARE_VERSION_DAY_BITSHIFT+DCSC_FIRMWARE_VERSION_DAY_SIZE
#define DCSC_FIRMWARE_VERSION_YEAR_BITSHIFT  DCSC_FIRMWARE_VERSION_MONTH_BITSHIFT+DCSC_FIRMWARE_VERSION_MONTH_SIZE
#define RCU_FIRMWARE_VERSION_ADDRESS         0x1001

#define RCUBUS_DRIVER_VERSION_MAJOR_BITSHIFT  16
#define RCUBUS_DRIVER_VERSION_MAJOR_SIZE      16
#define RCUBUS_DRIVER_VERSION_MINOR_BITSHIFT  0
#define RCUBUS_DRIVER_VERSION_MINOR_SIZE      16
#define DCSC_REQUIRED_DRIVER_VERSION_MAJOR    0
#define DCSC_REQUIRED_DRIVER_VERSION_MINOR    6

// the register addresses
// the registers are 8 bit wide, but in the address space they appear to be 32 bit
#define GENERAL_CTRL_REG_ADDR     0x00 // register 0 used for general control of message buffer
#define MAJOR_VERSION_NO_REG_ADDR 0x04 // minor version number of the firmware
#define MINOR_VERSION_NO_REG_ADDR 0x08 // minor version number of the firmware
#define REGISTER_3_ADDR           0x0c // register 3 not used

// bits in the GENERAL_CTRL_REG_ADDR 
#define COMMAND_EXECUTE           0x80 // execute the command in teh MIB
#define BUFFER_REREAD             0x40 // switch the internal multiplexer to read back the MIB
#define COMMAND_READY             0x01 // the interface sets the bit if ready
#define SIGNAL_CONFIGURATION      0x20 // bit 5 of register 0; switches to configuration mode
#define DIRECT_BUS_ACTIVATE       0x08 // bit 3 switches to direct bus mode
#define SELECTMAP_ACTIVATE        0x04 // bit 2 switches between flash mode and select map mode 
#define FIRMWARE_RESET            0x02 // bit 1 reset the firmware

// bits in the CFG_CTRL_REGISTER
#define BUFFER_0_EMPTY            0x01 // bit 0 of register 1, set if buffer (internal memory 0) can receive data
#define BUFFER_1_EMPTY            0x02 // bit 1 of register 1, set if buffer (internal memory 1) can receive data
#define CONFIGURATION_START       0x80 // bit 7 of register 1, bit reseted bei the machine if done
#define CONFIGURATION_STATUS_MASK 0x7c // bits 2 to 6 of register 1

// format og the information word (first word in the block)
struct msgbuf_header_t {
  int VALID_FROM_VERSION;           // first version this header format becomes valid
  int HEADER_VERSION_BITS;
  int HEADER_VERSION_BITSHIFT;
  int FRSTWORD_ADD_NUMWORDS;        // number of additional words beside the regular words of a certain command
  int FRSTWORD_BITSHIFT_CMDID;      // position of the command id
  int FRSTWORD_WIDTH_CMDID;         // width of the command id
  int FRSTWORD_BITSHIFT_SFTMSG;     // position of the safety message (defined in the v1 format, most likely never used)
  int FRSTWORD_WIDTH_SFTMSG;        // width of the safety message
  int FRSTWORD_BITSHIFT_DATAFMT;    // position of the data format
  int FRSTWORD_WIDTH_DATAFMT;       // width of the data format
  int FRSTWORD_BITSHIFT_NUMWORDS;   // position of the data word count field 
  int FRSTWORD_WIDTH_NUMWORDS;      // width of the data word count field
  int FRSTWORD_BITSHIFT_BLKNUM;     // position of the block counter field
  int FRSTWORD_WIDTH_BLKNUM;        // width of the block counter field
  int FRSTWORD_BITSHIFT_EXTENSION;  // defined in the v1 msg bufer specification, most likely never used
  int FRSTWORD_WIDTH_EXTENSION;     // dito
  int FRSTWORD_BITSHIFT_MODE;       // position of the mode field: rcu memory, flash, select map
  int FRSTWORD_WIDTH_MODE;          // width of the mode field
  int MARKER_PATTERN;
  int MARKER_WIDTH;
  int MARKER_BITSHIFT;
  int ENDMARKER_PATTERN;
  int ENDMARKER_WIDTH;
  int ENDMARKER_BITSHIFT;
};

struct msgbuf_header_t g_msgbuf_header_v1 = {
  /*   VALID_FROM_VERSION           */0, // first version this header format becomes valid
  /*   HEADER_VERSION_BITS          */0x0,
  /*   HEADER_VERSION_BITSHIFT      */28,
  /*   FRSTWORD_ADD_NUMWORDS        */2,
  /*   FRSTWORD_BITSHIFT_CMDID      */28,
  /*   FRSTWORD_WIDTH_CMDID         */4,
  /*   FRSTWORD_BITSHIFT_SFTMSG     */27,
  /*   FRSTWORD_WIDTH_SFTMSG        */0, // forseen by the v1 specification, but never implemented
  /*   FRSTWORD_BITSHIFT_DATAFMT    */26,
  /*   FRSTWORD_WIDTH_DATAFMT       */0, // forseen for compression of 10 bit pedestal words, but never implemented
  /*   FRSTWORD_BITSHIFT_NUMWORDS   */0,
  /*   FRSTWORD_WIDTH_NUMWORDS      */16,
  /*   FRSTWORD_BITSHIFT_BLKNUM     */16,
  /*   FRSTWORD_WIDTH_BLKNUM        */8,
  /*   FRSTWORD_BITSHIFT_EXTENSION  */24,
  /*   FRSTWORD_WIDTH_EXTENSION     */2,
  /*   FRSTWORD_BITSHIFT_MODE       */0, // mode operation was defined in version 2.2
  /*   FRSTWORD_WIDTH_MODE          */0,
  /*   MARKER_PATTERN               */0xaa55,
  /*   MARKER_WIDTH                 */16,
  /*   MARKER_BITSHIFT              */16,
  /*   ENDMARKER_PATTERN            */0xdd33,
  /*   ENDMARKER_WIDTH              */16,
  /*   ENDMARKER_BITSHIFT           */16,
};

struct msgbuf_header_t g_msgbuf_header_v2 = {
  /*   VALID_FROM_VERSION           */0x00020000, // available from version 2.0
  /*   HEADER_VERSION_BITS          */MSGBUF_VERSION_2,
  /*   HEADER_VERSION_BITSHIFT      */0, // MSGBUF_VERSION_2 already 32 bit number
  /*   FRSTWORD_ADD_NUMWORDS        */0,
  /*   FRSTWORD_BITSHIFT_CMDID      */0,
  /*   FRSTWORD_WIDTH_CMDID         */6,
  /*   FRSTWORD_BITSHIFT_SFTMSG     */0,
  /*   FRSTWORD_WIDTH_SFTMSG        */0,
  /*   FRSTWORD_BITSHIFT_DATAFMT    */0,
  /*   FRSTWORD_WIDTH_DATAFMT       */0,
  /*   FRSTWORD_BITSHIFT_NUMWORDS   */6,
  /*   FRSTWORD_WIDTH_NUMWORDS      */10,
  /*   FRSTWORD_BITSHIFT_BLKNUM     */16,
  /*   FRSTWORD_WIDTH_BLKNUM        */8,
  /*   FRSTWORD_BITSHIFT_EXTENSION  */0,
  /*   FRSTWORD_WIDTH_EXTENSION     */0,
  /*   FRSTWORD_BITSHIFT_MODE       */0,
  /*   FRSTWORD_WIDTH_MODE          */0,
  /*   MARKER_PATTERN               */0xaa55,
  /*   MARKER_WIDTH                 */16,
  /*   MARKER_BITSHIFT              */16,
  /*   ENDMARKER_PATTERN            */0xdd33,
  /*   ENDMARKER_WIDTH              */16,
  /*   ENDMARKER_BITSHIFT           */16,
};

struct msgbuf_header_t g_msgbuf_header_v2_2 = {
  /*   VALID_FROM_VERSION           */0x00020002, // available from version 2.2
  /*   HEADER_VERSION_BITS          */MSGBUF_VERSION_2_2,
  /*   HEADER_VERSION_BITSHIFT      */0, // MSGBUF_VERSION_2_2 already 32 bit number
  /*   FRSTWORD_ADD_NUMWORDS        */0,
  /*   FRSTWORD_BITSHIFT_CMDID      */0,
  /*   FRSTWORD_WIDTH_CMDID         */6,
  /*   FRSTWORD_BITSHIFT_SFTMSG     */0,
  /*   FRSTWORD_WIDTH_SFTMSG        */0,
  /*   FRSTWORD_BITSHIFT_DATAFMT    */24,
  /*   FRSTWORD_WIDTH_DATAFMT       */2,
  /*   FRSTWORD_BITSHIFT_NUMWORDS   */6,
  /*   FRSTWORD_WIDTH_NUMWORDS      */10,
  /*   FRSTWORD_BITSHIFT_BLKNUM     */16,
  /*   FRSTWORD_WIDTH_BLKNUM        */8,
  /*   FRSTWORD_BITSHIFT_EXTENSION  */0,
  /*   FRSTWORD_WIDTH_EXTENSION     */0,
  /*   FRSTWORD_BITSHIFT_MODE       */26,
  /*   FRSTWORD_WIDTH_MODE          */1,
  /*   MARKER_PATTERN               */0xaa55,
  /*   MARKER_WIDTH                 */16,
  /*   MARKER_BITSHIFT              */16,
  /*   ENDMARKER_PATTERN            */0xdd33,
  /*   ENDMARKER_WIDTH              */16,
  /*   ENDMARKER_BITSHIFT           */16,
};
struct msgbuf_header_t *g_pMsgBuf_header=&g_msgbuf_header_v2_2;


/******************************************************************************************************
 * 
 */
enum {
  eFlashAccessActel = 0, // access via flash interface of the RCU Actel
  eFlashAccessDcsc       // access via the DCS board firmware
};

#define FLASH_DCS_ACCESS_REQUIRED_FW_VERSION 0x00020002 // version 2.2 or higher required
#define COMPRESSED_DATA_REQUIRED_FW_VERSION 0x00020002 // version 2.2 or higher required

#define MSGBUF_MODE_MEMMAPPED 0x0 // transactions to rcu memory space
#define MSGBUF_MODE_FLASH     0x1 // transaction to flash memory space
/******************************************************************************************************
 * some global variables used by the interface
 */

/*
 * program options
 */
/* #define PRINT_COMMAND_BUFFER_CTR_STRING   "print command buffer" */
/* #define PRINT_RESULT_BUFFER_CTR_STRING     "print result buffer" */
/* #define CHECK_COMMAND_BUFFER_CTR_STRING   "check command buffer" */
/* #define IGNORE_BUFFER_CHECK_CTR_STRING    "ignore buffer check" */
/* #define PRINT_CTRL_REGISTER_CTR_STRING    "print control register" */
int g_options=DBG_DEFAULT;

// default device name, used if the device parameter of the initRcuAccess function is NULL
#ifndef DCSC_TEST
const char* g_pDeviceName="/dev/rcu/msgbuf";
#else //DCSC_TEST
const char* g_pDeviceName="/tmp/dcsc";
#endif //DCSC_TEST
                                                              

// the global variables
static int g_file=-1; // file descriptor for the device
static unsigned int g_dcscFlags=0;
static int g_verbosity=1;
static int g_iDriverVersion=-1;
static int g_iFlashAccessMode=eFlashAccessActel; // access mode for the flash
static int g_iFirmwareVersion=0;
static int g_bCompression=0;                     // enable compression for msg buffer transactions (v2.2 or higher) 

/* driver parameters, the interface requests the current values during the initialization and 
 * overrides eventually the default values
 */
static __u32 message_in_buffer_size=0x40;  // size of the Message In Buffer (MIB)
static __u32 message_out_buffer_size=0x40; // size of the Message Result Buffer (MRB)
static __u32 message_regfile_size=0x10;    // number of control registers
static __u32 message_out_buffer_offset=0;  // offset of the MRB, set during initialization
static __u32 regfile_offset=0;             // offset of the control registers, set during initialization

// global variables for the register response simulation
#ifdef DCSC_SIMULATION
int g_nofreg=2;
int g_bConfiguration=0;
int g_bConfStarted=0;
int g_iRegCount[4];
FILE* g_simregfile[4];
int g_count[4];
int g_value[4];
int g_oldval[4];
#endif // DCSC_SIMULATION

/******************************************************************************************************
 * code taken from altrogui/wrapper.c as a quick solution
 */
#define POLY 0x8408

// local copy of driver access space - used to write to and read from the DCS board
__u32* pBuffer=NULL;
__u32* pMib=NULL;
__u32* pMrb=NULL;
__u32* pReg=NULL;
__u32 bufferSize=0;
__u32 mibSize=0;
__u32 mrbSize=0;
__u32 regSize=0;

unsigned short crc16(char *data_p, int length){
  
  unsigned char i;
  unsigned int data;
  unsigned short crc = 0xffff;
  
  if (length == 0)
    return (~crc);
  
  do
    {
      for (i=0, data=(unsigned int)0xff & *data_p++;
           i < 8;
           i++, data >>= 1)
        {
          if ((crc & 0x0001) ^ (data & 0x0001))
            crc = (crc >> 1) ^ POLY;
          else  crc >>= 1;
        }
    } while (--length);
  
  crc = ~crc;
  data = crc;
  crc = (crc << 8) | ((data >> 8) & 0xff);
  
  return (crc);
}

__u32 mkFrstWrd(unsigned short numWords, unsigned short blockNum, unsigned short format, unsigned short mode, unsigned short cmdId){ 
 unsigned short extension = 0x0000;
 __u32 frstWrd = 0;
 if (g_pMsgBuf_header) {
   frstWrd |= (g_pMsgBuf_header->HEADER_VERSION_BITS)<<g_pMsgBuf_header->HEADER_VERSION_BITSHIFT;
   frstWrd |= (cmdId&((0x1<<g_pMsgBuf_header->FRSTWORD_WIDTH_CMDID)-1))<<g_pMsgBuf_header->FRSTWORD_BITSHIFT_CMDID;
   frstWrd |= (mode&((0x1<<g_pMsgBuf_header->FRSTWORD_WIDTH_MODE)-1))<<g_pMsgBuf_header->FRSTWORD_BITSHIFT_MODE;
   frstWrd |= (format&((0x1<<g_pMsgBuf_header->FRSTWORD_WIDTH_DATAFMT)-1))<<g_pMsgBuf_header->FRSTWORD_BITSHIFT_DATAFMT;
   frstWrd |= (extension&((0x1<<g_pMsgBuf_header->FRSTWORD_WIDTH_EXTENSION)-1))<<g_pMsgBuf_header->FRSTWORD_BITSHIFT_EXTENSION;
   frstWrd |= (blockNum&((0x1<<g_pMsgBuf_header->FRSTWORD_WIDTH_BLKNUM)-1))<<g_pMsgBuf_header->FRSTWORD_BITSHIFT_BLKNUM;
   frstWrd |= ((numWords+g_pMsgBuf_header->FRSTWORD_ADD_NUMWORDS)&((0x1<<g_pMsgBuf_header->FRSTWORD_WIDTH_NUMWORDS)-1))<<g_pMsgBuf_header->FRSTWORD_BITSHIFT_NUMWORDS;
 } else {
   fprintf(stderr, "dcscAccess: fatal error, no message buffer header format available\n");
 }

  return frstWrd;
}

__u32 mkLstWrd(unsigned short checksum){
  __u32 lstWrd = 0;
 if (g_pMsgBuf_header) {
  lstWrd = (g_pMsgBuf_header->MARKER_PATTERN<<g_pMsgBuf_header->MARKER_BITSHIFT) | (checksum<<0);
 } else {
   fprintf(stderr, "dcscAccess: fatal error, no message buffer header format available\n");
 }
  return lstWrd;
}

__u32 mkEndMarker(){
  __u32 endMarker = 0;
  unsigned short unused = 0x0000;
 if (g_pMsgBuf_header) {
   endMarker = (g_pMsgBuf_header->ENDMARKER_PATTERN<<g_pMsgBuf_header->ENDMARKER_BITSHIFT) | (unused<<0);
 } else {
   fprintf(stderr, "dcscAccess: fatal error, no message buffer header format available\n");
 }
  return endMarker;
} 

/******************************************************************************************************
 * helper functions
 */
void printBufferHex(unsigned char *pBuffer, int iBufferSize, int wordSize, const char* pMessage)
{
  if (pBuffer) {
    int i;
    int value=0;
    int hadr,ladr,adr;
    if (pMessage)
      printf("%s:", pMessage);
    for (i=0; i<iBufferSize; i++) {
      hadr=i/wordSize;
      ladr=i%wordSize;
      if (ladr==0)
	printf(" 0x");
      adr=(hadr+1)*wordSize-1-ladr;
      if (adr<iBufferSize)
	value=pBuffer[adr];
      printf("%02x", value);
    }
    printf("\n");
  } else {
    fprintf(stderr, "printMessageBufferHex: invalid parameter\n");
  }
}

void printBufferHexFormatted(unsigned char *pBuffer, int iBufferSize, int iWordSize, int iWordsPerRow, int iStartAddress, const char* pMessage)
{
  if (pBuffer) {
    if (pMessage) {
      printf("%s\n", pMessage);
    }
    char strAddressBuffer[20];
    char* strAddress=NULL;
    int iRowNo=0;
    int iPrintSize=iWordsPerRow*iWordSize;
    int iNofWords=iBufferSize/iWordSize;
    for (iRowNo=0; iRowNo<=(iNofWords-1)/iWordsPerRow; iRowNo++) {
      if (iStartAddress >=0 ) {
	sprintf(strAddressBuffer, "%#4x", iStartAddress+(iRowNo*iWordsPerRow));
	strAddress=strAddressBuffer;
      }
      if ((iRowNo+1)*iWordsPerRow>iNofWords)
	iPrintSize=(iNofWords-(iRowNo*iWordsPerRow))*iWordSize;
      printBufferHex(pBuffer+(iRowNo*iWordsPerRow*iWordSize), iPrintSize, iWordSize, strAddress);
    }
  }
}

/******************************************************************************************************
 * general initialization
 */

// forward declarations
int closeDevice();
int writeDcscRegister(int regNo, unsigned char data);
int readDcscRegister(int regNo, int bAlert);

/* internal initialization function
 * the function opens the device and sets the global file descriptor
 * it also requests the parameters (buffer lengths) from the driver and initializes the internal variables 
 * called from initRcuAccess
 */
int openDevice(const char* pDeviceName)
{
  int iResult=0;
  const char* pDevice;

  // open default or specified device 
  if (pDeviceName)
    pDevice=pDeviceName;
  else
    pDevice= g_pDeviceName;
  int flags=O_RDWR;
  int mode=0;
  if (strncmp(pDevice, "/dev/", 4)!=0) {
    flags|=O_CREAT;
    if (g_dcscFlags&DCSC_INIT_APPEND)
      flags|=O_APPEND;
    mode=S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP;
  }
  g_file=open(pDevice,flags, mode);

  if (g_file>0){
    __u32 ioctlReturn=0;

    // request the driver parameters
    if ((g_dcscFlags&DCSC_SKIP_DRV_ADPT)==0) {
      if (ioctl(g_file, IOCTL_GET_MSGBUF_IN_SIZE, &ioctlReturn)>=0) {
	message_in_buffer_size=ioctlReturn;
      } else if (g_verbosity>0)
	fprintf(stderr, "can not get message in buffer size from driver, using default of 0x%x\n", message_in_buffer_size);
      if (ioctl(g_file, IOCTL_GET_MSGBUF_OUT_SIZE, &ioctlReturn)>=0) {
	message_out_buffer_size=ioctlReturn;
      } else if (g_verbosity>0)
	fprintf(stderr, "can not get message out buffer size from driver, using default of 0x%x\n", message_out_buffer_size);
      if (ioctl(g_file, IOCTL_GET_REGFILE_SIZE, &ioctlReturn)>=0) {
	message_regfile_size=ioctlReturn;
      } else if (g_verbosity>0)
	fprintf(stderr, "can not get register buffer size from driver, using default of 0x%x\n", message_regfile_size);
      int iMajor=0;
      int iMinor=0;
      if ((iMajor=ioctl(g_file, IOCTL_READ_REG+MAJOR_VERSION_NO_REG_ADDR, 0))>=0) {
	if (iMajor>0) {
	  iMinor=ioctl(g_file, IOCTL_READ_REG+MINOR_VERSION_NO_REG_ADDR, 0);
	  g_iFirmwareVersion=(iMajor<<16)|iMinor;
	  if (g_iFirmwareVersion>=g_msgbuf_header_v2_2.VALID_FROM_VERSION) {
	    g_pMsgBuf_header=&g_msgbuf_header_v2_2;
	  } else {
	    g_pMsgBuf_header=&g_msgbuf_header_v2;
	  }

	  // set default behavior with respect to firmware version
	  if (g_iFirmwareVersion>=FLASH_DCS_ACCESS_REQUIRED_FW_VERSION) g_iFlashAccessMode=eFlashAccessDcsc;
	  else g_iFlashAccessMode=eFlashAccessActel;
	  if (g_iFirmwareVersion>=COMPRESSED_DATA_REQUIRED_FW_VERSION) g_bCompression=1;
	  else g_bCompression=0;

	  if (g_verbosity>0)
	    fprintf(stderr, "open device: using message buffer v2 format for firmware version %d.%d\n", iMajor, iMinor);
	} else {
	  g_pMsgBuf_header=&g_msgbuf_header_v1;
	  if (g_verbosity>0)
	    fprintf(stderr, "open device: using message buffer v1 format\n");
	}
      } else {
	g_pMsgBuf_header=&g_msgbuf_header_v2;
	if (g_verbosity>0)
	  fprintf(stderr, "can not get firmware version from driver, using msg buffer format v2\n");
      }
    }

    /* check for buffer sizes: for the message buffer interface min. 24 bytes for the MIB
     * (1 header word, 1 address, 1 count, 1 data, 2 markers) are required to support
     * all commands. The min MRB size is 12 (1 header, 1 status, 1 data) and for the
     * register buffer 1.
     */
    if (message_in_buffer_size<24 || message_out_buffer_size<12 || message_regfile_size<1) {
      fprintf(stderr, "warning: driver buffer(s) to small to operate on it, check 'driver info' 'd'\n");
      fprintf(stderr, "warning: minimum required sizes: in=26 out=12 register=1 byte(s)\n");
    }

    // initialize internal variables
    message_out_buffer_offset=message_in_buffer_size;
    regfile_offset=message_out_buffer_offset+message_out_buffer_size;

    // initalize buffer
    mibSize=message_in_buffer_size/4;
    mrbSize=message_out_buffer_size/4;
    regSize=message_regfile_size/4;
    bufferSize=mibSize+mrbSize+regSize;
    pBuffer = (__u32*)malloc(bufferSize*4); 
    pMib=pBuffer;
    pMrb=pBuffer+mibSize;
    pReg=pMrb+mrbSize;
 
    if (iResult<0) {
      closeDevice(); // close device in case of error
    } else if (g_verbosity>0) {
      // work around for DCSC_TEST case: the register can not be read
      // if a new file is used instead of the driver, this just writes something
      // to the last register position and thus creates the file of appropriate length 
      if (readDcscRegister(GENERAL_CTRL_REG_ADDR, 0)<0 && (g_dcscFlags&DCSC_INIT_ENCODE)==0) {
#ifdef DCSC_TEST
	writeDcscRegister(REGISTER_3_ADDR, 0);
#else  //DCSC_TEST
	fprintf(stderr, "hardware access error\n");
	closeDevice();
	iResult=-EIO;
#endif //DCSC_TEST
      }
      printDriverInfo(0); // print a short driver info
    }
  } else {
    fprintf(stderr, "can not open %s\n", pDevice);
    iResult=-ENOENT;
  }
  
  return iResult;
}

/* internal clean up function
 * closes the device
 * called by releaseRcuAccess
 */
int closeDevice()
{
  int iResult=0;
    if (g_file>0) {
      int file=g_file;
      g_file=-1;
      close(file);
  } else
    iResult=-ENXIO;

  //Deleting pBuffer
  mibSize=0;
  mrbSize=0;
  regSize=0;
  bufferSize=0;
  pMib=NULL;
  pMrb=NULL;
  pReg=NULL;
  free(pBuffer);

  return iResult;
}

/*
 * interface method, see dcscMsgBufferInterface.h for details
 * kept for backward compatibility
 */
int initRcuAccess(const char* pDeviceName) {
  return initRcuAccessExt(pDeviceName, NULL);
}

/*
 * interface method, see dcscMsgBufferInterface.h for details
 */
int initRcuAccessExt(const char* pDeviceName, TdcscInitArguments* pArg)
{
  int iResult=0;
/*   if (g_fp) { */
/*     printf("rcuAccess locked by another user, currently this is not a multiuser program\n"); */
/*     iResult=-EBUSY; */
/*   } else { */
  if (pArg) {
    g_dcscFlags|=pArg->flags;
    g_verbosity=pArg->iVerbosity;
    if (g_verbosity>1) {
      if (g_dcscFlags&DCSC_INIT_ENCODE)
	fprintf(stderr, "initRcuAccessExt info: set DCSC_INIT_ENCODE flag\n");
      if (g_dcscFlags&DCSC_INIT_APPEND)
	fprintf(stderr, "initRcuAccessExt info: set DCSC_INIT_APPEND flag\n");
    }
    if (pArg->iMIBSize>0) {
      message_in_buffer_size=pArg->iMIBSize;
      g_dcscFlags|=DCSC_SKIP_DRV_ADPT;
      if (g_verbosity>1) {
	fprintf(stderr, "initRcuAccessExt info: set MIB size to %d\n", message_in_buffer_size);
      }
    }
  }
  iResult=openDevice(pDeviceName);
/*   } */
  return iResult;
}

/*
 * interface method, see dcscMsgBufferInterface.h for details
 */
int releaseRcuAccess()
{
  int iResult=0;
  iResult=closeDevice();
  return iResult;
}

/******************************************************************************************************
 * basic read/write access to driver
 * all seek/write/read operations performed by the interface go through this basic functions
 */

/* internal function
 * set the position in the device
 * parameter:
 *    offset - offset in byte from the beginning
 *    start - ignored by now
 */
int seek_dcsc(long offset, int start)
{
  int iResult=0;
  if (start!=0) {
    // the driver supports only seeking from the beginning
    fprintf(stderr,"seek_dcsc: no other options than seek from beginning supported by now - please change your code!\n");
    iResult=-ENOSYS; // this function is not yet implemented
  } else {
    if (g_file>0) {
      off_t result=lseek(g_file, offset, SEEK_SET);
      if (result!=offset)
	iResult=-EIO;
    } else {
      iResult=-EBADF;
      fprintf(stderr, "seek_dcsc: internal error: device not opened\n");
    }
  }
  return iResult;
}

/* internal write function,  works like fwrite, return number of items written
 * parameter:
 *     ptr - pointer to data buffer
 *     size - size of one data member in byte
 *     nmemb - number of members
 */
int write_dcsc(const void *ptr, size_t size, size_t nmemb)
{
  int iResult=0;
  if (g_file>0) {
    ssize_t result=write(g_file, ptr, size*nmemb);
    if (result>=0)
      iResult=result/size;
    else {
      fprintf(stderr, "write_dcsc: internal error: can not write (%d)\n", result);
      iResult=-1;
    }
  } else {
    iResult=-EBADF;
    fprintf(stderr,"write_dcsc: internal error: device not opened\n");
  }
  return iResult;
}

/* internal read function,  works as fread, return number of items read
 * parameter:
 *     ptr - pointer to data buffer
 *     size - size of one data member in byte
 *     nmemb - number of members
 */
int read_dcsc(void *ptr, size_t size, size_t nmemb)
{
  int iResult=0;
  if (g_file>0) {
    ssize_t result=read(g_file, ptr, size*nmemb);
    iResult=result/size;
  } else {
    iResult=-EBADF;
    fprintf(stderr,"read_dcsc: internal error: device not opened\n");
  }
  return iResult;
}

/* basic device lock functions
 */
int g_AppID=0;
int lock_device()
{
  int iResult=0;
  if (g_file) {
    iResult=ioctl(g_file, IOCTL_LOCK_DRIVER, g_AppID);
  } else {
    iResult=-EBADF;
    fprintf(stderr,"lock_device: internal error: device not opened\n");
  }
  return iResult;
}

int unlock_device()
{
  int iResult=0;
  if (g_file)
    iResult=ioctl(g_file, IOCTL_UNLOCK_DRIVER, g_AppID);
  else {
    iResult=-EBADF;
    fprintf(stderr,"unlock_device: internal error: device not opened\n");
  }
  return iResult;
}

int seize_device()
{
  int iResult=0;
  if (g_AppID==0) {
    int lckCode=time(NULL);
    if (lckCode>0) {
      iResult=ioctl(g_file, IOCTL_SEIZE_DRIVER, lckCode);
      fprintf(stderr, "seize driver iResult %d\n", iResult);
      g_AppID=lckCode;
    } else {
      iResult=-ENOLCK;
    }
  } else {
    iResult=-EAGAIN;
  }
  return iResult;
}

int release_device()
{
  int iResult=0;
  if (g_AppID>0) {
    iResult=ioctl(g_file, IOCTL_RELEASE_DRIVER, g_AppID);
    g_AppID=0;
  } else {
    iResult=-ENOLCK;
  }
return iResult;
}

int reset_device_lock()
{
  int iResult=0;
  if (g_file)
    iResult=ioctl(g_file, IOCTL_RESET_LOCK, 0);
  else {
    iResult=-EBADF;
    fprintf(stderr,"reset_device_lock: internal error: device not opened\n");
  }
  return iResult;
}

int activate_device_lock()
{
  int iResult=0;
  if (g_file)
    iResult=ioctl(g_file, IOCTL_ACTIVATE_LOCK, 0);
  else {
    iResult=-EBADF;
    fprintf(stderr,"activate_device_lock: internal error: device not opened\n");
  }
  return iResult;
}

/* interface functions for driver debugging
 */

/* lock the driver
 */
int dcscLockCtrl(int cmd)
{
  int iResult=0;
  switch (cmd) {
  case eLock:
    iResult=lock_device();
    break;
  case eUnlock:
    iResult=unlock_device();
    break;
  case eSeize:
    iResult=seize_device();
    break;
  case eRelease:
    iResult=release_device();
    break;
  case eDeactivateLock:
    iResult=reset_device_lock();
    break;
  case eActivateLock:
    iResult=activate_device_lock();
    break;
  default:
    iResult=-EINVAL;
  }
  return iResult;
}

int dcscDriverDebug(unsigned int flags)
{
  int iResult=0;
  if (g_file)
    iResult=ioctl(g_file, IOCTL_SET_DEBUG_LEVEL, flags);
  else
    iResult=-EBADF;
  return iResult;
}

/******************************************************************************************************
 * this group implements the basic access to the buffers of the DCS message buffer interface 
 */

/* write one of the 8bit register of the register buffer
 * internal function
 * parameter:
 *    regNo - the register address
 *    data - the data to write 
 */
int writeDcscRegister(int regNo, unsigned char data)
{
  int iResult=0;
  if (g_options&PRINT_REGISTER_ACCESS)
    fprintf(stderr, "write %#x to register no. %d\n", data, regNo);

  // the function basically seeks to the position and writes the 8bit value
  // !!! there was some 8/32bit confision whe using another register than 0, the firmware didnt 
  // respond properly, but the is now some months ago so I dont really remember
  if (seek_dcsc(regfile_offset+regNo, 0)>=0){
    if (write_dcsc(&data, 1, 1)!=1){
      fprintf(stderr,"writeDcscRegister: failed to write register %d\n", regNo);
      iResult=-1;
    } else {
#ifdef DCSC_SIMULATION
      // this is an early implementation for simulation of the register response. This was designed
      // for the testing of the rcu-fpga configuration mode, which is subject of development
      if (regNo==GENERAL_CTRL_REG_ADDR) {
	if (data==SIGNAL_CONFIGURATION) {
	  g_bConfiguration=1;
	  writeDcscRegister(CFG_CTRL_REGISTER_ADDR, 0);
	} else {
	  g_bConfiguration=0;
	}
      }
#endif // DCSC_SIMULATION
    }
  } else {
    fprintf(stderr,"writeDcscRegister: seek to register %d failed\n", regNo);
    iResult=-1;
  }
  return iResult;
}

#ifdef DCSC_SIMULATION
// just a forward declaration
int readDcscRegisterFromFile(int regNo);
#endif //DCSC_SIMULATION

/* read one of the 8bit register of the register buffer
 * internal function
 * @param regAddress   address of the register
 * @return  register value if successful, neg. error code if failed
 */
int readDcscRegister(int regAddress, int bAlert)
{
  int iResult=0;
  unsigned char data=0;
#ifdef DCSC_SIMULATION
  if ((iResult=readDcscRegisterFromFile(regAddress))>=0){
    if (regAddress==GENERAL_CTRL_REG_ADDR && iResult>=0 && g_bConfiguration) {
      iResult|=SIGNAL_CONFIGURATION;
    }
  } else
#endif // DCSC_SIMULATION
  {
    iResult=0;
    if (seek_dcsc(regfile_offset+regAddress, 0)>=0){
      if (read_dcsc(&data, 1, 1)!=1){
	if (bAlert)
	  fprintf(stderr,"readDcscRegister: failed to read register address %d\n", regAddress);
	iResult=-1;
      }
    } else {
      if (bAlert)
	fprintf(stderr,"readDcscRegister: seek to register address %d failed\n", regAddress);
      iResult=-1;
    }
    if (iResult==0) {
      iResult=data;
    }
  } 
  if (g_options&PRINT_REGISTER_ACCESS && iResult>=0)
    fprintf(stderr, "read register address %d: %#x\n",regAddress, iResult); 
  return iResult;
}

int setDcscRegisterBit(int regNo, unsigned char pattern)
{
  int iResult=readDcscRegister(regNo,0);
  if (iResult>=0) {
    iResult=writeDcscRegister(regNo, iResult|pattern);
  }
  return iResult;
}

int clearDcscRegisterBit(int regNo, unsigned char pattern)
{
  int iResult=readDcscRegister(regNo,0);
  if (iResult>=0) {
    iResult=writeDcscRegister(regNo, iResult&(~pattern));
  }
  return iResult;
}

/* 
 */
enum {
  eUnknownState = 0,
  eMsgBuffer,
  eSelectMap,
  eFlash
};

/* the mode of the interface is steered by bits in the GENERAL_CTRL_REG. Normal mode means all communication
 * with the RCU goes via the message buffer. The flash and the selectmap mode operate directly on the bus
 * lines, in order to establish contact with the flash memory or the fpga
 * the interface can be in 3 states:
 *  1. the message buffer is active
 *  2. the selectmap interface is active
 *  3. the flash interface is active
 */
int checkState(int state, int bAlert)
{
  int iResult=0;
  if (g_dcscFlags&DCSC_INIT_ENCODE) {
    iResult=1;
  } else {
    int reg=readDcscRegister(GENERAL_CTRL_REG_ADDR, 1);
    const char* pCurrentState="";
    const char* pState="";
    const char* pAlertType="error";
    if (reg>=0) {
      if (reg & DIRECT_BUS_ACTIVATE) {
	if (reg & SELECTMAP_ACTIVATE) {
	  pCurrentState="selectmap";
	  iResult=state==eSelectMap;
	} else {
	  pCurrentState="flash";
	  iResult=state==eFlash;
	}
      } else {
	pCurrentState="msgbuffer";
	iResult=state==eMsgBuffer;
      }
      if (iResult==0 && bAlert>0 && (g_verbosity>0 || bAlert>2)) {
	switch (bAlert) {
	case 1: pAlertType="info"; break;
	case 2: pAlertType="warning"; break;
	case 3: pAlertType="error"; break;
	}
	switch (state) {
	case eMsgBuffer: pState="msgbuffer"; break;
	case eSelectMap: pState="selectmap"; break;
	case eFlash: pState="flash"; break;
	default:
	  pState="unknown";
	}
	fprintf(stderr, "%s: interface in wrong state \'%s\' (required \'%s\')\n", pAlertType, pCurrentState, pState);
      }
    }
  }
  return iResult;
}

/* read the content of the MRB into a buffer
 * internal function
 * parameter:
 *    pTargetBuffer - pointer to buffer to receive the data
 *    iTargetBufferSize - size of this buffer
 *    iStartOffset - start offset for the MRB
 * result: number of bytes copied to buffer
 *    <0 in case of error
 */
int readResultBuffer(unsigned char* pTargetBuffer, int iTargetBufferSize, int iStartOffset)
{
  int iResult=0;
  if ((g_dcscFlags&DCSC_INIT_ENCODE)==0) {
    if (pTargetBuffer && iTargetBufferSize>0) {
      if (iStartOffset<message_out_buffer_size) {
	if (seek_dcsc(message_out_buffer_offset+iStartOffset, 0)>=0){
	  if ((iResult=read_dcsc(pTargetBuffer, 1, iTargetBufferSize))!=iTargetBufferSize){
	    fprintf(stderr,"readMessageBuffer: failed to read message buffer\n");
	    iResult=-EIO;
	  }
	} else {
	  fprintf(stderr,"readMessageBuffer: seek to message buffer failed\n");
	  iResult=-ESPIPE;
	}
      } else {
	fprintf(stderr,"readMessageBuffer: start offset exceeds size of message buffer\n");
	iResult=-EINVAL;
      }
    }
  } else {
    // nothing to do, 0 bytes copied
  }
  return iResult;
}

/* translate the result from the status word and print it human readable
 * internal function
 */
int getErrorFromStatus(__u32 u32Status)
{
  int iResult=0;
  if (u32Status&MSGBUF_STATUS_NOMARKER) {
    if (g_options&PRINT_RESULT_HUMAN_READABLE) fprintf(stderr,"marker not found\n");
    iResult=-EMISS_MARKER;
  }
  if (u32Status&MSGBUF_STATUS_NOENDMRK) {
    if (g_options&PRINT_RESULT_HUMAN_READABLE)   fprintf(stderr,"no target answer\n");
    iResult=-ENOTARGET;
  }
  if (u32Status&MSGBUF_STATUS_NOTGTASW) {
    if (g_options&PRINT_RESULT_HUMAN_READABLE)   fprintf(stderr,"no bus grant\n");
    iResult=-ENOBUSGRANT;
  }
  if (u32Status&MSGBUF_STATUS_NOBUSGRT) {
    if (g_options&PRINT_RESULT_HUMAN_READABLE)   fprintf(stderr,"end marker not found\n");
    iResult=-EMISS_ENDMARKER;
  }
  if (u32Status&MSGBUF_STATUS_OLDFORMAT) {
    if (g_options&PRINT_RESULT_HUMAN_READABLE)   fprintf(stderr,"message buffer format prior to v2\n");
    iResult=-EMISS_ENDMARKER;
  }
  return iResult;
}

/* read the first two words in result buffer to get the status
 * internal function
 * translate the status human readable if option is on 
 * return: number of data words in the result buffer in case of success
 *    <0 in case of error
 */
int getCmdResult(__u32 u32CmdWord, char* pMessage)
{
  int iNofWords=0;
  __u32 data[2];
  if ((g_dcscFlags&DCSC_INIT_ENCODE)==0) {
    if (readResultBuffer((unsigned char*)&data, 2*sizeof(__u32), 0)>=0) {
      if (g_options&PRINT_RESULT_BUFFER) {
	printBufferHex((unsigned char*)&data, 2*sizeof(__u32), 4, "result buffer");
      }
      // TODO 06.01.2006: this check is obviously a bug
      if ((data[0]&0xc000)==(u32CmdWord&0xc00)) { // cross check of the command
	if (data[1]==0) { // all bits in status word cleare if no error
	  iNofWords=(data[0]&0xffff);
	  if (iNofWords<2) {
	    fprintf(stderr,"hardware error, invalid return size\n");
	    iNofWords=-EIO;
	  } else if (iNofWords>message_out_buffer_size/sizeof(__u32)) {
	    fprintf(stderr,"hardware error, returned size exceeds size of the buffer\n");
	    iNofWords=-EIO;
	  } else
	    iNofWords-=2;
	} else {
	  iNofWords=getErrorFromStatus(data[1]);
	}
      } else {
	if (pMessage) fprintf(stderr,"%s ", pMessage);
	fprintf(stderr,"hardware error: invalid result, the original command id must be returned in the result\n");
      }
    } else {
      if (pMessage) fprintf(stderr,"%s: ", pMessage);
      fprintf(stderr,"can't get result status\n");
      iNofWords=-EIO;
    }
  } else {
    // nothing to do, no data words in the result buffer
  }
  return iNofWords;
}

/* internal function for debugging purpose
 * read back the content of the MIB and compare it to the originally written buffer
 * the option can be turned on by the CHECK_COMMAND_BUFFER flag, a message is printed to std output
 * and the operation is terminated in case of missmatch, to continue the flag IGNORE_BUFFER_CHECK
 * can be set
 * to read back the buffer first an internal multiplexer in the dcs board firmware has to be switched
 * by setting the BUFFER_REREAD bit in the GENERAL_CTRL_REG
 * return: 0 case of success
 *    <0 in case of error
 */
int checkMsginBuffer(char* pCmdBuffer, int iCmdBufferSize, int iSilent) 
{
  int iResult=0;
  if (pCmdBuffer && iCmdBufferSize>0){
    iResult=setDcscRegisterBit(GENERAL_CTRL_REG_ADDR, BUFFER_REREAD);
    if (seek_dcsc(0, 0)>=0){
      char* pReadBuffer=(char*)malloc(iCmdBufferSize);
      if (pReadBuffer) {
	int iReadBack=read_dcsc(pReadBuffer, 1, iCmdBufferSize);
	if (iReadBack!=iCmdBufferSize){
	  fprintf(stderr,"checkMsginBuffer: failed to read command buffer of size %d from message in buffer, got %d\n", iCmdBufferSize, iReadBack);
	  iResult=-1;
	} else {
	  if ((iResult=memcmp(pCmdBuffer, pReadBuffer, iCmdBufferSize))!=0) {
	    printf("checkMsginBuffer: message in buffer does not match original command buffer\n");
	    printBufferHex((unsigned char*)pReadBuffer, iCmdBufferSize, 4, "reread buffer");
	    iResult=-EFAULT;
	  } else if (iSilent!=1){
	    printf("checkMsginBuffer: command reread and checked\n");
	  }
	}
	free(pReadBuffer);
      } else {
	fprintf(stderr,"checkMsginBuffer: out of memory\n");
	iResult=-ENOMEM;
      }
    } else {
      fprintf(stderr,"checkMsginBuffer: seek failed\n");
      iResult=-1;
    }
    clearDcscRegisterBit(GENERAL_CTRL_REG_ADDR, BUFFER_REREAD);
  } else
    iResult=-EFAULT;
  return iResult;
}

/* write a buffer to the MIB 
 * internal function
 * return: number of data words written
 *    <0 in case of error
 */
int writeToMsgInBuffer(char* pBuffer, int iBufferSize)
{
  int iResult=0;
  if (pBuffer && iBufferSize>0) {
    if (seek_dcsc(0, 0)>=0){
      if ((iResult=write_dcsc(pBuffer, 1, iBufferSize))!=iBufferSize){
	fprintf(stderr,"writeToMsgInBuffer: failed to write buffer of size %d to device\n", iBufferSize);
	iResult=-1;
      } else {
      }
    } else {
      fprintf(stderr,"writeToMsgInBuffer: seek failed\n");
      iResult=-1;
    }
  } else {
    iResult=-EINVAL;
  }
  return iResult;
} 

/* backbone for all rcu access methods, the function writes a fully encoded block to the MIB,
 * reads it back and checks it if desired, sets the COMMAND_EXECUTE flag and waits for the
 * interface to be ready 
 * internal function
 * @param pCmdBuffer
 * @param iCmdBufferSize
 * @param iTimeout  time out in seconds
 * @return: 
 *    -ETIMEDOUT - time out while waiting for the interface
 *    <0 in case of error
 */
int sendRcuCommand(char* pCmdBuffer, int iCmdBufferSize, int iTimeout)
{
  int iResult=0;
  int bSkipTest=(g_options&CHECK_COMMAND_BUFFER)==0; // the MIB reread function shall be skipped
  int bIgnoreTest=(g_options&IGNORE_BUFFER_CHECK)!=0; // the result of the MIB reread shall be ignored
  int i;
  int iDefaultTimeout=2;
  int iSleepPeriod=10;
  if (pCmdBuffer && iCmdBufferSize>0){
    // debug option: print command sequence
    if (g_options&PRINT_COMMAND_BUFFER)
      printBufferHex((unsigned char*)pCmdBuffer, iCmdBufferSize, 4, "command buffer");
    int bFlash=((*((__u32*)pCmdBuffer)>>g_pMsgBuf_header->FRSTWORD_BITSHIFT_MODE)&((0x1<<g_pMsgBuf_header->FRSTWORD_WIDTH_MODE)-1))&MSGBUF_MODE_FLASH;
    if ((bFlash==0 && checkState(eMsgBuffer, 3)>0) || ((bFlash==1 && checkState(eFlash, 3)>0))) {
      // write the command sequence to the message in buffer
      if ((iResult=writeToMsgInBuffer(pCmdBuffer, iCmdBufferSize))>=0){
	if ((g_dcscFlags&DCSC_INIT_ENCODE)==0) {
	  if (bSkipTest || (iResult=checkMsginBuffer(pCmdBuffer, iCmdBufferSize, 0))>=0 || bIgnoreTest==1) {
	    time_t startedTime, currentTime;
	    float fLastDiff=0;
	    time(&startedTime);
	    // set the 'execute' flag to launch interpretation of the command sequence
	    iResult=setDcscRegisterBit(GENERAL_CTRL_REG_ADDR, COMMAND_EXECUTE);
	    if (iResult>=0 ) {
	      // wait for the firmware to clear the 'execute' flag
	      do {
		if ((iResult=readDcscRegister(GENERAL_CTRL_REG_ADDR, 1))>0 && ((iResult&COMMAND_EXECUTE)==0)) {
		  // print the content of the register if termination is not during the first loop, this is just for debugging purpose
		  if (iResult>=0 && fLastDiff>=1.0 &&(g_options&PRINT_REGISTER_ACCESS)==0) {
		    fprintf(stderr, "\n"); // print a terminating newline after the dots
		  }
		  break;
		}
		usleep(iSleepPeriod);
		// check for time out
		time(&currentTime);
		float fDiff=difftime(currentTime, startedTime);
		if (fDiff>=(iTimeout==0?iDefaultTimeout:iTimeout)) {
		  if ((g_options&PRINT_REGISTER_ACCESS)==0 && fLastDiff>=1.0)
		    fprintf(stderr, "\n");
		  iResult=-ETIMEDOUT;
		} else if (fDiff>=fLastDiff+1) {
		  fLastDiff=fDiff;
		  fprintf(stderr, ".");
		}
	      } while (iResult>=0);
	    }
	  } else {
	    fprintf(stderr,"sendRcuCommand: command aborted\n");
	  }
	}
      }
    } else {
      iResult=-EBADFD;
    }
  } else
    iResult=-EFAULT;
  return iResult;
}

/******************************************************************************************************
 * extended interface methods to support the flash access via the msg buffer interface
 */

/* interface method function, refer to dcscMsgBufferInterface.h for details
 * 
 */
int rcuSingleWriteExt(__u32 address, __u32 data, unsigned short mode){
  int iResult=0;
  if (pMib && mibSize>=6) {
  pMib[0] = mkFrstWrd(2, 0, 0, mode, SINGLE_WRITE);
  pMib[1] = address;
  pMib[2] = data;
  pMib[3] = mkLstWrd(0);
  pMib[4] = mkEndMarker();
  lock_device();
  iResult=sendRcuCommand((unsigned char*)pMib, 5*4, 0);
  if(iResult==-ETIMEDOUT){
    iResult=-1;
    fprintf(stderr,"rcuSingleWrite: time out while waiting for ready signal\n");
  }
  else{
    if(iResult>=0){
      iResult=getCmdResult(*(unsigned char*)pMib, "rcuSingleWrite");
    }
  }
  unlock_device();
  } else {
    iResult=-EBADFD;
  }
  return iResult;
}

/* interface method function, refer to dcscMsgBufferInterface.h for details
 * 
 */
int rcuMultipleWriteExt(__u32 address, __u32* pData, int iSize, int iDataMode, unsigned short mode){
  int iResult=0;
  if (pMib && mibSize>=6) {
  if (pData==NULL) {
    fprintf(stderr,"rcuMultipleWrite: invalid parameter\n");
    return -EINVAL;
  }

  /* there was a not so smart first approach of the support of compressed data by the firmware,
   * unfortunately we chose a 'big endian' approach for the representation of the compressed
   * data within the msg buffer interface. Very unlucky, considered the little endian nature
   * of the arm linux. The format was thats why adopted, 
   * by the following switch: 1 indicates the big endian format, -1 the little endian
   */
  int switchBigEndianConversion=-1;

  // the number of words which fit into one 32 bit data word
  int iCompFactor=1;
  int iDataSize=iDataMode<0?-iDataMode:iDataMode;
  //fprintf(stderr,"rcuMultipleWriteExt: data mode %d data size %d\n", iDataMode, iDataSize); 
  switch (iDataSize) {
  case 1: iCompFactor=4; break;
  case 2: iCompFactor=2; break;
  case 3: iCompFactor=3; break;
  case 4: iCompFactor=1; break;
  default:
    fprintf(stderr,"rcuMultipleWrite: invalid dataSize\n");
    return -EINVAL;
  }
  if (!g_bCompression) iCompFactor=1; // always 1 word if compression is disabled

  if (iDataMode<0 && iCompFactor==3) {
    fprintf(stderr,"rcuMultipleWrite: swap is not supported for 10  bit compressed data\n");
    return -EINVAL;
  }

  if (iDataMode==-1) {
    fprintf(stderr,"rcuMultipleWrite warning: data swap has no effect to 8bit data\n");
  }

  __u32* arrayCmdWords=pMib+1;
  // calculate the maximum possible number of data words
  int iMaxDataWordsPerBlock=message_in_buffer_size/sizeof(__u32)-5;
  iMaxDataWordsPerBlock*=iCompFactor;
  // calculate the number of command blocks to be executed subsequently to write all the data
  int iNofBlocks=(iSize-1)/iMaxDataWordsPerBlock+1;
  int iBlockNo=0;
  if (g_options&PRINT_SPLIT_DEBUG)
    fprintf(stderr,"rcuMultipleWrite: write %d block(s) of maximum size %d with compression factor %d\n", iNofBlocks, iMaxDataWordsPerBlock, iCompFactor);
  // write loop
  for (;iBlockNo<iNofBlocks && iResult>=0; iBlockNo++) {
    // write the address to the first word of the block 
    arrayCmdWords[0]=address+iBlockNo*iMaxDataWordsPerBlock;
    // determine number of words to copy, calculate number of remaining data words if it is the last block
    // and write it to the second word of the block
    if (iBlockNo==iNofBlocks-1)
    arrayCmdWords[1]=iSize-iBlockNo*iMaxDataWordsPerBlock;
    else
    arrayCmdWords[1]=iMaxDataWordsPerBlock;

    // size of the payload in the MIB between 'count' and 'marker'
    int iPayloadSize=arrayCmdWords[1]/iCompFactor;

    if (g_bCompression==0 && iDataSize<4) {
      if (iDataSize==3) {
	fprintf(stderr, "rcuMultipleWrite: 10-bit data format not supported when hardware compression is disabled\n");
	iResult=-ENOSYS;
      } else {
      // special handling for 8 and 16 bit data, write the data to the block, beginning with an
      // offset of 2 (these two are occupied by address and number, refer to the command format)
      int i=0;
      for (i=0; i<arrayCmdWords[1]; i++) {
	int lsbOffset=0;
	int msbOffset=1;
	if (iDataMode==-2) {
	  lsbOffset=1;
	  msbOffset=0;
	}
        // write lsbs of the data
        arrayCmdWords[i+2]=0; 
        arrayCmdWords[i+2]=*(((char*)pData)+(iBlockNo*iMaxDataWordsPerBlock+i)*iDataSize+lsbOffset);
        if (iDataSize==2) {
          // write msbs for 16bit data  
          arrayCmdWords[i+2]+=(*(((char*)pData)+(iBlockNo*iMaxDataWordsPerBlock+i)*iDataSize+msbOffset))<<8;
        }
      }
/*      // check if there were stucking bits in other bytes of the block
      for (i=0; i<arrayCmdWords[1]; i++) {
        __u32 testPattern=0xffff0000;
        if (iDataSize==1)
        testPattern=0xffffff00;
        if ((arrayCmdWords[i+2]&(testPattern))!=0) {
          fprintf(stderr,"rcuMultipleWrite: error in data array\n");
          iResult=-EILSEQ;
          break;
        }
      }*/
      }
    } else {
      // just copy the data
      int iBulkCopy=iPayloadSize; // bulk copy 32 bit words
      int iRemCopy=arrayCmdWords[1]%iCompFactor;  // remaining bytes
      __u32* pSrc=pData+iBlockNo*iMaxDataWordsPerBlock/iCompFactor;
      // due to the 'big endian' format of the msg buffer 8bit data has to swapped always completely
      // as well as 16 bit and 32 bit words with 'swap' option
      if (iDataMode<1.5*switchBigEndianConversion) {
	//fprintf(stderr, "swapping bytes\n");
	swab(pSrc, &arrayCmdWords[2], iBulkCopy*sizeof(__u32));
      } else {
	memcpy (&arrayCmdWords[2], pSrc, iBulkCopy*sizeof(__u32));
      }
      if (iRemCopy!=0) {
	iPayloadSize++;
	if (iDataSize!=1) iRemCopy++; // for 16 and 10 bit data one byte more to copy
	memcpy(&arrayCmdWords[2+iBulkCopy], pSrc+iBulkCopy, iRemCopy);
	memset(((void*)&arrayCmdWords[2+iBulkCopy])+iRemCopy, 0, sizeof(__u32)-iRemCopy);
      }
    }

    if ((g_bCompression==1 && iDataMode<3*switchBigEndianConversion) || iDataMode==-4) {
      // additional swap of the 16 bit words applies always to 32 bit data in 
      // swap mode and to 16/8 bit compressed data
      //fprintf(stderr, "swapping 16bit words\n");
      __u16 tmp=0;
      __u16* pLsb=(__u16*)&arrayCmdWords[2];
      __u16* pMsb=((__u16*)&arrayCmdWords[2]); pMsb++;
      int i=0;
      for (i=0; i<iPayloadSize; i++) {
	//fprintf(stderr, "%#x <-> %#x\n", *pLsb, *pMsb);
	tmp=*pLsb;
	*pLsb=*pMsb; pLsb+=2;
	*pMsb=tmp; pMsb+=2;
      }
    }
    if (g_options&PRINT_SPLIT_DEBUG)
    fprintf(stderr,"rcuMultipleWrite: write block no %d, size=%d, address=%#x\n", iBlockNo, arrayCmdWords[1], arrayCmdWords[0]);
    if (iResult>=0) {
      pMib[0] = mkFrstWrd(iPayloadSize+2, 0, iCompFactor-1, mode, MULTI_WRITE);
      pMib[iPayloadSize+3] = mkLstWrd(0);
      pMib[iPayloadSize+4] = mkEndMarker();
      lock_device();
      iResult=sendRcuCommand((unsigned char*)pMib, (iPayloadSize+5)*4,0);
      if(iResult==-ETIMEDOUT){
        iResult=-1;
        fprintf(stderr,"rcuMutipleWrite: time out while waiting for ready signal\n");
      }
      else{
        if(iResult>=0){
          iResult=getCmdResult(*(unsigned char*)pMib, "rcuMultipleWrite");
        }
      }
      unlock_device();
    }
  }
  } else {
    iResult=-EBADFD;
  }
  return iResult;
}      

/* interface method function, refer to dcscMsgBufferInterface.h for details
 * 
 */
int rcuSingleReadExt(__u32 address, __u32* pData, unsigned short mode)
{
  int iResult=0;
  if (pMib && mibSize>=6) {
  pMib[0]=mkFrstWrd(1, 0, 0, mode, SINGLE_READ);
  pMib[1]=address;
  pMib[2]=mkLstWrd(0);
  pMib[3]=mkEndMarker();
  lock_device();
  iResult=sendRcuCommand((unsigned char*)pMib, 4*4, 0);
  if(iResult==-ETIMEDOUT){
    iResult=-1;
    fprintf(stderr,"rcuSingleRead: time out while waiting for ready signal\n");
  }
  else{
    if(iResult>=0) {
      if((iResult=getCmdResult(*(unsigned char*)pMib, "rcuSingleRead"))>0 && pData){
	if((readResultBuffer((unsigned char*)pData, sizeof(__u32), 2*sizeof(__u32)))>=0){
	  if(g_options&PRINT_RESULT_BUFFER)
	    printBufferHex((unsigned char*)pData, sizeof(__u32), 4, "data buffer");
	} else {
	  if(pData){
	    fprintf(stderr,"rcuSingleRead: failed to read data from result buffer\n");
	    iResult=-EIO;
	  } else {
	    iResult=0;
	  }
	}
      }
    }
  }
  unlock_device();
  } else {
    iResult=-EBADFD;
  }
  return iResult;
}

/* interface method function, refer to dcscMsgBufferInterface.h for details
 * 
 */
int rcuMultipleReadExt(__u32 address, int iSize, __u32* pData, unsigned short mode)
{
  int iResult=0;
  int iRead=0;
  if (pMib && mibSize>=6) {
//  char* pCmdBuffer=NULL;

  // calculate the maximum possible number of data words, two words at the beginning are result and status
  int iMaxDataWordsPerBlock=message_out_buffer_size/sizeof(__u32)-2;

  // calculate the number of blocks to be read subsequentially
  int iNofBlocks=(iSize-1)/iMaxDataWordsPerBlock+1;
  int iBlockNo=0;
  __u32* arrayCmdWords=pMib+1;
//  int iCmdBufferSize=0;
  if (g_options&PRINT_SPLIT_DEBUG)
    fprintf(stderr,"rcuMultipleRead: read %d block(s) of maximum size %d\n", iNofBlocks, iMaxDataWordsPerBlock);

  // read loop
  for (;iBlockNo<iNofBlocks && iResult>=0; iBlockNo++) {
    // write start address of the current block to first word of command buffer 
    arrayCmdWords[0]=address+iBlockNo*iMaxDataWordsPerBlock;

    // determine number of words to read, calculate number of remaining data words if it is the last block
    // and write it to the second word of the command buffer
    if (iBlockNo==iNofBlocks-1)
      arrayCmdWords[1]=iSize-iBlockNo*iMaxDataWordsPerBlock;
    else
      arrayCmdWords[1]=iMaxDataWordsPerBlock;

    if (g_options&PRINT_SPLIT_DEBUG)
      fprintf(stderr,"rcuMultipleRead: read block no %d, size=%d, address=%#x\n", iBlockNo, arrayCmdWords[1], arrayCmdWords[0]);

    // allocate and encode the command buffer
//    iCmdBufferSize=wrapBlock(MULTI_READ, 0, 4, arrayCmdWords, &pCmdBuffer);
    pMib[0]=mkFrstWrd(2, 0, 0, mode, MULTI_READ);
    pMib[3]=mkLstWrd(0);
    pMib[4]=mkEndMarker();
    lock_device();
    iResult=sendRcuCommand((unsigned char*)pMib, 5*4, 0);
    if(iResult==-ETIMEDOUT){
      fprintf(stderr,"rcuMultipleRead: time out while waiting for ready signal\n");
    } else {
      if(iResult>=0) {
	if((iResult=getCmdResult(*(unsigned char*)pMib, "rcuMultipleRead"))>0 && pData){
	  if(readResultBuffer((unsigned char*)(pData+iBlockNo*iMaxDataWordsPerBlock), iResult*sizeof(__u32), 2*sizeof(__u32))>=0){
	    iRead+=iResult;
	    if(g_options&PRINT_RESULT_BUFFER)
	      printBufferHex((unsigned char*)(pData+iBlockNo*iMaxDataWordsPerBlock), iResult*sizeof(__u32), 4, "data buffer");
	  } else {
	    if(pData){
	      fprintf(stderr,"rcuMultipleRead: failed to read from result buffer\n");
	      iResult=-EIO;
	    } else {
	      iResult=0;
	    }
	  }
	}
      }
    }
    unlock_device();
  }
  } else {
    iResult=-EBADFD;
  }
  if (iResult>=0) return iRead;
  return iResult;
}

/******************************************************************************************************
 * the interface methods
 */

/* interface method function, refer to dcscMsgBufferInterface.h for details
 * 
 */
int rcuSingleWrite(__u32 address, __u32 data){
  int iResult=rcuSingleWriteExt(address, data, MSGBUF_MODE_MEMMAPPED);
  return iResult;
}

/* interface method function, refer to dcscMsgBufferInterface.h for details
 * 
 */
int rcuMultipleWrite(__u32 address, __u32* pData, int iSize, int iDataSize){
  int iResult=rcuMultipleWriteExt(address, pData, iSize, iDataSize, MSGBUF_MODE_MEMMAPPED);
  return iResult;
}      

/* interface method function, refer to dcscMsgBufferInterface.h for details
 * 
 */
int rcuSingleRead(__u32 address, __u32* pData)
{
  int iResult=rcuSingleReadExt(address, pData, MSGBUF_MODE_MEMMAPPED);
  return iResult;
}

/* interface method function, refer to dcscMsgBufferInterface.h for details
 * 
 */
int rcuMultipleRead(__u32 address, int iSize, __u32* pData)
{
  int iResult=rcuMultipleReadExt(address, iSize, pData, MSGBUF_MODE_MEMMAPPED);
  return iResult;
}

/* interface method function, refer to dcscMsgBufferInterface.h for details
 * not yet implemented
 */
int dcscProvideMessageBuffer(__u32** ppBuffer, int* pSize)
{
  int iResult=-ENOSYS;

  //*ppBuffer=pMib;
  //Note that mibSize is __u32, pSize is int. Must be fixed.
  //*pSize=mibSize;
  //iResult=1;

  /*
 - this will be the basic buffer access function
   bit is also a method of the interface to be used for low level access  
   i.e. raw write of fully prepared command blocks
 - introduce a global command and result buffer
   2 global pointers and 2 global variables for the size
 - allocation during init, destruction during interface release
 - the function can basically clear the buffer and returns the pointer to 
   the buffer and its size in the two variables
  */
  return iResult;
}

/* interface method function, refer to dcscMsgBufferInterface.h for details
 * not yet implemented
 */
int dcscPrepareMessageBuffer(__u32** ppBuffer, int* pSize, unsigned int cmdID, unsigned int flags)
{
  int iResult=-ENOSYS;
  /*
  - call dcscProvideMessageBuffer function to get the buffer for raw write
  - place the command ID in the information word, all other parts of the information word
    must be added before execution
  - markers will also be added before execution
  - return pointer to first command word location, i.e. start + sizeof(__u32)
  */
  return iResult;
}

/* interface method function, refer to dcscMsgBufferInterface.h for details
 * not yet implemented
 */
int dcscExecuteCommand(__u32* pBuffer, int iSize, __u32** ppTarget, int* ppTargetSize, unsigned int operation)
{
  int iResult=-ENOSYS;
  /*
  - cross check the buffer, must either be the start or start + sizeof(__u32)
  - complete the information word and add the markers
  - execute the command, as a first approach the sendRcuCommand can be called as it is, later it has to be changed to
    use the mmap feature
  - read the status word
  - read the result buffer and return pointer to it and size 

  - sketch for later extension: the function should be able to handle an arbitrary buffer which 
    does not neccessarily has to be part of the internal buffer
    raw command write for command sequences of arbitrary length will be supported, if the whole
    sequence exceeds the size of the MIB the sequence must be splitted
    (additional end markers must be introduced and the block count in the information word must
    be adapted)
   */
  return iResult;
}

__u32* getPBuffer(){
  return pBuffer;
}

__u32* getPMib(){
  return pMib;
}

__u32* getPMrb(){
  return pMrb;
}

__u32* getPReg(){
  return pReg;
}

__u32 getBufferSize(){
  return bufferSize;
}

__u32 getMibSize(){
  return mibSize;
}

__u32 getMrbSize(){
  return mrbSize;
}

__u32 getRegSize(){
  return regSize;
}

int setDebugOptions(int options)
{
  g_options=options;
  return g_options;
}

int setDebugOptionFlag(int of)
{
  g_options|=of;
  return g_options;
}

int clearDebugOptionFlag(int of)
{
  g_options&=~of;
  return g_options;
}

/******************************************************************************************************
 * register response simulation for debugging purpose
 */

#ifdef DCSC_SIMULATION
/* read the register value from a file instead of the real real register
 * internal functions for debugging purpose
 * a sequence of values can be stored in a text file in the format: <access number> <value>
 * the number of access calls to a register is counted, the return value is that
 * value from the file with the highest access number smaller than the current count, e.g.
 * 1 0x01
 * 2 0x04
 * 8 0x01
 * will return 0x01 for the 1st access, 0x02 for the 2nd and all the subsequent calls up the 
 * the 7th, from the 8th access and higher up 0x01 will be returned
 * parameter:
 *    regNo - the register address
 */
int readDcscRegisterFromFile(int regNo)
{
  int iResult=0;

  // this is again the 8/32 bit problem mentioned above
  int no=regNo/4; // because of 32 bit data space the regNo is the register number times 4
  if (no<g_nofreg) {
    g_iRegCount[no]++;
    if (g_simregfile[no]) {
      char strInput[100];
      if (g_count[no]<g_iRegCount[no]) {
	if (g_options&PRINT_REGISTER_ACCESS) {
	  fprintf(stderr, "read register simulation from file: ");
	}
	if (fgets(strInput, 100, g_simregfile[no])) {
	  int iLen=strlen(strInput);
	  if (iLen>=2 && strInput[iLen-1]=='\n') strInput[iLen-1]=0;
	  if (g_options&PRINT_REGISTER_ACCESS) {
	    fprintf(stderr,"%s \n", strInput);
	  }
	  if (sscanf(strInput, " %d 0x%x", &g_count[no], &g_value[no])==2) {
	    if (g_iRegCount[no]>=g_count[no]) {
	      iResult=g_value[no];
	      g_oldval[no]=g_value[no];
	    } else
	      iResult=g_oldval[no];
	  } else {
	    fprintf(stderr, "wrong format in register file\n");
	  }
	} else {
	  if (g_options&PRINT_REGISTER_ACCESS) {
	    fprintf(stderr, "no more lines in register file\n");
	  }
	}
	if (g_options&PRINT_REGISTER_ACCESS) {
	  fflush(stderr);
	}
      } else if (g_count[no]==g_iRegCount[no]) {
	iResult=g_value[no];
      } else {
	iResult=g_oldval[no];
      }
    } else {
      iResult=-1;
    }
  } else {
    fprintf(stderr, "readDcscRegisterFromFile: invalid register no (%d)\n", regNo);
    iResult=-1;
  }
  return iResult;
}
#endif //DCSC_SIMULATION

/*
 * interface method for starting the simulation, refer to the dcscMsgBufferInterface.h file for details
 */
void resetSimulation()
{
#ifdef DCSC_SIMULATION
    int i=0;
    for (i=0; i<g_nofreg; i++) {
      if (g_simregfile[i])
	fseek(g_simregfile[i], 0, SEEK_SET);
      g_iRegCount[i]=0;
      g_count[i]=0;
      g_value[i]=0;
      g_oldval[i]=0;
    }
#endif //DCSC_SIMULATION
}

/*
 * interface method for starting the simulation, refer to the dcscMsgBufferInterface.h file for details
 */
void startSimulation()
{
#ifdef DCSC_SIMULATION
    fprintf(stderr, "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
    fprintf(stderr, "this is a test version which simulates register response\n");
    fprintf(stderr, "register response is read from files \'simulation.reg<no>\'\n");
    int i=0;
    char filename[50];
    for (i=0; i<g_nofreg; i++) {
      sprintf(filename, "simulation.reg%d", i);
      g_simregfile[i]=fopen(filename, "r");
      if (g_simregfile[i])
	fprintf(stderr, ">>> open register file \'%s\'\n", filename);
    }
    resetSimulation();
#endif //DCSC_SIMULATION
}

/*
 * interface method for ending the simulation, refer to the dcscMsgBufferInterface.h file for details
 */
void stopSimulation()
{
#ifdef DCSC_SIMULATION
  int i=0;
  for (i=0; i<g_nofreg; i++) {
    if (g_simregfile[i])
      fclose(g_simregfile[i]);
    g_simregfile[i]=NULL;
  }
#endif //DCSC_SIMULATION
}


/* firmware version number readout
 */
int getDCSCFirmwareVersionNo(int iVerbosity)
{
  int iResult=0;
  __u32 version=0;
  if (DCSC_FIRMWARE_VERSION_ADDRESS>0) {
  if ((iResult=rcuSingleRead(DCSC_FIRMWARE_VERSION_ADDRESS, &version))>=0) {
    int major=(version&((0x1<<DCSC_FIRMWARE_VERSION_MAJOR_SIZE)-1))>>(DCSC_FIRMWARE_VERSION_MAJOR_BITSHIFT);
    int minor=(version&((0x1<<DCSC_FIRMWARE_VERSION_MINOR_SIZE)-1))>>(DCSC_FIRMWARE_VERSION_MINOR_BITSHIFT);
    int year =(version&((0x1<<DCSC_FIRMWARE_VERSION_YEAR_SIZE)-1))>>(DCSC_FIRMWARE_VERSION_YEAR_BITSHIFT);
    int month=(version&((0x1<<DCSC_FIRMWARE_VERSION_MONTH_SIZE)-1))>>(DCSC_FIRMWARE_VERSION_MONTH_BITSHIFT);
    int day  =(version&((0x1<<DCSC_FIRMWARE_VERSION_DAY_SIZE)-1))>>(DCSC_FIRMWARE_VERSION_DAY_BITSHIFT);
    if (iVerbosity>=0) {
      if (version==0)
	fprintf(stderr, "no dcscard firmware version tagged, prior to 2.0 (May 2005)\n");
      else
	fprintf(stderr, "dcscard firmware version %d.%d (%d-%d-%d)\n", major, minor, year, month, day);
    }
  } else if (iVerbosity>0) {
    fprintf(stderr, "can not read dcscard firmware (%#x)\n", DCSC_FIRMWARE_VERSION_ADDRESS);
  }
  }
  return iResult;
}

void printDriverInfo(int iVerbosity)
{
  if (g_file) {
    int iDriverVersion=0;
    char* tgtBuffer=NULL;
    int iMaxLen=0;
    iMaxLen=ioctl(g_file, IOCTL_GET_VERS_STR_SIZE, 0);
    if (iMaxLen>0) tgtBuffer = (char*) malloc(iMaxLen);
    // the ioctl ids have been changed in driver version 0.3, the new ids are in the ranges formerly
    // not used, older drivers then 0.3 return 0
    if ((iDriverVersion=ioctl(g_file, IOCTL_GET_VERSION, tgtBuffer))>=0) {
      //printf("\tusing device %s with driver version %d.%d\n", g_pDeviceName, iDriverVersion >> 16, iDriverVersion&0x00ff);
      if (iDriverVersion==0) {
	// try to get the version number the old way
	iDriverVersion=ioctl(g_file, IOCTL_GET_VERSION_V02, 0);
      }
      if (iDriverVersion<((DCSC_REQUIRED_DRIVER_VERSION_MAJOR<<RCUBUS_DRIVER_VERSION_MAJOR_BITSHIFT)|(DCSC_REQUIRED_DRIVER_VERSION_MINOR<<RCUBUS_DRIVER_VERSION_MINOR_BITSHIFT))) {
	fprintf(stderr,"\n***********************************************************************\n");
	fprintf(stderr,"warning: this interface version requires driver versions higher than %d.%d,\n", DCSC_REQUIRED_DRIVER_VERSION_MAJOR, DCSC_REQUIRED_DRIVER_VERSION_MINOR);
      }
      if (iDriverVersion>=3 && tgtBuffer!=NULL && strlen(tgtBuffer)>0)
	fprintf(stderr,"current driver version %s\n", tgtBuffer);
      else if (iDriverVersion>0)
	fprintf(stderr,"currently driver version %d.%d\n", (iDriverVersion>>RCUBUS_DRIVER_VERSION_MAJOR_BITSHIFT)&((0x1<<RCUBUS_DRIVER_VERSION_MAJOR_SIZE)-1), (iDriverVersion>>RCUBUS_DRIVER_VERSION_MINOR_BITSHIFT)&((0x1<<RCUBUS_DRIVER_VERSION_MINOR_SIZE)-1));
      else
	fprintf(stderr,"the driver currently active seems to be older\n");
      if (iVerbosity>0) {
	fprintf(stderr,"\tmessage in buffer size %#x\n", message_in_buffer_size);
	fprintf(stderr,"\tmessage out buffer size %#x\n", message_out_buffer_size);
	fprintf(stderr,"\tmessage register buffer size %#x\n", message_regfile_size);
      }
      //if (iVerbosity>0)
      getDCSCFirmwareVersionNo(iVerbosity);
    } else if (iVerbosity>0)
      fprintf(stderr,"can not fetch driver info\n");
    if (tgtBuffer) free(tgtBuffer);
    tgtBuffer=NULL;
  }
}

int dcscCheckMsgBlock(__u32* pBuffer, int iSize, int iVerbosity) {
  int iResult=0;
  if (pBuffer!=NULL && iSize>0) {
    struct msgbuf_header_t *pFormat=&g_msgbuf_header_v2;
    __u32* pCurrent=pBuffer;
    int iBlockNo=0;
    int iNofWords=0;
    do {
      iNofWords=*pCurrent&(((0x1<<pFormat->FRSTWORD_WIDTH_NUMWORDS)-1)<<pFormat->FRSTWORD_BITSHIFT_NUMWORDS);
      iNofWords=iNofWords>>pFormat->FRSTWORD_BITSHIFT_NUMWORDS;
      iNofWords-=pFormat->FRSTWORD_ADD_NUMWORDS;
      iBlockNo=*pCurrent&(((0x1<<pFormat->FRSTWORD_WIDTH_BLKNUM)-1)<<pFormat->FRSTWORD_BITSHIFT_BLKNUM);
      iBlockNo=iBlockNo>>pFormat->FRSTWORD_BITSHIFT_BLKNUM;
      if (iVerbosity>1)
	fprintf(stderr, "check message buffer block: block %d, size %d\n", iBlockNo, iNofWords);
      pCurrent+=iNofWords+1;
      if (iVerbosity>1) {
	fprintf(stderr, "check message buffer block: check marker at offset %d: %#x\n", ((int)pCurrent-(int)pBuffer)/4, *pCurrent);
      }
      if (((*pCurrent&(((0x1<<pFormat->MARKER_WIDTH)-1)<<pFormat->MARKER_BITSHIFT))>>pFormat->MARKER_BITSHIFT)!=pFormat->MARKER_PATTERN) {
	iResult=0;
	if (iVerbosity>0)
	  fprintf(stderr, "check message buffer block: block %d, marker not found\n", iBlockNo);
	break;
      }
      pCurrent++;
      iResult+=iNofWords+2;
      if (iVerbosity>1)
	fprintf(stderr, "check message buffer block: block %d, size %d, checked\n", iBlockNo, iNofWords);
    } while (iResult>0 && iBlockNo>0);
    if (iResult>0) {
      if (iVerbosity>1) {
	fprintf(stderr, "check message buffer block: check end marker at offset %d: %#x\n", ((int)pCurrent-(int)pBuffer)/4, *pCurrent);
      }
      if (((*pCurrent&(((0x1<<pFormat->ENDMARKER_WIDTH)-1)<<pFormat->ENDMARKER_BITSHIFT))>>pFormat->ENDMARKER_BITSHIFT)!=pFormat->ENDMARKER_PATTERN) {
	iResult=0;
	if (iVerbosity>0)
	  fprintf(stderr, "check message buffer block: end marker not found\n");
      } else {
	iResult++;
      }
    }
  } else {
    iResult=-EINVAL;
  }
  return iResult;
}

/************************************************************************************************************
 * bus control functions
 */
int rcuFlashReset();
int rcuFlashID();

int rcuBusControlCmdExt(int iCmd, int iVerbosity) {
  int iResult=0;
  int verbosity=g_verbosity<iVerbosity?g_verbosity:iVerbosity;
  iResult=readDcscRegister(GENERAL_CTRL_REG_ADDR, 1);
  if (iResult>=0) {
    switch (iCmd) {
    case eEnableSelectmap:
      lock_device();
      if (checkState(eSelectMap, 0)==0) {
	if (checkState(eMsgBuffer, 3)) {
	  writeDcscRegister(GENERAL_CTRL_REG_ADDR, iResult|(DIRECT_BUS_ACTIVATE|SELECTMAP_ACTIVATE));
	  if (((iResult=readDcscRegister(GENERAL_CTRL_REG_ADDR, 1))&(DIRECT_BUS_ACTIVATE|SELECTMAP_ACTIVATE))!=(DIRECT_BUS_ACTIVATE|SELECTMAP_ACTIVATE)) {
	    fprintf(stderr, "rcuBusControlCmd (%s) error: can not activate selectmap mode\n",__FILE__);
	    iResult=-EIO;
	  } else if (verbosity>0) {
	    fprintf(stderr, "-> setting interface to mode \'selectmap\'\n",__FILE__);
	  }
	} else {
	  iResult=-EBADFD;
	}
      } else if (verbosity>0) {
	fprintf(stderr, "interface already in mode \'select map\'\n");
      }
      unlock_device();
      break;
    case eDisableSelectmap:
      lock_device();
      if (checkState(eSelectMap, 0)) {
	writeDcscRegister(GENERAL_CTRL_REG_ADDR, iResult&~(DIRECT_BUS_ACTIVATE|SELECTMAP_ACTIVATE));
	if (((iResult=readDcscRegister(GENERAL_CTRL_REG_ADDR, 1))&(DIRECT_BUS_ACTIVATE|SELECTMAP_ACTIVATE))!=0) {
	  fprintf(stderr, "rcuBusControlCmd (%s) error: can not deactivate selectmap mode\n",__FILE__);
	  iResult=-EIO;
	} else if (verbosity>0) {
	  fprintf(stderr, "-> setting interface to mode \'msgbuffer\'\n",__FILE__);
	}
      }
      unlock_device();
      break;
    case eEnableFlash:
      lock_device();
      if (checkState(eFlash, 0)==0) {
	if (checkState(eMsgBuffer, 3)) {
	  writeDcscRegister(GENERAL_CTRL_REG_ADDR, iResult|DIRECT_BUS_ACTIVATE);
	  if (((iResult=readDcscRegister(GENERAL_CTRL_REG_ADDR, 1))&DIRECT_BUS_ACTIVATE)!=DIRECT_BUS_ACTIVATE) {
	    fprintf(stderr, "rcuBusControlCmd (%s) error: can not activate flash mode\n",__FILE__);
	    iResult=-EIO;
	  } else if (verbosity>0) {
	    fprintf(stderr, "-> setting interface to mode \'flash\'\n",__FILE__);
	  }
	} else {
	  iResult=-EBADFD;
	}
      } else if (verbosity>0) {
	fprintf(stderr, "interface already in mode \'flash\'\n");
      }
      unlock_device();
      break;
    case eDisableFlash:
      lock_device();
      if (checkState(eFlash, 0)) {
	writeDcscRegister(GENERAL_CTRL_REG_ADDR, iResult&~DIRECT_BUS_ACTIVATE);
	if (((iResult=readDcscRegister(GENERAL_CTRL_REG_ADDR, 1))&DIRECT_BUS_ACTIVATE)!=0) {
	  fprintf(stderr, "rcuBusControlCmd (%s) error: can not deactivate flash mode\n",__FILE__);
	  iResult=-EIO;
	} else if (verbosity>0) {
	  fprintf(stderr, "-> setting interface to mode \'msgbuffer\'\n",__FILE__);
	}
      }
      unlock_device();
      break;
    case eEnableMsgBuf:
      lock_device();
      writeDcscRegister(GENERAL_CTRL_REG_ADDR, iResult&~(DIRECT_BUS_ACTIVATE|SELECTMAP_ACTIVATE));
      if (((iResult=readDcscRegister(GENERAL_CTRL_REG_ADDR, 1))&(SELECTMAP_ACTIVATE|DIRECT_BUS_ACTIVATE))!=0) {
	fprintf(stderr, "rcuBusControlCmd (%s) error: can not activate msgbuffer mode\n",__FILE__);
	iResult=-EIO;
      }
      unlock_device();
      break;
    case eResetFirmware:
      writeDcscRegister(GENERAL_CTRL_REG_ADDR, iResult|FIRMWARE_RESET);
      usleep(5);
      writeDcscRegister(GENERAL_CTRL_REG_ADDR, iResult&~FIRMWARE_RESET);
      break;
    case eReadCtrlReg:
      // nothing to do
      break;
    case eCheckSelectmap:
      iResult=checkState(eSelectMap, 0);
      break;
    case eCheckFlash:
      iResult=checkState(eFlash, 0);
      break;
    case eCheckMsgBuf:
      iResult=checkState(eMsgBuffer, 0);
      break;
    case eResetFlash:       // send reset command to flash
      iResult=rcuFlashReset();
      break;
    case eFlashID:           // read the flash id
      iResult=rcuFlashID();
      break;
    case eFlashCtrlDCS:
      if (g_iFirmwareVersion>=FLASH_DCS_ACCESS_REQUIRED_FW_VERSION) {
	g_iFlashAccessMode=eFlashAccessDcsc;
	if (g_verbosity>0)
	  fprintf(stdout, "switching to Flash access via DCS board firmware\n");fflush(stdout);
      } else {
	g_iFlashAccessMode=eFlashAccessActel;
	fprintf(stderr, "firmware version 2.2 or higher required for Flash access via DCS board firmware\n");
      }
      break;
    case eFlashCtrlActel:
      g_iFlashAccessMode=eFlashAccessActel;
      if (g_verbosity>0)
	fprintf(stdout, "switching to Flash access via Actel firmware\n");fflush(stdout);
      break;
    case eEnableCompression:
      if (g_iFirmwareVersion>=FLASH_DCS_ACCESS_REQUIRED_FW_VERSION) {
	g_bCompression=1;
	if (g_verbosity>0)
	  fprintf(stdout, "enable use of compressed data words\n");fflush(stdout);
      } else {
	g_bCompression=0;
	fprintf(stderr, "firmware version 2.2 or higher required for support of compressed data formats\n");
      }
      break;
    case eDisableCompression:
      g_bCompression=0;
      if (g_verbosity>0)
	fprintf(stdout, "disable use of compressed data words\n");fflush(stdout);
      break;
    default:
      fprintf(stderr, "rcuBusControlCmd (%s): unknown command %d\n", __FILE__, iCmd);
      iResult=-EINVAL;
    }
  }
  return iResult;
}

int rcuBusControlCmd(int iCmd) 
{
  return rcuBusControlCmdExt(iCmd, 2);
}

int msgBufReadRegister(int reg)
{
  int iResult=0;
  //fprintf(stderr, "msgBufReadRegister %d\n", reg);
  iResult=readDcscRegister(GENERAL_CTRL_REG_ADDR+reg, 1);
  return iResult;
}

int msgBufWriteRegister(int reg, unsigned char value)
{
  int iResult=0;
  //fprintf(stderr, "msgBufWriteRegister %d %#x\n", reg, value);
  iResult=writeDcscRegister(GENERAL_CTRL_REG_ADDR+reg, value);
  return iResult;
}

/************************************************************************************************************
 * flash control functions
 */

#define RCU_FLASH_SIZE    0x1000000 // 22 bit
#define RCU_FLASH_CMD     0xb000  // command register
#define RCU_FLASH_STATUS  0xb100  // status register
#define RCU_FLASH_ADDRH   0xb101  // address lines 16 to 21
#define RCU_FLASH_AHMASK  0x0ff0000  // msb mask
#define RCU_FLASH_AHBSHFT 16         // msb bitshift
#define RCU_FLASH_ADDRL   0xb102  // address lines 0 to 15
#define RCU_FLASH_ALMASK  0x000ffff  // lsb mask
#define RCU_FLASH_DATA    0xb103  // data lines
#define RCU_FLASH_READRES 0xb105  // read data

#define RCU_FLASH_ERASEALL  0x02  // erase the whole flash  
#define RCU_FLASH_WRITECMD  0x04  // execute flash write  
#define RCU_FLASH_READCMD   0x08  // execute flash read  
#define RCU_FLASH_ERASESEC  0x20  // erase sector, sector address specified in the upper 10 bits of the address range
                                  // that means in both registers
#define RCU_FLASH_RESETCMD  0x10  // reset command
#define RCU_FLASH_IDCMD     0x01  // read id code. switches to the special mode

#define RCU_FLASH_ID_HADDR  0x0
#define RCU_FLASH_ID_LADDR  0x1   // the id code is located in the address 0x1

#define RCU_FLASH_NOF_SECTORS 1024// TODO fix that number!

#define RCU_FLASH_STATE_IDLE 0x0
#define RCU_FLASH_STATE_BUSY 0x4000
#define RCU_FLASH_STATE_MASK 0xffff

#define RCU_FLASH_DEFAULT_WAITCYCLE         5 // micro sec
#define RCU_FLASH_ERASE_WAITCYCLE     1000000 // micro sec
#define RCU_FLASH_MAX_WAITSTATES           60

int rcuFlashWrite(__u32 address, __u32* pData, int iSize, int iDataSize)
{
  int iResult=0;
  if (pData && iSize>0) {
    lock_device();
    /* check the mode */
    if (checkState(eFlash, 3)) {
      if (address+iSize<RCU_FLASH_SIZE) {
	if (g_iFlashAccessMode==eFlashAccessDcsc) {
	  // dcs board fw 2.2 or higher support direct flash access
	  iResult=rcuMultipleWriteExt(address, pData, iSize, iDataSize, MSGBUF_MODE_FLASH);
	} else {
	  // older firmware: flash access via actel
	  /* since this version exploits the msg buffer to write to the flash, switch the mode due to that */
	  if ((iResult=rcuBusControlCmdExt(eDisableFlash, 0))>=0) {
	    __u32 flashstate=0xffff;
	    if ((iResult=rcuSingleRead(RCU_FLASH_STATUS, &flashstate))>=0 && (flashstate&RCU_FLASH_STATE_MASK)==RCU_FLASH_STATE_IDLE) {
	    int i=0;
	    __u32 aw=address;
	    __u32 dw=0;
	    int newline=0;
	    for (i=0; i<iSize && iResult>=0; i++, aw++) {
	      /* write the address */
	      if ((iResult=rcuSingleWrite(RCU_FLASH_ADDRL, (aw&RCU_FLASH_ALMASK)))>=0 &&
		  (iResult=rcuSingleWrite(RCU_FLASH_ADDRH, ((aw&RCU_FLASH_AHMASK)>>RCU_FLASH_AHBSHFT)))>=0) {
		if (iDataSize==1)
		  dw=*(((char*)pData)+i);
		else if (iDataSize==2)
		  dw=*(((__u16*)pData)+i);
		else if (iDataSize==4)
		  dw=*(pData+i);
		else {
		  fprintf(stderr, "rcuFlashWrite error: can not handle word size of %d\n", iDataSize);
		  iResult=-EINVAL;
		  break;
		}
		/* write the data word */
		if ((iResult=rcuSingleWrite(RCU_FLASH_DATA, dw))>=0) {
		  /* issue the write command */
		  if ((iResult=rcuSingleWrite(RCU_FLASH_CMD, RCU_FLASH_WRITECMD))>=0) {
		    int waitstates=0;
		    /* wait for the status to get idle */
		    for (;(iResult=rcuSingleRead(RCU_FLASH_STATUS, &flashstate))>=0 && waitstates<RCU_FLASH_MAX_WAITSTATES && (flashstate&RCU_FLASH_STATE_MASK)!=RCU_FLASH_STATE_IDLE; waitstates++) {
		      usleep(RCU_FLASH_DEFAULT_WAITCYCLE);
		    }
		    if (waitstates>=RCU_FLASH_MAX_WAITSTATES && (flashstate&RCU_FLASH_STATE_MASK)!=RCU_FLASH_STATE_IDLE) {
		      fprintf(stderr, "rcuFlashWrite error: time out while waiting for flash state IDLE, aborting write cycle %d after %d us \n", i, waitstates*RCU_FLASH_DEFAULT_WAITCYCLE);
		      iResult=-ETIMEDOUT;
		    }
		  } else {
		    fprintf(stderr, "rcuFlashWrite error: failed to issue write command (%d), cycle %d of %d \n", iResult, i, iSize);
		  }
		} else {
		  fprintf(stderr, "rcuFlashWrite error: can not write data word (%d), cycle %d of %d \n", iResult, i, iSize);
		}
	      } else {
		fprintf(stderr, "rcuFlashWrite error: can not write address word (%d), cycle %d of %d \n", iResult, i, iSize);
	      }
	      if (((i+1)%500)==0 && g_verbosity>0) {
		newline=1;
		fprintf(stderr, "."); fflush(stderr);
	      }
	    }
	    if (newline) fprintf(stderr, "\n");
	    } else if (iResult>=0) {
	      fprintf(stderr, "rcuFlashWrite error: flash is busy (state %#x)\n", flashstate);
	    } else  {
	      fprintf(stderr, "rcuFlashWrite error: can not read flash state\n");
	    }
	    /* set mode back to eFlash */
	    if ((rcuBusControlCmdExt(eEnableFlash, 0))<0) {
	      fprintf(stderr, "rcuFlashWrite error: can not set correct state\n");
	      iResult=-EIO;
	    }
	  } else {
	    fprintf(stderr, "rcuFlashWrite error: can not enable access\n");
	  }
	}
      } else  {
	fprintf(stderr, "rcuFlashWrite error: write attemp exceeds flash address range (address=%#x count=%d flash size %#x \n", address, iSize, RCU_FLASH_SIZE);
      }
    } else {
      iResult=-EACCES;
    }
    unlock_device();
  } else {
    fprintf(stderr, "rcuFlashWrite error: invalid parameter\n");
    iResult=-EINVAL;
  }
  return iResult;
}


int rcuFlashRead(__u32 address, int iSize,__u32* pData)
{
  int iResult=0;
  if (pData && iSize>0) {
    /* this was for testing without hardware
    int i=0;
    for (i=0; i<iSize; i++) pData[i]=0xffff0000|(i<<10)+address+1;
    return iSize;
    */
    lock_device();
    /* enable the flash */
    /* check the mode */
    if (checkState(eFlash, 3)) {
      if (address+iSize<RCU_FLASH_SIZE) {
	if (g_iFlashAccessMode==eFlashAccessDcsc) {
	  if (g_verbosity>1)
	    fprintf(stderr, "using flash access via dcs firmware\n");
	  // dcs board fw 2.2 or higher support direct flash access
	  iResult=rcuMultipleReadExt(address, iSize, pData, MSGBUF_MODE_FLASH);
	} else {
	  // older firmware: flash access via actel
	  /* since this version exploits the msg buffer to write to the flash, switch the mode due to that */
	  if ((iResult=rcuBusControlCmdExt(eDisableFlash, 0))>=0) {
	    if (g_verbosity>1)
	      fprintf(stderr, "using flash access via actel firmware\n");
	    __u32 flashstate=0xffff;
	    if ((iResult=rcuSingleRead(RCU_FLASH_STATUS, &flashstate))>=0 && (flashstate&RCU_FLASH_STATE_MASK)==RCU_FLASH_STATE_IDLE) {
	    int i=0;
	    __u32 aw=address;
	    __u32 dw=0;
	    int newline=0;
	    for (i=0; i<iSize && iResult>=0; i++, aw++) {
	      /* write the address */
	      if ((iResult=rcuSingleWrite(RCU_FLASH_ADDRL, (aw&RCU_FLASH_ALMASK)))>=0 &&
		  (iResult=rcuSingleWrite(RCU_FLASH_ADDRH, ((aw&RCU_FLASH_AHMASK)>>RCU_FLASH_AHBSHFT)))>=0) {
		/* issue the read command */
		if ((iResult=rcuSingleWrite(RCU_FLASH_CMD, RCU_FLASH_READCMD))>=0) {
		  int waitstates=0;
		  /* wait for the status to get idle */
		  for (;(iResult=rcuSingleRead(RCU_FLASH_STATUS, &flashstate))>=0 && waitstates<RCU_FLASH_MAX_WAITSTATES && (flashstate&RCU_FLASH_STATE_MASK)!=RCU_FLASH_STATE_IDLE; waitstates++) {
		    usleep(RCU_FLASH_DEFAULT_WAITCYCLE);
		  }
		  if (waitstates>=RCU_FLASH_MAX_WAITSTATES && (flashstate&RCU_FLASH_STATE_MASK)!=RCU_FLASH_STATE_IDLE) {
		    fprintf(stderr, "rcuFlashRead error: time out while waiting for flash state IDLE, aborting read cycle %d after %d us \n", i, waitstates*RCU_FLASH_DEFAULT_WAITCYCLE);
		    iResult=-ETIMEDOUT;
		  } else {
		    iResult=rcuSingleRead(RCU_FLASH_READRES, pData+i);
		  }
		} else {
		  fprintf(stderr, "rcuFlashRead error: failed to issue read command (%d), cycle %d of %d \n", iResult, i, iSize);
		}
	      } else {
		fprintf(stderr, "rcuFlashRead error: can not write address word (%d), cycle %d of %d \n", iResult, i, iSize);
	      }
	      if (((i+1)%500)==0 && g_verbosity>0) {
		newline=1;
		fprintf(stderr, "."); fflush(stderr);
	      }
	    }
	    if (newline) fprintf(stderr, "\n");
	    } else if (iResult>=0) {
	      fprintf(stderr, "rcuFlashRead error: flash is busy (state %#x)\n", flashstate);
	    } else  {
	      fprintf(stderr, "rcuFlashRead error: can not read flash state\n");
	    }
	    /* set mode back to eFlash */
	    if ((rcuBusControlCmdExt(eEnableFlash, 0))<0) {
	      fprintf(stderr, "rcuFlashRead error: can not set correct state\n");
	      iResult=-EIO;
	    }
	  } else {
	    fprintf(stderr, "rcuFlashRead error: can not enable access\n");
	  }
	}
      } else  {
	fprintf(stderr, "rcuFlashRead error: read attemp exceeds flash address range (address=%#x count=%d flash size %#x \n", address, iSize, RCU_FLASH_SIZE);
      }
    } else {
      iResult=-EACCES;
    }
    unlock_device();
  } else {
    fprintf(stderr, "rcuFlashRead error: invalid parameter\n");
    iResult=-EINVAL;
  }
  return iResult;
}

int rcuFlashErase(int startSec, int count)
{
  int iResult=0;
  __u32 command=0;
  if (count<=0 && startSec>=0) {
    fprintf(stderr, "warning: ignoring wrong parameter 'count'\n");
    count=1;
  }
  if (startSec<0) {
    command=RCU_FLASH_ERASEALL;
  } else {
    command=RCU_FLASH_ERASESEC;
  }
  if (command>0 && iResult>=0) {
    lock_device();
    /* check the mode */
    if (checkState(eFlash, 3)) {
      if (g_iFlashAccessMode==eFlashAccessDcsc) {
	// dcs board fw 2.2 or higher support direct flash access
	if (pMib && mibSize>=6) {
	  lock_device();
	  int k=0; 
	  int iTimeOut=2;
	  if (command==RCU_FLASH_ERASEALL) {
	    iTimeOut=45;
	    if (g_verbosity>0)
	      fprintf(stdout, "erase rcu flash memory \n");fflush(stdout);
	    pMib[k++]=mkFrstWrd(0, 0, 0, MSGBUF_MODE_FLASH, FLASH_ERASEALL);
	  } else {
	    if (count==1) pMib[k++]=mkFrstWrd(1, 0, 0, MSGBUF_MODE_FLASH, FLASH_ERASE_SEC);
	    else pMib[k++]=mkFrstWrd(2, 0, 0, MSGBUF_MODE_FLASH, FLASH_MULTI_ERASE);
	    pMib[k++]=startSec;
	    if (count>1) {
	      iTimeOut*=count;
	      pMib[k++]=count;
	    }
	    if (g_verbosity>0)
	      fprintf(stdout, "erase rcu flash memory: %d sector(s) from address %#x \n", count, startSec);fflush(stdout);
	  }
	  pMib[k++]=mkLstWrd(0);
	  pMib[k++]=mkEndMarker();
	  int iResult=sendRcuCommand((unsigned char*)pMib, k*4, iTimeOut);
	  if(iResult==-ETIMEDOUT){
	    iResult=-1;
	    fprintf(stderr,"rcuFlashErase: time out while waiting for ready signal\n");
	  }
	  unlock_device();
	} else {
	  fprintf(stderr, "rcuFlashErase error: interface not initialized\n");
	}
      } else {
	// older firmware: flash access via actel
      /* since this version exploits the msg buffer to write to the flash, switch the mode due to that */
      if ((iResult=rcuBusControlCmdExt(eDisableFlash, 0))>=0) {
	__u32 flashstate=0xffff;
	if ((iResult=rcuSingleRead(RCU_FLASH_STATUS, &flashstate))>=0 && (flashstate&RCU_FLASH_STATE_MASK)==RCU_FLASH_STATE_IDLE) {
	  /* issue the eraseall command */
	    if (command==RCU_FLASH_ERASEALL) {
	      if (g_verbosity>0)
		fprintf(stdout, "erase rcu flash memory .");fflush(stdout);
	    } else {
	      if (g_verbosity>0)
		fprintf(stdout, "erase rcu flash memory sector %#x .", startSec);fflush(stdout);
	      /* write the address */
	      /* sector no is specified in the upper 10 bit of the address range , i.e. bit 14 and 15 of the lower and */
	      /* bit 0 to 7 ( 16 to 21) in the higher address register */
	      if ((iResult=rcuSingleWrite(RCU_FLASH_ADDRL, (startSec<<12)&0xc000))>=0 &&
		  (iResult=rcuSingleWrite(RCU_FLASH_ADDRH, (startSec>>4)))>=0) {
	      } else {
		fprintf(stderr, "rcuFlashErase error: can not write sector no to address register (%d)\n", iResult);
	      }
	    }
	    if (iResult>=0) {
	      if ((iResult=rcuSingleWrite(RCU_FLASH_CMD, command))>=0) {
		int waitstates=0;
		/* wait for the status to get idle */
		for (;(iResult=rcuSingleRead(RCU_FLASH_STATUS, &flashstate))>=0 && waitstates<RCU_FLASH_MAX_WAITSTATES && (flashstate&RCU_FLASH_STATE_MASK)!=RCU_FLASH_STATE_IDLE; waitstates++) {
		  if ((waitstates%4)==0) fprintf(stdout, ".");fflush(stdout);
		  usleep(RCU_FLASH_ERASE_WAITCYCLE);
		}
		if (waitstates>=RCU_FLASH_MAX_WAITSTATES && (flashstate&RCU_FLASH_STATE_MASK)!=RCU_FLASH_STATE_IDLE) {
		  fprintf(stderr, "rcuFlashErase error: time out while waiting for flash state IDLE, leaving loop after %d us \n", waitstates*RCU_FLASH_ERASE_WAITCYCLE);
		  iResult=-ETIMEDOUT;
		} else {
		  fprintf(stdout, "... done\n");fflush(stdout);
		}
		if (g_verbosity>0 && count>1)
		  fprintf(stderr, "warning: multi erase is not available with the Actel interface, sector %#x successfully erased\n", startSec);
	      } else {
		fprintf(stderr, "rcuFlashErase error: failed to issue erase command (%d)\n", iResult);
	      }
	    }
	    /* set mode back to eFlash */
	    if ((iResult=rcuBusControlCmdExt(eEnableFlash, 0))<0) {
	      fprintf(stderr, "rcuFlashErase error: can not set correct state\n");
	    }
	} else if (iResult>=0) {
	  fprintf(stderr, "rcuFlashErase error: flash is busy (state %#x)\n", flashstate);
	} else  {
	  fprintf(stderr, "rcuFlashErase error: can not read flash state\n");
	}
	/* set mode back to eFlash */
	if ((rcuBusControlCmdExt(eEnableFlash, 0))<0) {
	  fprintf(stderr, "rcuFlashErase error: can not set correct state\n");
	  iResult=-EIO;
	}
      } else {
	fprintf(stderr, "rcuFlashErase error: can not enable access\n");
      }
      }
    } else {
      iResult=-EACCES;
    }
    lock_device();
  }
  return iResult;
}

int rcuFlashID()
{
  int iResult=0;
  __u32 data=0;
  lock_device();
  /* check the mode */
  if (checkState(eFlash, 3)) {
    if (g_iFlashAccessMode==eFlashAccessDcsc) {
      // dcs board fw 2.2 or higher support direct flash access
      if (pMib && mibSize>=6) {
	lock_device();
	int k=0; 
	pMib[k++]=mkFrstWrd(1, 0, 0, MSGBUF_MODE_FLASH, FLASH_READID);
	pMib[k++]=1;
	pMib[k++]=mkLstWrd(0);
	pMib[k++]=mkEndMarker();
	int iResult=sendRcuCommand((unsigned char*)pMib, k*4, 0);
	if(iResult==-ETIMEDOUT){
	  iResult=-1;
	  fprintf(stderr,"rcuFlashId: time out while waiting for ready signal\n");
	}
	else{
	  if(iResult>=0) {
	    if((iResult=getCmdResult(*(unsigned char*)pMib, "rcuFlashID"))>0){
	      __u32 id=0;
	      if((readResultBuffer((unsigned char*)&id, sizeof(__u32), 1*sizeof(__u32)))>=0){
		iResult=id;
		if(g_options&PRINT_RESULT_BUFFER)
		  printBufferHex((unsigned char*)&id, sizeof(__u32), 4, "data buffer"); 
	      } else {
		fprintf(stderr,"rcuSingleRead: failed to read data from result buffer\n");
		iResult=-EIO;
	      }
	    }
	  }
	}
	unlock_device();
      } else {
	fprintf(stderr, "rcuFlashId error: interface not initialized\n");
      }
    } else {
      // older firmware: flash access via actel
    /* since this version exploits the msg buffer to write to the flash, switch the mode due to that */
    if ((iResult=rcuBusControlCmdExt(eDisableFlash, 0))>=0) {
      __u32 flashstate=0xffff;
      if ((iResult=rcuSingleRead(RCU_FLASH_STATUS, &flashstate))>=0 && (flashstate&RCU_FLASH_STATE_MASK)==RCU_FLASH_STATE_IDLE) {
	/* write the address */
	if ((iResult=rcuSingleWrite(RCU_FLASH_ADDRL, RCU_FLASH_ID_LADDR))>=0 &&
	    (iResult=rcuSingleWrite(RCU_FLASH_ADDRH, RCU_FLASH_ID_HADDR)>=0)) {
	  /* issue the id command */
	  if ((iResult=rcuSingleWrite(RCU_FLASH_CMD, RCU_FLASH_IDCMD))>=0) {
	    usleep(10); // wait for to flash to change the mode
	    /* issue the read command */
	    if ((iResult=rcuSingleWrite(RCU_FLASH_CMD, RCU_FLASH_READCMD))>=0) {
	      int waitstates=0;
	      /* wait for the status to get idle */
	      for (;(iResult=rcuSingleRead(RCU_FLASH_STATUS, &flashstate))>=0 && waitstates<RCU_FLASH_MAX_WAITSTATES && (flashstate&RCU_FLASH_STATE_MASK)!=RCU_FLASH_STATE_IDLE; waitstates++) {
		usleep(RCU_FLASH_DEFAULT_WAITCYCLE);
	      }
	      if (waitstates>=RCU_FLASH_MAX_WAITSTATES && (flashstate&RCU_FLASH_STATE_MASK)!=RCU_FLASH_STATE_IDLE) {
		fprintf(stderr, "rcuFlashId error: time out while waiting for flash state IDLE (%d us), can not read flash id\n", waitstates*RCU_FLASH_DEFAULT_WAITCYCLE);
		iResult=-ETIMEDOUT;
	      } else {
		iResult=rcuSingleRead(RCU_FLASH_READRES, &data);
	      }
	    } else {
	      fprintf(stderr, "rcuFlashId error: failed to issue read command (%d)\n", iResult);
	    }
	  } else {
	    fprintf(stderr, "rcuFlashId error: failed to issue id command (%d)\n", iResult);
	  }
	} else {
	  fprintf(stderr, "rcuFlashId error: can not write address word (%d)\n", iResult);
	}
      } else if (iResult>=0) {
	fprintf(stderr, "rcuFlashId error: flash is busy (state %#x)\n", flashstate);
      } else  {
	fprintf(stderr, "rcuFlashId error: can not read flash state\n");
      }
      /* set mode back to eFlash */
      if ((rcuBusControlCmdExt(eEnableFlash, 0))<0) {
	fprintf(stderr, "rcuFlashId error: can not set correct state\n");
	iResult=-EIO;
      }
    } else {
      fprintf(stderr, "rcuFlashId error: can not enable access\n");
    }
    }
    if (iResult>=0) {
      iResult=data;
      if (g_verbosity>0) {
	fprintf(stdout, "flash id: %#x\n", data);
      }
    }
    rcuFlashReset();
  } else {
    iResult=-EACCES;
  }
  unlock_device();
  return iResult;
}

int rcuFlashReset()
{
  int iResult=0;
  lock_device();
  /* check the mode */
  if (checkState(eFlash, 3)) {
    if (g_iFlashAccessMode==eFlashAccessDcsc) {
      // dcs board fw 2.2 or higher support direct flash access
      if (pMib && mibSize>=6) {
	lock_device();
	int k=0; 
	pMib[k++]=mkFrstWrd(0, 0, 0, MSGBUF_MODE_FLASH, FLASH_RESET);
	pMib[k++]=mkLstWrd(0);
	pMib[k++]=mkEndMarker();
	int iResult=sendRcuCommand((unsigned char*)pMib, k*4, 0);
	if(iResult==-ETIMEDOUT){
	  iResult=-1;
	  fprintf(stderr,"rcuFlashReset: time out while waiting for ready signal\n");
	}
	unlock_device();
      } else {
	fprintf(stderr, "rcuFlashReset error: interface not initialized\n");
      }
    } else {
      // older firmware: flash access via actel
    /* since this version exploits the msg buffer to write to the flash, switch the mode due to that */
    if ((iResult=rcuBusControlCmdExt(eDisableFlash, 0))>=0) {
      __u32 flashstate=0xffff;
      if ((iResult=rcuSingleRead(RCU_FLASH_STATUS, &flashstate))>=0 && (flashstate&RCU_FLASH_STATE_MASK)==RCU_FLASH_STATE_IDLE) {
	/* issue the reset command */
	if ((iResult=rcuSingleWrite(RCU_FLASH_CMD, RCU_FLASH_RESETCMD))>=0) {
	  int waitstates=0;
	  /* wait for the status to get idle */
	  for (;(iResult=rcuSingleRead(RCU_FLASH_STATUS, &flashstate))>=0 && waitstates<RCU_FLASH_MAX_WAITSTATES && (flashstate&RCU_FLASH_STATE_MASK)!=RCU_FLASH_STATE_IDLE; waitstates++) {
	    usleep(RCU_FLASH_DEFAULT_WAITCYCLE);
	  }
	  if (waitstates>=RCU_FLASH_MAX_WAITSTATES && (flashstate&RCU_FLASH_STATE_MASK)!=RCU_FLASH_STATE_IDLE) {
	    fprintf(stderr, "rcuFlashReset error: time out while waiting for flash state IDLE (%d us), reset might have failed\n", waitstates*RCU_FLASH_DEFAULT_WAITCYCLE);
	    iResult=-ETIMEDOUT;
	  } else {
	    if (g_verbosity>0)
	      fprintf(stderr, "flash reset executed successfully\n");
	  }
	} else {
	  fprintf(stderr, "rcuFlashReset error: failed to issue reset command (%d)\n", iResult);
	}
      } else if (iResult>=0) {
	fprintf(stderr, "rcuFlashReset error: flash is busy (state %#x)\n", flashstate);
      } else  {
	fprintf(stderr, "rcuFlashReset error: can not read flash state\n");
      }
      /* set mode back to eFlash */
      if ((rcuBusControlCmdExt(eEnableFlash, 0))<0) {
	fprintf(stderr, "rcuFlashReset error: can not set correct state\n");
	iResult=-EIO;
      }
    } else {
      fprintf(stderr, "rcuFlashReset error: can not enable access\n");
    }
    }
  } else {
    iResult=-EACCES;
  }
  unlock_device();
  return iResult;
}
