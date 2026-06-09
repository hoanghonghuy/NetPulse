# NetPulse Code Issues Checklist

Ngày rà soát: 2026-06-09

Mục tiêu: tổng hợp các issue đã được xác nhận khi đọc luồng core, UI, build/test/resource của NetPulse. Danh sách này ưu tiên issue có bằng chứng từ source và có tác động thực tế.

## Quy Ước

- `[ ]` Chưa xử lý.
- `[~]` Đang xử lý hoặc cần xác minh thêm bằng build/runtime.
- `[x]` Đã xử lý.
- `Critical/High/Medium/Low` là mức ưu tiên sửa, không phải kết luận bảo mật.

## Critical

- [ ] **Sai include path `resource.h` làm build fail**
  - File: `src/ui/dialogs/SpeedTestDialog.cpp`, `src/ui/dialogs/ConnectionLogDialog.cpp`
  - Bằng chứng: hai file dialog nằm trong `src/ui/dialogs/` nhưng include `../resources/resource.h` hoặc `../../resources/resource.h`; pattern đúng trong các dialog khác là `../../../resources/resource.h`.
  - Tác động: CMake/VS build có thể fail với lỗi không tìm thấy `resource.h`.
  - Hướng sửa: đổi include về đường dẫn tương đối đúng hoặc thống nhất include qua include directory.

## High

- [ ] **`HistoryLogger` thiếu thread-safety quanh SQLite handle**
  - File: `src/core/HistoryLogger.cpp`, `include/NetPulse/HistoryLogger.h`
  - Bằng chứng: nhiều API dùng chung `m_db` nhưng class không có mutex bảo vệ; app có timer logging và UI/dashboard query cùng lúc.
  - Tác động: có thể gặp lỗi SQLite race, `SQLITE_BUSY`, dữ liệu lệch hoặc crash hiếm.
  - Hướng sửa: thêm mutex cho toàn bộ thao tác dùng `m_db`, hoặc mở connection theo thread/operation.

- [ ] **`HistoryLogger::InitializeSQLite()` vẫn đánh dấu SQLite available khi create schema fail**
  - File: `src/core/HistoryLogger.cpp`
  - Bằng chứng: `sqlite3_exec(create table)` fail chỉ log lỗi, sau đó vẫn set `m_sqliteAvailable = true`.
  - Tác động: app nghĩ DB dùng được dù schema không được tạo; insert/query fail về sau.
  - Hướng sửa: nếu create schema fail thì close DB, set unavailable và return.

- [ ] **Tray menu “Start with Windows” không bật/tắt autostart thật**
  - File: `src/core/MenuHandler.cpp`, `src/core/ConfigManager.cpp`, `src/ui/dialogs/SettingsDialog.cpp`
  - Bằng chứng: menu chỉ flip `m_pConfig->autoStart` rồi `SaveConfig()`; `SaveConfig()` cố ý không gọi `SetAutoStart()`. Settings dialog mới gọi `SetAutoStart()` đúng.
  - Tác động: trạng thái checkbox menu đổi trong session nhưng Windows startup không đổi.
  - Hướng sửa: khi xử lý `IDM_AUTOSTART`, gọi `ConfigManager::SetAutoStart()` hoặc chuyển action qua callback riêng giống Settings.

- [ ] **Logging sai delta/interface khi đổi hoặc fallback selected interface**
  - File: `src/core/UpdateCoordinator.cpp`, `src/core/Application.cpp`
  - Bằng chứng: `GetCurrentStats()` fallback sang aggregate nếu selected interface không tồn tại, nhưng callback logging vẫn dùng `m_config.selectedInterface`; `m_prevTotalBytesDown/Up` không reset khi đổi scope stats.
  - Tác động: có thể ghi delta sai lớn hoặc gán traffic aggregate vào tên interface sai.
  - Hướng sửa: trả về stats kèm interface thực tế đã dùng, reset baseline khi selected interface đổi/fallback đổi.

