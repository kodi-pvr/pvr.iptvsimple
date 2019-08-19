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

#include "Logger.h"

#include <cstdarg>

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

  char buffer[MESSAGE_BUFFER_SIZE];
  std::string logMessage = message;
  const std::string prefix = logger.m_prefix;

  // Prepend the prefix when set
  if (!prefix.empty())
    logMessage = prefix + " - " + message;

  va_list arguments;
  va_start(arguments, message);
  vsprintf(buffer, logMessage.c_str(), arguments);
  va_end(arguments);

  logger.m_implementation(level, buffer);
}

void Logger::SetImplementation(LoggerImplementation implementation)
{
  m_implementation = implementation;
}

void Logger::SetPrefix(const std::string& prefix)
{
  m_prefix = prefix;
}
