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
#include "p8-platform/threads/threads.h"
#include <map>
#include "PVRIptvData.h"
#include <string>

using namespace std;
using namespace ADDON;

class PVRRecJob
{
public:
    PVRRecJob(void);
    ~PVRRecJob(void);
    map<int,PVR_REC_JOB_ENTRY> getEntryData(void);
    bool addJobEntry(const PVR_REC_JOB_ENTRY &RecJobEntry);
    bool getJobEntry(const int ientryIndex, PVR_REC_JOB_ENTRY &entry);
    bool getJobEntry(const string strChannelName, PVR_REC_JOB_ENTRY &entry);
    bool updateJobEntry(const PVR_REC_JOB_ENTRY &RecJobEntry);
    bool delJobEntry(const int ientryIndex);
    bool getProperlyChannel (PVR_REC_JOB_ENTRY &entry, PVRIptvChannel &currentChannel);
    bool setLock (void);
    void setUnlock (void);
private:
    map <int,PVR_REC_JOB_ENTRY> m_JobEntryData;
    bool loadData(void);
    bool storeEntryData (void);
    string GetJobString(const PVR_REC_JOB_ENTRY &RecJobEntry);
    bool ParseJobString(string BuffStr, PVR_REC_JOB_ENTRY &RecJobEntry);
};
