/*
 * Ported to C++ from: https://github.com/e2iplayer/hlsdl
 */

#include "HlsDownloader.h"

#include "../../client.h"

#include "HlsParser.h"
#include "data/HlsMediaPlaylist.h"
#include "mpegts/mpegts.h"
#include "utilities/AesUtils.h"
#include "utilities/WebUtils.h"

using namespace ADDON;
using namespace iptvsimple;
using namespace iptvsimple::hls;

bool HlsDownloader::Start()
{
  if (m_running)
    return true;

  m_running = true;
  m_inputThread = std::thread([&] { Process(); });

  XBMC->Log(LOG_DEBUG, "%s HlsDownloader: Started", __FUNCTION__);

  return true;
}

bool HlsDownloader::Stop()
{
  std::lock_guard<std::mutex> lock(m_mutex);
  if (m_running)
  {
    m_running = false;

    //When we stop release any blocked thread so execution can finish cleanly
    m_refreshCondition.notify_all();
    m_emptyCondition.notify_all();

    if (m_inputThread.joinable())
      m_inputThread.join();
  }

  XBMC->Log(LOG_DEBUG, "%s HlsDownloader: Stopped", __FUNCTION__);

  return true;
}

bool HlsDownloader::Process()
{
  if (m_mediaPlaylist->m_isEndlist)
  {
    if (!StreamVodHls())
      return false;
  }
  else
  {
    if (!StreamLiveHls())
      return false;
  }

  return true;
}

bool HlsDownloader::StreamVodHls()
{
  XBMC->Log(LOG_NOTICE, "%s HLS Stream Type: VOD", __FUNCTION__);

  uint64_t streamedDurationMs = 0;
  int64_t streamSize;
  ByteBuffer segment;
  ByteBuffer segmentAudio;

  std::shared_ptr<HlsMediaSegment> mediaSegment = m_mediaPlaylist->m_firstMediaSegment;
  std::shared_ptr<HlsMediaSegment> audioMediaSegment = nullptr;
  write_ctx_t outputContext = {StaticWriteCallback, reinterpret_cast<void *>(this)};
  merge_context_t mergeContext = {};

  if (m_audioMediaPlaylist)
  {
    audioMediaSegment = m_audioMediaPlaylist->m_firstMediaSegment;
    mergeContext.out = &outputContext;
  }

  while (mediaSegment && m_running && m_isStreamerRunning.load())
  {
    if (!StreamVodSegment(m_mediaPlaylist, mediaSegment, segment))
    {
      break;
    }

    // first segment should be TS for success merge
    if (audioMediaSegment && segment.len > TS_PACKET_LENGTH && segment.data[0] == TS_SYNC_BYTE)
    {
      if (!StreamVodSegment(m_audioMediaPlaylist, audioMediaSegment, segmentAudio))
      {
        break;
      }

      streamSize += merge_packets(&mergeContext, segment.data, segment.len, segmentAudio.data, segmentAudio.len);

      delete segmentAudio.data;
      audioMediaSegment = audioMediaSegment->m_next;
    }
    else
    {
      streamSize += WriteToStream(segment.data, segment.len);

      segmentCount++;

      if (segmentCount == HLS_SEGMENT_COUNT_BEFORE_READY)
      {
        std::unique_lock<std::mutex> dataReadyLock(m_dataReadyMutex);
        m_readyForRead = true;
        m_dataReadyCondition.notify_one();
        dataReadyLock.unlock();
      }
    }

    delete segment.data;

    streamedDurationMs += mediaSegment->m_durationMs;

    mediaSegment = mediaSegment->m_next;
  }

  XBMC->Log(LOG_DEBUG, "%s Total duration: %u, Stream duration: %u stream size: %lld", __FUNCTION__,
    static_cast<uint32_t>(m_mediaPlaylist->m_totalDurationMs / 1000),
    static_cast<uint32_t>(streamedDurationMs / 1000), streamSize);

  return true;
}

