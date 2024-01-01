/*
 *  Copyright (C) 2005-2021 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include <kodi/addon-instance/PVR.h>

namespace iptvsimple
{
  class ATTR_DLL_LOCAL IConnectionListener : public kodi::addon::CInstancePVRClient
  {
  public:
    IConnectionListener(const kodi::addon::IInstanceInfo& instance)
      : kodi::addon::CInstancePVRClient(instance) { }
    virtual ~IConnectionListener() = default;

    virtual void ConnectionLost() = 0;
    virtual void ConnectionEstablished() = 0;
  };
} // namespace iptvsimple
