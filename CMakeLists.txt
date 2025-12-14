# ============================================================================
# File: CMakeLists.txt
# Description: CMake build configuration for NetworkMonitor
# Author: NetworkMonitor Project
# ============================================================================

cmake_minimum_required(VERSION 3.15)

# ============================================================================
# PROJECT CONFIGURATION
# ============================================================================

project(NetworkMonitor
    VERSION 1.5.0
    DESCRIPTION "Network Traffic Monitor for Windows System Tray"
    LANGUAGES C CXX
)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ============================================================================
# BUILD OPTIONS
# ============================================================================

option(BUILD_TESTS "Build unit tests" OFF)

# ============================================================================
# COMPILER FLAGS
# ============================================================================

if(MSVC)
    # Warning level 4 and treat warnings as errors
    add_compile_options(/W4 /WX)
    
    # Enable multiprocessor compilation
    add_compile_options(/MP)
    
    # Unicode support
    add_definitions(-DUNICODE -D_UNICODE)
    
    # Windows version targeting (Windows 7+)
    add_definitions(-D_WIN32_WINNT=0x0601)
    
    # Disable specific warnings
    add_compile_options(/wd4100) # unreferenced formal parameter
    
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# ============================================================================
# SOURCE FILES
# ============================================================================

# Header files
set(HEADER_FILES
    include/NetworkMonitor/Common.h
    include/NetworkMonitor/Utils.h
    include/NetworkMonitor/NetworkCalculator.h
    include/NetworkMonitor/NetworkMonitor.h
    include/NetworkMonitor/ConfigManager.h
    include/NetworkMonitor/TrayIcon.h
    include/NetworkMonitor/TaskbarOverlay.h
    include/NetworkMonitor/ThemeHelper.h
    include/NetworkMonitor/DialogThemeHelper.h
    include/NetworkMonitor/PingMonitor.h
    include/NetworkMonitor/HistoryLogger.h
    include/NetworkMonitor/HotkeyManager.h
    include/NetworkMonitor/MenuHandler.h
    include/NetworkMonitor/UpdateCoordinator.h
    include/NetworkMonitor/DialogManager.h
    include/NetworkMonitor/LanguageManager.h
    include/NetworkMonitor/DataUsageMonitor.h
    include/NetworkMonitor/FloatingWindow.h
    include/NetworkMonitor/SystemMonitor.h
    include/NetworkMonitor/Application.h
    include/NetworkMonitor/SettingsDialog.h
    include/NetworkMonitor/DashboardDialog.h
    include/NetworkMonitor/HistoryDialog.h
    include/NetworkMonitor/PerAppMonitor.h
    include/NetworkMonitor/PerAppDialog.h
    include/NetworkMonitor/EtwNetworkMonitor.h
    include/NetworkMonitor/SparklineRenderer.h
    include/NetworkMonitor/VpnProxyDetector.h
    include/NetworkMonitor/SpeedTester.h
    include/NetworkMonitor/SpeedTestHistory.h
    include/NetworkMonitor/Interfaces/IVpnProxyProvider.h
    include/NetworkMonitor/Interfaces/ISpeedTestProvider.h
)

# Source files
set(SOURCE_FILES
    src/entry/main.cpp
    src/core/Application.cpp
    src/ui/dialogs/SettingsDialog.cpp
    src/ui/dialogs/DashboardDialog.cpp
    src/ui/dialogs/HistoryDialog.cpp
    src/ui/dialogs/SpeedTestDialog.cpp
    src/core/Utils.cpp
    src/core/NetworkCalculator.cpp
    src/core/NetworkMonitor.cpp
    src/core/ConfigManager.cpp
    src/core/PingMonitor.cpp
    src/core/HotkeyManager.cpp
    src/core/MenuHandler.cpp
    src/core/UpdateCoordinator.cpp
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
    src/core/VpnProxyDetector.cpp
    src/core/SpeedTester.cpp
    src/core/SpeedTestHistory.cpp
    third_party/sqlite/sqlite3.c
)

if(MSVC)
    # Suppress deprecated GetVersionEx warnings in third-party SQLite only
    set_source_files_properties(third_party/sqlite/sqlite3.c PROPERTIES
        COMPILE_FLAGS "/wd4996"
    )
endif()

# Resource files
set(RESOURCE_FILES
    resources/app.rc
    resources/resource.h
    resources/app.manifest
)

# ============================================================================
# EXECUTABLE TARGET
# ============================================================================

# Create WIN32 executable (no console window)
add_executable(${PROJECT_NAME} WIN32
    ${HEADER_FILES}
    ${SOURCE_FILES}
    ${RESOURCE_FILES}
)

# ============================================================================
# INCLUDE DIRECTORIES
# ============================================================================

target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/resources
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/sqlite
)

# ============================================================================
# LINK LIBRARIES
# ============================================================================

# Windows system libraries
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        Iphlpapi.lib    # Network interface API
        shell32.lib     # System tray API
        advapi32.lib    # Registry API
        comctl32.lib    # Common Controls
        Dwmapi.lib      # Desktop Window Manager API (for taskbar overlay)
        d3d11.lib       # Direct3D 11
        dxgi.lib        # DirectX Graphics Infrastructure
        winhttp.lib     # HTTP API (for public IP fetching)
)

# ============================================================================
# SOURCE GROUPS (for IDE organization)
# ============================================================================

source_group("Header Files" FILES ${HEADER_FILES})
source_group("Source Files" FILES ${SOURCE_FILES})
source_group("Resource Files" FILES ${RESOURCE_FILES})

# ============================================================================
# COMPILER-SPECIFIC SETTINGS
# ============================================================================

if(MSVC)
    # Set subsystem to Windows (GUI application)
    # Rely on the default CRT entry point that matches our WinMain signature.
    set_target_properties(${PROJECT_NAME} PROPERTIES
        LINK_FLAGS "/SUBSYSTEM:WINDOWS"
    )
    
    # Set output directories
    set_target_properties(${PROJECT_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/Debug"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/Release"
    )
endif()

# ============================================================================
# POST-BUILD COMMANDS
# ============================================================================

# Copy icon files to output directory (if needed for development)
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

# Install icon files
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
# IMGUI STANDALONE APP (Optional - for testing modern UI)
# ============================================================================

# option(BUILD_IMGUI_APP "Build standalone ImGui application for testing" OFF)
# Legacy ImGui integration temporarily disabled in build system


# ============================================================================
# PRINT CONFIGURATION SUMMARY
# ============================================================================

message(STATUS "")
message(STATUS "=== NetworkMonitor Build Configuration ===")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "Install prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "Build tests: ${BUILD_TESTS}")
message(STATUS "==========================================")
message(STATUS "")
