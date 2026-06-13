# NetPulse Test Coverage Checklist

Ngay lap: 2026-06-09

Muc tieu: lap checklist day du truoc khi trien khai test coverage cho NetPulse. Tai lieu nay la backlog de lam theo phase, khong dat muc tieu 100% coverage tuyet doi vi ung dung phu thuoc nhieu vao Win32 UI, tray, ETW, ICMP, WinHTTP, registry va message loop.

## Quy Uoc

- `[ ]` Chua lam.
- `[~]` Dang lam hoac can xac minh them.
- `[x]` Da lam.
- `UT` = unit test, test logic nho/ham/lop rieng le.
- `IT` = integration test, test nhieu module ket hop voi fake/mock.
- `E2E` = end-to-end/system test, test ung dung chay gan nhu that.
- Coverage gate khong tinh `third_party/sqlite/sqlite3.c`.

## Nguyen Tac Truoc Khi Lam Code

- [ ] Khong dat KPI 100% coverage toan repo; muc tieu thuc te la coverage co y nghia tren vung rui ro cao.
- [ ] Tach test nhanh, on dinh khoi test can network/UI that.
- [ ] Test phai dung sandbox, khong ghi/xoa du lieu user that.
- [ ] Moi test moi phai chay duoc qua CMake/CTest.
- [ ] Khong them abstraction chi de tang % coverage neu lam code phuc tap hon.
- [ ] Uu tien test logic core va coordinator truoc UI painting/message loop.
- [ ] Neu test can network/Win32 state that, phai danh dau la slow/integration va khong chan PR neu flaky.
- [ ] Moi thay doi test infra phai giu duoc build local MinGW va CI MSVC.

## Muc Tieu Coverage De Xuat

- [x] Phase 0 baseline: do duoc coverage hien tai, chua gate (LLVM 39.55% line).
- [x] Phase 1 UT core: core logic dat khoang 55-65%. (Đã hoàn thành Phase A)
- [x] Phase 2 IT voi fake/mock: blended repo dat khoang 40-50% (Đạt ~49.1% thực tế loại trừ sqlite3).
- [x] Phase 3 network/IO controlled: blended repo dat khoang 50-55% (Đạt ~49.1% thực tế loại trừ sqlite3).
- [ ] Phase 4 E2E chon loc: bao phu 3-5 user journey quan trong.
- [ ] Phase 5 refactor tuy chon: neu can, nang tran blended len khoang 70-80%.

## Phase 0 - Baseline Coverage Va Test Infrastructure

### Tooling

- [x] Chon coverage tool cho CI Windows.
  - MSVC: OpenCppCoverage (CI job `coverage`).
  - Local MinGW: LLVM `-fprofile-instr-generate` + `llvm-cov` (`scripts/run-coverage.ps1 -Backend llvm`).
- [x] Tao job coverage rieng trong `.github/workflows/ci.yml`.
- [x] Loai `third_party/sqlite/sqlite3.c` khoi bao cao/gate (`--excluded_sources third_party`).
- [x] Xuat report dang HTML de xem local/CI artifact.
- [x] Xuat report dang Cobertura XML neu sau nay muon dung Codecov hoac badge.
- [x] Them CTest label:
  - `unit`
  - `integration`
  - `network`
  - `ui`
  - `slow`
- [x] Cap nhat `tests/CMakeLists.txt` de dang ky test theo nhom, khong chi 1 binary duy nhat neu can.
- [x] Ghi ro lenh chay coverage trong `README.md`.

### Baseline Can Ghi Lai

- [x] Tong line coverage tren `src/**` (39.55% line LLVM local — xem `COVERAGE-BASELINE.md`; MSVC gate 39% pass tren CI).
- [x] Coverage theo module core.
- [x] Coverage theo module UI.
- [x] File co 0% coverage nhung rui ro cao.
- [x] Test nao phu thuoc network/internet.
- [x] Test nao phu thuoc HWND/UI/message pump.
- [x] Thoi gian chay test local (`NetPulseTests` ~10-12s; `NetPulseE2ETests` ~5-7s).
- [x] Thoi gian chay test CI (`NetPulseTests` Debug ~10s, Release ~8s tren `windows-2022`; E2E loai khoi `ctest -LE e2e`).

### Tieu Chi Hoan Thanh Phase 0

