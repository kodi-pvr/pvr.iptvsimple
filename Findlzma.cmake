# - Try to find lzma
# Once done this will define
#
# LZMA_FOUND - system has xz
# LZMA_INCLUDE_DIRS - the xz include directory
# LZMA_LIBRARIES - the xz library

find_path(LZMA_INCLUDE_DIRS lzma.h)
find_library(LZMA_LIBRARIES NAMES lzma liblzma)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(lzma REQUIRED_VARS LZMA_INCLUDE_DIRS LZMA_LIBRARIES)

mark_as_advanced(LZMA_INCLUDE_DIRS LZMA_LIBRARIES)
