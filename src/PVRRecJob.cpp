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
#include "p8-platform/threads/threads.h"
#include <map>
#include "PVRIptvData.h"
#include "PVRUtils.h"
#include "PVRRecJob.h"
#include <time.h>
#include <string>
#include <mutex>

using namespace std;
using namespace ADDON;

extern PVRIptvData *m_data;
mutex p_mutex;
int p_RecJob_lock = 0;
#define RECJOB_FILE_NAME "pvrsimplerecorder.cache"

PVRRecJob::PVRRecJob()
{
    loadData();
    storeEntryData();
}

PVRRecJob::~PVRRecJob(void)
{
   m_JobEntryData.clear();
}

bool PVRRecJob::getProperlyChannel (PVR_REC_JOB_ENTRY &entry, PVRIptvChannel &currentChannel)
{
    bool foundChannel = false;
    PVR_CHANNEL channel;
    channel.iUniqueId = entry.Timer.iClientChannelUid;
    if (m_data->GetChannel(channel, currentChannel))
    {
        if (entry.strChannelName==currentChannel.strChannelName)
            foundChannel = true;
    }
    
    if (foundChannel == false)
    {
        if (m_data->GetChannelByName(entry.strChannelName, currentChannel))
            foundChannel = true;
    }
    
    if (foundChannel == true)
    {   
        entry.Timer.iClientChannelUid = currentChannel.iUniqueId;
        return true;
    }
    return false;
}

bool PVRRecJob::loadData(void)
{
    string jobFile = GetUserFilePath(RECJOB_FILE_NAME);
    string strContent;
    void* fileHandle = XBMC->OpenFile(jobFile.c_str(), 0);
    m_JobEntryData.clear();
    if (fileHandle)
    {
        char buffer[1024];
        while (int bytesRead = XBMC->ReadFile(fileHandle, buffer, 1024))
        {
            strContent.append(buffer, bytesRead);
        }
        XBMC->CloseFile(fileHandle);
        vector<string>ContVect = StringUtils::Split (strContent,"\n");
        vector <string>::iterator it ( ContVect.begin() );
        string BuffStr;
        string strContentOut = "";
        for (it=ContVect.begin(); it!=ContVect.end(); it++)
        {
            BuffStr = *it;
            BuffStr = StringUtils::Trim(BuffStr);
            PVR_REC_JOB_ENTRY jobEntry;
            if (ParseJobString(BuffStr, jobEntry))
            {
                if (jobEntry.Timer.state==PVR_TIMER_STATE_NEW || jobEntry.Timer.state==PVR_TIMER_STATE_SCHEDULED)
                {
                    if (jobEntry.Timer.startTime>time(NULL))
                    {
                        jobEntry.Status = PVR_STREAM_NO_STREAM;
                        m_JobEntryData[jobEntry.Timer.iClientIndex] = jobEntry;
                    }
                }
            }
        }
        return true;
    }
    return false;
}

bool PVRRecJob::storeEntryData (void)
{
    string strContent;
    for (std::map<int,PVR_REC_JOB_ENTRY>::iterator it=m_JobEntryData.begin(); it!=m_JobEntryData.end(); ++it)
    {
        strContent.append(GetJobString(it->second));
    }
    string jobFile = GetUserFilePath(RECJOB_FILE_NAME);
    void* writeFileHandle = XBMC->OpenFileForWrite(jobFile.c_str(), 1);
    if (writeFileHandle != NULL)
    {
        XBMC->WriteFile(writeFileHandle, strContent.c_str(), strContent.length());
        XBMC->CloseFile(writeFileHandle);
        return true;
    }
    else
        return false;
    return true;
}

map<int,PVR_REC_JOB_ENTRY> PVRRecJob::getEntryData(void)
{
    return m_JobEntryData;
}

bool PVRRecJob::addJobEntry(const PVR_REC_JOB_ENTRY &RecJobEntry)
{
    if (m_JobEntryData.find(RecJobEntry.Timer.iClientIndex) == m_JobEntryData.end())
    {
        XBMC->Log(LOG_NOTICE,"Add job entry %s",RecJobEntry.Timer.strTitle);
        m_JobEntryData[RecJobEntry.Timer.iClientIndex] = RecJobEntry;
        storeEntryData();
        return true;
    }
    return false;
}
    
