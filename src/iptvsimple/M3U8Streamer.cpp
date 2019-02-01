#include "M3U8Streamer.h"

#include "../client.h"

#include "hls/data/HlsMediaPlaylist.h"
#include "hls/utilities/WebUtils.h"

using namespace ADDON;
using namespace iptvsimple;
using namespace iptvsimple::hls;

M3U8Streamer::M3U8Streamer(const std::string &streamUrl, int readTimeout) : m_streamUrl(streamUrl)
{
  m_readTimeout = (readTimeout) ? readTimeout : DEFAULT_READ_TIMEOUT_SECONDS;
}

M3U8Streamer::~M3U8Streamer(void)
{
  Stop();

  XBMC->Log(LOG_DEBUG, "%s M3U8Streamer: Destroyed", __FUNCTION__);
}

bool M3U8Streamer::Start()
{
  if (m_running)
    return true;

  m_running = true;

  HlsParser hlsParser(m_streamUrl);

  if (hlsParser.Parse())
  {
    m_inputThread = std::thread([&] { Process(hlsParser); });

    std::unique_lock<std::mutex> lock(m_dataReadyMutex);
    m_dataReadyCondition.wait_for(lock, std::chrono::seconds(m_readTimeout));

    XBMC->Log(LOG_DEBUG, "%s M3U8Streamer: Started", __FUNCTION__);

    return m_readyForRead.load();
  }
  else
  {
    m_running = false;

    XBMC->Log(LOG_ERROR, "%s M3U8Streamer: Not Started - Parse failed", __FUNCTION__);

    return false;
  }
}

bool M3U8Streamer::Stop()
{
  std::unique_lock<std::mutex> lock(m_mutex);
  if (m_running)
  {
    m_running = false;

    m_hlsDownloader->Stop();
    delete m_hlsDownloader;

    if (m_inputThread.joinable())
      m_inputThread.join();
  }

  XBMC->Log(LOG_DEBUG, "%s M3U8Streamer: Stopped", __FUNCTION__);

  return true;
}

bool M3U8Streamer::Process(HlsParser &hlsParser)
{
  m_hlsDownloader = new HlsDownloader(hlsParser.GetMediaPlaylist(), hlsParser.GetAudioMediaPlaylist(), m_dataStreamQueue,
                                        m_running, m_dataReadyMutex, m_dataReadyCondition, m_readyForRead);
  m_hlsDownloader->Start();

  while (m_running && m_hlsDownloader->IsRunning())
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  return true;
}

ssize_t M3U8Streamer::ReadStreamData(unsigned char *buffer, unsigned int size)
{
  int totalRead = 0;

  while (totalRead < size)
  {
    if (m_dataBufferPosition == 0)
    {
      if (!m_dataStreamQueue.wait_dequeue_timed(m_currentDataBuffer, m_readTimeout))
      {
        //No data to read so return totalRead so far
        return totalRead;
      }
    }

    bool dataBufferEmpty = false;
    int bytesToRead = 0;

    std::string::iterator begin = m_currentDataBuffer.begin() + m_dataBufferPosition;

    if (m_currentDataBuffer.size() - m_dataBufferPosition > size - totalRead)
    {
      bytesToRead = size - totalRead;
      m_dataBufferPosition += bytesToRead;
    }
    else
    {
      bytesToRead = m_currentDataBuffer.size() - m_dataBufferPosition;
      m_dataBufferPosition = 0;
      dataBufferEmpty = true;
    }

    std::copy(begin, begin + bytesToRead, buffer + totalRead);

    totalRead += bytesToRead;

    if (dataBufferEmpty)
      m_currentDataBuffer.clear();
  }

  m_totalBytesStreamed += totalRead;

  return totalRead;
}