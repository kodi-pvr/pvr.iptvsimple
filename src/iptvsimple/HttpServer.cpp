#include "HttpServer.h"


using namespace iptvsimple;

HttpServer::HttpServer()
{
  Initialize();
}

HttpServer::~HttpServer()
{
  Shutdown();
}

void HttpServer::AddView(std::shared_ptr<IHttpView> view)
{
  if (view)
  {
    views.push_back(view);
    view->RegisterViews(svr);
  }
}

bool HttpServer::Initialize()
{
  kodi::Log(ADDON_LOG_INFO, "Initializing HTTP Server");
  svr.Get(R"(/|/index.html)",
          [this](const httplib::Request& req, httplib::Response& res)
          {
            std::string body =
                "<!DOCTYPE html>\n"
                "<html lang=\"en\">\n"
                "<head>\n"
                "    <meta charset=\"UTF-8\">\n"
                "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
                "    <title>IPTV Simple PVR Client</title>\n"
                "    <style>\n"
                "        /* Reset default styles */\n"
                "        * {\n"
                "            margin: 0;\n"
                "            padding: 0;\n"
                "            box-sizing: border-box;\n"
                "        }\n"
                "\n"
                "        html, body {\n"
                "            height: 100%;\n"
                "        }\n"
                "\n"
                "        body {\n"
                "            font-family: 'Segoe UI', Arial, sans-serif;\n"
                "            background-color: #f4f7fa;\n"
                "            color: #333;\n"
                "            line-height: 1.6;\n"
                "            padding: 0;\n"
                "            display: flex;\n"
                "            flex-direction: column;\n"
                "            min-height: 100vh;\n"
                "        }\n"
                "\n"
                "        /* Header styling */\n"
                "        header {\n"
                "            background: linear-gradient(135deg, #007bff, #0056b3);\n"
                "            color: white;\n"
                "            text-align: center;\n"
                "            padding: 2rem 1rem;\n"
                "            box-shadow: 0 2px 5px rgba(0, 0, 0, 0.1);\n"
                "        }\n"
                "\n"
                "        header h1 {\n"
                "            font-size: 2.5rem;\n"
                "            margin-bottom: 0.5rem;\n"
                "        }\n"
                "\n"
                "        /* Main container */\n"
                "        .container {\n"
                "            max-width: 800px;\n"
                "            margin: 2rem auto;\n"
                "            padding: 0 1rem;\n"
                "            flex: 1;\n"
                "        }\n"
                "\n"
                "        /* Section styling */\n"
                "        section {\n"
                "            background: white;\n"
                "            border-radius: 8px;\n"
                "            padding: 2rem;\n"
                "            box-shadow: 0 4px 10px rgba(0, 0, 0, 0.05);\n"
                "        }\n"
                "\n"
                "        section h2 {\n"
                "            font-size: 1.8rem;\n"
                "            color: #007bff;\n"
                "            margin-bottom: 1rem;\n"
                "        }\n"
                "\n"
                "        /* List styling */\n"
                "        ul {\n"
                "            list-style: none;\n"
                "        }\n"
                "\n"
                "        ul li {\n"
                "            margin: 0.5rem 0;\n"
                "        }\n"
                "\n"
                "        ul li a {\n"
                "            display: inline-block;\n"
                "            text-decoration: none;\n"
                "            color: #ffffff;\n"
                "            background-color: #007bff;\n"
                "            padding: 0.75rem 1.5rem;\n"
                "            border-radius: 5px;\n"
                "            transition: background-color 0.3s ease, transform 0.2s ease;\n"
                "        }\n"
                "\n"
                "        ul li a:hover {\n"
                "            background-color: #0056b3;\n"
                "            transform: translateY(-2px);\n"
                "        }\n"
                "\n"
                "        /* Responsive design */\n"
                "        @media (max-width: 600px) {\n"
                "            header h1 {\n"
                "                font-size: 1.8rem;\n"
                "            }\n"
                "\n"
                "            section {\n"
                "                padding: 1.5rem;\n"
                "            }\n"
                "\n"
                "            ul li a {\n"
                "                padding: 0.6rem 1.2rem;\n"
                "                font-size: 0.9rem;\n"
                "            }\n"
                "        }\n"
                "\n"
                "        /* Footer */\n"
                "        footer {\n"
                "            text-align: center;\n"
                "            padding: 1rem;\n"
                "            background-color: #007bff;\n"
                "            color: white;\n"
                "            width: 100%;\n"
                "            margin-top: 2rem;\n"
                "        }\n"
                "    </style>\n"
                "</head>\n"
                "<body>\n"
                "    <header>\n"
                "        <h1>IPTV Simple PVR Client</h1>\n"
                "    </header>\n"
                "\n"
                "    <div class=\"container\">\n"
                "        <section>\n"
                "            <h2>Available Views</h2>\n"
                "            <ul>\n";
            for (const auto& view : views)
            {
              body += "  <li><a href='" + view->GetViewUrl() + "'>" + view->GetViewText() +
                      "</a></li>\n";
            }
            body += "</ul>\n";
            body += "        </section>\n"
                    "    </div>\n"
                    "\n"
                    "    <footer>\n"
                    "        <p>&copy; 2025 IPTV Simple PVR Client. All rights reserved.</p>\n"
                    "    </footer>\n"
                    "</body>\n"
                    "</html>";
            res.set_content(body, "text/html");
          });

  svr.set_error_handler(
      [](const auto& req, auto& res)
      {
        auto fmt = "<p>Error Status: <span style='color:red;'>%d</span></p>";
        char buf[BUFSIZ];
        snprintf(buf, sizeof(buf), fmt, res.status);
        res.set_content(buf, "text/html");
      });

  svr.set_exception_handler(
      [](const auto& req, auto& res, std::exception_ptr ep)
      {
        auto fmt = "<h1>Error 500</h1><p>%s</p>";
        char buf[BUFSIZ];
        try
        {
          std::rethrow_exception(ep);
        }
        catch (std::exception& e)
        {
          snprintf(buf, sizeof(buf), fmt, e.what());
        }
        catch (...)
        {
          snprintf(buf, sizeof(buf), fmt, "Unknown Exception");
        }
        res.set_content(buf, "text/html");
        res.status = 500;
      });

  svr.set_logger(
      [](const auto& req, const auto& res)
      {
        std::string logMessage = "HTTP Request: Method=" + req.method + ", Path=" + req.path +
                                 ", Status=" + std::to_string(res.status);
        kodi::Log(ADDON_LOG_DEBUG, logMessage.c_str());
      });

  int port = 8080;

  t = std::thread(
      [&]()
      {
        if (!svr.listen("0.0.0.0", port))
        {
          kodi::Log(ADDON_LOG_ERROR, "Server is not running on port %d", port);
          svr.stop();
          port = svr.bind_to_any_port("0.0.0.0");
          if (!svr.listen_after_bind())
          {
            kodi::Log(ADDON_LOG_ERROR, "Failed to listen on any port");
          }
        }
      });

  svr.wait_until_ready();
  svr.wait_until_ready();
  if (svr.is_running())
  {
    // TODO: Add main settings option for port configuration.
    kodi::Log(ADDON_LOG_INFO, "Server is running on port %d.", port);
    return true;
  }

  return false;
}

void HttpServer::Shutdown()
{
  kodi::Log(ADDON_LOG_INFO, "Shutting down HTTP Server");
  svr.stop();
  t.join();
  if (!svr.is_running())
  {
    kodi::Log(ADDON_LOG_INFO, "Server is stopped");
  }
}
