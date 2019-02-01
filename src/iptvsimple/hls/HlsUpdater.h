#pragma once

#include <atomic>
#include <condition_variable>
#include <thread>
#include <mutex>
#include <string>

#include "data/HlsMediaPlaylist.h"

namespace iptvsimple
{
  namespace hls
  {
    static const int HLS_MIN_REFRESH_DELAY_SEC = 0;
    static const int HLS_MAX_REFRESH_DELAY_SEC = 5;

    class HlsUpdater
    {
    public:
      HlsUpdater(std::shared_ptr<HlsMediaPlaylist> mediaPlaylist, std::atomic<bool> &isDownloaderRunning,
                std::mutex &sharedMutex, std::condition_variable &emptyCondition, std::condition_variable &refreshCondition)
        : m_mediaPlaylist(mediaPlaylist), m_isDownloaderRunning(isDownloaderRunning), m_sharedMutex(sharedMutex),
          m_emptyCondition(emptyCondition), m_refreshCondition(refreshCondition) {};

      bool Start();
      bool Stop();
      bool IsRunning() { return m_running.load(); };

    private:

      bool Process();

      std::atomic<bool> m_running = { false };
      std::thread m_inputThread;
      std::condition_variable &m_emptyCondition;
      std::condition_variable &m_refreshCondition;
      std::mutex m_mutex;
      std::mutex &m_sharedMutex;

      std::atomic<bool> &m_isDownloaderRunning;

      std::shared_ptr<HlsMediaPlaylist> m_mediaPlaylist;
    };
  } // namespace hls
} //namespace iptvsimple