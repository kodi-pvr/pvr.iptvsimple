# - Try to find pugixml
# Once done this will define
#
# PUGIXML_FOUND - system has pugixml
# PUGIXML_INCLUDE_DIRS - the pugixml include directory
# PUGIXML_LIBRARIES - the pugixml library

find_path(PUGIXML_INCLUDE_DIRS pugixml.hpp)
find_library(PUGIXML_LIBRARIES pugixml)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(pugixml REQUIRED_VARS PUGIXML_INCLUDE_DIRS PUGIXML_LIBRARIES)

mark_as_advanced(PUGIXML_INCLUDE_DIRS PUGIXML_LIBRARIES)
