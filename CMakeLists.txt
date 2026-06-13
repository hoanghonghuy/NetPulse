# ============================================================================
# File: CMakeLists.txt
# Description: CMake build configuration for NetPulse
# Author: NetPulse Project
# ============================================================================

cmake_minimum_required(VERSION 3.15)

# ============================================================================
# PROJECT CONFIGURATION
# ============================================================================

project(NetPulse
    VERSION 2.4.2
    DESCRIPTION "Network Traffic Monitor for Windows System Tray"
    LANGUAGES C CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(${CMAKE_SOURCE_DIR}/cmake/NetPulseDefaults.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/NetPulseResources.cmake)
netpulse_setup_resource_script()

# ============================================================================
# BUILD OPTIONS
# ============================================================================

option(BUILD_TESTS "Build unit tests" OFF)

# ============================================================================
# SOURCE FILES
# ============================================================================

set(HEADER_FILES
    include/NetPulse/Common.h
    include/NetPulse/Utils.h
    include/NetPulse/NetworkCalculator.h
    include/NetPulse/NetworkMonitor.h
    include/NetPulse/ConfigManager.h
    include/NetPulse/TrayIcon.h
    include/NetPulse/TaskbarOverlay.h
    include/NetPulse/ThemeHelper.h
    include/NetPulse/DialogThemeHelper.h
    include/NetPulse/PingMonitor.h
    include/NetPulse/HistoryLogger.h
    include/NetPulse/HotkeyManager.h
    include/NetPulse/MenuHandler.h
    include/NetPulse/UpdateCoordinator.h
    include/NetPulse/UpdateChecker.h
    include/NetPulse/DialogManager.h
    include/NetPulse/LanguageManager.h
    include/NetPulse/DataUsageMonitor.h
    include/NetPulse/FloatingWindow.h
    include/NetPulse/SystemMonitor.h
    include/NetPulse/Application.h
    include/NetPulse/ApplicationRuntime.h
    include/NetPulse/SettingsDialog.h
    include/NetPulse/DashboardDialog.h
    include/NetPulse/HistoryDialog.h
    include/NetPulse/PerAppMonitor.h
    include/NetPulse/PerAppDialog.h
    include/NetPulse/EtwNetworkMonitor.h
    include/NetPulse/SparklineRenderer.h
    include/NetPulse/ChartRenderer.h
    include/NetPulse/VpnProxyDetector.h
    include/NetPulse/SpeedTester.h
    include/NetPulse/SpeedTestHistory.h
    include/NetPulse/Interfaces/IVpnProxyProvider.h
    include/NetPulse/Interfaces/ISpeedTestProvider.h
    include/NetPulse/Interfaces/IConfigProvider.h
    include/NetPulse/Interfaces/INetworkStatsProvider.h
    include/NetPulse/Interfaces/IPingProvider.h
    include/NetPulse/Interfaces/IHttpClient.h
    include/NetPulse/WinHttpClient.h
    include/NetPulse/ConnectionMonitor.h
    include/NetPulse/ConnectionLogDialog.h
    include/NetPulse/SpeedTestDialog.h
    include/NetPulse/ThemeColors.h
)

set(SOURCE_FILES
    src/entry/main.cpp
    src/core/Application.cpp
    src/core/ApplicationRuntime.cpp
    src/ui/dialogs/SettingsDialog.cpp
    src/ui/dialogs/DashboardDialog.cpp
    src/ui/dialogs/HistoryDialog.cpp
    src/ui/dialogs/SpeedTestDialog.cpp
    src/ui/dialogs/ConnectionLogDialog.cpp
    src/core/Utils.cpp
    src/core/NetworkCalculator.cpp
    src/core/NetworkMonitor.cpp
    src/core/ConfigManager.cpp
    src/core/PingMonitor.cpp
    src/core/HotkeyManager.cpp
    src/core/MenuHandler.cpp
    src/core/UpdateCoordinator.cpp
    src/core/UpdateChecker.cpp
    src/core/WinHttpClient.cpp
    src/core/LanguageManager.cpp
    src/core/DataUsageMonitor.cpp
    src/core/SystemMonitor.cpp
    src/ui/TrayIcon.cpp
    src/ui/TaskbarOverlay.cpp
    src/ui/FloatingWindow.cpp
    src/ui/ThemeHelper.cpp
    src/ui/DialogThemeHelper.cpp
    src/ui/DialogManager.cpp
    src/core/HistoryLogger.cpp
    src/core/PerAppMonitor.cpp
    src/ui/dialogs/PerAppDialog.cpp
    src/core/EtwNetworkMonitor.cpp
    src/ui/SparklineRenderer.cpp
    src/ui/ChartRenderer.cpp
    src/core/VpnProxyDetector.cpp
    src/core/SpeedTester.cpp
    src/core/SpeedTestHistory.cpp
    src/core/ConnectionMonitor.cpp
    third_party/sqlite/sqlite3.c
)

set(RESOURCE_FILES
    ${NETPULSE_APP_RC}
    resources/resource.h
    resources/app.manifest
)

# ============================================================================
# EXECUTABLE TARGET
# ============================================================================

add_executable(${PROJECT_NAME} WIN32
    ${HEADER_FILES}
    ${SOURCE_FILES}
    ${RESOURCE_FILES}
)

target_include_directories(${PROJECT_NAME} PRIVATE ${NETPULSE_INCLUDE_DIRECTORIES})
netpulse_apply_common_defaults(${PROJECT_NAME})
netpulse_add_resource_dependencies(${PROJECT_NAME})

if(MSVC)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        LINK_FLAGS "/SUBSYSTEM:WINDOWS"
        RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/Debug"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/Release"
    )
endif()

# ============================================================================
# SOURCE GROUPS (for IDE organization)
# ============================================================================

source_group("Header Files" FILES ${HEADER_FILES})
source_group("Source Files" FILES ${SOURCE_FILES})
source_group("Resource Files" FILES ${RESOURCE_FILES})

# ============================================================================
# POST-BUILD COMMANDS
# ============================================================================

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/resources/icons"
    COMMENT "Creating resources directory"
)

# ============================================================================
# INSTALLATION
# ============================================================================

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)

install(DIRECTORY resources/icons/
    DESTINATION bin/resources/icons
    FILES_MATCHING PATTERN "*.ico"
)

# ============================================================================
# UNIT TESTS (Optional)
# ============================================================================

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# ============================================================================
# PRINT CONFIGURATION SUMMARY
# ============================================================================

message(STATUS "")
message(STATUS "=== NetPulse Build Configuration ===")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "Install prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "Build tests: ${BUILD_TESTS}")
if(BUILD_TESTS)
    message(STATUS "Test coverage instrumentation: ${ENABLE_COVERAGE}")
endif()
message(STATUS "Defaults: cmake/NetPulseDefaults.cmake")
message(STATUS "==========================================")
message(STATUS "")
