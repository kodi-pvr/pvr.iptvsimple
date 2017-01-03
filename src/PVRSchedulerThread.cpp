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
#include "PVRRecorderThread.h"
#include "PVRSchedulerThread.h"
#include "PVRRecJob.h"
#include <time.h>
#include <string>

using namespace ADDON;
using namespace std;

extern PVRRecJob *p_RecJob;
extern PVRSchedulerThread *p_Scheduler;
extern bool p_getTimersTransferFinished;
bool s_triggerTimerUpdate;

PVRSchedulerThread::PVRSchedulerThread(void)
{
    XBMC->Log(LOG_NOTICE,"Creating scheduler thread");
    b_interval = 10;
    isWorking = false;
    b_stop = false;
    b_lastCheck = time(NULL)-b_interval;
    CreateThread();
    s_triggerTimerUpdate = false;
}

PVRSchedulerThread::~PVRSchedulerThread(void)
{
    b_stop = true;
    XBMC->Log(LOG_NOTICE,"Closing scheduler thread");
    while (isWorking==true)
    {
        XBMC->Log(LOG_NOTICE,"Waiting for close scheduler thread");
    }
}
void PVRSchedulerThread::StopThread(bool bWait /*= true*/)
{
    b_stop = true;
}

void *PVRSchedulerThread::Process(void)
{
    XBMC->Log(LOG_NOTICE,"Starting scheduler thread");
    isWorking = true;
    while (true) {
        if (b_stop==true)
        {
            isWorking = false;
            return NULL;
        }
        time_t now = time(NULL);
        if (now>=b_lastCheck+b_interval) {
            //XBMC->Log(LOG_NOTICE,"Checking for scheduled jobs");
            PVRIptvChannel currentChannel;
            bool jobsUpdated = false;
            p_RecJob->setLock();
            
            map<int,PVR_REC_JOB_ENTRY> RecJobs = p_RecJob->getEntryData();
            for (map<int,PVR_REC_JOB_ENTRY>::iterator rec=RecJobs.begin(); rec!=RecJobs.end(); ++rec)
            {  
                if (rec->second.Status==PVR_STREAM_STOPPED)
                {
                    XBMC->Log(LOG_NOTICE,"Try to delete recording %s",rec->second.Timer.strTitle);
                    p_RecJob->delJobEntry(rec->first);
                    s_triggerTimerUpdate = true;
                    //jobsUpdated = true;
                }
                else if (rec->second.Timer.endTime<time(NULL)) {
                    XBMC->Log(LOG_NOTICE,"Try to delete recording %s",rec->second.Timer.strTitle);
                    p_RecJob->delJobEntry(rec->first);
                    s_triggerTimerUpdate = true;
                    //jobsUpdated = true;
                }
                else if ((rec->second.Timer.startTime-10)<=time(NULL) && rec->second.Timer.state == PVR_TIMER_STATE_SCHEDULED)
                {
                    //Start new Recording
                    XBMC->Log(LOG_NOTICE,"Try to start recording %s",rec->second.Timer.strTitle);
                    startRecording(rec->second);
                    s_triggerTimerUpdate = true;
                    //jobsUpdated = true;
                }
                else if (rec->second.Timer.endTime<=time(NULL) && (rec->second.Timer.state==PVR_TIMER_STATE_RECORDING))
                {
                    XBMC->Log(LOG_NOTICE,"Try to stop recording %s",rec->second.Timer.strTitle);
                    //Stop Recording
                    stopRecording(rec->second);
                    s_triggerTimerUpdate = true;
                    //jobsUpdated = true;
                }
            }
            
            p_RecJob->setUnlock();
            if (p_getTimersTransferFinished==true && s_triggerTimerUpdate==true)
            {
                s_triggerTimerUpdate=false;
                try {
                    p_getTimersTransferFinished = false;
                    PVR->TriggerTimerUpdate();
                }catch( std::exception const & e ) {
                    //Closing Kodi, TriggerTimerUpdate is not available
                }
            }
            b_lastCheck=time(NULL);
        }
    }
    return NULL;
}

bool PVRSchedulerThread::startRecording (const PVR_REC_JOB_ENTRY &RecJobEntry)
{
    PVR_REC_JOB_ENTRY entry;
    PVRIptvChannel currentChannel;
    if (p_RecJob->getJobEntry(RecJobEntry.Timer.iClientIndex, entry))
    {
        entry = RecJobEntry;
        if (p_RecJob->getProperlyChannel(entry, currentChannel))
        {
            XBMC->Log(LOG_NOTICE,"Starting recording %s",RecJobEntry.Timer.strTitle);
            entry.Status = PVR_STREAM_START_RECORDING;
            entry.Timer.state = PVR_TIMER_STATE_RECORDING;
            p_RecJob->updateJobEntry(entry);
            PVRRecorderThread* TPtr = new PVRRecorderThread(currentChannel,RecJobEntry.Timer.iClientIndex);
            entry.ThreadPtr = TPtr;
            p_RecJob->updateJobEntry(entry);
            return true;
        }
        else
        {
            //Channel not found, update status
            entry.Status = PVR_STREAM_STOPPED;
            entry.Timer.state = PVR_TIMER_STATE_ERROR;
            p_RecJob->updateJobEntry(entry);
        }
    }
    
    return false;
}

bool PVRSchedulerThread::stopRecording (const PVR_REC_JOB_ENTRY &RecJobEntry)
{
    PVR_REC_JOB_ENTRY entry;
    entry = RecJobEntry;
    if (entry.Timer.state!=PVR_TIMER_STATE_NEW)
    {
        XBMC->Log(LOG_NOTICE,"Stopping recording %s",entry.Timer.strTitle);
        entry.Status = PVR_STREAM_IS_STOPPING;
        p_RecJob->updateJobEntry(entry);
    }
    
    return true;
}
