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

#include "PVRIptvData.h"
#include "platform/util/StringUtils.h"
#include <map>
#include "platform/threads/threads.h"

using namespace ADDON;
using namespace std;

extern void CloseRecordingThreads(void);
string inttostr (int i);
int strtoint(string s);

typedef enum
{
    PVR_STREAM_NO_STREAM              = 0,
    PVR_STREAM_START_RECORDING        = 1,
    PVR_STREAM_IS_RECORDING           = 2,
    PVR_STREAM_IS_STOPPING            = 3,
    PVR_STREAM_STOPPED                = 4
} PVR_STREAM_STATUS;

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

struct PVR_REC_JOB_ENTRY {
    PVR_STREAM_STATUS           Status;
    string                      strChannelName;
    PVR_TIMER                   Timer;
    void*                       ThreadPtr;
};

class PVRSimpleRecorderThread : PLATFORM::CThread {
    public: 
    PVRSimpleRecorderThread(PVRIptvChannel &currentChannel,int iClientIndex);
    virtual ~PVRSimpleRecorderThread(void);
    virtual void StopThread(bool bWait = true);
    virtual void *Process(void);
    private:
    
    void CorrectBufferDurationFlV (char* buffer, const double &duration, const int &pos);
    void CorrectDurationFLVFile (const string &videoFile, const double &duration);
    
    int t_iClientIndex;
    PVRIptvChannel t_currentChannel;
    time_t t_startRecTime;
};

class PVRRecJob {
public:
    PVRRecJob(PVRIptvData *data);
    ~PVRRecJob(void);
    map<int,PVR_REC_JOB_ENTRY> getEntryData(void);
    bool addJobEntry(const PVR_REC_JOB_ENTRY &RecJobEntry);
    bool getJobEntry(const int ientryIndex, PVR_REC_JOB_ENTRY &entry, PVRIptvChannel &currentChannel);
    bool getJobEntry(const string strChannelName, PVR_REC_JOB_ENTRY &entry, PVRIptvChannel &currentChannel);
    bool updateJobEntry(const PVR_REC_JOB_ENTRY &RecJobEntry, PVRIptvChannel &currentChannel);
    bool delJobEntry(const int ientryIndex, PVRIptvChannel &currentChannel);
    bool getProperlyChannel (PVR_REC_JOB_ENTRY &entry, PVRIptvChannel &currentChannel);
    bool setLock (void);
    void setUnlock (void);
private:
    PVRIptvData *m_data;
    map <int,PVR_REC_JOB_ENTRY> m_JobEntryData;
    bool loadData(void);
    bool storeEntryData (void);
    string GetJobString(const PVR_REC_JOB_ENTRY &RecJobEntry);
    bool ParseJobString(string BuffStr, PVR_REC_JOB_ENTRY &RecJobEntry);
};

class PVRSimpleRecorder {
public:
    PVRSimpleRecorder (PVRIptvData *data, const std::string &recordingsPath);
    PVR_ERROR AddTimer (const PVR_TIMER &timer);
    PVR_ERROR DeleteTimer(const PVR_TIMER &timer, bool bForceDelete);
    PVR_ERROR UpdateTimer(const PVR_TIMER &timer);
    PVR_ERROR GetTimers(ADDON_HANDLE handle);
    int GetTimersAmount(void);
private:
    std::string m_recordingsPath = "";
    int m_maxRecordingTime = 240;
    PVRIptvData *m_data;
    void updateRecordingStatus();
    bool startRecording (const PVR_REC_JOB_ENTRY &RecJobEntry, PVRIptvChannel &currentChannel);
    bool stopRecording (const PVR_REC_JOB_ENTRY &RecJobEntry, PVRIptvChannel &currentChannel);
};