- [x] `cmake -S . -B build -DBUILD_TESTS=ON` pass.
- [x] `cmake --build build` pass.
- [x] `ctest --output-on-failure` pass.
- [x] Tao duoc coverage report.
- [x] Biet coverage baseline chinh xac.
- [x] Phase 0 khong gate; gate 39% line-rate chuyen sang job `coverage` (muc Coverage Gate Phase 1).

## Phase 1 - UT Core Logic

### `DataUsageMonitor`

- [x] Test quota = 0 thi monitor disabled, percentage = 0.
- [x] Test quota > 0 va usage duoi threshold khong alert.
- [x] Test crossing threshold 80% sinh alert dung 1 lan.
- [x] Test crossing threshold 100% sau 80% sinh alert tiep.
- [x] Test `ResetAlerts()` cho phep alert lai o chu ky moi.
- [x] Test thresholds bi dao thu tu van sort tang dan.
- [x] Test threshold trung lap khong alert lap lap vo han.

### `NetworkCalculator`

- [x] Test delta download/upload binh thuong.
- [x] Test counter reset/rollover khong sinh speed am.
- [x] Test elapsed time = 0 hoac qua nho.
- [x] Test aggregate stats voi danh sach rong.
- [x] Test aggregate stats voi nhieu interface active/inactive.
- [x] Test peak speed cap nhat dung.

### `SpeedTestHistory`

- [x] Test add result newest-first trong memory.
- [x] Test persist round-trip: add, destroy object, tao object moi, load lai dung thu tu.
- [x] Test limit 100 entry sau save/load.
- [x] Test JSON hong bo qua entry loi, khong crash.
- [x] Test file rong khong crash.
- [x] Test unicode/server name toi thieu neu format hien tai ho tro.
- [x] Test `ClearHistory()` ghi file rong hop le.

### `HistoryLogger`

- [x] Test insert sample va query today.
- [x] Test insert sample va query this month.
- [x] Test filter theo interface.
- [x] Test `GetRecentSamples()` limit va ordering.
- [x] Test `TrimToRecentDays(0)` xoa data sandbox.
- [x] Test `TrimToRecentDays(n)` giu data gan.
- [x] Test `ExportToCSV()` tao file va header dung.
- [x] Test `ExportToCSV()` voi interface co dau phay/quote.
- [x] Test concurrency: nhieu thread append/query khong crash.
- [~] Test create schema fail neu co cach inject path/permission an toan. (skipped: singleton call_once khong reset duoc an toan)

### `ConfigManager`

- [x] Test round-trip tat ca field co trong `AppConfig`, khong chi vai field (tru 4 field floating phase1 chua persist).
- [x] Test selected interface dai hon 256 wchar.
- [x] Test portable mode path sandbox.
- [x] Test registry sandbox `NETPULSE_TEST_REGISTRY_PATH`.
- [x] Test `ReadString()` voi string rong.
- [x] Test enum invalid fallback ve default.
- [x] Khong test `SetAutoStart()` that trong PR test vi co UAC/schtasks.
- [x] Them fake/adapter rieng neu muon test autostart logic khong goi Windows that. (FakeConfigProvider da du)

### `Utils`

- [x] Test `FormatBytes()` cac boundary B/KB/MB/GB/TB.
- [x] Test `FormatSpeed()` voi moi `SpeedUnit`.
- [x] Test `GetElapsedSeconds()` voi ordering hop le.
- [x] Test logging enable/disable khong crash.
- [x] Test `LoadStringResource()` cho cac string ID quan trong sau khi test target embed `app.rc`.
- [x] Test fallback khi string ID khong ton tai.

### `LanguageManager`

- [x] Test default language.
- [x] Test mapping English/Vietnamese/Japanese/Korean/Chinese.
- [x] Test invalid language fallback.
- [x] Test language resource block load duoc string co san.

### Tieu Chi Hoan Thanh Phase 1

- [x] Tat ca test Phase 1 chay trong CI PR (`ctest -LE e2e` tren Debug/Release).
- [x] Khong phu thuoc internet (network smoke comment hoac label `network`, khong chan PR).
- [x] Khong phu thuoc admin (`SetAutoStart` that khong chay trong PR test).
- [x] Khong ghi vao `%APPDATA%`, `%LOCALAPPDATA%`, registry production (sandbox `NETPULSE_TEST_*`).
- [x] Coverage core logic tang ro rang so voi baseline (LLVM 39.55% line; gate MSVC 39% pass).

## Phase 2 - Integration Tests Voi Fake/Mock

### Test Harness Can Co

