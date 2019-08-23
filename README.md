[![Build Status](https://travis-ci.org/kodi-pvr/pvr.iptvsimple.svg?branch=master)](https://travis-ci.org/kodi-pvr/pvr.iptvsimple)
[![Coverity Scan Build Status](https://scan.coverity.com/projects/5120/badge.svg)](https://scan.coverity.com/projects/5120)

# IPTV Simple PVR

IPTV Live TV and Radio PVR client addon for [Kodi](http://kodi.tv)

## Build instructions

### Linux

1. `git clone --branch Leia https://github.com/xbmc/xbmc.git`
2. `git clone --branch Leia https://github.com/kodi-pvr/pvr.iptvsimple.git`
3. `cd pvr.iptvsimple && mkdir build && cd build`
4. `cmake -DADDONS_TO_BUILD=pvr.iptvsimple -DADDON_SRC_PREFIX=../.. -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=../../xbmc/addons -DPACKAGE_ZIP=1 ../../xbmc/cmake/addons`
5. `make`

### Mac OSX

In order to build the addon on mac the steps are different to Linux and Windows as the cmake command above will not produce an addon that will run in kodi. Instead using make directly as per the supported build steps for kodi on mac we can build the tools and just the addon on it's own. Following this we copy the addon into kodi. Note that we checkout kodi to a separate directory as this repo will only only be used to build the addon and nothing else.

#### Build tools and initial addon build

1. Get the repos
 * `cd $HOME`
 * `git clone --branch Leia https://github.com/xbmc/xbmc xbmc-addon`
 * `git clone --branch Leia https://github.com/kodi-pvr/pvr.iptvsimple`
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

##### Useful links

* [Kodi's PVR user support](http://forum.kodi.tv/forumdisplay.php?fid=167)
* [Kodi's PVR development support](http://forum.kodi.tv/forumdisplay.php?fid=136)
