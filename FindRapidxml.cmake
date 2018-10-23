# - Try to find rapidxml
# Once done this will define
#
# RAPIDXML_FOUND - system has rapidxml
# RAPIDXML_INCLUDE_DIRS - the rapidxml include directory

find_path(RAPIDXML_INCLUDE_DIRS rapidxml/rapidxml.hpp)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Rapidxml DEFAULT_MSG RAPIDXML_INCLUDE_DIRS)

mark_as_advanced(RAPIDXML_INCLUDE_DIRS)
