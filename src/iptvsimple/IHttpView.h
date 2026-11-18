#pragma once

#include <httplib.h>

namespace iptvsimple
{

class IHttpView
{
public:
  virtual ~IHttpView() = default;
  virtual void RegisterViews(httplib::Server& server) = 0;
  virtual std::string GetViewUrl() const = 0;
  virtual std::string GetViewText() const = 0;
};

} // namespace iptvsimple
