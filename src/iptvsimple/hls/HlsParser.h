#pragma once

#include <atomic>
#include <condition_variable>
#include <map>
#include <mutex>
#include <string>
#include <thread>

#include "data/ByteBuffer.h"
#include "data/EncAes128.h"
#include "data/HlsAudio.h"
#include "data/HlsMasterPlaylist.h"
#include "data/HlsMediaPlaylist.h"
#include "data/HlsMediaSegment.h"

namespace iptvsimple
{
  namespace hls
  {
    static const std::string HLS_MASTER_PLAYLIST_X_STREAM_PREFIX = "#EXT-X-STREAM-INF:";
    static const std::string HLS_MASTER_PLAYLIST_X_MEDIA_AUDIO_PREFIX = "#EXT-X-MEDIA:TYPE=AUDIO,";

    static const std::string HLS_MEDIA_PLAYLIST_EXTM3U_PREFIX = "#EXTM3U";
    static const std::string HLS_MEDIA_PLAYLIST_X_KEY_AES128_PREFIX = "#EXT-X-KEY:METHOD=AES-128";
    static const std::string HLS_MEDIA_PLAYLIST_X_KEY_AES_SAMPLE_PREFIX = "#EXT-X-KEY:METHOD=SAMPLE-AES";
    static const std::string HLS_MEDIA_PLAYLIST_EXTINF_PREFIX = "#EXTINF:";
    static const std::string HLS_MEDIA_PLAYLIST_X_ENDLIST_PREFIX = "#EXT-X-ENDLIST";
    static const std::string HLS_MEDIA_PLAYLIST_X_MEDIA_SEQUENCE_PREFIX = "#EXT-X-MEDIA-SEQUENCE:";
    static const std::string HLS_MEDIA_PLAYLIST_X_TARGETDURATION_PREFIX = "#EXT-X-TARGETDURATION:";
    static const std::string HLS_MEDIA_PLAYLIST_X_BYTERANGE_PREFIX = "#EXT-X-BYTERANGE:";

    enum class PlaylistType : int
    {
      UNKNOWN = -1,
      MASTER_PLAYLIST = 0,
      MEDIA_PLAYLIST = 1
    };

    class HlsParser
    {
    public:
      HlsParser(std::string &streamUrl) : m_streamUrl(streamUrl) {};
      ~HlsParser(void) = default;

      bool Parse();

      static uint64_t GetDurationHlsMediaPlaylist(std::shared_ptr<HlsMediaPlaylist> &mediaPlaylist);
      static bool MediaPlaylistGetLinks(std::shared_ptr<HlsMediaPlaylist> &mediaPlaylist);

      std::string& GetStreamUrl() { return m_streamUrl; };
      std::shared_ptr<HlsMasterPlaylist>& GetMasterPlaylist() { return m_masterPlaylist; };
      std::shared_ptr<HlsMediaPlaylist>& GetMediaPlaylist() { return m_mediaPlaylist; };
      std::shared_ptr<HlsMediaPlaylist>& GetAudioMediaPlaylist() { return m_audioMediaPlaylist; };

      static std::string GetM3U8Path(const std::string &url);
      static bool IsValidM3U8Url(std::string &streamUrl);

    private:
      static bool IsM3U8Playlist(std::string &streamUrl);
      static PlaylistType GetPlaylistType(std::string &source);
      static bool HandleHlsMediaPlaylist(std::shared_ptr<HlsMediaPlaylist> &mediaPlaylist);

      static bool GetNextAttribute(std::string &line, std::string &attributeName, std::string &attributeValue);
      static bool ExtendUrl(std::string &url, const std::string &baseURL);

      static bool ParseMediaPlaylistTags(std::shared_ptr<HlsMediaPlaylist> &mediaPlaylist, std::shared_ptr<HlsMediaSegment> &mediaSegment, std::string &tag, int64_t *segmentOffset, int64_t *segmentSize);
      static void HexStringToBinary(uint8_t *data, char *hexString, int len);
      static uint64_t GetDurationMs(std::string &tag);

      bool HandleHlsMasterPlaylist();

      std::string m_streamUrl;

      std::shared_ptr<HlsMasterPlaylist> m_masterPlaylist;
      std::shared_ptr<HlsMediaPlaylist> m_mediaPlaylist;
      std::shared_ptr<HlsMediaPlaylist> m_audioMediaPlaylist;
    };
  } //namespace hls
} //namespace iptvsimple