/*
 * Ported to C++ from: https://github.com/e2iplayer/hlsdl
 */

#include "HlsParser.h"

#include <algorithm>
#include <cstdio>
#include <inttypes.h>
#include <iostream>
#include <iterator>
#include <sstream>

#include "../../client.h"

#include "utilities/WebUtils.h"

#include "p8-platform/util/StringUtils.h"

using namespace ADDON;
using namespace iptvsimple::hls;

bool HlsParser::Parse()
{
  m_mediaPlaylist = nullptr;
  m_audioMediaPlaylist = std::make_shared<HlsMediaPlaylist>();

  std::string hlsFileSource;
  std::string url;

  int httpCode = WebUtils::GetData(m_streamUrl, hlsFileSource, url, OPEN_MAX_RETRIES);
  if (!(httpCode == 200 || httpCode == 206))
  {
    XBMC->Log(LOG_ERROR, "%s Could not open m3u8 URL: %s, HTTP Code: %d", __FUNCTION__, m_streamUrl.c_str(), httpCode);
    return false;
  }

  PlaylistType playlistType = GetPlaylistType(hlsFileSource);

  if (playlistType == PlaylistType::UNKNOWN)
  {
    XBMC->Log(LOG_ERROR, "%s Unknown playlist type for m3u8 URL: %s", __FUNCTION__, m_streamUrl.c_str());
    return false;
  }

  if (playlistType == PlaylistType::MASTER_PLAYLIST)
  {
    XBMC->Log(LOG_DEBUG, "%s Playlist type: Master", __FUNCTION__);

    m_masterPlaylist = std::make_shared<HlsMasterPlaylist>();
    m_masterPlaylist->m_source = hlsFileSource;
    m_masterPlaylist->m_url = url;

    if (!HandleHlsMasterPlaylist() && m_masterPlaylist->m_mediaPlaylist)
    {
      XBMC->Log(LOG_ERROR, "%s Error processing Master playlist or no child media playlist found for m3u8 URL: %s", __FUNCTION__, m_streamUrl.c_str());
      return false;
    }

    m_mediaPlaylist = m_masterPlaylist->m_mediaPlaylist;
    std::shared_ptr<HlsMediaPlaylist> tempMediaPlaylist = m_mediaPlaylist->m_next;

    while (tempMediaPlaylist) //select the best quality
    {
      if (tempMediaPlaylist->m_bitrate > m_mediaPlaylist->m_bitrate)
        m_mediaPlaylist = tempMediaPlaylist;
      tempMediaPlaylist = tempMediaPlaylist->m_next;
    }

    if (!m_mediaPlaylist)
    {
      XBMC->Log(LOG_ERROR, "%s Error selecting media playlist for m3u8 URL: %s", __FUNCTION__, m_streamUrl.c_str());
      return false;
    }

    XBMC->Log(LOG_DEBUG, "%s Chosen Quality - Bitrate: %u, AutdioGroup: %s, Resolution: %s, Codecs: '%s'", __FUNCTION__, m_mediaPlaylist->m_bitrate, m_mediaPlaylist->m_audioGroup.c_str(), m_mediaPlaylist->m_resolution.c_str(), m_mediaPlaylist->m_codecs.c_str());

    m_masterPlaylist->m_mediaPlaylist = nullptr; // we have our media playlist so discard from master
    m_mediaPlaylist->m_origUrl = m_mediaPlaylist->m_url;

    if (!m_mediaPlaylist->m_audioGroup.empty())
    {
      std::shared_ptr<HlsAudio> selectedAudio = nullptr;
      std::shared_ptr<HlsAudio> audio = m_masterPlaylist->m_audio;
      bool hasAudioPlaylist = false;

      if (audio)
      {
        while (audio)
        {
          if (audio->m_groupId == m_mediaPlaylist->m_audioGroup)
          {
            if (audio->m_groupId == m_mediaPlaylist->m_audioGroup && audio->m_isDefault)
            {
              selectedAudio = audio;
            }
          }

          audio = audio->m_next;
        }

        if (!selectedAudio) {
            XBMC->Log(LOG_ERROR, "%s Error selecting audio for m3u8 URL: %s", __FUNCTION__, m_streamUrl.c_str());
            return false;
        }

        m_masterPlaylist->m_audio = nullptr; // we have our audio so discard from master
        m_audioMediaPlaylist->m_origUrl = selectedAudio->m_url;
      }
    }
  }
  else // PlaylistType::MEDIA_PLAYLIST
  {
    XBMC->Log(LOG_DEBUG, "%s Playlist type: Media", __FUNCTION__);
    m_mediaPlaylist = std::make_shared<HlsMediaPlaylist>();

    m_mediaPlaylist->m_source = hlsFileSource;
    m_mediaPlaylist->m_bitrate = 0;
    m_mediaPlaylist->m_origUrl = m_streamUrl;
    m_mediaPlaylist->m_url = url;
  }

  if (!m_audioMediaPlaylist->m_origUrl.empty())
  {
    int httpCode = WebUtils::GetData(m_audioMediaPlaylist->m_origUrl, m_audioMediaPlaylist->m_source, m_audioMediaPlaylist->m_url, OPEN_MAX_RETRIES);
    if (!(httpCode == 200 || httpCode == 206))
    {
      XBMC->Log(LOG_ERROR, "%s Could not open audio playlist URL: %s", __FUNCTION__, m_audioMediaPlaylist->m_origUrl.c_str());
      return false;
    }

    if (GetPlaylistType(m_audioMediaPlaylist->m_source) != PlaylistType::MEDIA_PLAYLIST)
    {
      XBMC->Log(LOG_ERROR, "%s Audio Playlist URI is not a media playlistL: %s", __FUNCTION__, m_audioMediaPlaylist->m_source.c_str());
      return false;
    }
  }

  if (!HandleHlsMediaPlaylist(m_mediaPlaylist))
  {
    XBMC->Log(LOG_ERROR, "%s Error processing media playlist m3u8 URL: %s", __FUNCTION__, m_mediaPlaylist->m_url.c_str());
    return false;
  }

  if (!m_audioMediaPlaylist->m_url.empty() && !HandleHlsMediaPlaylist(m_audioMediaPlaylist))
  {
    XBMC->Log(LOG_ERROR, "%s Error processing audio media playlist URL: %s", __FUNCTION__, m_audioMediaPlaylist->m_url.c_str());
    return false;
  }

  if (m_mediaPlaylist->m_isEncrypted)
  {
    XBMC->Log(LOG_NOTICE, "%s HLS Stream is %s encrypted", __FUNCTION__, m_mediaPlaylist->m_encryptionType == EncryptionType::ENC_AES128 ? "AES-128" : "SAMPLE-AES");
  }

  XBMC->Log(LOG_DEBUG, "%s Media Playlist parsed successfully.", __FUNCTION__);

  std::shared_ptr<HlsMediaSegment> mediaSegment = m_mediaPlaylist->m_firstMediaSegment;
  while (mediaSegment)
  {
    XBMC->Log(LOG_DEBUG, "%s Media Segment URL: %s", __FUNCTION__, mediaSegment->m_url.c_str());
    mediaSegment = mediaSegment->m_next;
  }

  return true;
}

