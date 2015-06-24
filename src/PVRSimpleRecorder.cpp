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
#include <map>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <time.h>
#include <stdlib.h>

#include "PVRIptvData.h"
#include "PVRSimpleRecorder.h"
#include "platform/util/StringUtils.h"
#include "platform/threads/threads.h"
using namespace ADDON;
using namespace std;

#define RECJOB_FILE_NAME "pvrsimplerecorder.cache"
string p_recordingsPath;
PVRRecJob *p_RecJob;

int p_RecJob_lock = 0;

/****************************************************
 *  Utils
 ****************************************************/

int strtoint(string s)
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

string inttostr (int i)
{
    string tmp;
    sprintf((char*)tmp.c_str(), "%d", i);
    string str = tmp.c_str();
    return str;
}

extern void CloseRecordingThreads(void)
{
    PVRIptvChannel currentChannel;
    p_RecJob->setLock();
    /*
    while (p_RecJob_lock==1)
    {
        //wait for unlock 
    }
    p_RecJob_lock=1;
    */
    map<int,PVR_REC_JOB_ENTRY> RecJobs = p_RecJob->getEntryData();
    for (map<int,PVR_REC_JOB_ENTRY>::iterator rec=RecJobs.begin(); rec!=RecJobs.end(); ++rec)
    {
        PVR_REC_JOB_ENTRY entry = rec->second;
        if (entry.Status!=PVR_STREAM_NO_STREAM && entry.Status!=PVR_STREAM_STOPPED)
        {
            XBMC->Log(LOG_NOTICE,"Recording is still active, trying to stop: %s ",entry.Timer.strTitle);
            entry.Status = PVR_STREAM_IS_STOPPING;
            p_RecJob->updateJobEntry(entry, currentChannel);
            while (entry.Status!=PVR_STREAM_NO_STREAM && entry.Status!=PVR_STREAM_STOPPED)
            {    
                p_RecJob->getJobEntry(entry.Timer.iClientIndex, entry ,currentChannel);
                XBMC->Log(LOG_NOTICE,"Stopping recording %s:",entry.Timer.strTitle);
            }
        }
    }
    p_RecJob->setUnlock();
    //p_RecJob_lock=0;
    delete (p_RecJob);
}

/****************************************************
 *  Class PVRRecJob
 ****************************************************/

PVRRecJob::PVRRecJob(PVRIptvData *data)
{
    m_data = data;
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
        m_JobEntryData[RecJobEntry.Timer.iClientIndex] = RecJobEntry;
        storeEntryData();
        return true;
    }
    return false;
}
    
bool PVRRecJob::getJobEntry(const int ientryIndex, PVR_REC_JOB_ENTRY &entry, PVRIptvChannel &currentChannel)
{
    if (m_JobEntryData.find(ientryIndex) == m_JobEntryData.end()) return false;
    
    entry = m_JobEntryData[ientryIndex];
    return getProperlyChannel(entry, currentChannel);
}
    
bool PVRRecJob::getJobEntry(const string strChannelName, PVR_REC_JOB_ENTRY &entry, PVRIptvChannel &currentChannel)
{
    for (std::map<int,PVR_REC_JOB_ENTRY>::iterator it=m_JobEntryData.begin(); it!=m_JobEntryData.end(); ++it)
    {
        if (it->second.strChannelName==strChannelName)
        {
            entry = it->second;
            return getProperlyChannel(entry, currentChannel);
        }
        return false;
    }
    return false;
}
    
bool PVRRecJob::updateJobEntry(const PVR_REC_JOB_ENTRY &RecJobEntry, PVRIptvChannel &currentChannel)
{
    PVR_REC_JOB_ENTRY entry = RecJobEntry;
    if (getJobEntry(RecJobEntry.Timer.iClientIndex ,entry ,currentChannel))
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
        if (getProperlyChannel(entry, currentChannel))
        {
            m_JobEntryData[RecJobEntry.Timer.iClientIndex] = entry;
            storeEntryData();
            return true; 
        }
    }
    return false;
}
    
bool PVRRecJob::delJobEntry(const int ientryIndex, PVRIptvChannel &currentChannel)
{
    PVR_REC_JOB_ENTRY entry;
    if (getJobEntry(ientryIndex ,entry ,currentChannel))
    {
        m_JobEntryData.erase(ientryIndex);
        storeEntryData();
        return true;
    }
    return false;
}