bool PVRRecJob::getJobEntry(const int ientryIndex, PVR_REC_JOB_ENTRY &entry)
{
    if (m_JobEntryData.find(ientryIndex) == m_JobEntryData.end()) return false;
    
    entry = m_JobEntryData[ientryIndex];
    return true;
}
    
bool PVRRecJob::getJobEntry(const string strChannelName, PVR_REC_JOB_ENTRY &entry)
{
    for (std::map<int,PVR_REC_JOB_ENTRY>::iterator it=m_JobEntryData.begin(); it!=m_JobEntryData.end(); ++it)
    {
        if (it->second.strChannelName==strChannelName)
        {
            entry = it->second;
            return true;
        }
        return false;
    }
    return false;
}
    
bool PVRRecJob::updateJobEntry(const PVR_REC_JOB_ENTRY &RecJobEntry)
{
    PVR_REC_JOB_ENTRY entry = RecJobEntry;
    if (getJobEntry(RecJobEntry.Timer.iClientIndex ,entry))
    {
        //status flow validation
        switch (entry.Status)
        {
            case PVR_STREAM_NO_STREAM :
                { if (RecJobEntry.Status!=PVR_STREAM_START_RECORDING && RecJobEntry.Status!=PVR_STREAM_NO_STREAM) return false; break; }
            case PVR_STREAM_START_RECORDING :
                { if (RecJobEntry.Status!=PVR_STREAM_IS_RECORDING && RecJobEntry.Status!=PVR_STREAM_START_RECORDING) return false; break; }
            case PVR_STREAM_IS_RECORDING :
                { if (RecJobEntry.Status!=PVR_STREAM_IS_STOPPING && RecJobEntry.Status!=PVR_STREAM_STOPPED && RecJobEntry.Status!=PVR_STREAM_IS_RECORDING) return false; break; }
            case PVR_STREAM_IS_STOPPING :
                { if (RecJobEntry.Status!=PVR_STREAM_STOPPED && RecJobEntry.Status!=PVR_STREAM_IS_STOPPING) return false; break; }
            case PVR_STREAM_STOPPED :
                { if (RecJobEntry.Status!=PVR_STREAM_STOPPED) return false; break; }
        }
        entry = RecJobEntry;
        
        m_JobEntryData[RecJobEntry.Timer.iClientIndex] = entry;
        storeEntryData();
        XBMC->Log(LOG_NOTICE,"Updated job entry %s",RecJobEntry.Timer.strTitle);
        return true; 

    }
    return false;
}
    
bool PVRRecJob::delJobEntry(const int ientryIndex)
{
    PVR_REC_JOB_ENTRY entry;
    if (getJobEntry(ientryIndex ,entry))
    {
        XBMC->Log(LOG_NOTICE,"Deleted job entry %s",entry.Timer.strTitle);
        m_JobEntryData.erase(ientryIndex);
        storeEntryData();
        return true;
    }
    return false;
}

bool PVRRecJob::setLock (void)
{
    p_mutex.lock();
    return true;
/*
    bool ok = false;
    int lock_rand = rand() % 10000000;
    while (ok==false) {
        if (p_RecJob_lock>0 && p_RecJob_lock==lock_rand) return true;
        else {
            while (p_RecJob_lock>0)
            {
                //wait for unlock by other thread
            } 
            p_RecJob_lock = lock_rand;
        }
    }
    return true;
    */
}

