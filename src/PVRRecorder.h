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
#include "PVRRecJob.h"
#include "PVRSchedulerThread.h"
#include <string>

using namespace ADDON;
using namespace std;

struct RecordingStruct
{
    unsigned int  uintJobId;
    int           uintChannelUid;
    string        strChannelName;
    string        strTitle;
    time_t        startTime;
    time_t        endTime;
    int           state;
    int           iPriority;
    int           iLifetime;
    bool          bIsRepeating;
    time_t        firstDay;
    int           iWeekdays;
    int           iEpgUid;
    unsigned int  iMarginStart;
    unsigned int  iMarginEnd;
    int           iGenreType;
    int           iGenreSubType;
};

class PVRRecorder {
public:
    PVRRecorder (void);
    PVR_ERROR AddTimer (const PVR_TIMER &timer);
    PVR_ERROR DeleteTimer(const PVR_TIMER &timer, bool bForceDelete);
    PVR_ERROR UpdateTimer(const PVR_TIMER &timer);
    PVR_ERROR GetTimers(ADDON_HANDLE handle);
    int GetTimersAmount(void);
};

