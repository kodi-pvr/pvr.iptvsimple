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

#include "platform/util/StringUtils.h"
#include <map>
#include "platform/threads/threads.h"
#include "PVRRecorderThread.h"
#include "PVRIptvData.h"
#include "PVRRecJob.h"
#include "PVRSchedulerThread.h"
#include "PVRUtils.h"
#include "libexecstream/exec-stream.h"

#include <iostream>
#include <fstream>
#include <string>
#include <unistd.h>

using namespace ADDON;

extern PVRRecJob *p_RecJob;
extern PVRSchedulerThread *p_Scheduler;
extern string p_recordingsPath;
extern string p_ffmpegPath;
extern string p_ffmpegParams;
extern string p_rtmpdumpPath;

PVRRecorderThread::PVRRecorderThread(PVRIptvChannel &currentChannel, int iClientIndex)
{
    isWorking = false;
    t_iClientIndex = iClientIndex;
    t_currentChannel = currentChannel;
    CreateThread();
}

PVRRecorderThread::~PVRRecorderThread(void)
{
    PVR_REC_JOB_ENTRY entry;
    PVRIptvChannel currentChannel;
    p_RecJob->getJobEntry(t_iClientIndex, entry);
        
    XBMC->Log(LOG_DEBUG,"Closing thread %s",entry.Timer.strTitle);
    while (entry.Status==PVR_STREAM_IS_RECORDING)
    {
        p_RecJob->getJobEntry(t_iClientIndex, entry);
        XBMC->Log(LOG_DEBUG,"Closing recording thread %s",entry.Timer.strTitle);
    }
}

void PVRRecorderThread::StopThread(bool bWait /*= true*/)
{
    PVR_REC_JOB_ENTRY entry;
    p_RecJob->getJobEntry(t_iClientIndex, entry);
    entry.Status = PVR_STREAM_IS_STOPPING;
    p_RecJob->updateJobEntry(entry);
    XBMC->Log(LOG_DEBUG,"Stopping thread %s",entry.Timer.strTitle);
    CThread::StopThread(bWait);
}

void PVRRecorderThread::CorrectBufferDurationFlV (char* buffer, const double &duration, const int &pos)
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
    XBMC->Log(LOG_NOTICE, "Duration corrected");
}

void PVRRecorderThread::CorrectDurationFLVFile (const string &videoFile, const double &duration)
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
	XBMC->Log(LOG_NOTICE, "Duration corrected");
    }
}

bool PVRRecorderThread::OpenStream (const string strStreamUrl)
{   
    if(strStreamUrl.substr(0, 7) == "rtmp://"
       || strStreamUrl.substr(0, 8) == "rtmpt://"
       || strStreamUrl.substr(0, 8) == "rtmpe://"
       || strStreamUrl.substr(0, 9) == "rtmpte://"
       || strStreamUrl.substr(0, 8) == "rtmps://")
    {
	vector<string> stremaUrlVect = StringUtils::Split (strStreamUrl," ");
	string strParams;
	if (stremaUrlVect.size()>0) {
	    strParams = " -r \""+stremaUrlVect[0]+"\"";
	    for (unsigned int i=1; i<stremaUrlVect.size();i++) {
		string line = StringUtils::Trim(stremaUrlVect[i]);
		if (line.length()>0) {
		    vector<string> lineVect = StringUtils::Split (line,"=");
		    if (lineVect[0]=="live") {
			strParams = strParams + " --live";
		    }
		    else {
			strParams = strParams + " --"+line;
		    }
		}
	    }
	}
	try {
	    string rtmpDump = p_rtmpdumpPath;
	    //string rtmpDump = "cat";
	    string strCommand = rtmpDump+strParams;
	    XBMC->Log(LOG_NOTICE,"Starting rtmpdump: %s",strCommand.c_str());
	    es.set_binary_mode(exec_stream_t::s_out);
	    es.set_wait_timeout(exec_stream_t::s_out,10000);
	    es.start( rtmpDump , strParams);
	    return true;
	
	}catch( std::exception const & e ) {
	    return false;
        }
	return false;
    }
    return false;
}

int PVRRecorderThread::ReadStream(void *fileHandle)
{
    
    try {
	string strBuffer;
	getline( es.out(), strBuffer,'\n' ).good();
	strBuffer = strBuffer+"\n";
	XBMC->WriteFile(fileHandle, strBuffer.c_str(), strBuffer.size());
	return strBuffer.size();
	
    }catch( std::exception const & e ) {
	return 0;
    }
    return -1;
}

