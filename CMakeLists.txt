cmake_minimum_required(VERSION 3.24)

include(cmake/prelude.cmake)

project(
    plotz
    VERSION 0.1.0
    LANGUAGES CXX
)

include(cmake/project-is-top-level.cmake)
include(cmake/variables.cmake)

add_library(${PROJECT_NAME}_${PROJECT_NAME} OBJECT)
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME}_${PROJECT_NAME})

file(GLOB_RECURSE headers "${PROJECT_SOURCE_DIR}/include/*.hpp")
file(GLOB_RECURSE sources "${PROJECT_SOURCE_DIR}/src/*.cpp")

message(STATUS "Sources: ${sources}")

target_sources(${PROJECT_NAME}_${PROJECT_NAME} 
    PRIVATE 
        ${sources}
    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS "${PROJECT_SOURCE_DIR}/include"
        FILES ${headers}
)

if (MSVC)
   string(REGEX MATCH "\/cl(.exe)?$" matched_cl ${CMAKE_CXX_COMPILER})
   if (matched_cl)
      # for a C++ standards compliant preprocessor, not needed for clang-cl
      target_compile_options(${PROJECT_NAME}_${PROJECT_NAME} PRIVATE "/Zc:preprocessor" /GL /permissive- /Zc:lambda)
      target_link_options(${PROJECT_NAME}_${PROJECT_NAME} PRIVATE /LTCG /INCREMENTAL:NO)
   endif()
else()
   target_compile_options(${PROJECT_NAME}_${PROJECT_NAME} PRIVATE "-Wno-missing-braces")
endif()

set_property(TARGET ${PROJECT_NAME}_${PROJECT_NAME} PROPERTY EXPORT_NAME ${PROJECT_NAME})

target_compile_features(${PROJECT_NAME}_${PROJECT_NAME} PUBLIC cxx_std_20)
target_include_directories(
    ${PROJECT_NAME}_${PROJECT_NAME} ${warning_guard}
    PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
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

target_compile_features(${PROJECT_NAME}_${PROJECT_NAME} PUBLIC cxx_std_23)
target_include_directories(${PROJECT_NAME}_${PROJECT_NAME} PUBLIC
    ${PNG_INCLUDE_DIRS}
    ${FREETYPE_INCLUDE_DIRS}
)

# Link libraries to the target
target_link_libraries(${PROJECT_NAME}_${PROJECT_NAME} PUBLIC
    ${PNG_LIBRARIES}
    ${FREETYPE_LIBRARIES}
)

if (PROJECT_IS_TOP_LEVEL)
  include(cmake/dev-mode.cmake)
endif()
