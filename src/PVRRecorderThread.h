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
#include "libexecstream/exec-stream.h"

using namespace ADDON;

class PVRRecorderThread : P8PLATFORM::CThread
{
    public: 
    PVRRecorderThread(PVRIptvChannel &currentChannel,int iClientIndex);
    virtual ~PVRRecorderThread(void);
    virtual void StopThread(bool bWait = true);
    virtual void *Process(void);
    bool isWorking;
    private:
    FILE* t_proc;
    exec_stream_t es;
    double t_duration;
    void CorrectDurationFLVFile (const string &videoFile, const double &duration);
    int t_iClientIndex;
    PVRIptvChannel t_currentChannel;
    time_t t_startRecTime;
};
