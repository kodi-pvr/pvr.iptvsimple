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
                    if (jobEntry.Timer.startTime>time(NULL) || jobEntry.Timer.iTimerType!=PVR_TIMER_TYPE_NONE)
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
bool PVRRecJob::rescheduleJobEntry (const PVR_REC_JOB_ENTRY &RecJobEntry)
{
    PVR_REC_JOB_ENTRY entry = RecJobEntry;
    if (getJobEntry(RecJobEntry.Timer.iClientIndex ,entry))
    {
        struct tm *startTime = localtime (&entry.Timer.startTime);
        int oneDayInt = 24*3600;

        bool reScheduled = false;

        //Create DOW schedule map
        map<int,int> daysOfWeek;
        int i,k;
        k = 1;
        for (i=1; i<=7; i++) {
            daysOfWeek[i] = entry.Timer.iWeekdays & k;
            daysOfWeek[i+7] = entry.Timer.iWeekdays & k;
            k = k<<1;
        }

        //Calculate next DOW recording day
        int nextRecDays = 1;
        for (i=startTime->tm_wday+1; i<=14; i++) {
            if (daysOfWeek[i]>0) {
                reScheduled = true;
                break;
            }
            nextRecDays++;
        }

        if (reScheduled==true) {
            entry.Timer.startTime = entry.Timer.startTime+(oneDayInt*nextRecDays);
            entry.Timer.endTime = entry.Timer.endTime+(oneDayInt*nextRecDays);
            entry.Timer.firstDay = entry.Timer.startTime;
            entry.Timer.state = PVR_TIMER_STATE_SCHEDULED;
            entry.Status = PVR_STREAM_NO_STREAM;

            m_JobEntryData[RecJobEntry.Timer.iClientIndex] = entry;
            storeEntryData();
            XBMC->Log(LOG_NOTICE,"Rescheduled job entry %s",RecJobEntry.Timer.strTitle);
            return true;
        }
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
}

void PVRRecJob::setUnlock (void)
{
    p_mutex.unlock();
}

string PVRRecJob::GetJobString(const PVR_REC_JOB_ENTRY &RecJobEntry )
{
    string ChannelName = RecJobEntry.strChannelName;
    StringUtils::Replace(ChannelName,"|","\\|");
    
    string strTitle = RecJobEntry.Timer.strTitle;
    StringUtils::Replace(strTitle,"|","\\|");
    
    string strEpgSearchString = RecJobEntry.Timer.strEpgSearchString;
    StringUtils::Replace(strEpgSearchString,"|","\\|");
    
    string strDirectory = RecJobEntry.Timer.strDirectory;
    StringUtils::Replace(strDirectory,"|","\\|");
    
    string strSummary = RecJobEntry.Timer.strSummary;
    StringUtils::Replace(strSummary,"|","\\|");
    
    string Line = "\""+ChannelName;                                             //0
    Line = Line+"\"|\""+inttostr(RecJobEntry.Timer.iClientIndex);               //1
    Line = Line+"\"|\""+inttostr(RecJobEntry.Timer.iParentClientIndex);         //2
    Line = Line+"\"|\""+inttostr(RecJobEntry.Timer.iClientChannelUid);          //3
    Line = Line+"\"|\""+inttostr(RecJobEntry.Timer.startTime);                  //4
    Line = Line+"\"|\""+inttostr(RecJobEntry.Timer.endTime);                    //5
    Line = Line+"\"|\""+inttostr(RecJobEntry.Timer.state);                      //6
    Line = Line+"\"|\""+inttostr(RecJobEntry.Timer.iTimerType);                 //7
    Line = Line+"\"|\""+strTitle;                                               //8
    Line = Line+"\"|\""+strEpgSearchString;                                     //9
    if (RecJobEntry.Timer.bFullTextEpgSearch==true)
        Line = Line+"\"|\"1";
    else
        Line = Line+"\"|\"0";                                                   //10
    Line = Line+"\"|\""+strDirectory;                                           //11
    Line = Line+"\"|\""+strSummary;                                             //12
    Line = Line+"\"|\""+inttostr(RecJobEntry.Timer.iPriority);                  //13
    Line = Line+"\"|\""+inttostr(RecJobEntry.Timer.iLifetime);                  //14
    Line = Line+"\"|\""+inttostr(RecJobEntry.Timer.iRecordingGroup);            //15
    Line = Line+"\"|\""+inttostr(RecJobEntry.Timer.firstDay);                   //16
    Line = Line+"\"|\""+inttostr(RecJobEntry.Timer.iWeekdays);                  //17
    Line = Line+"\"|\""+inttostr(RecJobEntry.Timer.iPreventDuplicateEpisodes);  //18
    Line = Line+"\"|\""+inttostr(RecJobEntry.Timer.iEpgUid);                    //19
    Line = Line+"\"|\""+inttostr(RecJobEntry.Timer.iMarginStart);               //20
    Line = Line+"\"|\""+inttostr(RecJobEntry.Timer.iMarginEnd);                 //21
    Line = Line+"\"|\""+inttostr(RecJobEntry.Timer.iGenreType);                 //22
    Line = Line+"\"|\""+inttostr(RecJobEntry.Timer.iGenreSubType);              //23
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

    RecJobEntry.strChannelName                    = lineVect[0];                                        //0
    StringUtils::Replace(RecJobEntry.strChannelName,"\\|","|");
    
    RecJobEntry.Timer.iClientIndex                = strtoint(lineVect[1]);                              //1
    RecJobEntry.Timer.iParentClientIndex          = strtoint(lineVect[2]);                              //2
    RecJobEntry.Timer.iClientChannelUid           = strtoint(lineVect[3]);                              //3
    RecJobEntry.Timer.startTime                   = strtoint(lineVect[4]);                              //4
    RecJobEntry.Timer.endTime                     = strtoint(lineVect[5]);                              //5
    RecJobEntry.Timer.state                       = (PVR_TIMER_STATE) strtoint(lineVect[6]);            //6
    RecJobEntry.Timer.iTimerType                  = strtoint(lineVect[7]);                              //7
    
    string strTitle                               = lineVect[8];                                        //8
    StringUtils::Replace(strTitle,"\\|","|");
    strncpy(RecJobEntry.Timer.strTitle, strTitle.c_str(), PVR_ADDON_NAME_STRING_LENGTH - 1);
    
    string strEpgSearchString                     = lineVect[9];                                        //9
    StringUtils::Replace(strEpgSearchString,"\\|","|");
    strncpy(RecJobEntry.Timer.strEpgSearchString, strEpgSearchString.c_str(), PVR_ADDON_NAME_STRING_LENGTH - 1);
    
    RecJobEntry.Timer.bFullTextEpgSearch          = strtoint(lineVect[10]);                             //10
    
    string strDirectory                           = lineVect[11];                                       //11
    StringUtils::Replace(strDirectory,"\\|","|");
    strncpy(RecJobEntry.Timer.strDirectory, strDirectory.c_str(), PVR_ADDON_NAME_STRING_LENGTH - 1);
    
    string strSummary                              = lineVect[12];                                      //12
    StringUtils::Replace(strSummary,"\\|","|");
    strncpy(RecJobEntry.Timer.strSummary, strSummary.c_str(), PVR_ADDON_NAME_STRING_LENGTH - 1);
    
    RecJobEntry.Timer.iPriority                   = strtoint(lineVect[13]);                             //13
    RecJobEntry.Timer.iLifetime                   = strtoint(lineVect[14]);                             //14
    RecJobEntry.Timer.iRecordingGroup             = strtoint(lineVect[15]);                             //15
    RecJobEntry.Timer.firstDay                    = strtoint(lineVect[16]);                             //16
    RecJobEntry.Timer.iWeekdays                   = strtoint(lineVect[17]);                             //17
    RecJobEntry.Timer.iPreventDuplicateEpisodes   = strtoint(lineVect[18]);                             //18
    RecJobEntry.Timer.iEpgUid                     = strtoint(lineVect[19]);                             //19
    RecJobEntry.Timer.iMarginStart                = strtoint(lineVect[20]);                             //20
    RecJobEntry.Timer.iMarginEnd                  = strtoint(lineVect[21]);                             //21
    RecJobEntry.Timer.iGenreType                  = strtoint(lineVect[22]);                             //22
    RecJobEntry.Timer.iGenreSubType               = strtoint(lineVect[23]);                             //23
    return true;
}