bool PVRRecJob::setLock (void)
{
    bool ok = false;
    int lock_rand = rand() % 10000000;
    while (ok==false) {
        if (p_RecJob_lock>0 && p_RecJob_lock==lock_rand) return true;
        else {
            while (p_RecJob_lock>0)
            {
                //wait for unlock 
            } 
            p_RecJob_lock = lock_rand;
        }
    }
}

void PVRRecJob::setUnlock (void)
{
    p_RecJob_lock = 0;
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
    if (RecJobEntry.Timer.bIsRepeating==true)
        Line = Line+"\"|\"1";
    else
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
    if (strtoint(lineVect[9])==1) RecJobEntry.Timer.bIsRepeating = true; else RecJobEntry.Timer.bIsRepeating = false;
    RecJobEntry.Timer.firstDay        = strtoint(lineVect[10]);
    RecJobEntry.Timer.iWeekdays       = strtoint(lineVect[11]);
    RecJobEntry.Timer.iEpgUid         = strtoint(lineVect[12]);
    RecJobEntry.Timer.iMarginStart    = strtoint(lineVect[13]);
    RecJobEntry.Timer.iMarginEnd      = strtoint(lineVect[14]);
    RecJobEntry.Timer.iGenreType      = strtoint(lineVect[15]);
    RecJobEntry.Timer.iGenreSubType   = strtoint(lineVect[16]);
    return true;
}

/****************************************************
 *  Class PVRSimpleRecorderThread
 ****************************************************/
void PVRSimpleRecorderThread::CorrectBufferDurationFlV (char* buffer, const double &duration, const int &pos)
{
    union
    {
        unsigned char dc[8];
        double dd;
    } d;
    d.dd = duration;
    long one = 1;
    //is isBigEndian?
    if(!(*((char *)(&one))))
    {
        buffer[pos+0] = d.dc[0]; buffer[pos+1] = d.dc[1]; buffer[pos+2] = d.dc[2]; buffer[pos+3] = d.dc[3]; buffer[pos+4] = d.dc[4]; buffer[pos+5] = d.dc[5]; buffer[pos+6] = d.dc[6]; buffer[pos+7] = d.dc[7];
    }
    else
    {
        buffer[pos+0] = d.dc[7]; buffer[pos+1] = d.dc[6]; buffer[pos+2] = d.dc[5]; buffer[pos+3] = d.dc[4]; buffer[pos+4] = d.dc[3]; buffer[pos+5] = d.dc[2]; buffer[pos+6] = d.dc[1]; buffer[pos+7] = d.dc[0];
    }
}

void PVRSimpleRecorderThread::CorrectDurationFLVFile (const string &videoFile, const double &duration)
{
    //read 4096 first file bytes
    char buffer[4096];
    void *fileHandle;
    fileHandle = XBMC->OpenFile(videoFile.c_str(), 0);
    XBMC->ReadFile(fileHandle, buffer, 4096);
    XBMC->CloseFile(fileHandle);
    int loop_end = 4096-8-sizeof(double);
    int loop;
    int pos = -1;
    for (loop=0; loop<loop_end;loop++)
    {
        if (buffer[loop]=='d' && buffer[loop+1]=='u' && buffer[loop+2]=='r' && buffer[loop+3]=='a' && buffer[loop+4]=='t' && buffer[loop+5]=='i' && buffer[loop+6]=='o' && buffer[loop+7]=='n')
            pos = loop;
    }
    //correct 4096 first file bytes
    if (pos>=0)
    {
        pos = pos+9;
        CorrectBufferDurationFlV(buffer,duration,pos);
        fstream out2(videoFile, ios::binary | ios::out | ios::in);
        out2.seekp(0, ios::beg);
        out2.write(buffer, 4096);
        out2.close();
    }
}

PVRSimpleRecorderThread::PVRSimpleRecorderThread(PVRIptvChannel &currentChannel, int iClientIndex)
{
    t_iClientIndex = iClientIndex;
    t_currentChannel = currentChannel;
    CreateThread();
}

PVRSimpleRecorderThread::~PVRSimpleRecorderThread(void)
{
    PVR_REC_JOB_ENTRY entry;
    PVRIptvChannel currentChannel;
    p_RecJob->getJobEntry(t_iClientIndex, entry, currentChannel);
        
    XBMC->Log(LOG_DEBUG,"Closing thread %s:",entry.Timer.strTitle);
    while (entry.Status==PVR_STREAM_IS_RECORDING)
    {
        p_RecJob->getJobEntry(t_iClientIndex, entry, currentChannel);
        XBMC->Log(LOG_DEBUG,"Closing recording thread %s ",entry.Timer.strTitle);
    }
}