- [ ] **CMake/MSVC không embed full `app.manifest`**
  - File: `CMakeLists.txt`, `resources/app.rc`, `resources/app.manifest`, `src/entry/main.cpp`
  - Bằng chứng: `app.rc` chỉ embed manifest ngoài MSVC; CMake chỉ đưa `app.manifest` vào source list nhưng không đảm bảo embed như `.vcxproj`.
  - Tác động: binary CMake có thể khác binary VS về compatibility, UAC/trustInfo hoặc DPI behavior.
  - Hướng sửa: cấu hình CMake/MSVC embed manifest đầy đủ hoặc để `app.rc` embed manifest thống nhất.

- [ ] **Version resource lệch với app version**
  - File: `CMakeLists.txt`, `include/NetPulse/Common.h`, `resources/app.rc`, `CHANGELOG.md`
  - Bằng chứng: runtime/CMake/CHANGELOG là `2.3.0`, nhưng `VS_VERSION_INFO` trong `app.rc` còn `2.0.0.0`.
  - Tác động: Explorer properties, release artifact và update metadata lệch nhau.
  - Hướng sửa: đồng bộ version resource với `APP_VERSION` hoặc sinh version từ một nguồn duy nhất.

## Medium

- [ ] **Update checker gọi sai GitHub repo**
  - File: `src/core/UpdateChecker.cpp`, `include/NetPulse/Common.h`, `.git/config`
  - Bằng chứng: app/remote/README dùng `hoanghonghuy/NetPulse`, nhưng update checker gọi `/repos/hoanghonghuy/NetworkMonitor/releases/latest`.
  - Tác động: check update fail hoặc kiểm tra nhầm release.
  - Hướng sửa: dùng `APP_GITHUB_LINK` hoặc endpoint đúng `hoanghonghuy/NetPulse`.

- [ ] **Đóng Speed Test khi đang chạy có thể block UI**
  - File: `src/ui/dialogs/SpeedTestDialog.cpp`, `src/core/SpeedTester.cpp`
  - Bằng chứng: `OnClose()` chỉ `CancelTest()` rồi `EndDialog()`; destructor của `SpeedTester` join thread, trong khi WinHTTP có thể đang block.
  - Tác động: dialog/app có thể đơ khi đóng lúc request mạng chưa trả về.
  - Hướng sửa: thiết kế cancel bất đồng bộ an toàn, set timeout rõ ràng, không join blocking trên UI thread.

- [ ] **Speed Test callback post tới HWND đã hủy và có thể leak**
  - File: `src/ui/dialogs/SpeedTestDialog.cpp`, `src/core/SpeedTester.cpp`
  - Bằng chứng: callback allocate `new SpeedTestResult` rồi `PostMessageW(hDlg, ...)`; nếu dialog đã đóng, message không được xử lý và object không được delete.
  - Tác động: leak memory và cập nhật UI sai sau cancel/close.
  - Hướng sửa: kiểm tra `IsWindow(hDlg)` trước khi post, hoặc dùng ownership an toàn và clear callback khi close.

- [ ] **`SpeedTestHistory::LoadFromFile()` có thể crash khi JSON hỏng**
  - File: `src/core/SpeedTestHistory.cpp`
  - Bằng chứng: dùng `std::stod`, `std::stoi`, `std::stoll` trực tiếp không `try/catch`.
  - Tác động: file history do user sửa/hỏng có thể làm mở Speed Test dialog crash.
  - Hướng sửa: bắt exception theo từng entry, bỏ qua entry lỗi và log debug/error.

- [ ] **Thứ tự Speed Test history sai sau restart**
  - File: `src/core/SpeedTestHistory.cpp`, `tests/speed_test_tests.cpp`
  - Bằng chứng: `AddResult()` insert newest ở đầu vector, nhưng `LoadFromFile()` push_back theo thứ tự file; test ordering chỉ kiểm tra in-memory.
  - Tác động: UI “newest first” có thể sai sau khi load lại file.
  - Hướng sửa: giữ file newest-first và load tương ứng, hoặc sort theo timestamp sau load.

