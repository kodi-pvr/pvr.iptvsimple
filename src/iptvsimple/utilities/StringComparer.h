/*
 *  Copyright (C) 2005-2021 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include <algorithm>
#include <set>
#include <string>
#include "../InstanceSettings.h"

namespace iptvsimple
{
namespace utilities
{

class StringComparer
{
public:
  /**
   * @brief Calculates the Sørensen-Dice similarity coefficient between two strings
   * @param text1 First string to compare
   * @param text2 Second string to compare 
   * @param settings The instance settings
   * @return A value between 0 and 1, where 1 means the strings are identical
   */
  static double SorensenDiceSimilarity(const std::string& text1,
                                      const std::string& text2,
                                      const std::shared_ptr<iptvsimple::InstanceSettings>& settings);
};

} // namespace utilities
} // namespace iptvsimple