PlaylistType HlsParser::GetPlaylistType(std::string &source)
{
  if (!StringUtils::StartsWith(source, "#EXTM3U"))
    return PlaylistType::UNKNOWN;

  if (source.find("#EXT-X-STREAM-INF") != std::string::npos)
    return PlaylistType::MASTER_PLAYLIST;

  return PlaylistType::MEDIA_PLAYLIST;
}

bool HlsParser::HandleHlsMasterPlaylist()
{
  if (!m_masterPlaylist)
  {
    XBMC->Log(LOG_ERROR, "%s Master Playlist does not exist, Parse must have been unsuccessful or not run", __FUNCTION__);
    return false;
  }

  bool urlExpected = false;
  unsigned int bitrate = 0;
  std::string resolution;
  std::string codecs;
  std::string audioGroup;
  std::string attributeName;
  std::string attributeValue;

  std::istringstream stream(m_masterPlaylist->m_source);
  std::string line;
  int lineNumber = 0;
  while (std::getline(stream, line))
  {
    if (StringUtils::StartsWith(line, "#"))
    {
      if (StringUtils::StartsWith(line, HLS_MASTER_PLAYLIST_X_STREAM_PREFIX))
      {
        urlExpected = false;
        bitrate = 0;
        resolution.clear();
        codecs.clear();
        audioGroup.clear();

        line = line.substr(HLS_MASTER_PLAYLIST_X_STREAM_PREFIX.size(), line.size());

        while (GetNextAttribute(line, attributeName, attributeValue))
        {
          if (attributeName == "BANDWIDTH")
            std::sscanf(attributeValue.c_str(), "%u", &bitrate);
          else if (attributeName == "AUDIO")
            audioGroup = attributeValue;
          else if (attributeName == "RESOLUTION")
            resolution = attributeValue;
          else if (attributeName == "CODECS")
            codecs = attributeValue;
        }

        urlExpected = true;
      }
      else if (StringUtils::StartsWith(line, HLS_MASTER_PLAYLIST_X_MEDIA_AUDIO_PREFIX))
      {
        std::string groupId;
        std::string name;
        std::string lang;
        std::string url;
        bool isDefault;

        line = line.substr(HLS_MASTER_PLAYLIST_X_MEDIA_AUDIO_PREFIX.size(), line.size());

        while (GetNextAttribute(line, attributeName, attributeValue))
        {
          if (StringUtils::StartsWith(attributeName, "GROUP-ID"))
          {
            groupId = attributeValue;
          }
          else if (StringUtils::StartsWith(attributeName, "NAME"))
          {
            name = attributeValue;
          }
          else if (StringUtils::StartsWith(attributeName, "LANGUAGE"))
          {
            lang = attributeValue;
          }
          else if (StringUtils::StartsWith(attributeName, "URI"))
          {
            url = attributeValue;
          }
          else if (StringUtils::StartsWith(attributeName, "DEFAULT"))
          {
            if (StringUtils::StartsWith(attributeValue, "YES"))
              isDefault = true;
          }

          if (!groupId.empty() && !name.empty() && !url.empty())
          {
            std::shared_ptr<HlsAudio> audio = std::make_shared<HlsAudio>();
            audio->m_url = url;
            ExtendUrl(audio->m_url, m_masterPlaylist->m_url);

            audio->m_groupId = groupId;
            audio->m_name = name;
            audio->m_lang = lang;
            audio->m_isDefault = isDefault;

            if (m_masterPlaylist->m_audio)
              audio->m_next = m_masterPlaylist->m_audio;
            m_masterPlaylist->m_audio = audio;
          }
        }
      }
    }
    else if (urlExpected)
    {
      std::shared_ptr<HlsMediaPlaylist> mediaPlaylist = std::make_shared<HlsMediaPlaylist>();

      mediaPlaylist->m_url = line;
      ExtendUrl(mediaPlaylist->m_url, m_masterPlaylist->m_url);

      XBMC->Log(LOG_DEBUG, "%s Extended URL: %s", __FUNCTION__, mediaPlaylist->m_url.c_str());

      mediaPlaylist->m_bitrate = bitrate;
      mediaPlaylist->m_audioGroup = audioGroup;
      mediaPlaylist->m_resolution = !resolution.empty() ? resolution : "unknown";
      mediaPlaylist->m_codecs = !codecs.empty() ? codecs : "unknown";

      XBMC->Log(LOG_DEBUG, "%s Quality - Bitrate: %u, AutdioGroup: %s, Resolution: %s, Codecs: '%s'", __FUNCTION__, mediaPlaylist->m_bitrate, mediaPlaylist->m_audioGroup.c_str(), mediaPlaylist->m_resolution.c_str(), mediaPlaylist->m_codecs.c_str());

      if (m_masterPlaylist->m_mediaPlaylist) {
          mediaPlaylist->m_next = m_masterPlaylist->m_mediaPlaylist;
      }
      m_masterPlaylist->m_mediaPlaylist = mediaPlaylist;

      urlExpected = false;
    }
  }

  return true;
}

