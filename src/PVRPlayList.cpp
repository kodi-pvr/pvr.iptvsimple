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
#include <vector>
#include <string>
#include "PVRIptvData.h"
#include "PVRPlayList.h"
#include "PVRUtils.h"

using namespace std;
using namespace ADDON;

extern int g_streamQuality;
bool PVRPlayList::GetPlaylist (string &strStreamUrl, vector<string> &vstrList)
{
    vstrList.clear();
    //find url for chunks
    vector<string> vstrUrlElems;
    vstrUrlElems = StringUtils::Split (strStreamUrl,"/");
    vstrUrlElems.pop_back();
    vector <string>::iterator it ( vstrList.begin() );
    string strUrl; 
    for (it=vstrUrlElems.begin(); it!=vstrUrlElems.end(); it++)
    {
        strUrl.append(*it);
        strUrl.append("/");
    }
    
    string strContent;
    void* fileHandle = XBMC->OpenFile(strStreamUrl.c_str(), 0);
    bool start = true;
    if (fileHandle)
    {
        char buffer[1024];
        while (int bytesRead = XBMC->ReadFile(fileHandle, buffer, 1024))
        {
            strContent.append(buffer, bytesRead);
            //Check if it is m38u list
            if (start==true)
            {
                vector<string> vstrElems;
                vstrElems = StringUtils::Split (strContent,"#EXTM3U");
                if (vstrElems.size()<2)
                {
                    //it is not m3u8 list
                    XBMC->Log(LOG_NOTICE, "Standard m3u8 list not found");
                    XBMC->CloseFile(fileHandle);
                    return false; 
                }
                start = false;
            }
        }
        XBMC->CloseFile(fileHandle);
        
    }
    else {
        return false;
    }
    
    vector<string> vstrLines;
    vstrLines = StringUtils::Split (strContent,"#EXT-X-STREAM-INF:");
    if (vstrLines.size()>1)
    {
        //Found m3u8 list with banwitch
        XBMC->Log(LOG_NOTICE, "Found standard m3u8 list with bandwitch switcher");
        bool found = false;
        
        int maxBandwith =0;
        int minBandwith =0;
        string PlaylistUrl;
        bool ret = false;
        
        bool found_min = false;
        if (g_streamQuality==0)
        {
            XBMC->Log(LOG_NOTICE, "Searching for low quality input stream");
        }
        else
        {
            XBMC->Log(LOG_NOTICE, "Searching for hi quality input stream");
        }
   
        vector <string>::iterator it ( vstrLines.begin() );
        for (it=vstrLines.begin(); it!=vstrLines.end(); it++)
        {
            if (found==false)
            {
                found = true;
                continue;
            }
            vector<string> vstrElems;
            vstrElems = StringUtils::Split (*it,"\n");
            
            //Find list url
            if (vstrElems[1].size()>0)
            {
                string strTrimedElem = StringUtils::Trim(vstrElems[1]);
                string strStream;
                if (!StringUtils::StartsWith(strTrimedElem,strUrl))
                {
                    strStream = strUrl;
                    strStream.append(strTrimedElem);
                }
                else
                    strStream = strTrimedElem;
                //vstrList.push_back(strStream);
                XBMC->Log(LOG_NOTICE, "Found m3u8 address: %s",strStream.c_str());
                
                //Find list params
                vector <string> vstrParamsElems = StringUtils::Split (vstrElems[0],",");
                vector <string>::iterator itparam ( vstrParamsElems.begin() );
                
                for (itparam=vstrLines.begin(); itparam!=vstrLines.end(); itparam++)
                {
                    vector<string> vstrParamElems = StringUtils::Split (*it,"=");
                    if (StringUtils::Trim(vstrParamElems[0])=="BANDWIDTH")
                    {
                        int myBandwith = strtoint(StringUtils::Trim(vstrParamElems[1]));
                        XBMC->Log(LOG_NOTICE, "Found BANDWIDTH: %d",myBandwith );

                        if (found_min==false)
                        {
                            found_min = true;
                            minBandwith= myBandwith;
                            PlaylistUrl = strStream;
                            ret = true;
                            
                        }
                        if (g_streamQuality==0)
                        {
                            if (myBandwith<minBandwith)
                            {
                                minBandwith= myBandwith;
                                PlaylistUrl = strStream;
                                ret = true;
                            }
                        }
                        else
                        {
                            if (myBandwith>maxBandwith)
                            {
                                maxBandwith= myBandwith;
                                PlaylistUrl = strStream;
                                ret = true;
                            }
                        }
                    }
                }
            }
        }
        
        if (ret==true)
        {
            ret = GetPlaylist(PlaylistUrl,vstrList);
            strStreamUrl = PlaylistUrl;
        }
        return ret;
    }
    else
    {
        //Found standard list
        vstrLines = StringUtils::Split (strContent,"#EXTINF:");
        if (vstrLines.size()>1)
        {
            //XBMC->Log(LOG_NOTICE, "Found standard m3u8 list");
            bool found = false;
            vector <string>::iterator it ( vstrLines.begin() );
            for (it=vstrLines.begin(); it!=vstrLines.end(); it++)
            {
                if (found==false)
                {
                    found = true;
                    continue;
                }
                vector<string> vstrElems;
                vstrElems = StringUtils::Split (*it,"\n");
                if (vstrElems[1].size()>0)
                {
                    string strTrimedElem = StringUtils::Trim(vstrElems[1]);
                    string strStream;
                    if (!StringUtils::StartsWith(strTrimedElem,strUrl))
                    {
                        strStream = strUrl;
                        strStream.append(strTrimedElem);
                    }
                    else
                        strStream = strTrimedElem;
                    vstrList.push_back(strStream);
                    //XBMC->Log(LOG_NOTICE, "Found chunk: %s",strStream.c_str());
                }
                
            }
            return true;
        }
        else return false;
        
    }
    return true;
}
