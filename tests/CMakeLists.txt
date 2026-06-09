# CMake configuration for NetPulse unit tests

include(${CMAKE_SOURCE_DIR}/cmake/NetPulseDefaults.cmake)

set(NETPULSE_TEST_SOURCES
    main_tests.cpp
    TestUtils.cpp
    history_logger_tests.cpp
    network_monitor_tests.cpp
    utils_tests.cpp
    network_calculator_tests.cpp
    config_manager_tests.cpp
    ui_tests.cpp
    vpn_detection_tests.cpp
    speed_test_tests.cpp
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
    ../src/core/VpnProxyDetector.cpp
    ../src/core/SpeedTester.cpp
    ../src/core/SpeedTestHistory.cpp
    ../src/ui/dialogs/SpeedTestDialog.cpp
    ../src/ui/dialogs/ConnectionLogDialog.cpp
    ../src/core/ConnectionMonitor.cpp
    ../src/ui/ChartRenderer.cpp
    ../third_party/sqlite/sqlite3.c
    ../resources/app.rc
)

add_executable(NetPulseTests ${NETPULSE_TEST_SOURCES})

set_target_properties(NetPulseTests PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

target_include_directories(NetPulseTests PRIVATE ${NETPULSE_INCLUDE_DIRECTORIES})
netpulse_apply_common_defaults(NetPulseTests)

if(MSVC)
    set_target_properties(NetPulseTests PROPERTIES
        LINK_FLAGS "/SUBSYSTEM:CONSOLE"
    )
endif()

add_test(NAME NetPulseTests_All COMMAND NetPulseTests)
