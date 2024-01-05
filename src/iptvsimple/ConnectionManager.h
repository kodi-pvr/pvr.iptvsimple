/*
 *  Copyright (C) 2005-2021 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include "InstanceSettings.h"

#include <atomic>
#include <mutex>
#include <string>
#include <thread>

#include <kodi/addon-instance/pvr/General.h>

namespace iptvsimple
{
  static const int FAST_RECONNECT_ATTEMPTS = 5;
  static const int SLEEP_INTERVAL_STEP_MS = 500;

  class IConnectionListener;

  class ATTR_DLL_LOCAL ConnectionManager
  {
  public:
    ConnectionManager(IConnectionListener& connectionListener, std::shared_ptr<iptvsimple::InstanceSettings> settings);
    ~ConnectionManager();

    void Start();
    void Stop();
    void Disconnect();
    void Reconnect();

    void OnSleep();
    void OnWake();

  private:
    void Process();
    void SetState(PVR_CONNECTION_STATE state);
    void SteppedSleep(int intervalMs);

    IConnectionListener& m_connectionListener;
    std::atomic<bool> m_running = {false};
    std::thread m_thread;
    mutable std::mutex m_mutex;
    bool m_suspended;
    PVR_CONNECTION_STATE m_state;

    bool m_onStartupOnly = true;

    std::shared_ptr<iptvsimple::InstanceSettings> m_settings;
  };
} // namespace iptvsimple
