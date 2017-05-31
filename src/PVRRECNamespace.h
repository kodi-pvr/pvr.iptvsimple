#pragma once
/*
 *      Copyright (C) 2015 Radek Kubera
 *      http://github.com/afedchin/xbmc-addon-iptvsimple/
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include <string>
//#include "xbmc_pvr_dll.h"
//#include "libXBMC_pvr.h"
#include "p8-platform/util/util.h"
using namespace std;
using namespace ADDON;

namespace ADDON
{
    typedef enum
    {
        PVR_STREAM_TYPE_RTMP              = 1,
        PVR_STREAM_TYPE_FILE              = 2
    } PVR_STREAM_TYPE;
    
    typedef enum
    {
        PVR_STREAM_NO_STREAM              = 0,
        PVR_STREAM_START_RECORDING        = 1,
        PVR_STREAM_IS_RECORDING           = 2,
        PVR_STREAM_IS_STOPPING            = 3,
        PVR_STREAM_STOPPED                = 4
    } PVR_STREAM_STATUS;
    
    struct PVR_REC_JOB_ENTRY
    {
        PVR_STREAM_STATUS           Status;
        string                      strChannelName;
        PVR_TIMER                   Timer;
        void*                       ThreadPtr;
        int                         toDelete = 0;
    };
}
