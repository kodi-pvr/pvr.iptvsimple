/*
 *  Copyright (C) 2005-2021 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include <string>

#include <pugixml.hpp>

namespace iptvsimple
{
  namespace data
  {
    class EpgGenre
    {
    public:
      int GetGenreType() const { return m_genreType; }
      void SetGenreType(int value) { m_genreType = value; }

      int GetGenreSubType() const { return m_genreSubType; }
      void SetGenreSubType(int value) { m_genreSubType = value; }

      const std::string& GetGenreString() const { return m_genreString; }
      void SetGenreString(const std::string& value) { m_genreString = value; }

      bool UpdateFrom(const pugi::xml_node& genreNode);

    private:
      int m_genreType;
      int m_genreSubType;
      std::string m_genreString;
    };
  } //namespace data
} //namespace iptvsimple
