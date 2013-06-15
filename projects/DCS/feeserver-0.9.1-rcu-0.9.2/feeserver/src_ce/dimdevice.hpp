// $Id: dimdevice.hpp,v 1.3 2007/05/25 09:59:09 richter Exp $

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

#ifndef __DIM_DEVICE_HPP
#define __DIM_DEVICE_HPP

#include "device.hpp"

struct ceServiceData_t;

/**
 * @class CEDimDevice
 * The class implements a standard handling of state and alarm publishing
 * through DIM channels
 * @ingroup rcu_ce_base
 */
class CEDimDevice  : public CEDevice {
public:
  /** constructor */
  CEDimDevice(std::string name="", int id=-1, CEDevice* pParent=NULL, std::string arguments="");
  /** destructor */
  ~CEDimDevice();

  /** 
   * create a state channel for this state machine.
   * The function will be called at initialisation and registers a DIM
   * channel for the State.
   */
  int CreateStateChannel();

  /** 
   * create an alarm channel for this state machine.
   * The function will be called at initialisation and registers a DIM
   * channel for the Alarm
   */
  int CreateAlarmChannel();

  /**
   * Fetch the next alarm from the alarm queue and update the float location.
   * @param pF           location to update
   * @return             neg. error code if failed 
   */
  int SendNextAlarm(ceServiceData_t* pData);

  /** 
   * Send an alarm.
   * @param alarm        alarm id
   */
  int SendAlarm(int alarm);

  /** the last alarm which was sent */
  int fLastSentAlarm;
};

#endif //__RCU_DEVICE_HPP
