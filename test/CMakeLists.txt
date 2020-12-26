cmake_minimum_required(VERSION 3.14)

find_package(Catch2 REQUIRED)


###############################################################################
# liblpvc-test

set(PROJECT_NAME liblpvc-test)

project(${PROJECT_NAME})

add_executable(${PROJECT_NAME}
  "test.cpp"
)

target_link_libraries(${PROJECT_NAME}
  liblpvc::liblpvc
  Catch2::Catch2
)

set_target_properties(${PROJECT_NAME}
  PROPERTIES
    CXX_STANDARD 17
)


###############################################################################
# CTest

include(CTest)
include(Catch)

catch_discover_tests(${PROJECT_NAME})


###############################################################################
# Warning configuration

if(MSVC)
  target_compile_definitions(${PROJECT_NAME}
    PRIVATE
      _CRT_SECURE_NO_WARNINGS
  )
endif()