- [ ] **Data usage alert không reset khi sang tháng mới**
  - File: `src/core/DataUsageMonitor.cpp`, `src/core/UpdateCoordinator.cpp`
  - Bằng chứng: `ResetAlerts()` tồn tại nhưng không có caller.
  - Tác động: nếu app chạy qua tháng mới, alert 80%/100% đã báo tháng trước có thể không báo lại.
  - Hướng sửa: track billing month/current month trong coordinator và gọi `ResetAlerts()` khi tháng đổi.

- [ ] **Quota/data usage alert tính toàn bộ interface, không theo selected interface**
  - File: `src/core/UpdateCoordinator.cpp`, `src/core/HistoryLogger.cpp`
  - Bằng chứng: `GetThisMonthTotalBytes()` được gọi không truyền `interfaceFilter`, dù HistoryLogger hỗ trợ filter.
  - Tác động: số quota alert có thể khác dashboard khi user chọn một interface cụ thể.
  - Hướng sửa: truyền selected interface khi phù hợp hoặc ghi rõ quota là tổng mọi interface.

- [ ] **Floating window luôn dùng aggregate stats, không theo selected interface**
  - File: `src/core/Application.cpp`, `src/core/UpdateCoordinator.cpp`
  - Bằng chứng: update floating window gọi trực tiếp `m_pNetworkMonitor->GetAggregatedStats()` thay vì dùng cùng logic `GetCurrentStats()`.
  - Tác động: widget tốc độ không khớp tray/overlay khi user chọn interface riêng.
  - Hướng sửa: expose helper stats selection hoặc để `UpdateCoordinator` cung cấp stats hiện tại cho floating window.

- [ ] **Connection notification hiện “Network Connected” ngay tick đầu**
  - File: `src/core/UpdateCoordinator.cpp`
  - Bằng chứng: `m_wasConnected` khởi tạo `false`; tick đầu có network active đi vào nhánh reconnected.
  - Tác động: app có thể bắn notification connected mỗi lần khởi động.
  - Hướng sửa: thêm flag baseline initialized; tick đầu chỉ set trạng thái ban đầu.

- [ ] **Right-click taskbar overlay có thể không nhận click**
  - File: `src/ui/TaskbarOverlay.cpp`, `src/core/Application.cpp`
  - Bằng chứng: overlay dùng `WS_EX_TRANSPARENT`, trong khi có handler `WM_RBUTTONUP` và callback mở tray menu.
  - Tác động: right-click overlay không mở menu như kỳ vọng.
  - Hướng sửa: điều chỉnh hit-test/style; chỉ bật click-through khi người dùng chọn chế độ click-through.

- [ ] **`TaskbarOverlay::IsVisible()` không phản ánh visibility thực tế**
  - File: `src/ui/TaskbarOverlay.cpp`, `include/NetPulse/TaskbarOverlay.h`
  - Bằng chứng: fullscreen path `ShowWindow(SW_HIDE)` nhưng giữ `m_isVisible == true`.
  - Tác động: checkmark menu/hotkey có thể lệch trạng thái thật của window.
  - Hướng sửa: tách “user wants visible” và “currently visible”, hoặc `IsVisible()` dùng trạng thái window thật khi cần.

- [ ] **`WM_UPDATE_STATS` gửi tới Dashboard nhưng Dashboard không xử lý**
  - File: `src/ui/DialogManager.cpp`, `src/ui/dialogs/DashboardDialog.cpp`, `include/NetPulse/Common.h`
  - Bằng chứng: `DialogManager` gửi `WM_UPDATE_STATS`; `DashboardDialog` không có handler tương ứng.
  - Tác động: Apply settings/theme khi dashboard đang mở không refresh theo ý đồ comment.
  - Hướng sửa: thêm handler hoặc bỏ message dead code và gọi API refresh rõ ràng.

- [ ] **`FloatingWindow::ExportChartAsPNG()` báo thành công sai và thực chất xuất BMP**
  - File: `src/ui/FloatingWindow.cpp`
  - Bằng chứng: TODO nói chưa có PNG; code đổi `.png` sang `.bmp`; cuối hàm `return true` kể cả khi create file fail ở một số nhánh.
  - Tác động: UI/caller có thể báo export PNG thành công nhưng file không đúng định dạng hoặc không được tạo.
  - Hướng sửa: đổi tên API/UI thành BMP hoặc implement PNG thật; return false khi fail.

