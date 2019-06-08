cmake_minimum_required(VERSION 3.0)

project(openssl VERSION 1.1.1 LANGUAGES C)

find_package(StrawberryPerl REQUIRED)

include(CheckSymbolExists)
check_symbol_exists(_X86_ "Windows.h" _X86_)
check_symbol_exists(_AMD64_ "Windows.h" _AMD64_)

if(_X86_)
	message(STATUS "Win32")
	set(OPENSSL_PLATFORM VC-WIN32)
elseif(_AMD64_)
	message(STATUS "x64")
	set(OPENSSL_PLATFORM VC-WIN64A)
else()
	message(FATAL_ERROR "Unsupported target architecture")
endif()

message(STATUS "perl Configure ${OPENSSL_PLATFORM} disable-capieng enable-static-engine no-asm no-dso no-shared no-tests no-ui-console zlib --prefix=${CMAKE_INSTALL_PREFIX} --with-zlib-include=${OUTPUT_DIR}/include")
set(ENV{PATH} "${STRAWBERRYPERL_PATH};$ENV{PATH}")
execute_process(COMMAND perl Configure ${OPENSSL_PLATFORM} disable-capieng enable-static-engine no-asm no-dso no-shared no-tests no-ui-console zlib --prefix=${CMAKE_INSTALL_PREFIX} --with-zlib-include=${OUTPUT_DIR}/include WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

add_custom_target(openssl ALL COMMAND nmake build_generated libcrypto.lib libssl.lib WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

install(DIRECTORY include/openssl DESTINATION include/ FILES_MATCHING PATTERN "*.h")
install(FILES libcrypto.lib libssl.lib DESTINATION lib/)
