cmake_minimum_required(VERSION 3.22)
set(SC_MODULE qrencode)
project(simply-cpp-${SC_MODULE} HOMEPAGE_URL "https://github.com/roelofrossouw/simply-cpp-qrencode" LANGUAGES CXX)
include(cmake/sc_bootstrap.cmake)

option(SC_QRENCODE_BUILD_TESTS "Build simply-cpp-qrencode tests" ${PROJECT_IS_TOP_LEVEL})

get_sc_version()
message(STATUS "SimplyCPP ${SC_MODULE} Version: ${SC_VERSION}")

# sc::distance for an SVG's size; sc::params' man pages (1.24.0) for qrencode; zlib for the PNG's pixels.
find_or_install_package(sc-core simply-cpp-dev simply-cpp 1.24.0)
find_or_install_package(ZLIB zlib1g-dev zlib)

set(SOURCE_OBJECTS)
set(SOURCE_LIBRARIES)
set(SOURCE_LINK_LIBRARIES)

# Project Nayuki's QR Code generator (MIT, third_party/qrcodegen, from commit 3c6d0b3cefb4e049dc337e82237c9644399716a8), compiled in.
# Its namespace becomes sc_qrcodegen, so a program with its own copy doesn't clash with ours.
add_sc_object(sc_qrencode
        SOURCES third_party/qrcodegen/qrcodegen.cpp third_party/qrcodegen/qrcodegen.hpp
        INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/third_party/qrcodegen
        LINK_LIBRARIES sc::sc-core ZLIB::ZLIB
        PUBLIC_LINK_LIBRARIES sc::sc-core ZLIB::ZLIB)
target_compile_definitions(sc-obj-sc_qrencode PRIVATE qrcodegen=sc_qrcodegen)

add_sc_libraries(DESCRIPTION "simply-cpp QR codes from text or bytes, as SVG or PNG, with no image library")
target_compile_features(sc-qrencode PUBLIC cxx_std_20)
target_compile_features(sc-qrencode-shared PUBLIC cxx_std_20)

install_sc_module(PATH_VARS CMAKE_INSTALL_INCLUDEDIR)
install(FILES cmake/SimplyCppFunctions.cmake cmake/sc_bootstrap.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/sc-qrencode
        COMPONENT development)

if (SC_QRENCODE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests sc-qrencode-test)
endif ()

add_sc_example(sc-qrencode-demo)

# qrencode, the program: libqrencode's qrencode command line, for what sc-qrencode can do.
add_executable(qrencode apps/qrencode.cpp)
target_link_libraries(qrencode PRIVATE sc::sc-qrencode-shared)
target_compile_definitions(qrencode PRIVATE SC_QRENCODE_VERSION="${SC_VERSION}")
sc_program_info(qrencode)
install(TARGETS qrencode RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT runtime)
sc_man_page(qrencode EXTRA ${CMAKE_CURRENT_SOURCE_DIR}/man/qrencode.md COMPONENT runtime)

package_sc_module(DEV_DEPENDS "simply-cpp-dev (>= 1.24.0~)" zlib1g-dev)
