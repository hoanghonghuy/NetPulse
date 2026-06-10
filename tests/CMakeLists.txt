# CMake configuration for NetPulse unit tests

include(${CMAKE_SOURCE_DIR}/cmake/NetPulseDefaults.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/NetPulseCoverage.cmake)

set(NETPULSE_TEST_SOURCES
    main_tests.cpp
    TestUtils.cpp
    system_monitor_tests.cpp
    history_logger_tests.cpp
    network_monitor_tests.cpp
    utils_tests.cpp
    network_calculator_tests.cpp
    config_manager_tests.cpp
    ui_tests.cpp
    vpn_detection_tests.cpp
    speed_test_tests.cpp
    data_usage_monitor_tests.cpp
    update_coordinator_tests.cpp
    menu_handler_tests.cpp
    language_manager_tests.cpp
    speed_test_history_persistence_tests.cpp
    dialog_manager_tests.cpp
    connection_monitor_tests.cpp
    update_checker_tests.cpp
    etw_network_monitor_tests.cpp
    component_renderer_tests.cpp
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
    ../src/core/UpdateChecker.cpp
    ../src/core/WinHttpClient.cpp
    ../src/core/SpeedTester.cpp
    ../src/core/SpeedTestHistory.cpp
    ../src/ui/dialogs/SpeedTestDialog.cpp
    ../src/ui/dialogs/ConnectionLogDialog.cpp
    ../src/core/ConnectionMonitor.cpp
    ../src/ui/ChartRenderer.cpp
    ../third_party/sqlite/sqlite3.c
    ${NETPULSE_APP_RC}
)

add_executable(NetPulseTests ${NETPULSE_TEST_SOURCES})

set_target_properties(NetPulseTests PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

target_include_directories(NetPulseTests PRIVATE
    ${NETPULSE_INCLUDE_DIRECTORIES}
    ${CMAKE_CURRENT_SOURCE_DIR}
)
netpulse_apply_common_defaults(NetPulseTests)
netpulse_add_resource_dependencies(NetPulseTests)
netpulse_apply_test_coverage(NetPulseTests)

if(MSVC)
    set_target_properties(NetPulseTests PROPERTIES
        LINK_FLAGS "/SUBSYSTEM:CONSOLE"
    )
endif()

add_test(NAME NetPulseTests_All COMMAND NetPulseTests)
set_tests_properties(NetPulseTests_All PROPERTIES
    LABELS "unit;integration;ui;network;slow"
)

set(NETPULSE_E2E_SOURCES
    application_e2e_tests.cpp
    TestUtils.cpp
    ../src/core/Application.cpp
    ../src/core/ApplicationRuntime.cpp
    ../src/core/HotkeyManager.cpp
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
    ../src/ui/ChartRenderer.cpp
    ../src/core/VpnProxyDetector.cpp
    ../src/core/UpdateChecker.cpp
    ../src/core/WinHttpClient.cpp
    ../src/core/SpeedTester.cpp
    ../src/core/SpeedTestHistory.cpp
    ../src/ui/dialogs/SpeedTestDialog.cpp
    ../src/ui/dialogs/ConnectionLogDialog.cpp
    ../src/core/ConnectionMonitor.cpp
    ../third_party/sqlite/sqlite3.c
    ${NETPULSE_APP_RC}
)

add_executable(NetPulseE2ETests ${NETPULSE_E2E_SOURCES})

set_target_properties(NetPulseE2ETests PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

target_include_directories(NetPulseE2ETests PRIVATE
    ${NETPULSE_INCLUDE_DIRECTORIES}
    ${CMAKE_CURRENT_SOURCE_DIR}
)
netpulse_apply_common_defaults(NetPulseE2ETests)
netpulse_add_resource_dependencies(NetPulseE2ETests)

if(MSVC)
    set_target_properties(NetPulseE2ETests PROPERTIES
        LINK_FLAGS "/SUBSYSTEM:CONSOLE"
    )
endif()

add_test(NAME NetPulseE2ETests COMMAND NetPulseE2ETests)
set_tests_properties(NetPulseE2ETests PROPERTIES
    LABELS "e2e;slow;ui"
    TIMEOUT 120
)
