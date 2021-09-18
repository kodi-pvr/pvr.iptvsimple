/*
 *  Copyright (C) 2005-2021 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

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
      LEVEL_DEBUG,
      LEVEL_INFO,
      LEVEL_WARNING,
      LEVEL_ERROR,
      LEVEL_FATAL
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