bool HlsParser::HandleHlsMediaPlaylist(std::shared_ptr<HlsMediaPlaylist> &mediaPlaylist)
{
  mediaPlaylist->m_isEncrypted = false;
  mediaPlaylist->m_encryptionType = EncryptionType::ENC_NONE;

  if (mediaPlaylist->m_source.empty())
  {
    int httpCode = WebUtils::GetData(mediaPlaylist->m_origUrl, mediaPlaylist->m_source, mediaPlaylist->m_url, OPEN_MAX_RETRIES);
    if (!(httpCode == 200 || httpCode == 206))
    {
      XBMC->Log(LOG_ERROR, "%s Could not open media playlist URL: %s", __FUNCTION__, mediaPlaylist->m_origUrl.c_str());
      return false;
    }
  }

  mediaPlaylist->m_firstMediaSegment = nullptr;
  mediaPlaylist->m_lastMediaSegment = nullptr;
  mediaPlaylist->m_targetDurationMs = 0;
  mediaPlaylist->m_isEndlist = false;
  mediaPlaylist->m_lastMediaSequence = 0;

  if (!MediaPlaylistGetLinks(mediaPlaylist))
  {
    XBMC->Log(LOG_ERROR, "%s Could not parse media playlist links URL: %s", __FUNCTION__, mediaPlaylist->m_origUrl.c_str());
    return false;
  }

  mediaPlaylist->m_totalDurationMs = GetDurationHlsMediaPlaylist(mediaPlaylist);

  return true;
}

