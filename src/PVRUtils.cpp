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
#include <stdlib.h>
#include "PVRUtils.h"
#include "PVRIptvData.h"
#include <map>
#include "PVRRecJob.h"
#include "PVRSchedulerThread.h"

using namespace std;
using namespace ADDON;

extern PVRRecJob *p_RecJob;
extern PVRSchedulerThread *p_Scheduler;

extern void CloseRecordingThreads(void)
{
    delete (p_Scheduler);
    XBMC->Log(LOG_NOTICE,"Scheduler deleted");
    p_RecJob->setLock();
    map<int,PVR_REC_JOB_ENTRY> RecJobs = p_RecJob->getEntryData();
    for (map<int,PVR_REC_JOB_ENTRY>::iterator rec=RecJobs.begin(); rec!=RecJobs.end(); ++rec)
    {
        PVR_REC_JOB_ENTRY entry = rec->second;
        if (entry.Status!=PVR_STREAM_NO_STREAM && entry.Status!=PVR_STREAM_STOPPED)
        {
            XBMC->Log(LOG_NOTICE,"Recording is still active, trying to stop: %s",entry.Timer.strTitle);
            entry.Status = PVR_STREAM_IS_STOPPING;
            p_RecJob->updateJobEntry(entry);
            while (entry.Status!=PVR_STREAM_NO_STREAM && entry.Status!=PVR_STREAM_STOPPED)
            {    
                p_RecJob->getJobEntry(entry.Timer.iClientIndex, entry);
                XBMC->Log(LOG_NOTICE,"Stopping recording %s",entry.Timer.strTitle);
            }
        }
    }
    p_RecJob->setUnlock();
    XBMC->Log(LOG_NOTICE,"Stopped all recodrings jobs");
    delete (p_RecJob);
    XBMC->Log(LOG_NOTICE,"RecJob deleted"); 
}

extern int strtoint(string s)
{
    int tmp = 0;
    unsigned int i = 0;
    bool m = false;
    if(s[0] == '-')
    {
        m = true;
        i++;
    }
    for(; i < s.size(); i++)
        tmp = 10 * tmp + s[i] - 48;
    return m ? -tmp : tmp;
}

extern string inttostr (int i)
{
    string tmp;
    tmp = std::to_string(i);
    return tmp;
}

