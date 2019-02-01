/*
 * Ported to C++ from: https://github.com/e2iplayer/hlsdl
 */

#include "HlsUpdater.h"

#include "../../client.h"

#include "HlsParser.h"
#include "utilities/WebUtils.h"
#include "data/HlsMediaPlaylist.h"

using namespace ADDON;
using namespace iptvsimple;
using namespace iptvsimple::hls;

bool HlsUpdater::Start()
{
  if (m_running)
    return true;

  m_running = true;
  m_inputThread = std::thread([&] { Process(); });

  XBMC->Log(LOG_DEBUG, "%s HlsUpdater: Started", __FUNCTION__);

  return true;
}

bool HlsUpdater::Stop()
{
  std::lock_guard<std::mutex> lock(m_mutex);
  if (m_running)
  {
    m_running = false;

    if (m_inputThread.joinable())
      m_inputThread.join();
  }

  XBMC->Log(LOG_DEBUG, "%s HlsUpdater: Stopped", __FUNCTION__);

  return true;
}

bool HlsUpdater::Process()
{
  bool isEndlist = false;
  int refreshDelaySeconds = 0;

  // no lock is needed here because download_live_hls not change this fields
  isEndlist = m_mediaPlaylist->m_isEndlist;
  refreshDelaySeconds = static_cast<int>(m_mediaPlaylist->m_targetDurationMs / 1000);

  if (refreshDelaySeconds > HLS_MAX_REFRESH_DELAY_SEC)
    refreshDelaySeconds = HLS_MAX_REFRESH_DELAY_SEC;
  else if (refreshDelaySeconds < HLS_MIN_REFRESH_DELAY_SEC)
    refreshDelaySeconds = HLS_MIN_REFRESH_DELAY_SEC;

  bool firstRun = true;;

  while (!isEndlist && m_running && m_isDownloaderRunning.load())
  {
    // wait for singal from streamer or the refresh delay
    std::unique_lock<std::mutex> lock(m_sharedMutex);
    if (!firstRun)
      m_refreshCondition.wait_for(lock, std::chrono::seconds(refreshDelaySeconds));
    lock.unlock();

    firstRun = false;

    // update playlist
    std::shared_ptr<HlsMediaPlaylist> newMediaPlaylist = std::make_shared<HlsMediaPlaylist>();
    int httpCode = WebUtils::GetData(m_mediaPlaylist->m_url, newMediaPlaylist->m_source, newMediaPlaylist->m_url, OPEN_MAX_RETRIES);
    if (httpCode == 200 || httpCode == 206)
    {
      if (HlsParser::MediaPlaylistGetLinks(newMediaPlaylist))
      {
        // no mutex is needed here because download_live_hls not change this fields
        if (newMediaPlaylist->m_isEndlist ||
            newMediaPlaylist->m_firstMediaSequence != m_mediaPlaylist->m_firstMediaSequence ||
            newMediaPlaylist->m_lastMediaSequence != m_mediaPlaylist->m_lastMediaSequence)
        {
          bool listExtended = false;

          // we need to update list
          std::unique_lock<std::mutex> updateLock(m_sharedMutex);

          m_mediaPlaylist->m_isEndlist = newMediaPlaylist->m_isEndlist;
          isEndlist = newMediaPlaylist->m_isEndlist;
          m_mediaPlaylist->m_firstMediaSequence = newMediaPlaylist->m_firstMediaSequence;

          if (newMediaPlaylist->m_lastMediaSequence > m_mediaPlaylist->m_lastMediaSequence)
          {
            // add new segments
            std::shared_ptr<HlsMediaSegment> mediaSegment = newMediaPlaylist->m_firstMediaSegment;
            while (mediaSegment)
            {
              if (mediaSegment->m_sequenceNumber > m_mediaPlaylist->m_lastMediaSequence)
              {
                if (mediaSegment->m_prev)
                {
                  mediaSegment->m_prev->m_next = nullptr;
                }
                mediaSegment->m_prev = nullptr;

                if (m_mediaPlaylist->m_lastMediaSegment)
                {
                    m_mediaPlaylist->m_lastMediaSegment->m_next = mediaSegment;
                }
                else
                {
                  m_mediaPlaylist->m_firstMediaSegment = mediaSegment;
                }

                m_mediaPlaylist->m_lastMediaSegment = newMediaPlaylist->m_lastMediaSegment;
                m_mediaPlaylist->m_lastMediaSequence = newMediaPlaylist->m_lastMediaSequence;

                if (mediaSegment == newMediaPlaylist->m_firstMediaSegment)
                {
                  // all segments are new
                  newMediaPlaylist->m_firstMediaSegment = nullptr;
                  newMediaPlaylist->m_lastMediaSegment = nullptr;
                }

                while (mediaSegment)
                {
                  m_mediaPlaylist->m_totalDurationMs += mediaSegment->m_durationMs;
                  mediaSegment = mediaSegment->m_next;
                }

                listExtended = true;
                break;
              }

              mediaSegment = mediaSegment->m_next;
            }
          }
          if (listExtended)
          {
            //Tell the streamer it can startng reading again
            m_emptyCondition.notify_one();
          }
          updateLock.unlock();
        }
      }
      else
      {
        XBMC->Log(LOG_NOTICE, "%s Failed to update media playlist: %s", __FUNCTION__, m_mediaPlaylist->m_url.c_str());
      }
    }
    else
    {
      XBMC->Log(LOG_ERROR, "%s Could not open media playlist URL: %s", __FUNCTION__, m_mediaPlaylist->m_url.c_str());
    }
  }

  return true;
}