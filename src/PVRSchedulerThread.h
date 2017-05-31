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
#include "p8-platform/util/StringUtils.h"
#include <map>
#include "p8-platform/threads/threads.h"
#include "PVRIptvData.h"
#include <string>

using namespace ADDON;
using namespace std;

class PVRSchedulerThread : P8PLATFORM::CThread
{
    public: 
    PVRSchedulerThread(void);
    virtual ~PVRSchedulerThread(void);
    virtual void StopThread(bool bWait = true);
    virtual void *Process(void);
    bool startRecording (const PVR_REC_JOB_ENTRY &RecJobEntry);
    bool stopRecording (const PVR_REC_JOB_ENTRY &RecJobEntry);
    private: 
    bool b_stop;
    bool isWorking;
    time_t b_lastCheck;
    int b_interval;
};
