#pragma once

#include <atomic>
#include <condition_variable>
#include <thread>
#include <mutex>
#include <string>

#include "HlsParser.h"
#include "HlsUpdater.h"
#include "data/HlsMediaPlaylist.h"
#include "queue/readerwriterqueue.h"

namespace iptvsimple
{
  namespace hls
  {
    static const int HLS_LIVE_START_OFFSET_SEC = 120;
    static const int HLS_SEGMENT_COUNT_BEFORE_READY = 1;

    class HlsDownloader
    {
    public:
      HlsDownloader(std::shared_ptr<HlsMediaPlaylist> mediaPlaylist, std::shared_ptr<HlsMediaPlaylist> audioMediaPlaylist,
                    moodycamel::BlockingReaderWriterQueue<std::string> &dataStreamQueue, std::atomic<bool> &isStreamerRunning,
                    std::mutex &dataReadyMutex, std::condition_variable &dataReadyCondition, std::atomic<bool> &readyForRead)
        : m_mediaPlaylist(mediaPlaylist), m_audioMediaPlaylist(audioMediaPlaylist), m_dataStreamQueue(dataStreamQueue),
          m_isStreamerRunning(isStreamerRunning), m_dataReadyMutex(dataReadyMutex), m_dataReadyCondition(dataReadyCondition), m_readyForRead(readyForRead) {};

      // Static Callback for mpegts/mpegts.c
      static size_t StaticWriteCallback(const uint8_t *buffer, size_t length, void *opaque);

      bool Start();
      bool Stop();
      bool IsRunning() { return m_running.load(); };

    private:

      bool Process();

      bool StreamVodHls();
      bool StreamVodSegment(std::shared_ptr<HlsMediaPlaylist> &mediaPlaylist, std::shared_ptr<HlsMediaSegment> &mediaSegment, ByteBuffer &segment);

      bool StreamLiveHls();
      bool StreamLiveSegment(std::shared_ptr<HlsMediaSegment> &mediaSegment, ByteBuffer &segment);
      void SkipFirstSegments();

      size_t WriteToStream(const uint8_t *buffer, size_t length);

      std::atomic<bool> m_running = { false };
      std::thread m_inputThread;
      std::condition_variable m_condition;

      std::condition_variable m_emptyCondition;
      std::condition_variable m_refreshCondition;

      std::mutex m_mutex;
      std::mutex m_sharedMutex;

      std::atomic<bool> &m_readyForRead;

      std::string m_outputFilePath;

      std::condition_variable &m_dataReadyCondition;
      std::mutex &m_dataReadyMutex;

      std::shared_ptr<hls::HlsMediaPlaylist> m_mediaPlaylist;
      std::shared_ptr<hls::HlsMediaPlaylist> m_audioMediaPlaylist;
      std::atomic<bool> &m_isStreamerRunning;

      HlsUpdater *m_hlsUpdater;

      uint64_t segmentCount = 0;

      moodycamel::BlockingReaderWriterQueue<std::string> &m_dataStreamQueue;
    };
  } // namespace hls
} //namespace iptvsimple