bool HlsParser::MediaPlaylistGetLinks(std::shared_ptr<HlsMediaPlaylist> &mediaPlaylist)
{
  std::shared_ptr<HlsMediaSegment> mediaSegment = std::make_shared<HlsMediaSegment>();
  std::shared_ptr<HlsMediaSegment> currentMediaSegment;

  int64_t segmentOffset = 0;
  int64_t segmentSize = -1;

  XBMC->Log(LOG_DEBUG, "%s Starting to get links for: %s", __FUNCTION__, mediaPlaylist->m_origUrl.c_str());

  std::istringstream stream(mediaPlaylist->m_source);
  std::string line;
  int mediaSequenceCount = 0;
  while (std::getline(stream, line))
  {
    line = StringUtils::Trim(line);
    if (!line.empty())
    {
      if (StringUtils::StartsWith(line, "#"))
      {
        if (!ParseMediaPlaylistTags(mediaPlaylist, mediaSegment, line, &segmentOffset, &segmentSize))
        {
          XBMC->Log(LOG_ERROR, "%s Could not parse media playlist tags for line: %s", __FUNCTION__, line.c_str());
          return false;
        }
      }
      else
      {
        mediaSegment->m_url = StringUtils::Trim(line);
        mediaSegment->m_sequenceNumber = mediaSequenceCount + mediaPlaylist->m_firstMediaSequence;
        if (mediaPlaylist->m_encryptionType == EncryptionType::ENC_AES128 || mediaPlaylist->m_encryptionType == EncryptionType::ENC_AES_SAMPLE)
        {
          std::copy(std::begin(mediaPlaylist->m_encAes.m_keyValue), std::end(mediaPlaylist->m_encAes.m_keyValue), std::begin(mediaSegment->m_encAes.m_keyValue));
          std::copy(std::begin(mediaPlaylist->m_encAes.m_ivValue), std::end(mediaPlaylist->m_encAes.m_ivValue), std::begin(mediaSegment->m_encAes.m_ivValue));
          mediaSegment->m_encAes.m_keyUrl = mediaPlaylist->m_encAes.m_keyUrl;

          if (!mediaPlaylist->m_encAes.m_ivIsStatic)
          {
            char ivString[KEYLEN_HEX_CHAR];
            std::snprintf(ivString, KEYLEN_HEX_CHAR, "%032x\n", mediaSegment->m_sequenceNumber);
            HexStringToBinary(mediaSegment->m_encAes.m_ivValue, ivString, KEYLEN);
          }
        }

        /* Get full url */
        ExtendUrl(mediaSegment->m_url, mediaPlaylist->m_url);

        mediaSegment->m_size = segmentSize;
        if (segmentSize >= 0)
        {
            mediaSegment->m_offset = segmentOffset;
            segmentOffset += segmentSize;
            segmentSize = -1;
        }
        else
        {
            mediaSegment->m_offset = 0;
            segmentOffset = 0;
        }

        /* Add new segment to segment list */
        if (mediaPlaylist->m_firstMediaSegment == nullptr)
        {
            mediaPlaylist->m_firstMediaSegment = mediaSegment;
            currentMediaSegment = mediaSegment;
        }
        else
        {
            currentMediaSegment->m_next = mediaSegment;
            mediaSegment->m_prev = currentMediaSegment;
            currentMediaSegment = mediaSegment;
        }
        mediaSegment = std::make_shared<HlsMediaSegment>();

        mediaSequenceCount++;
      }
    }
  }

  mediaPlaylist->m_lastMediaSegment = currentMediaSegment;

  if (mediaSequenceCount > 0)
    mediaPlaylist->m_lastMediaSequence = mediaPlaylist->m_firstMediaSequence + mediaSequenceCount - 1;

  XBMC->Log(LOG_DEBUG, "%s Finished getting links for: %s", __FUNCTION__, mediaPlaylist->m_origUrl.c_str());

  return true;
}

