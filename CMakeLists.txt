cmake_minimum_required(VERSION 3.8.2)

project(flumps VERSION 0.0.1)

option(FLUMPS_BUILD_TESTS "Build flumps tests with the project." OFF)
option(FLUMPS_INSTALL "Enable installation of flumps. Projects embedding flumps may want to turn this OFF." ON)
set(FLUMPS_BUILD_NUMBER 1 CACHE STRING "Build number for generated RPMs.")

set(flumps_package_name "libflumps")
set(flumps_description "A fast, zero-copy, validating JSON parser.")
set(flumps_url "https://github.com/chrisvroberts/flumps")

# TODO: add static analysis checks

include(GNUInstallDirs)

configure_file(
  include/flumps/version.h.in
  include/flumps/version.h
)

set(flumps_headers
  include/flumps/exception.h
  include/flumps/json_callback_visitor.h
  include/flumps/json_echo_visitor.h
  include/flumps/json_parser.h
  include/flumps/json_value_path.h
  include/flumps/json_value_type.h
  include/flumps/string_helpers.h
  include/flumps/detail/json_echo_visitor_impl.h
  include/flumps/detail/json_callback_visitor_impl.h
  include/flumps/detail/json_parser_impl.h
  "${CMAKE_CURRENT_BINARY_DIR}/include/flumps/version.h"
)

set(flumps_sources
  src/flumps/exception.cpp
  src/flumps/json_callback_visitor.cpp
  src/flumps/json_value_path.cpp
  src/flumps/json_value_type.cpp
  src/flumps/string_helpers.cpp
)

add_library(flumps_objects OBJECT
  ${flumps_headers}
  ${flumps_sources}
)

target_include_directories(flumps_objects
  PUBLIC
    $<INSTALL_INTERFACE:include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
)

# Note: This limits us to gcc type compilers.
target_compile_options(flumps_objects PUBLIC
  $<BUILD_INTERFACE:-Wall -Wextra -Wpedantic -Werror>
)

set_target_properties(flumps_objects
  PROPERTIES
    POSITION_INDEPENDENT_CODE 1
)

target_compile_features(flumps_objects
  PRIVATE
    cxx_std_14
)

add_library(flumps_shared SHARED
  $<TARGET_OBJECTS:flumps_objects>
)

# Include directories are not transitive from object libraries so need to add
# the includes again here to avoid having to export flumps_objects.
target_include_directories(flumps_shared
  PUBLIC
    $<INSTALL_INTERFACE:include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
)

set_target_properties(flumps_shared
  PROPERTIES
    OUTPUT_NAME "flumps"
    VERSION ${flumps_VERSION}
    SOVERSION ${flumps_VERSION_MAJOR}
)

if(FLUMPS_INSTALL)
  install(
    TARGETS flumps_shared
    DESTINATION ${CMAKE_INSTALL_LIBDIR}
    EXPORT flumps_targets
    COMPONENT LIBFLUMPS
  )
endif()

add_library(flumps_static STATIC
  $<TARGET_OBJECTS:flumps_objects>
)

set_target_properties(flumps_static
  PROPERTIES
    OUTPUT_NAME "flumps"
)

# Include directories are not transitive from object libraries so need to add
# the includes again here to avoid having to export flumps_objects.
target_include_directories(flumps_static
  PUBLIC
    $<INSTALL_INTERFACE:include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
)

if(FLUMPS_INSTALL)
  install(
   TARGETS flumps_static
   DESTINATION ${CMAKE_INSTALL_LIBDIR}
   EXPORT flumps_targets
   COMPONENT LIBFLUMPS_DEVEL
  )

  install(
    FILES
      ${flumps_headers}
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/flumps"
    COMPONENT LIBFLUMPS_DEVEL
  )
endif()

add_executable(flumps
  src/flumps/main.cpp
)

target_link_libraries(flumps
  PRIVATE
    flumps_static
)

# Note: This limits us to gcc type compilers.
target_compile_options(flumps PUBLIC
  $<BUILD_INTERFACE:-Wall -Wextra -Wpedantic -Werror>
)

target_compile_features(flumps
  PRIVATE
    cxx_std_14
)

if(FLUMPS_INSTALL)
  install(
   TARGETS flumps
   DESTINATION ${CMAKE_INSTALL_BINDIR}
   COMPONENT FLUMPS
  )

  install(
    EXPORT flumps_targets
    FILE FlumpsTargets.cmake
    DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/cmake/flumps"
    COMPONENT LIBFLUMPS_DEVEL
  )
endif()

include(CMakePackageConfigHelpers)

configure_package_config_file(Config.cmake.in
  "${CMAKE_CURRENT_BINARY_DIR}/FlumpsConfig.cmake"
  INSTALL_DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/cmake/flumps"
  NO_SET_AND_CHECK_MACRO
  NO_CHECK_REQUIRED_COMPONENTS_MACRO
)

write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/FlumpsConfigVersion.cmake"
  VERSION "${flumps_VERSION_MAJOR}.${flumps_VERSION_MINOR}"
  COMPATIBILITY AnyNewerVersion
)

if(FLUMPS_INSTALL)
  install(
    FILES
      "${CMAKE_CURRENT_BINARY_DIR}/FlumpsConfig.cmake"
      "${CMAKE_CURRENT_BINARY_DIR}/FlumpsConfigVersion.cmake"
    DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/cmake/flumps"
    COMPONENT LIBFLUMPS_DEVEL
  )

  export(EXPORT flumps_targets
    FILE "${CMAKE_CURRENT_BINARY_DIR}/FlumpsTargets.cmake"
  )