- [ ] **`EtwNetworkMonitor::Start()` để lại static instance khi start fail**
  - File: `src/core/EtwNetworkMonitor.cpp`
  - Bằng chứng: `s_instance = this` trước `StartTraceW`; fail path không clear.
  - Tác động: callback ETW/static state có thể trỏ tới object chưa chạy hoặc đã bị cleanup.
  - Hướng sửa: chỉ set `s_instance` sau khi start thành công, clear ở mọi fail path.

- [ ] **`SpeedTester::MeasurePing()` gọi `WSACleanup()` trong nhánh resolve fail**
  - File: `src/core/SpeedTester.cpp`
  - Bằng chứng: comment nói Winsock lifecycle do `RunTest()` quản lý, nhưng `MeasurePing()` cleanup khi `GetAddrInfoW` fail.
  - Tác động: có thể cleanup Winsock sớm trong `RunTest()` hoặc cleanup khi caller chưa startup.
  - Hướng sửa: bỏ cleanup khỏi `MeasurePing()`; lifecycle nằm ở owner.

- [ ] **Per-app monitor thiếu IPv6**
  - File: `src/core/PerAppMonitor.cpp`, `src/core/ConnectionMonitor.cpp`
  - Bằng chứng: `PerAppMonitor` chỉ gọi `GetExtendedTcpTable/GetExtendedUdpTable` với `AF_INET`; `ConnectionMonitor` có cả IPv4 và IPv6.
  - Tác động: Per-App dialog thiếu connection IPv6, thống kê lệch.
  - Hướng sửa: thêm path IPv6 giống `ConnectionMonitor`.

- [ ] **Chuỗi i18n tiếng Anh bị thiếu**
  - File: `resources/app.rc`, `src/ui/dialogs/SpeedTestDialog.cpp`, `src/ui/dialogs/SettingsDialog.cpp`
  - Bằng chứng: một số ID như `IDS_SPEED_TEST_DIALOG_TITLE`, `IDS_SPEED_RESULTS`, `IDS_SETTINGS_LABEL_DATA_USAGE_ENABLE` thiếu trong English block hoặc thiếu toàn cục.
  - Tác động: UI tiếng Anh có thể trống hoặc fallback không đúng.
  - Hướng sửa: bổ sung string table đầy đủ cho English và các locale còn thiếu.

- [ ] **Test suite có side effect lên dữ liệu user thật**
  - File: `tests/history_logger_tests.cpp`, `tests/speed_test_tests.cpp`, `tests/config_manager_tests.cpp`
  - Bằng chứng: test gọi `DeleteAll()`, `ClearHistory()`, registry round-trip trên path production `%LOCALAPPDATA%`, `%APPDATA%`, `HKCU\Software\NetworkMonitor`.
  - Tác động: chạy test local có thể xóa/sửa history/config thật của user.
  - Hướng sửa: thêm test mode/path override hoặc abstraction storage để test dùng sandbox.

## Low

- [ ] **Ping target sai bị fallback im lặng sang `8.8.8.8`**
  - File: `src/core/PingMonitor.cpp`
  - Bằng chứng: DNS fail set `m_targetIP = 0x08080808` và return true.
  - Tác động: user nhập target sai nhưng UI vẫn ping Google DNS, che lỗi cấu hình.
  - Hướng sửa: giữ lỗi resolve và hiển thị unavailable, hoặc chỉ fallback khi target trống.

- [ ] **Registry string bị giới hạn 256 wchar**
  - File: `src/core/ConfigManager.cpp`
  - Bằng chứng: `ReadString()` dùng buffer cố định `wchar_t buffer[256]`.
  - Tác động: interface name/path/string dài bị cắt.
  - Hướng sửa: query size trước rồi allocate buffer đúng kích thước.

