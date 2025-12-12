# CMake configuration for NetworkMonitor tests

cmake_minimum_required(VERSION 3.16)
project(NetworkMonitorTests LANGUAGES C CXX)

set(TEST_SOURCES
    main_tests.cpp
    TestUtils.cpp
    history_logger_tests.cpp
    network_monitor_tests.cpp
    utils_tests.cpp
    network_calculator_tests.cpp
    config_manager_tests.cpp
    ui_tests.cpp
    ../src/core/HistoryLogger.cpp
    ../src/core/NetworkMonitor.cpp
    ../src/core/NetworkCalculator.cpp
    ../src/core/ConfigManager.cpp
    ../src/core/PingMonitor.cpp
    ../src/core/Utils.cpp
    ../src/ui/TrayIcon.cpp
    ../src/ui/TaskbarOverlay.cpp
    ../src/ui/ThemeHelper.cpp
    ../src/ui/DialogThemeHelper.cpp
    ../src/core/MenuHandler.cpp
    ../src/core/UpdateCoordinator.cpp
    ../src/core/LanguageManager.cpp
    ../src/core/DataUsageMonitor.cpp
    ../src/core/SystemMonitor.cpp
    ../src/ui/DialogManager.cpp
    ../src/ui/FloatingWindow.cpp
    ../src/ui/dialogs/SettingsDialog.cpp
    ../src/ui/dialogs/DashboardDialog.cpp
    ../src/ui/dialogs/HistoryDialog.cpp
    ../src/ui/dialogs/PerAppDialog.cpp
    ../src/core/PerAppMonitor.cpp
    ../src/core/EtwNetworkMonitor.cpp
    ../src/ui/SparklineRenderer.cpp
    ../third_party/sqlite/sqlite3.c
)

add_executable(${PROJECT_NAME} ${TEST_SOURCES})

# Use the same C++ standard as the main project
set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
)

# Include main project headers and sqlite
target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
        ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/sqlite
)

# Link against the same Windows system libraries as the main app
if(MSVC)
    target_link_libraries(${PROJECT_NAME}
        PRIVATE
            Iphlpapi.lib
            shell32.lib
            advapi32.lib
            comctl32.lib
            Dwmapi.lib
    )

    # Suppress deprecated GetVersionEx warnings in third-party SQLite only
    set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/../third_party/sqlite/sqlite3.c PROPERTIES
        COMPILE_FLAGS "/wd4996"
    )
endif()

# Register the test executable with CTest
add_test(NAME NetworkMonitorTests_All COMMAND ${PROJECT_NAME})
