/*
 *  Copyright (C) 2005-2021 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "StringComparer.h"

using namespace iptvsimple;
using namespace iptvsimple::utilities;

double StringComparer::SorensenDiceSimilarity(const std::string& text1,
                                              const std::string& text2,
                                              const std::shared_ptr<iptvsimple::InstanceSettings>& settings)
{
  // Empty strings should return 0 similarity
  if (text1.empty() || text2.empty() || text1 == "-1" || text2 == "-1")
    return 0.0;

  // Create local copies for case conversion
  std::string str1 = text1;
  std::string str2 = text2;

  if (settings->IgnoreCaseForEpgChannelIds())
  {
    std::transform(str1.begin(), str1.end(), str1.begin(), ::tolower);
    std::transform(str2.begin(), str2.end(), str2.begin(), ::tolower);
  }

  // Get threshold early to optimize
  double threshold = static_cast<double>(settings->GetEpgChannelNameMatchThreshold());
  if (threshold >= 100.0)
    return str1 == str2 ? 1.0 : 0.0;

  // Identical strings should return 1.0 similarity
  if (str1 == str2)
    return 1.0;

  std::set<std::string> bigrams1;
  std::set<std::string> bigrams2;

  // Generate bigrams for first string
  for (size_t i = 0; i < str1.length() - 1; i++)
    bigrams1.insert(str1.substr(i, 2));

  // Generate bigrams for second string
  for (size_t i = 0; i < str2.length() - 1; i++)
    bigrams2.insert(str2.substr(i, 2));

  // Handle case where no bigrams were generated
  if (bigrams1.empty() || bigrams2.empty())
    return 0.0;

  // Count intersection
  size_t intersection = 0;
  for (const auto& bigram : bigrams1)
  {
    if (bigrams2.find(bigram) != bigrams2.end())
      intersection++;
  }

  // Calculate similarity
  double similarity = (2.0 * intersection) / (bigrams1.size() + bigrams2.size());

  // Check if similarity meets the threshold
  if (threshold > 0.0)
  {
    double minSimilarity = threshold / 100.0;
    if (similarity < minSimilarity)
      return 0.0;
  }

  return similarity;
}
