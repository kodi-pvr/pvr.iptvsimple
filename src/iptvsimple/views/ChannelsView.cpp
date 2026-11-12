#include "ChannelsView.h"

#include <algorithm>
#include <sstream>


using namespace iptvsimple::utilities;
namespace iptvsimple
{

ChannelsView::ChannelsView(std::unordered_map<std::string, IptvSimple*>& usedInstances)
  : m_usedInstances(usedInstances)
{
}

std::string ChannelsView::GetViewUrl() const
{
  return "/instances";
}

std::string ChannelsView::GetViewText() const
{
  return "IPTV Simple Instances";
}

void ChannelsView::RegisterViews(httplib::Server& server)
{
  // Main page view - List of instances
  server.Get("/instances", [this](const httplib::Request& req, httplib::Response& res)
             { ShowInstances(req, res); });

  // Instance channels view
  server.Get("/instances/:id", [this](const httplib::Request& req, httplib::Response& res)
             { ShowInstanceChannels(req, res); });

  // Edit channel form
  server.Get("/instances/:id/channels/:channelId/edit",
             [this](const httplib::Request& req, httplib::Response& res)
             { ShowEditChannelForm(req, res); });

  // Update channel
  server.Post("/instances/:id/channels/:channelId",
              [this](const httplib::Request& req, httplib::Response& res)
              { UpdateChannel(req, res); });
}

std::string ChannelsView::GetBaseHtmlTemplate(const std::string& title, const std::string& body)
{
  std::string html = HTML_TEMPLATE;
  size_t titlePos = html.find("{title}");
  if (titlePos != std::string::npos)
    html.replace(titlePos, 7, title);

  size_t bodyPos = html.find("{body}");
  if (bodyPos != std::string::npos)
    html.replace(bodyPos, 6, body);

  return html;
}

std::string ChannelsView::GetInstancesListHtml(
    const std::unordered_map<std::string, IptvSimple*>& instances)
{
  std::stringstream ss;
  ss << "<div class='back'><a href='/'>+ Back to Home</a></div>\n"
     << "<h1>IPTV Simple Instances</h1>\n"
     << "<div class='instances'>\n";

  if (instances.empty())
  {
    ss << "    <div class='instance'>\n"
       << "      <h2>No instances available</h2>\n"
       << "    </div>\n";
  }
  else
  {
    for (const auto& [id, instance] : instances)
    {
      std::string instanceHtml = INSTANCE_TEMPLATE;
      size_t idPos = instanceHtml.find("{id}");
      while (idPos != std::string::npos)
      {
        instanceHtml.replace(idPos, 4, id);
        idPos = instanceHtml.find("{id}", idPos + id.length());
      }
      ss << instanceHtml;
    }
  }

  ss << "</div>";
  return ss.str();
}

std::string ChannelsView::GetChannelsListHtml(const std::string& instanceId, Channels& channels)
{
  std::stringstream ss;
  ss << "<div class='back'><a href='/instances'>+ Back to Instances</a></div>\n"
     << "<h1>Instance " << instanceId << " Channels</h1>\n"
     << "<div class='channels'>\n";

  if (channels.GetChannelsList().empty())
  {
    ss << "    <div class='channel'>\n"
       << "      <h2>No channels available</h2>\n"
       << "    </div>\n";
  }
  else
  {
    for (const auto& channel : channels.GetChannelsList())
    {
      std::string channelHtml = CHANNEL_TEMPLATE;
      size_t namePos = channelHtml.find("{name}");
      if (namePos != std::string::npos)
        channelHtml.replace(namePos, 6, channel.GetChannelName());

      size_t streamUrlPos = channelHtml.find("{stream_url}");
      if (streamUrlPos != std::string::npos)
        channelHtml.replace(streamUrlPos, 12, channel.GetStreamURL());

      size_t instanceIdPos = channelHtml.find("{instance_id}");
      if (instanceIdPos != std::string::npos)
        channelHtml.replace(instanceIdPos, 13, instanceId);

      size_t channelIdPos = channelHtml.find("{channel_id}");
      if (channelIdPos != std::string::npos)
        channelHtml.replace(channelIdPos, 12, std::to_string(channel.GetUniqueId()));

      ss << channelHtml;
    }
  }

  ss << "</div>";
  return ss.str();
}

std::string ChannelsView::GetEditChannelFormHtml(const std::string& instanceId,
                                                 const iptvsimple::data::Channel& channel)
{
  std::string formHtml = EDIT_FORM_TEMPLATE;

  size_t instanceIdPos = formHtml.find("{instance_id}");
  while (instanceIdPos != std::string::npos)
  {
    formHtml.replace(instanceIdPos, 13, instanceId);
    instanceIdPos = formHtml.find("{instance_id}", instanceIdPos + instanceId.length());
  }

  size_t channelIdPos = formHtml.find("{channel_id}");
  if (channelIdPos != std::string::npos)
    formHtml.replace(channelIdPos, 12, std::to_string(channel.GetUniqueId()));

  size_t namePos = formHtml.find("{name}");
  if (namePos != std::string::npos)
    formHtml.replace(namePos, 6, channel.GetChannelName());

  size_t streamUrlPos = formHtml.find("{stream_url}");
  if (streamUrlPos != std::string::npos)
    formHtml.replace(streamUrlPos, 12, channel.GetStreamURL());

  size_t catchupModePos = formHtml.find("{catchup_mode}");
  if (catchupModePos != std::string::npos)
    formHtml.replace(catchupModePos, 14,
                     std::to_string(static_cast<int>(channel.GetCatchupMode())));

  size_t channelNumberPos = formHtml.find("{channel_number}");
  if (channelNumberPos != std::string::npos)
    formHtml.replace(channelNumberPos, 16, std::to_string(channel.GetChannelNumber()));

  size_t subChannelNumberPos = formHtml.find("{sub_channel_number}");
  if (subChannelNumberPos != std::string::npos)
    formHtml.replace(subChannelNumberPos, 20, std::to_string(channel.GetSubChannelNumber()));

  size_t tvgShiftPos = formHtml.find("{tvg_shift}");
  if (tvgShiftPos != std::string::npos)
    formHtml.replace(tvgShiftPos, 11, std::to_string(channel.GetTvgShift()));

  size_t catchupDaysPos = formHtml.find("{catchup_days}");
  if (catchupDaysPos != std::string::npos)
    formHtml.replace(catchupDaysPos, 14, std::to_string(channel.GetCatchupDays()));

  size_t catchupSourcePos = formHtml.find("{catchup_source}");
  if (catchupSourcePos != std::string::npos)
    formHtml.replace(catchupSourcePos, 16, channel.GetCatchupSource());

  size_t catchupTSStreamPos = formHtml.find("{catchup_ts_stream}");
  if (catchupTSStreamPos != std::string::npos)
    formHtml.replace(catchupTSStreamPos, 19, channel.IsCatchupTSStream() ? "checked" : "");

  size_t catchupSupportsTimeshiftingPos = formHtml.find("{catchup_supports_timeshifting}");
  if (catchupSupportsTimeshiftingPos != std::string::npos)
    formHtml.replace(catchupSupportsTimeshiftingPos, 31,
                     channel.CatchupSupportsTimeshifting() ? "checked" : "");

  size_t catchupSourceTerminatesPos = formHtml.find("{catchup_source_terminates}");
  if (catchupSourceTerminatesPos != std::string::npos)
    formHtml.replace(catchupSourceTerminatesPos, 27,
                     channel.CatchupSourceTerminates() ? "checked" : "");

  size_t catchupGranularitySecondsPos = formHtml.find("{catchup_granularity_seconds}");
  if (catchupGranularitySecondsPos != std::string::npos)
    formHtml.replace(catchupGranularitySecondsPos, 29,
                     std::to_string(channel.GetCatchupGranularitySeconds()));

  size_t catchupCorrectionSecsPos = formHtml.find("{catchup_correction_secs}");
  if (catchupCorrectionSecsPos != std::string::npos)
    formHtml.replace(catchupCorrectionSecsPos, 25,
                     std::to_string(channel.GetCatchupCorrectionSecs()));

  size_t tvgIdPos = formHtml.find("{tvg_id}");
  if (tvgIdPos != std::string::npos)
    formHtml.replace(tvgIdPos, 8, channel.GetTvgId());

  size_t tvgNamePos = formHtml.find("{tvg_name}");
  if (tvgNamePos != std::string::npos)
    formHtml.replace(tvgNamePos, 10, channel.GetTvgName());

  size_t inputStreamNamePos = formHtml.find("{input_stream_name}");
  if (inputStreamNamePos != std::string::npos)
    formHtml.replace(inputStreamNamePos, 19, channel.GetInputStreamName());

  size_t iconPathPos = formHtml.find("{icon_path}");
  if (iconPathPos != std::string::npos)
    formHtml.replace(iconPathPos, 11, channel.GetIconPath());

  size_t hasCatchupPos = formHtml.find("{has_catchup}");
  if (hasCatchupPos != std::string::npos)
    formHtml.replace(hasCatchupPos, 13, channel.HasCatchup() ? "checked" : "");

  std::string propertiesHtml = CreatePropertiesHtml(channel.GetProperties());
  size_t propertiesPos = formHtml.find("{properties}");
  if (propertiesPos != std::string::npos)
    formHtml.replace(propertiesPos, 12, propertiesHtml);

  return formHtml;
}

void ChannelsView::ShowInstances(const httplib::Request& req, httplib::Response& res)
{
  std::string body = GetInstancesListHtml(m_usedInstances);
  std::string html = GetBaseHtmlTemplate("IPTV Simple - Instances", body);
  res.set_content(html, HTTP_CONTENT_TYPE_HTML);
}

void ChannelsView::ShowInstanceChannels(const httplib::Request& req, httplib::Response& res)
{
  std::string instanceId = req.path_params.at("id");
  Logger::Log(LogLevel::LEVEL_INFO, "Instance ID: %s", instanceId.c_str());
  auto it = m_usedInstances.find(instanceId);
  if (it != m_usedInstances.end())
  {
    std::string body = GetChannelsListHtml(instanceId, it->second->GetChannelsManager());
    std::string html =
        GetBaseHtmlTemplate("IPTV Simple - Instance " + instanceId + " Channels", body);
    res.set_content(html, HTTP_CONTENT_TYPE_HTML);
  }
  else
  {
    res.status = 404;
    res.set_content(HTTP_404_MESSAGE, HTTP_CONTENT_TYPE_PLAIN);
  }
}

void ChannelsView::ShowEditChannelForm(const httplib::Request& req, httplib::Response& res)
{
  std::string instanceId = req.path_params.at("id");
  std::string channelId = req.path_params.at("channelId");
  auto instanceIt = m_usedInstances.find(instanceId);
  if (instanceIt != m_usedInstances.end())
  {
    iptvsimple::data::Channel* channel =
        instanceIt->second->GetChannelsManager().FindChannel(std::stoi(channelId));
    if (channel != nullptr)
    {

      std::string body = GetEditChannelFormHtml(instanceId, *channel);
      std::string html = GetBaseHtmlTemplate("Edit Channel", body);
      res.set_content(html, HTTP_CONTENT_TYPE_HTML);
    }
    else
    {
      Logger::Log(LogLevel::LEVEL_ERROR, "Channel is null");
      res.status = 404;
      res.set_content(HTTP_404_CHANNEL_MESSAGE, HTTP_CONTENT_TYPE_PLAIN);
    }
  }
  else
  {
    res.status = 404;
    res.set_content(HTTP_404_MESSAGE, HTTP_CONTENT_TYPE_PLAIN);
  }
}

void ChannelsView::UpdateChannel(const httplib::Request& req, httplib::Response& res)
{
  std::string instanceId = req.path_params.at("id");
  std::string channelId = req.path_params.at("channelId");

  auto instanceIt = m_usedInstances.find(instanceId);
  if (instanceIt != m_usedInstances.end())
  {
    iptvsimple::data::Channel* channel =
        instanceIt->second->GetChannelsManager().FindChannel(std::stoi(channelId));
    if (channel != nullptr)
    {
      //std::string name = req.get_param_value("name");
      std::string streamUrl = req.get_param_value("stream_url");
      std::string channelNumber = req.get_param_value("channel_number");
      std::string subChannelNumber = req.get_param_value("sub_channel_number");
      std::string tvgShift = req.get_param_value("tvg_shift");
      std::string iconPath = req.get_param_value("icon_path");
      std::string hasCatchup = req.get_param_value("has_catchup");
      std::string catchupMode = req.get_param_value("catchup_mode");
      std::string catchupDays = req.get_param_value("catchup_days");
      std::string catchupSource = req.get_param_value("catchup_source");
      std::string catchupTSStream = req.get_param_value("catchup_ts_stream");
      std::string catchupSupportsTimeshifting =
          req.get_param_value("catchup_supports_timeshifting");
      std::string catchupSourceTerminates = req.get_param_value("catchup_source_terminates");
      std::string catchupGranularitySeconds = req.get_param_value("catchup_granularity_seconds");
      std::string catchupCorrectionSecs = req.get_param_value("catchup_correction_secs");
      std::string tvgId = req.get_param_value("tvg_id");
      std::string tvgName = req.get_param_value("tvg_name");
      std::string inputStreamName = req.get_param_value("input_stream_name");
      std::multimap<std::string, std::string> params = req.params;
      std::map<std::string, std::string> properties = channel->GetProperties();
      for (const auto& [key, value] : params)
      {
        if (key.rfind("property_", 0) == 0)
        {
          std::string propertyKey = key.substr(9);
          if (properties.find(propertyKey) == properties.end())
            properties.insert({propertyKey, value});
          else
            properties[propertyKey] = value;
        }
      }
      channel->SetProperties(properties);
      //channel->SetChannelName(name);
      channel->SetStreamURL(streamUrl);
      channel->SetChannelNumber(std::stoi(channelNumber));
      channel->SetSubChannelNumber(std::stoi(subChannelNumber));
      channel->SetTvgShift(std::stoi(tvgShift));
      channel->SetIconPath(iconPath);
      channel->SetHasCatchup(hasCatchup == "on" ? true : false);
      int catchupModeInt = std::stoi(catchupMode);
      if (catchupModeInt >= 0 && catchupModeInt <= 7)
        channel->SetCatchupMode(static_cast<iptvsimple::CatchupMode>(catchupModeInt));
      channel->SetCatchupDays(std::stoi(catchupDays));
      channel->SetCatchupSource(catchupSource);
      channel->SetCatchupTSStream(catchupTSStream == "on" ? true : false);
      channel->SetCatchupSupportsTimeshifting(catchupSupportsTimeshifting == "on" ? true : false);
      channel->SetCatchupSourceTerminates(catchupSourceTerminates == "on" ? true : false);
      channel->SetCatchupGranularitySeconds(std::stoi(catchupGranularitySeconds));
      channel->SetCatchupCorrectionSecs(std::stoi(catchupCorrectionSecs));
      channel->SetTvgId(tvgId);
      channel->SetTvgName(tvgName);
      channel->SetInputStreamName(inputStreamName);

      res.set_redirect("/instances/" + instanceId);
    }
    else
    {
      res.status = 404;
      res.set_content(HTTP_404_CHANNEL_MESSAGE, HTTP_CONTENT_TYPE_PLAIN);
    }
  }
  else
  {
    res.status = 404;
    res.set_content(HTTP_404_MESSAGE, HTTP_CONTENT_TYPE_PLAIN);
  }
}

std::string ChannelsView::CreatePropertiesHtml(const std::map<std::string, std::string>& properties)
{
  std::stringstream ss;
  for (const auto& [key, value] : properties)
  {
    std::string propertyHtml = PROPERTY_TEMPLATE;
    Logger::Log(LogLevel::LEVEL_INFO, "Key: %s, Value: %s", key.c_str(), value.c_str());
    size_t keyPos = propertyHtml.find("{key}");
    while (keyPos != std::string::npos)
    {
      propertyHtml.replace(keyPos, 5, key);
      keyPos = propertyHtml.find("{key}", keyPos + key.length());
    }
    size_t valuePos = propertyHtml.find("{value}");
    if (valuePos != std::string::npos)
      propertyHtml.replace(valuePos, 7, value);
    ss << propertyHtml;
  }
  return ss.str();
}

} // namespace iptvsimple
