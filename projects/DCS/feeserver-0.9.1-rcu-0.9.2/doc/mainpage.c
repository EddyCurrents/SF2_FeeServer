/*
 * $Id: mainpage.c,v 1.7 2007/08/20 21:52:44 richter Exp $
 *
/************************************************************************
**
** RCU FeeServer project
** Copyright (c) 2005
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
** You should have received a copy of the GNU Lesser General Public
** License along with this library; if not, write to the Free
** Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, 
** MA 02111-1307  USA  
**
*************************************************************************/

/** @file   mainpage.c
    @author Matthias Richter
    @date   
    @brief  Title page documentation. */
/** @mainpage FeeServer

    @section intro Introduction

    The ALICE detector is a dedicated heavy-ion detector currently built
    at the Large Hadron Collider (LHC) at CERN. The Detector Control System
    (DCS) covers the task of controlling, configuring and monitoring the
    detector.

    For the Front-end Electronics (FEE) of a few sub-detectors, a common
    software architecture has been developed, the \b FeeCom software.

    The lowest layer of the hierarchical system is based on a server
    application running on an embedded Linux system in the FEE. This server
    has access to the hardware via the so called ControlEngine (CE).

    <center><img src="pic_FeeCom.png" border="0" alt=""></center>

    All Communication between the layers is based on the DIM protocol, 
    Distributed Information Management System, an open source communication
    framework developed at CERN. It provides a network-transparent
    inter-process communication for distributed and heterogeneous
    environments. TCP/IP over Ethernet is used as transport layer. A common
    library for many different operating systems is provided by the
    framework. DIM implements a client-server relation with service and
    command channels. The FEE and FED APIs are based on DIM channels.

    This package contains a CE originally developed for the TPC, but as
    several detectors use the same FEE and a large fraction of the CE
    functionality is common for different detectors, a common package
    has been released and the basic CE functionality modularized.
    @ref mpg_ce_plusplus interface provides an object oriented framework
    for ControlEngines.

    @section sysregs System requirements

    The FeeServer is intended to run on the <b> DCS board </b>, a custom
    made single board computer developed at KIP/University of Heidelberg,
    Germany. The DCS board is part of the Detector Control System (DCS)
    for ALICE Front-end electronics.

    For debugging purpose, the FeeServer can also run on a normal Linux
    PC, some emulation of the RCU hardware is implemented.

    @section mpg_installation Installation
    See the @ref readme for information on the installation procedure.

    @section mpg_core The FeeServer core

    The core of the FeeServer handles the interface to the DIM framework and
    the FEE API. See @ref feesrv_core.

    @section mpg_ce The Control Engine

    The \em Control Engine is a dedicated part of the FeeServer which handles the
    hardware access. The @ref mpg_ceapi defines the interface between FeeServer
    core and CE. From CE version 0.9 @ref mpg_ce_plusplus interface has been
    introduced, which provides a fully object oriented implementation of the CE.
    
    @subsection mpg_ceapi FeeServer CE API

    This API defines a couple of C functions for initialization, service
    registration logging, etc. See @ref feesrv_ceapi.

    @subsection mpg_cebase Basic functionality of the CE
    The CE as implemented in this specific flavor of the FeeServer comes with
    some common functionality which is totally device independent and could be
    a part of the FeeServer core (@ref rcu_ce_base). Those modules are:
    - @ref rcu_ce_base_services
    - @ref rcu_ce_base_logging
    - @ref rcu_ce_base_prop
    - @ref rcu_ce_base_states

    @subsection mpg_ce_plusplus The CE++ interface

    An object oriented interface for CE's has been introduced in CE version 0.9.
    CE implemetations can simply derive from the ControlEngine base class and
    have to implement a child of CEagent to handle the creation the relevant
    objects.

    A full CE is implemented for the RCU, see RCUControlEngine and
    @ref mpg_rcuce. It can be customized for several detectors.

    A sample implementation is available for TRD, see @ref ce_trd.
    
    @subsection mpg_rcuce The ControlEngine for the RCU
    The ControlEngine for the RCU is a specific device implementation. It
    was originally developed to control a sub set of the TPC Front-end
    electronics consiting of the RCU and attached FECs. The electronics has been
    used also in PHOS and FMD.
    - @ref sec_rcuce
    - @ref rcu_ce_tpc
    - @ref rcu_ce_phos
    - @ref rcu_ce_fmd

    @section mpg_sample_commands Example Commands

    - @ref feesrv_exac
    - @ref rcu_issue

    @section mpg_links Related links on the web

    
    - <a class="el" href="http://web.ift.uib.no/~kjeks/wiki/index.php?title=Detector_Control_System_%28DCS%29_for_ALICE_Front-end_electronics">
          Detector Control System for the ALICE TPC electronics </a> 
    - <a class="el" href="http://ep-ed-alice-tpc.web.cern.ch/ep-ed-alice-tpc/">ALICE TPC electronics pages</a>
    - <a class="el" href="http://www.kip.uni-heidelberg.de/ti/DCS-Board/current/"> DCS board pages</a>
    - <a class="el" href="http://frodo.nt.fh-koeln.de/%7Etkrawuts/dcs.html"> ARM linux on the DCS board</a>
    - <a class="el" href="http://alicedcs.web.cern.ch/AliceDCS/"> ALICE DCS pages</a>
    - <a class="el" href="http://www.cern.ch/dim"> 
          Distributed Information Management System (DIM)</a>
    - <a class="el" href="http://www.ztt.fh-worms.de/en/projects/Alice-FEEControl/index.shtml"> 
          FeeCom Software pages (ZTT Worms)</a>


*/

#error Not for compilation
//
// EOF
//