void HlsParser::HexStringToBinary(uint8_t *data, char *hexString, int len)
{
  char *pos = hexString;

  for (int count = 0; count < len; count++)
  {
    char buf[3] = {pos[0], pos[1], 0};
    data[count] = static_cast<uint8_t>(std::strtol(buf, nullptr, KEYLEN));
    pos += 2;
  }
}

bool HlsParser::ParseMediaPlaylistTags(std::shared_ptr<HlsMediaPlaylist> &mediaPlaylist, std::shared_ptr<HlsMediaSegment> &mediaSegment, std::string &tag, int64_t *segmentOffset, int64_t *segmentSize)
{
  EncryptionType encryptionType;

  if (StringUtils::StartsWith(tag, HLS_MEDIA_PLAYLIST_X_KEY_AES128_PREFIX))
  {
    encryptionType = EncryptionType::ENC_AES128;
  }
  else if (StringUtils::StartsWith(tag, HLS_MEDIA_PLAYLIST_X_KEY_AES_SAMPLE_PREFIX))
  {
    encryptionType = EncryptionType::ENC_AES_SAMPLE;
  }
  else
  {
    if (StringUtils::StartsWith(tag, HLS_MEDIA_PLAYLIST_EXTINF_PREFIX))
    {
      tag = tag.substr(HLS_MEDIA_PLAYLIST_EXTINF_PREFIX.length(), tag.length());
      mediaSegment->m_durationMs = GetDurationMs(tag);
      return true;
    }
    else if (StringUtils::StartsWith(tag, HLS_MEDIA_PLAYLIST_X_ENDLIST_PREFIX))
    {
      mediaPlaylist->m_isEndlist = true;
      return true;
    }
    else if (StringUtils::StartsWith(tag, HLS_MEDIA_PLAYLIST_X_MEDIA_SEQUENCE_PREFIX))
    {
      tag = tag.substr(HLS_MEDIA_PLAYLIST_X_MEDIA_SEQUENCE_PREFIX.length(), tag.length());
      if(std::sscanf(tag.c_str(), "%d",  &(mediaPlaylist->m_firstMediaSequence)) == 1)
        return true;
    }
    else if (StringUtils::StartsWith(tag, HLS_MEDIA_PLAYLIST_X_TARGETDURATION_PREFIX))
    {
      tag = tag.substr(HLS_MEDIA_PLAYLIST_X_TARGETDURATION_PREFIX.length(), tag.length());
      mediaPlaylist->m_targetDurationMs = GetDurationMs(tag);
      return true;
    }
    else if (StringUtils::StartsWith(tag, HLS_MEDIA_PLAYLIST_X_BYTERANGE_PREFIX))
    {
      tag = tag.substr(HLS_MEDIA_PLAYLIST_X_BYTERANGE_PREFIX.length(), tag.length());

      *segmentSize = std::strtoll(tag.c_str(), nullptr, 10);

      std::size_t found = tag.find('@');
      *segmentOffset = 0;
      if (found != std::string::npos)
      {
        if (found + 1 < tag.length())
        {
          tag = tag.substr(0, found + 1);
          *segmentOffset = std::strtoll(tag.c_str(), nullptr, 10);
        }
      }

      return true;
    }

    return true;
  }

  mediaPlaylist->m_isEncrypted = true;
  mediaPlaylist->m_encryptionType = encryptionType;
  mediaPlaylist->m_encAes.m_ivIsStatic = false;

  char* linkToKey = new char[tag.length() + mediaPlaylist->m_url.length() + 10];
  char ivString[KEYLEN_HEX_CHAR] = {0};
  char sep = '\0';
  if ((std::sscanf(tag.c_str(), "#EXT-X-KEY:METHOD=AES-128,URI=\"%[^\"]\",IV=0%c%32[0-9a-f]", linkToKey, &sep, ivString) > 0 ||
       std::sscanf(tag.c_str(), "#EXT-X-KEY:METHOD=SAMPLE-AES,URI=\"%[^\"]\",IV=0%c%32[0-9a-f]", linkToKey, &sep, ivString) > 0))
  {
      if (sep == 'x' || sep == 'X')
      {
        HexStringToBinary(mediaPlaylist->m_encAes.m_ivValue, ivString, KEYLEN);
        mediaPlaylist->m_encAes.m_ivIsStatic = true;
      }

      std::string link = linkToKey;
      ExtendUrl(link, mediaPlaylist->m_url);
      mediaPlaylist->m_encAes.m_keyUrl = link;
  }

  delete linkToKey;

  return true;
}

