/*
 *  Copyright (C) 2005-2021 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

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
