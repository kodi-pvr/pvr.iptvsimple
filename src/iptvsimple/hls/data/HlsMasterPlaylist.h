/*
 * Ported to C++ from: https://github.com/e2iplayer/hlsdl
 */

#pragma once

#include <memory>
#include <string>

#include "HlsAudio.h"
#include "HlsMediaPlaylist.h"

namespace iptvsimple
{
  namespace hls
  {
    struct HlsMasterPlaylist
    {
      std::string m_origUrl;
      std::string m_url;
      std::string m_source;
      std::shared_ptr<HlsMediaPlaylist> m_mediaPlaylist;
      std::shared_ptr<HlsAudio> m_audio;
    };
  } //namespace hls
} //namespace iptvsimple