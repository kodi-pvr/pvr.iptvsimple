/*
 * Ported to C++ from: https://github.com/e2iplayer/hlsdl
 */

#pragma once

#include <memory>
#include <string>

#include "EncAes128.h"
#include "HlsMediaSegment.h"

namespace iptvsimple
{
  namespace hls
  {
    struct HlsMediaPlaylist
    {
        std::string m_origUrl;
        std::string m_url;
        std::string m_source;
        std::string m_audioGroup;
        std::string m_resolution;
        std::string m_codecs;
        unsigned int m_bitrate;
        uint64_t m_targetDurationMs;
        uint64_t m_totalDurationMs;
        bool m_isEndlist;
        bool m_isEncrypted;
        EncryptionType m_encryptionType;
        int m_firstMediaSequence;
        int m_lastMediaSequence;
        std::shared_ptr<HlsMediaSegment> m_firstMediaSegment;
        std::shared_ptr<HlsMediaSegment> m_lastMediaSegment;
        EncAes128 m_encAes;

        std::shared_ptr<HlsMediaPlaylist> m_next;
    };
  } //namespace hls
} //namespace iptvsimple