bool HlsDownloader::StreamVodSegment(std::shared_ptr<HlsMediaPlaylist> &mediaPlaylist, std::shared_ptr<HlsMediaSegment> &mediaSegment, ByteBuffer &segment)
{
  int retries = 0;
  bool haveData = false;
  char rangeBuffer[22];
  size_t size;
  std::string dataBuffer;

  XBMC->Log(LOG_DEBUG, "%s Start stream segment: %d", __FUNCTION__, mediaSegment->m_sequenceNumber);

  while (true)
  {
    size = 0;
    dataBuffer.clear();

    char *range = nullptr;
    if (mediaSegment->m_size > -1)
    {
      std::snprintf(rangeBuffer, sizeof(rangeBuffer), "%lld-%lld", mediaSegment->m_offset, mediaSegment->m_offset + mediaSegment->m_size - 1);
      range = rangeBuffer;
    }

    int httpCode = WebUtils::GetDataRange(mediaSegment->m_url, dataBuffer, &size, range, 1);

    if (!(httpCode == 200 || httpCode == 206) && m_running)
    {
      if (httpCode != 403 && httpCode != 401 && httpCode != 410 && retries <= OPEN_MAX_RETRIES)
      {
        retries += 1;
        XBMC->Log(LOG_DEBUG, "%s VOD retry segment %d stream, due to previous error. HTTP code: %d. Try %d of %d failed", __FUNCTION__, mediaSegment->m_sequenceNumber, httpCode, retries, OPEN_MAX_RETRIES);
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        continue;
      }
      break;
    }
    else
    {
      haveData = true;
      break;
    }
  }

  if (haveData)
  {
    segment.data = new uint8_t[size];
    std::copy(dataBuffer.begin(), dataBuffer.end(), segment.data);
    segment.len = static_cast<int>(size);

    if (mediaPlaylist->m_isEncrypted)
    {
      if (mediaPlaylist->m_encryptionType == EncryptionType::ENC_AES128)
      {
        XBMC->Log(LOG_DEBUG, "%s Streaming segment: %d, AES128 encrypted, size: %d, url: %s", __FUNCTION__, mediaSegment->m_sequenceNumber, segment.len, mediaSegment->m_url.c_str());

        if (!AesUtils::DecryptAes128(mediaSegment, segment))
          return false;
      }
      else if (mediaPlaylist->m_encryptionType == EncryptionType::ENC_AES_SAMPLE)
      {
        XBMC->Log(LOG_DEBUG, "%s Streaming segment: %d, SAMPLE AES encrypted, size: %d, url: %s", __FUNCTION__, mediaSegment->m_sequenceNumber, segment.len, mediaSegment->m_url.c_str());

        if (!AesUtils::DecryptSampleAes(mediaSegment, segment))
          return false;
      }
    }
    else
    {
      XBMC->Log(LOG_DEBUG, "%s Streaming segment: %d, unencrypted, size: %d, url: %s", __FUNCTION__, mediaSegment->m_sequenceNumber, segment.len, mediaSegment->m_url.c_str());
    }

    return true;
  }
  else
  {
    return false;
  }
}

bool HlsDownloader::StreamLiveHls()
{
  XBMC->Log(LOG_NOTICE, "%s HLS Stream Type: Live", __FUNCTION__);

  SkipFirstSegments();

  // start update thread
  HlsUpdater hlsUpdater(m_mediaPlaylist, m_running, m_sharedMutex, m_emptyCondition, m_refreshCondition);
  hlsUpdater.Start();

  ByteBuffer segment;
  uint64_t streamedDurationMs = 0;
  int64_t streamSize = 0;
  bool stream = true;

  while (stream && m_running && m_isStreamerRunning.load())
  {
    std::unique_lock<std::mutex> lock(m_sharedMutex);
    std::shared_ptr<HlsMediaSegment> mediaSegment = m_mediaPlaylist->m_firstMediaSegment;
    if (mediaSegment != nullptr)
    {
      m_mediaPlaylist->m_firstMediaSegment = mediaSegment->m_next;
      if (m_mediaPlaylist->m_firstMediaSegment)
        m_mediaPlaylist->m_firstMediaSegment->m_prev = nullptr;
    }
    else
    {
      m_mediaPlaylist->m_lastMediaSegment = nullptr;
      stream = !m_mediaPlaylist->m_isEndlist;
    }

    if (mediaSegment == nullptr)
    {
      if (stream)
      {
        m_refreshCondition.notify_one();
        m_emptyCondition.wait(lock);
      }
    }
    lock.unlock();

    if (mediaSegment == nullptr || !(m_running && m_isStreamerRunning.load()))
      continue;

    if (StreamLiveSegment(mediaSegment, segment))
    {
      streamSize += WriteToStream(segment.data, segment.len);

      segmentCount++;

      if (segmentCount == HLS_SEGMENT_COUNT_BEFORE_READY)
      {
        std::unique_lock<std::mutex> dataReadyLock(m_dataReadyMutex);
        m_readyForRead = true;
        m_dataReadyCondition.notify_one();
        dataReadyLock.unlock();
      }

      delete segment.data;
    }
    else
    {
      break;
    }
  }

  hlsUpdater.Stop();

  XBMC->Log(LOG_DEBUG, "%s Total duration: %u, Stream duration: %u stream size: %lld", __FUNCTION__,
    static_cast<uint32_t>(m_mediaPlaylist->m_totalDurationMs / 1000),
    static_cast<uint32_t>(streamedDurationMs / 1000), streamSize);

  return true;
}