void PVRSimpleRecorderThread::StopThread(bool bWait /*= true*/)
{
    PVR_REC_JOB_ENTRY entry;
    PVRIptvChannel currentChannel;
    p_RecJob->getJobEntry(t_iClientIndex, entry, currentChannel);
    entry.Status = PVR_STREAM_IS_STOPPING;
    p_RecJob->updateJobEntry(entry, currentChannel);
    XBMC->Log(LOG_DEBUG,"Stopping thread %s:",entry.Timer.strTitle);
    CThread::StopThread(bWait);
}

void *PVRSimpleRecorderThread::Process(void)
{   
    PVR_REC_JOB_ENTRY entry;
    p_RecJob->getJobEntry(t_iClientIndex, entry, t_currentChannel);
    entry.Status = PVR_STREAM_IS_RECORDING;
    p_RecJob->updateJobEntry(entry, t_currentChannel);
    
    char strPlayList[65535];
    char buffer[1024];
    int bytes_read;
    int size = XBMC->GetStreamPlaylist(t_currentChannel.strStreamURL.c_str(), strPlayList, 65535);
    
    string strChunk = "";
    
    struct tm *current;
    time_t now;
	
    time(&now);
    current = localtime(&now);
    string month = inttostr(current->tm_mon);
    if (current->tm_mon<10) month = "0"+month;
    string day = inttostr(current->tm_mday);
    if (current->tm_mday<10) day = "0"+day;
    string hour = inttostr(current->tm_hour);
    if (current->tm_hour<10) hour = "0"+hour;
    string min = inttostr(current->tm_min);
    if (current->tm_min<10) min = "0"+min;
    string sec = inttostr(current->tm_sec);
    if (current->tm_sec<10) sec = "0"+sec;
    
    string strDate = " ("+inttostr(current->tm_year+1900)+"-"+month+"-"+day+" "+hour+"-"+min+"-"+sec+")";
    if (size>0)
    {
        bool opened = false;
        void *fileHandle;
        
        while (true)
        {
            if (entry.Timer.endTime<time(NULL) || entry.Status==PVR_STREAM_IS_STOPPING || entry.Status==PVR_STREAM_STOPPED)
            {
                XBMC->CloseFile(fileHandle);
                XBMC->Log(LOG_NOTICE, "Recording stopped %s", entry.Timer.strTitle);
                entry.Status = PVR_STREAM_STOPPED;
                entry.Timer.state= PVR_TIMER_STATE_COMPLETED;
                p_RecJob->updateJobEntry(entry, t_currentChannel);
                PVR->TriggerTimerUpdate();
                return NULL;
            }
                
            //Stream is chunked
            string strList = (string) strPlayList;
            vector<string> vstrList = StringUtils::Split (strPlayList,"\n");
            vector <string>::iterator it ( vstrList.begin() );
            bool foundChunk = false;
            for (it=vstrList.begin(); it!=vstrList.end(); it++)
                if (*it==strChunk) foundChunk = true;
            for (it=vstrList.begin(); it!=vstrList.end(); it++)
            {
                if (foundChunk==true)
                {
                    if (*it!=strChunk)
                        continue;
                    else
                    {
                        foundChunk = false;
                        continue;
                    }
                }
                strChunk = *it;
                //Prepare file name
                string filename = entry.Timer.strTitle;
                filename = filename+strDate+".flv";
                string videoFile = p_recordingsPath + filename;
                
                
                if (opened==false)
                {
                    XBMC->Log(LOG_NOTICE,"File to write (ch): %s ",videoFile.c_str());
                    XBMC->Log(LOG_NOTICE,"Recording started using chunks method: %s ",entry.Timer.strTitle);
                    fileHandle = XBMC->OpenFileForWrite(videoFile.c_str(), true);
                    opened=true;
                }

                
                void* stream = XBMC->OpenStream(strChunk.c_str());
                if (stream)
                {
                    while ((bytes_read = XBMC->ReadStream(stream, buffer, 1024)))
                    {
                        XBMC->WriteFile(fileHandle, buffer, bytes_read);
                    }
                    XBMC->CloseStream(stream);
                }

                p_RecJob->getJobEntry(t_iClientIndex, entry, t_currentChannel);
                if (entry.Timer.endTime<time(NULL) || entry.Status==PVR_STREAM_IS_STOPPING || entry.Status==PVR_STREAM_STOPPED)
                {
                    XBMC->CloseFile(fileHandle);
                    XBMC->Log(LOG_NOTICE, "Recording stopped %s", entry.Timer.strTitle);
                    entry.Status = PVR_STREAM_STOPPED;
                    entry.Timer.state= PVR_TIMER_STATE_COMPLETED;
                    p_RecJob->updateJobEntry(entry, t_currentChannel);
                    PVR->TriggerTimerUpdate();
                    return NULL;
                }
            }
            int size = XBMC->GetStreamPlaylist(t_currentChannel.strStreamURL.c_str(), strPlayList, 65535);
        }
    }
    else
    {   
        XBMC->Log(LOG_NOTICE, "Try to open stream %s", t_currentChannel.strStreamURL.c_str());
        void *stream = XBMC->OpenStream(t_currentChannel.strStreamURL.c_str());   
        if (stream)
        {
            //Stream is live
            string filename = entry.Timer.strTitle;
            filename = filename+strDate+".flv";

            string videoFile = p_recordingsPath + filename;
            XBMC->Log(LOG_NOTICE,"File to write (s): %s ",videoFile.c_str());
            void *fileHandle;
            fileHandle = XBMC->OpenFileForWrite(videoFile.c_str(), true);
        
            int bytes_read;
            double duration = entry.Timer.endTime-entry.Timer.startTime;
            bool found = false;
            
            char buffer[1024];
            char buff [9];
            t_startRecTime = time(NULL);
            bytes_read = XBMC->ReadStream(stream,buffer,1024);
            bool opened = false;
            
            while(bytes_read>=0)
            {
                if (opened==false) {
                    XBMC->Log(LOG_NOTICE,"Recording started using rtmp: %s ",entry.Timer.strTitle);
                }
                opened = true;
                p_RecJob->getJobEntry(t_iClientIndex, entry, t_currentChannel);
                if (entry.Timer.endTime<time(NULL) || entry.Status==PVR_STREAM_IS_STOPPING || entry.Status==PVR_STREAM_STOPPED)
                {
                    XBMC->CloseStream(stream);
                    XBMC->CloseFile(fileHandle);
                            
                    XBMC->Log(LOG_NOTICE, "Recording stopped %s", entry.Timer.strTitle);
                    time_t end_time = time(NULL);
                    if (end_time<entry.Timer.endTime)
                    {
                        //Correct duration time
                        duration = end_time-t_startRecTime;
                        CorrectDurationFLVFile (videoFile,duration);
                    }
                    entry.Status = PVR_STREAM_STOPPED;
                    entry.Timer.state= PVR_TIMER_STATE_COMPLETED;
                    p_RecJob->updateJobEntry(entry, t_currentChannel);
                    PVR->TriggerTimerUpdate();
                    return NULL;
                }
                if (bytes_read>0)
                {
                    int loop_end = bytes_read-8-sizeof(double);
                    if (found==false)
                    {
                        t_startRecTime = time(NULL);
                        double duration = entry.Timer.endTime-t_startRecTime;
                        int loop;
                        int pos = -1;
                        for (loop=0; loop<loop_end;loop++)
                        {
                            if (buffer[loop]=='d' && buffer[loop+1]=='u' && buffer[loop+2]=='r' && buffer[loop+3]=='a' && buffer[loop+4]=='t' && buffer[loop+5]=='i' && buffer[loop+6]=='o' && buffer[loop+7]=='n')
                                pos = loop;
                        } 
                        if (pos>=0)
                        {
                            found = true;
                            pos = pos+9;
                            CorrectBufferDurationFlV(buffer,duration,pos);
                        }    
                    }
                    XBMC->WriteFile(fileHandle, buffer, bytes_read);
                }
                else
                {
                    XBMC->CloseStream(stream);
                    XBMC->CloseFile(fileHandle);
    
                    XBMC->Log(LOG_NOTICE, "Recording stopped %s", entry.Timer.strTitle);
                    time_t end_time = time(NULL);
                    if (end_time<entry.Timer.endTime)
                    {
                        //Correct duration time
                        duration = end_time-t_startRecTime;
                        CorrectDurationFLVFile (videoFile,duration);
                    }
                    entry.Status = PVR_STREAM_STOPPED;
                    entry.Timer.state= PVR_TIMER_STATE_COMPLETED;
                    p_RecJob->updateJobEntry(entry, t_currentChannel);
                    PVR->TriggerTimerUpdate();
                    return NULL;
                }
                bytes_read = XBMC->ReadStream(stream,buffer,1024);
            }
            if (opened==true) {
                XBMC->Log(LOG_NOTICE, "Recording stopped %s", entry.Timer.strTitle);
                XBMC->CloseFile(fileHandle);
                XBMC->CloseStream(stream);
                time_t end_time = time(NULL);
                if (end_time<entry.Timer.endTime)
                {
                    //Correct duration time
                    duration = end_time-t_startRecTime;
                    CorrectDurationFLVFile (videoFile,duration);
                }
                entry.Status = PVR_STREAM_STOPPED;
                entry.Timer.state= PVR_TIMER_STATE_COMPLETED;
                p_RecJob->updateJobEntry(entry, t_currentChannel);
                PVR->TriggerTimerUpdate();
                return NULL;
            }
        }
        // Bad status.
        XBMC->Log(LOG_NOTICE,"Failed to start recording %s ",entry.Timer.strTitle);
        entry.Status = PVR_STREAM_STOPPED;
        entry.Timer.state= PVR_TIMER_STATE_ERROR;
        p_RecJob->updateJobEntry(entry, t_currentChannel);
        PVR->TriggerTimerUpdate();
        return NULL;
    }
    return NULL;
}

