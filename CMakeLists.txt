cmake_minimum_required(VERSION 3.15)

# ============================================================================
# VitaSDK Toolchain (MUST be before project())
# ============================================================================

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VITASDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "vitasdk toolchain")
  else()
    message(FATAL_ERROR "VITASDK environment variable not set. Please set it to your vitasdk installation path.")
  endif()
endif()

project(VitaSpot C)

# Include vitasdk CMake support
include("${VITASDK}/share/vita.cmake" REQUIRED)

# ============================================================================
# Project Metadata
# ============================================================================

set(VITA_APP_NAME  "VitaSpot")
set(VITA_TITLEID   "VTSP00001")
set(VITA_VERSION   "01.00")
set(VITA_VERSION_MAJOR 0)
set(VITA_VERSION_MINOR 1)
set(VITA_VERSION_PATCH 0)

# ============================================================================
# Source Files
# ============================================================================

set(SOURCES
    src/main.c
    
    # Message Bus
    src/message_bus/bus.c
    
    # Agents
    src/agents/auth/auth_agent.c
    src/agents/api/api_agent.c
    src/agents/playback/playback_agent.c
    src/agents/ui/ui_agent.c
    src/agents/cache/cache_agent.c
    
    # Spotify
    src/spotify/spotify_auth.c
    src/spotify/spotify_api.c
    
    # Utils
    src/utils/logger.c
    src/utils/http.c
    src/utils/json.c
    src/utils/base64.c
    src/utils/sha256.c
)

# ============================================================================
# Create Executable
# ============================================================================

add_executable(${PROJECT_NAME} ${SOURCES})

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

# ============================================================================
# Required Libraries
# ============================================================================

target_link_libraries(${PROJECT_NAME}
    # Networking
    curl
    ssl
    crypto
    z
    
    # JSON parsing
    jansson
    
    # Image libraries
    png
    jpeg
    
    # Graphics & Input
    vita2d
    freetype
    
    # Vita System Libraries
    SceKernel_stub
    SceLibc_stub
    SceDisplay_stub
    SceGxm_stub
    SceSysmodule_stub
    SceCtrl_stub
    SceTouch_stub
    ScePower_stub
    SceRtc_stub
    SceIofilemgr_stub
    SceNet_stub
    SceNetCtl_stub
    SceSysmodule_stub
    SceHttp_stub
    SceAppUtil_stub
)

# ============================================================================
# Compiler Flags
# ============================================================================

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -Wall -Wextra -O2")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-builtin-printf")

# ============================================================================
# Create .self (ELF to SELFs conversion)
# ============================================================================

vita_create_self(${PROJECT_NAME}.self ${PROJECT_NAME})

# ============================================================================
# Create VPK (Package) - includes SFO automatically
# ============================================================================

vita_create_vpk(${PROJECT_NAME}.vpk ${VITA_TITLEID} ${PROJECT_NAME}.self
    VERSION "${VITA_VERSION}"
    NAME "${VITA_APP_NAME}"
)

# ============================================================================
# Build Output
# ============================================================================

message(STATUS "VitaSpot Build Configuration")
message(STATUS "  App Name: ${VITA_APP_NAME}")
message(STATUS "  Title ID: ${VITA_TITLEID}")
message(STATUS "  Version: ${VITA_VERSION}")
message(STATUS "  Target Output: ${PROJECT_NAME}.vpk")
