cmake_minimum_required(VERSION 3.14)  # Updated to 3.14 for FetchContent improvements
project(plot2d)

set(CMAKE_CXX_STANDARD 20)

file(GLOB srcs src/*.cpp include/plotz/*.hpp)

# Organize files into directories
foreach(FILE ${srcs})
   # Get the directory of the source file
   get_filename_component(PARENT_DIR "${FILE}" PATH)

   # Remove common directory prefix to make the group
   string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}" "" GROUP "${PARENT_DIR}")

if(MSVC)
   # Make sure we are using windows slashes
   string(REPLACE "/" "\\" GROUP "${GROUP}")
endif()

   # Group into "Source Files" and "Header Files"
   if ("${FILE}" MATCHES ".*\\.cpp")
      set(GROUP "Source Files${GROUP}")
   elseif("${FILE}" MATCHES ".*\\.h")
      set(GROUP "Header Files${GROUP}")
   endif()

   source_group("${GROUP}" FILES "${FILE}")
endforeach()

# Include directories
include_directories(include)

# Include FetchContent module
include(FetchContent)

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

# Add the executable target
add_executable(${PROJECT_NAME} ${srcs})

target_compile_definitions(${PROJECT_NAME} PRIVATE 
    FONTS_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}/fonts\"
)

# Include directories for the target
target_include_directories(${PROJECT_NAME} PUBLIC
    ${PNG_INCLUDE_DIRS}
    ${glaze_SOURCE_DIR}/include
    include
)

# Link libraries to the target
target_link_libraries(${PROJECT_NAME} PUBLIC
    ${PNG_LIBRARIES}
)

# If using any definitions or properties, set them here
# For example, if you need to define PNG_DEBUG
# target_compile_definitions(${PROJECT_NAME} PRIVATE PNG_DEBUG)

# Include directories for FreeType
target_include_directories(${PROJECT_NAME} PUBLIC
    ${FREETYPE_INCLUDE_DIRS}
)

# Link FreeType library
target_link_libraries(${PROJECT_NAME} PUBLIC
    ${FREETYPE_LIBRARIES}
)
