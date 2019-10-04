/*
 *      Copyright (C) 2005-2019 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 51 Franklin Street, Fifth Floor, Boston,
 *  MA 02110-1335, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "EpgGenre.h"

#include "../utilities/XMLUtils.h"

#include <cstdlib>

#include <p8-platform/util/StringUtils.h>

using namespace iptvsimple;
using namespace iptvsimple::data;
using namespace rapidxml;

bool EpgGenre::UpdateFrom(rapidxml::xml_node<>* genreNode)
{
  std::string buffer;

  if (GetAttributeValue(genreNode, "genreId", buffer))
  {
    //Combined genre id read as a single hex value.
    int genreId = std::strtol(buffer.c_str(), nullptr, 16);

    m_genreString = genreNode->value();
    m_genreType = genreId & 0xF0;
    m_genreSubType = genreId & 0x0F;
  }
  else
  {
    if (!GetAttributeValue(genreNode, "type", buffer))
      return false;

    if (!StringUtils::IsNaturalNumber(buffer))
      return false;

    m_genreString = genreNode->value();
    m_genreType = std::atoi(buffer.c_str());
    m_genreSubType = 0;

    if (GetAttributeValue(genreNode, "subtype", buffer) && StringUtils::IsNaturalNumber(buffer))
      m_genreSubType = std::atoi(buffer.c_str());
  }

  return true;
}