/****************************************************
 *  Class PVRSimpleRecorder
 ****************************************************/

PVRSimpleRecorder::PVRSimpleRecorder (PVRIptvData *data, const std::string &recordingsPath)
{
    m_data = data;
    m_recordingsPath = recordingsPath;
    p_RecJob = new PVRRecJob (data);
}

bool PVRSimpleRecorder::startRecording (const PVR_REC_JOB_ENTRY &RecJobEntry, PVRIptvChannel &currentChannel)
{
    PVR_REC_JOB_ENTRY entry;
    if (p_RecJob->getJobEntry(RecJobEntry.Timer.iClientIndex, entry, currentChannel))
    {
        entry = RecJobEntry;
        if (p_RecJob->getProperlyChannel(entry, currentChannel))
        {
            XBMC->Log(LOG_NOTICE,"Starting recording %s ",RecJobEntry.Timer.strTitle);
            entry.Status = PVR_STREAM_START_RECORDING;
            entry.Timer.state = PVR_TIMER_STATE_RECORDING;
            //p_iClientIndex = RecJobEntry.Timer.iClientIndex;
            p_recordingsPath = m_recordingsPath;
            p_RecJob->updateJobEntry(entry, currentChannel);
            PVRSimpleRecorderThread* TPtr = new PVRSimpleRecorderThread(currentChannel,RecJobEntry.Timer.iClientIndex);
            entry.ThreadPtr = TPtr;
            p_RecJob->updateJobEntry(entry, currentChannel);
            PVR->TriggerTimerUpdate();
            return true;
        }
        else
        {
            //Channel not found, update status
            entry.Timer.state = PVR_TIMER_STATE_ERROR;
            p_RecJob->updateJobEntry(entry, currentChannel);
            PVR->TriggerTimerUpdate();
        }
    }
    return false;
}

