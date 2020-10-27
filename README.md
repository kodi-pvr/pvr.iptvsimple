[![License: GPL-2.0-or-later](https://img.shields.io/badge/License-GPL%20v2+-blue.svg)](LICENSE.md)
[![Build Status](https://travis-ci.org/kodi-pvr/pvr.iptvsimple.svg?branch=Matrix)](https://travis-ci.org/kodi-pvr/pvr.iptvsimple/branches)
[![Build Status](https://dev.azure.com/teamkodi/kodi-pvr/_apis/build/status/kodi-pvr.pvr.iptvsimple?branchName=Matrix)](https://dev.azure.com/teamkodi/kodi-pvr/_build/latest?definitionId=57&branchName=Matrix)
[![Build Status](https://jenkins.kodi.tv/view/Addons/job/kodi-pvr/job/pvr.iptvsimple/job/Matrix/badge/icon)](https://jenkins.kodi.tv/blue/organizations/jenkins/kodi-pvr%2Fpvr.iptvsimple/branches/)
[![Coverity Scan Build Status](https://scan.coverity.com/projects/5120/badge.svg)](https://scan.coverity.com/projects/5120)

# IPTV Simple PVR

IPTV Live TV and Radio PVR client addon for [Kodi](https://kodi.tv)

For a listing of the supported M3U and XMLTV elements see the appendix [here](#supported-m3u-and-xmltv-elements)

## Build instructions

### Linux

1. `git clone --branch master https://github.com/xbmc/xbmc.git`
2. `git clone https://github.com/kodi-pvr/pvr.iptvsimple.git`
3. `cd pvr.iptvsimple && mkdir build && cd build`
4. `cmake -DADDONS_TO_BUILD=pvr.iptvsimple -DADDON_SRC_PREFIX=../.. -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=../../xbmc/addons -DPACKAGE_ZIP=1 ../../xbmc/cmake/addons`
5. `make`

### Mac OSX

In order to build the addon on mac the steps are different to Linux and Windows as the cmake command above will not produce an addon that will run in Kodi. Instead using make directly as per the supported build steps for Kodi on mac we can build the tools and just the addon on its own. Following this we copy the addon into Kodi. Note that we checkout Kodi to a separate directory as this repo will only only be used to build the addon and nothing else.

#### Build tools and initial addon build

1. Get the repos
 * `cd $HOME`
 * `git clone https://github.com/xbmc/xbmc xbmc-addon`
 * `git clone https://github.com/kodi-pvr/pvr.iptvsimple`
2. Build the Kodi tools
 * `cd $HOME/xbmc-addon/tools/depends`
 * `./bootstrap`
 * `./configure --host=x86_64-apple-darwin`
 * `make -j$(getconf _NPROCESSORS_ONLN)`
3. Build the addon
 * `cd $HOME/xbmc-addon`
 * `make -j$(getconf _NPROCESSORS_ONLN) -C tools/depends/target/binary-addons ADDONS="pvr.iptvsimple" ADDON_SRC_PREFIX=$HOME`

Note that the steps in the following section need to be performed before the addon is installed and you can run it in Kodi.

#### To rebuild the addon and copy to Kodi after changes (after the initial addon build)

1. `cd $HOME/pvr.iptvsimple`
2. `./build-install-mac.sh ../xbmc-addon`

If you would prefer to run the rebuild steps manually instead of using the above helper script check the appendix [here](#manual-steps-to-rebuild-the-addon-on-macosx)

##### Useful links

* [Kodi's PVR user support](https://forum.kodi.tv/forumdisplay.php?fid=167)
* [Kodi's PVR development support](https://forum.kodi.tv/forumdisplay.php?fid=136)

### Settings Levels
In Kodi 18.2 the level of settings shown will correspond to the level set in the main Kodi settings UI: `Basic`, `Standard`, `Advanced` and `Expert`. From Kodi 19 it will be possible to change the settings level from within the addon settings itself.

### General
General settings required for the addon to function.

* **Location**: Select where to find the M3U resource. The options are:
    - `Local path` - A path to an M3U file whether it be on the device or the local network.
    - `Remote path` - A URL specifying the location of the M3U file.
* **M3U play list path**: If location is `Local path` this setting must contain a valid path for the addon to function.
* **M3U play list URL**: If location is `Remote path` this setting must contain a valid URL for the addon to function.
* **Cache M3U at local storage**: If location is `Remote path` select whether or not the the M3U file should be cached locally.
* **Start channel number**: The number to start numbering channels from. Only used when `Use backend channel numbers` from PVR settings is enabled and a channel number is not supplied in the M3U file.
* **Only number by channel order in M3U**: Ignore any 'tvg-chno' tags and only number channels by the order in the M3U starting at 'Start channel number'.
* **Auto refresh mode**: Select the auto refresh mode for the M3U/XMLTV files. Note that caching is disabled if auto refresh is used. The options are:
    - `Disabled` - Don't auto refresh the files.
    - `Repeated refresh` - Refresh the files on a minute based interval.
    - `Once per day` - Refresh the files once per day.
* **Refresh interval**: If auto refresh mode is `Repeated refresh` refresh the files every time this number of minutes passes. Max 120 minutes.
* **Refresh hour (24h)**: If auto refresh mode is `Once per day` refresh the files every time this hour of the day is reached.

### EPG
Settings related to the EPG.

For settings related to genres please see the next section.

* **Location**: Select where to find the XMLTV resource. The options are:
    - `Local path` - A path to an XMLTV file whether it be on the device or the local network.
    - `Remote path` - A URL specifying the location of the XMLTV file.
* **XMLTV path**: If location is `Local Path` this setting should contain a valid path.
* **XMLTV URL**: If location is `Remote Path` this setting should contain a valid URL.
* **Cache XMLTV at local storage**: If location is `Remote path` select whether or not the the XMLTV file should be cached locally.
* **EPG time shift**: Adjust the EPG times by this value, from -12 hours to +14 hours.
* **Apply time shift to all channels**: Whether or not to override the time shift for all channels with `EPG time shift`. If not enabled `EPG time shift` plus the individual time shift per channel (if available) will be used.

### Genres
Settings related to genres.

The addon will read all the `<category>` elements of a `programme` and use this as the genre string. It is also possible to supply a mapping file to convert the genre string to a genre ID, allowing colour coding of the EPG. When using a mapping file each category will be checked in order until a match is found. Please see: [Using a mapping file for Genres](#using-a-mapping-file-for-genres) in the Appendix for details on how to set this up.

* **Use genre text from XMLTV when mapping genres**: If enabled, and a genre mapping file is used to get a genre type and sub type use the EPG's genre text (i.e. 'category' text) for the genre instead of the Kodi default text. Only the genre type (and not the sub type) will be used if a mapping is found.
* **Location**: Select where to find the genres XML resource. The options are:
    - `Local path` - A path to a gernes XML file whether it be on the device or the local network.
    - `Remote path` - A URL specifying the location of the genres XML file.
* **Genres path**: If location is `Local Path` this setting should contain a valid path.
* **Genres URL**: If location is `Remote Path` this setting should contain a valid URL.

### Channel Logos
Settings related to Channel Logos.

* **Location**: Select where to find the channel logos. The options are:
    - `Local path` - A path to a folder whether it be on the device or the local network.
    - `Remote path ` - A base URL specifying the location of the logos.
* **Channel logos folder**: If location is `Local Path` this setting should contain a valid folder.
* **Channel logos base URL**: If location is `Remote Path` this setting should contain a valid base URL.
* **Channel logos from XMLTV**: Preference on how to handle channel logos. The options are:
    - `Ignore` - Don't use channel logos from an XMLTV file.
    - `Prefer M3U` - Use the channel logo from the M3U if available otherwise use the XMLTV logo.
    - `Prefer XMLTV` - Use the channel logo from the XMLTV file if available otherwise use the M3U logo.

### Timeshift
Timeshift settings for pausing/rewinding and fast-forwarding live streams.

Note that if your stream does is not using a supported protocol you can still enable timeshift by adding some `KODIPROP`'s to the M3U file. The following three properties can be added before each channel/stream you wish to enable it for.

```
#KODIPROP:inputstream=inputstream.ffmpegdirect
#KODIPROP:inputstream.ffmpegdirect.stream_mode=timeshift
#KODIPROP:inputstream.ffmpegdirect.is_realtime_stream=true
```

For any TS stream an additional property should be added to every M3U entry using timeshift. This will speed up initial load time:

```
#KODIPROP:mimetype=video/mp2t
```

* **Enable timeshift**: Enable the timeshift feature.
* **Enable timeshift for all streams**: Enable the timeshift feature for all streams. Will only apply to streams that do not select a specific inputstream addon for playback. The `inputstream.ffmpegdirect` addon will be used for timeshift."
* **Enable timeshift for HTTP based streams**: Enable the timeshift feature for HTTP based streams. Will only apply to streams that do not select a specific inputstream addon for playback. The `inputstream.ffmpegdirect` addon will be used for timeshift."
* **Enable timeshift for UDP based streams**: Enable the timeshift feature for UDP based streams. Will only apply to streams that do not select a specific inputstream addon for playback. The `inputstream.ffmpegdirect` addon will be used for timeshift."
* **Always enable timeshift stream mode if missing**: If a `stream_mode` property is not specified along with an `inputstream.ffmpegdirect` property always add it for any supported stream types. Note that this setting has no effect on catchup VOD streams that are not live.
* **Modify inputstream.ffmpegdirect settings..**: Open settings dialog for inputstream.ffmpegdirect for modification of timeshift and other settings.

### Catchup
Catchup settings for viewing archived live programmes. Allows the option of 'Play programme' when viewing EPG entry info. Only works if your provider supports catchup.

Note that catchup functionality can work one of two ways. The first is where a standard inputstream is used, allowing the use of the EPG to view a programme from within the catchup window like a video. The second is where the `inputstream.ffmpegdirect` addon is installed which allows timeshift, either as a video or with playback as live. When playback as live is enabled it's possible to skip back/forward programme by programme and jump to other channel and programmes via the OSD menus. Also note that using `inputstream.ffmpegdirect` is automatic if using TS/M3U8 streams that support catchup. If this inputstream is incorrectly chosen it's possible to override this by adding a `#KODIPROP:inputstream=inputstream.ffmpeg` line before the channel's M3U entry (or any alternative inputstream instead).

Catchup tags can be specified in the M3U entries and these tags override any values configured in the Catchup settings. The supported M3U catchup tags are `catchup`, `catchup-source` and `catchup-days`. See the [Supported M3U elements section](#m3u-format-elements) below for further details. The full catchup URL or a query to be appended to the stream URL is either provided in the `catchup-source` tag (i.e. complete with format specifiers) or in the case this is not provided the `Query format string` setting is appended to stream URL. See the [Catchup format specifiers section](#catchup-format-specifiers) below for a more detailed explanation and further examples.

Note that some modes/providers only permit catchup streams that terminate, i.e. the stream will play for a time and then stop. If using `inputstream.ffmpegdirect` for timeshifting then these streams will restart when they end if they are close to live (within 4 hours). If order for this to work seeking near live is limited to not closer than 1 minute for streams with a 1 second precision and 2 minutes for streams will a 60 second precision.

Addon settings for catchup:

* **Enable catchup**: Should be enabled if there are channels supporting catchup in your M3U list.
* **Query format string**: A format string (provider dependent) allowing timestamp information to be appended to a URL to denote when to catchup from. E.g. `&cutv={Y}-{m}-{d}T{H}:{M}:{S}`, which allows year, month, day, hour minute and second to be inserted to give: `&cutv=2019-11-26T22:00:32`. If the M3U entry using has a catchup mode of `default` or `append` and a `catchup-source` tag is provided in the M3U entry this setting will be ignored.
* **Catchup window**: The number of days into the past in which it is possible to catchup on a show. Can be overidden in an M3U entry using a 'catchup-days' tag.
* **All channels support catchup**: If enabled it is assumed that all channels support catchup. If there are no catchup specific tags in the M3U entries then the stream URL will be used as the source, and the URL format and catchup days will come from the addon settings. If this option is disabled then a M3U entry must have at least a `catchup="true"` or `catchup="default"` tag to enable catchup.
* **All channels support catchup using mode**: If enabled it is assumed that all channels support catchup using the selected mode if they do not have catchup tags. In this case the 'Query format string' and 'Catchup window' number of days will come from the addon settings if needed. If this option is disabled then an M3U entry must have at least a `catchup=` tag to enable catchup. The options for how to build the catch URL are:
    - `Disabled` - Do not assume all channel support catchup.
    - `Default` - Use catchup source as the full catchup URL, if there is no catchup source use Append mode.
    - `Append` - Append the catchup source to the channel URL, if there is no catchup source append the `Query format string` instead.
    - `Shift (SIPTV)` - Append the standard SIPTV catchup string to the channel URL.
    - `Flussonic` - Build a flussonic URL from the channel URL.
    - `Xtream codes` - Build an Xtream codes URL from the channel URL.
* **Play from EPG in Live TV mode (using timeshift)**: When disabled any catchup show from the past will be played like a video (bounded by start and end times). If enabled, it will instead act like a live stream with timeshift, also allowing the ability to skip back and forward programmes. Note that the only effect this option has on streams that do not support timeshifting is whether or not to apply the before/after buffer.
* **Buffer before programme start**: The amount of buffer to give before the playback start point of an EPG entry that will be watched as a video.
* **Buffer after programme end**: The amount of buffer to give after the playback end point of an EPG entry that will be watched as a video.
* **Catchup only available on finished programmes**: When selected from the EPG the current live programme cannot be watched as catchup until finished.

### Advanced
Advanced settings such as multicast relays.

* **Transform multicast stream URLs**: Multicast (UDP/RTP) streams do not work well on Wifi networks. A multicast relay can convert the stream from UDP/RTP multicast to HTTP. Enabling this option will transform multicast stream URLs from the M3U file to HTTP addresses so they can be accesssed via a 'udpxy' relay on the local network. E.g. a UDP multicast stream URL like `udp://@239.239.3.38:5239` would get transformed to something like `http://192.168.1.1:4000/udp/239.239.3.38:5239`.
* **Relay hostname or IP address**: The hostname or ip address of the multicast relay (`udpxy`) on the local network.
* **Relay port**: The port of the multicast relay (`udpxy`) on the local network.
* **Use FFMpeg http reconnect options if possible**: Note this can only apply to http/https streams that are processed by libavformat (e.g. M3u8/HLS). Using libavformat can be specified in an M3U file by setting the property `inputstream` as `inputstream.ffmpeg`. I.e. adding the line: `#KODIPROP:inputstream=inputstream.ffmpeg`. If this opton is not enabled it can still be enabled per stream/channel by adding a Kodi property, i.e.: `#KODIPROP:http-reconnect=true`.
* **Use inputstream.adaptive for m3u8 (HLS) streams**: Use inputstream.adaptive instead of ffmpeg's libavformat for m3u8 (HLS) streams.
* **User-Agent**: Select which User-Agent to use if there is not one supplied as a property or as part of the channel stream URL.
* **Inputstream name**: Use this inputsream as the default if there is not one supplied as a property (KODIPROP) of the channel. Use with care as this will disable any use of the addon's default stream inspection behaviour. Note that for `inputstream.ffmpegdirect` if both `mimetype` and `inputstream.ffmpegdirect.manifest_type` are unset stream inspection will still occur if required.
* **MIME type**: Use this MIME type as the default if there is not one supplied as a property (KODIPROP) of the channel. Use with care as this will disable any use of the addon's default stream inspection behaviour.

## Appendix

### Catchup format specifiers

The format specifiers are substitution based and work as follows:

- `{utc}`: The start time of the programme in UTC format.
- `${start}`: Same as `{utc}`.
- `{lutc}`: Current time in UTC format.
- `${timestamp}`: Same as `{lutc}`.
- `{utcend}`: The start time of the programme in UTC format + `${duration}`.
- `${end}`: Same as `{utcend}`.
- `{Y}`: The 4-digit year (YYYY) of the start date\time.
- `{m}`: The month (01-12) of the start date\time.
- `{d}`: The day (01-31) of the start date\time.
- `{H}`: The hour (00-23) of the start date\time.
- `{M}`: The minute (00-59) of the start date\time.
- `{S}`: The second (00-59) of the start date\time.
- `{duration}`: The programme duration + any start and end buffer (if set).
- `{duration:X}`: The programme duration (as above) divided by X seconds. Allows conversion to minutes and other time units. The minimum divider is 1, it must be an integer (not 1.5 or 2.25 etc.) and it must be a positive value. E.g. If you have a duration of 7200 seconds and you need 2 hours (2 hours is 7200 seconds), it means your divider is 3600: `{offset:3600}`. If you need minutes for the same duration you could use: `{offset:60}` which would result in a value of 120.
- `{offset:X}`: The current offset (now - start time) divided by X seconds. Allows conversion to minutes and other time units. The minimum divider is 1, it must be an integer (not 1.5 or 2.25 etc.) and it must be a positive value. E.g. If you need an offset of 720 for a start time of 2 hours ago (2 hours is 7200 seconds), it means your divider is 10: `{offset:10}`. If you need minutes for the same offset you could use: `{offset:60}` which would result in a value of 120.
- `{catchup-id}`: A programme specific identifier required in the catchup URL, value loaded from XMLTV programme entries.

Here’s some examples of how the different formats would look:
- `?utc={utc}&lutc={lutc}`
- `&t={Y}-{m}-{d}-{H}-{M}-{S}`
- `?offset={offset:60}`

### Supported M3U and XMLTV elements

#### M3U format elements:

```
#EXTM3U tvg-shift="-4.5" x-tvg-url="http://path-to-xmltv/guide.xml catchup-correction="-2.5"
#EXTINF:0 tvg-id="channel-x" tvg-name="Channel_X" group-title="Entertainment" tvg-chno="10" tvg-logo="http://path-to-icons/channel-x.png" radio="true" tvg-shift="-3.5",Channel X
#EXTVLCOPT:program=745
#KODIPROP:key=val
http://path-to-stream/live/channel-x.ts
#EXTINF:0 tvg-id="channel-x" tvg-name="Channel-X-HD" group-title="Entertainment;HD Channels",Channel X HD
http://path-to-stream/live/channel-x-hd.ts
#EXTINF:0 tvg-id="channel-y" tvg-name="Channel_Y",Channel Y
#EXTGRP:Entertainment
http://path-to-stream/live/channel-y.ts
#EXTINF:0,Channel Z
http://path-to-stream/live/channel-z.ts
#EXTINF:0 catchup="default",Channel A
http://path-to-stream/live/channel-a.ts
#EXTINF:0 catchup="default" catchup-source="http://path-to-stream/live/catchup-b.ts&cutv={Y}-{m}-{d}T{H}:{M}:{S}" catchup-days="3",Channel B
http://path-to-stream/live/channel-b.ts
#EXTINF:0 catchup="append" catchup-source="&cutv={Y}-{m}-{d}T{H}:{M}:{S}" catchup-days="3" catchup-correction="-4.0",Channel C
http://path-to-stream/live/channel-c.ts
#EXTINF:0 tvg-id="channel-d" tvg-name="Channel-D" catchup="shift" catchup-days="3",Channel D
http://path-to-stream/live/channel-d.ts
#EXTINF:0 tvg-id="channel-e" tvg-name="Channel-E" timeshift="3",Channel E
http://path-to-stream/live/channel-e.ts
#EXTINF:0 tvg-id="channel-f" tvg-name="Channel-F" catchup="default" catchup-days="1" catchup-source="http://yoururl/channeld/video-{utc}-{duration}.m3u8",Channel F
http://yoururl/channeld/video.m3u8
#EXTINF:-1 catchup="fs",Channel G
http://list.tv:8888/325/mono.m3u8?token=secret
#EXTINF:-1 catchup="fs",Channel H
http://list.tv:8080/my@account.xc/my_password/1477
#EXTINF:-1 catchup="fs",Channel I
http://list.tv:8080/live/my@account.xc/my_password/1477.m3u8
#EXTINF:-1 catchup="vod" catchup-source="http://path-to-stream/live/catchup-j.ts&cutv={Y}-{m}-{d}T{H}:{M}:{S}" catchup-days="3",Channel J
http://path-to-stream/live/channel-j.ts
#EXTINF:-1 catchup="vod",Channel K
plugin://plugin.video.my-vod-addon/play/catalog/channels/d8659669-b964-414c-aa9c-e31d8d15696b
```

*Explanation for Catchup entries*
- For `Channel A` the stream URL will be used and the Query format string from the addon settings will be appended to construct the catchup URL.
- For `Channel B` the stream URL will not be used, instead using catchup-source as the catchup URL.
- For `Channel C` the stream URL will be used and catchup-source will be appended to form the catchup URL. The time used for the URL generation will corrected by -4.0 hours.
- For `Channel D` this is an example of a siptv style entry which auto generates the catchup-source by appending `?utc={utc}&lutc={lutc}` or `&utc={utc}&lutc={lutc}` to the channel URL.
- For `Channel E` this is an example of the old siptv style entry which uses a `timeshift` tag to combine shift and days into one field. The catchup-source output will be the same as channel D.
- For `Channel F` this is an example of a flussonic style entry manually specifying the catcup-source. Note that the mode is still `default`.
- For `Channel G` this is an example of a flussonic style entry which auto generates the catchup-source.
- For `Channel H` this is an example of a xtream codes style entry which auto generates the catchup-source for `ts` streams.
- For `Channel I` this is an example of a xtream codes style entry which auto generates the catchup-source for `m3u8` streams.
- For `Channel J` this is an example of a VOD style entry which will only populated and play the `catchup-source` using a value of 3 `catchup-days`.
- For `Channel K` this is an example of a VOD style entry which uses a default `catchup-source` of `{catchup-id}` and will allow playback of any EPG entry with a `catchup-id` past, present or future via a Kodi plugin URL.

*Channel k Plugin example:*

-*m3u*
```
#EXTINF:-1 tvg-chno="131" tvg-id="131@footv" tvg-name="FooBar" tvg-logo="https://images.provider.iptv/channels/foobar/logo.png" group-title="Drama" radio="false" catchup="vod",FooBar TV
https://service.provider.iptv/hls/channel/foobar/master.m3u8
```
-*programme*
```
<programme start="20201021230000" channel="131@footv" catchup-id="plugin://plugin.video.footv/?mode=vod&amp;&amp;catchupid=episode1" stop="20201022010000">
    <title lang="en">All Aboard: Foo Bar</title>
</programme>
```

------------

Note: The minimum required for a channel/stream is an `#EXTINF` line with a channel name and the `URL` line. E.g. a minimal version of the example file above would be:

```
#EXTM3U
#EXTINF:0,Channel X
http://path-to-stream/live/channel-x.ts
#EXTINF:0,Channel X HD
http://path-to-stream/live/channel-x-hd.ts
#EXTINF:0,Channel Y
http://path-to-stream/live/channel-y.ts
#EXTINF:0,Channel Z
http://path-to-stream/live/channel-z.ts
```

- `#EXTM3U`: Marker for the start of an M3U file.
  - `tvg-shift`: Value that will be used for all channels if a `tvg-shift` value is not supplied per channel.
  - `x-tvg-url`: URL for the XMLTV data. Only used if the addon settings do not contain an EPG location for XMLTV data.
  - `catchup-correction`: Value that will be used for all channels if a `catchup-correction` value is not supplied per channel.
- `#EXTINF`: Contains a set of values, ending with a comma followed by the `channel name`.
  - `tvg-id`: A unique identifier for this channel used to map to the EPG XMLTV data.
  - `tvg-name`: A name for this channel in the EPG XMLTV data.
  - `group-title`: A semi-colon separted list of channel groups that this channel belongs to.
  - `tvg-chno`: The number to be used for this channel.
  - `tvg-logo`: A URL pointing to the logo for this channel. For relative URLs `.png` will be appended if not provided, absolute URLs will not be modified.
  - `radio`: If the value matches "true" (case insensitive) this is a radio channel.
  - `tvg-shift`: Channel specific timezone shift value in hours for epg entries.
  - `catchup`: Which mode of catchup to use. Supported modes of `default` and `append`. Required unless the setting `All channels support catchup` is enabled.
  - `catchup-source`: Can contain the full catchup URL (complete with format specifiers) if in `default` mode. Or if the mode is `append` just the query string with format specifiers which will be appended to the channel URL. If omitted the `Query format string` from settings will be appended.
  - `catchup-days`: The number of days in the past catchup is available for.
  - `catchup-correction`: A correction to the time used for catchup stream URL generation. Useful for catchup streams which are geo mis-matched to the wrong time.
- `#EXTGRP`: A semi-colon separted list of channel groups that this channel belongs to.
- `#KODIPROP`: A single property in the format `key=value` that can be passed to Kodi. Multiple can be passed each on a separate line.
- `#EXTVLCOPT`: A single property in the format `key=value` that can be passed to Kodi. Multiple can be passed each on a separate line. Note that if either a `http-user-agent` or a `http-referrer` property is found it will added to the URL as a HTTP header as `user-agent` or `referrer` respectively if not already provided in the URL. These two fields specifically will be dropped as properties whether or not they are added as header values. They will be added in the same format as the `URL` below.
- `#EXT-X-PLAYLIST-TYPE`: If this element is present with a value of `VOD` (Video on Demand) the stream is marked as not being live.
- `URL`: The final line in each channel stanza is the URL used for the stream. Appending `|user-agent=<agent-name>` will change the user agent. Other HTTP header fields can be set in the same fashion: `|name1=val1&name2=val2` etc. The header fields supported in this way by Kodi can be found [here](#http-header-fields-supported-by-kodi). If you want to pass custom headers that are not supported by Kodi you need to prefix them with an `!`, for example: : `|!name1=val1&!name2=val2`.

When processing an XMLTV file the addon will attempt to find a channel loaded from the M3U that matches the EPG channel. It will cycle through the full set of M3U channels checking for one condition on each pass. The first channel found to match is the channel chosen for this EPG channel data.

 - *1st pass*: Does the`id` attribute of the `<channel>` element from the XMLTV match the `tvg-id` from the M3U channel. If yes we have a match, don't continue.
  - *Before the second pass*: Was a <display-name> value provided, if not skip this channels EPG data.
 - *2nd pass*: Does the <display-name> as it is or with spaces replaced with '_''s match `tvg-name` from the M3U channel. If yes we have a match, don't continue.
 - *3rd pass*: Does the <display-name> match the M3U `channel name`. If yes we have a match, phew, eventually found a match.

#### XMLTV format elemnents:

General information on the XMLTV format can be found [here](http://wiki.xmltv.org/index.php/XMLTVFormat). There is also the [DTD](https://github.com/XMLTV/xmltv/blob/master/xmltv.dtd).

**Channel elements**
```
<channel id="channel-x">
  <display-name>Channel X</display-name>
  <display-name>Channel X HD</display-name>
  <icon src="http://path-to-icons/channel-x.png"/>
</channel>
```

- When matching against M3U channels the `id` attribute will be used first, followed by each `display-name`.
- If multiple `icon` elements are provided only the first will be used.

**Programme elements**
```
  <programme start="20080715003000 -0600" stop="20080715010000 -0600" channel="channel-x" catchup-id="34534590">
    <title>My Show</title>
    <desc>Description of My Show</desc>
    <category>Drama</category>
    <category>Mystery</category>
    <sub-title>Episode name for My Show</sub-title>
    <date>20080711</date>
    <star-rating>
      <value>6/10</value>
    </star-rating>
    <episode-num system="xmltv_ns">0.1.0/1</episode-num>
    <episode-num system="onscreen">S01E02</episode-num>
    <credits>
      <director>Director One</director>
      <writer>Writer One</writer>
      <actor>Actor One</actor>
    </credits>
    <icon src="http://path-to-icons/my-show.png"/>
  </programme>
```
The `programme` element supports the attributes `start`/`stop` in the format `YYYmmddHHMMSS +/-HHMM` and the attribute `channel` which needs to match the `channel` element's attribute `id`. There is an extra attribute which is not part of the XMLTV specification called `catchup-id`. Some providers require a programme specific id, this value can be used as part of the [catchup query format string specifiers](#catchup-format-specifiers).

- `title`: The title of the prgramme.
- `desc`: A descption of the programme.
- `category`: If multiple elements are provided only the first will be used to populate the genre.
- `sub-title`: Used to populate episode name.
- `date`: Used to populate year and first aired date.
- `star-rating`: If multiple elements are provided only the first will be used. The value will be converted to a scale of 10 if required.
- `episode-num`: The`xmltv_ns`system will be preferred over `onscreen` and the first successfully parsed element will be used.
  - For `episode-num` elements using the `xmltv_ns` system at least season and episode must be supplied, i.e. `0.1` (season 1, episode 2). If the 3rd element episode part number is supplied it must contain both the part number and the total number of parts, i.e. `0.1.0/2` (season 1, episode 2, part 1 of 2).
  - For `episode-num` elements using the `onscreen` system only the `S01E02` format is supported.
- `credits`: Only director, writer and actor are supported (multiple of each can be supplied).
- `icon`: If multiple elements are provided only the first will be used.

### Config files

The various config files have examples allowing users to create their own, making it possible to support custom config, currently regarding genres. The best way to learn about them is to read the config files themselves. Each contains details of how the config file works.

All of the files listed below are overwritten each time the addon starts (excluding genres.xml). Therefore if you are customising files please create new copies with different file names. Note: that only the files below are overwritten any new files you create will not be touched.

After adding and selecting new config files you will need to clear the EPG cache `Settings->PVR & Live TV->Guide->Clear cache` for it to take effect in the case of EPG relatd config and for channel related config will need to clear the full cache `Settings->PVR & Live TV->General->Clear cache`.

If you would like to support other formats/languages please raise an issue at the github project https://github.com/kodi-pvr/pvr.iptvsimple, where you can either create a PR or request your new configs be shipped with the addon.

There is one config file located here: `userdata/addon_data/pvr.iptvsimple/genres/kodiDvbGenres.xml`. This simply contains the DVB genre IDs that Kodi supports and uses hex for the IDs. Can be a useful reference if creating your own configs. There is also `userdata/addon_data/pvr.iptvsimple/genres/kodiDvbGenresTypeSubtype.xml`, which uses two decimal values instead of hex. This file is also overwritten each time the addon restarts.

### Using a mapping file for Genres

Users can create there own genre mapping files to map their genre strings to genre IDs. This allows the EPG UI to be colour coded per genre.

Kodi uses the following standard for its genre IDs: https://www.etsi.org/deliver/etsi_en/300400_300499/300468/01.11.01_60/en_300468v011101p.pdf

By default the addon will try to load a file called `genres.xml` and expect it to be here: `userdata/addon_data/pvr.iptvsimple/genres/genreTextMappings/`. However any genres file can be chosen in the addon settings.

The following files are currently available with the addon (this file uses hexadeciaml `genreId` attributes):
    - `Rytec-UK-Ireland.xml`

The file can specify either a hexadecimal `genreId` attribute (recommended) or separate integer values for `type` and `subType`. Mathematically `genreId` is equals to the logical OR or `type` and `subType`, i.e. `genreId = type | subType`.

Note: Once mapped to genre IDs the text displayed can either be the DVB standard text or the genre string text supplied in the XML. If using the text supplied in the XML only the genre type will be passed and each value will correspond to a category and colour (depedning on skin) on the UI. Here are the categories (all examples have 0 for the sub type). It's important you map correctly as genres can be used for search.

```
- 0x10: General Movie / Drama
- 0x20: News / Current Affairs
- 0x30: Show / Game Show
- 0x40: Sports
- 0x50: Children's / Youth Programmes
- 0x60: Music / Ballet / Dance
- 0x70: Arts / Culture
- 0x80: Social / Political / Economics
- 0x90: Education / Science / Factual
- 0xA0: Leisure / Hobbies
- 0xB0: Special Characteristics
```

- `<name>`: There should be a single `<name>` element. The value should denote the purpose of this genre mapping file.
- The value of the `<genre>` element is what is used to map from in order to get the genre IDs. Many mapping values are allowed to map to the same IDs.

**Example using hexadecimal `genreId` attributes (recommended)**:

```
<genres>
  <name>My Streams Genres Mappings</name>
  <genre genreId="0x10">Movie</genre>               <!-- General Movie/Drama - 0x10 in DVB hex-->
  <genre genreId="0x10">Movie - Comedy</genre>      <!-- General Movie/Drama - 0x10 in DVB hex-->
  <genre genreId="0x10">Movie - Romance</genre>     <!-- General Movie/Drama - 0x10 in DVB hex-->
  <genre genreId="0x30">TV Show</genre>             <!-- Show/Game Show - 0x30 in DVB hex-->
  <genre genreId="0x30">Game Show</genre>           <!-- Show/Game Show - 0x30 in DVB hex-->
  <genre genreId="0x30">Talk Show</genre>           <!-- Show/Game Show - 0x30 in DVB hex-->
  <genre genreId="0xA0">Leisure</genre>             <!-- Leisure/Hobbies - 0xA0 in DVB hex-->
</genres>
```

- The `genreId` attribute is a single hex value ranging from 0x10 to 0xFF.

**Example using integer `type` and `subtype` attributes**:

```
<genres>
  <name>My Streams Genres Mappings</name>
  <genre type="16" subtype="0">Movie</genre>               <!-- General Movie/Drama - 0x10 in DVB hex-->
  <genre type="16" subtype="0">Movie - Comedy</genre>      <!-- General Movie/Drama - 0x10 in DVB hex-->
  <genre type="16" subtype="0">Movie - Romance</genre>     <!-- General Movie/Drama - 0x10 in DVB hex-->
  <genre type="54" subtype="0">TV Show</genre>             <!-- Show/Game Show - 0x30 in DVB hex-->
  <genre type="54" subtype="0">Game Show</genre>           <!-- Show/Game Show - 0x30 in DVB hex-->
  <genre type="54" subtype="0">Talk Show</genre>           <!-- Show/Game Show - 0x30 in DVB hex-->
  <genre type="160" subtype="0">Leisure</genre>            <!-- Leisure/Hobbies - 0xA0 in DVB hex-->
</genres>
```

- The `type` attribute can contain a values ranging from 16 to 240 in multiples of 16 (would be 0x10 to 0xF0 if in hex) and the `subtype` attributes can contain a value from 0 to 15 (would be 0x00 to 0x0F if in hex). `subtype` is optional.


### HTTP header fields supported by Kodi

HTTP header fields can be sent by appending the following format to the URL: `|name1=val1&name2=val2`. Note that Kodi does not support sending not standard header fields.

**Special fields**

`cookie, cookies, seekable, user-agent`

**Standard fields**

`accept, accept-language, accept-datetime, authorization, cache-control, connection, content-md5, date, expect, forwarded, from, if-match, if-modified-since, if-none-match, if-range, if-unmodified-since, max-forwards, origin, pragma, range, referer, te, upgrade, via, warning, x-requested-with, dnt, x-forwarded-for, x-forwarded-host, x-forwarded-proto, front-end-https, x-http-method-override, x-att-deviceid, x-wap-profile, x-uidh, x-csrf-token, x-request-id, x-correlation-id`

**Other fields supported by ffmpeg**

`reconnect_at_eof, reconnect_streamed, reconnect_delay_max, icy, icy_metadata_headers, icy_metadata_packet`

### Manual Steps to rebuild the addon on MacOSX

The following steps can be followed manually instead of using the `build-install-mac.sh` in the root of the addon repo after the [initial addon build](#build-tools-and-initial-addon-build) has been completed.

**To rebuild the addon after changes**

1. `rm tools/depends/target/binary-addons/.installed-macosx*`
2. `make -j$(getconf _NPROCESSORS_ONLN) -C tools/depends/target/binary-addons ADDONS="pvr.iptvsimple" ADDON_SRC_PREFIX=$HOME`

or

1. `cd tools/depends/target/binary-addons/macosx*`
2. `make`

**Copy the addon to the Kodi addon directory on Mac**

1. `rm -rf "$HOME/Library/Application Support/Kodi/addons/pvr.iptvsimple"`
2. `cp -rf $HOME/xbmc-addon/addons/pvr.iptvsimple "$HOME/Library/Application Support/Kodi/addons"`
