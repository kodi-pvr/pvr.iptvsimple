#pragma once

#include "client.h"

#include <string>

namespace iptvsimple
{

class LocalizedString
{
public:
  explicit LocalizedString(int id)
  {
    Load(id);
  }

  bool Load(int id)
  {
    char *str;
    if ((str = XBMC->GetLocalizedString(id)))
    {
      m_localizedString = str;
      XBMC->FreeString(str);
      return true;
    }

    m_localizedString = "";
    return false;
  }

  std::string Get()
  {
    return m_localizedString;
  }

  operator std::string()
  {
    return Get();
  }

  const char* c_str()
  {
    return m_localizedString.c_str();
  }

private:
  LocalizedString() = delete;
  LocalizedString(const LocalizedString&) = delete;
  LocalizedString &operator =(const LocalizedString&) = delete;

  std::string m_localizedString;
};

} //namespace iptvsimple