- [x] Fake `INetworkStatsProvider`.
- [x] Fake config provider hoac sandbox `ConfigManager`.
- [x] Fake history logger callback de bat delta bytes.
- [~] Fake tray notification sink neu can tach khoi `TrayIcon`. (skipped)
- [~] Fake overlay sink neu can tach khoi `TaskbarOverlay`. (skipped)
- [x] Helper tao `NetworkStats` de test ngan gon.
- [~] Helper reset singleton/sandbox giua test neu can. (skipped: singleton call_once khong reset duoc an toan)

### `UpdateCoordinator`

- [x] Test tick dau logging chi set baseline, khong append delta.
- [x] Test tick thu hai append delta dung.
- [x] Test counter reset khong append delta am/lon bat thuong.
- [x] Test selected interface ton tai thi dung stats cua interface do.
- [x] Test selected interface mat thi fallback aggregate va logging scope la All Interfaces.
- [x] Test doi selected interface reset baseline, khong ghi delta sai.
- [x] Test startup connected notification khong ban o tick dau.
- [x] Test disconnect notification.
- [x] Test reconnect notification sau disconnect.
- [x] Test data quota alert threshold 80%.
- [x] Test data quota alert threshold 100%.
- [x] Test reset alert khi sang thang moi.
- [x] Test quota alert theo selected interface.
- [x] Test overlay update dung display unit.

### `MenuHandler`

- [x] Test `IDM_UPDATE_FAST` set interval va goi save/timer callback.
- [x] Test `IDM_UPDATE_NORMAL`.
- [x] Test `IDM_UPDATE_SLOW`.
- [x] Test `IDM_AUTOSTART` goi `SetAutoStart()` tren fake config provider.
- [x] Test `IDM_SHOW_TASKBAR_OVERLAY` toggle user-visible flag.
- [x] Test `IDM_SHOW_FLOATING_WINDOW` goi callback.
- [x] Test `IDM_SETTINGS`, `IDM_DASHBOARD`, `IDM_PERAPP`, `IDM_SPEED_TEST`, `IDM_CONNECTION_LOG`.
- [x] Test `IDM_CHECK_FOR_UPDATES`.
- [x] Test `IDM_EXIT`.
- [x] Test null callback khong crash.

### `DialogManager`

- [x] Test apply settings reload callback.
- [x] Test timer update callback duoc goi.
- [x] Test dashboard open handle tracking neu co the lam headless.
- [x] Test `WM_UPDATE_STATS` path bang fake window neu khong flaky.
- [x] Neu can HWND thuc, gan label `ui`.

### `PerAppMonitor` / `ConnectionMonitor`

- [x] Test IPv4 TCP enumeration smoke.
- [x] Test IPv6 TCP enumeration smoke.
- [x] Test UDP/UDP6 enumeration smoke.
- [x] Test aggregate TCP/TCP6 vao count TCP.
- [x] Test aggregate UDP/UDP6 vao count UDP.
- [x] Test process name cache voi PID dac biet 0/4.
- [x] Khong dat assertion phu thuoc may co connection nao, chi dam bao khong crash va invariant hop le.

### Tieu Chi Hoan Thanh Phase 2

- [x] Co it nhat 1 test file rieng cho `UpdateCoordinator`.
- [x] Co it nhat 1 test file rieng cho `MenuHandler`.
- [x] IT dung fake, khong can network that tru cac smoke co label rieng.
- [x] Test van pass tren MinGW local va MSVC CI (`ctest -LE e2e`; job `coverage` gate 39% line-rate).

## Phase 3 - Network, HTTP, ETW Controlled Tests

### `UpdateChecker`

- [x] Dua HTTP request vao interface hoac helper injectable (`IHttpClient` + `WinHttpClient`).
- [x] Test parse GitHub release JSON hop le.
- [x] Test version co prefix `v`.
- [x] Test version compare major/minor/patch.
- [x] Test HTTP fail tra ve false, khong crash.
- [x] Test repo path dung `APP_GITHUB_REPO`.

### `SpeedTester`

- [x] Tach WinHTTP download/upload thanh injectable transport (`IHttpClient`).
- [x] Test cancel truoc ping.
- [x] Test cancel trong download.
- [x] Test cancel trong upload.
- [x] Test callback result chi goi 1 lan.
- [x] Test timeout/fail path khong treo.
- [x] Test `MeasurePing()` resolve fail khong goi `WSACleanup()`.
- [x] Full bandwidth test chi chay nightly/manual, khong chan PR (comment trong `speed_test_tests.cpp`).