void PVRRecJob::setUnlock (void)
{
    p_mutex.unlock();
    //p_RecJob_lock = 0;
}
string PVRRecJob::GetJobString(const PVR_REC_JOB_ENTRY &RecJobEntry )
{
    string ChannelName = RecJobEntry.strChannelName;
    StringUtils::Replace(ChannelName,"|","\\|");
    string TitleName = RecJobEntry.Timer.strTitle;
    StringUtils::Replace(TitleName,"|","\\|");
    string Line = "\""+inttostr(RecJobEntry.Timer.iClientIndex);
    Line = Line+"\"|\""+inttostr(RecJobEntry.Timer.iClientChannelUid);
    Line = Line+"\"|\""+ChannelName;
    Line = Line+"\"|\""+TitleName;
    Line = Line+"\"|\""+inttostr(RecJobEntry.Timer.startTime);
    Line = Line+"\"|\""+inttostr(RecJobEntry.Timer.endTime);
    Line = Line+"\"|\""+inttostr(RecJobEntry.Timer.state);
    Line = Line+"\"|\""+inttostr(RecJobEntry.Timer.iPriority);
    Line = Line+"\"|\""+inttostr(RecJobEntry.Timer.iLifetime);
    //if (RecJobEntry.Timer.bIsRepeating==true)
    //    Line = Line+"\"|\"1";
    //else
        Line = Line+"\"|\"0";
    Line = Line+"\"|\""+inttostr(RecJobEntry.Timer.firstDay);
    Line = Line+"\"|\""+inttostr(RecJobEntry.Timer.iWeekdays);
    Line = Line+"\"|\""+inttostr(RecJobEntry.Timer.iEpgUid);
    Line = Line+"\"|\""+inttostr(RecJobEntry.Timer.iMarginStart);
    Line = Line+"\"|\""+inttostr(RecJobEntry.Timer.iMarginEnd);
    Line = Line+"\"|\""+inttostr(RecJobEntry.Timer.iGenreType);
    Line = Line+"\"|\""+inttostr(RecJobEntry.Timer.iGenreSubType);
    Line = Line+"\"\n";
    return Line;
}

bool PVRRecJob::ParseJobString(string BuffStr, PVR_REC_JOB_ENTRY &RecJobEntry)
{
    BuffStr = StringUtils::Trim(BuffStr);
    if (BuffStr.length()==0)
        return false;
    
    //StringUtils::Trim " from start and EOL
    BuffStr = BuffStr.substr(1,BuffStr.length()-2);
    if (BuffStr.length()==0)
        return false;
        
    vector<string> lineVect = StringUtils::Split (BuffStr,"\"|\"");
    if (lineVect.size()<17)
        return false;

    RecJobEntry.Timer.iClientIndex       = strtoint(lineVect[0]);
    RecJobEntry.Timer.iClientChannelUid  = strtoint(lineVect[1]);
    RecJobEntry.strChannelName  = lineVect[2];
    StringUtils::Replace(RecJobEntry.strChannelName,"\\|","|");
    string strTitle = lineVect[3];
    StringUtils::Replace(strTitle,"\\|","|");
    strncpy(RecJobEntry.Timer.strTitle, strTitle.c_str(), sizeof(RecJobEntry.Timer.strTitle) - 1);
    RecJobEntry.Timer.startTime       = strtoint(lineVect[4]);
    RecJobEntry.Timer.endTime         = strtoint(lineVect[5]);
    RecJobEntry.Timer.state           = (PVR_TIMER_STATE) strtoint(lineVect[6]);
    RecJobEntry.Timer.iPriority       = strtoint(lineVect[7]);
    RecJobEntry.Timer.iLifetime       = strtoint(lineVect[8]);
    //if (strtoint(lineVect[9])==1) RecJobEntry.Timer.bIsRepeating = true; else RecJobEntry.Timer.bIsRepeating = false;
    RecJobEntry.Timer.firstDay        = strtoint(lineVect[10]);
    RecJobEntry.Timer.iWeekdays       = strtoint(lineVect[11]);
    RecJobEntry.Timer.iEpgUid         = strtoint(lineVect[12]);
    RecJobEntry.Timer.iMarginStart    = strtoint(lineVect[13]);
    RecJobEntry.Timer.iMarginEnd      = strtoint(lineVect[14]);
    RecJobEntry.Timer.iGenreType      = strtoint(lineVect[15]);
    RecJobEntry.Timer.iGenreSubType   = strtoint(lineVect[16]);
    return true;
}
