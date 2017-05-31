[![Build Status](https://travis-ci.org/kodi-pvr/pvr.iptvsimple.svg?branch=master)](https://travis-ci.org/kodi-pvr/pvr.iptvsimple)
[![Coverity Scan Build Status](https://scan.coverity.com/projects/5120/badge.svg)](https://scan.coverity.com/projects/5120)

# IPTV Simple PVR
IPTV Live TV and Radio PVR client addon for [Kodi] (http://kodi.tv)

## Build instructions

### Linux

1. `git clone https://github.com/xbmc/xbmc.git`
2. `git clone https://github.com/kodi-pvr/pvr.iptvsimple.git`
3. `cd pvr.iptvsimple && mkdir build && cd build`
4. `cmake -DADDONS_TO_BUILD=pvr.iptvsimple -DADDON_SRC_PREFIX=../.. -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=../../xbmc/addons -DPACKAGE_ZIP=1 ../../xbmc/cmake/addons`
5. `make`

##### Useful links

* [Kodi's PVR user support] (http://forum.kodi.tv/forumdisplay.php?fid=167)
* [Kodi's PVR development support] (http://forum.kodi.tv/forumdisplay.php?fid=136)

## Radek Kubera (rkubera) modification
##### Features
1. Mutlitask recording
2. Time shifting support
3. EPG recording support

##### Requirements
1. avconv binary
2. rtmpdump binary

##### Tested with
1. OSX Sierra
2. Raspberry pi 2 (OSMC - debian)
3. Ubuntu 16.10