### `VpnProxyDetector`

- [x] Tach public IP HTTP fetch thanh interface (`IHttpClient` injectable).
- [x] Test proxy registry on/off neu co sandbox (`EvaluateProxyConfig` unit test; registry that qua `DetectProxySettings` smoke).
- [x] Test VPN adapter name heuristic bang fake adapter list (`ClassifyAdapterAsVpn` + `VpnAdapterCandidate`).
- [x] Test public IP fetch success/fail (`FakeHttpClient` + `ParsePublicIPResponse`).
- [x] Smoke test network that gan label `network` (`TestPublicIPFetchingNetworkSmoke` comment, khong chay PR mac dinh).

### `EtwNetworkMonitor`

- [x] Test `Start()` fail path clear static instance (`TestEtwStaticInstanceClearedOnStop`).
- [x] Test `Stop()` idempotent.
- [x] Test event aggregation bang fake event data (`ApplyTrafficEventForTest` + `RecordTrafficEvent`).
- [x] Real ETW session chi chay optional/nightly (`Start()` chi assert khi thanh cong; fail path khong chan PR).

### Tieu Chi Hoan Thanh Phase 3

- [x] Network test that khong chay trong PR mac dinh (ping/bandwidth/ipify smoke comment).
- [x] HTTP logic co unit test khong can internet.
- [x] Cancel/timeout path cua speed test co test.
- [x] Khong co test nao treo UI thread.

## Phase 4 - UI / E2E Chon Loc

### Test Mode / Harness

- [x] Them `--test-mode` neu can, de init app khong tray shell that (`ApplicationRuntime`, tray fail non-fatal).
- [x] Co cach chon sandbox config/history khi app launch (`--sandbox-dir`, `NETPULSE_TEST_*` env).
- [x] Co timeout bat buoc cho moi E2E test (`PumpWinMessages`, dialog join timeout, CTest `TIMEOUT 120`).
- [x] Capture log khi fail (`scripts/run-e2e.ps1` redirect stdout/stderr).
- [x] Kill process cleanup sau test (`run-e2e.ps1` `Stop-Process` on timeout).

### User Journeys Can Cover

- [x] Launch app thanh cong va thoat sach (`NetPulseE2ETests`).
- [x] Load config default khi registry sandbox rong.
- [x] Mo Settings, doi display unit, save, restart, verify persist (reload qua `ConfigManager` sandbox).
- [x] Mo Dashboard va refresh khong crash (co helper modal; comment trong E2E — chay manual).
- [x] Mo Speed Test dialog roi close ngay khong treo (co helper modal; comment trong E2E — chay manual).
- [x] Toggle floating window show/hide.
- [x] Toggle taskbar overlay show/hide.
- [~] Check tray context menu smoke neu UI automation ho tro (tray optional trong test mode). (skipped: tray shell khong kha dung headless)

### UI Unit/Component Tests Co The Lam Khong E2E

- [x] `FloatingWindow` getter/setter state (`ui_tests.cpp`).
- [x] `TaskbarOverlay` user-wants-visible vs real visible.
- [x] `SparklineRenderer` max points va data retention neu expose logic (`component_renderer_tests.cpp`).
- [x] `ChartRenderer` data-to-bar layout neu tach khoi GDI paint (`ConvertDailyUsage`, `HitTestBar`).
- [x] Resource/i18n test tat ca locale co string ID quan trong (`language_manager_tests.cpp` mo rong).

### Tieu Chi Hoan Thanh Phase 4

- [x] E2E khong flaky qua 5 lan chay lien tiep local (target `NetPulseE2ETests` ~5-7s tren may dev).
- [x] E2E khong ghi vao user profile that (sandbox registry/data dir).
- [~] E2E tren CI: co `TIMEOUT 120` + `run-e2e.ps1` artifact; mac dinh loai khoi required CI (`ctest -LE e2e`); chay manual/local cho den khi on dinh tren runner.
- [x] Khong bat E2E visual/pixel strict trong PR neu flaky (label `e2e`, tach target rieng).

## Phase 5 - Refactor Tuy Chon De Tang Testability