endif()

if(FLUMPS_BUILD_TESTS)
  set(
    flumps_test_config_input_directory
    "${CMAKE_CURRENT_SOURCE_DIR}/test/flumps/test_inputs"
  )
  configure_file(test/flumps/test_config.h.in test/flumps/test_config.h)

  option(INSTALL_GTEST OFF)
  find_package(GTest)
  if(GTEST_FOUND)
    set(flumps_gtest_include_dirs
      ${GTEST_INCLUDE_DIRS}
    )
  else()
    add_subdirectory(extern/googletest EXCLUDE_FROM_ALL)
    set(flumps_gtest_include_dirs
      "${gtest_SOURCE_DIR}/include"
      "${gtest_SOURCE_DIR}"
    )
  endif()

  enable_testing()

  function(flumps_test test_name)
    set(test_executable run_${test_name})
    set(test_source test/flumps/${test_name}.cpp)

    add_executable(${test_executable} ${test_source})

    target_include_directories(${test_executable}
      PRIVATE
        "${CMAKE_CURRENT_BINARY_DIR}/test"
        ${flumps_gtest_include_dirs}
    )

    target_link_libraries(${test_executable}
      PRIVATE
        stdc++fs gtest gmock gtest_main flumps_static
    )

    add_test(${test_name} ${test_executable})
  endfunction()

  flumps_test(json_callback_visitor_test)
  flumps_test(json_parser_test)
  flumps_test(string_helpers_test)
  flumps_test(json_value_path_test)
endif()

# Configure the pkg-config metadata. No dependent libs now but if added will
# need to be fed into here.
configure_file(
  libflumps.pc.in
  "${flumps_package_name}.pc"
  @ONLY
)

if(FLUMPS_INSTALL)
  # Install pkg-config metadata into standard location within the prefix...
  install(
    FILES
      "${CMAKE_CURRENT_BINARY_DIR}/${flumps_package_name}.pc"
    DESTINATION
      "${CMAKE_INSTALL_LIBDIR}/pkgconfig/"
    COMPONENT LIBFLUMPS_DEVEL
  )

  install(
    FILES
      LICENSE
    DESTINATION
      "${CMAKE_INSTALL_DATAROOTDIR}/licenses/${CMAKE_PROJECT_NAME}-${flumps_VERSION}"
    COMPONENT LIBFLUMPS
  )
endif()

set(CPACK_PACKAGE_VERSION_MAJOR ${flumps_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${flumps_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${flumps_VERSION_PATCH})
set(CPACK_RPM_PACKAGE_RELEASE ${FLUMPS_BUILD_NUMBER})
set(CPACK_RPM_COMPONENT_INSTALL ON)
set(CPACK_RPM_FLUMPS_PACKAGE_NAME ${CMAKE_PROJECT_NAME})
set(CPACK_RPM_LIBFLUMPS_PACKAGE_NAME ${flumps_package_name})
set(CPACK_RPM_LIBFLUMPS_DEVEL_PACKAGE_NAME "${flumps_package_name}-devel")
set(FLUMPS_RPM_SUFFIX
  "${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}-${CPACK_RPM_PACKAGE_RELEASE}.${CMAKE_HOST_SYSTEM_PROCESSOR}.rpm")
set(CPACK_RPM_FLUMPS_FILE_NAME
  "${CPACK_RPM_FLUMPS_PACKAGE_NAME}-${FLUMPS_RPM_SUFFIX}")
set(CPACK_RPM_LIBFLUMPS_FILE_NAME
  "${CPACK_RPM_LIBFLUMPS_PACKAGE_NAME}-${FLUMPS_RPM_SUFFIX}")
set(CPACK_RPM_LIBFLUMPS_DEVEL_FILE_NAME
  "${CPACK_RPM_LIBFLUMPS_DEVEL_PACKAGE_NAME}-${FLUMPS_RPM_SUFFIX}")
set(CPACK_SOURCE_GENERATOR "TBZ2")
set(CPACK_SOURCE_IGNORE_FILES
  "/build/;/.git*;~$;${CPACK_SOURCE_IGNORE_FILES}")
set(CPACK_PACKAGE_VENDOR "(none)")
set(CPACK_RPM_FLUMPS_PACKAGE_GROUP "Development/Tools")
set(CPACK_RPM_PACKAGE_GROUP "Development/Libraries")
set(CPACK_RPM_PACKAGE_LICENSE "GPLv3+")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_RPM_PACKAGE_URL ${flumps_url})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY ${flumps_description})
set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README.txt")
set(CPACK_GENERATOR "RPM")
set(CPACK_RPM_FLUMPS_PACKAGE_REQUIRES "")
set(CPACK_RPM_LIBFLUMPS_PACKAGE_REQUIRES "")
set(CPACK_RPM_LIBFLUMPS_DEVEL_PACKAGE_REQUIRES
  "${flumps_package_name} >= ${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}")
set(CPACK_RPM_FLUMPS_BUILDREQUIRES "")
set(CPACK_RPM_LIBFLUMPS_BUILDREQUIRES "")
set(CPACK_RPM_LIBFLUMPS_DEVEL_BUILDREQUIRES "")

include(CPack)
