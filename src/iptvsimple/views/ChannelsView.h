#pragma once

#include "../IHttpView.h"
#include "../../IptvSimple.h"
#include "../Channels.h"
#include "../data/Channel.h"
#include "../utilities/Logger.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace iptvsimple
{

class ChannelsView : public IHttpView
{
public:
  ChannelsView(std::unordered_map<std::string, IptvSimple*>& usedInstances);
  ~ChannelsView() = default;

  // Prevent copy/move
  ChannelsView(const ChannelsView&) = delete;
  ChannelsView& operator=(const ChannelsView&) = delete;
  ChannelsView(ChannelsView&&) = delete;
  ChannelsView& operator=(ChannelsView&&) = delete;

  // IHttpView implementation
  void RegisterViews(httplib::Server& server) override;
  std::string GetViewUrl() const override;
  std::string GetViewText() const override;

private:
  std::unordered_map<std::string, IptvSimple*>& m_usedInstances;

  // View handlers
  void ShowInstances(const httplib::Request& req, httplib::Response& res);
  void ShowInstanceChannels(const httplib::Request& req, httplib::Response& res);
  void ShowEditChannelForm(const httplib::Request& req, httplib::Response& res);
  void UpdateChannel(const httplib::Request& req, httplib::Response& res);

  // HTML template methods
  std::string GetBaseHtmlTemplate(const std::string& title, const std::string& body);
  std::string GetInstancesListHtml(const std::unordered_map<std::string, IptvSimple*>& instances);
  std::string GetChannelsListHtml(const std::string& instanceId, Channels& channels);
  std::string GetEditChannelFormHtml(const std::string& instanceId,
                                     const iptvsimple::data::Channel& channel);
  std::string CreatePropertiesHtml(const std::map<std::string, std::string>& properties);

  // HTTP constants
  static constexpr const char* HTTP_404_NOT_FOUND = "Not Found";
  static constexpr const char* HTTP_404_MESSAGE = "Instance not found";
  static constexpr const char* HTTP_404_CHANNEL_MESSAGE = "Channel not found";
  static constexpr const char* HTTP_CONTENT_TYPE_HTML = "text/html";
  static constexpr const char* HTTP_CONTENT_TYPE_PLAIN = "text/plain";

  // HTML template constants
  static constexpr const char* HTML_TEMPLATE = R"(
<!DOCTYPE html>
<html>
<head>
  <title>{title}</title>
  <style>
    body { font-family: Arial, sans-serif; margin: 20px; }
    .instance, .channel { padding: 10px; margin: 5px; background: #f0f0f0; }
    a { text-decoration: none; color: #0066cc; }
    .back { margin-bottom: 20px; }
    form { max-width: 500px; }
    .form-group { margin-bottom: 15px; }
    label { display: block; margin-bottom: 5px; }
    input[type='text'] { width: 100%; padding: 8px; }
    button { padding: 10px 20px; background: #0066cc; color: white; border: none; }
  </style>
</head>
<body>
{body}
</body>
</html>
)";

  static constexpr const char* INSTANCE_TEMPLATE = R"(
    <div class='instance'>
      <h2><a href='/instances/{id}'>Instance {id}</a></h2>
    </div>
)";

  static constexpr const char* CHANNEL_TEMPLATE = R"(
    <div class='channel'>
      <h2>{name}</h2>
      <p>Stream URL: {stream_url}</p>
      <a href='/instances/{instance_id}/channels/{channel_id}/edit'>Edit</a>
    </div>
)";

  static constexpr const char* EDIT_FORM_TEMPLATE = R"(
  <div class='back'><a href='/instances/{instance_id}'>+ Back to Channels</a></div>
  <h1>Edit Channel</h1>
  <form method='POST' action='/instances/{instance_id}/channels/{channel_id}'>
    <div class='form-group'>
      <label for='name'>Channel Name:</label>
      <input type='text' id='name' name='name' value='{name}' disabled>
    </div>
    <div class='form-group'>
      <label for='stream_url'>Stream URL:</label>
      <input type='text' id='stream_url' name='stream_url' value='{stream_url}'>
    </div>
    <div class='form-group'>
      <label for='channel_number'>Channel Number:</label>
      <input type='number' id='channel_number' name='channel_number' value='{channel_number}'>
    </div>
    <div class='form-group'>
      <label for='sub_channel_number'>Sub Channel Number:</label>
      <input type='number' id='sub_channel_number' name='sub_channel_number' value='{sub_channel_number}'>
    </div>
    <div class='form-group'>
      <label for='tvg_shift'>TVG Shift:</label>
      <input type='number' id='tvg_shift' name='tvg_shift' value='{tvg_shift}'>
    </div>
    <div class='form-group'>
      <label for='icon_path'>Icon Path:</label>
      <input type='text' id='icon_path' name='icon_path' value='{icon_path}'>
    </div>
    <div class='form-group'>
      <label for='has_catchup'>Has Catchup:</label>
      <input type='checkbox' id='has_catchup' name='has_catchup' {has_catchup_checked}>
    </div>
    <div class='form-group'>
      <label for='catchup_mode'>Catchup Mode:</label>
      <select id='catchup_mode' name='catchup_mode' value='{catchup_mode}'>
        <option value='0'>DISABLED</option>
        <option value='1'>DEFAULT</option>
        <option value='2'>APPEND</option>
        <option value='3'>SHIFT</option>
        <option value='4'>FLUSSONIC</option>
        <option value='5'>XTREAM_CODES</option>
        <option value='6'>TIMESHIFT</option>
        <option value='7'>VOD</option>
      </select>
    </div>
     <div class='form-group'>
      <label for='catchup_days'>Catchup Days:</label>
      <input type='number' id='catchup_days' name='catchup_days' value='{catchup_days}'>
    </div>
    <div class='form-group'>
      <label for='catchup_source'>Catchup Source:</label>
      <input type='text' id='catchup_source' name='catchup_source' value='{catchup_source}'>
    </div>
    <div class='form-group'>
      <label for='is_catchup_ts_stream'>Is Catchup TS Stream?:</label>
      <input type='checkbox' id='is_catchup_ts_stream' name='is_catchup_ts_stream' {is_catchup_ts_stream_checked}>
    </div>
    <div class='form-group'>
      <label for='catchup_supports_timeshifting'>Catchup Supports Timeshifting?:</label>
      <input type='checkbox' id='catchup_supports_timeshifting' name='catchup_supports_timeshifting' {catchup_supports_timeshifting_checked}>
    </div>
    <div class='form-group'>
      <label for='catchup_source_terminates'>Catchup Source Terminates?:</label>
      <input type='checkbox' id='catchup_source_terminates' name='catchup_source_terminates' {catchup_source_terminates_checked}>
    </div>
    <div class='form-group'>
      <label for='catchup_granularity_seconds'>Catchup Granularity Seconds:</label>
      <input type='number' id='catchup_granularity_seconds' name='catchup_granularity_seconds' value='{catchup_granularity_seconds}'>
    </div>
    <div class='form-group'>
      <label for='catchup_correction_secs'>Catchup Correction Secs:</label>
      <input type='number' id='catchup_correction_secs' name='catchup_correction_secs' value='{catchup_correction_secs}'>
    </div>
    <div class='form-group'>
      <label for='tvg_id'>TVG ID:</label>
      <input type='text' id='tvg_id' name='tvg_id' value='{tvg_id}'>
    </div>
    <div class='form-group'>
      <label for='tvg_name'>TVG Name:</label>
      <input type='text' id='tvg_name' name='tvg_name' value='{tvg_name}'>
    </div>
    <div class='form-group'>
      <label for='input_stream_name'>Input Stream Name:</label>
      <input type='text' id='input_stream_name' name='input_stream_name' value='{input_stream_name}'>
    </div>
    <h2>Properties:</h2>
    {properties}
    <div id='new-properties'></div>
    <div class='form-group' style='display: flex; gap: 10px;'>
      <input type='text' id='new-property-key' placeholder='Key' list='key-suggestions'>
      <input type='text' id='new-property-value' placeholder='Value'>
    </div>
    <datalist id='key-suggestions'>
      <option value='isWebUrl'>
      <option value='isrealtimestream'>
      <option value='http-reconnect'>
      <option value='http-user-agent'>
      <option value='http-referrer'>
      <option value='program'>
      <option value='web-regex'>
      <option value='web-headers'>
      <option value='inputstream'>
      <option value='mimetype'>
      <option value='inputstream.ffmpegdirect.manifest_type'>
      <option value='inputstream.ffmpegdirect.stream_mode'>
      <option value='inputstream.ffmpegdirect.is_realtime_stream'>
      <option value='inputstream.ffmpegdirect.stream_headers'>
      <option value='inputstream.ffmpegdirect.manifest_headers'>
    </datalist>
    <button type='button' onclick='addProperty()'>Add Property</button>
    <button type='submit'>Save Changes</button>
  </form>
  <script>
    function addProperty() {
      const newProperties = document.getElementById('new-properties');
      const newProperty = document.createElement('div');
      newProperty.className = 'form-group';
      const key = document.getElementById('new-property-key').value;
      const value = document.getElementById('new-property-value').value;
      if (key && value) {
      newProperty.innerHTML = `
        <label for='property_${key}'>${key}:</label>
        <input type='text' id='property_${key}' name='property_${key}' value='${value}'>
      `;
        newProperties.appendChild(newProperty);
        document.getElementById('new-property-key').value = '';
        document.getElementById('new-property-value').value = '';
      }
    }
  </script>
)";

  static constexpr const char* PROPERTY_TEMPLATE = R"(
    <div class='form-group'>
      <label for='property_{key}'>{key}:</label>
      <input type='text' id='property_{key}' name='property_{key}' value='{value}'>
    </div>
  )";
};

} // namespace iptvsimple
