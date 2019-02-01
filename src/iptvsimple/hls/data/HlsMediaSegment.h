/*
 * Ported to C++ from: https://github.com/e2iplayer/hlsdl
 */

#pragma once

#include <memory>
#include <string>

#include "EncAes128.h"

namespace iptvsimple
{
  namespace hls
  {
    struct HlsMediaSegment
    {
      std::string m_url;
      int64_t m_offset;
      int64_t m_size;
      int m_sequenceNumber;
      uint64_t m_durationMs;
      EncAes128 m_encAes;
      std::shared_ptr<HlsMediaSegment> m_next;
      std::shared_ptr<HlsMediaSegment> m_prev;
    };
  } //namespace hls
} //namespace iptvsimple