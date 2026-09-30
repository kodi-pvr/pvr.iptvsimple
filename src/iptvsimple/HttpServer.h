#pragma once

#include "IHttpView.h"

#include <kodi/General.h>

#include <memory>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace iptvsimple
{

class HttpServer
{
public:
  HttpServer();
  ~HttpServer();

  // Prevent copy/move
  HttpServer(const HttpServer&) = delete;
  HttpServer& operator=(const HttpServer&) = delete;
  HttpServer(HttpServer&&) = delete;
  HttpServer& operator=(HttpServer&&) = delete;

  // Add an endpoint interface to the server
  void AddView(std::shared_ptr<IHttpView> view);

private:
  httplib::Server svr;
  std::thread t;
  std::vector<std::shared_ptr<IHttpView>> views;

  // Initialize the server
  bool Initialize();

  // Shutdown the server
  void Shutdown();
};

} // namespace iptvsimple