bool PVRSimpleRecorder::stopRecording (const PVR_REC_JOB_ENTRY &RecJobEntry, PVRIptvChannel &currentChannel)
{
    PVR_REC_JOB_ENTRY entry;
    entry = RecJobEntry;
    if (entry.Timer.state!=PVR_TIMER_STATE_NEW)
    {
        XBMC->Log(LOG_NOTICE,"Stopping recording %s ",entry.Timer.strTitle);
        entry.Status = PVR_STREAM_IS_STOPPING;
        p_RecJob->updateJobEntry(entry, currentChannel);
        PVR->TriggerTimerUpdate();
    }
    
    return true;
}

PVR_ERROR PVRSimpleRecorder::GetTimers(ADDON_HANDLE handle)
{
    PVRIptvChannel currentChannel;
    /*
    while (p_RecJob_lock==1)
    {
        //wait for unlock 
    }
    p_RecJob_lock=1;
    */
    p_RecJob->setLock();
    
    map<int,PVR_REC_JOB_ENTRY> RecJobs = p_RecJob->getEntryData();
    for (map<int,PVR_REC_JOB_ENTRY>::iterator rec=RecJobs.begin(); rec!=RecJobs.end(); ++rec)
    {
        if (p_RecJob->getProperlyChannel(rec->second,currentChannel))
        {
            p_RecJob->updateJobEntry(rec->second, currentChannel);
            PVR->TransferTimerEntry(handle, &rec->second.Timer); 
        }
    }
    
    //p_RecJob_lock=0;
    p_RecJob->setUnlock();
    
    return PVR_ERROR_NO_ERROR;
}

