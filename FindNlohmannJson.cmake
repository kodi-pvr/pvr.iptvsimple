# - Find NlohmannJson
# Find the native NlohmannJson includes
#
#   NLOHMANNJSON_FOUND        - True if NlohmannJson found.
#   NLOHMANNJSON_INCLUDE_DIRS - where to find nlohmann/json.hpp, etc.
#

find_path(NLOHMANNJSON_INCLUDE_DIR nlohmann/json.hpp)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(NlohmannJson DEFAULT_MSG NLOHMANNJSON_INCLUDE_DIR)

if(NLOHMANNJSON_FOUND)
  set(NLOHMANNJSON_INCLUDE_DIRS ${NLOHMANNJSON_INCLUDE_DIR})
endif()

mark_as_advanced(NLOHMANNJSON_INCLUDE_DIR)