void PVRRecorderThread::CloseStream (void)
{
    
    try {
	if (!es.close())
	{
	    es.kill();  
	}
    }catch( std::exception const & e ) {
	es.kill();
    }
}

void *PVRRecorderThread::Process(void)
{   
    PVR_REC_JOB_ENTRY entry;
    p_RecJob->getJobEntry(t_iClientIndex, entry);
    entry.Status = PVR_STREAM_IS_RECORDING;
    p_RecJob->updateJobEntry(entry);
    
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
    
    XBMC->Log(LOG_NOTICE, "Try to open stream %s", t_currentChannel.strStreamURL.c_str());
    
    string filename = entry.Timer.strTitle;
    filename = filename+strDate+".flv";
    filename = (string) XBMC->UnknownToUTF8(filename.c_str());
	    
    string videoFile = p_recordingsPath + filename;
    XBMC->Log(LOG_NOTICE,"File to write: %s ",videoFile.c_str());
    
    if(t_currentChannel.strStreamURL.substr(0, 7) == "rtmp://"
       || t_currentChannel.strStreamURL.substr(0, 8) == "rtmpt://"
       || t_currentChannel.strStreamURL.substr(0, 8) == "rtmpe://"
       || t_currentChannel.strStreamURL.substr(0, 9) == "rtmpte://"
       || t_currentChannel.strStreamURL.substr(0, 8) == "rtmps://")
    {
	if (p_rtmpdumpPath.length()==0)
	{
	    XBMC->Log(LOG_ERROR,"Path to rtmpdump binary is not set. Please change addon configuration.");
	    entry.Status = PVR_STREAM_STOPPED;
            entry.Timer.state= PVR_TIMER_STATE_ERROR;
	    p_RecJob->updateJobEntry(entry);
            try {
		PVR->TriggerTimerUpdate();
	    }catch( std::exception const & e ) {
		//Closing Kodi, TriggerTimerUpdate is not available
	    }
	    return NULL;
	}
	
	if (OpenStream(t_currentChannel.strStreamURL.c_str()))
        {
	    //Stream is live
            void *fileHandle;
            fileHandle = XBMC->OpenFileForWrite(videoFile.c_str(), true);
        
            int bytes_read;
            double duration = entry.Timer.endTime-entry.Timer.startTime;
            bool found = false;
            
            char buffer[1024];
            char buff [9];
            t_startRecTime = time(NULL);
	    
	    bytes_read = ReadStream(fileHandle);
            bool opened = false;
	    bool started = false;
            while(bytes_read>0)
            {
		if (started==false)
		{
		    t_startRecTime = time(NULL);
		    XBMC->Log(LOG_NOTICE,"Recording started using rtmp method: %s ",entry.Timer.strTitle);
		    started = true;
		}
                
                p_RecJob->getJobEntry(t_iClientIndex, entry);
                if (entry.Timer.endTime<time(NULL) || entry.Status==PVR_STREAM_IS_STOPPING || entry.Status==PVR_STREAM_STOPPED)
                {
                    CloseStream();
                    XBMC->CloseFile(fileHandle);
                            
                    XBMC->Log(LOG_NOTICE, "Recording stopped %s", entry.Timer.strTitle);
                    time_t end_time = time(NULL);
                   
                    //Correct duration time
                    duration = end_time-t_startRecTime;
                    CorrectDurationFLVFile (videoFile,duration);
                    
                    entry.Status = PVR_STREAM_STOPPED;
                    entry.Timer.state= PVR_TIMER_STATE_COMPLETED;
                    p_RecJob->updateJobEntry(entry);
                    try {
			PVR->TriggerTimerUpdate();
		    }catch( std::exception const & e ) {
			//Closing Kodi, TriggerTimerUpdate is not available
		    }
                    return NULL;
                }
                if (bytes_read<=0)
                {
                    CloseStream();
                    XBMC->CloseFile(fileHandle);
    
                    XBMC->Log(LOG_NOTICE, "Recording stopped %s", entry.Timer.strTitle);
                    time_t end_time = time(NULL);
                    
                    //Correct duration time
                    duration = end_time-t_startRecTime;
                    CorrectDurationFLVFile (videoFile,duration);
                    entry.Status = PVR_STREAM_STOPPED;
                    entry.Timer.state= PVR_TIMER_STATE_COMPLETED;
                    p_RecJob->updateJobEntry(entry);
                    try {
			PVR->TriggerTimerUpdate();
		    }catch( std::exception const & e ) {
			//Closing Kodi, TriggerTimerUpdate is not available
		    }
                    return NULL;
                }
		bytes_read = ReadStream(fileHandle);
            }
	}
    }
    else
    {
	if (p_ffmpegPath.length()==0)
	{
	    XBMC->Log(LOG_ERROR,"Path to ffmpeg binary is not set. Please change addon configuration.");
	    entry.Status = PVR_STREAM_STOPPED;
            entry.Timer.state= PVR_TIMER_STATE_ERROR;
	    p_RecJob->updateJobEntry(entry);
            try {
		PVR->TriggerTimerUpdate();
	    }catch( std::exception const & e ) {
		//Closing Kodi, TriggerTimerUpdate is not available
	    }
	    return NULL;
	}
	
	if (p_ffmpegParams.length()==0)
	{
	    XBMC->Log(LOG_ERROR,"Recompression params for ffmpeg are not set. Please change addon configuration.");
	    entry.Status = PVR_STREAM_STOPPED;
            entry.Timer.state= PVR_TIMER_STATE_ERROR;
	    p_RecJob->updateJobEntry(entry);
            try {
		PVR->TriggerTimerUpdate();
	    }catch( std::exception const & e ) {
		//Closing Kodi, TriggerTimerUpdate is not available
	    }
	    return NULL;
	}
    
	vector<string> stremaUrlVect = StringUtils::Split (t_currentChannel.strStreamURL," ");
	string strParams;
	if (stremaUrlVect.size()>0)
	{
	    strParams = " -i \""+stremaUrlVect[0];
	    for (unsigned int i=1; i<stremaUrlVect.size();i++)
	    {
		string line = StringUtils::Trim(stremaUrlVect[i]);
		if (line.length()>0) {
		    vector<string> lineVect = StringUtils::Split (line,"=");
		    if (lineVect[0]=="live")
		    {
			strParams = strParams + " --live";
		    }
		    else
		    {
			strParams = strParams + " --"+line;
		    }
		}
	    }
	    strParams =  strParams+"\" "+p_ffmpegParams+" \""+videoFile+"\"";
	}
	
	string strCommand = p_ffmpegPath+strParams;
    
	XBMC->Log(LOG_NOTICE,"Starting ffmpeg: %s",strCommand.c_str());
	es.set_binary_mode(exec_stream_t::s_out);
	//es.set_wait_timeout(exec_stream_t::s_out,100000);
	try {
	    es.start( p_ffmpegPath , strParams);
	}catch( std::exception const & e ) {
	    XBMC->Log(LOG_NOTICE, "Error recording %s", entry.Timer.strTitle);
	    entry.Status = PVR_STREAM_STOPPED;
	    entry.Timer.state= PVR_TIMER_STATE_ERROR;
	    p_RecJob->updateJobEntry(entry);
	    try {
		PVR->TriggerTimerUpdate();
	    }catch( std::exception const & e ) {
		//Closing Kodi, TriggerTimerUpdate is not available
	    }
	    return NULL;
	}
	while (true)
	{
	    p_RecJob->getJobEntry(t_iClientIndex, entry);
	    if (entry.Timer.endTime<time(NULL) || entry.Status==PVR_STREAM_IS_STOPPING || entry.Status==PVR_STREAM_STOPPED)
	    {
		es.kill();
		XBMC->Log(LOG_NOTICE, "Recording stopped %s", entry.Timer.strTitle);
		entry.Status = PVR_STREAM_STOPPED;
		entry.Timer.state= PVR_TIMER_STATE_COMPLETED;
		p_RecJob->updateJobEntry(entry);
		time_t end_time = time(NULL);
		//Correct duration time
		double duration = end_time-t_startRecTime;
		CorrectDurationFLVFile (videoFile,duration);
		try {
		    PVR->TriggerTimerUpdate();
		}catch( std::exception const & e ) {
		    //Closing Kodi, TriggerTimerUpdate is not available
		}
		return NULL;
	    }
	}
    }
    return NULL;
}