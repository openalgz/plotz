cmake_minimum_required(VERSION 3.24)

include(cmake/prelude.cmake)

project(
    plotz
    VERSION 0.0.1
    LANGUAGES CXX
)

include(cmake/project-is-top-level.cmake)
include(cmake/variables.cmake)

add_library(${PROJECT_NAME}_${PROJECT_NAME} INTERFACE)
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME}_${PROJECT_NAME})

if (MSVC)
   string(REGEX MATCH "\/cl(.exe)?$" matched_cl ${CMAKE_CXX_COMPILER})
   if (matched_cl)
      # for a C++ standards compliant preprocessor, not needed for clang-cl
      target_compile_options(${PROJECT_NAME}_${PROJECT_NAME} INTERFACE "/Zc:preprocessor" /GL /permissive- /Zc:lambda)
      target_link_options(${PROJECT_NAME}_${PROJECT_NAME} INTERFACE /LTCG /INCREMENTAL:NO)
   endif()
else()
   target_compile_options(${PROJECT_NAME}_${PROJECT_NAME} INTERFACE "-Wno-missing-braces")
endif()

set_property(TARGET ${PROJECT_NAME}_${PROJECT_NAME} PROPERTY EXPORT_NAME ${PROJECT_NAME})

target_compile_features(${PROJECT_NAME}_${PROJECT_NAME} INTERFACE cxx_std_20)
target_include_directories(
    ${PROJECT_NAME}_${PROJECT_NAME} ${warning_guard}
    INTERFACE "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
)

# Find libpng
find_package(PNG REQUIRED)

if(PNG_FOUND)
    message(STATUS "libpng found: ${PNG_INCLUDE_DIRS}")
else()
    message(FATAL_ERROR "libpng not found!")
endif()

find_package(Freetype REQUIRED)

if(Freetype_FOUND)
    message(STATUS "FreeType found: ${FREETYPE_INCLUDE_DIRS}")
else()
    message(FATAL_ERROR "FreeType not found!")
endif()

if(NOT CMAKE_SKIP_INSTALL_RULES)
  include(cmake/install-rules.cmake)
endif()

target_include_directories(${PROJECT_NAME}_${PROJECT_NAME} INTERFACE
    ${PNG_INCLUDE_DIRS}
    ${FREETYPE_INCLUDE_DIRS}
)

# Link libraries to the target
target_link_libraries(${PROJECT_NAME}_${PROJECT_NAME} INTERFACE
    ${PNG_LIBRARIES}
    ${FREETYPE_LIBRARIES}
)

if (PROJECT_IS_TOP_LEVEL)
  include(cmake/dev-mode.cmake)
endif()
