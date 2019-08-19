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

#include <functional>
#include <string>

namespace iptvsimple
{
  namespace utilities
  {
    /**
     * Represents the log level
     */
    enum LogLevel
    {
      LEVEL_ERROR,
      LEVEL_NOTICE,
      LEVEL_INFO,
      LEVEL_DEBUG,
      LEVEL_TRACE
    };

    /**
     * Short-hand for a function that acts as the logger implementation
     */
    typedef std::function<void(LogLevel level, const char* message)> LoggerImplementation;

    /**
     * The logger class. It is a singleton that by default comes with no
     * underlying implementation. It is up to the user to supply a suitable
     * implementation as a lambda using SetImplementation().
     */
    class Logger
    {
    public:
      /**
       * Returns the singleton instance
       * @return
       */
      static Logger& GetInstance();

      /**
       * Logs the specified message using the specified log level
       * @param level the log level
       * @param message the log message
       * @param ... parameters for the log message
       */
      static void Log(LogLevel level, const char* message, ...);

      /**
       * Configures the logger to use the specified implementation
       * @param implementation lambda
       */
      void SetImplementation(LoggerImplementation implementation);

      /**
       * Sets the prefix to use in log messages
       * @param prefix
       */
      void SetPrefix(const std::string& prefix);

    private:
      static const unsigned int MESSAGE_BUFFER_SIZE = 16384;

      Logger();

      /**
       * The logger implementation
       */
      LoggerImplementation m_implementation;

      /**
       * The log message prefix
       */
      std::string m_prefix;
    };
  } // namespace utilities
} // namespace iptvsimple