bool HlsDownloader::StreamLiveSegment(std::shared_ptr<HlsMediaSegment> &mediaSegment, ByteBuffer &segment)
{
  int retries = 0;
  bool haveData = false;
  char rangeBuffer[22];
  size_t size;
  std::string dataBuffer;

  XBMC->Log(LOG_DEBUG, "%s Start stream segment: %d", __FUNCTION__, mediaSegment->m_sequenceNumber);

  while (true)
  {
    size = 0;
    dataBuffer.clear();

    char *range = nullptr;
    if (mediaSegment->m_size > -1)
    {
      std::snprintf(rangeBuffer, sizeof(rangeBuffer), "%lld-%lld", mediaSegment->m_offset, mediaSegment->m_offset + mediaSegment->m_size - 1);
      range = rangeBuffer;
    }

    int httpCode = WebUtils::GetDataRange(mediaSegment->m_url, dataBuffer, &size, range, 1);
    if (!(httpCode == 200 || httpCode == 206) && m_running)
    {
      int firstMediaSequence = 0;

      std::unique_lock<std::mutex> sequenceLock(m_sharedMutex);
      firstMediaSequence = m_mediaPlaylist->m_firstMediaSequence;
      sequenceLock.unlock();

      if (httpCode != 403 && httpCode != 401 && httpCode != 410 && retries <= OPEN_MAX_RETRIES &&
        mediaSegment->m_sequenceNumber > firstMediaSequence)
      {
        retries += 1;
        XBMC->Log(LOG_DEBUG, "%s Live retry segment %d stream, due to previous error. HTTP code: %d. Try %d of %d failed", __FUNCTION__, mediaSegment->m_sequenceNumber, httpCode, retries, OPEN_MAX_RETRIES);
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        continue;
      }
      else
      {
        XBMC->Log(LOG_NOTICE, "%s Live mode skipping segment %d", __FUNCTION__,  mediaSegment->m_sequenceNumber);
        break;
      }
    }
    else
    {
      haveData = true;
      break;
    }
  }

  if (haveData && m_running)
  {
    segment.data = new uint8_t[size];
    std::copy(dataBuffer.begin(), dataBuffer.end(), segment.data);
    segment.len = static_cast<int>(size);

    if (m_mediaPlaylist->m_isEncrypted)
    {
      if (m_mediaPlaylist->m_encryptionType == EncryptionType::ENC_AES128)
      {
        XBMC->Log(LOG_DEBUG, "%s Streaming segment: %d, AES128 encrypted, size: %d, url: %s", __FUNCTION__, mediaSegment->m_sequenceNumber, segment.len, mediaSegment->m_url.c_str());

        if (!AesUtils::DecryptAes128(mediaSegment, segment))
          return false;
      }
      else if (m_mediaPlaylist->m_encryptionType == EncryptionType::ENC_AES_SAMPLE)
      {
        XBMC->Log(LOG_DEBUG, "%s Streaming segment: %d, SAMPLE AES encrypted, size: %d, url: %s", __FUNCTION__, mediaSegment->m_sequenceNumber, segment.len, mediaSegment->m_url.c_str());

        if (!AesUtils::DecryptSampleAes(mediaSegment, segment))
          return false;
      }
    }
    else
    {
      XBMC->Log(LOG_DEBUG, "%s Streaming segment: %d, unencrypted, size: %d, url: %s", __FUNCTION__, mediaSegment->m_sequenceNumber, segment.len, mediaSegment->m_url.c_str());
    }

    return true;
  }
  else
  {
    return false;
  }
}

void HlsDownloader::SkipFirstSegments()
{
  // skip first segments
  if (m_mediaPlaylist->m_firstMediaSegment != m_mediaPlaylist->m_lastMediaSegment)
  {
    std::shared_ptr<HlsMediaSegment> mediaSegment = m_mediaPlaylist->m_lastMediaSegment;
    uint64_t durationMs = 0;
    uint64_t durationOffsetMs = HLS_LIVE_START_OFFSET_SEC * 1000;

    while (mediaSegment)
    {
      durationMs += mediaSegment->m_durationMs;
      if (durationMs >= durationOffsetMs)
        break;
      mediaSegment = mediaSegment->m_prev;
    }

    if (mediaSegment && mediaSegment != m_mediaPlaylist->m_firstMediaSegment)
    {
      // remove segments
      while (m_mediaPlaylist->m_firstMediaSegment != mediaSegment)
      {
        m_mediaPlaylist->m_firstMediaSegment = m_mediaPlaylist->m_firstMediaSegment->m_next;
      }
      mediaSegment->m_prev = nullptr;
      m_mediaPlaylist->m_firstMediaSegment = mediaSegment;
    }

    m_mediaPlaylist->m_totalDurationMs = HlsParser::GetDurationHlsMediaPlaylist(m_mediaPlaylist);
  }
}

// Static and object write callbacks for mpeg/mpegts.c

size_t HlsDownloader::StaticWriteCallback(const uint8_t *buffer, size_t length, void *opaque)
{
  HlsDownloader *downloader = reinterpret_cast<HlsDownloader*>(opaque);

  return downloader->WriteToStream(buffer, length);
}

size_t HlsDownloader::WriteToStream(const uint8_t *buffer, size_t length)
{
  std::string dataBuffer(reinterpret_cast<char const*>(buffer), length);
  m_dataStreamQueue.enqueue(dataBuffer);

  return length;
}