/*
 * Ported to C++ from: https://github.com/e2iplayer/hlsdl
 */

#pragma once

#include <memory>
#include <string>

namespace iptvsimple
{
  namespace hls
  {
    struct HlsAudio
    {
      std::string m_url;
      std::string m_groupId;
      std::string m_lang;
      std::string m_name;
      bool m_isDefault;
      std::shared_ptr<HlsAudio> m_next;
    };
  } //namespace hls
} //namespace iptvsimple