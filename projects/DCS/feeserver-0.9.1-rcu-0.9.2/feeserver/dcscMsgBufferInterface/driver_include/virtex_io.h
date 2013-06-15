/************************************************************************
**
**
** This file is property of and copyright by the Experimental Nuclear 
** Physics Group, Dep. of Physics and Technology
** University of Bergen, Norway, 2006
** This file has been written by 
** Stian Skjerveggen <h106353@stud.hib.no>,
** Jon Kristian Bernhardsen <h106384@stud.hib.no>
** Jan Martin Langeland < h107995@stud.hib.no>
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

#ifndef _VIRTEX_IO_H_
#define _VIRTEX_IO_H_

#define VIRTEX_MAGIC_NR 'o' //experimental, could produce conflicts with other ioctls (should be altered later).
#define VIRTEX_READ_REG _IOWR(VIRTEX_MAGIC_NR, 1, unsigned long int)
#define VIRTEX_WRITE_REG _IOWR(VIRTEX_MAGIC_NR, 2, unsigned long int)
#define VIRTEX_SET_REG _IOWR(VIRTEX_MAGIC_NR, 3, unsigned long int)
#define VIRTEX_READ_FRAMES _IOWR(VIRTEX_MAGIC_NR, 4, unsigned long int)
#define VIRTEX_WRITE_WORD _IOWR(VIRTEX_MAGIC_NR, 5, unsigned long int)
#define VIRTEX_READ_WORD _IOWR(VIRTEX_MAGIC_NR, 6, unsigned long int)

extern struct file_operations virtex_fops;
extern int bytes_written;

/* header is used to store a header when writing to register and reading a frame */
static unsigned long int header = 0;

#endif /* _VIRTEX_IO_H */
