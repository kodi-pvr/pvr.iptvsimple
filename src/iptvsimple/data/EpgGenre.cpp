/*
 *  Copyright (C) 2005-2021 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "EpgGenre.h"

#include "../utilities/XMLUtils.h"

#include <cstdlib>

#include <kodi/tools/StringUtils.h>

using namespace kodi::tools;
using namespace iptvsimple;
using namespace iptvsimple::data;
using namespace pugi;

bool EpgGenre::UpdateFrom(const xml_node& genreNode)
{
  std::string buffer;

  if (GetAttributeValue(genreNode, "genreId", buffer))
  {
    //Combined genre id read as a single hex value.
    int genreId = std::strtol(buffer.c_str(), nullptr, 16);

    m_genreString = genreNode.child_value();
    m_genreType = genreId & 0xF0;
    m_genreSubType = genreId & 0x0F;
  }
  else
  {
    if (!GetAttributeValue(genreNode, "type", buffer))
      return false;

    if (!StringUtils::IsNaturalNumber(buffer))
      return false;

    m_genreString = genreNode.child_value();
    m_genreType = std::atoi(buffer.c_str());
    m_genreSubType = 0;

    if (GetAttributeValue(genreNode, "subtype", buffer) && StringUtils::IsNaturalNumber(buffer))
      m_genreSubType = std::atoi(buffer.c_str());
  }

  return true;
}