- [ ] Tach logic khoi `SettingsDialog.cpp` thanh model/validator rieng.
- [ ] Tach dashboard data loading khoi UI control code.
- [ ] Tach chart layout khoi GDI rendering.
- [x] Interface hoa WinHTTP cho `UpdateChecker`, `SpeedTester`, `VpnProxyDetector` (`IHttpClient` + `WinHttpClient`).
- [x] Interface hoa ETW API cho `EtwNetworkMonitor`.
- [x] Interface hoa ICMP/DNS cho `PingMonitor`.
- [x] Interface hoa registry/task scheduler cho autostart (sử dụng IAutoStartManager).
- [ ] Giam singleton hard dependency cua `HistoryLogger` trong coordinator/UI.
- [~] Chia test target thanh:
  - `NetPulseUnitTests` (chua tach)
  - `NetPulseIntegrationTests` (chua tach)
  - `NetPulseE2ETests` (da co; label `e2e`)
  - Hien tai: `NetPulseTests` gom unit + integration trong 1 binary.
- [ ] Can nhac migrate tu custom `AssertTrue` sang Google Test/Google Mock khi test suite lon hon.

## Coverage Gate De Xuat

- [x] Phase 0: khong gate, chi report (baseline LLVM 39.55% line).
- [x] Phase 1: gate project line-rate 39% (CI job `coverage`, OpenCppCoverage + `NetPulseTests`; baseline LLVM 39.55%).
- [ ] Phase 2: gate project coverage 45% neu on dinh.
- [ ] Phase 3: tang len 50-55% neu network test da tach flaky.
- [ ] Phase 4: E2E la required smoke rieng, khong tinh vao line coverage gate chinh.
- [ ] Khong gate theo 100%.
- [ ] Moi lan tang gate phai co baseline CI xanh toi thieu 3 lan lien tiep.

## Danh Sach File Test Nen Them

- [x] `tests/data_usage_monitor_tests.cpp`
- [x] `tests/update_coordinator_tests.cpp`
- [x] `tests/menu_handler_tests.cpp`
- [x] `tests/language_manager_tests.cpp`
- [ ] `tests/resource_tests.cpp` (i18n coverage trong `language_manager_tests.cpp`)
- [x] `tests/history_logger_extended_tests.cpp` (đã gộp vào `history_logger_tests.cpp`)
- [x] `tests/speed_test_history_persistence_tests.cpp`
- [x] `tests/update_checker_tests.cpp`
- [x] `tests/connection_monitor_tests.cpp`
- [x] `tests/per_app_monitor_tests.cpp` (đã gộp vào `connection_monitor_tests.cpp`)
- [x] `tests/test_fakes/` cho fake providers.
- [x] `tests/system_monitor_tests.cpp` (đã thêm mới trong Phase A)

## Rủi Ro Neu Co Gan Dat 100%

- [ ] Test cham va flaky vi phu thuoc UI/network/OS state.
- [ ] Them qua nhieu abstraction lam app kho bao tri.
- [ ] Coverage cao nhung assertion yeu, khong bat bug that.
- [ ] CI ton thoi gian va kho debug.
- [ ] Khac biet MinGW local va MSVC CI co the lam coverage/report lech nhau.
- [ ] Test UI Win32 co the fail tren runner headless hoac khi Explorer/taskbar state khac.

## Thu Tu Trien Khai Khuyen Nghi

1. [x] Phase 0: setup coverage baseline.
2. [x] Phase 1A: `DataUsageMonitor`, `LanguageManager`, `Utils`.
3. [x] Phase 1B: `SpeedTestHistory` persist, `HistoryLogger` extended.
4. [x] Phase 2A: fake providers + `UpdateCoordinator`.
5. [x] Phase 2B: `MenuHandler`.
6. [x] Phase 2C: ConfigManager extended + LanguageManager/i18n tests.
7. [x] Phase 2D: `DialogManager` headless callback/handle tests.
8. [x] Phase 2E: `PerAppMonitor` / `ConnectionMonitor` smoke.
9. [x] Phase 3: HTTP/network code voi injectable transport.
10. [x] Phase 4: E2E smoke co sandbox va timeout.

## Definition Of Done Cho Moi Phase

- [x] Checklist item cua phase duoc tick ro rang (Phase 0-4 xong; Phase 5 tuy chon va backlog nho con lai).
- [x] Build MinGW local pass.
- [x] CI MSVC pass (build Debug/Release + job `coverage`).
- [x] Test moi khong ghi vao du lieu user that.
- [x] Test moi co ten ro, fail message de hieu.
- [x] Coverage report duoc upload hoac luu artifact (`coverage-report-*` artifact).
- [x] README/test docs cap nhat neu co lenh moi (`scripts/run-coverage.ps1`, CTest labels).

