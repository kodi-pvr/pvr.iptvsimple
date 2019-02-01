#pragma once

#include <atomic>
#include <condition_variable>
#include <thread>
#include <mutex>
#include <string>

#include "hls/HlsDownloader.h"
#include "hls/HlsParser.h"
#include "hls/queue/readerwriterqueue.h"

#include "libXBMC_addon.h"

namespace iptvsimple
{
  static const int DEFAULT_READ_TIMEOUT_SECONDS = 30;

  enum class StreamMode : int
  {
    BY_FILE = 0,
    IN_MEMORY = 1
  };

  class M3U8Streamer
  {
  public:
    M3U8Streamer(const std::string &streamUrl, int readTimeout);
    ~M3U8Streamer(void);

    bool Start();
    bool Stop();
    bool IsRunning() { return m_running.load(); };
    bool Process(hls::HlsParser &hlsParser);

    ssize_t ReadStreamData(unsigned char *buffer, unsigned int size);
    long long GetTotalBytesStreamed() { return m_totalBytesStreamed; };

    bool SwitchStream(const std::string &newStreamUrl);

  private:
    std::atomic<bool> m_running = { false };
    std::thread m_inputThread;
    std::condition_variable m_condition;
    std::mutex m_mutex;

    std::atomic<bool> m_readyForRead = { false };
    std::condition_variable m_dataReadyCondition;
    std::mutex m_dataReadyMutex;

    std::string m_streamUrl;
    hls::HlsDownloader *m_hlsDownloader;

    int m_readTimeout;

    moodycamel::BlockingReaderWriterQueue<std::string> m_dataStreamQueue;
    int m_dataBufferPosition = 0;
    long long m_totalBytesStreamed = 0;
    std::string m_currentDataBuffer = "";
  };
} //namespace iptvsimple