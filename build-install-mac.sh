#!/bin/bash

set -e

if [ "$#" -ne 1 ] || ! [ -d "$1" ]; then
  echo "Usage: $0 <XBMC-SRC-DIR>" >&2
  exit 1
fi

if [[ "$OSTYPE" != "darwin"* ]]; then
  echo "Error: Script only for use on MacOSX" >&2
  exit 1
fi

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
if [[ "$1" = /* ]]
then
  #absolute path
  SCRIPT_DIR=""  
else
  #relative
  SCRIPT_DIR="$SCRIPT_DIR/"
fi

BINARY_ADDONS_TARGET_DIR="$1/tools/depends/target/binary-addons"
MACOSX_BINARY_ADDONS_TARGET_DIR=""
KODI_ADDONS_DIR="$HOME/Library/Application Support/Kodi/addons"
ADDON_NAME=`basename -s .git \`git config --get remote.origin.url\``

if [ ! -d "$BINARY_ADDONS_TARGET_DIR" ]; then
  echo "Error: Could not find binary addons directory at: $BINARY_ADDONS_TARGET_DIR" >&2
  exit 1
fi

for DIR in "$BINARY_ADDONS_TARGET_DIR/"macosx*; do
    if [ -d "${DIR}" ]; then
	    MACOSX_BINARY_ADDONS_TARGET_DIR="${DIR}"
      break
    fi
done

if [ -z "$MACOSX_BINARY_ADDONS_TARGET_DIR" ]; then
  echo "Error: Could not find binary addons build directory at: $BINARY_ADDONS_TARGET_DIR/macosx*" >&2
  exit 1
fi

if [ ! -d "$KODI_ADDONS_DIR" ]; then
  echo "Error: Kodi addons dir does not exist at: $KODI_ADDONS_DIR" >&2
  exit 1
fi

cd "$MACOSX_BINARY_ADDONS_TARGET_DIR"
make

XBMC_BUILD_ADDON_INSTALL_DIR=$(cd "$SCRIPT_DIR$1/addons/$ADDON_NAME" 2> /dev/null && pwd -P)
rm -rf "$KODI_ADDONS_DIR/$ADDON_NAME"
echo "Removed previous addon build from: $KODI_ADDONS_DIR"
cp -rf "$XBMC_BUILD_ADDON_INSTALL_DIR" "$KODI_ADDONS_DIR"
echo "Copied new addon build to: $KODI_ADDONS_DIR"