- [ ] **Tray icon theme ban đầu có thể sai**
  - File: `src/core/Application.cpp`, `src/ui/TrayIcon.cpp`
  - Bằng chứng: `TrayIcon::Initialize()` chạy trước `SetConfigSource(&m_config)`, nên init dùng system theme thay vì config.
  - Tác động: icon idle ban đầu có thể lệch theme cho tới tick refresh.
  - Hướng sửa: set config source trước initialize hoặc truyền config vào `Initialize()`.

- [ ] **`FloatingWindow::SetConfigChangeCallback` đang dead code**
  - File: `src/ui/FloatingWindow.cpp`, `include/NetPulse/FloatingWindow.h`, `src/core/Application.cpp`
  - Bằng chứng: callback được set nhưng không có chỗ gọi.
  - Tác động: code gây hiểu nhầm, không có tác dụng runtime.
  - Hướng sửa: gọi callback tại nơi thay đổi sparkline range hoặc xóa callback nếu không còn dùng.

- [ ] **Resource numeric ID trùng giữa control và string**
  - File: `resources/resource.h`
  - Bằng chứng: `IDC_PORTABLE_MODE_BUTTON` và `IDS_MENU_CHECK_FOR_UPDATES` cùng numeric ID.
  - Tác động: hiện không crash vì resource type khác, nhưng dễ gây nhầm khi bảo trì.
  - Hướng sửa: cấp lại ID không trùng theo range.

- [ ] **CMake/vcxproj/test config lệch nhau**
  - File: `CMakeLists.txt`, `NetPulse.vcxproj`, `tests/CMakeLists.txt`
  - Bằng chứng: CMake bật `/WX`, `_WIN32_WINNT=0x0601`, link lib thừa; vcxproj không đồng bộ; test target không embed resource.
  - Tác động: CI và Visual Studio có thể cho kết quả khác nhau.
  - Hướng sửa: thống nhất compiler definitions, warning policy và resource/link settings.

- [ ] **README chưa hướng dẫn chạy test**
  - File: `README.md`
  - Tác động: contributor khó biết `BUILD_TESTS` và `ctest`.
  - Hướng sửa: thêm mục test ngắn theo workflow CI.

## Khoảng Trống Test Nên Bổ Sung

- [ ] Test `UpdateCoordinator` cho delta logging, đổi selected interface, fallback interface, startup notification, quota alert.
- [ ] Test `DataUsageMonitor` cho threshold crossing, reset theo tháng, quota = 0.
- [ ] Test `HistoryLogger` concurrency, create-table failure, export CSV, daily/monthly aggregation.
- [ ] Test `SpeedTestHistory` persist round-trip: save, tạo object mới, kiểm tra ordering.
- [ ] Test `SpeedTestDialog` async close/cancel, post message tới HWND invalid.
- [ ] Test `TaskbarOverlay` right-click/hit-test, fullscreen hide/restore, visible flag vs window visibility.
- [ ] Test `MenuHandler` cho mọi `IDM_*`, đặc biệt autostart phải gọi operation thật.
- [ ] Test `PerAppMonitor` IPv6 và consistency với `ConnectionMonitor`.
- [ ] Test resource/i18n để bắt string ID thiếu trong English và các locale.
- [ ] Tách test storage khỏi dữ liệu user thật bằng path override hoặc mock provider.

## Validation Hiện Tại

- [~] Chưa chạy được CMake build/test trong phiên này vì `cmake` không có trong PATH.
- [~] Lệnh kiểm tra `msbuild/devenv` bị treo và đã được dừng.
- [ ] Cần chạy lại trên môi trường có Visual Studio Build Tools + CMake:
  - `cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DBUILD_TESTS=ON`
  - `cmake --build build --config Debug`
  - `ctest -C Debug --output-on-failure --verbose`

## Ghi Chú Loại Trừ / Đính Chính

- Không giữ issue “thiếu icon resource” trong danh sách chính vì audit build/test đã xác nhận `resources/icons/*.ico` tồn tại trong repo. Nếu build vẫn lỗi icon, cần kiểm tra lại working tree hoặc file bị ignore/local.
- Các nghi ngờ chưa có bằng chứng runtime rõ ràng, ví dụ schema payload ETW cụ thể, chưa đưa vào checklist chính.
