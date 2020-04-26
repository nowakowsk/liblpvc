cmake_minimum_required(VERSION 3.14)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(CTest)

find_package(zstd REQUIRED)


###############################################################################
# liblpvc

set(PROJECT_NAME liblpvc)
set(PROJECT_VERSION 0.1.0)

project(${PROJECT_NAME} VERSION ${PROJECT_VERSION})

configure_file("cmake/version.cpp.in" "${CMAKE_CURRENT_BINARY_DIR}/version.cpp" @ONLY)

set(PROJECT_INCLUDES
  "include/lpvc/detail/lpvc_impl.h"
  "include/lpvc/detail/serialization.h"
  "include/lpvc/detail/variant_utils.h"
  "include/lpvc/detail/zstd_wrapper.h"
  "include/lpvc/lpvc.h"
)

add_library(${PROJECT_NAME}
  ${PROJECT_INCLUDES}
  "${CMAKE_CURRENT_BINARY_DIR}/version.cpp"
  "src/lpvc.cpp"
)

target_include_directories(${PROJECT_NAME}
  PUBLIC
    "$<INSTALL_INTERFACE:include>"
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
)

target_link_libraries(${PROJECT_NAME}
  PUBLIC
    zstd::libzstd_static
)

set_target_properties(${PROJECT_NAME}
  PROPERTIES
    CXX_STANDARD 17
)


###############################################################################
# Warning configuration

if(MSVC)
  target_compile_definitions(${PROJECT_NAME}
    PRIVATE
      _CRT_SECURE_NO_WARNINGS
  )
endif()


###############################################################################
# Tests

if(BUILD_TESTING)
  add_subdirectory(test)
endif()


###############################################################################
# Installation

set(PACKAGE_CONFIG_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}")
set(PACKAGE_TARGETS_NAME "${PROJECT_NAME}-targets")

install(
  TARGETS ${PROJECT_NAME}
  EXPORT ${PACKAGE_TARGETS_NAME}
  ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
  INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)

install(
  EXPORT ${PACKAGE_TARGETS_NAME}
  FILE "${PROJECT_NAME}-targets.cmake"
  NAMESPACE ${PROJECT_NAME}::
  DESTINATION "${PACKAGE_CONFIG_INSTALL_DIR}"
)

install(
  DIRECTORY 
    "include/"
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)

write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config-version.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
  "${CMAKE_CURRENT_LIST_DIR}/cmake/config.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config.cmake"
  INSTALL_DESTINATION "${PACKAGE_CONFIG_INSTALL_DIR}"
)

install(
  FILES
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config-version.cmake"
    
  DESTINATION
    "${PACKAGE_CONFIG_INSTALL_DIR}"
)