void PVRSimpleRecorder::updateRecordingStatus()
{
    PVRIptvChannel currentChannel;
    bool jobsUpdated = false;
    /*
    while (p_RecJob_lock==1)
    {
        //wait for unlock 
    }
    p_RecJob_lock=1;
    */
    p_RecJob->setLock();
    
    map<int,PVR_REC_JOB_ENTRY> RecJobs = p_RecJob->getEntryData();
    for (map<int,PVR_REC_JOB_ENTRY>::iterator rec=RecJobs.begin(); rec!=RecJobs.end(); ++rec)
    {
        if (rec->second.Status==PVR_STREAM_STOPPED)
        {
            p_RecJob->delJobEntry(rec->first, currentChannel);
            jobsUpdated = true;
        }
        else if (rec->second.Timer.endTime<time(NULL)) {
            p_RecJob->delJobEntry(rec->first, currentChannel);
            jobsUpdated = true;
        }
        else if ((rec->second.Timer.startTime-10)<=time(NULL) && rec->second.Timer.state == PVR_TIMER_STATE_SCHEDULED)
        {
            //Start new Recording
            XBMC->Log(LOG_NOTICE,"Try to start recording %s ",rec->second.Timer.strTitle);
            if (startRecording(rec->second,currentChannel)) jobsUpdated = true;
        }
        else if (rec->second.Timer.endTime<=time(NULL) && (rec->second.Timer.state==PVR_TIMER_STATE_RECORDING))
        {
            XBMC->Log(LOG_NOTICE,"Try to stop recording %s ",rec->second.Timer.strTitle);
            //Stop Recording
            if (stopRecording(rec->second,currentChannel)) jobsUpdated = true;
        }
    }
    
    //p_RecJob_lock=0;
    p_RecJob->setUnlock();
    
    if (jobsUpdated)
    {
        //PVR->TriggerTimerUpdate();
    }
}

int PVRSimpleRecorder::GetTimersAmount(void)
{    
    updateRecordingStatus();
    /*
    while (p_RecJob_lock==1)
    {
        //wait for unlock 
    }
    p_RecJob_lock=1;
    */
    
    p_RecJob->setLock();
    
    map<int,PVR_REC_JOB_ENTRY> RecJobs = p_RecJob->getEntryData();
    int size = RecJobs.size();
    
    //p_RecJob_lock=0;
    p_RecJob->setUnlock();
    
    return size;
}

PVR_ERROR PVRSimpleRecorder::UpdateTimer(const PVR_TIMER &timer)
{
    PVR_TIMER myTimer = timer;
    PVR_REC_JOB_ENTRY entry;
    PVRIptvChannel currentChannel;
    if (p_RecJob->getJobEntry(timer.iClientIndex,entry, currentChannel))
    {
        
        if (entry.Timer.state==PVR_TIMER_STATE_RECORDING && (entry.Status==PVR_STREAM_IS_RECORDING || entry.Status==PVR_STREAM_START_RECORDING || entry.Status==PVR_STREAM_IS_STOPPING)) {
            //Job is active
            if (timer.state==PVR_TIMER_STATE_CANCELLED)
            {
               //stop recording
               stopRecording(entry, currentChannel);
            }
            else
            {
                myTimer.state = entry.Timer.state;
            }
        }
        entry.Timer = myTimer;
        p_RecJob->updateJobEntry(entry, currentChannel);
        PVR->TriggerTimerUpdate();
        return PVR_ERROR_NO_ERROR;
    }
    PVR->TriggerTimerUpdate();
    return PVR_ERROR_FAILED;
}

