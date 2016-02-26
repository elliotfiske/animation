cmake_minimum_required(VERSION 2.8)

# Name of the project
project(Assignment3)

# Use glob to get the list of all source files.
file(GLOB_RECURSE SOURCES "src/*.cpp")

# We don't really need to include header and resource files to build, but it's
# nice to have them show up in IDEs.
file(GLOB_RECURSE HEADERS "src/*.h")
file(GLOB_RECURSE GLSL "resources/*.glsl")

# Set the executable.
add_executable(${CMAKE_PROJECT_NAME} ${SOURCES} ${HEADERS} ${GLSL})

# Get the Eigen environment variable. Since Eigen is a header-only library, we
# just need to add it to the include directory.
set(EIGEN3_INCLUDE_DIR "$ENV{EIGEN3_INCLUDE_DIR}")
if(NOT EIGEN3_INCLUDE_DIR)
  MESSAGE(FATAL_ERROR "Please point the environment variable EIGEN3_INCLUDE_DIR to the include directory of your Eigen3 installation.")
endif()
include_directories(${EIGEN3_INCLUDE_DIR})

# Get the GLFW environment variable. There should be a CMakeLists.txt in the
# specified directory.
set(GLFW_DIR "$ENV{GLFW_DIR}")
if(NOT GLFW_DIR)
  message(FATAL_ERROR "Please point the environment variable GLFW_DIR to the root directory of your GLFW3 installation.")
endif()
option(GLFW_BUILD_EXAMPLES "GLFW_BUILD_EXAMPLES" OFF)
option(GLFW_BUILD_TESTS "GLFW_BUILD_TESTS" OFF)
option(GLFW_BUILD_DOCS "GLFW_BUILD_DOCS" OFF)
if(CMAKE_BUILD_TYPE MATCHES Release)
  add_subdirectory(${GLFW_DIR} ${GLFW_DIR}/release)
else()
  add_subdirectory(${GLFW_DIR} ${GLFW_DIR}/debug)
endif()
include_directories(${GLFW_DIR}/include)
target_link_libraries(${CMAKE_PROJECT_NAME} glfw ${GLFW_LIBRARIES})

# Get the GLEW environment variable.
set(GLEW_DIR "$ENV{GLEW_DIR}")
if(NOT GLEW_DIR)
  message(FATAL_ERROR "Please point the environment variable GLEW_DIR to the root directory of your GLEW installation.")
endif()
include_directories(${GLEW_DIR}/include)
if(WIN32)
  # With prebuilt binaries
  target_link_libraries(${CMAKE_PROJECT_NAME} ${GLEW_DIR}/lib/Release/Win32/glew32s.lib)
else()
  target_link_libraries(${CMAKE_PROJECT_NAME} ${GLEW_DIR}/lib/libGLEW.a)
endif()


# Get the CERES environment variable.
set(CERES_DIR "$ENV{CERES_DIR}")
if(NOT CERES_DIR)
  message(FATAL_ERROR "Please point the environment variable CERES_DIR to the root directory of your CERES installation.")
endif()
include_directories(${CERES_DIR}/include)
include_directories(${CERES_DIR}/build/config)
include_directories(${CERES_DIR}/internal/ceres/miniglog)
if(WIN32)
  # Add static option for MSVC
  option(MSVC_USE_STATIC_CRT
    "MS Visual Studio: Use static C-Run Time Library in place of shared." ON)
  # Update the C/CXX flags for MSVC to use either the static or shared
  # C-Run Time (CRT) library based on the user option: MSVC_USE_STATIC_CRT.
  list(APPEND C_CXX_FLAGS
    CMAKE_CXX_FLAGS
    CMAKE_CXX_FLAGS_DEBUG
    CMAKE_CXX_FLAGS_RELEASE
    CMAKE_CXX_FLAGS_MINSIZEREL
    CMAKE_CXX_FLAGS_RELWITHDEBINFO)
  foreach(FLAG_VAR ${C_CXX_FLAGS})
    if (MSVC_USE_STATIC_CRT)
      # Use static CRT.
      if (${FLAG_VAR} MATCHES "/MD")
        string(REGEX REPLACE "/MD" "/MT" ${FLAG_VAR} "${${FLAG_VAR}}")
      endif (${FLAG_VAR} MATCHES "/MD")
    else (MSVC_USE_STATIC_CRT)
      # Use shared, not static, CRT.
      if (${FLAG_VAR} MATCHES "/MT")
        string(REGEX REPLACE "/MT" "/MD" ${FLAG_VAR} "${${FLAG_VAR}}")
      endif (${FLAG_VAR} MATCHES "/MT")
    endif (MSVC_USE_STATIC_CRT)
  endforeach()
  # Separate .lib file for Release and Debug
  target_link_libraries(${CMAKE_PROJECT_NAME}
      debug ${CERES_DIR}/build/lib/Debug/ceres-debug.lib
      optimized ${CERES_DIR}/build/lib/Release/ceres.lib)
else()
  # Same .a file for Release and Debug
  target_link_libraries(${CMAKE_PROJECT_NAME} ${CERES_DIR}/build/lib/libceres.a)
endif()

# OS specific options and libraries
if(WIN32)
  # c++0x is enabled by default.
  # -Wall produces way too many warnings.
  # -pedantic is not supported.
else()
  # Enable all pedantic warnings.
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x -Wall -pedantic")
  if(APPLE)
    # Add required frameworks for GLFW.
    target_link_libraries(${CMAKE_PROJECT_NAME} "-framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo")
  else()
    #Link the Linux OpenGL library
    target_link_libraries(${CMAKE_PROJECT_NAME} "GL")
  endif()
endif()
