[![Build Status](https://travis-ci.org/kodi-pvr/pvr.iptvsimple.svg?branch=Matrix)](https://travis-ci.org/kodi-pvr/pvr.iptvsimple/branches)
[![Build Status](https://ci.appveyor.com/api/projects/status/github/kodi-pvr/pvr.iptvsimple?branch=Matrix&svg=true)](https://ci.appveyor.com/project/kodi-pvr/pvr-iptvsimple?branch=Matrix)
[![Coverity Scan Build Status](https://scan.coverity.com/projects/5120/badge.svg)](https://scan.coverity.com/projects/5120)

# IPTV Simple PVR

IPTV Live TV and Radio PVR client addon for [Kodi](https://kodi.tv)

## Build instructions

### Linux

1. `git clone --branch master https://github.com/xbmc/xbmc.git`
2. `git clone https://github.com/kodi-pvr/pvr.iptvsimple.git`
3. `cd pvr.iptvsimple && mkdir build && cd build`
4. `cmake -DADDONS_TO_BUILD=pvr.iptvsimple -DADDON_SRC_PREFIX=../.. -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=../../xbmc/addons -DPACKAGE_ZIP=1 ../../xbmc/cmake/addons`
5. `make`

### Mac OSX

In order to build the addon on mac the steps are different to Linux and Windows as the cmake command above will not produce an addon that will run in kodi. Instead using make directly as per the supported build steps for kodi on mac we can build the tools and just the addon on it's own. Following this we copy the addon into kodi. Note that we checkout kodi to a separate directory as this repo will only only be used to build the addon and nothing else.

#### Build tools and initial addon build

1. Get the repos
 * `cd $HOME`
 * `git clone https://github.com/xbmc/xbmc xbmc-addon`
 * `git clone https://github.com/kodi-pvr/pvr.iptvsimple`
2. Build the kodi tools
 * `cd $HOME/xbmc-addon/tools/depends`
 * `./bootstrap`
 * `./configure --host=x86_64-apple-darwin`
 * `make -j$(getconf _NPROCESSORS_ONLN)`
3. Build the addon
 * `cd $HOME/xbmc-addon`
 * `make -j$(getconf _NPROCESSORS_ONLN) -C tools/depends/target/binary-addons ADDONS="pvr.iptvsimple" ADDON_SRC_PREFIX=$HOME`

Note that the steps in the following section need to be performed before the addon is installed and you can run it in Kodi.

#### To rebuild the addon and copy to kodi after changes (after the initial addon build)

1. `cd $HOME/pvr.iptvsimple`
2. `./build-install-mac.sh ../xbmc-addon`

If you would prefer to run the rebuild steps manually instead of using the above helper script check the appendix [here](#manual-steps-to-rebuild-the-addon-on-macosx)

##### Useful links

* [Kodi's PVR user support](https://forum.kodi.tv/forumdisplay.php?fid=167)
* [Kodi's PVR development support](https://forum.kodi.tv/forumdisplay.php?fid=136)

### Settings Levels
In Kodi 18.2 the level of settings shown will correspond to the level set in the main kodi settings UI: `Basic`, `Standard`, `Advanced` and `Expert`. From Kodi 19 it will be possible to change the settingds level from within the addon settings itself.

### General
General settings required for the addon to function.

* **Location**: Select where to find the M3U resource. The options are:
    - `Local path` - A path to an M3U file whether it be on the device or the local network.
    - `Remote path` - A URL specifying the location of the M3U file.
* **M3U play list path**: If location is `Local path` this setting must contain a valid path for the addon to function.
* **M3U play list URL**: If location is `Remote path` this setting must contain a valid URL for the addon to function.
* **Cache M3U at local storage**: If location is `Remote path` select whether or not the the M3U file should be cached locally.
* **Start channel number**: The number to start numbering channels from.

### EPG Settings
Settings related to the EPG.

* **Location**: Select where to find the XMLTV resource. The options are:
    - `Local path` - A path to an XMLTV file whether it be on the device or the local network.
    - `Remote path` - A URL specifying the location of the XMLTV file.
* **XMLTV path**: If location is `Local Path` this setting should contain a valid path.
* **XMLTV URL**: If location is `Remote Path` this setting should contain a valid URL.
* **Cache XMLTV at local storage**: If location is `Remote path` select whether or not the the XMLTV file should be cached locally.
* **EPG time shift**: Adjust the EPG times by this value in minutes, range is from -720 mins to +720 mins (+/- 12 hours).
* **Apply time shift to all channels**: Whether or not to override the time shift for all channels with `EPG time shift`. If not enabled `EPG time shift` plus the individual time shift per channel (if available) will be used.

### Channel Logos
Settings realted to Channel Logos.

* **Location**: Select where to find the channel logos. The options are:
    - `Local path` - A path to a folder whether it be on the device or the local network.
    - `Remote path ` - A base URL specifying the location of the logos.
* **Channel logos folder**: If location is `Local Path` this setting should contain a valid folder.
* **Channel logos base URL**: If location is `Remote Path` this setting should contain a valid base URL.
* **Channel logos from XMLTV**: Preference on how to handle channel logos. The options are:
    - `Ignore` - Don't use channel logos from an XMLTV file.
    - `Prefer M3U` - Use the channel logo from the M3U if available otherwise use the XMLTV logo.
    - `Prefer XMLTV` - Use the channel logo from the XMLTV file if available otherwise use the M3U logo.

## Appendix

### Manual Steps to rebuild the addon on MacOSX

The following steps can be followed manually instead of using the `build-install-mac.sh` in the root of the addon repo after the [initial addon build](#build-tools-and-initial-addon-build) has been completed.

**To rebuild the addon after changes**

1. `rm tools/depends/target/binary-addons/.installed-macosx*`
2. `make -j$(getconf _NPROCESSORS_ONLN) -C tools/depends/target/binary-addons ADDONS="pvr.vuplus" ADDON_SRC_PREFIX=$HOME`

or

1. `cd tools/depends/target/binary-addons/macosx*`
2. `make`

**Copy the addon to the Kodi addon directory on Mac**

1. `rm -rf "$HOME/Library/Application Support/Kodi/addons/pvr.vuplus"`
2. `cp -rf $HOME/xbmc-addon/addons/pvr.vuplus "$HOME/Library/Application Support/Kodi/addons"`