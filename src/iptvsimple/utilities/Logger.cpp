/*
 *  Copyright (C) 2005-2021 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "Logger.h"


#include <cstdarg>

#include <kodi/tools/StringUtils.h>

using namespace kodi::tools;
using namespace iptvsimple::utilities;

Logger::Logger()
{
  // Use an empty implementation by default
  SetImplementation([](LogLevel level, const char* message)
  {
  });
}

Logger& Logger::GetInstance()
{
  static Logger instance;
  return instance;
}

void Logger::Log(LogLevel level, const char* message, ...)
{
  auto& logger = GetInstance();

  std::string logMessage;

  // Prepend the prefix when set
  const std::string prefix = logger.m_prefix;
  if (!prefix.empty())
    logMessage = prefix + " - ";

  logMessage += message;

  va_list arguments;
  va_start(arguments, message);
  logMessage = StringUtils::FormatV(logMessage.c_str(), arguments);
  va_end(arguments);

  {
    std::lock_guard<std::mutex> lock(logger.m_redactionsMutex);
    for (const auto& redaction : logger.m_redactions)
      StringUtils::Replace(logMessage, redaction.first, redaction.second);
  }

  logger.m_implementation(level, logMessage.c_str());
}

void Logger::SetImplementation(LoggerImplementation implementation)
{
  m_implementation = implementation;
}

void Logger::SetPrefix(const std::string& prefix)
{
  m_prefix = prefix;
}

void Logger::AddRedaction(const std::string& text, const std::string& replacement)
{
  if (text.empty())
    return;

  std::lock_guard<std::mutex> lock(m_redactionsMutex);
  m_redactions[text] = replacement;
}