PVR_ERROR PVRSimpleRecorder::DeleteTimer(const PVR_TIMER &timer, bool bForceDelete)
{    
    PVR_REC_JOB_ENTRY entry;
    PVRIptvChannel currentChannel;
    if (p_RecJob->getJobEntry(timer.iClientIndex,entry, currentChannel))
    {
        if (stopRecording(entry, currentChannel))
        {
            PVR->TriggerTimerUpdate();
            return PVR_ERROR_NO_ERROR;
        }
    }
    return PVR_ERROR_FAILED;
}

PVR_ERROR PVRSimpleRecorder::AddTimer (const PVR_TIMER &timer)
{
    if (m_recordingsPath.length()==0)
    {
        XBMC->Log(LOG_ERROR,"Recordins store folder is not set. Please change addon configuration.");
        return PVR_ERROR_FAILED;
    }
    //Set start time for Job
    time_t startTime = timer.startTime;
    //Bad start time and end time  
    if (timer.startTime>=timer.endTime)
        return PVR_ERROR_FAILED;

    //Correct start time if needed
    if (timer.startTime == 0 || timer.startTime<time(NULL))
      startTime = time(NULL);
    
    //GetChannel definition
    PVR_CHANNEL channel;
    channel.iUniqueId = timer.iClientChannelUid;
    
    PVRIptvChannel currentChannel;
    
    if (!m_data->GetChannel(channel, currentChannel))
        return PVR_ERROR_FAILED;
    
    //set end time for Job
    time_t endTime = timer.endTime;
    if (timer.startTime==0)
    {     
        EPG_TAG tag;
        PVR_ERROR myData = m_data->GetEPGTagForChannel(tag,channel, startTime, startTime);
        if (myData == PVR_ERROR_NO_ERROR)
            endTime = tag.endTime;
    }
    
    /*
    while (p_RecJob_lock==1)
    {
        //wait for unlock 
    }
    p_RecJob_lock=1;
    */
    p_RecJob->setLock();
    
    //Check, if job is aleready schduled for this time
    map<int,PVR_REC_JOB_ENTRY> RecJobs = p_RecJob->getEntryData();
    for (map<int,PVR_REC_JOB_ENTRY>::iterator rec=RecJobs.begin(); rec!=RecJobs.end(); ++rec)
    {
        if (rec->second.strChannelName==currentChannel.strChannelName)
        {
            if ((rec->second.Timer.endTime>startTime && rec->second.Timer.startTime<endTime)
                    || (rec->second.Timer.startTime<endTime && rec->second.Timer.endTime>startTime)
                    || (rec->second.Timer.startTime<startTime && rec->second.Timer.endTime>endTime)
                    || (rec->second.Timer.startTime>startTime && rec->second.Timer.endTime<endTime)) 
            {
                //Similar recording already scheduled
                if ((rec->second.Timer.state==PVR_TIMER_STATE_SCHEDULED || (rec->second.Timer.state==PVR_TIMER_STATE_RECORDING)) && (rec->second.Status==PVR_STREAM_IS_RECORDING || rec->second.Status==PVR_STREAM_START_RECORDING))
                {
                    //Send message to refresh timers
                    p_RecJob_lock=0;
                    return PVR_ERROR_ALREADY_PRESENT;
                }
            }
        }
    }
    
    //p_RecJob_lock=0;
    p_RecJob->setUnlock();
    
    //recalculate new job id
    int iJobId = timer.iClientIndex;
    if (iJobId==-1)
        iJobId = time(NULL);
        
    //Prepare new entry
    PVR_REC_JOB_ENTRY recJobEntry;
    recJobEntry.Timer = timer;
    recJobEntry.Timer.iClientIndex = iJobId;
    recJobEntry.Timer.startTime = startTime;
    recJobEntry.Timer.endTime = endTime;
    recJobEntry.strChannelName = currentChannel.strChannelName;
    p_RecJob->addJobEntry(recJobEntry);
    
    if (startTime<=time(NULL))
    {
        recJobEntry.Status = PVR_STREAM_IS_RECORDING;
        recJobEntry.Timer.state = PVR_TIMER_STATE_RECORDING;
        p_RecJob->updateJobEntry(recJobEntry, currentChannel);
        startRecording(recJobEntry, currentChannel);
        
    }
    else
    {
        recJobEntry.Status = PVR_STREAM_NO_STREAM;
        recJobEntry.Timer.state = PVR_TIMER_STATE_SCHEDULED;
        p_RecJob->updateJobEntry(recJobEntry, currentChannel);
    }
    PVR->TriggerTimerUpdate();
    return PVR_ERROR_NO_ERROR;
}