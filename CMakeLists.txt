cmake_minimum_required(VERSION 3.5)

project(OpenVR-FBTWalk)

# -----------------------------------------------------------------------------
# Check that the steamVR SDK is installed 
# (needed to prevent a segfault in OpenVR).
if(CMAKE_HOST_UNIX)
  find_file(OPENVRPATHS openvrpaths.vrpath PATHS $ENV{HOME}/.config/openvr "$ENV{HOME}/Library/Application Support/OpenVR/.openvr")
  if(${OPENVRPATHS} MATCHES OPENVRPATHS-NOTFOUND)
    message(FATAL_ERROR "${OPENVRPATHS} Please install SteamVR SDK to continue..")
  endif()
endif()

# -----------------------------------------------------------------------------
## SUBDIRECTORIES ##

add_subdirectory(OpenVR-FBTWalk)