uint64_t HlsParser::GetDurationMs(std::string &tag)
{
  std::string durationInSeconds = StringUtils::Trim(tag);
  size_t found = durationInSeconds.find(',');

  if (found != std::string::npos)
  {
    durationInSeconds = durationInSeconds.substr(0, found);
  }
  else
  {
    found = durationInSeconds.find(' ');
    if (found != std::string::npos)
      durationInSeconds = durationInSeconds.substr(0, found);
  }

  durationInSeconds = StringUtils::Trim(durationInSeconds);

  bool hasDot = false;
  found = durationInSeconds.find('.');
  if (found != std::string::npos)
    hasDot = true;

  uint64_t seconds = 0;
  uint64_t milliseconds = 0;

  if (hasDot)
    std::sscanf(durationInSeconds.c_str(), "%" SCNu64 ".%" SCNu64, &seconds, &milliseconds);
  else
    std::sscanf(durationInSeconds.c_str(), "%" SCNu64, &seconds);

  return (seconds * 1000) + milliseconds;
}

uint64_t HlsParser::GetDurationHlsMediaPlaylist(std::shared_ptr<HlsMediaPlaylist> &mediaPlaylist)
{
  uint64_t durationMs = 0;

  std::shared_ptr<HlsMediaSegment> mediaSegment = mediaPlaylist->m_firstMediaSegment;
  while(mediaSegment)
  {
    durationMs += mediaSegment->m_durationMs;
    mediaSegment = mediaSegment->m_next;
  }

  return durationMs;
}

