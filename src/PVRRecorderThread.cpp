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
#include "PVRRecorderThread.h"
#include "PVRIptvData.h"
#include "PVRRecJob.h"
#include "PVRSchedulerThread.h"
#include "PVRUtils.h"
#include "PVRPlayList.h"
#include "libexecstream/exec-stream.h"

#include <iostream>
#include <fstream>
#include <string>
#include <unistd.h>

using namespace ADDON;

extern PVRRecJob *p_RecJob;
extern PVRSchedulerThread *p_Scheduler;
extern string g_recordingsPath;
extern string g_ffmpegPath;
extern string g_ffmpegParams;
extern string g_rtmpdumpPath;
extern string g_fileExtension;
extern int g_streamTimeout;

extern bool s_triggerTimerUpdate;

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

void PVRRecorderThread::CorrectDurationFLVFile (const string &videoFile, const double &duration)
{
    if (duration<0){
	XBMC->Log(LOG_NOTICE, "Duration correction failed");
	return;
    }
    //read 1024 first file bytes
    char buffer[1024];
    void *fileHandle;
    fileHandle = XBMC->OpenFile(videoFile.c_str(), 0);
    XBMC->ReadFile(fileHandle, buffer, 1024);
    XBMC->CloseFile(fileHandle);
    int loop_end = 1024-8-sizeof(double);
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
        
	fileHandle = XBMC->OpenFileForWrite(videoFile.c_str(), 0);
	XBMC->SeekFile(fileHandle,0, ios::beg);
	int size = XBMC->WriteFile(fileHandle, buffer, 1024);
	if (size>0) {
	    XBMC->Log(LOG_NOTICE, "Duration corrected");
	}
	XBMC->CloseFile(fileHandle);
	return;
    }
    XBMC->Log(LOG_NOTICE, "Duration correction failed");
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
    string month = inttostr(current->tm_mon+1);
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
    
    //GetPlayList
    vector<string> vstrList;
    PVRPlayList* playList = new PVRPlayList();
    string strStreamUrl = t_currentChannel.strStreamURL;
    playList->GetPlaylist (strStreamUrl, vstrList);
    delete (playList);
    
    string filename = entry.Timer.strTitle;
    filename = filename+strDate+"."+g_fileExtension;
    
    string illegalChars = "\\/:?\"<>|*'";
    string::iterator it ( filename.begin() );
    for (it = filename.begin() ; it < filename.end() ; ++it){
	bool found = illegalChars.find(*it) != string::npos;
	if(found){
	    *it = ' ';
	}
    }
    
    string videoFile = g_recordingsPath + filename;
    
    XBMC->Log(LOG_NOTICE,"File to write: %s ",videoFile.c_str());
    string strParams;
    string strCommand;
    
    double duration = entry.Timer.endTime-entry.Timer.startTime;
    t_duration = duration;
    
    double length = 0;
    
    int rtmpStream = 0;
    if(strStreamUrl.substr(0, 7) == "rtmp://"
       || strStreamUrl.substr(0, 8) == "rtmpt://"
       || strStreamUrl.substr(0, 8) == "rtmpe://"
       || strStreamUrl.substr(0, 9) == "rtmpte://"
       || strStreamUrl.substr(0, 8) == "rtmps://")
    {
	rtmpStream = 1;
    }
    
    vector<string> stremaUrlVect = StringUtils::Split (strStreamUrl," ");
    if (stremaUrlVect.size()>0) {
	strParams = stremaUrlVect[0];
	for (unsigned int i=1; i<stremaUrlVect.size();i++) {
	    string line = StringUtils::Trim(stremaUrlVect[i]);
	    if (line.length()>0) {
		vector<string> lineVect = StringUtils::Split (line,"=");
		if (lineVect[0]=="live") {
		    strParams = strParams + " --live";
		}
		else {
		    if (rtmpStream==1) {
			if (lineVect.size()>1) {
			    strParams = strParams + " --"+lineVect[0]+"=\""+lineVect[1]+"\"";
			}
			else {
			    strParams = strParams + " --"+line;
			}
		    }
		    else {
			strParams = strParams + " --"+line;
		    }
		}
	    }
        }
    }
    
    if(rtmpStream==1)
    {
	strParams = " -r "+strParams;
	if (g_rtmpdumpPath.length()==0)
	{
	    XBMC->Log(LOG_ERROR,"Path to rtmpdump binary is not set. Please change addon configuration.");
	    entry.Status = PVR_STREAM_STOPPED;
            entry.Timer.state= PVR_TIMER_STATE_ERROR;
	    p_RecJob->updateJobEntry(entry);
	    s_triggerTimerUpdate = true;
	    return NULL;
	}
	   
	string strCommandLog = g_rtmpdumpPath+strParams;
	strCommand = g_rtmpdumpPath;
	XBMC->Log(LOG_NOTICE,"Starting rtmpdump: %s", strCommandLog.c_str());
    }
    else
    {
	strParams =  " -i \""+strParams+"\" "+g_ffmpegParams+" -f "+g_fileExtension+" -";
	if (g_ffmpegPath.length()==0)
	{
	    XBMC->Log(LOG_ERROR,"Path to ffmpeg binary is not set. Please change addon configuration.");
	    entry.Status = PVR_STREAM_STOPPED;
            entry.Timer.state= PVR_TIMER_STATE_ERROR;
	    p_RecJob->updateJobEntry(entry);
	    s_triggerTimerUpdate = true;
	    return NULL;
	}
	
	if (g_ffmpegParams.length()==0)
	{
	    XBMC->Log(LOG_ERROR,"Recompression params for ffmpeg are not set. Please change addon configuration.");
	    entry.Status = PVR_STREAM_STOPPED;
            entry.Timer.state= PVR_TIMER_STATE_ERROR;
	    p_RecJob->updateJobEntry(entry);
	    return NULL;
	}
	
	string strCommandLog = g_ffmpegPath+strParams;
	strCommand = g_ffmpegPath;
	XBMC->Log(LOG_NOTICE,"Starting ffmpeg: %s",strCommandLog.c_str());
    }
    
    //POSIX
    es.set_binary_mode(exec_stream_t::s_out);
    es.set_wait_timeout(exec_stream_t::s_in,g_streamTimeout*1000);
    es.set_wait_timeout(exec_stream_t::s_out,g_streamTimeout*1000);
    es.start( strCommand , strParams);

    XBMC->Log(LOG_NOTICE,"Set stream timeout: %d",g_streamTimeout);
    void *fileHandle;
    fileHandle = XBMC->OpenFileForWrite(videoFile.c_str(), true);
    
    string buffer;
    //int bytes_read;
    streamsize bytes_read;
    bool startTransmission = false;
    bool firstKilobyteReaded = false;
    time_t last_readed = time(NULL);
    while(true)
    {
	string buff;
	try {
	    getline( es.out(bytes_read), buff,'\n' ).good();
	    buffer = buffer+buff+"\n";
	    length=length+buff.size();
	    last_readed = time(NULL);
	    
	    if (startTransmission == false)
	    {
		t_startRecTime = time(NULL);
		startTransmission = true;
	    }
	    
	    if (firstKilobyteReaded==false)
	    {
		int loop;
		int pos = -1;
		int loop_end = buffer.size()-8-sizeof(double);
		for (loop=0; loop<loop_end;loop++)
		{
		    if (buffer[loop]=='d' && buffer[loop+1]=='u' && buffer[loop+2]=='r' && buffer[loop+3]=='a' && buffer[loop+4]=='t' && buffer[loop+5]=='i' && buffer[loop+6]=='o' && buffer[loop+7]=='n')
			pos = loop;
		} 
		if (pos>=0)
		{
		    pos = pos+9;
		    union
		    {
			unsigned char dc[8];
			double dd;
		    } d;
		    d.dd = t_duration;
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
	    }
	    
	    if (buffer.size()>1024)
	    {
		firstKilobyteReaded=true;
	    }
	    
	    if (firstKilobyteReaded==true)
	    {
		XBMC->WriteFile(fileHandle, buffer.c_str(), buffer.size());
		buffer.clear();
	    }
	}catch( std::exception const & e ) {
	    //nothing to read
	}
	
	p_RecJob->getJobEntry(t_iClientIndex, entry);
	
	now = time(NULL);
	if (now-last_readed>=g_streamTimeout)
	{
	    //something wrong - data not growing
	    es.close();
	    es.kill();
            XBMC->CloseFile(fileHandle);
               
            XBMC->Log(LOG_NOTICE, "Recording failed %s", entry.Timer.strTitle);
                   
            //Correct duration time
	    if (length>0)
	    {
		double duration = last_readed-t_startRecTime;
		CorrectDurationFLVFile (videoFile,duration);
	    }
	    else
	    {
		XBMC->DeleteFile(videoFile.c_str());
	    }
            entry.Status = PVR_STREAM_STOPPED;
            entry.Timer.state= PVR_TIMER_STATE_ERROR;
            p_RecJob->updateJobEntry(entry);
	    s_triggerTimerUpdate = true;
            return NULL;
	    
	}
        if (entry.Timer.endTime<time(NULL) || entry.Status==PVR_STREAM_IS_STOPPING || entry.Status==PVR_STREAM_STOPPED)
        {
	    es.close();
	    es.kill();
            XBMC->CloseFile(fileHandle);
                            
            XBMC->Log(LOG_NOTICE, "Recording stopped %s", entry.Timer.strTitle);
                   
            //Correct duration time
	    if (length>0)
	    {
		double duration = last_readed-t_startRecTime;
		CorrectDurationFLVFile (videoFile,duration);
	    }
	    else
	    {
		XBMC->DeleteFile(videoFile.c_str());
	    }
            entry.Status = PVR_STREAM_STOPPED;
            entry.Timer.state= PVR_TIMER_STATE_COMPLETED;
            p_RecJob->updateJobEntry(entry);
	    s_triggerTimerUpdate = true;
            return NULL;
        }
    }
    es.close();
    es.kill();
    XBMC->CloseFile(fileHandle);
    time_t end_time = time(NULL);
    //Correct duration time
    if (length>0)
    {
	double duration = end_time-t_startRecTime;
	CorrectDurationFLVFile (videoFile,duration);
    }
    else
    {
	//File is empty
	XBMC->DeleteFile(videoFile.c_str());
    }
    entry.Status = PVR_STREAM_STOPPED;
    entry.Timer.state= PVR_TIMER_STATE_COMPLETED;
    p_RecJob->updateJobEntry(entry);
    s_triggerTimerUpdate = true;

    return NULL;
	
}