#pragma once

/*
 *      Copyright (C) 2005-2015 Team Kodi
 *      http://kodi.tv
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
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include <string>
#include <vector>

#include <pugixml.hpp>

inline std::string GetNodeValue(const pugi::xml_node& rootNode, const char* tag)
{
  const auto& childNode = rootNode.child(tag);
  if (!childNode)
    return "";

  return childNode.child_value();
}

inline std::string GetJoinedNodeValues(const pugi::xml_node& rootNode, const char* tag)
{
  std::string stringValue;

  for (const auto& childNode : rootNode.children(tag))
  {
    if (childNode)
    {
      if (!stringValue.empty())
        stringValue += ",";
      stringValue += childNode.child_value();
    }
  }

  return stringValue;
}

inline std::vector<std::string> GetNodeValuesList(const pugi::xml_node& rootNode, const char* tag)
{
  std::vector<std::string> stringValues;

  for(const auto& childNode : rootNode.children(tag))
  {
    if (childNode)
      stringValues.emplace_back(childNode.child_value());
  }

  return stringValues;
}

inline bool GetAttributeValue(const pugi::xml_node& node, const char* attributeName, std::string& stringValue)
{
  const auto& attribute = node.attribute(attributeName);
  if (!attribute)
    return false;

  stringValue = attribute.value();
  return true;
}

inline int GetParseErrorString(const char* buffer, int errorOffset, std::string& errorString)
{
  errorString = buffer;

  // Try and start the error string two newlines before the error offset
  int startOffset = errorOffset;
  size_t found = errorString.rfind("\n", errorOffset);
  if (found != std::string::npos)
  {
    startOffset = found;
    found = errorString.rfind("\n", startOffset - 1);
    if (found != std::string::npos && startOffset != 0)
      startOffset = found;
  }

  // And end itone newline after
  int endOffset = errorOffset;
  found = errorString.find("\n", errorOffset);
  if (found != std::string::npos)
    endOffset = found;

  errorString = errorString.substr(startOffset, endOffset - startOffset);
  return errorOffset - startOffset;
}