bool HlsParser::GetNextAttribute(std::string &line, std::string &attributeName, std::string &attributeValue)
{
  line = StringUtils::TrimLeft(line, ", ");

  if (!line.empty())
  {
    std::size_t found = line.find('=');
    if (found != std::string::npos)
    {
      attributeName = line.substr(0, found);
      attributeName = StringUtils::Trim(attributeName);

      line = line.substr(found + 1, line.size());
      line = StringUtils::TrimLeft(line);

      found = line.find('"');
      if (found == 0) //look for next double quotes
      {
        line = line.erase(0, 1);
        line = StringUtils::TrimLeft(line);

        found = line.find('"');
        if (found != std::string::npos)
        {
          attributeValue = line.substr(0, found);
          attributeValue = StringUtils::Trim(attributeValue);

          line = line.substr(found + 1, line.size());
        }
        else
        {
          return false;
        }
      }
      else // look for next comma
      {
        found = line.find(',');
        if (found != std::string::npos)
        {
          attributeValue = line.substr(0, found);
          line = line.substr(found, line.size());
        }
        else
        {
          attributeValue = line;
          line.clear();
        }
      }

      return true;
    }
    else
    {
      return false;
    }
  }

  return false;
}

std::string HlsParser::GetM3U8Path(const std::string &url)
{
  std::string path;

  size_t found = url.find(".m3u8");
  if (found != std::string::npos)
  {
    path = url.substr(0, url.find(".m3u8"));

    path = path.substr(0, url.find_last_of("/") + 1);
  }

  return path;
}

bool HlsParser::ExtendUrl(std::string &url, const std::string &baseURL)
{
  std::string path = GetM3U8Path(baseURL);

  if (StringUtils::StartsWith(url, "http://") || StringUtils::StartsWith(url, "https://"))
  {
    return true;
  }
  else if (StringUtils::StartsWith(url, "/")) //It's relative to root of the domain/protocol
  {
    std::string protocol;
    std::string domain;

    std::size_t found = baseURL.find(':');
    if (found != std::string::npos)
    {
      protocol = baseURL.substr(0, found);

      if (found + 3 < baseURL.size())// after ://
      {
        domain = baseURL.substr(found + 3, baseURL.size()); // after ://
        found = domain.find('/');
        if (found != std::string::npos)
          domain = domain.substr(0, found);

        if (StringUtils::StartsWith(url, "//")) //protocol relative
          url = StringUtils::Format("%s:%s", protocol.c_str(), url.c_str());
        else //domain relative
          url = StringUtils::Format("%s://%s%s", protocol.c_str(), domain.c_str(), url.c_str());
      }
    }

    return true;
  }
  else
  {
    if (!path.empty()) //If we have a path already
    {
      url = path + url;
    }
    else //Try ? and /../
    {
      std::size_t found = baseURL.find('?');
      if (found != std::string::npos)
        path = baseURL.substr(0, found);
      url = path + "/../" + url;
    }

    return true;
  }
}

bool HlsParser::IsValidM3U8Url(std::string &streamUrl)
{
  //First check if the file contains .m3u8
  bool validM3U8Url = !HlsParser::GetM3U8Path(streamUrl).empty();

  if (!validM3U8Url)
    validM3U8Url = IsM3U8Playlist(streamUrl);

  return validM3U8Url;
}

bool HlsParser::IsM3U8Playlist(std::string &streamUrl)
{
  std::string source;
  std::string url;

  int httpCode = WebUtils::GetData(streamUrl, source, url, OPEN_MAX_RETRIES, true);
  if (!(httpCode == 200 || httpCode == 206))
  {
    XBMC->Log(LOG_ERROR, "%s Could not open URL: %s", __FUNCTION__, streamUrl.c_str());
    return false;
  }

  return StringUtils::StartsWith(source, "#EXTM3U");
}
